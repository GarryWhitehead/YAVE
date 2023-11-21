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

#include "yave/vertex_buffer.h"

#include <vulkan-api/driver.h>

namespace yave
{
class VkDriver;

class IVertexBuffer : public VertexBuffer
{
public:
    IVertexBuffer() = default;

    void shutDown(vkapi::VkDriver& driver);

    static auto attributeToWidthFormat(backend::BufferElementType type);

    void addAttributeI(backend::BufferElementType type, uint32_t binding);

    void buildI(vkapi::VkDriver& driver, uint32_t vertexCount, void* vertexData);

    vk::VertexInputAttributeDescription* getInputAttr() noexcept { return attributes_; }

    vk::VertexInputBindingDescription* getInputBind() noexcept { return bindDesc_; }

    [[nodiscard]] util::BitSetEnum<VertexBuffer::BindingType> getAtrributeBits() const noexcept;

    vkapi::VertexBuffer* getGpuBuffer(vkapi::VkDriver& driver) noexcept;

    // =============== client api virtual =======================

    void addAttribute(BindingType bindType, backend::BufferElementType attrType) override;

    void build(Engine* engine, uint32_t vertexCount, void* vertexData) override;

private:
    vk::VertexInputAttributeDescription attributes_[vkapi::PipelineCache::MaxVertexAttributeCount];
    vk::VertexInputBindingDescription bindDesc_[vkapi::PipelineCache::MaxVertexAttributeCount];
    vkapi::VertexBufferHandle vHandle_;
};

} // namespace yave