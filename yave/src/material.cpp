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


#include "material.h"

#include "backend/convert_to_vk.h"
#include "camera.h"
#include "engine.h"
#include "mapped_texture.h"
#include "renderable.h"
#include "scene.h"
#include "utility/assertion.h"
#include "utility/bitset_enum.h"
#include "vulkan-api/pipeline.h"
#include "vulkan-api/pipeline_cache.h"
#include "vulkan-api/sampler_cache.h"
#include "vulkan-api/utility.h"
#include "yave/texture_sampler.h"

#include <spdlog/spdlog.h>

namespace yave
{

IMaterial::IMaterial(IEngine& engine) : doubleSided_(false), pipelineId_(0), viewLayer_(0x2)
{
    pushBlock_[util::ecast(backend::ShaderStage::Vertex)] =
        std::make_unique<PushBlock>("VertexPushBlock", "push_params");
    pushBlock_[util::ecast(backend::ShaderStage::Fragment)] =
        std::make_unique<PushBlock>("FragmentPushBlock", "push_params");

    ubos_[0] = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboSetValue, VertexUboBindPoint, "VertMaterialUbo", "ubo");
    ubos_[1] = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboSetValue, FragmentUboBindPoint, "FragMaterialUbo", "material_ubo");

    programBundle_ = engine.driver().progManager().createProgramBundle();

    // default workflow is to use gbuffers for rendering.
    variantBits_.setBit(Variants::EnableGBufferPipeline);
}

IMaterial::~IMaterial() {}

std::string imageTypeToStr(Material::ImageType type)
{
    switch (type)
    {
        case Material::ImageType::BaseColour:
            return "BaseColourSampler";
        case Material::ImageType::Normal:
            return "NormalSampler";
        case Material::ImageType::MetallicRoughness:
            return "MetallicRoughnessSampler";
        case Material::ImageType::Emissive:
            return "EmissiveSampler";
        case Material::ImageType::Occlusion:
            return "OcclusionSampler";
    }
    return "";
}

void IMaterial::addVariant(const Material::ImageType type)
{
    switch (type)
    {
        case Material::ImageType::BaseColour:
            variantBits_ |= Variants::HasBaseColourSampler;
            break;
        case Material::ImageType::Normal:
            variantBits_ |= Variants::HasNormalSampler;
            break;
        case Material::ImageType::MetallicRoughness:
            variantBits_ |= Variants::HasMrSampler;
            break;
        case Material::ImageType::Emissive:
            variantBits_ |= Variants::HasEmissiveSampler;
            break;
        case Material::ImageType::Occlusion:
            variantBits_ |= Variants::HasOcclusionSampler;
            break;
        default:
            SPDLOG_WARN("Invalid material variant bit. Ignoring....");
    }
}

void IMaterial::addVariant(Variants variant) { variantBits_ |= variant; }

