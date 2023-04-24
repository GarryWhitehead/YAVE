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
#include "common.h"
#include "pipeline.h"
#include "pipeline_cache.h"
#include "resource_cache.h"
#include "shader.h"
#include "utility/bitset_enum.h"
#include "utility/compiler.h"
#include "utility/cstring.h"
#include "utility/enum_cast.h"
#include "utility/murmurhash.h"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vkapi
{
// forward declarations
class VkContext;
class VkDriver;
class PipelineLayout;

class ShaderProgram
{
public:
    void addAttributeBlock(const std::string& block);

    std::string build();

    // void addStageBlock(const std::string& block) { stageBlock_ = block; }

    void addShader(Shader* shader) noexcept { shader_ = shader; }
    Shader* getShader() noexcept { return shader_; }

    void parseMaterialShaderBlock(const std::vector<std::string>& lines, size_t& index);

    void parseShader(const std::vector<std::string>& lines);

    friend class VkDriver;

private:
    // Used for creating the glsl string representation used for compilation.
    // the main() code section
    std::string mainStageBlock_;
    // the attribute descriptors for the main code block
    std::string attributeDescriptorBlock_;
    // addition code which is specific to the materal
    std::string materialShaderBlock_;
    // additional attributes (ubos, ssbos, samplers)
    std::vector<std::string> attributeBlocks_;

    // Keep a track of include statement within the shader.
    std::vector<std::string> includes_;

    // Shaders used by this program. Note: these are not owned by the program
    // but by the cached conatiner in the program manager.
    Shader* shader_;
};

class ShaderProgramBundle
{
public:
    struct RasterState
    {
        vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eNone;
        vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
        vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
    };

    struct DepthStencilState
    {
        struct StencilState
        {
            VkBool32 useStencil = VK_FALSE;
            vk::StencilOp failOp = vk::StencilOp::eKeep;
            vk::StencilOp passOp = vk::StencilOp::eKeep;
            vk::StencilOp depthFailOp = vk::StencilOp::eKeep;
            vk::StencilOp stencilFailOp = vk::StencilOp::eKeep;
            vk::CompareOp compareOp = vk::CompareOp::eLessOrEqual;
            uint32_t compareMask = 0;
            uint32_t writeMask = 0;
            uint32_t reference = 0;
            VkBool32 frontEqualBack = VK_TRUE;
        };

        /// depth state
        VkBool32 testEnable = VK_TRUE;
        VkBool32 writeEnable = VK_TRUE;
        VkBool32 stencilTestEnable = VK_FALSE;
        vk::CompareOp compareOp = vk::CompareOp::eLessOrEqual;

        /// only processed if the above is true
        StencilState frontStencil;
        StencilState backStencil;
    };

    struct BlendFactorState
    {
        VkBool32 blendEnable = VK_FALSE;
        vk::BlendFactor srcColor = vk::BlendFactor::eZero;
        vk::BlendFactor dstColor = vk::BlendFactor::eZero;
        vk::BlendOp colour = vk::BlendOp::eAdd;
        vk::BlendFactor srcAlpha = vk::BlendFactor::eZero;
        vk::BlendFactor dstAlpha = vk::BlendFactor::eZero;
        vk::BlendOp alpha = vk::BlendOp::eAdd;
    };

    struct RenderPrimitive
    {
        uint32_t indicesCount;
        uint32_t offset;
        uint32_t vertexCount;
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
        VkBool32 primitiveRestart = VK_FALSE;
        vk::IndexType indexBufferType;
    };

    ShaderProgramBundle();
    ~ShaderProgramBundle();

    ShaderProgram* createProgram(const backend::ShaderStage& type);

    void createPushBlock(size_t size, backend::ShaderStage stage);

    std::vector<vk::PipelineShaderStageCreateInfo> getShaderStagesCreateInfo();

    void parseMaterialShader(const std::filesystem::path& shaderPath);

    void setTexture(const TextureHandle& handle, uint8_t binding, vk::Sampler sampler);

    void setPushBlockData(backend::ShaderStage stage, void* data);

    void setScissor(uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset);

    void setViewport(uint32_t width, uint32_t height, float minDepth, float maxDepth);

    void addTextureSampler(vk::Sampler sampler, uint32_t binding);

    void addRenderPrimitive(
        vk::PrimitiveTopology topo,
        vk::IndexType indexBufferType,
        uint32_t indicesCount,
        uint32_t indicesOffset,
        VkBool32 primRestart = VK_FALSE);

    // used when no indices are to be used for the draw
    void addRenderPrimitive(vk::PrimitiveTopology topo, uint32_t vertexCount, VkBool32 primRestart);

    // Used when no index buffer is to be bound.
    void addRenderPrimitive(uint32_t count);

    void buildShader(std::string filename);

    void buildShaders(const std::string& filename) { buildShader(filename); }

    template <typename... ShaderArgs>
    void buildShaders(const std::string& filename, const ShaderArgs&... shaderArgs);

    // =================== getters ============================

    ShaderProgram* getProgram(backend::ShaderStage type) noexcept;

    PipelineLayout& getPipelineLayout() noexcept;

    size_t getShaderId() const { return shaderId_; }

    friend class VkDriver;

private:
    // The id of this program bundle
    size_t shaderId_;

    std::array<std::unique_ptr<ShaderProgram>, util::ecast(backend::ShaderStage::Count)> programs_ =
        {nullptr};

    // Information about the pipeline layout associated with this shader
    // program. This is derived from shader reflecton.
    std::unique_ptr<PipelineLayout> pipelineLayout_;

    // Note: this is initialised after a call to updateDescriptorLayouts.
    std::unique_ptr<PipelineLayout::PushBlockBindParams> pushBlock_[2];

    // Index into the resource cache for the texture for each attachment.
    std::array<TextureHandle, PipelineCache::MaxSamplerBindCount> textureHandles_;

    std::array<vk::Sampler, PipelineCache::MaxSamplerBindCount> samplers_;

    // We keep a record of descriptors here and their binding info for
    // use at the pipeline binding draw stage
    struct DescriptorBindInfo
    {
        uint32_t binding;
        vk::Buffer buffer;
        uint32_t size;
        vk::DescriptorType type;
    };
    std::vector<DescriptorBindInfo> descBindInfo_;

    RenderPrimitive renderPrim_;

    vk::Rect2D scissor_;
    vk::Viewport viewport_;

public:
    // The rasterisation and depth/stencil states used for pipeline
    // binding time, hence why this information is stored here.
    RasterState rasterState_;
    DepthStencilState dsState_;
    BlendFactorState blendState_;

    void addDescriptorBinding(
        uint32_t size,
        uint32_t binding,
        vk::Buffer buffer,
        vk::DescriptorType type);
};

template <typename... ShaderArgs>
void ShaderProgramBundle::buildShaders(const std::string& filename, const ShaderArgs&... shaderArgs)
{
    buildShader(filename);
    buildShaders(shaderArgs...);
}

class ProgramManager
{

public:
   
    // =============== cached shader hasher ======================

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wpadded"

    struct CachedKey
    {
        uint64_t variantBits;
        uint32_t shaderId;
        uint32_t shaderStage;
        uint32_t topology;
        uint8_t padding[4];
    };

    static_assert(
        std::is_pod<CachedKey>::value, "CachedKey must be a POD for the hashing to work correctly");

#pragma clang diagnostic pop

private:

    using CachedHasher = util::Murmur3Hasher<CachedKey>;

    struct CachedEqual
    {
        bool operator()(const CachedKey& lhs, const CachedKey& rhs) const
        {
            return lhs.shaderId == rhs.shaderId && lhs.shaderStage == rhs.shaderStage &&
                lhs.variantBits == rhs.variantBits && lhs.topology == rhs.topology;
        }
    };

    using ShaderCacheMap =
        std::unordered_map<CachedKey, std::unique_ptr<Shader>, CachedHasher, CachedEqual>;

public:

    ProgramManager(VkDriver& driver);
    ~ProgramManager();

    ShaderProgramBundle* createProgramBundle();

    Shader* findShaderVariantOrCreate(
        const VDefinitions& variants,
        backend::ShaderStage type,
        vk::PrimitiveTopology topo,
        ShaderProgramBundle* bundle,
        uint64_t variantBits = 0);

    Shader* compileShader(
        const std::string& shaderCode,
        const backend::ShaderStage& type,
        const VDefinitions& variants,
        const CachedKey& key);

    Shader* findCachedShaderVariant(const CachedKey& key);

private:
    VkDriver& driver_;

    /// fully compiled, complete shader programs
    std::vector<std::unique_ptr<ShaderProgramBundle>> programBundles_;

    /// this is where individual shaders are cached until required to assemble
    /// into a shader program
    ShaderCacheMap shaderCache_;
};

} // namespace vkapi
