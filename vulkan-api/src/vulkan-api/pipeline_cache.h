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

#include "common.h"
#include "shader.h"
#include "utility/compiler.h"
#include "utility/enum_cast.h"
#include "utility/murmurhash.h"

#include <unordered_map>

namespace vkapi
{
// forward declarations
class ShaderProgramBundle;
class RenderPass;
class FrameBuffer;
class VkContext;
class GraphicsPipeline;
class PipelineLayout;
class ComputePipeline;


class PipelineCache
{
public:
    constexpr static uint32_t InitialDescriptorPoolSize = 1000;

    constexpr static uint8_t MaxSamplerBindCount = 10;
    constexpr static uint8_t MaxUboBindCount = 8;
    constexpr static uint8_t MaxUboDynamicBindCount = 4;
    constexpr static uint8_t MaxSsboBindCount = 4;
    constexpr static uint8_t MaxVertexAttributeCount = 8;
    constexpr static uint8_t MaxStorageImageBindCount = 6;

    // shader set values for each descriptor type
    constexpr static uint8_t UboSetValue = 0;
    constexpr static uint8_t UboDynamicSetValue = 1;
    constexpr static uint8_t SsboSetValue = 2;
    constexpr static uint8_t SamplerSetValue = 3;
    constexpr static uint8_t StorageImageSetValue = 4;
    constexpr static uint8_t MaxDescriptorTypeCount = 5;

    PipelineCache(VkContext& context, VkDriver& driver);
    ~PipelineCache();

    void init() noexcept;

    // Reset the descriptor and pipeline requirements to default values.
    // Should be called before beginning a new binding session.
    void resetKeys() noexcept;

    // =============== pipeline hasher ======================

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wpadded"

    struct RasterStateBlock
    {
        vk::CullModeFlagBits cullMode;
        vk::PolygonMode polygonMode;
        vk::FrontFace frontFace;
        vk::PrimitiveTopology topology;
        vk::ColorComponentFlags colorWriteMask;
        uint32_t colourAttachCount;
        VkBool32 primRestart;
        VkBool32 depthTestEnable;
        VkBool32 depthWriteEnable;
    };

    struct DepthStencilBlock
    {
        vk::CompareOp compareOp;
        vk::StencilOp stencilFailOp;
        vk::StencilOp depthFailOp;
        vk::StencilOp passOp;
        uint32_t compareMask;
        uint32_t writeMask;
        uint32_t reference;
        VkBool32 stencilTestEnable;
    };

    struct BlendFactorBlock
    {
        VkBool32 blendEnable;
        vk::BlendFactor srcColorBlendFactor;
        vk::BlendFactor dstColorBlendFactor;
        vk::BlendOp colorBlendOp;
        vk::BlendFactor srcAlphaBlendFactor;
        vk::BlendFactor dstAlphaBlendFactor;
        vk::BlendOp alphaBlendOp;
    };

    struct GraphicsPlineKey
    {
        RasterStateBlock rasterState;
        DepthStencilBlock dsBlock;
        BlendFactorBlock blendState;
        vk::RenderPass renderPass;
        vk::PipelineShaderStageCreateInfo shaders[util::ecast(backend::ShaderStage::Count)];
        vk::VertexInputAttributeDescription vertAttrDesc[MaxVertexAttributeCount];
        vk::VertexInputBindingDescription vertBindDesc[MaxVertexAttributeCount];
        size_t tesselationVertCount;

        bool operator==(const GraphicsPlineKey& rhs) const noexcept;
    };

    static_assert(
        std::is_trivially_copyable<GraphicsPlineKey>::value,
        "PipelineKey must be a POD for the hashing to work correctly");
    static_assert(
        std::is_trivially_copyable<RasterStateBlock>::value,
        "RasterStateBlock must be a POD for the hashing to work correctly");
    static_assert(
        std::is_trivially_copyable<DepthStencilBlock>::value,
        "DepthStencilBlock must be a POD for the hashing to work correctly");

    using PLineHasher = util::Murmur3Hasher<GraphicsPlineKey>;

    // ================ compute shaders ====================

    struct ComputePlineKey
    {
        vk::PipelineShaderStageCreateInfo shader;

        bool operator==(const ComputePlineKey& rhs) const noexcept;
    };

    static_assert(
        std::is_trivially_copyable<ComputePlineKey>::value,
        "ComputeKey must be a POD for the hashing to work correctly");

    using ComputePlineHasher = util::Murmur3Hasher<ComputePlineKey>;

    // =============== descriptor hasher ===================

    struct DescriptorImage
    {
        vk::ImageView imageView;
        vk::ImageLayout imageLayout;
        uint32_t padding = 0;
        vk::Sampler imageSampler;
    };

    struct DescriptorKey
    {
        vk::Buffer ubos[MaxUboBindCount];
        vk::Buffer dynamicUbos[MaxUboDynamicBindCount];
        vk::Buffer ssbos[MaxSsboBindCount];
        size_t bufferSizes[MaxUboBindCount];
        size_t dynamicBufferSizes[MaxUboDynamicBindCount];
        size_t ssboBufferSizes[MaxSsboBindCount];
        DescriptorImage samplers[MaxSamplerBindCount];
        DescriptorImage storageImages[MaxStorageImageBindCount];

        bool operator==(const DescriptorKey& rhs) const noexcept;
    };

#pragma clang diagnostic pop

