/* Copyright (c) 2018-2020 Garry Whitehead
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
#include "utility/assertion.h"
#include "utility/compiler.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace vkapi
{
// forward declerations
class VkDriver;
class VkContext;

struct CmdFence
{
    CmdFence(VkContext& context);

    VkContext& context_;
    vk::Fence fence;
};

struct CmdBuffer
{
    vk::CommandBuffer cmdBuffer;
    std::unique_ptr<CmdFence> fence;
};

class Commands
{
public:
    // some erbitarty numbers which need monitoring for possible issues due to
    // overflow
    constexpr static uint32_t MaxCommandBufferSize = 10;

    struct ThreadedCmdBuffer
    {
        vk::CommandBuffer secondary;
        vk::CommandPool cmdPool;
        bool isExecuted = false;
    };

    Commands(VkDriver& driver, vk::Queue queue);
    ~Commands();

    // not copyable
    Commands(const Commands&) = delete;
    Commands& operator=(const Commands) = delete;

    CmdBuffer& getCmdBuffer();

    void freeCmdBuffers();

    void flush();

    vk::Semaphore* getFinishedSignal() noexcept;

    void setExternalWaitSignal(vk::Semaphore* sp) noexcept
    {
        ASSERT_FATAL(sp, "External semaphore is nullptr");
        externalSignal_ = sp;
    }

private:
    VkDriver& driver_;

    /// the main command pool - only to be used on the main thread
    vk::CommandPool cmdPool_;

    CmdBuffer* currentCmdBuffer_;
    vk::Semaphore* currentSignal_;

    // current semaphore that has been submitted to the queue
    vk::Semaphore* submittedSignal_;

    // wait semaphore passed by the client
    vk::Semaphore* externalSignal_;

    vk::Queue queue_;

    std::array<CmdBuffer, MaxCommandBufferSize> cmdBuffers_;
    std::array<vk::Semaphore, MaxCommandBufferSize> signals_;

    size_t availableCmdBuffers_;
};


} // namespace vkapi
