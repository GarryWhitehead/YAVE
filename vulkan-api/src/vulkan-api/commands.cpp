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

#include "commands.h"

#include "context.h"
#include "driver.h"
#include "swapchain.h"
#include "utility.h"
#include "utility/assertion.h"
#include "utility/logger.h"

#include <spdlog/spdlog.h>

namespace vkapi
{

CmdFence::CmdFence(VkContext& context) : context_(context)
{
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits(0));
    VK_CHECK_RESULT(context.device().createFence(&fenceInfo, nullptr, &fence));
}

Commands::Commands(VkDriver& driver, vk::Queue queue)
    : driver_(driver),
      currentCmdBuffer_(nullptr),
      currentSignal_(nullptr),
      submittedSignal_(nullptr),
      externalSignal_(nullptr),
      queue_(queue),
      availableCmdBuffers_(MaxCommandBufferSize)
{
    // create the main cmd pool for this buffer - TODO: we should allow for the
    // user to define the queue to use for the pool
    const VkContext& context = driver_.context();
    vk::CommandPoolCreateInfo createInfo {
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
            vk::CommandPoolCreateFlagBits::eTransient,
        context.queueIndices().graphics};
    VK_CHECK_RESULT(context.device().createCommandPool(&createInfo, nullptr, &cmdPool_));

    // create the semaphore for signalling a new frame is ready now
    for (auto& signal : signals_)
    {
        vk::SemaphoreCreateInfo semaphoreCreateInfo;
        VK_CHECK_RESULT(context.device().createSemaphore(&semaphoreCreateInfo, nullptr, &signal));
    }
}

Commands::~Commands()
{
    freeCmdBuffers();

    driver_.context().device().destroy(cmdPool_, nullptr);

    for (const auto& signal : signals_)
    {
        driver_.context().device().destroy(signal, nullptr);
    }
}

CmdBuffer& Commands::getCmdBuffer()
{
    if (currentCmdBuffer_)
    {
        return *currentCmdBuffer_;
    }

    auto& context = driver_.context();

    // wait for cmd buffers to finish before getting a new one.
    while (availableCmdBuffers_ == 0)
    {
        freeCmdBuffers();
    }

    size_t idx = 0;
    for (auto& buffer : cmdBuffers_)
    {
        if (!buffer.cmdBuffer)
        {
            currentCmdBuffer_ = &buffer;
            currentSignal_ = &signals_[idx];
            --availableCmdBuffers_;
            break;
        }
        ++idx;
    }
    ASSERT_LOG(currentCmdBuffer_);
    ASSERT_LOG(currentSignal_);

    vk::CommandBufferAllocateInfo allocInfo(cmdPool_, vk::CommandBufferLevel::ePrimary, 1);
    VK_CHECK_RESULT(
        context.device().allocateCommandBuffers(&allocInfo, &currentCmdBuffer_->cmdBuffer));

    vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    vk::CommandBufferBeginInfo beginInfo(usageFlags, nullptr);
    VK_CHECK_RESULT(currentCmdBuffer_->cmdBuffer.begin(&beginInfo));

    currentCmdBuffer_->fence = std::make_unique<CmdFence>(driver_.context());

    return *currentCmdBuffer_;
}

void Commands::freeCmdBuffers()
{
    auto& context = driver_.context();

    // wait for all cmdbuffers to finish their work except the current
    // buffer
    vk::Fence fences[Commands::MaxCommandBufferSize];
    uint32_t count = 0;
    for (auto& buffer : cmdBuffers_)
    {
        if (buffer.cmdBuffer && currentCmdBuffer_ != &buffer)
        {
            fences[count++] = buffer.fence->fence;
        }
    }
    if (count)
    {
        VK_CHECK_RESULT(context.device().waitForFences(count, fences, VK_TRUE, UINT64_MAX));
    }

    // Get all currently allocated fences and wait for them to finish
    for (auto& buffer : cmdBuffers_)
    {
        if (buffer.cmdBuffer)
        {
            auto result = context.device().waitForFences(1, &buffer.fence->fence, VK_TRUE, 0);
            if (result == vk::Result::eSuccess)
            {
                context.device().freeCommandBuffers(cmdPool_, 1, &buffer.cmdBuffer);
                buffer.cmdBuffer = VK_NULL_HANDLE;
                buffer.fence.reset();
                ++availableCmdBuffers_;
            }
        }
    }
}

void Commands::flush()
{
    // nothing to flush if we have no commands
    if (!currentCmdBuffer_)
    {
        return;
    }

    // reset the bound pipeline associated with this cmd buffer
    driver_.pipelineCache().setPipelineKeyToDefault();

    currentCmdBuffer_->cmdBuffer.end();

    std::vector<vk::Semaphore> waitSignals;
    waitSignals.reserve(2);

    if (submittedSignal_)
    {
        waitSignals.emplace_back(*submittedSignal_);
    }
    if (externalSignal_)
    {
        waitSignals.emplace_back(*externalSignal_);
    }

    vk::PipelineStageFlags flags[2] = {
        vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands};
    vk::SubmitInfo info {
        static_cast<uint32_t>(waitSignals.size()),
        waitSignals.data(),
        flags,
        1,
        &currentCmdBuffer_->cmdBuffer,
        1,
        currentSignal_};
    VK_CHECK_RESULT(queue_.submit(1, &info, currentCmdBuffer_->fence->fence));

    SPDLOG_DEBUG("Command flush:");
    if (submittedSignal_)
    {
        SPDLOG_DEBUG("wait signal (submitted): {:p}", fmt::ptr((void*)*submittedSignal_));
    }
    if (externalSignal_)
    {
        SPDLOG_DEBUG("wait signal (external): {:p}", fmt::ptr((void*)*externalSignal_));
    }
    if (currentSignal_)
    {
        SPDLOG_DEBUG("signal: {:p}", fmt::ptr((void*)*currentSignal_));
    }

    currentCmdBuffer_ = nullptr;
    externalSignal_ = nullptr;
    submittedSignal_ = currentSignal_;
}

vk::Semaphore* Commands::getFinishedSignal() noexcept
{
    vk::Semaphore* output = submittedSignal_;
    submittedSignal_ = nullptr;

    SPDLOG_DEBUG("Acquired finished signal: {:p}", fmt::ptr((void*)*output));

    return output;
}


} // namespace vkapi
