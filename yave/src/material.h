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

#pragma once

#include "backend/enums.h"
#include "samplerset.h"
#include "uniform_buffer.h"
#include "utility/assertion.h"
#include "utility/cstring.h"
#include "utility/handle.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/pipeline_cache.h"
#include "vulkan-api/program_manager.h"
#include "vulkan-api/shader.h"
#include "vulkan-api/texture.h"
#include "yave/material.h"

#include <array>
#include <memory>
#include <string>

namespace yave
{
// forward declerations
class ShaderProgram;
class VkDriver;
class IMaterial;
class IEngine;
class IRenderable;
class IMappedTexture;
class IRenderPrimitive;
class IScene;

using MaterialHandle = util::Handle<IMaterial>;

class IMaterial : public Material
{
public:
    constexpr static int VertexUboBindPoint = 4;
    constexpr static int FragmentUboBindPoint = 5;

    constexpr static int MaxSamplerCount = 6;

    enum Variants : uint64_t
    {
        MrPipeline,
        SpecularPipeline,
        HasBaseColourSampler,
        HasNormalSampler,
        HasMrSampler,
        HasOcclusionSampler,
        HasEmissiveSampler,
        HasBaseColourFactor,
        HasAlphaMask,
        HasAlphaMaskCutOff,
        HasMetallicFactor,
        HasRoughnessFactor,
        HasEmissiveFactor,
        HasSpecularFactor,
        HasDiffuseFactor,
        EnableGBufferPipeline,
        __SENTINEL__
    };

    IMaterial(IEngine& engine);
    ~IMaterial();

    // no copy allowed
    IMaterial(const IMaterial&) = delete;
    IMaterial& operator=(const IMaterial&) = delete;
    IMaterial(IMaterial&&) = default;
    IMaterial& operator=(IMaterial&&) = default;

    static vkapi::VDefinitions createVariants(util::BitSetEnum<IMaterial::Variants>& bits);

    void addVariant(const Material::ImageType type);
    void addVariant(Variants variant);

    void addBuffer(BufferBase* buffer, backend::ShaderStage type);

    void addImageTexture(
        vkapi::VkDriver& driver,
        IMappedTexture* texture,
        Material::ImageType type,
        backend::ShaderStage stage,
        backend::TextureSamplerParams& params,
        uint32_t binding);

    // Uses custom image sampler naming - but doesn't set the
    // shader variant (i.e. base colour).
    void addImageTexture(
        const std::string& samplerName,
        vkapi::VkDriver& driver,
        const vkapi::TextureHandle& handle,
        backend::ShaderStage stage,
        const backend::TextureSamplerParams& params,
        uint32_t binding);

    // and this overload is for using when the sampler
    // set has already been defined (does a lookup using
    // the samplerName)
    void addImageTexture(
        const std::string& samplerName,
        vkapi::VkDriver& driver,
        const vkapi::TextureHandle& handle,
        backend::ShaderStage stage,
        const backend::TextureSamplerParams& params);

    void build(
        IEngine& engine,
        IScene& scene,
        IRenderable& renderable,
        IRenderPrimitive* prim,
        const std::string& matShader,
        const std::string& mainShaderPath);

    void update(IEngine& engine) noexcept;

    void updateUboParamI(const std::string& name, backend::ShaderStage stage, void* value);

    void addUboParamI(
        const std::string& elementName,
        backend::BufferElementType type,
        size_t arrayCount,
        backend::ShaderStage stage,
        void* value);

    void updatePushConstantParamI(const std::string& name, backend::ShaderStage stage, void* value)
    {
        pushBlock_[util::ecast(stage)]->updateElement(name, value);
    }

    void addPushConstantParamI(
        const std::string& elementName,
        backend::BufferElementType type,
        backend::ShaderStage stage,
        void* value)
    {
        pushBlock_[util::ecast(stage)]->addElement(elementName, type, (void*)value);
    }

    void setSamplerParamI(
        const std::string& name,
        uint8_t binding,
        backend::ShaderStage stage,
        SamplerSet::SamplerType type)
    {
        // all samplers use the same set
        samplerSet_[util::ecast(stage)].pushSampler(
            name, vkapi::PipelineCache::SamplerSetValue, binding, type);
    }

    // ====== material state setters ========

    void setDoubleSidedStateI(bool state);
    void setTestEnableI(bool state);
    void setWriteEnableI(bool state);
    void setDepthCompareOpI(vk::CompareOp op);
    void setPolygonModeI(vk::PolygonMode mode);
    void setFrontFaceI(vk::FrontFace face);
    void setCullModeI(vk::CullModeFlagBits mode);
    void setPipelineI(Material::Pipeline pipeline) noexcept;

