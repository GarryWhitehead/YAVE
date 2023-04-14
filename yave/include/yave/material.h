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

#include <utility/colour.h>

#include <model_parser/gltf/model_material.h>

#include <string>

namespace yave
{
class TextureSampler;
class Texture;
class Engine;

class Material
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

    ~Material();
    
    virtual void addPushConstantParam(
        const std::string& elementName,
        backend::BufferElementType type,
        backend::ShaderStage stage,
        size_t size,
        void* value = nullptr) = 0;

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
        addPushConstantParam(
            elementName, type, stage, sizeof(T), (void*)&value);
    }

    virtual void updatePushConstantParam(
        const std::string& elementName,
        backend::ShaderStage stage,
        void* value) = 0;

    template <typename T>
    void updatePushConstantParam(
        const std::string& elementName,
        backend::ShaderStage stage,
        T* value)
    {
        updatePushConstantParam(elementName, stage, (void*)value);
    }

    template <typename T>
    void updatePushConstantParam(
        const std::string& elementName,
        backend::ShaderStage stage,
        T& value)
    {
        updatePushConstantParam(elementName, stage, (void*)&value);
    }

    virtual void addUboParam(
        const std::string& elementName,
        backend::BufferElementType type,
        size_t size,
        void* value) = 0;

    template <typename T>
    void addUboParam(
        const std::string& elementName,
        backend::BufferElementType type,
        T* value = nullptr)
    {
        addUboParam(elementName, type, sizeof(T), (void*)value);
    }

    template <typename T>
    void addUboParam(
        const std::string& elementName,
        backend::BufferElementType type,
        T& value)
    {
        addUboParam(elementName, type, sizeof(T), (void*)&value);
    }

    virtual void updateUboParam(
    const std::string& elementName,
    void* value) = 0;

    template <typename T>
    void updateUboParam(
        const std::string& elementName,
        T* value)
    {
        updateUboParam(elementName, (void*)value);
    }

    template <typename T>
    void updateUboParam(
        const std::string& elementName,
        T& value)
    {
        updateUboParam(elementName, (void*)&value);
    }

    virtual void setColourBaseFactor(const util::Colour4& col) noexcept = 0;
    virtual void setAlphaMask(float alphaMask) noexcept = 0;
    virtual void setAlphaMaskCutOff(float cutOff) noexcept = 0;
    virtual void setMetallicFactor(float metallic) noexcept = 0;
    virtual void setRoughnessFactor(float roughness) noexcept = 0;
    virtual void setDiffuseFactor(const util::Colour4& diffuse) noexcept = 0;
    virtual void setSpecularFactor(const util::Colour4& spec) noexcept = 0;
    virtual void setEmissiveFactor(const util::Colour4& emissive) noexcept = 0;
    virtual void setMaterialFactors(const MaterialFactors& factors) = 0;

    virtual void setDepthEnable(bool writeFlag, bool testFlag) = 0;

    virtual void setCullMode(backend::CullMode mode) = 0;

    virtual void setDoubleSidedState(bool state) noexcept = 0;

    virtual void setPipeline(Pipeline pipeline) = 0;

    virtual void setViewLayer(uint8_t layer) = 0;

    virtual ImageType convertImageType(ModelMaterial::TextureType type) = 0;

    virtual Pipeline convertPipeline(ModelMaterial::PbrPipeline pipeline) = 0;

    virtual void setBlendFactor(const BlendFactorParams& factors) = 0;

    virtual void setBlendFactor(backend::BlendFactorPresets preset) = 0;

    virtual void setScissor(
        uint32_t width,
        uint32_t height,
        uint32_t xOffset,
        uint32_t yOffset) = 0;

    virtual void setViewport(
        uint32_t width, uint32_t height, float minDepth, float maxDepth) = 0;

   virtual void addTexture(
        Engine* engine,
        void* imageBuffer,
        uint32_t width,
        uint32_t height,
        backend::TextureFormat format,
        ImageType type,
        const TextureSampler& sampler) = 0;

    virtual void addTexture(
       Engine* engine,
       Texture* texture,
       ImageType type,
       const TextureSampler& sampler) = 0;

protected:
    Material();
};

}