vkapi::VDefinitions IMaterial::createVariants(util::BitSetEnum<IMaterial::Variants>& bits)
{
    vkapi::VDefinitions map(static_cast<uint8_t>(backend::ShaderStage::Fragment));
    if (bits.testBit(MrPipeline))
    {
        map.emplace("METALLIC_ROUGHNESS_PIPELINE", 1);
    }
    if (bits.testBit(Variants::SpecularPipeline))
    {
        map.emplace("SPECULAR_GLOSSINESS_PIPELINE", 1);
    }
    if (bits.testBit(Variants::HasBaseColourSampler))
    {
        map.emplace("HAS_BASECOLOUR_SAMPLER", 1);
    }
    if (bits.testBit(Variants::HasNormalSampler))
    {
        map.emplace("HAS_NORMAL_SAMPLER", 1);
    }
    if (bits.testBit(Variants::HasMrSampler))
    {
        map.emplace("HAS_METALLICROUGHNESS_SAMPLER", 1);
    }
    if (bits.testBit(Variants::HasOcclusionSampler))
    {
        map.emplace("HAS_OCCLUSION_SAMPLER", 1);
    }
    if (bits.testBit(Variants::HasEmissiveSampler))
    {
        map.emplace("HAS_EMISSIVE_SAMPLER", 1);
    }
    if (bits.testBit(Variants::HasBaseColourFactor))
    {
        map.emplace("HAS_BASECOLOUR_FACTOR", 1);
    }
    if (bits.testBit(Variants::HasAlphaMask))
    {
        map.emplace("HAS_ALPHA_MASK", 1);
    }
    if (bits.testBit(Variants::HasAlphaMaskCutOff))
    {
        map.emplace("HAS_ALPHA_MASK_CUTOFF", 1);
    }
    if (bits.testBit(Variants::HasDiffuseFactor))
    {
        map.emplace("HAS_DIFFUSE_FACTOR", 1);
    }
    if (bits.testBit(Variants::HasSpecularFactor))
    {
        map.emplace("HAS_SPECULAR_FACTOR", 1);
    }
    if (bits.testBit(Variants::HasEmissiveFactor))
    {
        map.emplace("HAS_EMISSIVE_FACTOR", 1);
    }
    if (bits.testBit(Variants::HasMetallicFactor))
    {
        map.emplace("HAS_METALLIC_FACTOR", 1);
    }
    if (bits.testBit(Variants::HasRoughnessFactor))
    {
        map.emplace("HAS_ROUGHNESS_FACTOR", 1);
    }
    if (bits.testBit(Variants::EnableGBufferPipeline))
    {
        map.emplace("USE_GBUFFER_OUTPUT", 1);
    }
    return map;
}

void IMaterial::setPipelineI(Material::Pipeline pipeline) noexcept
{
    if (pipeline == Material::Pipeline::MetallicRoughness)
    {
        variantBits_ |= IMaterial::Variants::MrPipeline;
    }
    else if (pipeline == Material::Pipeline::SpecularGlosiness)
    {
        variantBits_ |= IMaterial::Variants::SpecularPipeline;
    }
}

Material::Pipeline IMaterial::getPipelineState() noexcept
{
    if (variantBits_.testBit(IMaterial::Variants::MrPipeline))
    {
        return Material::Pipeline::MetallicRoughness;
    }
    if (variantBits_.testBit(IMaterial::Variants::SpecularPipeline))
    {
        return Material::Pipeline::SpecularGlosiness;
    }
    return Material::Pipeline::None;
}

void IMaterial::addImageTexture(
    vkapi::VkDriver& driver,
    IMappedTexture* texture,
    Material::ImageType type,
    const backend::TextureSamplerParams& params,
    uint32_t binding)
{
    addVariant(type);

    ASSERT_FATAL(
        binding < vkapi::PipelineCache::MaxSamplerBindCount,
        "Out of range for texture binding (=%d). Max allowed count is %d\n",
        binding,
        vkapi::PipelineCache::MaxSamplerBindCount);

    // TODO: check for 3d textures when supported
    SamplerSet::SamplerType samplerType =
        texture->isCubeMap() ? SamplerSet::SamplerType::Cube : SamplerSet::SamplerType::e2d;

    setSamplerParamI(imageTypeToStr(type), binding, samplerType);

    programBundle_->setTexture(
        texture->getBackendHandle(), binding, driver.getSamplerCache().createSampler(params));
}

void IMaterial::addBuffer(BufferBase* buffer, backend::ShaderStage type)
{
    ASSERT_FATAL(buffer, "Buffer is NULL.");
    buffers_.push_back(std::make_pair(type, buffer));
}

