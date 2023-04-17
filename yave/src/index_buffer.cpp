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

#include "index_buffer.h"

#include "engine.h"

namespace yave
{

IIndexBuffer::IIndexBuffer() : bufferType_(backend::IndexBufferType::Uint32) {}
IIndexBuffer::~IIndexBuffer() = default;

void IIndexBuffer::shutDown(vkapi::VkDriver& driver) { driver.deleteIndexBuffer(ihandle_); }

void IIndexBuffer::buildI(
    vkapi::VkDriver& driver,
    uint32_t indicesCount,
    void* indicesData,
    backend::IndexBufferType type)
{
    uint32_t byteSize =
        type == backend::IndexBufferType::Uint16 ? sizeof(uint16_t) : sizeof(uint32_t);

    if (ihandle_)
    {
        driver.mapIndexBuffer(ihandle_, indicesCount * byteSize, indicesData);
        return;
    }
    indicesCount_ = indicesCount;
    bufferType_ = type;

    ihandle_ = driver.addIndexBuffer(indicesCount * byteSize, indicesData);
}

vkapi::IndexBuffer* IIndexBuffer::getGpuBuffer(vkapi::VkDriver& driver) noexcept
{
    return driver.getIndexBuffer(ihandle_);
}

// ===================== clinet api =======================

IndexBuffer::IndexBuffer() {}
IndexBuffer::~IndexBuffer() {}

void IIndexBuffer::build(
    Engine* engine, uint32_t indicesCount, void* indicesData, backend::IndexBufferType type)
{
    buildI(reinterpret_cast<IEngine*>(engine)->driver(), indicesCount, indicesData, type);
}

} // namespace yave
