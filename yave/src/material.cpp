#include "private/material.h"

#include "private/engine.h"
#include "private/mapped_texture.h"
#include "yave/texture_sampler.h"

#include <spdlog/spdlog.h>

namespace yave
{

void Material::addPushConstantParam(
    const std::string& elementName,
    backend::BufferElementType type,
    backend::ShaderStage stage,
    size_t size,
    void* value)
{
    static_cast<IMaterial*>(this)->addPushConstantParam(elementName, type, stage, value);
}

void Material::updatePushConstantParam(
    const std::string& elementName, backend::ShaderStage stage, void* value)
{
    static_cast<IMaterial*>(this)->updatePushConstantParam(elementName, stage, value);
}

void Material::addUboParam(
    const std::string& elementName,
    backend::BufferElementType type,
    size_t size,
    size_t arrayCount,
    backend::ShaderStage stage,
    void* value)
{
    static_cast<IMaterial*>(this)->addUboParam(elementName, type, arrayCount, stage, value);
}

void Material::updateUboParam(
    const std::string& elementName, backend::ShaderStage stage, void* value)
{
    static_cast<IMaterial*>(this)->updateUboParam(elementName, stage, value);
}

void Material::setColourBaseFactor(const util::Colour4& col) noexcept
{
    addUboParam(
        "baseColourFactor",
        backend::BufferElementType::Float4,
        sizeof(mathfu::vec4),
        1,
        backend::ShaderStage::Fragment,
        (void*)&col);
    static_cast<IMaterial*>(this)->addVariant(IMaterial::Variants::HasBaseColourFactor);
}

void Material::setAlphaMask(float alphaMask) noexcept
{
    addUboParam(
        "alphaMask",
        backend::BufferElementType::Float,
        sizeof(float),
        1,
        backend::ShaderStage::Fragment,
        (void*)&alphaMask);
    static_cast<IMaterial*>(this)->addVariant(IMaterial::Variants::HasAlphaMask);
}

void Material::setAlphaMaskCutOff(float cutOff) noexcept
{
    addUboParam(
        "alphaMaskCutOff",
        backend::BufferElementType::Float,
        sizeof(float),
        1,
        backend::ShaderStage::Fragment,
        (void*)&cutOff);
    static_cast<IMaterial*>(this)->addVariant(IMaterial::Variants::HasAlphaMaskCutOff);
}

void Material::setMetallicFactor(float metallic) noexcept
{
    addUboParam(
        "metallicFactor",
        backend::BufferElementType::Float,
        sizeof(float),
        1,
        backend::ShaderStage::Fragment,
        &metallic);
    static_cast<IMaterial*>(this)->addVariant(IMaterial::Variants::HasMetallicFactor);
}

void Material::setRoughnessFactor(float roughness) noexcept
{
    addUboParam(
        "roughnessFactor",
        backend::BufferElementType::Float,
        sizeof(float),
        1,
        backend::ShaderStage::Fragment,
        (void*)&roughness);
    static_cast<IMaterial*>(this)->addVariant(IMaterial::Variants::HasRoughnessFactor);
}

void Material::setDiffuseFactor(const util::Colour4& diffuse) noexcept
{
    addUboParam(
        "diffuseFactor",
        backend::BufferElementType::Float4,
        sizeof(mathfu::vec4),
        1,
        backend::ShaderStage::Fragment,
        (void*)&diffuse);
    static_cast<IMaterial*>(this)->addVariant(IMaterial::Variants::HasDiffuseFactor);
}

void Material::setSpecularFactor(const util::Colour4& spec) noexcept
{
    addUboParam(
        "specularFactor",
        backend::BufferElementType::Float4,
        sizeof(mathfu::vec4),
        1,
        backend::ShaderStage::Fragment,
        (void*)&spec);
    static_cast<IMaterial*>(this)->addVariant(IMaterial::Variants::HasSpecularFactor);
}

void Material::setEmissiveFactor(const util::Colour4& emissive) noexcept
{
    addUboParam(
        "emissiveFactor",
        backend::BufferElementType::Float4,
        sizeof(mathfu::vec4),
        1,
        backend::ShaderStage::Fragment,
        (void*)&emissive);
    static_cast<IMaterial*>(this)->addVariant(IMaterial::Variants::HasEmissiveFactor);
}

void Material::setMaterialFactors(const MaterialFactors& factors)
{
    setColourBaseFactor(factors.baseColourFactor);
    setEmissiveFactor(factors.emissiveFactor);
    if (static_cast<IMaterial*>(this)->getPipelineState() == Pipeline::MetallicRoughness)
    {
        setDiffuseFactor(factors.diffuseFactor);
        setSpecularFactor(factors.specularFactor);
    }
    else if (static_cast<IMaterial*>(this)->getPipelineState() == Pipeline::SpecularGlosiness)
    {
        setMetallicFactor(factors.metallicFactor);
        setRoughnessFactor(factors.roughnessFactor);
    }
    setAlphaMask(factors.alphaMask);
    setAlphaMaskCutOff(factors.alphaMaskCutOff);
}

void Material::setDepthEnable(bool writeFlag, bool testFlag)
{
    static_cast<IMaterial*>(this)->setTestEnable(testFlag);
    static_cast<IMaterial*>(this)->setWriteEnable(writeFlag);
}

void Material::setCullMode(backend::CullMode mode)
{
    static_cast<IMaterial*>(this)->setCullMode(backend::cullModeToVk(mode));
}

void Material::setDoubleSidedState(bool state) noexcept
{
    static_cast<IMaterial*>(this)->setDoubleSidedState(state);
}

void Material::setPipeline(Pipeline pipeline)
{
    static_cast<IMaterial*>(this)->setPipeline(pipeline);
}

void Material::setViewLayer(uint8_t layer) { static_cast<IMaterial*>(this)->setViewLayer(layer); }

Material::ImageType Material::convertImageType(ModelMaterial::TextureType type)
{
    switch (type)
    {
        case ModelMaterial::TextureType::BaseColour:
            return Material::ImageType::BaseColour;
        case ModelMaterial::TextureType::Normal:
            return Material::ImageType::Normal;
        case ModelMaterial::TextureType::Emissive:
            return Material::ImageType::Emissive;
        case ModelMaterial::TextureType::Occlusion:
            return Material::ImageType::Occlusion;
        case ModelMaterial::TextureType::MetallicRoughness:
            return Material::ImageType::MetallicRoughness;
        default:
            SPDLOG_WARN("Model texture type not supported.");
            break;
    }
    return Material::ImageType::BaseColour;
}

Material::Pipeline Material::convertPipeline(ModelMaterial::PbrPipeline pipeline)
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
        case ModelMaterial::PbrPipeline::None:
            break;
    }
    return output;
}

