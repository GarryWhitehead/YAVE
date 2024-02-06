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
#include "vulkan-api/pipeline_cache.h"
#include "vulkan-api/sampler_cache.h"
#include "vulkan-api/utility.h"
#include "yave/texture_sampler.h"

#include <spdlog/spdlog.h>

namespace yave
{

IMaterial::IMaterial(IEngine& engine)
    : doubleSided_(false), withDynMeshTransformUbo_(true), pipelineId_(0), viewLayer_(0x2)
{
    for (int i = 0; i < util::ecast(backend::ShaderStage::Count); ++i)
    {
        auto stage = static_cast<backend::ShaderStage>(i);
        std::string shaderName = vkapi::Shader::shaderTypeToString(stage);

        std::string pushBlockName = shaderName + "PushBlock";
        std::string uboName = shaderName + "Ubo";

        pushBlock_[i] = std::make_unique<PushBlock>(pushBlockName, "push_params");

        // this is the bind value offset of the ubo buffers for the materials to avoid clashes with
        // other buffer bind points - shpuld probably be calculated in a more robust way.
        size_t offset = 4;
        ubos_[i] = std::make_unique<UniformBuffer>(
            vkapi::PipelineCache::UboSetValue, i + offset, uboName, "material_ubo");
    }

    programBundle_ = engine.driver().progManager().createProgramBundle();

    // default workflow is to use gbuffers for rendering.
    variantBits_.setBit(Variants::EnableGBufferPipeline);
}

IMaterial::~IMaterial() = default;

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

void IMaterial::setPipeline(Material::Pipeline pipeline) noexcept
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
    backend::ShaderStage stage,
    backend::TextureSamplerParams& params,
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

    setSamplerParam(imageTypeToStr(type), binding, stage, samplerType);

    params.mipLevels = texture->getMipLevels();
    programBundle_->setImageSampler(
        texture->getBackendHandle(), binding, driver.getSamplerCache().createSampler(params));
}

void IMaterial::addImageTexture(
    const std::string& samplerName,
    vkapi::VkDriver& driver,
    const vkapi::TextureHandle& handle,
    backend::ShaderStage stage,
    const backend::TextureSamplerParams& params,
    uint32_t binding)
{
    setSamplerParam(samplerName, binding, stage, SamplerSet::SamplerType::e2d);

    programBundle_->setImageSampler(
        handle, binding, driver.getSamplerCache().createSampler(params));
}

void IMaterial::addImageTexture(
    const std::string& samplerName,
    vkapi::VkDriver& driver,
    const vkapi::TextureHandle& handle,
    backend::ShaderStage stage,
    const backend::TextureSamplerParams& params)
{
    uint32_t binding = samplerSet_[util::ecast(stage)].getSamplerBinding(samplerName);
    programBundle_->setImageSampler(
        handle, binding, driver.getSamplerCache().createSampler(params));
}

void IMaterial::addBuffer(BufferBase* buffer, backend::ShaderStage type)
{
    ASSERT_FATAL(buffer, "Buffer is NULL.");
    buffers_.emplace_back(type, buffer);
}

