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

IScene::IScene(IEngine& engine) : engine_(engine), camera_(nullptr), skybox_(nullptr)
{
    vkapi::VkDriver& driver = engine_.driver();

    transUbo_ = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboDynamicSetValue, 0, "TransformUbo", "mesh_ubo");
    transUbo_->pushElement("modelMatrix", backend::BufferElementType::Mat4, sizeof(mathfu::mat4));
    transUbo_->createGpuBuffer(driver, ModelBufferInitialSize * transUbo_->size());

    skinUbo_ = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboDynamicSetValue, 1, "skinUbo", "skin_ubo");
    skinUbo_->pushElement(
        "jointMatrices",
        backend::BufferElementType::Mat4,
        sizeof(mathfu::mat4),
        nullptr,
        ITransformManager::MaxBoneCount);
    skinUbo_->pushElement("jointCount", backend::BufferElementType::Float, sizeof(float));
    skinUbo_->createGpuBuffer(driver, ModelBufferInitialSize * skinUbo_->size());
}

IScene::~IScene() {}

void IScene::shutDown(vkapi::VkDriver& driver) noexcept {}

void IScene::setSkyboxI(ISkybox* skybox) noexcept
{
    ASSERT_FATAL(camera_, "The camera must be set before declaring the skybox.");
    skybox_ = skybox;
}

void IScene::setCameraI(ICamera* cam) noexcept
{
    ASSERT_FATAL(cam, "The camera is nullptr.");
    camera_ = cam;

    auto& driver = engine_.driver();
    camera_->createUbo(driver);
}

bool IScene::update()
{
    const auto& lightManager = engine_.getLightManagerI();
    const auto& rendManager = engine_.getRenderableManagerI();

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

    // Update the lights since we have no updated the camera for this frame.
    lightManager->update(*camera_);

    // At the moment we iterate through the list of objects
    // and find any that have a renderable or light component. If they are
    // active then these are added as a potential candiate lighting source
    auto& objects = engine_.getObjectList();

    std::vector<LightInstance*> candLightObjs;
    candLightObjs.reserve(objects.size());

    // TODO
    mathfu::mat4 worldTransform = mathfu::mat4::Identity();

    for (const auto& object : objects)
    {
        if (!object->isActive())
        {
            continue;
        }

        ObjectHandle rHandle = rendManager->getObjIndex(*object);
        if (rHandle.valid())
        {
            VisibleCandidate candidate = {buildRendCandidate(object.get(), worldTransform)};
            candRenderableObjs_.emplace_back(candidate);
        }

        ObjectHandle lHandle = lightManager->getObjIndex(*object);
        if (lHandle.valid())
        {
            candLightObjs.emplace_back(lightManager->getLightInstance(object.get()));
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
    // camera buffer is updated every frame as we expect this to change a lot
    updateCameraBuffer();

    // we also update the transforms every frame though could have a dirty flag
    updateTransformBuffer(candRenderableObjs_, staticModelCount, skinnedModelCount);

    lightManager->updateSsbo(candLightObjs);

    return true;
}

IScene::VisibleCandidate IScene::buildRendCandidate(IObject* obj, const mathfu::mat4& worldMatrix)
{
    auto* transManager = engine_.getTransformManagerI();
    auto* rendManager = engine_.getRenderableManagerI();

    VisibleCandidate candidate;
    candidate.renderable = rendManager->getMesh(*obj);
    candidate.transform = transManager->getTransform(*obj);

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

void IScene::updateCameraBuffer()
{
    // update everything in the buffer
    void* uboData = camera_->updateUbo();
    ASSERT_LOG(uboData);
    camera_->getUbo().mapGpuBuffer(engine_.driver(), uboData);
}

void IScene::updateTransformBuffer(
    const std::vector<IScene::VisibleCandidate>& candIObjects,
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
        skinPtr = static_cast<uint8_t*>(
            util::align_alloc(skinDynAlign * skinnedModelCount, skinDynAlign));
        ASSERT_FATAL(skinPtr, "Unable to allocate aligned memory for skin ubo");
    }

    size_t staticCount = 0;
    size_t skinnedCount = 0;

    for (auto& cand : candIObjects)
    {
        IRenderable* rend = cand.renderable;
        if (!rend->getVisibility().testBit(IRenderable::Visible::Render))
        {
            continue;
        }

        TransformInfo* transInfo = cand.transform;

        size_t meshOffset = staticDynAlign * staticCount++;
        uint8_t* currStaticPtr = (uint8_t*)((uint64_t)transPtr + (meshOffset));
        memcpy(currStaticPtr, &transInfo->modelTransform, sizeof(mathfu::mat4));

        // the dynamic buffer offsets are stored in the renderable for ease of
        // access when drawing
        rend->setMeshDynamicOffset(static_cast<uint32_t>(meshOffset));

        if (!transInfo->jointMatrices.empty())
        {
            size_t skinOffset = skinDynAlign * skinnedCount++;
            uint8_t* currSkinPtr = (uint8_t*)((uint64_t)skinPtr + (skinDynAlign * skinnedCount++));

            // rather than throw an error, clamp the joint if it exceeds the max
            uint32_t jointCount = std::min(
                ITransformManager::MaxBoneCount,
                static_cast<uint32_t>(transInfo->jointMatrices.size()));
            memcpy(currSkinPtr, transInfo->jointMatrices.data(), jointCount * sizeof(mathfu::mat4));

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

// ======================== client api =======================

Scene::Scene() {}
Scene::~Scene() {}

void IScene::setSkybox(Skybox* skybox) { setSkyboxI(reinterpret_cast<ISkybox*>(skybox)); }

void IScene::setCamera(Camera* cam) { setCameraI(reinterpret_cast<ICamera*>(cam)); }

Camera* IScene::getCurrentCamera() { return reinterpret_cast<Camera*>(&(getCurrentCameraI())); }

} // namespace yave