void Material::setBlendFactor(const BlendFactorParams& factors)
{
    auto* imat = static_cast<IMaterial*>(this);
    imat->setBlendFactorState(static_cast<VkBool32>(factors.state));
    imat->setSrcColorBlendFactor(backend::blendFactorToVk(factors.srcColor));
    imat->setSrcAlphaBlendFactor(backend::blendFactorToVk(factors.srcAlpha));
    imat->setDstColorBlendFactor(backend::blendFactorToVk(factors.dstColor));
    imat->setDstAlphaBlendFactor(backend::blendFactorToVk(factors.dstAlpha));
    imat->setColourBlendOp(backend::blendOpToVk(factors.colour));
    imat->setColourBlendOp(backend::blendOpToVk(factors.alpha));
}

void Material::setBlendFactor(backend::BlendFactorPresets preset)
{
    Material::BlendFactorParams params {};

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

void Material::setScissor(uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset)
{
    static_cast<IMaterial*>(this)->setScissor(width, height, xOffset, yOffset);
}

void Material::setViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    static_cast<IMaterial*>(this)->setViewport(width, height, minDepth, maxDepth);
}

void Material::addTexture(
    Engine* engine,
    void* imageBuffer,
    uint32_t width,
    uint32_t height,
    backend::TextureFormat format,
    ImageType type,
    backend::ShaderStage stage,
    TextureSampler& sampler)
{
    ASSERT_LOG(imageBuffer);

    auto* iengine = static_cast<IEngine*>(engine);
    IMappedTexture* tex = iengine->createMappedTexture();
    tex->setTexture(imageBuffer, width, height, 1, 1, format, backend::ImageUsage::Sampled);
    addTexture(engine, tex, type, stage, sampler);
}

void Material::addTexture(
    Engine* engine,
    Texture* texture,
    ImageType type,
    backend::ShaderStage stage,
    TextureSampler& sampler)
{
    uint32_t binding = util::ecast(type);
    static_cast<IMaterial*>(this)->addImageTexture(
        static_cast<IEngine*>(engine)->driver(),
        static_cast<IMappedTexture*>(texture),
        type,
        stage,
        sampler.get(),
        binding);
}

} // namespace yave