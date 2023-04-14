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
#include <unordered_set>

namespace vkapi
{

// forward declerations
class VkContext;
class VkDriver;

/**
 * @brief A simplisitic staging pool for CPU-only stages. Used when copying to
 * GPU only mem
 */
class StagingPool
{
public:
    StagingPool(VmaAllocator& vmaAlloc);

    struct StageInfo
    {
        VkBuffer buffer;
        VkDeviceSize size;
        VmaAllocation mem;
        VmaAllocationInfo allocInfo;
        uint64_t frameLastUsed;
    };

    StageInfo* getStage(VkDeviceSize reqSize);

    StageInfo* create(const VkDeviceSize size);

    void garbageCollection(uint64_t currentFrame);

    void clear();

private:
    // keep a refernce to the memory allocator here
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

    void prepare(
        VmaAllocator& vmaAlloc, vk::DeviceSize size, VkBufferUsageFlags usage);

    void destroy(VmaAllocator& vmaAlloc);

    void mapToStage(void* data, size_t size, StagingPool::StageInfo* stage);
    void mapToGpuBuffer(void* data, size_t dataSize); 

    void createBuffer(
        VmaAllocator& vmaAlloc, VkBufferUsageFlags usage, VkDeviceSize size);

    void copyStagedToGpu(
        VkDriver& driver,
        VkDeviceSize size,
        StagingPool::StageInfo* stage,
        VkBufferUsageFlags usage);

    void mapAndCopyToGpu(
        VkDriver& driver,
        StagingPool& pool,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        void* data);

    vk::Buffer get();
  
    uint64_t getSize() const;
    uint64_t getOffset() const;

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
        VkDriver& driver,
        VkContext& context,
        VmaAllocator& vmaAlloc,
        StagingPool& pool,
        void* data,
        const VkDeviceSize size);

};


class IndexBuffer : public Buffer
{
public:

    IndexBuffer() = default;

    void create(
        VkDriver& driver,
        VkContext& context,
        VmaAllocator& vmaAlloc,
        StagingPool& pool,
        void* data,
        const VkDeviceSize size);

};

} // namespace vkapi
