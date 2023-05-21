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
#include "context.h"
#include "renderpass.h"
#include "utility/compiler.h"
#include "utility/cstring.h"
#include "utility/handle.h"
#include "utility/murmurhash.h"

#include <unordered_map>

namespace vkapi
{
// forward declreations
class VkDriver;

using RenderpassHandle = util::Handle<RenderPass>;
using FramebufferHandle = util::Handle<FrameBuffer>;

class FramebufferCache
{
public:
    static constexpr int FramesUntilClear = 10;

    // ================ Frame buffer cache =========================

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wpadded"

    struct RPassKey
    {
        vk::ImageLayout initialLayout[RenderTarget::MaxColourAttachCount];
        vk::ImageLayout finalLayout[RenderTarget::MaxColourAttachCount];
        vk::Format colourFormats[RenderTarget::MaxColourAttachCount];
        backend::LoadClearFlags loadOp[RenderTarget::MaxColourAttachCount];
        backend::StoreClearFlags storeOp[RenderTarget::MaxColourAttachCount];
        backend::LoadClearFlags dsLoadOp[2];
        backend::StoreClearFlags dsStoreOp[2];
        vk::Format depth;
        uint32_t samples;
        bool multiView;
        uint8_t padding[3];

        bool operator==(const RPassKey& rhs) const noexcept;
    };

    struct FboKey
    {
        vk::RenderPass renderpass;
        vk::ImageView views[RenderTarget::MaxColourAttachCount];
        uint32_t width;
        uint32_t height;
        uint16_t samples;
        uint16_t layer;
        uint32_t padding;

        bool operator==(const FboKey& rhs) const noexcept;
    };

#pragma clang diagnostic pop

    static_assert(
        std::is_pod<RPassKey>::value, "RPassKey must be a POD for the hashing to work correctly");
    static_assert(
        std::is_trivially_copyable<FboKey>::value,
        "FboKey must be a POD for the hashing to work correctly");

    using RPassHasher = ::util::Murmur3Hasher<RPassKey>;
    using FboHasher = ::util::Murmur3Hasher<FboKey>;

    using RenderPassMap = std::unordered_map<RPassKey, RenderPass*, RPassHasher>;
    using FrameBufferMap = std::unordered_map<FboKey, FrameBuffer*, FboHasher>;

    FramebufferCache(VkContext& conext, VkDriver& driver);

    RenderPass* findOrCreateRenderPass(const RPassKey& key);

    FrameBuffer* findOrCreateFrameBuffer(FboKey& key, uint32_t count);

    // Remove old renderpasses and framebuffers based on frame count.
    void cleanCache(uint64_t currentFrame) noexcept;

    // Destroy all renderpasses and framebuffers currently cached.
    void clear() noexcept;

private:
    VkContext& context_;
    VkDriver& driver_;

    RenderPassMap renderPasses_;
    FrameBufferMap frameBuffers_;
};

} // namespace vkapi