void IMaterial::build(
    IEngine& engine, IRenderable& renderable, IRenderPrimitive* prim, const std::string& matShader)
{
    auto& manager = engine.driver().progManager();
    auto& driver = engine.driver();

    IScene* scene = engine.getCurrentScene();

    // create the material shaders to start.
    programBundle_->parseMaterialShader(matShader);
    programBundle_->buildShaders("material.vert", "material.frag");

    // add any additional buffer elemnts, push blocks or image samplers
    // to the appropiate shader before building
    auto* vProgram = programBundle_->getProgram(backend::ShaderStage::Vertex);
    auto* fProgram = programBundle_->getProgram(backend::ShaderStage::Fragment);

    // default ubo buffers - camera and mesh transform
    addBuffer(&scene->getCurrentCameraI()->getUbo(), backend::ShaderStage::Vertex);
    addBuffer(&scene->getTransUbo(), backend::ShaderStage::Vertex);

    for (const auto& [stage, buffer] : buffers_)
    {
        if (stage == backend::ShaderStage::Vertex)
        {
            vProgram->addAttributeBlock(buffer->createShaderStr());
        }
        else if (stage == backend::ShaderStage::Fragment)
        {
            fProgram->addAttributeBlock(buffer->createShaderStr());
        }

        auto params = buffer->getBufferParams(driver);
        ASSERT_FATAL(params.buffer, "Vulkan buffer handle is invalid.");

        programBundle_->addDescriptorBinding(
            params.size, params.binding, params.buffer, params.type);
    }

    // add the custom material ubo - we create the device buffer here so
    // its no longer possible to change to outlay of the ubo
    for (int i = 0; i < 2; ++i)
    {
        if (!ubos_[i]->empty())
        {
            ubos_[i]->createGpuBuffer(driver);
            auto uboParams = ubos_[i]->getBufferParams(driver);
            programBundle_->addDescriptorBinding(
                uboParams.size, uboParams.binding, uboParams.buffer, uboParams.type);
        }
    }

    PushBlock* vPushBlock = pushBlock_[util::ecast(backend::ShaderStage::Vertex)].get();
    PushBlock* fPushBlock = pushBlock_[util::ecast(backend::ShaderStage::Fragment)].get();

    vProgram->addAttributeBlock(vPushBlock->createShaderStr());
    fProgram->addAttributeBlock(fPushBlock->createShaderStr());
    vProgram->addAttributeBlock(ubos_[0]->createShaderStr());
    fProgram->addAttributeBlock(ubos_[1]->createShaderStr());
    fProgram->addAttributeBlock(samplerSet_.createShaderStr());

    // add the render primitive, with sub meshes (not properly implemented yet)
    const auto& drawData = prim->getDrawData();
    if (prim->getIndexBuffer())
    {
        programBundle_->addRenderPrimitive(
            prim->getTopology(),
            backend::indexBufferTypeToVk(prim->getIndexBuffer()->getBufferType()),
            drawData.indexCount,
            drawData.indexPrimitiveOffset,
            prim->getPrimRestartState());
    }
    else
    {
        programBundle_->addRenderPrimitive(
            prim->getTopology(), drawData.vertexCount, prim->getPrimRestartState());
    }

    // create the veretx shader (renderable)
    // variants for the vertex - in/out attributes - these are also
    // used on the fragment shader.
    vkapi::VDefinitions vertexVariants;
    if (prim->getVertexBuffer())
    {
        vertexVariants = prim->createVertexAttributeVariants();
    }

    vkapi::Shader* vertexShader = manager.findShaderVariantOrCreate(
        vertexVariants, backend::ShaderStage::Vertex, prim->getTopology(), programBundle_);
    vProgram->addShader(vertexShader);

    // create the fragment shader (material)
    if (!renderable.useGBuffer())
    {
        variantBits_.resetBit(Variants::EnableGBufferPipeline);
    }
    vkapi::VDefinitions fragVariants = IMaterial::createVariants(variantBits_);
    fragVariants.insert(vertexVariants.begin(), vertexVariants.end());

    vkapi::Shader* fragShader = manager.findShaderVariantOrCreate(
        fragVariants,
        backend::ShaderStage::Fragment,
        prim->getTopology(),
        programBundle_,
        variantBits_.getUint64());
    fProgram->addShader(fragShader);
}

void IMaterial::update(IEngine& engine) noexcept
{
    // TODO: could do with dirty flags here so we aren't updating data
    // that hasn't changed.
    PushBlock* vPushBlock = pushBlock_[util::ecast(backend::ShaderStage::Vertex)].get();
    PushBlock* fPushBlock = pushBlock_[util::ecast(backend::ShaderStage::Fragment)].get();
    if (!vPushBlock->empty())
    {
        void* data = vPushBlock->getBlockData();
        programBundle_->setPushBlockData(backend::ShaderStage::Vertex, data);
    }
    if (!fPushBlock->empty())
    {
        void* data = fPushBlock->getBlockData();
        programBundle_->setPushBlockData(backend::ShaderStage::Fragment, data);
    }

    // update the ubos
    for (int i = 0; i < 2; ++i)
    {
        if (!ubos_[i]->empty())
        {
            ubos_[i]->createGpuBuffer(engine.driver());
            ubos_[i]->mapGpuBuffer(engine.driver(), ubos_[i]->getBlockData());
        }
    }
}

