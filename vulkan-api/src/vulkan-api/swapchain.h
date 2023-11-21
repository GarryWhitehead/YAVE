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
#include "resource_cache.h"
#include "utility/compiler.h"
#include "utility/handle.h"

#include <vector>

namespace vkapi
{

// forward declerations
class VkDriver;
class Image;
class ImageView;
class VkContext;

struct SwapchainContext
{
    TextureHandle handle;
    vk::CommandBuffer cmdBuffer;
    vk::Fence fence;
};

class Swapchain
{
public:
    Swapchain();
    ~Swapchain();

    bool
    create(VkDriver& driver, const vk::SurfaceKHR& surface, uint32_t winWidth, uint32_t winHeight);

    void destroy(VkContext& context);

    [[nodiscard]] const vk::SwapchainKHR& get() const;
    [[nodiscard]] uint32_t extentsHeight() const;
    [[nodiscard]] uint32_t extentsWidth() const;

    TextureHandle& getTexture(uint32_t index);

private:
    /// creates the image views for the swapchain
    void prepareImageViews(VkDriver& driver, const vk::SurfaceFormatKHR& surfaceFormat);

private:
    // the dimensions of the current swapchain
    vk::Extent2D extent_;

    // a swapchain based on the present surface type
    vk::SwapchainKHR swapchain_;

    vk::SurfaceFormatKHR surfaceFormat_;
    std::vector<SwapchainContext> contexts_;
};

using SwapchainHandle = util::Handle<vkapi::Swapchain>;

} // namespace vkapi