void IMaterial::build(
    IEngine& engine,
    IScene& scene,
    IRenderable& renderable,
    IRenderPrimitive* prim,
    const std::string& matShader,
    const std::string& mainShaderPath)
{
    auto& manager = engine.driver().progManager();
    auto& driver = engine.driver();

    bool withTesselationStages = renderable.getTesselationVertCount() > 0;

    // if we have already built the shader programs for this material, then
    // don't waste time rebuilding everything.
    if (!programBundle_->hasProgram(backend::ShaderStage::Vertex) &&
        !programBundle_->hasProgram(backend::ShaderStage::Fragment))
    {
        std::string vertPath = mainShaderPath + ".vert";
        std::string fragPath = mainShaderPath + ".frag";
        auto vertShaderCode = vkapi::ShaderProgramBundle::loadShader(vertPath.c_str());
        auto fragShaderCode = vkapi::ShaderProgramBundle::loadShader(fragPath.c_str());
        ASSERT_FATAL(
            !vertShaderCode.empty() && !fragShaderCode.empty(),
            "Error whilst loading material vert/frag shaders.");

        // create the material shaders to start.
        programBundle_->parseMaterialShader(matShader);
        programBundle_->buildShaders(
            vertShaderCode,
            backend::ShaderStage::Vertex,
            fragShaderCode,
            backend::ShaderStage::Fragment);

        if (withTesselationStages)
        {
            std::string tessePath = mainShaderPath + ".tesse";
            std::string tesscPath = mainShaderPath + ".tessc";
            auto tesseShaderCode = vkapi::ShaderProgramBundle::loadShader(tessePath.c_str());
            auto tesscShaderCode = vkapi::ShaderProgramBundle::loadShader(tesscPath.c_str());
            ASSERT_FATAL(
                !tesseShaderCode.empty() && !tesscShaderCode.empty(),
                "Error whilst loading material tesselation eval/control shaders.");

            programBundle_->buildShaders(
                tesseShaderCode,
                backend::ShaderStage::TesselationEval,
                tesscShaderCode,
                backend::ShaderStage::TesselationCon);
        }
    }
    programBundle_->clear();
    buffers_.clear();

    // add any additional buffer elements, push blocks or image samplers
    // to the appropriate shader before building
    auto addElements =
        [this, &driver, &scene](backend::ShaderStage stage, vkapi::ShaderProgram* prog) {
            size_t idx = util::ecast(stage);

            // only the scene ubo is added by default - all other uniforms are optional
            // to increase the usability of materials.
            addBuffer(&scene.getSceneUbo().get(), stage);

            for (const auto& [s, buffer] : buffers_)
            {
                if (prog && stage == s)
                {
                    prog->addAttributeBlock(buffer->createShaderStr());

                    auto params = buffer->getBufferParams(driver);
                    ASSERT_FATAL(params.buffer, "Vulkan buffer handle is invalid.");

                    programBundle_->addDescriptorBinding(
                        params.size, params.binding, params.buffer, params.type);
                }
            }

            // uniform buffers
            if (!ubos_[idx]->empty())
            {
                ubos_[idx]->createGpuBuffer(driver);
                auto uboParams = ubos_[idx]->getBufferParams(driver);
                programBundle_->addDescriptorBinding(
                    uboParams.size, uboParams.binding, uboParams.buffer, uboParams.type);
            }

            // add ubo and push block strings to shader block
            if (prog)
            {
                PushBlock* pushBlock = pushBlock_[idx].get();
                prog->addAttributeBlock(pushBlock->createShaderStr());
                prog->addAttributeBlock(ubos_[idx]->createShaderStr());
                prog->addAttributeBlock(samplerSet_[idx].createShaderStr());
            }
        };

    auto* vProgram = programBundle_->getProgram(backend::ShaderStage::Vertex);
    auto* fProgram = programBundle_->getProgram(backend::ShaderStage::Fragment);

    if (withDynMeshTransformUbo_)
    {
        addBuffer(&scene.getTransUbo(), backend::ShaderStage::Vertex);
    }

    addElements(backend::ShaderStage::Vertex, vProgram);
    addElements(backend::ShaderStage::Fragment, fProgram);

    if (withTesselationStages)
    {
        auto* tesseProgram = programBundle_->getProgram(backend::ShaderStage::TesselationEval);
        auto* tesscProgram = programBundle_->getProgram(backend::ShaderStage::TesselationCon);
        addElements(backend::ShaderStage::TesselationEval, tesseProgram);
        addElements(backend::ShaderStage::TesselationCon, tesscProgram);
    }

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

    // create the vertex shader (renderable)
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
    if (!scene.withGbuffer())
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

    // create the tesselation shaders if required (no variants at present supported)
    if (withTesselationStages)
    {
        programBundle_->setTesselationVertCount(renderable.getTesselationVertCount());

        vkapi::Shader* tesseShader = manager.findShaderVariantOrCreate(
            vertexVariants,
            backend::ShaderStage::TesselationEval,
            prim->getTopology(),
            programBundle_);
        programBundle_->getProgram(backend::ShaderStage::TesselationEval)->addShader(tesseShader);

        vkapi::Shader* tesscShader = manager.findShaderVariantOrCreate(
            vertexVariants,
            backend::ShaderStage::TesselationCon,
            prim->getTopology(),
            programBundle_);
        programBundle_->getProgram(backend::ShaderStage::TesselationCon)->addShader(tesscShader);
    }
}

