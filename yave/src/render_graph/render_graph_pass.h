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

#include "render_graph_handle.h"
#include "resources.h"
#include "utility/bitset_enum.h"
#include "utility/colour.h"
#include "utility/cstring.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/renderpass.h"

#include <array>
#include <functional>

namespace yave
{
namespace rg
{

class RenderGraph;
class RenderPassNode;
class RenderGraphResource;

struct PassAttachment
{
    RenderGraphHandle colour[vkapi::RenderTarget::MaxColourAttachCount];
    RenderGraphHandle depth;
    RenderGraphHandle stencil;
};

union PassAttachmentUnion
{
    PassAttachmentUnion() {}
    PassAttachment attach;
    RenderGraphHandle attachArray[vkapi::RenderTarget::MaxAttachmentCount] = {};
};

struct PassDescriptor
{
    PassDescriptor() = default;
    PassAttachmentUnion attachments;
    util::Colour4 clearColour {0.0f, 0.0f, 0.0f, 1.0f};
    uint8_t samples = 1;
    std::array<vkapi::LoadClearFlags, 2> dsLoadClearFlags = {vkapi::LoadClearFlags::DontCare};
    std::array<vkapi::StoreClearFlags, 2> dsStoreClearFlags = {vkapi::StoreClearFlags::DontCare};

    struct VkBackend
    {
        vkapi::RenderTargetHandle rtHandle;
    } vkBackend;
};

class RenderGraphPassBase
{
public:
    RenderGraphPassBase() = default;
    virtual ~RenderGraphPassBase() {}

    void setNode(RenderPassNode* node) { node_ = node; }
    RenderPassNode const* node() { return node_; }

    virtual void execute(vkapi::VkDriver& driver, const RenderGraphResource& resource) = 0;

protected:
    RenderPassNode* node_;
};

template <typename DataType, typename ExecuteFunc>
class RenderGraphPass : public RenderGraphPassBase
{
public:
    RenderGraphPass(ExecuteFunc&& execute) : execute_(std::move(execute)) {}

    void execute(vkapi::VkDriver& driver, const RenderGraphResource& resource) override
    {
        execute_(driver, data_, resource);
    }

    const DataType& getData() const noexcept { return data_; }

private:
    DataType data_;
    ExecuteFunc execute_;
};

} // namespace rg
} // namespace yave
