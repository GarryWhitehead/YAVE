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

#include "vulkan-api/common.h"

#include <array>
#include <unordered_map>
#include <vector>

namespace yave
{

// forward declarations
class IScene;
class IEngine;

// TODO: would be better to have a sorting key per queue type.
struct SortKey
{
    union
    {
        struct
        {
            //
            uint64_t screenLayer : 4;
            uint64_t viewLayer : 4;
            // a hash derived from the pipeline key
            uint64_t pipelineId : 32;
            uint64_t depth : 24;
        } s;

        uint64_t flags;

    } u;

    float depth = 0.0f; // for transparency;
};

using RenderQueueFunc = void (*)(IEngine&, IScene&, const vk::CommandBuffer&, void*, void*);

/**
 * @brief All the information required to render the item to a cmd buffer. This
 * is set by the renderer update function.
 */
struct RenderableQueueInfo
{
    // render callback function
    RenderQueueFunc renderFunc;

    void* renderableHandle;

    // data specific to the renderable - mainly drawing information
    void* renderableData;

    void* primitiveData;

    // the point in the render this will be drawn
    SortKey sortingKey;
};

class RenderQueue
{
public:
    static constexpr int MaxViewLayerCount = 6;

    // the type of queue to use when drawing. Only "Colour" supported at the moment.
    enum Type
    {
        Colour,
        Transparency,
        Depth,
        Count
    };

    RenderQueue();
    ~RenderQueue();

    // not copyable
    RenderQueue(const RenderQueue&) = delete;
    RenderQueue& operator=(const RenderQueue&) = delete;

    void resetAll();

    void pushRenderables(std::vector<RenderableQueueInfo>& newRenderables, const Type type);

    static SortKey createSortKey(uint8_t screenLayer, uint8_t viewLayer, size_t pipelineId);

    void sortQueue(Type type);

    void sortAll();

    void render(
        IEngine& engine,
        IScene& scene,
        const vk::CommandBuffer& cmd,
        RenderQueue::Type type,
        size_t startIdx,
        size_t endIdx);

    void
    render(IEngine& engine, IScene& scene, const vk::CommandBuffer& cmd, RenderQueue::Type type);

    /**
     * @brief Returns all renderables in the specified queue
     * @param part Specifies whether the queue should be sorted before fetching
     * @return A vector containing the renderables in the specified range
     */
    [[nodiscard]] const std::vector<RenderableQueueInfo>&
    queue(RenderQueue::Type type) const noexcept;

private:
    // ordered by queue type
    std::vector<RenderableQueueInfo> renderables_[Type::Count];
};

} // namespace yave