void IMaterial::updateUboParamI(const std::string& name, backend::ShaderStage stage, void* value)
{
    ubos_[util::ecast(stage)]->updateElement(name, value);
}

void IMaterial::addUboParamI(
    const std::string& elementName,
    backend::BufferElementType type,
    size_t size,
    size_t arrayCount,
    backend::ShaderStage stage,
    void* value)
{
    ubos_[util::ecast(stage)]->pushElement(
        elementName, type, static_cast<uint32_t>(size), (void*)value, arrayCount);
}

void IMaterial::setDoubleSidedStateI(bool state)
{
    doubleSided_ = state;
    programBundle_->rasterState_.cullMode =
        state ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;
}

void IMaterial::setTestEnableI(bool state) { programBundle_->dsState_.testEnable = state; }

void IMaterial::setWriteEnableI(bool state) { programBundle_->dsState_.writeEnable = state; }

void IMaterial::setDepthCompareOpI(vk::CompareOp op) { programBundle_->dsState_.compareOp = op; }

void IMaterial::setPolygonModeI(vk::PolygonMode mode)
{
    programBundle_->rasterState_.polygonMode = mode;
}

void IMaterial::setFrontFaceI(vk::FrontFace face) { programBundle_->rasterState_.frontFace = face; }

void IMaterial::setCullModeI(vk::CullModeFlagBits mode)
{
    programBundle_->rasterState_.cullMode = mode;
}

void IMaterial::setBlendFactorStateI(VkBool32 state)
{
    programBundle_->blendState_.blendEnable = state;
}

void IMaterial::setSrcColorBlendFactorI(vk::BlendFactor factor)
{
    programBundle_->blendState_.srcColor = factor;
}

void IMaterial::setDstColorBlendFactorI(vk::BlendFactor factor)
{
    programBundle_->blendState_.dstColor = factor;
}
void IMaterial::setColourBlendOpI(vk::BlendOp op) { programBundle_->blendState_.colour = op; }

void IMaterial::setSrcAlphaBlendFactorI(vk::BlendFactor factor)
{
    programBundle_->blendState_.srcAlpha = factor;
}

void IMaterial::setDstAlphaBlendFactorI(vk::BlendFactor factor)
{
    programBundle_->blendState_.dstAlpha = factor;
}

void IMaterial::setAlphaBlendOpI(vk::BlendOp op) { programBundle_->blendState_.alpha = op; }

void IMaterial::setScissorI(uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset)
{
    programBundle_->setScissor(width, height, xOffset, yOffset);
}

void IMaterial::setViewportI(uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    programBundle_->setViewport(width, height, minDepth, maxDepth);
}

void IMaterial::setViewLayerI(uint8_t layer)
{
    if (layer > RenderQueue::MaxViewLayerCount)
    {
        SPDLOG_WARN(
            "Layer value of {} is outside max allowed value ({}). Ignoring.",
            layer,
            RenderQueue::MaxViewLayerCount);
        return;
    }
    viewLayer_ = layer;
}

// ========================== client api =========================

Material::Material() = default;
Material::~Material() = default;

void IMaterial::addTexture(
    Engine* engine, Texture* texture, ImageType type, const TextureSampler& sampler)
{
    uint32_t binding = util::ecast(type);
    addImageTexture(
        reinterpret_cast<IEngine*>(engine)->driver(),
        reinterpret_cast<IMappedTexture*>(texture),
        type,
        sampler.get(),
        binding);
}

