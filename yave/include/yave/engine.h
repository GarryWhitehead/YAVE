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

#include "vulkan-api/swapchain.h"

#include <filesystem>
#include <memory>

namespace yave
{
class Window;
class Scene;
class Renderer;
class VertexBuffer;
class IndexBuffer;
class RenderPrimitive;
class Renderable;
class RenderableManager;
class Object;
class TransformManager;
class LightManager;
class Texture;
class Skybox;
class Camera;
class Window;

class Engine
{
public:
    virtual ~Engine();

    static Engine* create(Window* win);
    static void destroy(Engine* engine);

    virtual Scene* createScene() = 0;

    virtual vkapi::SwapchainHandle createSwapchain(Window* win) = 0;

    virtual Renderer* createRenderer() = 0;

    virtual VertexBuffer* createVertexBuffer() = 0;

    virtual IndexBuffer* createIndexBuffer() = 0;

    virtual RenderPrimitive* createRenderPrimitive() = 0;

    virtual Renderable* createRenderable() = 0;

    virtual void setCurrentSwapchain(const vkapi::SwapchainHandle& handle) = 0;

    virtual void setCurrentScene(Scene* scene) = 0;

    virtual RenderableManager* getRenderManager() = 0;

    virtual TransformManager* getTransformManager() = 0;

    virtual LightManager* getLightManager() = 0;

    virtual Texture* createTexture() = 0;

    virtual Skybox* createSkybox() = 0;

    virtual Camera* createCamera() = 0;

    virtual void destroy(IndexBuffer* buffer) = 0;
    virtual void destroy(VertexBuffer* buffer) = 0;
    virtual void destroy(RenderPrimitive* buffer) = 0;
    virtual void destroy(Renderable* buffer) = 0;
    virtual void destroy(Scene* buffer) = 0;
    virtual void destroy(Camera* buffer) = 0;

protected:
    Engine();
};

} // namespace yave
