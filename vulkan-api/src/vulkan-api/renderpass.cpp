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

#include "renderpass.h"

#include "backend/convert_to_vk.h"
#include "context.h"
#include "image.h"
#include "utility.h"
#include "utility/assertion.h"

#include <spdlog/spdlog.h>

namespace vkapi
{

// ============== framebuffer ======================

FrameBuffer::FrameBuffer(VkContext& context) : context_(context) {}

FrameBuffer::~FrameBuffer() {}

void FrameBuffer::create(
    vk::RenderPass renderpass,
    vk::ImageView* imageViews,
    uint32_t count,
    uint32_t width,
    uint32_t height,
    uint8_t layers)
{
    ASSERT_LOG(width > 0);
    ASSERT_LOG(height > 0);
    ASSERT_LOG(imageViews);

    width_ = width;
    height_ = height;

    // and create the framebuffer.....
    vk::FramebufferCreateInfo fboInfo {{}, renderpass, count, imageViews, width, height, layers};

    VK_CHECK_RESULT(context_.device().createFramebuffer(&fboInfo, nullptr, &fbo_));
}

const vk::Framebuffer& FrameBuffer::get() const { return fbo_; }

uint32_t FrameBuffer::getWidth() const { return width_; }

uint32_t FrameBuffer::getHeight() const { return height_; }

// ================== Renderpass ================================

RenderPass::RenderPass(VkContext& context) : context_(context), hasDepth_(false), depthClear_(1.0f)
{
}

RenderPass::~RenderPass() {}

vk::ImageLayout RenderPass::getAttachmentLayout(vk::Format format)
{
    return (isStencil(format) || isDepth(format)) ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                                                  : vk::ImageLayout::eColorAttachmentOptimal;
}

AttachmentHandle RenderPass::addAttachment(const RenderPass::Attachment& attachInfo)
{
    vk::AttachmentDescription attachDescr;
    attachDescr.format = attachInfo.format;
    attachDescr.initialLayout = vk::ImageLayout::eUndefined;
    attachDescr.finalLayout = attachInfo.finalLayout;

    // samples
    attachDescr.samples = backend::samplesToVk(attachInfo.sampleCount);

    // clear flags
    attachDescr.loadOp = backend::loadFlagsToVk(attachInfo.loadOp); // pre image state
    attachDescr.storeOp = backend::storeFlagsToVk(attachInfo.storeOp); // post image state
    attachDescr.stencilLoadOp =
        backend::loadFlagsToVk(attachInfo.stencilLoadOp); // pre stencil state
    attachDescr.stencilStoreOp =
        backend::storeFlagsToVk(attachInfo.stencilStoreOp); // post stencil state

    AttachmentHandle handle(static_cast<uint32_t>(attachmentDescrs_.size()));
    attachmentDescrs_.emplace_back(attachDescr);
    return handle;
}

void RenderPass::addSubpassDependency(DependencyType dependType)
{
    dependencies_[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    dependencies_[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies_[0].dstSubpass = 0;
    dependencies_[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies_[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;

    if (dependType == DependencyType::ColourPass)
    {
        dependencies_[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies_[0].dstAccessMask =
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    }
    else if (dependType == DependencyType::DepthStencilPass)
    {
        dependencies_[0].dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependencies_[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    }
    else if (dependType == DependencyType::StencilPass)
    {
        dependencies_[0].dstStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
        dependencies_[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    }
    else if (dependType == DependencyType::SurfaceKHR)
    {
        dependencies_[0].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies_[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies_[0].srcAccessMask = vk::AccessFlagBits(0);
        dependencies_[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    }
    else
    {
        SPDLOG_WARN("Unsupported dependency read type. This may lead to "
                    "invalid dst stage masks.");
    }

    // src and dst stage masks cannot be zero
    ASSERT_LOG(dependencies_[0].srcStageMask != vk::PipelineStageFlags(0));
    ASSERT_LOG(dependencies_[0].dstStageMask != vk::PipelineStageFlags(0));

    // and the next dependenciesency stage
    dependencies_[1].srcSubpass = dependencies_[0].dstSubpass;
    dependencies_[1].dstSubpass = dependencies_[0].srcSubpass;
    dependencies_[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    if (dependType == DependencyType::SurfaceKHR)
    {
        dependencies_[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies_[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    }
    else
    {
        dependencies_[1].srcStageMask = dependencies_[0].dstStageMask;
        dependencies_[1].dstStageMask = dependencies_[0].srcStageMask;
        dependencies_[1].srcAccessMask = dependencies_[0].dstAccessMask;
        dependencies_[1].dstAccessMask = dependencies_[0].srcAccessMask;
    }
}

void RenderPass::prepare(bool multiView)
{
    // create the attachment references
    bool surfacePass = false;
    for (size_t count = 0; count < attachmentDescrs_.size(); ++count)
    {
        auto& descr = attachmentDescrs_[count];

        if (descr.finalLayout == vk::ImageLayout::ePresentSrcKHR)
        {
            surfacePass = true;
        }

        vk::AttachmentReference ref {
            static_cast<uint32_t>(count), getAttachmentLayout(descr.format)};

        if (isDepth(descr.format) || isStencil(descr.format))
        {
            hasDepth_ = true;
            depthAttachDescr_ = ref;
        }
        else
        {
            colourAttachRefs_.emplace_back(ref);
        }
    }

    // add the dependdencies
    if (colourAttachRefs_.empty() && hasDepth_)
    {
        addSubpassDependency(DependencyType::DepthStencilPass);
    }
    else if (surfacePass)
    {
        addSubpassDependency(DependencyType::SurfaceKHR);
    }
    else
    {
        addSubpassDependency(DependencyType::ColourPass);
    }

    vk::SubpassDescription subpassDescr {
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        static_cast<uint32_t>(colourAttachRefs_.size()),
        colourAttachRefs_.data(),
        nullptr,
        hasDepth_ ? &depthAttachDescr_ : nullptr,
        0,
        nullptr};

    vk::RenderPassCreateInfo createInfo(
        {},
        static_cast<uint32_t>(attachmentDescrs_.size()),
        attachmentDescrs_.data(),
        1,
        &subpassDescr,
        static_cast<uint32_t>(dependencies_.size()),
        dependencies_.data());

    vk::RenderPassMultiviewCreateInfo mvCreateInfo;
    std::vector<uint32_t> viewMasks(attachmentDescrs_.size());
    std::vector<uint32_t> correlationMasks(attachmentDescrs_.size());
    if (multiView)
    {
        // Note: at present only multi view rendering to cube maps
        // is supported.
        for (size_t i = 0; i < attachmentDescrs_.size(); ++i)
        {
            viewMasks[i] = 0b00111111;
            correlationMasks[i] = viewMasks[i];
        }
        mvCreateInfo = {
            static_cast<uint32_t>(viewMasks.size()),
            viewMasks.data(),
            0,
            nullptr,
            static_cast<uint32_t>(correlationMasks.size()),
            correlationMasks.data()};
        createInfo.pNext = &mvCreateInfo;
    }

    VK_CHECK_RESULT(context_.device().createRenderPass(&createInfo, nullptr, &renderpass_));
}

const vk::RenderPass& RenderPass::get() const { return renderpass_; }

void RenderPass::setDepthClear(float col) { depthClear_ = col; }

std::vector<vk::AttachmentDescription>& RenderPass::getAttachments() { return attachmentDescrs_; }

std::vector<vk::PipelineColorBlendAttachmentState> RenderPass::getColourAttachs()
{
    size_t attachCount = attachmentDescrs_.size();
    ASSERT_LOG(attachCount > 0);
    std::vector<vk::PipelineColorBlendAttachmentState> colAttachs;
    colAttachs.reserve(attachCount);

    // for each clear output colour attachment in the renderpass, we need a
    // blend attachment
    for (uint32_t i = 0; i < attachmentDescrs_.size(); ++i)
    {
        // only colour attachments....
        if (isDepth(attachmentDescrs_[i].format) || isStencil(attachmentDescrs_[i].format))
        {
            continue;
        }
        vk::PipelineColorBlendAttachmentState colour;
        colour.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colour.blendEnable = VK_FALSE; //< TODO: need to add blending
        colAttachs.emplace_back(colour);
    }
    return colAttachs;
}


} // namespace vkapi
