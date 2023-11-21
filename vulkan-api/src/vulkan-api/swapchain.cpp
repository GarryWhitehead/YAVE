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

#include "swapchain.h"

#include "context.h"
#include "driver.h"
#include "image.h"
#include "utility/assertion.h"
#include "utility/logger.h"

#include <algorithm>

namespace vkapi
{

Swapchain::Swapchain() = default;
Swapchain::~Swapchain() = default;

void Swapchain::destroy(VkContext& context) { context.device().destroy(swapchain_); }

bool Swapchain::create(
    VkDriver& driver, const vk::SurfaceKHR& surface, uint32_t winWidth, uint32_t winHeight)
{
    vk::Device device = driver.context().device();
    vk::PhysicalDevice gpu = driver.context().physical();

    // Get the basic surface properties of the physical device
    vk::SurfaceCapabilitiesKHR capabilities = gpu.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = gpu.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> presentModes = gpu.getSurfacePresentModesKHR(surface);

    // make sure that we have suitable swap chain extensions available before
    // continuing
    if (surfaceFormats.empty() || presentModes.empty())
    {
        LOGGER_ERROR("Critcal error! Unable to locate suitable swap chains on device.");
        return false;
    }

    // Next step is to determine the surface format. Ideally undefined format is
    // preffered, so we can set our own, otherwise we will go with one that suits
    // our colour needs - i.e. 8bitBGRA and SRGB.
    vk::SurfaceFormatKHR requiredSurfaceFormats;

    if (!surfaceFormats.empty() && (surfaceFormats[0].format == vk::Format::eUndefined))
    {
        requiredSurfaceFormats.format = vk::Format::eB8G8R8A8Unorm;
        requiredSurfaceFormats.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    }
    else
    {
        for (auto& format : surfaceFormats)
        {
            if (format.format == vk::Format::eB8G8R8A8Unorm &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                requiredSurfaceFormats = format;
                break;
            }
        }
    }
    surfaceFormat_ = requiredSurfaceFormats;

    // And then the presentation format - the preferred format is triple
    // buffering; immediate mode is only supported on some drivers.
    vk::PresentModeKHR requiredPresentMode;
    for (auto& mode : presentModes)
    {
        if (mode == vk::PresentModeKHR::eFifo || mode == vk::PresentModeKHR::eImmediate)
        {
            requiredPresentMode = mode;
            break;
        }
    }

    // Finally set the resolution of the swap chain buffers
    // First of check if we can manually set the dimension - some GPUs allow
    // this by setting the max as the size of uint32
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        extent_ = capabilities.currentExtent; // go with the automatic settings
    }
    else
    {
        extent_.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, winWidth));
        extent_.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, winHeight));
    }

    // Get the number of possible images we can send to the queue
    uint32_t imageCount =
        capabilities.minImageCount + 1; // adding one as we would like to implement triple buffering
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    auto compositeFlag = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
    {
        compositeFlag = vk::CompositeAlphaFlagBitsKHR::eInherit;
    }

    // if the graphics and presentation aren't the same then use concurrent
    // sharing mode
    vk::SharingMode sharingMode = vk::SharingMode::eExclusive;

    const uint32_t graphFamilyIdx = driver.context().queueIndices().graphics;
    const uint32_t presentFamilyIdx = driver.context().queueIndices().present;

    if (graphFamilyIdx != presentFamilyIdx)
    {
        sharingMode = vk::SharingMode::eConcurrent;
    }

    vk::SwapchainCreateInfoKHR createInfo(
        {},
        surface,
        imageCount,
        requiredSurfaceFormats.format,
        requiredSurfaceFormats.colorSpace,
        extent_,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        sharingMode,
        0,
        nullptr,
        capabilities.currentTransform,
        compositeFlag,
        requiredPresentMode,
        VK_TRUE,
        {});

    // And finally, create the swap chain
    VK_CHECK_RESULT(device.createSwapchainKHR(&createInfo, nullptr, &swapchain_));

    prepareImageViews(driver, surfaceFormat_);

    return true;
}

void Swapchain::prepareImageViews(VkDriver& driver, const vk::SurfaceFormatKHR& surfaceFormat)
{
    vk::Device device = driver.context().device();

    // Get the image locations created when creating the swap chain
    std::vector<vk::Image> images = device.getSwapchainImagesKHR(swapchain_);

    for (const auto& image : images)
    {
        SwapchainContext scContext;
        scContext.handle =
            driver.createTexture2d(surfaceFormat.format, extent_.width, extent_.height, image);
        contexts_.emplace_back(scContext);
    }
}

TextureHandle& Swapchain::getTexture(const uint32_t index)
{
    ASSERT_FATAL(index < contexts_.size(), "Index out of range (=%i) for image view.", index);
    return contexts_[index].handle;
}

const vk::SwapchainKHR& Swapchain::get() const { return swapchain_; }

uint32_t Swapchain::extentsHeight() const { return extent_.height; }

uint32_t Swapchain::extentsWidth() const { return extent_.width; }

} // namespace vkapi
