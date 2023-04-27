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

#include <mathfu/glsl_mappings.h>

namespace yave
{
class Engine;
class Texture;
class Scene;
class Renderer;
class Camera;
class VertexBuffer;
class IndexBuffer;
class RenderPrimitive;
class Renderable;

class PreFilter
{
public:

    struct Options
    {
        float sampleCount = 32.0f;
    };

    PreFilter(Engine* engine, const Options& options);
    ~PreFilter();

    Texture* eqirectToCubemap(Texture* image);

    Texture* createIrradianceEnvMap(Texture* cubeMap);

    Texture* createSpecularEnvMap(Texture* cubeMap);

private:

    Engine* engine_;

    Scene* scene_;
    Renderer* renderer_;
    Camera* camera_;
    RenderPrimitive* prim_;
    Renderable* render_;

    VertexBuffer* vBuffer;
    IndexBuffer* iBuffer;

    const Options& options_;
};

} // namespace yave