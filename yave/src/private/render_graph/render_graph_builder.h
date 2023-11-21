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
#include "utility/colour.h"
#include "utility/cstring.h"
#include "vulkan-api/renderpass.h"

namespace vkapi
{
class Image;
class ImageView;
} // namespace vkapi

namespace yave::rg
{

// forward declerations
class RenderGraph;
class PassNodeBase;
struct PassDescriptor;

/**
 * @brief Useful helper functions for building the rendergraph. Used inside the
 * setup functor for declaring aspcets of the renderpass and uderlying
 * functionality.
 */
class RenderGraphBuilder
{
public:
    RenderGraphBuilder(RenderGraph* rGraph, PassNodeBase* node);

    /**
     * @ creates a texture resource for using as a render target in a graphics
     * pass
     */
    RenderGraphHandle
    createResource(const util::CString& name, const TextureResource::Descriptor& desc);

    RenderGraphHandle createSubResource(
        const util::CString& name,
        const TextureResource::Descriptor& desc,
        const RenderGraphHandle& parent);

    /**
     * @brief Adds a reader (i.e. input attachment) to the render pass.
     */
    RenderGraphHandle addReader(const RenderGraphHandle& handle, vk::ImageUsageFlags usage);

    /**
     * @brief Adds a writer (i.e. colour/depth/stencil attachment) to the pass
     */
    RenderGraphHandle addWriter(const RenderGraphHandle& handle, vk::ImageUsageFlags usage);

    RenderGraphHandle createRenderTarget(const util::CString& name, const PassDescriptor& desc);

    void addPresent(const RenderGraphHandle& presentHandle);

    void addSideEffect() noexcept;

private:
    // a reference to the graph and pass we are building
    RenderGraph* rGraph_;
    RenderPassNode* passNode_;
};

} // namespace yave::rg
