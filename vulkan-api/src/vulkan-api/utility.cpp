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

#include "utility.h"

#include "utility/logger.h"

namespace vkapi
{

vk::Format findSupportedFormat(
    std::vector<vk::Format>& formats,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags formatFeature,
    vk::PhysicalDevice& gpu)
{
    vk::Format outputFormat;

    for (const auto& format : formats)
    {
        vk::FormatProperties properties = gpu.getFormatProperties(format);

        if ((tiling == vk::ImageTiling::eLinear &&
             formatFeature == (properties.linearTilingFeatures & formatFeature)) ||
            (tiling == vk::ImageTiling::eOptimal &&
             formatFeature == (properties.optimalTilingFeatures & formatFeature)))
        {
            outputFormat = format;
            break;
        }
        else
        {
            // terminal error so throw
            throw std::runtime_error("Error! Unable to find supported vulkan format");
        }
    }
    return outputFormat;
}

vk::Format getSupportedDepthFormat(vk::PhysicalDevice& gpu)
{
    // in order of preference - TODO: allow user to define whether stencil
    // format is required or not
    std::vector<vk::Format> formats = {
        vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD32Sfloat};

    return findSupportedFormat(
        formats,
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment,
        gpu);
}

vk::Format convertToVk(const std::string& type, uint32_t width)
{
    // TODO: add other base types and widths
    vk::Format format = vk::Format::eUndefined;

    // floats
    if (width == 32)
    {
        if (type == "float")
        {
            format = vk::Format::eR32Sfloat;
        }
        else if (type == "vec2")
        {
            format = vk::Format::eR32G32Sfloat;
        }
        else if (type == "vec3")
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (type == "vec4")
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        // signed integers
        else if (type == "int")
        {
            format = vk::Format::eR32Sint;
        }
        else if (type == "ivec2")
        {
            format = vk::Format::eR32G32Sint;
        }
        else if (type == "ivec3")
        {
            format = vk::Format::eR32G32B32Sint;
        }
        else if (type == "ivec4")
        {
            format = vk::Format::eR32G32B32A32Sint;
        }
        else
        {
            LOGGER_ERROR("Unsupported Vulkan format type specified: %s", type.c_str());
        }
    }

    return format;
}

uint32_t getStrideFromType(const std::string& type)
{
    // TODO: add other base types and widths
    uint32_t size = 0;

    // floats
    if (type == "float" || type == "int")
    {
        size = 4;
    }
    else if (type == "vec2" || type == "ivec2")
    {
        size = 8;
    }
    else if (type == "vec3" || type == "ivec3")
    {
        size = 12;
    }
    else if (type == "vec4" || type == "ivec4")
    {
        size = 16;
    }
    else
    {
        LOGGER_ERROR(
            "Unsupported type specified: %s. Unable to determine stride size.", type.c_str());
    }

    return size;
}

bool isDepth(const vk::Format format)
{
    std::vector<vk::Format> depthFormats = {
        vk::Format::eD16Unorm,
        vk::Format::eX8D24UnormPack32,
        vk::Format::eD32Sfloat,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD32SfloatS8Uint};
    return std::find(depthFormats.begin(), depthFormats.end(), format) != std::end(depthFormats);
}

bool isStencil(const vk::Format format)
{
    std::vector<vk::Format> stencilFormats = {
        vk::Format::eS8Uint,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD32SfloatS8Uint};
    return std::find(stencilFormats.begin(), stencilFormats.end(), format) !=
        std::end(stencilFormats);
}

bool isBufferType(const vk::DescriptorType& type)
{
    if (type == vk::DescriptorType::eUniformBuffer || type == vk::DescriptorType::eStorageBuffer ||
        type == vk::DescriptorType::eUniformBufferDynamic ||
        type == vk::DescriptorType::eStorageBufferDynamic)
    {
        return true;
    }
    return false;
}

bool isSamplerType(const vk::DescriptorType& type)
{
    if (type == vk::DescriptorType::eSampler || type == vk::DescriptorType::eStorageImage ||
        type == vk::DescriptorType::eCombinedImageSampler ||
        type == vk::DescriptorType::eSampledImage)
    {
        return true;
    }
    return false;
}

} // namespace vkapi
