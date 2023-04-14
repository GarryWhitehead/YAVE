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

#include <cstdint>

#include "vulkan-api/driver.h"

#include "yave/index_buffer.h"

namespace yave
{
class Engine;

class IIndexBuffer : public IndexBuffer
{
public:
    IIndexBuffer();
    ~IIndexBuffer();

    void shutDown(vkapi::VkDriver& driver);

    void buildI(
        vkapi::VkDriver& driver,
        uint32_t indicesCount,
        void* indicesData,
        backend::IndexBufferType type);

    vkapi::IndexBuffer* getGpuBuffer(vkapi::VkDriver& driver) noexcept;

    uint64_t getIndicesSize() const noexcept { return indicesCount_; }

    backend::IndexBufferType getBufferType() noexcept { return bufferType_; }

    // =============== client api virtual =======================

    void build(
        Engine* engine,
        uint32_t indicesCount,
        void* indicesData,
        backend::IndexBufferType type) override;

private:
    vkapi::IndexBufferHandle ihandle_;
    backend::IndexBufferType bufferType_;
    uint64_t indicesCount_;
};

}