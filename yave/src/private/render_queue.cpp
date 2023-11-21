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

#include "render_queue.h"

#include "engine.h"

namespace yave
{

RenderQueue::RenderQueue() = default;
RenderQueue::~RenderQueue() = default;

void RenderQueue::resetAll()
{
    for (size_t i = 0; i < RenderQueue::Type::Count; ++i)
    {
        renderables_[i].clear();
    }
}

void RenderQueue::pushRenderables(
    std::vector<RenderableQueueInfo>& newRenderables, const RenderQueue::Type type)
{
    size_t newSize = newRenderables.size();
    std::vector<RenderableQueueInfo>& rQueue = renderables_[type];
    rQueue.clear();
    rQueue.resize(newSize);

    std::copy(newRenderables.begin(), newRenderables.end(), rQueue.begin());
}

const std::vector<RenderableQueueInfo>&
RenderQueue::queue(const RenderQueue::Type type) const noexcept
{
    return renderables_[type];
}

void RenderQueue::sortQueue(const RenderQueue::Type type)
{
    std::vector<RenderableQueueInfo>& rQueue = renderables_[type];
    if (rQueue.empty())
    {
        return;
    }

    // TODO : use a radix sort instead
    std::sort(
        rQueue.begin(), rQueue.end(), [](const RenderableQueueInfo& lhs, RenderableQueueInfo& rhs) {
            return lhs.sortingKey.u.flags < rhs.sortingKey.u.flags;
        });
}

void RenderQueue::sortAll()
{
    for (size_t i = 0; i < RenderQueue::Type::Count; ++i)
    {
        sortQueue(static_cast<RenderQueue::Type>(i));
    }
}

SortKey RenderQueue::createSortKey(uint8_t screenLayer, uint8_t viewLayer, size_t pipelineId)
{
    SortKey key;
    key.u.s.screenLayer = (uint64_t)screenLayer;
    key.u.s.viewLayer = (uint64_t)viewLayer;
    key.u.s.pipelineId = pipelineId;
    key.u.s.depth = 0; // TODO - this is camera-view . mesh_centre - camera-pos

    return key;
}

void RenderQueue::render(
    IEngine& engine,
    IScene& scene,
    const vk::CommandBuffer& cmd,
    RenderQueue::Type type,
    size_t startIdx,
    size_t endIdx)
{
    const auto& queue = renderables_[type];
    ASSERT_FATAL(
        startIdx <= endIdx,
        "Start index is greater than the end index (start: %i; end: %i)",
        startIdx,
        endIdx);
    sortQueue(type);
    for (size_t i = startIdx; i < endIdx; ++i)
    {
        const RenderableQueueInfo& info = queue[i];
        info.renderFunc(engine, scene, cmd, info.renderableData, info.primitiveData);
    }
}

void RenderQueue::render(
    IEngine& engine, IScene& scene, const vk::CommandBuffer& cmd, RenderQueue::Type type)
{
    render(engine, scene, cmd, type, 0, renderables_[type].size());
}

} // namespace yave
