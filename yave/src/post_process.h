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

#include "backend/enums.h"
#include "render_graph/render_graph.h"
#include "yave/object.h"

#include <vulkan-api/driver.h>

#include <memory>
#include <string>

namespace yave
{
class IEngine;
class IMaterial;
class Object;
class IMappedTexture;
class Compute;
struct BloomOptions;
class IScene;

class PostProcess
{
private:
    using ElementType = backend::BufferElementType;

    struct PpRegister
    {
        struct UboParams
        {
            std::string name;
            ElementType type;
            size_t arrayCount = 1;
        };
        std::string shader;
        std::vector<UboParams> uboElements;
        std::vector<std::string> samplers;
    };

    struct PpMaterial
    {
        IMaterial* material = nullptr;
        Object obj;
    };

public:
    // bloom
    struct BloomData
    {
        float gamma = 2.2f;
        rg::RenderGraphHandle bloom;
        rg::RenderGraphHandle rt;
        // inputs
        rg::RenderGraphHandle light;
    };

    PostProcess(IEngine& engine);
    ~PostProcess();

    void init(IScene& scene);

    PpMaterial getMaterial(const std::string& name);

    rg::RenderGraphHandle bloom(
        rg::RenderGraph& rGraph,
        uint32_t width,
        uint32_t height,
        const BloomOptions& options,
        float dt);

private:
    IEngine& engine_;

    std::unordered_map<std::string, PpMaterial> materials_;

    IMappedTexture* averageLumLut_;
    std::unique_ptr<Compute> lumCompute_;
    std::unique_ptr<Compute> avgCompute_;
};

} // namespace yave