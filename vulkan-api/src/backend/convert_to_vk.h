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

#include "enums.h"

#include "vulkan/vulkan.hpp"

namespace backend
{

vk::BlendFactor blendFactorToVk(backend::BlendFactor factor);

vk::BlendOp blendOpToVk(backend::BlendOp op);

vk::SamplerAddressMode samplerAddrModeToVk(SamplerAddressMode mode);

vk::Filter samplerFilterToVk(SamplerFilter filter);

vk::CullModeFlagBits cullModeToVk(CullMode mode);

vk::CompareOp compareOpToVk(CompareOp op);

vk::PrimitiveTopology primitiveTopologyToVk(PrimitiveTopology topo);

vk::IndexType indexBufferTypeToVk(backend::IndexBufferType type);

vk::Format textureFormatToVk(backend::TextureFormat type);

} // namespace backend
