
#include "private/engine.h"

#include "private/camera.h"
#include "private/index_buffer.h"
#include "private/indirect_light.h"
#include "private/managers/renderable_manager.h"
#include "private/render_primitive.h"
#include "private/renderable.h"
#include "private/scene.h"
#include "private/vertex_buffer.h"
#include "private/wave_generator.h"

namespace yave
{
// ==================== client api ========================

Engine* Engine::create(Window* win) { return IEngine::create(win); }

void Engine::destroy(Engine* engine) { IEngine::destroy(static_cast<IEngine*>(engine)); }

Scene* Engine::createScene() { return static_cast<IEngine*>(this)->createScene(); }

vkapi::SwapchainHandle Engine::createSwapchain(Window* win)
{
    return static_cast<IEngine*>(this)->createSwapchain(win);
}

Renderer* Engine::createRenderer() { return static_cast<IEngine*>(this)->createRenderer(); }

VertexBuffer* Engine::createVertexBuffer()
{
    return static_cast<IEngine*>(this)->createVertexBuffer();
}

IndexBuffer* Engine::createIndexBuffer()
{
    return static_cast<IEngine*>(this)->createIndexBuffer();
}

RenderPrimitive* Engine::createRenderPrimitive()
{
    return static_cast<IEngine*>(this)->createRenderPrimitive();
}

Renderable* Engine::createRenderable() { return static_cast<IEngine*>(this)->createRenderable(); }

void Engine::setCurrentSwapchain(const SwapchainHandle& handle)
{
    static_cast<IEngine*>(this)->setCurrentSwapchain(handle);
}

RenderableManager* Engine::getRenderManager()
{
    return static_cast<IEngine*>(this)->getRenderableManager();
}

TransformManager* Engine::getTransformManager()
{
    return static_cast<IEngine*>(this)->getTransformManager();
}

LightManager* Engine::getLightManager() { return static_cast<IEngine*>(this)->getLightManager(); }

ObjectManager* Engine::getObjectManager() { return static_cast<IEngine*>(this)->getObjManager(); }

Texture* Engine::createTexture() { return static_cast<IEngine*>(this)->createMappedTexture(); }

Skybox* Engine::createSkybox(Scene* scene)
{
    return static_cast<IEngine*>(this)->createSkybox(*static_cast<IScene*>(scene));
}

IndirectLight* Engine::createIndirectLight()
{
    return static_cast<IEngine*>(this)->createIndirectLight();
}

Camera* Engine::createCamera() { return static_cast<IEngine*>(this)->createCamera(); }

WaveGenerator* Engine::createWaveGenerator(Scene* scene)
{
    return static_cast<IEngine*>(this)->createWaveGenerator(*(static_cast<IScene*>(scene)));
}

void Engine::flushCmds() { static_cast<IEngine*>(this)->flush(); }

void Engine::destroy(VertexBuffer* buffer)
{
    static_cast<IEngine*>(this)->destroy(static_cast<IVertexBuffer*>(buffer));
};

void Engine::destroy(IndexBuffer* buffer)
{
    static_cast<IEngine*>(this)->destroy(static_cast<IIndexBuffer*>(buffer));
}

void Engine::destroy(RenderPrimitive* buffer)
{
    static_cast<IEngine*>(this)->destroy(static_cast<IRenderPrimitive*>(buffer));
}

void Engine::destroy(Renderable* buffer)
{
    static_cast<IEngine*>(this)->destroy(static_cast<IRenderable*>(buffer));
}

void Engine::destroy(Scene* buffer)
{
    static_cast<IEngine*>(this)->destroy(static_cast<IScene*>(buffer));
}

void Engine::destroy(Camera* buffer)
{
    static_cast<IEngine*>(this)->destroy(static_cast<ICamera*>(buffer));
}

void Engine::destroy(Renderer* renderer)
{
    static_cast<IEngine*>(this)->destroy(static_cast<IRenderer*>(renderer));
}

void Engine::deleteRenderTarget(const vkapi::RenderTargetHandle& handle)
{
    static_cast<IEngine*>(this)->deleteRenderTarget(handle);
}

} // namespace yave