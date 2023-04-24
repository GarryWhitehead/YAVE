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

#include "colour_pass.h"

#include "camera.h"
#include "engine.h"
#include "managers/renderable_manager.h"
#include "material.h"
#include "render_graph/render_graph_builder.h"
#include "render_graph/render_graph_pass.h"
#include "render_graph/rendergraph_resource.h"
#include "render_queue.h"
#include "renderable.h"
#include "scene.h"
#include "utility/assertion.h"
#include "vulkan-api/common.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/image.h"
#include "vulkan-api/pipeline.h"
#include "vulkan-api/pipeline_cache.h"

namespace yave
{

void ColourPass::render(
    IEngine& engine,
    IScene& scene,
    rg::RenderGraph& rGraph,
    uint32_t width,
    uint32_t height,
    vk::Format depthFormat)
{
    // add the main colour pass
    auto rg = rGraph.addPass<ColourPassData>(
        "DeferredPass",
        [&](rg::RenderGraphBuilder& builder, ColourPassData& data) {
            rg::TextureResource::Descriptor desc;
            desc.width = width;
            desc.height = height;
            desc.format = vk::Format::eR16G16B16A16Sfloat;
            data.position = builder.createResource("position", desc);

            desc.format = vk::Format::eR8G8B8A8Unorm;
            data.colour = builder.createResource("colour", desc);

            desc.format = vk::Format::eR8G8B8A8Unorm;
            data.normal = builder.createResource("normal", desc);

            desc.format = vk::Format::eR16G16Sfloat;
            data.pbr = builder.createResource("pbr", desc);

            desc.format = vk::Format::eR16G16B16A16Sfloat;
            data.emissive = builder.createResource("emissive", desc);

            desc.format = depthFormat;
            data.depth = builder.createResource("depth", desc);

            auto* blackboard = rGraph.getBlackboard();
            // store all the gbuffer resource handles for sampling in a later
            // pass
            blackboard->add("position", data.position);
            blackboard->add("colour", data.colour);
            blackboard->add("normal", data.normal);
            blackboard->add("emissive", data.emissive);
            blackboard->add("pbr", data.pbr);
            blackboard->add("gbufferDepth", data.depth);

            data.position =
                builder.addWriter(data.position, vk::ImageUsageFlagBits::eColorAttachment);
            data.colour = builder.addWriter(data.colour, vk::ImageUsageFlagBits::eColorAttachment);
            data.normal = builder.addWriter(data.normal, vk::ImageUsageFlagBits::eColorAttachment);
            data.pbr = builder.addWriter(data.pbr, vk::ImageUsageFlagBits::eColorAttachment);
            data.emissive =
                builder.addWriter(data.emissive, vk::ImageUsageFlagBits::eColorAttachment);
            data.depth =
                builder.addWriter(data.depth, vk::ImageUsageFlagBits::eDepthStencilAttachment);

            builder.addSideEffect();

            rg::PassDescriptor passDesc;
            passDesc.attachments.attach.colour[0] = data.colour;
            passDesc.attachments.attach.colour[1] = data.position;
            passDesc.attachments.attach.colour[2] = data.normal;
            passDesc.attachments.attach.colour[3] = data.emissive;
            passDesc.attachments.attach.colour[4] = data.pbr;
            passDesc.attachments.attach.depth = {data.depth};
            passDesc.dsLoadClearFlags = {vkapi::LoadClearFlags::Clear};
            data.rt = builder.createRenderTarget("deferredTarget", passDesc);
        },
        [=, &scene, &engine](
            ::vkapi::VkDriver& driver,
            const ColourPassData& data,
            const rg::RenderGraphResource& resources) {
            auto& cmds = driver.getCommands();
            auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

            const auto& info = resources.getRenderPassInfo(data.rt);
            auto& queue = scene.getRenderQueue();

            driver.beginRenderpass(cmdBuffer, info.data, info.handle);
            queue.render(engine, scene, cmdBuffer, RenderQueue::Type::Colour);
            driver.endRenderpass(cmdBuffer);

            cmds.flush();
        });
}

void ColourPass::drawCallback(
    IEngine& engine,
    IScene& scene,
    const vk::CommandBuffer& cmdBuffer,
    void* renderableData,
    void* primitiveData)
{
    auto& driver = engine.driver();

    IRenderable* renderData = static_cast<IRenderable*>(renderableData);
    ASSERT_LOG(renderData);

    IRenderPrimitive* prim = static_cast<IRenderPrimitive*>(primitiveData);
    IMaterial* mat = prim->getMaterial();
    auto* programBundle = mat->getProgram();

    IVertexBuffer* vBuffer = prim->getVertexBuffer();
    IIndexBuffer* iBuffer = prim->getIndexBuffer();

    bool hasSkin = prim->getVariantBits().testBit(IRenderPrimitive::Variants::HasSkin);

    std::vector<uint32_t> dynamicOffsets = {renderData->getMeshDynamicOffset()};
    ASSERT_FATAL(
        dynamicOffsets[0] != TransformInfo::Uninitialised,
        "A mesh dynmaic offset must be initialised.");
    if (hasSkin)
    {
        dynamicOffsets.emplace_back(renderData->getSkinDynamicOffset());
    }

    vk::Buffer vertexBuffer = vBuffer ? vBuffer->getGpuBuffer(driver)->get() : nullptr;
    vk::Buffer indexBuffer = iBuffer ? iBuffer->getGpuBuffer(driver)->get() : nullptr;
    vk::VertexInputAttributeDescription* attrDesc = vBuffer ? vBuffer->getInputAttr() : nullptr;
    vk::VertexInputBindingDescription* bindDesc = vBuffer ? vBuffer->getInputBind() : nullptr;

    driver.draw(
        cmdBuffer,
        *programBundle,
        vertexBuffer,
        indexBuffer,
        attrDesc,
        bindDesc,
        dynamicOffsets);
}

} // namespace yave
