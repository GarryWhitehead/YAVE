/* Copyright (c) 2022-2023 Garry Whitehead
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

#include "buffer.h"

#include "commands.h"
#include "context.h"
#include "driver.h"
#include "utility/assertion.h"

#include <cstring>

namespace vkapi
{

// ================== StagingPool =======================
StagingPool::StagingPool(VmaAllocator& vmaAlloc) : vmaAlloc_(vmaAlloc) {}

StagingPool::StageInfo* StagingPool::create(const VkDeviceSize size)
{
    ASSERT_LOG(size > 0);

    StageInfo* stage = new StageInfo();
    stage->size = size;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.size = size;

    // cpu staging pool
    VmaAllocationCreateInfo createInfo = {};
    createInfo.usage = VMA_MEMORY_USAGE_AUTO;
    createInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VMA_CHECK_RESULT(vmaCreateBuffer(
        vmaAlloc_, &bufferInfo, &createInfo, &stage->buffer, &stage->mem, &stage->allocInfo));

    return stage;
}

StagingPool::StageInfo* StagingPool::getStage(VkDeviceSize reqSize)
{
    // check for a free staging space that is equal or greater than the required
    // size
    auto iter = std::lower_bound(
        freeStages_.begin(),
        freeStages_.end(),
        reqSize,
        [](StageInfo* lhs, const VkDeviceSize rhs) { return lhs->size < rhs; });

    // if we have a free staging area, return that. Otherwise allocate a new
    // stage
    if (iter != freeStages_.end())
    {
        StageInfo* stage = *iter;
        freeStages_.erase(iter);
        inUseStages_.insert(stage);
        return stage;
    }

    StageInfo* newStage = create(reqSize);
    inUseStages_.insert(newStage);
    return newStage;
}

void StagingPool::garbageCollection(uint64_t currentFrame)
{
    // destroy buffers that have not been used in some time
    std::vector<StageInfo*> newFreeStages;
    for (auto* stage : freeStages_)
    {
        uint64_t collectionFrame = stage->frameLastUsed + Commands::MaxCommandBufferSize;
        if (collectionFrame < currentFrame)
        {
            vmaDestroyBuffer(vmaAlloc_, stage->buffer, stage->mem);
            delete stage;
        }
        else
        {
            freeStages_.emplace_back(stage);
        }
    }
    freeStages_.swap(newFreeStages);

    // buffers that were currently in use can be moved to the
    // free stage container once it is safe to do so.
    std::unordered_set<StageInfo*> newInUseStages;
    for (auto* stage : inUseStages_)
    {
        uint64_t collectionFrame = stage->frameLastUsed + Commands::MaxCommandBufferSize;
        if (collectionFrame < currentFrame)
        {
            freeStages_.emplace_back(stage);
        }
        else
        {
            newInUseStages.insert(stage);
        }
    }
    inUseStages_.swap(newInUseStages);
}

void StagingPool::clear()
{
    for (const auto* stage : freeStages_)
    {
        vmaDestroyBuffer(vmaAlloc_, stage->buffer, stage->mem);
        delete stage;
    }

    for (auto* stage : inUseStages_)
    {
        vmaDestroyBuffer(vmaAlloc_, stage->buffer, stage->mem);
        delete stage;
    }
}

// ==================== Buffer ==========================

Buffer::Buffer() = default;

void Buffer::prepare(VmaAllocator& vmaAlloc, vk::DeviceSize buffSize, VkBufferUsageFlags usage)
{
    size_ = buffSize;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buffSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | usage;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VMA_CHECK_RESULT(
        vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocCreateInfo, &buffer_, &mem_, &allocInfo_));
}

void Buffer::createBuffer(VmaAllocator& vmaAlloc, VkBufferUsageFlags usage, VkDeviceSize size)
{
    // create GPU memory
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;

    VmaAllocationCreateInfo createInfo = {};
    createInfo.usage = VMA_MEMORY_USAGE_AUTO;
    VMA_CHECK_RESULT(
        vmaCreateBuffer(vmaAlloc, &bufferInfo, &createInfo, &buffer_, &mem_, &allocInfo_));
}

void Buffer::mapToStage(void* data, size_t dataSize, StagingPool::StageInfo* stage)
{
    ASSERT_FATAL(data, "Data pointer is nullptr for buffer mapping.");
    memcpy(stage->allocInfo.pMappedData, data, dataSize);
}

void Buffer::mapToGpuBuffer(void* data, size_t dataSize)
{
    ASSERT_FATAL(data, "Data pointer is nullptr for buffer mapping.");
    memcpy(allocInfo_.pMappedData, data, dataSize);
}

void Buffer::mapAndCopyToGpu(
    VkDriver& driver, StagingPool& pool, VkDeviceSize size, VkBufferUsageFlags usage, void* data)
{
    StagingPool::StageInfo* stage = pool.getStage(size);
    mapToStage(data, size, stage);
    copyStagedToGpu(driver, size, stage, usage);
}

void Buffer::copyStagedToGpu(
    VkDriver& driver, VkDeviceSize size, StagingPool::StageInfo* stage, VkBufferUsageFlags usage)
{
    // copy from the staging area to the allocated GPU memory
    auto& cmds = driver.getCommands();
    auto& cmd = cmds.getCmdBuffer();

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd.cmdBuffer, stage->buffer, buffer_, 1, &copyRegion);

    // ensure that the copy finishes before the next frames draw call
    if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT || usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    {
        vk::BufferMemoryBarrier memBarrier {
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eVertexAttributeRead |
                vk::AccessFlagBits::eIndexRead,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            buffer_,
            0,
            VK_WHOLE_SIZE};
        cmd.cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eVertexInput,
            vk::DependencyFlags(0),
            0,
            nullptr,
            1,
            &memBarrier,
            0,
            nullptr);
    }
    else if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    {
        vk::BufferMemoryBarrier memBarrier {
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eUniformRead,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            buffer_,
            0,
            VK_WHOLE_SIZE};
        cmd.cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eVertexShader |
                vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlags(0),
            0,
            nullptr,
            1,
            &memBarrier,
            0,
            nullptr);
    }
}

void Buffer::destroy(VmaAllocator& vmaAlloc) { vmaDestroyBuffer(vmaAlloc, buffer_, mem_); }

vk::Buffer Buffer::get() { return vk::Buffer(buffer_); }

uint64_t Buffer::getSize() const { return size_; }

uint64_t Buffer::getOffset() const { return allocInfo_.offset; }

// ================ Vertex buffer =======================
// Note: The following functions use VMA hence don't use the vulkan hpp C++
// style vulkan bindings. Hence, the code has been left in this verbose format -
// as it doesn't follow the format of the rest of the codebase

void VertexBuffer::create(
    VkDriver& driver,
    VkContext& context,
    VmaAllocator& vmaAlloc,
    StagingPool& pool,
    void* data,
    const VkDeviceSize dataSize)
{
    ASSERT_FATAL(data, "Data pointer is nullptr for buffer copy.");

    // get a staging pool for hosting on the CPU side
    StagingPool::StageInfo* stage = pool.getStage(dataSize);

    mapToStage(data, dataSize, stage);
    createBuffer(vmaAlloc, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, dataSize);
    copyStagedToGpu(driver, dataSize, stage, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

// ======================= IndexBuffer ================================

void IndexBuffer::create(
    VkDriver& driver,
    VkContext& context,
    VmaAllocator& vmaAlloc,
    StagingPool& pool,
    void* data,
    const VkDeviceSize dataSize)
{
    ASSERT_FATAL(data, "Data pointer is nullptr for index buffer initialisation.");

    // get a staging pool for hosting on the CPU side
    StagingPool::StageInfo* stage = pool.getStage(dataSize);
    ASSERT_LOG(stage);

    mapToStage(data, dataSize, stage);
    createBuffer(vmaAlloc, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, dataSize);
    copyStagedToGpu(driver, dataSize, stage, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}


} // namespace vkapi
