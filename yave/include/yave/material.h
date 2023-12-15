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

#include "yave_api.h"

#include "backend/enums.h"

#include <model_parser/gltf/model_material.h>
#include <utility/colour.h>

#include <string>

namespace yave
{
class TextureSampler;
class Texture;
class Engine;

class Material : public YaveApi
{
public:
    enum class ImageType
    {
        BaseColour,
        Normal,
        Occlusion,
        Emissive,
        MetallicRoughness
    };

    struct MaterialFactors
    {
        util::Colour4 baseColourFactor {0.8f};
        util::Colour4 diffuseFactor {0.4f};
        util::Colour4 specularFactor {0.2f};
        util::Colour4 emissiveFactor {0.3f};
        float roughnessFactor = 1.0f;
        float metallicFactor = 1.0f;
        float alphaMask = 1.0f;
        float alphaMaskCutOff = 1.0f;
    };

    struct BlendFactorParams
    {
        bool state;
        backend::BlendFactor srcColor;
        backend::BlendFactor dstColor;
        backend::BlendOp colour;
        backend::BlendFactor srcAlpha;
        backend::BlendFactor dstAlpha;
        backend::BlendOp alpha;
    };

    enum class Pipeline
    {
        MetallicRoughness,
        SpecularGlosiness,
        None
    };

    void addPushConstantParam(
        const std::string& elementName,
        backend::BufferElementType type,
        backend::ShaderStage stage,
        size_t size,
        void* value = nullptr);

    template <typename T>
    void addPushConstantParam(
        const std::string& elementName,
        backend::BufferElementType type,
        backend::ShaderStage stage,
        T* value)
    {
        addPushConstantParam(elementName, type, stage, sizeof(T), (void*)value);
    }

    template <typename T>
    void addPushConstantParam(
        const std::string& elementName,
        backend::BufferElementType type,
        backend::ShaderStage stage,
        T& value)
    {
        addPushConstantParam(elementName, type, stage, sizeof(T), (void*)&value);
    }

    void updatePushConstantParam(
        const std::string& elementName, backend::ShaderStage stage, void* value);

    template <typename T>
    void
    updatePushConstantParam(const std::string& elementName, backend::ShaderStage stage, T* value)
    {
        updatePushConstantParam(elementName, stage, (void*)value);
    }

    template <typename T>
    void
    updatePushConstantParam(const std::string& elementName, backend::ShaderStage stage, T& value)
    {
        updatePushConstantParam(elementName, stage, (void*)&value);
    }

    void addUboParam(
        const std::string& elementName,
        backend::BufferElementType type,
        size_t size,
        size_t arrayCount,
        backend::ShaderStage stage,
        void* value);

    template <typename T>
    void addUboParam(
        const std::string& elementName,
        backend::BufferElementType type,
        backend::ShaderStage stage,
        T* value = nullptr)
    {
        addUboParam(elementName, type, sizeof(T), 1, stage, (void*)value);
    }

    template <typename T>
    void addUboParam(
        const std::string& elementName,
        backend::BufferElementType type,
        backend::ShaderStage stage,
        T& value)
    {
        addUboParam(elementName, type, sizeof(T), 1, stage, (void*)&value);
    }

    template <typename T>
    void addUboArrayParam(
        const std::string& elementName,
        backend::BufferElementType type,
        size_t arrayCount,
        backend::ShaderStage stage,
        T* value)
    {
        addUboParam(elementName, type, sizeof(T), arrayCount, stage, (void*)value);
    }

    void
    updateUboParam(const std::string& elementName, backend::ShaderStage stage, void* value);

    template <typename T>
    void updateUboParam(const std::string& elementName, T* value)
    {
        updateUboParam(elementName, (void*)value);
    }

    template <typename T>
    void updateUboParam(const std::string& elementName, T& value)
    {
        updateUboParam(elementName, (void*)&value);
    }

    void setColourBaseFactor(const util::Colour4& col) noexcept;
    void setAlphaMask(float alphaMask) noexcept;
    void setAlphaMaskCutOff(float cutOff) noexcept;
    void setMetallicFactor(float metallic) noexcept;
    void setRoughnessFactor(float roughness) noexcept;
    void setDiffuseFactor(const util::Colour4& diffuse) noexcept;
    void setSpecularFactor(const util::Colour4& spec) noexcept;
    void setEmissiveFactor(const util::Colour4& emissive) noexcept;
    void setMaterialFactors(const MaterialFactors& factors);

    void setDepthEnable(bool writeFlag, bool testFlag);

    void setCullMode(backend::CullMode mode);

    void setDoubleSidedState(bool state) noexcept;

    void setPipeline(Pipeline pipeline);

    void setViewLayer(uint8_t layer);

    ImageType convertImageType(ModelMaterial::TextureType type);

    Pipeline convertPipeline(ModelMaterial::PbrPipeline pipeline);

    void setBlendFactor(const BlendFactorParams& factors);

    void setBlendFactor(backend::BlendFactorPresets preset);

    void
    setScissor(uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset);

    void setViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth);

    void addTexture(
        Engine* engine,
        void* imageBuffer,
        uint32_t width,
        uint32_t height,
        backend::TextureFormat format,
        ImageType type,
        backend::ShaderStage stage,
        TextureSampler& sampler);

    void addTexture(
        Engine* engine,
        Texture* texture,
        ImageType type,
        backend::ShaderStage stage,
        TextureSampler& sampler);

protected:

    Material() = default;
    ~Material() = default;
};

} // namespace yave