void IMaterial::addTexture(
    Engine* engine,
    void* imageBuffer,
    uint32_t width,
    uint32_t height,
    backend::TextureFormat format,
    Material::ImageType type,
    const TextureSampler& sampler)
{
    ASSERT_LOG(imageBuffer);

    IEngine* iengine = reinterpret_cast<IEngine*>(engine);
    IMappedTexture* tex = iengine->createMappedTextureI();
    tex->setTextureI(imageBuffer, width, height, 1, 1, format, backend::ImageUsage::Sampled);
    addTexture(engine, tex, type, sampler);
}

void IMaterial::updatePushConstantParam(
    const std::string& elementName, backend::ShaderStage stage, void* value)
{
    updatePushConstantParamI(elementName, stage, value);
}

void IMaterial::updateUboParam(
    const std::string& elementName, backend::ShaderStage stage, void* value)
{
    updateUboParamI(elementName, stage, value);
}

void IMaterial::addPushConstantParam(
    const std::string& elementName,
    backend::BufferElementType type,
    backend::ShaderStage stage,
    size_t size,
    void* value)
{
    addPushConstantParamI(elementName, type, stage, size, value);
}

void IMaterial::addUboParam(
    const std::string& elementName,
    backend::BufferElementType type,
    size_t size,
    size_t arrayCount,
    backend::ShaderStage stage,
    void* value)
{
    addUboParamI(elementName, type, size, arrayCount, stage, value);
}

void IMaterial::setColourBaseFactor(const util::Colour4& col) noexcept
{
    addUboParam(
        "baseColourFactor",
        backend::BufferElementType::Float4,
        sizeof(mathfu::vec4),
        1,
        backend::ShaderStage::Fragment,
        (void*)&col);
    addVariant(IMaterial::Variants::HasBaseColourFactor);
}

void IMaterial::setAlphaMask(float alphaMask) noexcept
{
    addUboParam(
        "alphaMask",
        backend::BufferElementType::Float,
        sizeof(float),
        1,
        backend::ShaderStage::Fragment,
        (void*)&alphaMask);
    addVariant(IMaterial::Variants::HasAlphaMask);
}

void IMaterial::setAlphaMaskCutOff(float cutOff) noexcept
{
    addUboParam(
        "alphaMaskCutOff",
        backend::BufferElementType::Float,
        sizeof(float),
        1,
        backend::ShaderStage::Fragment,
        (void*)&cutOff);
    addVariant(IMaterial::Variants::HasAlphaMaskCutOff);
}

void IMaterial::setMetallicFactor(float metallic) noexcept
{
    addUboParam(
        "metallicFactor",
        backend::BufferElementType::Float,
        sizeof(float),
        1,
        backend::ShaderStage::Fragment,
        &metallic);
    addVariant(IMaterial::Variants::HasMetallicFactor);
}

void IMaterial::setRoughnessFactor(float roughness) noexcept
{
    addUboParam(
        "roughnessFactor",
        backend::BufferElementType::Float,
        sizeof(float),
        1,
        backend::ShaderStage::Fragment,
        (void*)&roughness);
    addVariant(IMaterial::Variants::HasRoughnessFactor);
}

void IMaterial::setDiffuseFactor(const util::Colour4& diffuse) noexcept
{
    addUboParam(
        "diffuseFactor",
        backend::BufferElementType::Float4,
        sizeof(mathfu::vec4),
        1,
        backend::ShaderStage::Fragment,
        (void*)&diffuse);
    addVariant(IMaterial::Variants::HasDiffuseFactor);
}

void IMaterial::setSpecularFactor(const util::Colour4& spec) noexcept
{
    addUboParam(
        "specularFactor",
        backend::BufferElementType::Float4,
        sizeof(mathfu::vec4),
        1,
        backend::ShaderStage::Fragment,
        (void*)&spec);
    addVariant(IMaterial::Variants::HasSpecularFactor);
}