void IMaterial::update(IEngine& engine) noexcept
{
    // TODO: could do with dirty flags here so we aren't updating data
    // that hasn't changed.
    // update the ubos and push blocks
    for (int i = 0; i < util::ecast(backend::ShaderStage::Count); ++i)
    {
        auto stage = static_cast<backend::ShaderStage>(i);
        PushBlock* pushBlock = pushBlock_[i].get();
        if (!pushBlock->empty())
        {
            void* data = pushBlock->getBlockData();
            programBundle_->setPushBlockData(stage, data);
        }

        if (!ubos_[i]->empty())
        {
            ubos_[i]->createGpuBuffer(engine.driver());
            ubos_[i]->mapGpuBuffer(engine.driver(), ubos_[i]->getBlockData());
        }
    }
}

void IMaterial::updateUboParam(const std::string& name, backend::ShaderStage stage, void* value)
{
    ubos_[util::ecast(stage)]->updateElement(name, value);
}

void IMaterial::addUboParam(
    const std::string& elementName,
    backend::BufferElementType type,
    size_t arrayCount,
    backend::ShaderStage stage,
    void* value)
{
    ubos_[util::ecast(stage)]->addElement(elementName, type, (void*)value, arrayCount);
}

void IMaterial::setDoubleSidedState(bool state)
{
    doubleSided_ = state;
    programBundle_->rasterState_.cullMode =
        state ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;
}

void IMaterial::setTestEnable(bool state) { programBundle_->dsState_.testEnable = state; }

void IMaterial::setWriteEnable(bool state) { programBundle_->dsState_.writeEnable = state; }

void IMaterial::setDepthCompareOp(vk::CompareOp op) { programBundle_->dsState_.compareOp = op; }

void IMaterial::setPolygonMode(vk::PolygonMode mode)
{
    programBundle_->rasterState_.polygonMode = mode;
}

void IMaterial::setFrontFace(vk::FrontFace face) { programBundle_->rasterState_.frontFace = face; }

void IMaterial::setCullMode(vk::CullModeFlagBits mode)
{
    programBundle_->rasterState_.cullMode = mode;
}

void IMaterial::setBlendFactorState(VkBool32 state)
{
    programBundle_->blendState_.blendEnable = state;
}

void IMaterial::setSrcColorBlendFactor(vk::BlendFactor factor)
{
    programBundle_->blendState_.srcColor = factor;
}

void IMaterial::setDstColorBlendFactor(vk::BlendFactor factor)
{
    programBundle_->blendState_.dstColor = factor;
}
void IMaterial::setColourBlendOp(vk::BlendOp op) { programBundle_->blendState_.colour = op; }

void IMaterial::setSrcAlphaBlendFactor(vk::BlendFactor factor)
{
    programBundle_->blendState_.srcAlpha = factor;
}

void IMaterial::setDstAlphaBlendFactor(vk::BlendFactor factor)
{
    programBundle_->blendState_.dstAlpha = factor;
}

void IMaterial::setAlphaBlendOp(vk::BlendOp op) { programBundle_->blendState_.alpha = op; }

void IMaterial::setScissor(uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset)
{
    programBundle_->setScissor(width, height, xOffset, yOffset);
}

void IMaterial::setViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    programBundle_->setViewport(width, height, minDepth, maxDepth);
}

void IMaterial::setViewLayer(uint8_t layer)
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

} // namespace yave
