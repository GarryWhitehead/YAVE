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

#include "pipeline_cache.h"

#include "buffer.h"
#include "context.h"
#include "driver.h"
#include "image.h"
#include "pipeline.h"
#include "program_manager.h"
#include "texture.h"
#include "utility.h"

#include <spdlog/spdlog.h>
#include <utility/assertion.h>

namespace vkapi
{

PipelineCache::PipelineCache(VkContext& context, VkDriver& driver)
    : context_(context), driver_(driver), currentDescPoolSize_(InitialDescriptorPoolSize)
{
    setPipelineKeyToDefault();
}

PipelineCache::~PipelineCache() {}

bool PipelineCache::PipelineKey::operator==(const PipelineKey& rhs) const noexcept
{
    // this compare could be one big memcpy but for debugging purposes its
    // easier to split the comparisons into their component parts.
    if (memcmp(&rasterState, &rhs.rasterState, sizeof(RasterStateBlock)) != 0)
    {
        return false;
    }
    if (memcmp(&dsBlock, &rhs.dsBlock, sizeof(DepthStencilBlock)) != 0)
    {
        return false;
    }
    for (int idx = 0; idx < util::ecast(backend::ShaderStage::Count); ++idx)
    {
        if (shaders[idx] != rhs.shaders[idx])
        {
            return false;
        }
    }
    for (int idx = 0; idx < MaxVertexAttributeCount; ++idx)
    {
        if (vertAttrDesc[idx] != rhs.vertAttrDesc[idx] ||
            vertBindDesc[idx] != rhs.vertBindDesc[idx])
        {
            return false;
        }
    }
    if (renderPass != rhs.renderPass)
    {
        return false;
    }
    return true;
}

bool PipelineCache::DescriptorKey::operator==(const DescriptorKey& rhs) const noexcept
{
    for (int idx = 0; idx < MaxUboBindCount; ++idx)
    {
        if (ubos[idx] != rhs.ubos[idx] || bufferSizes[idx] != rhs.bufferSizes[idx])
        {
            return false;
        }
    }
    for (int idx = 0; idx < MaxUboDynamicBindCount; ++idx)
    {
        if (dynamicUbos[idx] != rhs.dynamicUbos[idx] ||
            dynamicBufferSizes[idx] != rhs.dynamicBufferSizes[idx])
        {
            return false;
        }
    }
    for (int idx = 0; idx < MaxSsboBindCount; ++idx)
    {
        if (ssbos[idx] != rhs.ssbos[idx] || ssboBufferSizes[idx] != rhs.ssboBufferSizes[idx])
        {
            return false;
        }
    }
    for (int idx = 0; idx < MaxSamplerBindCount; ++idx)
    {
        if (samplers[idx].imageLayout != rhs.samplers[idx].imageLayout ||
            samplers[idx].imageSampler != rhs.samplers[idx].imageSampler ||
            samplers[idx].imageView != rhs.samplers[idx].imageView)
        {
            return false;
        }
    }
    return true;
}

void PipelineCache::init() noexcept
{
    // allocate pools for all descriptor types
    createDescriptorPools();
}

void PipelineCache::resetKeys() noexcept
{
    for (int i = 0; i < MaxUboBindCount; ++i)
    {
        descRequires_.ubos[i] = VK_NULL_HANDLE;
        descRequires_.bufferSizes[i] = 0;
    }
    for (int i = 0; i < MaxUboDynamicBindCount; ++i)
    {
        descRequires_.dynamicUbos[i] = VK_NULL_HANDLE;
        descRequires_.dynamicBufferSizes[i] = 0;
    }
    for (int i = 0; i < MaxUboDynamicBindCount; ++i)
    {
        descRequires_.ssbos[i] = VK_NULL_HANDLE;
        descRequires_.ssboBufferSizes[i] = 0;
    }
    for (int i = 0; i < MaxSamplerBindCount; ++i)
    {
        descRequires_.samplers[i] = {};
    }
}

void PipelineCache::setPipelineKeyToDefault() noexcept
{
    RasterStateBlock& rsBlock = pipelineRequires_.rasterState;
    rsBlock.cullMode = vk::CullModeFlagBits::eFront;
    rsBlock.polygonMode = vk::PolygonMode::eFill;
    rsBlock.frontFace = vk::FrontFace::eCounterClockwise;
    rsBlock.primRestart = VK_FALSE;
    rsBlock.depthTestEnable = VK_FALSE;
    rsBlock.depthWriteEnable = VK_FALSE;
    rsBlock.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    BlendFactorBlock bfBlock = pipelineRequires_.blendState;
    bfBlock.srcColorBlendFactor = vk::BlendFactor::eZero;
    bfBlock.dstColorBlendFactor = vk::BlendFactor::eZero;
    bfBlock.colorBlendOp = vk::BlendOp::eAdd;
    bfBlock.srcAlphaBlendFactor = vk::BlendFactor::eZero;
    bfBlock.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    bfBlock.alphaBlendOp = vk::BlendOp::eAdd;
    bfBlock.blendEnable = VK_FALSE;

    DepthStencilBlock& dsBlock = pipelineRequires_.dsBlock;
    dsBlock.stencilTestEnable = VK_FALSE;
    dsBlock.compareOp = vk::CompareOp::eLessOrEqual;
    dsBlock.stencilFailOp = vk::StencilOp::eZero;
    dsBlock.depthFailOp = vk::StencilOp::eZero;
    dsBlock.passOp = vk::StencilOp::eZero;
    dsBlock.compareMask = 0;
    dsBlock.writeMask = 0;
    dsBlock.reference = 0;

    for (int i = 0; i < util::ecast(backend::ShaderStage::Count); ++i)
    {
        pipelineRequires_.shaders[i].pName = nullptr;
    }
    for (int i = 0; i < MaxVertexAttributeCount; ++i)
    {
        pipelineRequires_.vertAttrDesc[i].format = vk::Format::eUndefined;
    }
}

void PipelineCache::bindPipeline(vk::CommandBuffer& cmdBuffer, PipelineLayout& pipelineLayout)
{
    // check if the required pipeline is already bound. If so, nothing to do
    // here.
//    if (boundPipeline_ == pipelineRequires_)
  //  {
   //     pipelines_[boundPipeline_]->lastUsedFrameStamp_ = driver_.getCurrentFrame();
   //     setPipelineKeyToDefault();
   //     return;
   // }

    Pipeline* pline = findOrCreatePipeline(pipelineLayout);
    ASSERT_FATAL(pline, "When trying to find or create pipeline, returned nullptr.");

    pline->lastUsedFrameStamp_ = driver_.getCurrentFrame();

    vk::PipelineBindPoint bindPoint = Pipeline::createBindPoint(pline->getType());
    cmdBuffer.bindPipeline(bindPoint, pline->get());

    boundPipeline_ = pipelineRequires_;
    setPipelineKeyToDefault();
}

Pipeline* PipelineCache::findOrCreatePipeline(PipelineLayout& pipelineLayout)
{
    auto iter = pipelines_.find(pipelineRequires_);

    // if the pipeline has already has an instance return this
    if (iter != pipelines_.end())
    {
        return iter->second.get();
    }

    // else create a new pipeline - If we are in a threaded environemt then we
    // can't add to the list until we are out of the thread
    std::unique_ptr<Pipeline> pline =
        std::make_unique<Pipeline>(context_, Pipeline::Type::Graphics);
    Pipeline* output = pline.get();
    pline->create(pipelineRequires_, const_cast<PipelineLayout&>(pipelineLayout));
    pipelines_.emplace(pipelineRequires_, std::move(pline));

    return output;
}

void PipelineCache::bindShaderModules(ShaderProgramBundle& prog)
{
    memcpy(
        pipelineRequires_.shaders,
        prog.getShaderStagesCreateInfo().data(),
        sizeof(vk::PipelineShaderStageCreateInfo) * util::ecast(backend::ShaderStage::Count));
}

void PipelineCache::bindRenderPass(const vk::RenderPass& rpass)
{
    ASSERT_LOG(rpass);
    pipelineRequires_.renderPass = rpass;
}

void PipelineCache::bindCullMode(vk::CullModeFlagBits cullMode)
{
    pipelineRequires_.rasterState.cullMode = cullMode;
}

void PipelineCache::bindPolygonMode(vk::PolygonMode polyMode)
{
    pipelineRequires_.rasterState.polygonMode = polyMode;
}

void PipelineCache::bindFrontFace(vk::FrontFace frontFace)
{
    pipelineRequires_.rasterState.frontFace = frontFace;
}

void PipelineCache::bindTopology(vk::PrimitiveTopology topo)
{
    pipelineRequires_.rasterState.topology = topo;
}

void PipelineCache::bindPrimRestart(bool state)
{
    pipelineRequires_.rasterState.primRestart = state;
}

void PipelineCache::bindDepthTestEnable(bool state)
{
    pipelineRequires_.rasterState.depthTestEnable = state;
}

void PipelineCache::bindDepthWriteEnable(bool state)
{
    pipelineRequires_.rasterState.depthWriteEnable = state;
}

void PipelineCache::bindDepthStencilBlock(const DepthStencilBlock& dsBlock)
{
    pipelineRequires_.dsBlock = dsBlock;
}

void PipelineCache::bindColourAttachCount(uint32_t count) noexcept
{
    pipelineRequires_.rasterState.colourAttachCount = count;
}

void PipelineCache::bindBlendFactorBlock(const BlendFactorBlock& block) noexcept
{
    pipelineRequires_.blendState = block;
}

void PipelineCache::bindVertexInput(
    vk::VertexInputAttributeDescription* vertAttrDesc,
    vk::VertexInputBindingDescription* vertBindDesc)
{
    ASSERT_LOG(vertAttrDesc);
    ASSERT_LOG(vertBindDesc);
    memcpy(
        pipelineRequires_.vertAttrDesc,
        vertAttrDesc,
        sizeof(vk::VertexInputAttributeDescription) * PipelineCache::MaxVertexAttributeCount);
    memcpy(
        pipelineRequires_.vertBindDesc,
        vertBindDesc,
        sizeof(vk::VertexInputBindingDescription) * PipelineCache::MaxVertexAttributeCount);
}

void PipelineCache::bindUbo(uint8_t bindValue, vk::Buffer buffer, uint32_t size)
{
    ASSERT_FATAL(
        bindValue < MaxUboBindCount,
        "Ubo binding value (%d) exceeds max allowed binding count (%d)",
        bindValue,
        MaxUboBindCount);
    descRequires_.ubos[bindValue] = buffer;
    descRequires_.bufferSizes[bindValue] = size;
}

void PipelineCache::bindUboDynamic(uint8_t bindValue, vk::Buffer buffer, uint32_t size)
{
    ASSERT_FATAL(
        bindValue < MaxUboDynamicBindCount,
        "Dynamic ubo binding value (%d) exceeds max allowed binding count (%d)",
        bindValue,
        MaxUboDynamicBindCount);
    ASSERT_LOG(size > 0);
    descRequires_.dynamicUbos[bindValue] = buffer;
    descRequires_.dynamicBufferSizes[bindValue] = size;
}

void PipelineCache::bindSsbo(uint8_t bindValue, vk::Buffer buffer, uint32_t size)
{
    ASSERT_LOG(size > 0);
    ASSERT_FATAL(
        bindValue < MaxSsboBindCount,
        "SSBO binding value (%d) exceeds max allowed binding count (%d)",
        bindValue,
        MaxSsboBindCount);
    descRequires_.ssbos[bindValue] = buffer;
    descRequires_.ssboBufferSizes[bindValue] = size;
}

void PipelineCache::bindScissor(vk::CommandBuffer cmdBuffer, const vk::Rect2D& newScissor)
{
    cmdBuffer.setScissor(0, 1, &newScissor);
}

void PipelineCache::bindViewport(vk::CommandBuffer cmdBuffer, const vk::Viewport& newViewPort)
{
    cmdBuffer.setViewport(0, 1, &newViewPort);
}

void PipelineCache::bindSampler(DescriptorImage descImages[MaxSamplerBindCount])
{
    memcpy(&descRequires_.samplers, descImages, sizeof(DescriptorImage) * MaxSamplerBindCount);
}

void PipelineCache::bindDescriptors(
    vk::CommandBuffer& cmdBuffer,
    PipelineLayout& pipelineLayout,
    const std::vector<uint32_t>& dynamicOffsets)
{
    // check if the required descriptor set is already bound. If so, nothing to
    // do here.
    if (boundDescriptor_ == descRequires_)
    {
        descriptorSets_[boundDescriptor_].frameLastUsed = driver_.getCurrentFrame();
        resetKeys();
        return;
    }

    // Check if a descriptor set in the cache fills the requirements and use
    // that if so.
    DescriptorSetInfo descSetInfo;
    auto iter = descriptorSets_.find(descRequires_);
    if (iter != descriptorSets_.end())
    {
        descSetInfo = iter->second;
        descSetInfo.frameLastUsed = driver_.getCurrentFrame();
    }
    else
    {
        // create a new descriptor set if no cached set matches the requirements
        createDescriptorSets(pipelineLayout, descSetInfo);
        descSetInfo.frameLastUsed = driver_.getCurrentFrame();
        descriptorSets_.emplace(descRequires_, descSetInfo);
    }

    vk::PipelineBindPoint bindPoint = Pipeline::createBindPoint(Pipeline::Type::Graphics);
    cmdBuffer.bindDescriptorSets(
        bindPoint,
        pipelineLayout.get(),
        0,
        static_cast<uint32_t>(MaxDescriptorTypeCount),
        descSetInfo.descrSets,
        static_cast<uint32_t>(dynamicOffsets.size()),
        dynamicOffsets.data());

    boundDescriptor_ = descRequires_;
    resetKeys();
}

void PipelineCache::createDescriptorSets(
    PipelineLayout& pipelineLayout, DescriptorSetInfo& descSetInfo)
{
    auto layouts = pipelineLayout.getDescSetLayout();
    for (size_t idx = 0; idx < PipelineCache::MaxDescriptorTypeCount; ++idx)
    {
        descSetInfo.layout[idx] = layouts[idx];
    }

    const vk::Device& device = context_.device();
    if (descriptorSets_.size() * MaxDescriptorTypeCount > currentDescPoolSize_)
    {
        increasePoolCapacity();
    }

    // create descriptor set for each layout
    allocDescriptorSets(descSetInfo.layout, descSetInfo.descrSets);

    // update the descriptor sets for each type (buffer, sampler, attachment)
    std::vector<vk::WriteDescriptorSet> writeSets;
    writeSets.reserve(20);

    std::array<vk::DescriptorBufferInfo, MaxUboBindCount> bufferInfos;
    std::array<vk::DescriptorBufferInfo, MaxUboBindCount> dynamicBufferInfos;
    std::array<vk::DescriptorBufferInfo, MaxSsboBindCount> ssboInfos;
    std::array<vk::DescriptorImageInfo, MaxSamplerBindCount> samplerInfos;

    // unoform buffers
    for (uint8_t i = 0; i < MaxUboBindCount; ++i)
    {
        if (descRequires_.ubos[i])
        {
            ASSERT_FATAL(descSetInfo.descrSets[UboSetValue], "UBO Descriptor set is NULL.");

            vk::DescriptorBufferInfo& bufferInfo = bufferInfos[i];
            bufferInfo.buffer = descRequires_.ubos[i];
            bufferInfo.offset = 0;
            bufferInfo.range = descRequires_.bufferSizes[i];

            writeSets.push_back(
                {descSetInfo.descrSets[UboSetValue],
                 i,
                 0,
                 1,
                 vk::DescriptorType::eUniformBuffer,
                 nullptr,
                 &bufferInfo,
                 nullptr});
        }
    }
    // dynamic uniform buffers
    for (uint8_t i = 0; i < MaxUboDynamicBindCount; ++i)
    {
        if (descRequires_.dynamicUbos[i])
        {
            ASSERT_FATAL(
                descSetInfo.descrSets[UboDynamicSetValue], "Dynamic UBO Descriptor set is NULL.");

            vk::DescriptorBufferInfo& bufferInfo = dynamicBufferInfos[i];
            bufferInfo.buffer = descRequires_.dynamicUbos[i];
            bufferInfo.offset = 0;
            bufferInfo.range = descRequires_.dynamicBufferSizes[i];

            writeSets.push_back(
                {descSetInfo.descrSets[UboDynamicSetValue],
                 i,
                 0,
                 1,
                 vk::DescriptorType::eUniformBufferDynamic,
                 nullptr,
                 &bufferInfo,
                 nullptr});
        }
    }
    // storage buffers
    for (uint8_t i = 0; i < MaxSsboBindCount; ++i)
    {
        if (descRequires_.ssbos[i])
        {
            ASSERT_FATAL(descSetInfo.descrSets[SsboSetValue], "SSBO Descriptor set is NULL.");

            vk::DescriptorBufferInfo& bufferInfo = ssboInfos[i];
            bufferInfo.buffer = descRequires_.ssbos[i];
            bufferInfo.offset = 0;
            bufferInfo.range = descRequires_.ssboBufferSizes[i];

            writeSets.push_back(
                {descSetInfo.descrSets[SsboSetValue],
                 i,
                 0,
                 1,
                 vk::DescriptorType::eStorageBuffer,
                 nullptr,
                 &bufferInfo,
                 nullptr});
        }
    }
    // samplers
    for (uint8_t i = 0; i < MaxSamplerBindCount; ++i)
    {
        if (descRequires_.samplers[i].imageSampler)
        {
            ASSERT_FATAL(
                descRequires_.samplers[i].imageSampler,
                "Image sampler not set for descriptor binding %d",
                i);
            ASSERT_FATAL(
                descRequires_.samplers[i].imageView,
                "Image view not set for descriptor binding %d",
                i);
            ASSERT_FATAL(descSetInfo.descrSets[SamplerSetValue], "Descriptor set is NULL.");

            vk::DescriptorImageInfo& imageInfo = samplerInfos[i];
            imageInfo.imageLayout = descRequires_.samplers[i].imageLayout;
            imageInfo.imageView = descRequires_.samplers[i].imageView;
            imageInfo.sampler = descRequires_.samplers[i].imageSampler;

            writeSets.push_back(
                {descSetInfo.descrSets[SamplerSetValue],
                 i,
                 0,
                 1,
                 vk::DescriptorType::eCombinedImageSampler,
                 &imageInfo,
                 nullptr,
                 nullptr});
        }
    }
    // TODO: add input attachments

    device.updateDescriptorSets(
        static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

void PipelineCache::allocDescriptorSets(
    vk::DescriptorSetLayout* descLayouts, vk::DescriptorSet* descSets)
{
    ASSERT_FATAL(
        descriptorPool_,
        "The descriptor pool must be initialised before allocating descriptor "
        "sets.");
    vk::DescriptorSetAllocateInfo allocInfo(descriptorPool_, MaxDescriptorTypeCount, descLayouts);
    VK_CHECK_RESULT(context_.device().allocateDescriptorSets(&allocInfo, descSets));
}

void PipelineCache::createDescriptorPools()
{
    std::array<vk::DescriptorPoolSize, MaxDescriptorTypeCount> pools;

    pools[0] = vk::DescriptorPoolSize {
        vk::DescriptorType::eUniformBuffer, currentDescPoolSize_ * MaxUboBindCount};
    pools[1] = vk::DescriptorPoolSize {
        vk::DescriptorType::eUniformBufferDynamic, currentDescPoolSize_ * MaxUboDynamicBindCount};
    pools[2] = vk::DescriptorPoolSize {
        vk::DescriptorType::eCombinedImageSampler, currentDescPoolSize_ * MaxSamplerBindCount};
    pools[3] = vk::DescriptorPoolSize {
        vk::DescriptorType::eStorageBuffer, currentDescPoolSize_ * MaxSsboBindCount};

    vk::DescriptorPoolCreateInfo createInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        currentDescPoolSize_ * MaxDescriptorTypeCount,
        static_cast<uint32_t>(pools.size()),
        pools.data());
    VK_CHECK_RESULT(context_.device().createDescriptorPool(&createInfo, nullptr, &descriptorPool_));
}

void PipelineCache::increasePoolCapacity()
{
    descPoolsForDeletion_.push_back(descriptorPool_);

    // schedule all descriptor sets associated with this pool
    // for deletion as well
    for (const auto& [key, info] : descriptorSets_)
    {
        descSetsForDeletion_.push_back(info);
    }
    descriptorSets_.clear();

    currentDescPoolSize_ *= 2;
    createDescriptorPools();
}

void PipelineCache::cleanCache(uint64_t currentFrame)
{
    // Destroy any pipelines that have reached there lifetime after their last use.
    for (auto iter = pipelines_.begin(); iter != pipelines_.end();)
    {
        vk::Pipeline pl = iter->second->get();
        uint64_t collectionFrame = iter->second->lastUsedFrameStamp_ + Pipeline::LifetimeFrameCount;
        if (pl && collectionFrame < currentFrame)
        {
            context_.device().destroy(pl);
            iter = pipelines_.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    // Destroy any descriptor sets that have reached there lifetime after their last
    // use.
    // TODO: we realy should be deleting the pipeline layout associated with
    // these sets too.
    for (auto iter = descriptorSets_.begin(); iter != descriptorSets_.end();)
    {
        DescriptorSetInfo info = iter->second;
        uint64_t collectionFrame = info.frameLastUsed + Pipeline::LifetimeFrameCount;
        if (collectionFrame < currentFrame)
        {
            for (int i = 0; i < MaxDescriptorTypeCount; ++i)
            {
                if (info.layout[i])
                {
                    // context_.device().destroy(info.layout[i]);
                }
                // context_.device().freeDescriptorSets(
                //     descriptorPool_, MaxDescriptorTypeCount, info.descrSets);
            }
            iter = descriptorSets_.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    // remove stale sets and pools
    if (!descSetsForDeletion_.empty())
    {
        uint64_t collectionFrame =
            descSetsForDeletion_[0].frameLastUsed + Pipeline::LifetimeFrameCount;
        if (collectionFrame < currentFrame)
        {
            for (auto const& pool : descPoolsForDeletion_)
            {
                context_.device().destroy(pool);
            }
        }
    }
}

void PipelineCache::clear() noexcept
{
    // Destroy all desciptor sets and layouts associated with this cache.
    for (const auto& [key, info] : descriptorSets_)
    {
        for (int idx = 0; idx < MaxDescriptorTypeCount; ++idx)
        {
            if (info.layout[idx])
            {
                context_.device().destroy(info.layout[idx]);
            }
        }
    }
    descriptorSets_.clear();

    // Destroy the descriptor pool.
    context_.device().destroy(descriptorPool_);

    // Destroy all pipelines associated with this cache.
    for (const auto& [key, pl] : pipelines_)
    {
        if (pl->get())
        {
            context_.device().destroy(pl->get());
        }
    }
    pipelines_.clear();
}

} // namespace vkapi
