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

#include "engine.h"

#include "camera.h"
#include "managers/component_manager.h"
#include "managers/light_manager.h"
#include "managers/renderable_manager.h"
#include "mapped_texture.h"
#include "renderable.h"
#include "scene.h"
#include "skybox.h"
#include "utility/timer.h"
#include "vulkan-api/swapchain.h"
#include "yave/renderable.h"

#include <yave_app/window.h>

#include <thread>

using namespace std::literals::chrono_literals;

namespace yave
{

IEngine::IEngine()
    : currentWindow_(nullptr),
      rendManager_(std::make_unique<IRenderableManager>(*this)),
      transformManager_(std::make_unique<ITransformManager>(*this)),
      currentScene_(nullptr),
      currentSwapchain_(nullptr),
      driver_(nullptr)
{
}

IEngine::~IEngine() = default;

IEngine* IEngine::create(Window* win)
{
    // Create and initialise the vulkan backend
    vkapi::VkDriver* driver = new vkapi::VkDriver;
    ASSERT_LOG(win);
    const auto& [ext, count] = win->getInstanceExt();
    driver->createInstance(ext, count);

    // Create the window surface
    win->createSurfaceVk(driver->context().instance());

    // Create the abstract physical device object
    driver->init(win->getSurface());

    IEngine* engine = new IEngine();
    engine->currentWindow_ = win;
    engine->driver_ = std::unique_ptr<vkapi::VkDriver>(driver);

    // its safe to initialise the lighting manager now
    // (requires the device to be init)
    engine->lightManager_ = std::make_unique<ILightManager>(*engine);

    engine->createDefaultQuadBuffers();

    return engine;
}

void IEngine::destroy(IEngine* engine)
{
    if (engine)
    {
        engine->shutdown();
        delete engine;
    }
}

void IEngine::shutdown() { driver_->shutdown(); }

void IEngine::createDefaultQuadBuffers() noexcept
{
    // clang-format off
    float vertices[] = {
         1.0f, 1.0f, 0.0f,    1.0f, 1.0f,  
        -1.0f, 1.0f, 0.0f,    0.0f, 1.0f,  
        -1.0f, -1.0f, 0.0,    0.0f, 0.0f, 
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f};
    // clang-format on

    const std::vector<int> indices = {0, 1, 2, 2, 3, 0};

    quadVertexBuffer_.addAttribute(
        yave::VertexBuffer::BindingType::Position, backend::BufferElementType::Float3);
    quadVertexBuffer_.addAttribute(
        yave::VertexBuffer::BindingType::Uv, backend::BufferElementType::Float2);

    quadVertexBuffer_.buildI(*driver_, 4, (void*)vertices);

    quadIndexBuffer_.buildI(
        *driver_,
        static_cast<uint32_t>(indices.size()),
        (void*)indices.data(),
        backend::IndexBufferType::Uint32);

    quadPrimitive_.addMeshDrawData(indices.size(), 0);
}

void IEngine::setCurrentSwapchainI(const SwapchainHandle& handle) noexcept
{
    ASSERT_LOG(handle.getKey() < swapchains_.size());
    currentSwapchain_ = swapchains_[handle.getKey()];
}

vkapi::Swapchain* IEngine::getCurrentSwapchain() noexcept
{
    ASSERT_FATAL(
        currentSwapchain_,
        "No swapchain has been registered with the engine via "
        "setCurrentSwapchain.");
    return currentSwapchain_;
}

SwapchainHandle IEngine::createSwapchainI(Window* win)
{
    // create a swapchain for surface rendering based on the platform specific
    // window surface
    auto sc = new vkapi::Swapchain();
    sc->create(driver(), win->getSurface(), win->width(), win->height());
    SwapchainHandle handle {static_cast<uint32_t>(swapchains_.size())};
    swapchains_.emplace_back(sc);
    return handle;
}

IRenderer* IEngine::createRendererI(const SwapchainHandle& handle, IScene& scene)
{
    return createResource(renderers_, this);
}

IScene* IEngine::createSceneI() { return createResource(scenes_, *this); }

IVertexBuffer* IEngine::createVertexBufferI() noexcept { return createResource(vBuffers_); }

IIndexBuffer* IEngine::createIndexBufferI() noexcept { return createResource(iBuffers_); }

IRenderPrimitive* IEngine::createRenderPrimitiveI() noexcept { return createResource(primitives_); }

IRenderable* IEngine::createRenderableI() noexcept { return createResource(renderables_); }

IMappedTexture* IEngine::createMappedTextureI() noexcept
{
    return createResource(mappedTextures_, *this);
}

ISkybox* IEngine::createSkyboxI() noexcept { return createResource(skyboxes_, *this); }

ICamera* IEngine::createCameraI() noexcept { return createResource(cameras_); }

template <typename RESOURCE>
void IEngine::destroyResource(RESOURCE* buffer, std::unordered_set<RESOURCE*>& container)
{
    ASSERT_LOG(buffer);

    // we silently fail if the buffer is not found in the resource list.
    if (container.find(buffer) == container.end())
    {
        return;
    }
    buffer->shutDown(*driver_);
    container.erase(buffer);
    delete buffer;
}

template void IEngine::destroyResource<IVertexBuffer>(
    IVertexBuffer*, std::unordered_set<IVertexBuffer*>& container);

template void
IEngine::destroyResource<IIndexBuffer>(IIndexBuffer*, std::unordered_set<IIndexBuffer*>& container);

template void IEngine::destroyResource<IRenderPrimitive>(
    IRenderPrimitive*, std::unordered_set<IRenderPrimitive*>& container);

template void
IEngine::destroyResource<IRenderable>(IRenderable*, std::unordered_set<IRenderable*>& container);

template void IEngine::destroyResource<IScene>(IScene*, std::unordered_set<IScene*>& container);

template void IEngine::destroyResource<ICamera>(ICamera*, std::unordered_set<ICamera*>& container);

IObject* IEngine::createObjectI()
{
    uint64_t id = 0;
    if (!freeIds_.empty() && freeIds_.size() > MinimumFreeIds)
    {
        id = freeIds_.front();
        freeIds_.pop_front();
    }
    else
    {
        id = nextId_++;
    }

    auto object = std::make_unique<IObject>(id);
    objects_.emplace_back(std::move(object));
    return objects_.back().get();
}

void IEngine::destroyObject(IObject* obj)
{
    size_t count = 0;
    for (auto& object : objects_)
    {
        if (*obj == *object)
        {
            break;
        }
        ++count;
    }
    // completley remove from the list - costly!
    objects_.erase(objects_.begin() + count);
    freeIds_.push_front(obj->id());
}

// ==================== client api ========================

Engine::Engine() {}
Engine::~Engine() {}

Engine* Engine::create(Window* win) { return reinterpret_cast<Engine*>(IEngine::create(win)); }

void Engine::destroy(Engine* engine) { IEngine::destroy(reinterpret_cast<IEngine*>(engine)); }

Scene* IEngine::createScene() { return reinterpret_cast<Scene*>(createSceneI()); }

vkapi::SwapchainHandle IEngine::createSwapchain(Window* win) { return createSwapchainI(win); }

Renderer* IEngine::createRenderer(const vkapi::SwapchainHandle& handle, Scene* scene)
{
    return reinterpret_cast<Renderer*>(
        createRendererI(handle, *(reinterpret_cast<IScene*>(scene))));
}

VertexBuffer* IEngine::createVertexBuffer()
{
    return reinterpret_cast<VertexBuffer*>(createVertexBufferI());
}

IndexBuffer* IEngine::createIndexBuffer()
{
    return reinterpret_cast<IndexBuffer*>(createIndexBufferI());
}

RenderPrimitive* IEngine::createRenderPrimitive()
{
    return reinterpret_cast<RenderPrimitive*>(createRenderPrimitiveI());
}

Renderable* IEngine::createRenderable()
{
    return reinterpret_cast<Renderable*>(createRenderableI());
}

void IEngine::setCurrentSwapchain(const SwapchainHandle& handle) { setCurrentSwapchainI(handle); }

void IEngine::setCurrentScene(Scene* scene) { setCurrentSceneI(reinterpret_cast<IScene*>(scene)); }

RenderableManager* IEngine::getRenderManager()
{
    return reinterpret_cast<RenderableManager*>(getRenderableManagerI());
}

TransformManager* IEngine::getTransformManager()
{
    return reinterpret_cast<TransformManager*>(getTransformManagerI());
}

LightManager* IEngine::getLightManager()
{
    return reinterpret_cast<LightManager*>(getLightManagerI());
}

Object* IEngine::createObject() { return reinterpret_cast<Object*>(createObjectI()); }

Texture* IEngine::createTexture() { return reinterpret_cast<Texture*>(createMappedTextureI()); }

Skybox* IEngine::createSkybox() { return reinterpret_cast<Skybox*>(createSkyboxI()); }

Camera* IEngine::createCamera() { return reinterpret_cast<Camera*>(createCameraI()); }

void IEngine::destroy(VertexBuffer* buffer)
{
    destroyResource(reinterpret_cast<IVertexBuffer*>(buffer), vBuffers_);
};

void IEngine::destroy(IndexBuffer* buffer)
{
    destroyResource(reinterpret_cast<IIndexBuffer*>(buffer), iBuffers_);
}

void IEngine::destroy(RenderPrimitive* buffer)
{
    destroyResource(reinterpret_cast<IRenderPrimitive*>(buffer), primitives_);
}

void IEngine::destroy(Renderable* buffer)
{
    destroyResource(reinterpret_cast<IRenderable*>(buffer), renderables_);
}

void IEngine::destroy(Scene* buffer)
{
    destroyResource(reinterpret_cast<IScene*>(buffer), scenes_);
}

void IEngine::destroy(Camera* buffer)
{
    destroyResource(reinterpret_cast<ICamera*>(buffer), cameras_);
}


} // namespace yave