    // blend factor states
    void setBlendFactorStateI(VkBool32 state);
    void setSrcColorBlendFactorI(vk::BlendFactor factor);
    void setDstColorBlendFactorI(vk::BlendFactor factor);
    void setColourBlendOpI(vk::BlendOp op);
    void setSrcAlphaBlendFactorI(vk::BlendFactor factor);
    void setDstAlphaBlendFactorI(vk::BlendFactor factor);
    void setAlphaBlendOpI(vk::BlendOp op);

    void setScissorI(uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset);
    void setViewportI(uint32_t width, uint32_t height, float minDepth, float maxDepth);

    // defines the draw ordering of the material
    void setViewLayerI(uint8_t layer);

    void withDynamicMeshTransformUbo(bool state) noexcept { withDynMeshTransformUbo_ = state; }

    // ================= getters ===========================

    vkapi::ShaderProgramBundle* getProgram() noexcept { return programBundle_; }

    Material::Pipeline getPipelineState() noexcept;

    uint8_t getViewLayer() const noexcept { return viewLayer_; }
    uint32_t getPipelineId() const noexcept { return pipelineId_; }

    // ================= client api =========================

    void setColourBaseFactor(const util::Colour4& col) noexcept override;
    void setAlphaMask(float alphaMask) noexcept override;
    void setAlphaMaskCutOff(float cutOff) noexcept override;
    void setMetallicFactor(float metallic) noexcept override;
    void setRoughnessFactor(float roughness) noexcept override;
    void setDiffuseFactor(const util::Colour4& diffuse) noexcept override;
    void setSpecularFactor(const util::Colour4& spec) noexcept override;
    void setEmissiveFactor(const util::Colour4& emissive) noexcept override;
    void setMaterialFactors(const MaterialFactors& factors) override;
    void setDepthEnable(bool writeFlag, bool testFlag) override;
    void setCullMode(backend::CullMode mode) override;
    void setBlendFactor(const BlendFactorParams& factors) override;
    void setBlendFactor(backend::BlendFactorPresets preset) override;
    void setDoubleSidedState(bool state) noexcept override;
    void setPipeline(Material::Pipeline pipeline) noexcept override;
    void setViewLayer(uint8_t layer) override;
    ImageType convertImageType(ModelMaterial::TextureType type) override;
    Pipeline convertPipeline(ModelMaterial::PbrPipeline pipeline) override;

    void setScissor(uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset) override;

    void setViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth) override;

    void addPushConstantParam(
        const std::string& elementName,
        backend::BufferElementType type,
        backend::ShaderStage stage,
        size_t size,
        void* value) override;

    void addUboParam(
        const std::string& elementName,
        backend::BufferElementType type,
        size_t size,
        size_t arrayCount,
        backend::ShaderStage stage,
        void* value) override;

    void updatePushConstantParam(
        const std::string& elementName, backend::ShaderStage stage, void* value) override;

    void updateUboParam(
        const std::string& elementName, backend::ShaderStage stage, void* value) override;

    void addTexture(
        Engine* engine,
        void* imageBuffer,
        uint32_t width,
        uint32_t height,
        backend::TextureFormat format,
        ImageType type,
        backend::ShaderStage stage,
        TextureSampler& sampler) override;

    void addTexture(
        Engine* engine,
        Texture* texture,
        ImageType type,
        backend::ShaderStage stage,
        TextureSampler& sampler) override;

private:
    // handle to ourself
    MaterialHandle handle_;

    // shader variants associated with this material
    util::BitSetEnum<Variants> variantBits_;

    // used to generate the push block for the material shader -
    // push blocks are allowed for the vertex (0) and fragment (1)
    // stages at present.
    std::unique_ptr<PushBlock> pushBlock_[util::ecast(backend::ShaderStage::Count)];

    // uniform buffers - only vertex and fragment shader for now
    std::unique_ptr<UniformBuffer> ubos_[util::ecast(backend::ShaderStage::Count)];

    std::vector<std::pair<backend::ShaderStage, BufferBase*>> buffers_;

    // used to generate the samplers for this material
    SamplerSet samplerSet_[util::ecast(backend::ShaderStage::Count)];

    bool doubleSided_;

    // states whether to add the dynamic mesh transform buffer to the shader
    bool withDynMeshTransformUbo_;

    // used for the sorting key
    // pipeline id is a hash of the pipeline key
    uint32_t pipelineId_;
    uint8_t viewLayer_;

    // ============== vulkan backend stuff =======================

    /// details for rendering this material
    vkapi::ShaderProgramBundle* programBundle_;

    /// the sampler descriptor bindings
    vkapi::PipelineCache::DescriptorImage samplers_[vkapi::PipelineCache::MaxSamplerBindCount];
};
} // namespace yave
