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

#include "pipeline.h"

#include "context.h"
#include "driver.h"
#include "program_manager.h"
#include "renderpass.h"
#include "shader.h"
#include "utility/assertion.h"

namespace vkapi
{

PipelineLayout::PipelineLayout() = default;
PipelineLayout::~PipelineLayout() = default;

void PipelineLayout::createDescriptorLayouts(VkContext& context)
{
    for (uint8_t i = 0; i < PipelineCache::MaxDescriptorTypeCount; ++i)
    {
        auto& setLayoutBinding = descriptorBindings_[i];
        const vk::DescriptorSetLayoutBinding* bindingData =
            setLayoutBinding.empty() ? nullptr : setLayoutBinding.data();
        const uint32_t bindingSize =
            setLayoutBinding.empty() ? 0 : static_cast<uint32_t>(setLayoutBinding.size());
        vk::DescriptorSetLayoutCreateInfo layoutInfo {{}, bindingSize, bindingData};
        VK_CHECK_RESULT(context.device().createDescriptorSetLayout(
            &layoutInfo, nullptr, &descriptorLayouts_[i]));
    }
}

void PipelineLayout::build(VkContext& context)
{
    if (layout_)
    {
        return;
    }

    createDescriptorLayouts(context);

    // create push constants - just the size for now. The data contents are set
    // at draw time
    std::vector<vk::PushConstantRange> pConstants;
    pConstants.reserve(3);

    for (const auto& [type, size] : pConstantSizes_)
    {
        ASSERT_LOG(size > 0);
        vk::PushConstantRange push(type, 0, static_cast<uint32_t>(size));
        pConstants.push_back(push);
    }

    vk::PipelineLayoutCreateInfo pipelineInfo(
        {},
        static_cast<uint32_t>(PipelineCache::MaxDescriptorTypeCount),
        descriptorLayouts_,
        static_cast<uint32_t>(pConstants.size()),
        pConstants.data());

    VK_CHECK_RESULT(context.device().createPipelineLayout(&pipelineInfo, nullptr, &layout_));
}

void PipelineLayout::addPushConstant(backend::ShaderStage type, size_t size)
{
    pConstantSizes_[Shader::getStageFlags(type)] = size;
}

void PipelineLayout::bindPushBlock(
    const vk::CommandBuffer& cmdBuffer, const PushBlockBindParams& pushBlock)
{
    cmdBuffer.pushConstants(layout_, pushBlock.stage, 0, pushBlock.size, pushBlock.data);
}

void PipelineLayout::addDescriptorLayout(
    uint8_t set, uint32_t binding, vk::DescriptorType descType, vk::ShaderStageFlags stageFlags)
{
    vk::DescriptorSetLayoutBinding descSetLayoutBinding {binding, descType, 1, stageFlags};
    ASSERT_FATAL(
        set < PipelineCache::MaxDescriptorTypeCount,
        "Set value (%d) is out of bounds - max descriptor set count of %d",
        set,
        PipelineCache::MaxDescriptorTypeCount);

    auto iter = descriptorBindings_.find(set);
    if (iter != descriptorBindings_.end())
    {
        auto& descSetLayouts = iter->second;
        auto result = std::find_if(
            descSetLayouts.begin(), descSetLayouts.end(), [&binding](const auto& descSetLayout) {
                return binding == descSetLayout.binding;
            });
        if (result != descSetLayouts.end())
        {
            ASSERT_FATAL(
                result->descriptorType == descType,
                "Set %d; binding %d - change in descriptor type stage since "
                "last addition.",
                std::to_string(set).c_str(),
                std::to_string(binding).c_str());
            // if the set and binding have already been added, this may be another shader stage
            result->stageFlags |= stageFlags;
            return;
        }
    }
    descriptorBindings_[set].emplace_back(descSetLayoutBinding);
}

// ================== pipeline =======================
GraphicsPipeline::GraphicsPipeline(VkContext& context) : lastUsedFrameStamp_(0), context_(context)
{
}

GraphicsPipeline::~GraphicsPipeline() = default;

void GraphicsPipeline::create(
    const PipelineCache::GraphicsPlineKey& key, PipelineLayout& pipelineLayout)
{
    // sort the vertex attribute descriptors so only ones that are used
    // are applied to the pipeline
    std::vector<vk::VertexInputAttributeDescription> inputDesc;
    for (const auto& desc : key.vertAttrDesc)
    {
        if (desc.format != vk::Format::eUndefined)
        {
            inputDesc.emplace_back(desc);
        }
    }

    bool hasInputState = !inputDesc.empty();
    vk::PipelineVertexInputStateCreateInfo vertexInputState;
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(inputDesc.size());
    vertexInputState.pVertexAttributeDescriptions = hasInputState ? inputDesc.data() : nullptr;
    vertexInputState.vertexBindingDescriptionCount = hasInputState ? 1 : 0;
    vertexInputState.pVertexBindingDescriptions = hasInputState ? key.vertBindDesc : nullptr;

    // ============== primitive topology =====================
    vk::PipelineInputAssemblyStateCreateInfo assemblyState;
    assemblyState.topology = key.rasterState.topology;
    assemblyState.primitiveRestartEnable = key.rasterState.primRestart;

    // ============== multi-sample state =====================
    vk::PipelineMultisampleStateCreateInfo sampleState;

    // ============== depth/stenicl state ====================
    vk::PipelineDepthStencilStateCreateInfo depthStencilState;
    depthStencilState.depthTestEnable = key.rasterState.depthTestEnable;
    depthStencilState.depthWriteEnable = key.rasterState.depthWriteEnable;
    depthStencilState.depthCompareOp = key.dsBlock.compareOp;

    // ============== stencil state =====================
    depthStencilState.stencilTestEnable = key.dsBlock.stencilTestEnable;
    if (key.dsBlock.stencilTestEnable)
    {
        depthStencilState.front.failOp = key.dsBlock.stencilFailOp;
        depthStencilState.front.depthFailOp = key.dsBlock.depthFailOp;
        depthStencilState.front.passOp = key.dsBlock.passOp;
        depthStencilState.front.compareMask = key.dsBlock.compareMask;
        depthStencilState.front.writeMask = key.dsBlock.writeMask;
        depthStencilState.front.reference = key.dsBlock.reference;
        depthStencilState.front.compareOp = key.dsBlock.compareOp;
        depthStencilState.back = depthStencilState.front;
    }

    // ============ raster state =======================
    vk::PipelineRasterizationStateCreateInfo rasterState;
    rasterState.cullMode = key.rasterState.cullMode;
    rasterState.frontFace = key.rasterState.frontFace;
    rasterState.polygonMode = key.rasterState.polygonMode;
    rasterState.lineWidth = 1.0f;

    // ============ dynamic states ====================
    vk::PipelineDynamicStateCreateInfo dynamicCreateState;
    dynamicCreateState.dynamicStateCount = static_cast<uint32_t>(dynamicStates_.size());
    dynamicCreateState.pDynamicStates = dynamicStates_.data();

    // =============== viewport state ====================
    // scissor and viewport are set at draw time
    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // =============== tesselation =======================
    vk::PipelineTessellationStateCreateInfo tessCreateInfo;
    tessCreateInfo.patchControlPoints = key.tesselationVertCount;

    // ============= colour attachment =================
    // all blend attachments are the same for each pass
    vk::PipelineColorBlendStateCreateInfo colourBlendState;
    std::vector<vk::PipelineColorBlendAttachmentState> attachState(
        RenderTarget::MaxColourAttachCount);
    for (uint8_t i = 0; i < key.rasterState.colourAttachCount; ++i)
    {
        attachState[i].srcColorBlendFactor = key.blendState.srcColorBlendFactor;
        attachState[i].dstColorBlendFactor = key.blendState.dstColorBlendFactor;
        attachState[i].colorBlendOp = key.blendState.colorBlendOp;
        attachState[i].srcAlphaBlendFactor = key.blendState.srcAlphaBlendFactor;
        attachState[i].dstAlphaBlendFactor = key.blendState.dstAlphaBlendFactor;
        attachState[i].alphaBlendOp = key.blendState.alphaBlendOp;
        attachState[i].colorWriteMask = key.rasterState.colorWriteMask;
        attachState[i].blendEnable = key.blendState.blendEnable;
        ASSERT_FATAL(key.blendState.blendEnable <= 1, "Wrong!");
    }
    colourBlendState.attachmentCount = key.rasterState.colourAttachCount;
    colourBlendState.pAttachments = attachState.data();

    // ================= create the pipeline =======================
    ASSERT_FATAL(pipelineLayout.get(), "The pipeline layout must be initialised.");

    // we only want to add valid shaders to the pipeline. Because the key states
    // all shaders whether they are required or not, we need to create a
    // container with only valid shaders which is checked by testing whether the
    // name field of the create info is not NULL
    std::vector<vk::PipelineShaderStageCreateInfo> shaders;
    for (const auto& createInfo : key.shaders)
    {
        if (createInfo.pName)
        {
            shaders.emplace_back(createInfo);
        }
    }
    ASSERT_FATAL(!shaders.empty(), "No shaders associated with this pipeline.");

    vk::GraphicsPipelineCreateInfo createInfo(
        {},
        static_cast<uint32_t>(shaders.size()),
        shaders.data(),
        &vertexInputState,
        &assemblyState,
        key.tesselationVertCount > 0 ? &tessCreateInfo : VK_NULL_HANDLE,
        &viewportState,
        &rasterState,
        &sampleState,
        &depthStencilState,
        &colourBlendState,
        &dynamicCreateState,
        pipelineLayout.get(),
        key.renderPass,
        0,
        nullptr,
        0);

    VK_CHECK_RESULT(
        context_.device().createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline_));
}

ComputePipeline::ComputePipeline(VkContext& context) : context_(context) {}
ComputePipeline::~ComputePipeline() = default;

void ComputePipeline::create(
    const PipelineCache::ComputePlineKey& key, PipelineLayout& pipelineLayout)
{
    ASSERT_FATAL(pipelineLayout.get(), "The pipeline layout must be initialised.");
    vk::ComputePipelineCreateInfo createInfo {{}, key.shader, pipelineLayout.get()};
    VK_CHECK_RESULT(
        context_.device().createComputePipelines({}, 1, &createInfo, nullptr, &pipeline_));
}

} // namespace vkapi
