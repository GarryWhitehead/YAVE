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

#include "vertex_buffer.h"

#include "engine.h"

namespace yave
{

auto IVertexBuffer::attributeToWidthFormat(backend::BufferElementType attr)
{
    uint32_t width = 0;
    vk::Format format;

    switch (attr)
    {
        case backend::BufferElementType::Float: {
            width = 4;
            format = vk::Format::eR32Sfloat;
            break;
        }
        case backend::BufferElementType::Float2: {
            width = 8;
            format = vk::Format::eR32G32Sfloat;
            break;
        }
        case backend::BufferElementType::Float3: {
            width = 12;
            format = vk::Format::eR32G32B32Sfloat;
            break;
        }
        case backend::BufferElementType::Float4: {
            width = 16;
            format = vk::Format::eR32G32B32A32Sfloat;
            break;
        }
        case backend::BufferElementType::Uint: {
            width = 1;
            format = vk::Format::eR8Uint;
            break;
        }
        case backend::BufferElementType::Int: {
            width = 1;
            format = vk::Format::eR8Unorm;
            break;
        }
        case backend::BufferElementType::Int2: {
            width = 2;
            format = vk::Format::eR8G8Unorm;
            break;
        }
        case backend::BufferElementType::Int3: {
            width = 3;
            format = vk::Format::eR8G8B8Unorm;
            break;
        }
        case backend::BufferElementType::Int4: {
            width = 4;
            format = vk::Format::eR8G8B8A8Unorm;
            break;
        }
        case backend::BufferElementType::Mat3: {
            width = 36;
            format = vk::Format::eR32G32B32Sfloat;
            break;
        }
        case backend::BufferElementType::Mat4: {
            width = 64;
            format = vk::Format::eR32G32B32A32Sfloat;
            break;
        }
    }

    return std::make_tuple(width, format);
}

void IVertexBuffer::shutDown(vkapi::VkDriver& driver) { driver.deleteVertexBuffer(vHandle_); }

void IVertexBuffer::addAttributeI(backend::BufferElementType type, uint32_t binding)
{
    ASSERT_FATAL(
        binding < vkapi::PipelineCache::MaxVertexAttributeCount,
        "Attributes out of range (%d > %d)",
        binding,
        vkapi::PipelineCache::MaxVertexAttributeCount);
    const auto& [width, format] = attributeToWidthFormat(type);
    attributes_[binding] = {binding, 0, format, width};
}

util::BitSetEnum<VertexBuffer::BindingType> IVertexBuffer::getAtrributeBits() const noexcept
{
    util::BitSetEnum<VertexBuffer::BindingType> attrBits;
    for (const auto& attr : attributes_)
    {
        if (attr.format != vk::Format::eUndefined)
        {
            attrBits |= (VertexBuffer::BindingType)attr.location;
        }
    }
    return attrBits;
}

void IVertexBuffer::buildI(vkapi::VkDriver& driver, uint32_t vertexCount, void* vertexData)
{
    ASSERT_LOG(vertexData);

    // if the buffer has already been created, map the data into the
    // already existing allocated space. Note: this will reallocate if
    // the space already allocated is too small.
    if (vHandle_)
    {
        driver.mapVertexBuffer(vHandle_, vertexCount, vertexData);
        return;
    }

    // sort out the attribute stride and offsets
    uint32_t stride = 0;
    for (size_t idx = 0; idx < vkapi::PipelineCache::MaxVertexAttributeCount; ++idx)
    {
        if (attributes_[idx].format != vk::Format::eUndefined)
        {
            stride += attributes_[idx].offset;
        }
    }

    uint32_t currOffset = 0;
    uint32_t prevOffset = 0;
    for (size_t idx = 0; idx < vkapi::PipelineCache::MaxVertexAttributeCount; ++idx)
    {
        if (attributes_[idx].format != vk::Format::eUndefined)
        {
            currOffset = attributes_[idx].offset;
            attributes_[idx].offset = prevOffset;
            prevOffset += currOffset;
        }
    }

    // only supporting a single binding descriptor at present
    bindDesc_[0] = {0, stride, vk::VertexInputRate::eVertex};
    vHandle_ = driver.addVertexBuffer(vertexCount, vertexData);
}

vkapi::VertexBuffer* IVertexBuffer::getGpuBuffer(vkapi::VkDriver& driver) noexcept
{
    return driver.getVertexBuffer(vHandle_);
}

// ====================== client api ========================

VertexBuffer::VertexBuffer() = default;
VertexBuffer::~VertexBuffer() = default;

void IVertexBuffer::build(Engine* engine, uint32_t vertexCount, void* vertexData)
{
    buildI(reinterpret_cast<IEngine*>(engine)->driver(), vertexCount, vertexData);
}

void IVertexBuffer::addAttribute(BindingType bindType, backend::BufferElementType attrType)
{
    addAttributeI(attrType, static_cast<uint32_t>(bindType));
}

} // namespace yave