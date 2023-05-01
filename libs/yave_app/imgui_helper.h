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

#include "yave/object.h"

#include <imgui.h>
#include <mathfu/glsl_mappings.h>
#include <yave/texture_sampler.h>

#include <filesystem>
#include <vector>


namespace yave
{
class Engine;
class VertexBuffer;
class IndexBuffer;
class Texture;
class Application;
class Material;
class Renderable;
class RenderPrimitive;
class Scene;

class ImGuiHelper
{
public:
    ImGuiHelper(Engine* engine, Scene* scene, std::filesystem::path& fontPath);
    ~ImGuiHelper();

    void beginFrame(Application& app) noexcept;

    void createBuffers(size_t reqBufferCount);

    void updateDrawCommands(ImDrawData* drawData, const ImGuiIO& io);

    void setDisplaySize(float winWidth, float winHeight, float scaleX, float scaleY);

private:
    struct PushConstant
    {
        mathfu::vec2 scale;
        mathfu::vec2 translate;
    };

    ImGuiContext* context_;

    Engine* engine_;

    Object rendObj_;
    Renderable* renderable_;

    struct RenderParams
    {
        Material* material;
        RenderPrimitive* prim;
    };

    std::vector<RenderParams> renderParams_;
    std::vector<VertexBuffer*> vBuffers_;
    std::vector<IndexBuffer*> iBuffers_;

    Texture* texture_;
    TextureSampler sampler_;
};

} // namespace yave