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

#include <vector>

namespace vkapi
{

bool isDepth(vk::Format format);
bool isStencil(vk::Format format);
bool isBufferType(const vk::DescriptorType& type);
bool isSamplerType(const vk::DescriptorType& type);

vk::Format findSupportedFormat(
    std::vector<vk::Format>& formats,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags formatFeature,
    vk::PhysicalDevice& gpu);

vk::Format getSupportedDepthFormat(vk::PhysicalDevice& gpu);

// Takes a string of certain type and if valid, returns as a vulkan recognisable
// type.
vk::Format convertToVk(const std::string& type, uint32_t width);

// Derives from the type specified, the stride in bytes
uint32_t getStrideFromType(const std::string& type);

} // namespace vkapi