void IMaterial::setEmissiveFactor(const util::Colour4& emissive) noexcept
{
    addUboParam(
        "emissiveFactor",
        backend::BufferElementType::Float4,
        sizeof(mathfu::vec4),
        1,
        backend::ShaderStage::Fragment,
        (void*)&emissive);
    addVariant(IMaterial::Variants::HasEmissiveFactor);
}
void IMaterial::setMaterialFactors(const MaterialFactors& factors)
{
    setColourBaseFactor(factors.baseColourFactor);
    setEmissiveFactor(factors.emissiveFactor);
    if (getPipelineState() == Pipeline::MetallicRoughness)
    {
        setDiffuseFactor(factors.diffuseFactor);
        setSpecularFactor(factors.specularFactor);
    }
    else if (getPipelineState() == Pipeline::SpecularGlosiness)
    {
        setMetallicFactor(factors.metallicFactor);
        setRoughnessFactor(factors.roughnessFactor);
    }
    setAlphaMask(factors.alphaMask);
    setAlphaMaskCutOff(factors.alphaMaskCutOff);
}

void IMaterial::setDepthEnable(bool writeFlag, bool testFlag)
{
    setTestEnableI(testFlag);
    setWriteEnableI(writeFlag);
}

void IMaterial::setCullMode(backend::CullMode mode) { setCullModeI(backend::cullModeToVk(mode)); }

void IMaterial::setPipeline(Material::Pipeline pipeline) noexcept { setPipelineI(pipeline); }

void IMaterial::setDoubleSidedState(bool state) noexcept { setDoubleSidedStateI(state); }

void IMaterial::setViewLayer(uint8_t layer) { setViewLayerI(layer); }

void IMaterial::setBlendFactor(const Material::BlendFactorParams& factors)
{
    setBlendFactorStateI(static_cast<VkBool32>(factors.state));
    setSrcColorBlendFactorI(backend::blendFactorToVk(factors.srcColor));
    setSrcAlphaBlendFactorI(backend::blendFactorToVk(factors.srcAlpha));
    setDstColorBlendFactorI(backend::blendFactorToVk(factors.dstColor));
    setDstAlphaBlendFactorI(backend::blendFactorToVk(factors.dstAlpha));
    setColourBlendOpI(backend::blendOpToVk(factors.colour));
    setColourBlendOpI(backend::blendOpToVk(factors.alpha));
}

void IMaterial::setBlendFactor(backend::BlendFactorPresets preset)
{
    Material::BlendFactorParams params;

    if (preset == backend::BlendFactorPresets::Translucent)
    {
        params.srcColor = backend::BlendFactor::SrcAlpha;
        params.dstColor = backend::BlendFactor::OneMinusSrcAlpha;
        params.colour = backend::BlendOp::Add;
        params.srcAlpha = backend::BlendFactor::OneMinusSrcAlpha;
        params.dstAlpha = backend::BlendFactor::Zero;
        params.alpha = backend::BlendOp::Add;
        params.state = VK_TRUE;
        setBlendFactor(params);
    }
    else
    {
        SPDLOG_WARN("Unrecognised blend factor preset.");
    }
}

void IMaterial::setScissor(uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset)
{
    setScissorI(width, height, xOffset, yOffset);
}

void IMaterial::setViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    setViewportI(width, height, minDepth, maxDepth);
}

Material::ImageType IMaterial::convertImageType(ModelMaterial::TextureType type)
{
    switch (type)
    {
        case ModelMaterial::TextureType::BaseColour:
            return Material::ImageType::BaseColour;
            break;
        case ModelMaterial::TextureType::Normal:
            return Material::ImageType::Normal;
            break;
        case ModelMaterial::TextureType::Emissive:
            return Material::ImageType::Emissive;
            break;
        case ModelMaterial::TextureType::Occlusion:
            return Material::ImageType::Occlusion;
            break;
        case ModelMaterial::TextureType::MetallicRoughness:
            return Material::ImageType::MetallicRoughness;
            break;
        default:
            SPDLOG_WARN("Model texture type not supported.");
            break;
    }
    return Material::ImageType::BaseColour;
}

Material::Pipeline IMaterial::convertPipeline(ModelMaterial::PbrPipeline pipeline)
{
    Material::Pipeline output = Material::Pipeline::None;
    switch (pipeline)
    {
        case ModelMaterial::PbrPipeline::MetallicRoughness:
            output = Material::Pipeline::MetallicRoughness;
            break;
        case ModelMaterial::PbrPipeline::SpecularGlosiness:
            output = Material::Pipeline::SpecularGlosiness;
            break;
    }
    return output;
}


} // namespace yave
