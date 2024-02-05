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

#include <unordered_set>
#include <vector>

namespace vkapi
{

// forward declarations
class VkContext;
class VkDriver;

/**
 * @brief A simplistic staging pool for CPU-only stages. Used when copying to and from
 * GPU only mem
 */
class StagingPool
{
public:
    explicit StagingPool(VmaAllocator& vmaAlloc);

    struct StageInfo
    {
        VkBuffer buffer;
        VkDeviceSize size;
        VmaAllocation mem;
        VmaAllocationInfo allocInfo;
        uint64_t frameLastUsed;
    };

    StageInfo* getStage(VkDeviceSize reqSize);

    StageInfo* create(VkDeviceSize size);

    void garbageCollection(uint64_t currentFrame);

    void clear();

private:
    // keep a reference to the memory allocator here
    VmaAllocator& vmaAlloc_;

    // a list of free stages and their size
    std::vector<StageInfo*> freeStages_;
    std::unordered_set<StageInfo*> inUseStages_;
};


/** @brief A wrapper around a VkBuffer allowing easier mem allocation using VMA
 * This is for dynamic mem type allocation, i.e. uniform buffers, etc.
 */
class Buffer
{
public:
    Buffer();
    virtual ~Buffer() = default;

    void alloc(VmaAllocator& vmaAlloc, vk::DeviceSize size, VkBufferUsageFlags usage) noexcept;

    void destroy(VmaAllocator& vmaAlloc) noexcept;

    static void mapToStage(void* data, size_t size, StagingPool::StageInfo* stage) noexcept;

    void mapToGpuBuffer(void* data, size_t dataSize) const noexcept;

    void downloadToHost(VkDriver& driver, void* hostBuffer, size_t dataSize) const noexcept;

    void copyStagedToGpu(
        VkDriver& driver,
        VkDeviceSize size,
        StagingPool::StageInfo* stage,
        VkBufferUsageFlags usage);

    void mapAndCopyToGpu(VkDriver& driver, VkDeviceSize size, VkBufferUsageFlags usage, void* data);

    vk::Buffer get();

    [[nodiscard]] uint64_t getSize() const;

    friend class ResourceCache;

protected:
    VmaAllocationInfo allocInfo_;
    VmaAllocation mem_;
    VkDeviceSize size_;
    VkBuffer buffer_;

    uint32_t framesUntilGc_;
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer() = default;

    void create(
        VkDriver& driver, VmaAllocator& vmaAlloc, StagingPool& pool, void* data, VkDeviceSize size);
};


class IndexBuffer : public Buffer
{
public:
    IndexBuffer() = default;

    void create(
        VkDriver& driver, VmaAllocator& vmaAlloc, StagingPool& pool, void* data, VkDeviceSize size);
};

} // namespace vkapi
