/* Copyright (c) 2018-2020 Garry Whitehead
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
#include "resource_cache.h"
#include "texture.h"
#include "utility/bitset_enum.h"
#include "utility/colour.h"
#include "utility/compiler.h"
#include "utility/handle.h"

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace vkapi
{

// forward declerations
class ImageView;
class VkContext;

struct RenderTarget
{
    static constexpr int MaxColourAttachCount = 6;
    // Allow for depth and stencil info
    static constexpr int MaxAttachmentCount = MaxColourAttachCount + 2;

    static constexpr int DepthIndex = MaxColourAttachCount + 1;
    static constexpr int StencilIndex = MaxColourAttachCount + 2;

    struct AttachmentInfo
    {
        uint8_t layer = 0;
        uint8_t level = 0;
        TextureHandle handle;
    };

    AttachmentInfo depth;
    AttachmentInfo stencil;
    std::array<AttachmentInfo, MaxColourAttachCount> colours;
    util::Colour4 clearCol {0.0f};
    uint8_t samples = 1;
    bool multiView = false;
};

using RenderTargetHandle = util::Handle<RenderTarget>;
using AttachmentHandle = util::Handle<vk::AttachmentDescription>;

class RenderPass
{
public:
    static constexpr int LifetimeFrameCount = 10;

    enum class DependencyType
    {
        ColourPass,
        DepthPass,
        StencilPass,
        DepthStencilPass,
        SurfaceKHR
    };


    // Describe the elements of an colour/depth/stencil attachment.
    struct Attachment
    {
        vk::Format format;
        uint32_t sampleCount = 1;
        vk::ImageLayout finalLayout;
        backend::LoadClearFlags loadOp;
        backend::StoreClearFlags storeOp;
        backend::LoadClearFlags stencilLoadOp;
        backend::StoreClearFlags stencilStoreOp;
        uint32_t width;
        uint32_t height;
    };

    RenderPass(VkContext& context);
    ~RenderPass();

    // static functions
    static vk::ImageLayout getAttachmentLayout(vk::Format format);

    // Add an attahment for this pass. This can be a colour or depth attachment
    AttachmentHandle addAttachment(const Attachment& attachInfo);

    void addSubpassDependency(DependencyType dependType);

    // Create the renderpass based on the above definitions and create the
    // framebuffer
    void prepare(bool multiView);

    // ====================== the getter and setters
    // =================================
    const vk::RenderPass& get() const;

    void setDepthClear(float col);

    // Functions that return the state of various aspects of this pass
    std::vector<vk::AttachmentDescription>& getAttachments();

    std::vector<vk::PipelineColorBlendAttachmentState> getColourAttachs();

    uint32_t colAttachCount() const noexcept
    {
        return static_cast<uint32_t>(colourAttachRefs_.size());
    }

    // The frame in which this renderpass was created. Used to
    // calculate the point at which this renderpass will be destroyed
    // based on its lifetime.
    uint64_t lastUsedFrameStamp_;

private:
    VkContext& context_;

    vk::RenderPass renderpass_;

    // The colour/input attachments
    std::vector<vk::AttachmentDescription> attachmentDescrs_;
    std::vector<vk::AttachmentReference> colourAttachRefs_;
    vk::AttachmentReference depthAttachDescr_;
    bool hasDepth_;

    // The dependencies between renderpasses and external sources
    std::array<vk::SubpassDependency, 2> dependencies_;

    // The clear colour for this pass - for each attachment
    float depthClear_;
};

/**
 * @ brief Used for building a concrete vulkan renderpass. The data is obtained
 * from the RenderGraph side.
 */
struct RenderPassData
{
    std::array<backend::LoadClearFlags, RenderTarget::MaxAttachmentCount> loadClearFlags = {
        backend::LoadClearFlags::DontCare};
    std::array<backend::StoreClearFlags, RenderTarget::MaxAttachmentCount> storeClearFlags = {
        backend::StoreClearFlags::DontCare};
    std::array<vk::ImageLayout, RenderTarget::MaxAttachmentCount> finalLayouts = {
        vk::ImageLayout::eShaderReadOnlyOptimal};
    util::Colour4 clearCol;
    uint32_t width = 0;
    uint32_t height = 0;
};

class FrameBuffer
{
public:
    static constexpr int LifetimeFrameCount = 10;

    FrameBuffer(VkContext& context);
    ~FrameBuffer();

    void create(
        vk::RenderPass renderpass,
        vk::ImageView* imageViews,
        uint32_t count,
        uint32_t width,
        uint32_t height,
        uint8_t layers);

    const vk::Framebuffer& get() const;

    uint32_t getWidth() const;
    uint32_t getHeight() const;

    // The frame in which this framebuffer was created. Used to
    // work out the point at which it will be destroyed based on its
    // lifetime.
    uint64_t lastUsedFrameStamp_;

private:
    VkContext& context_;

    vk::Framebuffer fbo_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

} // namespace vkapi
