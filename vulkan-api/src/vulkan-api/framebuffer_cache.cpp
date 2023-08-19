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

#include "framebuffer_cache.h"

#include "driver.h"

namespace vkapi
{

FramebufferCache::FramebufferCache(VkContext& context, VkDriver& driver)
    : context_(context), driver_(driver)
{
}

bool FramebufferCache::RPassKey::operator==(const RPassKey& rhs) const noexcept
{
    if (memcmp(
            colourFormats,
            rhs.colourFormats,
            sizeof(vk::Format) * RenderTarget::MaxColourAttachCount) != 0)
    {
        return false;
    }
    if (memcmp(
            initialLayout,
            rhs.initialLayout,
            sizeof(vk::ImageLayout) * RenderTarget::MaxColourAttachCount) != 0)
    {
        return false;
    }
    if (memcmp(
            finalLayout,
            rhs.finalLayout,
            sizeof(vk::ImageLayout) * RenderTarget::MaxColourAttachCount) != 0)
    {
        return false;
    }
    if (memcmp(
            loadOp,
            rhs.loadOp,
            sizeof(backend::LoadClearFlags) * RenderTarget::MaxColourAttachCount) != 0)
    {
        return false;
    }
    if (memcmp(
            storeOp,
            rhs.storeOp,
            sizeof(backend::StoreClearFlags) * RenderTarget::MaxColourAttachCount) != 0)
    {
        return false;
    }
    if (depth != rhs.depth || dsStoreOp[0] != rhs.dsStoreOp[0] || dsLoadOp[0] != rhs.dsLoadOp[0] ||
        dsStoreOp[1] != rhs.dsStoreOp[1] || dsLoadOp[1] != rhs.dsLoadOp[1] ||
        multiView != rhs.multiView)
    {
        return false;
    }
    return true;
}

bool FramebufferCache::FboKey::operator==(const FboKey& rhs) const noexcept
{
    if (memcmp(views, rhs.views, sizeof(VkImageView) * 6) != 0)
    {
        return false;
    }
    if (width != rhs.width || height != rhs.height || renderpass == rhs.renderpass ||
        samples != rhs.samples || layer != rhs.layer)
    {
        return false;
    }
    return true;
}

RenderPass* FramebufferCache::findOrCreateRenderPass(const RPassKey& key)
{
    auto iter = renderPasses_.find(key);
    if (iter != renderPasses_.end())
    {
        iter->second->lastUsedFrameStamp_ = driver_.getCurrentFrame();
        return iter->second;
    }
    // create a new renderpass
    RenderPass* rpass = new RenderPass(context_);
    rpass->lastUsedFrameStamp_ = driver_.getCurrentFrame();

    // add the colour attachments
    for (int idx = 0; idx < RenderTarget::MaxColourAttachCount; ++idx)
    {
        if (key.colourFormats[idx] != vk::Format::eUndefined)
        {
            ASSERT_LOG(key.finalLayout[idx] != vk::ImageLayout::eUndefined);
            RenderPass::Attachment attach;
            attach.format = key.colourFormats[idx];
            attach.initialLayout = key.initialLayout[idx];
            attach.finalLayout = key.finalLayout[idx];
            attach.loadOp = key.loadOp[idx];
            attach.storeOp = key.storeOp[idx];
            attach.stencilLoadOp = key.dsLoadOp[1];
            attach.stencilStoreOp = key.dsStoreOp[1];
            rpass->addAttachment(attach);
        }
    }
    if (key.depth != vk::Format(0))
    {
        RenderPass::Attachment attach;
        attach.format = key.depth;
        attach.initialLayout = vk::ImageLayout::eUndefined;
        attach.finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        attach.loadOp = key.dsLoadOp[0];
        attach.storeOp = key.dsStoreOp[0];
        attach.stencilLoadOp = key.dsLoadOp[1];
        attach.stencilStoreOp = key.dsStoreOp[1];
        rpass->addAttachment(attach);
    }
    rpass->prepare(key.multiView);

    renderPasses_.emplace(key, rpass);
    return rpass;
}

FrameBuffer* FramebufferCache::findOrCreateFrameBuffer(FboKey& key, uint32_t count)
{
    auto iter = frameBuffers_.find(key);
    if (iter != frameBuffers_.end())
    {
        return iter->second;
    }

    FrameBuffer* fbo = new FrameBuffer(context_);
    fbo->create(key.renderpass, key.views, count, key.width, key.height, key.layer);
    frameBuffers_.emplace(key, fbo);
    return fbo;
}

void FramebufferCache::cleanCache(uint64_t currentFrame) noexcept
{
    for (auto iter = frameBuffers_.begin(); iter != frameBuffers_.end();)
    {
        vk::Framebuffer fb = iter->second->get();
        uint64_t framesUntilCollection =
            iter->second->lastUsedFrameStamp_ + FrameBuffer::LifetimeFrameCount;
        if (fb && framesUntilCollection < currentFrame)
        {
            context_.device().destroy(fb);
            delete iter->second;
            iter = frameBuffers_.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    for (auto iter = renderPasses_.begin(); iter != renderPasses_.end();)
    {
        vk::RenderPass rpass = iter->second->get();
        uint64_t framesUntilCollection =
            iter->second->lastUsedFrameStamp_ + RenderPass::LifetimeFrameCount;
        if (rpass && framesUntilCollection < currentFrame)
        {
            context_.device().destroy(rpass);
            delete iter->second;
            iter = renderPasses_.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void FramebufferCache::clear() noexcept
{
    for (const auto& [key, info] : frameBuffers_)
    {
        vk::Framebuffer fb = info->get();
        if (fb)
        {
            context_.device().destroy(fb);
            delete info;
        }
    }
    frameBuffers_.clear();

    for (const auto& [key, info] : renderPasses_)
    {
        vk::RenderPass rpass = info->get();
        if (rpass)
        {
            context_.device().destroy(rpass);
            delete info;
        }
    }
    renderPasses_.clear();
}

} // namespace vkapi