    static_assert(
        std::is_trivially_copyable<DescriptorKey>::value,
        "DescriptorKey must be a POD for the hashing to work correctly");
    static_assert(
        std::is_trivially_copyable<DescriptorImage>::value,
        "DescriptorKey must be a POD for the hashing to work correctly");
    using DescriptorHasher = util::Murmur3Hasher<DescriptorKey>;

    struct DescriptorSetInfo
    {
        vk::DescriptorSetLayout layout[MaxDescriptorTypeCount] = {};
        vk::DescriptorSet descrSets[MaxDescriptorTypeCount] = {};
        uint64_t frameLastUsed = 0;
    };

    // =============== graphic pipelines ====================

    GraphicsPipeline* findOrCreateGraphicsPipeline(PipelineLayout& pipelineLayout);

    void setPipelineKeyToDefault() noexcept;

    void bindGraphicsPipeline(vk::CommandBuffer& cmdBuffer, PipelineLayout& pipelineLayout);

    void bindGraphicsShaderModules(ShaderProgramBundle& prog);

    void bindRenderPass(const vk::RenderPass& rpass);

    void bindCullMode(vk::CullModeFlagBits cullMode);
    void bindPolygonMode(vk::PolygonMode polyMode);
    void bindFrontFace(vk::FrontFace frontFace);
    void bindTopology(vk::PrimitiveTopology topo);
    void bindPrimRestart(bool state);
    void bindDepthTestEnable(bool state);
    void bindDepthWriteEnable(bool state);
    void bindDepthStencilBlock(const DepthStencilBlock& dsBlock);
    void bindColourAttachCount(uint32_t count) noexcept;
    void bindBlendFactorBlock(const BlendFactorBlock& block) noexcept;
    void bindTesselationVertCount(size_t count) noexcept;

    // =============== compute pipelines =============

    ComputePipeline* findOrCreateComputePipeline(PipelineLayout& pipelineLayout);
    void bindComputePipeline(vk::CommandBuffer& cmdBuffer, PipelineLayout& pipelineLayout);
    void bindComputeShaderModules(ShaderProgramBundle& prog);

    // ============ descriptor sets ==================

    void bindVertexInput(
        vk::VertexInputAttributeDescription* vertAttrDesc,
        vk::VertexInputBindingDescription* vertBindDesc);

    void bindUbo(uint8_t bindValue, vk::Buffer buffer, uint32_t size);
    void bindUboDynamic(uint8_t bindValue, vk::Buffer buffer, uint32_t size);
    void bindSsbo(uint8_t bindValue, vk::Buffer buffer, uint32_t size);

    static void bindScissor(vk::CommandBuffer cmdBuffer, const vk::Rect2D& newScissor);
    static void bindViewport(vk::CommandBuffer cmdBuffer, const vk::Viewport& newViewPort);
    void bindSampler(DescriptorImage descImages[MaxSamplerBindCount]);
    void bindStorageImage(DescriptorImage descImages[MaxStorageImageBindCount]);


    void bindDescriptors(
        vk::CommandBuffer& cmdBuffer,
        PipelineLayout& pipelineLayout,
        const std::vector<uint32_t>& dynamicOffsets = {},
        vk::PipelineBindPoint plineBindPoint = vk::PipelineBindPoint::eGraphics);

    void createDescriptorSets(PipelineLayout& pipelineLayout, DescriptorSetInfo& descSetInfo);
    void allocDescriptorSets(vk::DescriptorSetLayout* descLayouts, vk::DescriptorSet* descSets);
    void createDescriptorPools();
    void increasePoolCapacity();

    void cleanCache(uint64_t currentFrame);
    void clear() noexcept;

    using PipelineCacheMap =
        std::unordered_map<GraphicsPlineKey, std::unique_ptr<GraphicsPipeline>, PLineHasher>;
    using DescriptorSetCache =
        std::unordered_map<DescriptorKey, DescriptorSetInfo, DescriptorHasher>;
    using ComputePlineCacheMap =
        std::unordered_map<ComputePlineKey, std::unique_ptr<ComputePipeline>, ComputePlineHasher>;

private:
    VkContext& context_;
    VkDriver& driver_;

    PipelineCacheMap pipelines_;
    ComputePlineCacheMap computePipelines_;
    DescriptorSetCache descriptorSets_;

    /// the main descriptor pool
    vk::DescriptorPool descriptorPool_;

    uint32_t currentDescPoolSize_;

    /// current bound descriptor
    DescriptorKey boundDescriptor_;

    /// current bound pipeline.
    GraphicsPlineKey boundGraphicsPline_;
    ComputePlineKey boundComputePline_;

    /// the requirements of the current descriptor and pipelines
    GraphicsPlineKey graphicsPlineRequires_;
    ComputePlineKey computePlineRequires_;
    DescriptorKey descRequires_;

    /// A pool of descriptor sets for each descriptor type.
    /// Reference to these sets are also stored in the cache - so
    /// the cached descriptor sets must be cleared if destroying the
    /// the sets stored in this pool.
    std::array<vk::DescriptorSet, MaxDescriptorTypeCount> descSetPool_;

    // containers for storing pools and descriptor sets that are
    // waiting to be destroyed once they reach their lifetime.
    std::vector<DescriptorSetInfo> descSetsForDeletion_;
    std::vector<vk::DescriptorPool> descPoolsForDeletion_;
};

} // namespace vkapi
