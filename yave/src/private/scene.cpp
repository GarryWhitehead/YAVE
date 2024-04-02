/* Copyright (c) 2022 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "scene.h"

#include "aabox.h"
#include "camera.h"
#include "colour_pass.h"
#include "engine.h"
#include "frustum.h"
#include "managers/component_manager.h"
#include "managers/renderable_manager.h"
#include "managers/transform_manager.h"
#include "renderable.h"

#include <tbb/tbb.h>
#include <utility/aligned_alloc.h>
#include <utility/assertion.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/pipeline_cache.h>
#include <yave/skybox.h>

#include <algorithm>
#include <cstring>

namespace yave
{

IScene::IScene(IEngine& engine)
    : engine_(engine),
      camera_(nullptr),
      skybox_(nullptr),
      waveGen_(nullptr),
      indirectLight_(nullptr),
      sceneUbo_(std::make_unique<SceneUbo>(engine_.driver())),
      usePostProcessing_(true),
      useGbuffer_(true)
{
    vkapi::VkDriver& driver = engine_.driver();

    transUbo_ = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboDynamicSetValue, 0, "TransformUbo", "mesh_ubo");
    transUbo_->addElement("modelMatrix", backend::BufferElementType::Mat4);
    transUbo_->createGpuBuffer(driver, ModelBufferInitialSize * transUbo_->size());

    skinUbo_ = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboDynamicSetValue, 1, "skinUbo", "skin_ubo");
    skinUbo_->addElement(
        "jointMatrices",
        backend::BufferElementType::Mat4,
        nullptr,
        ITransformManager::MaxBoneCount);
    skinUbo_->addElement("jointCount", backend::BufferElementType::Float);
    skinUbo_->createGpuBuffer(driver, ModelBufferInitialSize * skinUbo_->size());
}

IScene::~IScene() = default;

void IScene::shutDown(vkapi::VkDriver& driver) noexcept { YAVE_UNUSED(driver); }

void IScene::setSkybox(ISkybox* skybox) noexcept
{
    ASSERT_FATAL(camera_, "The camera must be set before declaring the skybox.");
    skybox_ = skybox;
}

void IScene::setIndirectLight(IIndirectLight* il)
{
    indirectLight_ = il;

    ILightManager* lm = engine_.getLightManager();
    lm->enableAmbientLight();
    // TODO: also deal with the indirect light being removed (set to null)
}

void IScene::setCamera(ICamera* cam) noexcept
{
    ASSERT_FATAL(cam, "The camera is nullptr.");
    camera_ = cam;
}

void IScene::setWaveGenerator(IWaveGenerator* waterGen) noexcept
{
    ASSERT_FATAL(waterGen, "Water generator is nullptr");
    waveGen_ = waterGen;
}

bool IScene::update()
{
    ASSERT_FATAL(camera_, "No camera has been set.");

    ILightManager* lm = engine_.getLightManager();
    IRenderableManager* rm = engine_.getRenderableManager();
    IObjectManager* om = engine_.getObjManager();

    if (skybox_)
    {
        skybox_->update(*camera_);
    }

    // clear the render queue
    renderQueue_.resetAll();
    candRenderableObjs_.clear();

    // Prepare the camera frustum
    // Update the camera matrices before constructing the fustrum
    Frustum frustum;
    frustum.projection(camera_->projMatrix() * camera_->viewMatrix());

    // Update the lights since we have not updated the camera for this frame.
    lm->prepare(this);
    lm->update(*camera_);

    // At the moment we iterate through the list of objects
    // and find any that have a renderable or light component. If they are
    // active then these are added as a potential candiate lighting source
    std::vector<LightInstance*> candLightObjs;
    candLightObjs.reserve(objects_.size());

    // TODO
    mathfu::mat4 worldTransform = mathfu::mat4::Identity();

    for (Object& object : objects_)
    {
        if (!om->isAlive(object))
        {
            continue;
        }

        ObjectHandle rHandle = rm->getObjIndex(object);
        if (rHandle.valid())
        {
            VisibleCandidate candidate = {buildRendCandidate(object, worldTransform)};
            candRenderableObjs_.emplace_back(candidate);
        }

        ObjectHandle lHandle = lm->getObjIndex(object);
        if (lHandle.valid())
        {
            candLightObjs.emplace_back(lm->getLightInstance(object));
        }
    }

    // ============ visibility checks and culling ===================
    // First renderables - split work tasks and run async - Sets the visibility
    // bit if passes intersection test This will then be used to generate the
    // render queue
    // NOTE: These checks will eventually be done in a compute shader.
    getVisibleRenderables(frustum, candRenderableObjs_);

    getVisibleLights(frustum, candLightObjs);

    // ============ render queue generation =========================
    std::vector<RenderableQueueInfo> queueRend;
    queueRend.reserve(200);

    // key a count of the number of static and skinned models for later
    size_t staticModelCount = 0;
    size_t skinnedModelCount = 0;

    for (const VisibleCandidate& cand : candRenderableObjs_)
    {
        IRenderable* rend = cand.renderable;
        auto& meshVariants = rend->getRenderPrimitive()->getVariantBits();

        // only add visible renderables to the queue
        if (!rend->getVisibility().testBit(IRenderable::Visible::Render) &&
            !rend->getVisibility().testBit(IRenderable::Visible::Ignore))
        {
            continue;
        }

        if (meshVariants.testBit(IRenderPrimitive::Variants::HasSkin))
        {
            ++skinnedModelCount;
        }
        ++staticModelCount;

        // Let's update the material now as all data that requires an update
        // "should" have been done by now for this frame.
        for (IRenderPrimitive* prim : rend->getAllRenderPrimitives())
        {
            IMaterial* mat = prim->getMaterial();
            mat->update(engine_);

            RenderableQueueInfo queueInfo;
            queueInfo.renderableData = (void*)rend;
            queueInfo.primitiveData = (void*)prim;
            queueInfo.renderableHandle = this;
            queueInfo.renderFunc = ColourPass::drawCallback;
            // TODO: screen layer and depth is ignored at present
            queueInfo.sortingKey =
                RenderQueue::createSortKey(0, mat->getViewLayer(), mat->getPipelineId());
            queueRend.emplace_back(queueInfo);
        }
    }
    renderQueue_.pushRenderables(queueRend, RenderQueue::Type::Colour);

    // ================== update ubos =================================
    sceneUbo_->updateCamera(*camera_);
    sceneUbo_->updateIbl(indirectLight_);
    sceneUbo_->updateDirLight(engine_, lm->getDirLightParams());
    sceneUbo_->upload(engine_);

    // we also update the transforms every frame though could have a dirty flag
    updateTransformBuffer(candRenderableObjs_, staticModelCount, skinnedModelCount);

    lm->updateSsbo(candLightObjs);

    return true;
}

IScene::VisibleCandidate IScene::buildRendCandidate(Object& obj, const mathfu::mat4& worldMatrix)
{
    auto* transManager = engine_.getTransformManager();
    auto* rendManager = engine_.getRenderableManager();

    VisibleCandidate candidate;
    candidate.renderable = rendManager->getMesh(obj);
    candidate.transform = transManager->getTransform(obj);

    // if this renderable is void from the visibility checks,
    // then return early.
    if (candidate.renderable->getVisibility().testBit(IRenderable::Visible::Ignore))
    {
        return candidate;
    }

    // calculate the world-orientated AABB
    mathfu::mat4 localMat = candidate.transform->modelTransform;
    candidate.worldTransform = worldMatrix * localMat;

    AABBox box {
        candidate.renderable->getRenderPrimitive()->getDimensions().min,
        candidate.renderable->getRenderPrimitive()->getDimensions().max};
    candidate.worldAABB = AABBox::calculateRigidTransform(box, candidate.worldTransform);
    return candidate;
}

void IScene::getVisibleRenderables(
    Frustum& frustum, std::vector<IScene::VisibleCandidate>& renderables)
{
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, renderables.size()), [&](tbb::blocked_range<size_t> range) {
            for (size_t idx = range.begin(); idx < range.end(); ++idx)
            {
                if (frustum.checkIntersection(renderables[idx].worldAABB))
                {
                    renderables[idx].renderable->getVisibility() |= IRenderable::Visible::Render;
                }
            }
        });
}

void IScene::getVisibleLights(Frustum& frustum, std::vector<LightInstance*>& lights)
{
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, lights.size()), [&](tbb::blocked_range<size_t> range) {
            for (size_t idx = range.begin(); idx < range.end(); ++idx)
            {
                // no visibility checks to be done on directional lights
                if (lights[idx]->type == LightManager::Type::Directional)
                {
                    lights[idx]->isVisible = true;
                    continue;
                }

                // check whether this light is with the frustum boundaries
                lights[idx]->isVisible = false;
                if (frustum.checkSphereIntersect(
                        lights[idx]->position, lights[idx]->spotLightInfo.radius))
                {
                    lights[idx]->isVisible = true;
                }
            }
        });
}

void IScene::updateTransformBuffer(
    const std::vector<IScene::VisibleCandidate>& candObjects,
    const size_t staticModelCount,
    const size_t skinnedModelCount)
{
    // Dynamic buffers are aligned to >256 bytes as designated by the Vulkan
    // spec
    const size_t staticDynAlign = (transUbo_->size() + 256 - 1) & ~(256 - 1);
    const size_t skinDynAlign = (skinUbo_->size() + 256 - 1) & ~(256 - 1);

    uint8_t* transPtr = nullptr;
    uint8_t* skinPtr = nullptr;

    if (staticModelCount > 0)
    {
        transPtr = static_cast<uint8_t*>(
            util::align_alloc(staticDynAlign * staticModelCount, staticDynAlign));
        ASSERT_FATAL(transPtr, "Unable to allocate aligned memory for transform ubo");
    }
    if (skinnedModelCount > 0)
    {
        // NOTE: This is wrong! The number of joint matrices per skinned model needs to be
        // taken into account. Will probably need a loop to get the total size so memory
        // can be allocated up front.
        skinPtr = static_cast<uint8_t*>(
            util::align_alloc(skinDynAlign * skinnedModelCount, skinDynAlign));
        ASSERT_FATAL(skinPtr, "Unable to allocate aligned memory for skin ubo");
    }

    size_t staticCount = 0;
    size_t skinnedCount = 0;

    for (auto& cand : candObjects)
    {
        IRenderable* rend = cand.renderable;
        if (!rend->getVisibility().testBit(IRenderable::Visible::Render))
        {
            continue;
        }

        TransformInfo* transInfo = cand.transform;

        size_t meshOffset = staticDynAlign * staticCount++;
        auto* currStaticPtr = reinterpret_cast<mathfu::mat4*>(transPtr + meshOffset);
        *currStaticPtr = transInfo->modelTransform;

        // the dynamic buffer offsets are stored in the renderable for ease of
        // access when drawing
        rend->setMeshDynamicOffset(static_cast<uint32_t>(meshOffset));

        if (!transInfo->jointMatrices.empty())
        {
            // NOTE: The offset needs to take into account the number of joints per model skin (i.e.
            // keep track of the previous count)
            size_t skinOffset = skinDynAlign * skinnedCount++;
            auto* currSkinPtr = reinterpret_cast<mathfu::mat4*>(skinPtr + skinDynAlign);
            *currSkinPtr = *transInfo->jointMatrices.data();

            // rather than throw an error, clamp the joint if it exceeds the max
            /*uint32_t jointCount = std::min(
                ITransformManager::MaxBoneCount,
                static_cast<uint32_t>(transInfo->jointMatrices.size()));*/

            rend->setSkinDynamicOffset(static_cast<uint32_t>(skinOffset));
        }
    }

    auto& driver = engine_.driver();
    if (staticCount > 0)
    {
        size_t staticDataSize = staticModelCount * staticDynAlign;
        transUbo_->mapGpuBuffer(driver, transPtr, staticDataSize);
        util::align_free(transPtr);
    }

    if (skinnedCount > 0)
    {
        size_t skinnedDataSize = skinnedModelCount * skinDynAlign;
        skinUbo_->mapGpuBuffer(driver, skinPtr, skinnedDataSize);
        util::align_free(skinPtr);
    }
}

void IScene::destroyObject(Object obj)
{
    auto iter = std::find_if(objects_.begin(), objects_.end(), [&obj](const Object& rhs) {
        return obj.getId() == rhs.getId();
    });
    ASSERT_FATAL(
        iter != objects_.end(),
        "Trying to delete an object of id %d that is not present within the objects list for this "
        "scene");
    objects_.erase(iter);
}

void IScene::addObject(Object obj) { objects_.emplace_back(obj); }

void IScene::usePostProcessing(bool state) { usePostProcessing_ = state; }

void IScene::useGbuffer(bool state) { useGbuffer_ = state; }

void IScene::setBloomOptions(const BloomOptions& bloom) { bloomOptions_ = bloom; }

BloomOptions& IScene::getBloomOptions() { return bloomOptions_; }

void IScene::setGbufferOptions(const GbufferOptions& gb) { gbufferOptions_ = gb; }

GbufferOptions& IScene::getGbufferOptions() { return gbufferOptions_; }


} // namespace yave
