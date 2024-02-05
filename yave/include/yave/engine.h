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

#include "vulkan-api/driver.h"
#include "vulkan-api/swapchain.h"
#include "yave_api.h"

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
class ObjectManager;
class TransformManager;
class LightManager;
class Texture;
class Skybox;
class IndirectLight;
class Camera;
class Window;
class WaveGenerator;

class Engine : public YaveApi
{
public:
    static Engine* create(vkapi::VkDriver* driver);
    static void destroy(Engine* engine);

    Scene* createScene();

    vkapi::SwapchainHandle
    createSwapchain(const vk::SurfaceKHR& surface, uint32_t width, uint32_t height);

    Renderer* createRenderer();

    VertexBuffer* createVertexBuffer();

    IndexBuffer* createIndexBuffer();

    RenderPrimitive* createRenderPrimitive();

    Renderable* createRenderable();

    void setCurrentSwapchain(const vkapi::SwapchainHandle& handle);

    RenderableManager* getRenderManager();

    TransformManager* getTransformManager();

    LightManager* getLightManager();

    ObjectManager* getObjectManager();

    Texture* createTexture();

    Skybox* createSkybox(Scene* scene);

    IndirectLight* createIndirectLight();

    Camera* createCamera();

    WaveGenerator* createWaveGenerator(Scene* scene);

    void flushCmds();

    void destroy(IndexBuffer* buffer);
    void destroy(VertexBuffer* buffer);
    void destroy(RenderPrimitive* buffer);
    void destroy(Renderable* buffer);
    void destroy(Scene* buffer);
    void destroy(Camera* buffer);
    void destroy(Renderer* renderer);

    void deleteRenderTarget(const vkapi::RenderTargetHandle& handle);

protected:
    ~Engine() = default;
};

} // namespace yave
