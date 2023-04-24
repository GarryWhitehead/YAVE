/* Copyright (c) 2018-2020 Garry Whitehead
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

#include "light_manager.h"

#include "camera.h"
#include "engine.h"
#include "object_instance.h"
#include "render_graph/rendergraph_resource.h"
#include "scene.h"
#include "utility/assertion.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/program_manager.h"
#include "vulkan-api/sampler_cache.h"
#include "yave/texture_sampler.h"

namespace yave
{

ILightManager::ILightManager(IEngine& engine) : engine_(engine), programBundle_(nullptr)
{
    ssbo_ = std::make_unique<StorageBuffer>(
        StorageBuffer::AccessType::ReadOnly,
        vkapi::PipelineCache::SsboSetValue,
        0,
        "LightSsbo",
        "light_ssbo");

    ssbo_->pushElement(
        "params", backend::BufferElementType::Struct, sizeof(LightSsbo), nullptr, 0, "LightParams");
    ssbo_->createGpuBuffer(engine_.driver(), MaxLightCount * sizeof(ILightManager::LightSsbo));

    // A sampler for each of the gbuffer render targets.
    samplerSets_.pushSampler(
        "PositionSampler",
        vkapi::PipelineCache::SamplerSetValue,
        SamplerPositionBinding,
        SamplerSet::SamplerType::e2d);
    samplerSets_.pushSampler(
        "BaseColourSampler",
        vkapi::PipelineCache::SamplerSetValue,
        SamplerColourBinding,
        SamplerSet::SamplerType::e2d);
    samplerSets_.pushSampler(
        "NormalSampler",
        vkapi::PipelineCache::SamplerSetValue,
        SamplerNormalBinding,
        SamplerSet::SamplerType::e2d);
    samplerSets_.pushSampler(
        "PbrSampler",
        vkapi::PipelineCache::SamplerSetValue,
        SamplerPbrBinding,
        SamplerSet::SamplerType::e2d);
    samplerSets_.pushSampler(
        "EmissiveSampler",
        vkapi::PipelineCache::SamplerSetValue,
        SamplerEmissiveBinding,
        SamplerSet::SamplerType::e2d);
}

ILightManager::~ILightManager() {}

void ILightManager::prepareI()
{
    // if we have already initialised, then let's not do it again.
    if (programBundle_)
    {
        return;
    }
    auto& driver = engine_.driver();
    auto& manager = driver.progManager();

    programBundle_ = manager.createProgramBundle();
    programBundle_->buildShaders("lighting.vert", "lighting.frag");

    IScene* scene = engine_.getCurrentScene();
    ICamera* camera = scene->getCurrentCameraI();

    // The render primitive - simple version which only states the vertex
    // count for the full-screen quad. The vertex count is three as we
    // draw a triangle which covers the screen with clipping.
    programBundle_->addRenderPrimitive(3);

    programBundle_->rasterState_.cullMode = vk::CullModeFlagBits::eFront;
    programBundle_->rasterState_.frontFace = vk::FrontFace::eCounterClockwise;

    // The camera uniform buffer required by the vertex shader.
    auto* vProgram = programBundle_->getProgram(backend::ShaderStage::Vertex);
    vProgram->addAttributeBlock(camera->getUbo().createShaderStr());

    // Add the samplers and push block code for the fragment shader.
    auto* fProgram = programBundle_->getProgram(backend::ShaderStage::Fragment);
    fProgram->addAttributeBlock(samplerSets_.createShaderStr());
    fProgram->addAttributeBlock(ssbo_->createShaderStr());

    // Camera ubo
    auto camUbo = camera->getUbo().getBufferParams(driver);
    programBundle_->addDescriptorBinding(
        static_cast<uint32_t>(camUbo.size),
        camUbo.binding,
        camUbo.buffer,
        vk::DescriptorType::eUniformBuffer);

    // Storage buffer
    auto ssboParams = ssbo_->getBufferParams(driver);
    programBundle_->addDescriptorBinding(
        MaxLightCount * sizeof(ILightManager::LightSsbo),
        ssboParams.binding,
        ssboParams.buffer,
        vk::DescriptorType::eStorageBuffer);
}

void ILightManager::calculateSpotCone(float outerCone, float innerCone, LightInstance& light)
{
    if (light.type != LightManager::Type::Spot)
    {
        return;
    }

    // first calculate the spotlight cone values
    float outer = std::min(std::abs(outerCone), static_cast<float>(M_PI));
    float inner = std::min(std::abs(innerCone), static_cast<float>(M_PI));
    inner = std::min(inner, outer);

    float cosOuter = std::cos(outer);
    float cosInner = std::cos(inner);

    light.spotLightInfo.outer = outer;
    light.spotLightInfo.cosOuterSquared = cosOuter * cosOuter;
    light.spotLightInfo.scale = 1.0f / std::max(1.0f / 1024.0f, cosInner - cosOuter);
    light.spotLightInfo.offset = -cosOuter * light.spotLightInfo.scale;
}

void ILightManager::setIntensity(float intensity, LightManager::Type type, LightInstance& light)
{
    switch (type)
    {
        case LightManager::Type::Directional:
            light.intensity = intensity;
            break;
        case LightManager::Type::Point:
            light.intensity = intensity * static_cast<float>(M_1_PI) * 0.25f;
            break;
        case LightManager::Type::Spot:
            light.intensity = intensity * static_cast<float>(M_1_PI);
    }
}

void ILightManager::setRadius(float fallout, LightInstance& light)
{
    if (light.type != LightManager::Type::Directional)
    {
        light.spotLightInfo.radius = fallout;
    }
}

void ILightManager::createLight(
    const LightManager::CreateInfo& ci, IObject* obj, LightManager::Type type)
{
    // first add the object which will give us a free slot
    ObjectHandle handle = addObject(*obj);

    auto instance = std::make_unique<LightInstance>();
    instance->type = type;
    instance->position = ci.position;
    instance->target = ci.target;
    instance->colour = ci.colour;
    instance->fov = ci.fov;
    instance->spotLightInfo.radius = ci.fallout;

    setRadius(ci.fallout, *instance);
    setIntensity(ci.intensity, type, *instance);
    calculateSpotCone(ci.outerCone, ci.innerCone, *instance);

    // check whether we just add to the back or use a freed slot
    if (handle.get() >= lights_.size())
    {
        lights_.emplace_back(std::move(instance));
    }
    else
    {
        lights_[handle.get()] = std::move(instance);
    }
}

void ILightManager::update(const ICamera& camera)
{
    auto& manager = engine_.driver().progManager();

    for (auto& light : lights_)
    {
        mathfu::mat4 projection =
            mathfu::mat4::Perspective(light->fov, 1.0f, camera.getNear(), camera.getFar());
        mathfu::mat4 view =
            mathfu::mat4::LookAt(light->target, light->position, {0.0f, 1.0f, 0.0f});
        light->mvp = projection * view;
    }

    // Create the lighting shader.
    auto* vProgram = programBundle_->getProgram(backend::ShaderStage::Vertex);
    auto* fProgram = programBundle_->getProgram(backend::ShaderStage::Fragment);

    vkapi::Shader* vertexShader = manager.findShaderVariantOrCreate(
        {}, backend::ShaderStage::Vertex, vk::PrimitiveTopology::eTriangleList, programBundle_);
    vProgram->addShader(vertexShader);

    vkapi::Shader* fragShader = manager.findShaderVariantOrCreate(
        {}, backend::ShaderStage::Fragment, vk::PrimitiveTopology::eTriangleList, programBundle_);
    fProgram->addShader(fragShader);
}

void ILightManager::updateSsbo(std::vector<LightInstance*>& lights)
{
    ASSERT_FATAL(
        lights.size() < ILightManager::MaxLightCount,
        "Number of lights (%d) exceed the max allowed (%d).",
        lights.size(),
        ILightManager::MaxLightCount);

    // clear the buffer so we don't get any invalid values
    memset(ssboBuffer_, 0, sizeof(ILightManager::LightSsbo) * ILightManager::MaxLightCount);

    int idx = 0;
    for (const auto* light : lights)
    {
        if (!light->isVisible)
        {
            continue;
        }
        ssboBuffer_[idx] = {
            light->mvp,
            {light->position, 1.0f},
            {light->target, 1.0f},
            {light->colour, light->intensity},
            static_cast<int>(light->type)};

        if (light->type == LightManager::Type::Point)
        {
            ssboBuffer_[idx].fallOut = light->intensity;
        }
        else if (light->type == LightManager::Type::Spot)
        {
            ssboBuffer_[idx].fallOut = light->intensity;
            ssboBuffer_[idx].scale = light->spotLightInfo.scale;
            ssboBuffer_[idx].offset = light->spotLightInfo.offset;
        }
        ++idx;
    }
    // The end of the viable lights to render is signified on the shader
    // by a light type of 0xFF;
    ssboBuffer_[idx].type = EndOfBufferSignal;

    auto& driver = engine_.driver();
    uint32_t lightCount = lights.size() + 1;
    size_t mappedSize = lightCount * sizeof(ILightManager::LightSsbo);
    ssbo_->mapGpuBuffer(driver, ssboBuffer_, mappedSize);
}

LightInstance* ILightManager::getLightInstance(IObject* obj)
{
    ASSERT_FATAL(obj, "Can not retrive light instance, object is null.");
    ASSERT_FATAL(
        hasObject(*obj), "Object with id %d is not associated with this manager", obj->id());
    return lights_[getObjIndex(*obj).get()].get();
}

size_t ILightManager::getLightCount() const { return lights_.size(); }

void ILightManager::setIntensityI(float intensity, IObject* obj)
{
    LightInstance* instance = getLightInstance(obj);
    setIntensity(intensity, instance->type, *instance);
}

void ILightManager::setFalloutI(float fallout, IObject* obj)
{
    LightInstance* instance = getLightInstance(obj);
    setRadius(fallout, *instance);
}

void ILightManager::setPositionI(const mathfu::vec3& pos, IObject* obj)
{
    LightInstance* instance = getLightInstance(obj);
    instance->position = pos;
}

void ILightManager::setTargetI(const mathfu::vec3& target, IObject* obj)
{
    LightInstance* instance = getLightInstance(obj);
    instance->target = target;
}

void ILightManager::setColourI(const mathfu::vec3& col, IObject* obj)
{
    LightInstance* instance = getLightInstance(obj);
    instance->colour = col;
}

void ILightManager::setFovI(float fov, IObject* obj)
{
    LightInstance* instance = getLightInstance(obj);
    instance->fov = fov;
}

void ILightManager::destroy(const IObject& obj)
{
    ASSERT_FATAL(obj.isActive(), "Something went wrong - invalid object handle: not active.");
    removeObject(obj);
}

rg::RenderGraphHandle ILightManager::render(
    rg::RenderGraph& rGraph, uint32_t width, uint32_t height, vk::Format depthFormat)
{
    struct LightPassData
    {
        rg::RenderGraphHandle rt;
        rg::RenderGraphHandle light;
        rg::RenderGraphHandle depth;
        // inputs
        rg::RenderGraphHandle position;
        rg::RenderGraphHandle normal;
        rg::RenderGraphHandle colour;
        rg::RenderGraphHandle pbr;
        rg::RenderGraphHandle emissive;
    };

    auto rg = rGraph.addPass<LightPassData>(
        "LightingPass",
        [&](rg::RenderGraphBuilder& builder, LightPassData& data) {
            auto* blackboard = rGraph.getBlackboard();

            // Get the resources from the colour pass
            auto position = blackboard->get("position");
            auto colour = blackboard->get("colour");
            auto normal = blackboard->get("normal");
            auto emissive = blackboard->get("emissive");
            auto pbr = blackboard->get("pbr");
            auto gbDepth = blackboard->get("gbufferDepth");

            rg::TextureResource::Descriptor texDesc;
            texDesc.format = vk::Format::eR8G8B8A8Unorm;
            texDesc.width = width;
            texDesc.height = height;
            data.light = builder.createResource("light", texDesc);

            texDesc.format = depthFormat;
            data.depth = builder.createResource("lightDepth", texDesc);

            data.light = builder.addWriter(data.light, vk::ImageUsageFlagBits::eColorAttachment);
            data.depth =
                builder.addWriter(data.depth, vk::ImageUsageFlagBits::eDepthStencilAttachment);

            // inputs into the pass
            data.position = builder.addReader(position, vk::ImageUsageFlagBits::eSampled);
            data.colour = builder.addReader(colour, vk::ImageUsageFlagBits::eSampled);
            data.normal = builder.addReader(normal, vk::ImageUsageFlagBits::eSampled);
            data.emissive = builder.addReader(emissive, vk::ImageUsageFlagBits::eSampled);
            data.pbr = builder.addReader(pbr, vk::ImageUsageFlagBits::eSampled);

            blackboard->add("light", data.light);
            blackboard->add("lightDepth", data.depth);

            rg::PassDescriptor desc;
            desc.attachments.attach.colour[0] = data.light;
            desc.attachments.attach.depth = {data.depth};
            desc.dsLoadClearFlags = {vkapi::LoadClearFlags::Clear};
            data.rt = builder.createRenderTarget("lightRT", desc);
        },
        [=](::vkapi::VkDriver& driver,
            const LightPassData& data,
            const rg::RenderGraphResource& resources) {
            auto& cmds = driver.getCommands();
            vk::CommandBuffer cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

            auto info = resources.getRenderPassInfo(data.rt);
            driver.beginRenderpass(cmdBuffer, info.data, info.handle);

            // use the gbuffer render targets as the samplers in this lighting
            // pass
            TextureSampler samplerParams(
                backend::SamplerFilter::Linear,
                backend::SamplerFilter::Linear,
                backend::SamplerAddressMode::ClampToEdge,
                8.0f);
            vk::Sampler sampler =
                engine_.driver().getSamplerCache().createSampler(samplerParams.get());
            programBundle_->setTexture(
                resources.getTextureHandle(data.position), SamplerPositionBinding, sampler);
            programBundle_->setTexture(
                resources.getTextureHandle(data.colour), SamplerColourBinding, sampler);
            programBundle_->setTexture(
                resources.getTextureHandle(data.normal), SamplerNormalBinding, sampler);
            programBundle_->setTexture(
                resources.getTextureHandle(data.pbr), SamplerPbrBinding, sampler);
            programBundle_->setTexture(
                resources.getTextureHandle(data.emissive), SamplerEmissiveBinding, sampler);

            driver.draw(cmdBuffer, *programBundle_);

            driver.endRenderpass(cmdBuffer);

            cmds.flush();
        });

    return rg.getData().light;
}

// ========================= client api =================================

LightManager::LightManager() = default;
LightManager::~LightManager() = default;

void ILightManager::create(const CreateInfo& ci, Type type, Object* obj)
{
    createLight(ci, reinterpret_cast<IObject*>(obj), type);
}

void ILightManager::prepare() { prepareI(); }

void ILightManager::setIntensity(float intensity, Object* obj)
{
    setIntensityI(intensity, reinterpret_cast<IObject*>(obj));
}

void ILightManager::setFallout(float fallout, Object* obj)
{
    setFalloutI(fallout, reinterpret_cast<IObject*>(obj));
}

void ILightManager::setPosition(const mathfu::vec3& pos, Object* obj)
{
    setPositionI(pos, reinterpret_cast<IObject*>(obj));
}

void ILightManager::setTarget(const mathfu::vec3& target, Object* obj)
{
    setTargetI(target, reinterpret_cast<IObject*>(obj));
}

void ILightManager::setColour(const mathfu::vec3& col, Object* obj)
{
    setColourI(col, reinterpret_cast<IObject*>(obj));
}

void ILightManager::setFov(float fov, Object* obj)
{
    setFovI(fov, reinterpret_cast<IObject*>(obj));
}

} // namespace yave
