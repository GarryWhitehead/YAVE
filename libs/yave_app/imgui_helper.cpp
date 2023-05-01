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

#include "imgui_helper.h"

#include "app.h"
#include "yave/engine.h"
#include "yave/index_buffer.h"
#include "yave/material.h"
#include "yave/object.h"
#include "yave/object_manager.h"
#include "yave/render_primitive.h"
#include "yave/renderable.h"
#include "yave/renderable_manager.h"
#include "yave/scene.h"
#include "yave/texture.h"
#include "yave/transform_manager.h"
#include "yave/vertex_buffer.h"

#include <cstdlib>
#include <filesystem>

namespace yave
{

ImGuiHelper::ImGuiHelper(Engine* engine, Scene* scene, std::filesystem::path& fontPath)
    : context_(ImGui::CreateContext()), engine_(engine), texture_(nullptr)
{
    ImGuiIO& io = ImGui::GetIO();

    if (!fontPath.empty() && std::filesystem::exists(fontPath))
    {
        auto platformPath = fontPath.make_preferred();
        ImFont* font = io.Fonts->AddFontFromFileTTF(platformPath.string().c_str(), 16.0f);
        ASSERT_FATAL(font, "Error whilst trying to add font to IMGUI.");
    }

    static uint8_t* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    uint32_t dataSize = width * height * 4;
    texture_ = engine->createTexture();
    Texture::Params params {
        pixels,
        dataSize,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        backend::TextureFormat::RGBA8,
        backend::ImageUsage::Sampled};
    texture_->setTexture(params);
    sampler_ = TextureSampler(backend::SamplerFilter::Nearest, backend::SamplerFilter::Nearest);

    ObjectManager* objManager = engine->getObjectManager();
    rendObj_ = objManager->createObject();
    scene->addObject(rendObj_);
    renderable_ = engine_->createRenderable();

    ImGui::StyleColorsDark();
}

ImGuiHelper ::~ImGuiHelper() {}

void ImGuiHelper::beginFrame(Application& app) noexcept
{
    ImGui::SetCurrentContext(context_);
    ImGuiIO& io = ImGui::GetIO();

    ImGui::NewFrame();

    // execute imgui commands defined by the user
    app.uiCallback(engine_);

    // generate the draw data
    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();

    createBuffers(drawData->CmdListsCount);

    updateDrawCommands(drawData, io);
}

void ImGuiHelper::createBuffers(size_t reqBufferCount)
{
    if (reqBufferCount > vBuffers_.size())
    {
        size_t currentSize = vBuffers_.size();
        vBuffers_.resize(reqBufferCount);
        for (size_t idx = currentSize; idx < reqBufferCount; ++idx)
        {
            vBuffers_[idx] = engine_->createVertexBuffer();
            vBuffers_[idx]->addAttribute(
                VertexBuffer::BindingType::Position, backend::BufferElementType::Float2);
            vBuffers_[idx]->addAttribute(
                VertexBuffer::BindingType::Uv, backend::BufferElementType::Float2);
            vBuffers_[idx]->addAttribute(
                VertexBuffer::BindingType::Colour, backend::BufferElementType::Int4);
        }
    }

    if (reqBufferCount > iBuffers_.size())
    {
        size_t currentSize = iBuffers_.size();
        iBuffers_.resize(reqBufferCount);
        for (size_t idx = currentSize; idx < reqBufferCount; ++idx)
        {
            iBuffers_[idx] = engine_->createIndexBuffer();
        }
    }
}

void ImGuiHelper::setDisplaySize(float winWidth, float winHeight, float scaleX, float scaleY)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(winWidth, winHeight);
    io.DisplayFramebufferScale = ImVec2(scaleX, scaleY);
}

void ImGuiHelper::updateDrawCommands(ImDrawData* drawData, const ImGuiIO& io)
{
    int width = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int height = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (width == 0 || height == 0)
    {
        return;
    }
    drawData->ScaleClipRects(io.DisplayFramebufferScale);

    if (!drawData->CmdListsCount)
    {
        return;
    }

    auto rendManager = engine_->getRenderManager();

    size_t primCount = 0;
    for (int32_t idx = 0; idx < drawData->CmdListsCount; idx++)
    {
        primCount += drawData->CmdLists[idx]->CmdBuffer.size();
    }
    size_t currentSize = renderParams_.size();
    renderParams_.resize(primCount);
    for (size_t idx = currentSize; idx < primCount; ++idx)
    {
        renderParams_[idx].material = rendManager->createMaterial();
        renderParams_[idx].prim = engine_->createRenderPrimitive();
    }

    renderable_->setPrimitiveCount(primCount);
    renderable_->skipVisibilityChecks();

    rendManager->destroy(rendObj_);

    uint32_t primIdx = 0;
    for (int32_t idx = 0; idx < drawData->CmdListsCount; idx++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[idx];

        // copy the vertices and indices for this set of commands to the device
        vBuffers_[idx]->build(
            engine_, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), cmd_list->VtxBuffer.Data);
        iBuffers_[idx]->build(
            engine_,
            cmd_list->IdxBuffer.Size,
            cmd_list->IdxBuffer.Data,
            backend::IndexBufferType::Uint16);

        for (const auto& pcmd : cmd_list->CmdBuffer)
        {
            RenderParams& params = renderParams_[primIdx];

            params.material->setBlendFactor(backend::BlendFactorPresets::Translucent);
            params.material->setViewLayer(0x5);

            uint32_t width = static_cast<uint32_t>(pcmd.ClipRect.z - pcmd.ClipRect.x);
            uint32_t height = static_cast<uint32_t>(pcmd.ClipRect.w - pcmd.ClipRect.y);
            uint32_t offsetX = std::max(static_cast<int32_t>(pcmd.ClipRect.x), 0);
            uint32_t offsetY = std::max(static_cast<int32_t>(pcmd.ClipRect.y), 0);
            params.material->setScissor(width, height, offsetX, offsetY);

            // primitive data
            params.prim->addMeshDrawData(pcmd.ElemCount, pcmd.IdxOffset, 0);
            params.prim->setIndexBuffer(iBuffers_[idx]);
            params.prim->setVertexBuffer(vBuffers_[idx]);

            // font texture
            params.material->addTexture(
                engine_, texture_, Material::ImageType::BaseColour, sampler_);

            // set the push constant
            PushConstant push;
            push.scale = {2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y};
            push.translate = {
                -1.0f - drawData->DisplayPos.x * push.scale.x,
                -1.0f - drawData->DisplayPos.y * push.scale.y};

            params.material->addPushConstantParam(
                "scale",
                backend::BufferElementType::Float2,
                backend::ShaderStage::Vertex,
                &push.scale);
            params.material->addPushConstantParam(
                "translate",
                backend::BufferElementType::Float2,
                backend::ShaderStage::Vertex,
                push.translate);

            params.prim->setMaterial(params.material);
            renderable_->setPrimitive(params.prim, primIdx);

            ++primIdx;
        }
    }

    rendManager->build(renderable_, rendObj_, {}, "ui.glsl");
}

} // namespace yave
