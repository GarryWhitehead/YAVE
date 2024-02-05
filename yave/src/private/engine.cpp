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
#include "indirect_light.h"
#include "managers/component_manager.h"
#include "managers/light_manager.h"
#include "managers/renderable_manager.h"
#include "mapped_texture.h"
#include "post_process.h"
#include "renderable.h"
#include "scene.h"
#include "skybox.h"
#include "vulkan-api/swapchain.h"
#include "wave_generator.h"
#include "yave/renderable.h"

#include <thread>

using namespace std::literals::chrono_literals;

namespace yave
{

IEngine::IEngine()
    : rendManager_(std::make_unique<IRenderableManager>(*this)),
      transformManager_(std::make_unique<ITransformManager>(*this)),
      objManager_(std::make_unique<IObjectManager>()),
      currentSwapchain_(nullptr),
      dummyCubeMap_(nullptr),
      dummyTexture_(nullptr),
      driver_(nullptr)
{
}

IEngine::~IEngine() = default;

IEngine* IEngine::create(vkapi::VkDriver* driver)
{
    // Create and initialise the vulkan backend
    auto* engine = new IEngine();
    engine->driver_ = std::unique_ptr<vkapi::VkDriver>(driver);

    // it's safe to initialise the lighting manager and post process now
    // (requires the device to be init)
    engine->lightManager_ = std::make_unique<ILightManager>(*engine);
    engine->postProcess_ = std::make_unique<PostProcess>(*engine);

    engine->init();

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

void IEngine::init() noexcept
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
        util::ecast(yave::VertexBuffer::BindingType::Position), backend::BufferElementType::Float3);
    quadVertexBuffer_.addAttribute(
        util::ecast(yave::VertexBuffer::BindingType::Uv), backend::BufferElementType::Float2);

    quadVertexBuffer_.build(*driver_, 4, (void*)vertices);

    quadIndexBuffer_.build(
        *driver_,
        static_cast<uint32_t>(indices.size()),
        (void*)indices.data(),
        backend::IndexBufferType::Uint32);

    quadPrimitive_.addMeshDrawData(indices.size(), 0, 0);

    // initialise duumy ibl textures
    dummyCubeMap_ = createMappedTexture();
    dummyTexture_ = createMappedTexture();

    uint32_t zeroBuffer[6] = {0};
    dummyCubeMap_->setTexture(
        zeroBuffer,
        sizeof(zeroBuffer),
        1,
        1,
        1,
        6,
        Texture::TextureFormat::RGBA8,
        backend::ImageUsage::Sampled);
    dummyTexture_->setTexture(
        zeroBuffer,
        sizeof(zeroBuffer),
        1,
        1,
        1,
        1,
        Texture::TextureFormat::RGBA8,
        backend::ImageUsage::Sampled);
}

void IEngine::setCurrentSwapchain(const SwapchainHandle& handle) noexcept
{
    ASSERT_LOG(handle.getKey() < swapchains_.size());
    currentSwapchain_ = swapchains_[handle.getKey()];
}

vkapi::Swapchain* IEngine::getCurrentSwapchain() noexcept { return currentSwapchain_; }

SwapchainHandle IEngine::createSwapchain(const vk::SurfaceKHR& surface, uint32_t width, uint32_t height)
{
    // create a swapchain for surface rendering based on the platform-specific
    // window surface
    auto sc = new vkapi::Swapchain();
    sc->create(driver(), surface, width, height);
    SwapchainHandle handle {static_cast<uint32_t>(swapchains_.size())};
    swapchains_.emplace_back(sc);
    return handle;
}

IRenderer* IEngine::createRenderer() { return createResource(renderers_, this); }

IScene* IEngine::createScene() { return createResource(scenes_, *this); }

IVertexBuffer* IEngine::createVertexBuffer() noexcept { return createResource(vBuffers_); }

IIndexBuffer* IEngine::createIndexBuffer() noexcept { return createResource(iBuffers_); }

IRenderPrimitive* IEngine::createRenderPrimitive() noexcept { return createResource(primitives_); }

IRenderable* IEngine::createRenderable() noexcept { return createResource(renderables_); }

IMappedTexture* IEngine::createMappedTexture() noexcept
{
    return createResource(mappedTextures_, *this);
}

ISkybox* IEngine::createSkybox(IScene& scene) noexcept
{
    return createResource(skyboxes_, *this, scene);
}

IIndirectLight* IEngine::createIndirectLight() noexcept { return createResource(indirectLights_); }

ICamera* IEngine::createCamera() noexcept { return createResource(cameras_); }

IWaveGenerator* IEngine::createWaveGenerator(IScene& scene) noexcept
{
    return createResource(waterGens_, *this, scene);
}

void IEngine::flush()
{
    auto& cmds = driver_->getCommands();
    cmds.flush();
}

void IEngine::destroy(IRenderer* renderer) { destroyResource(renderer, renderers_); }

void IEngine::destroy(IScene* scene) { destroyResource(scene, scenes_); }

void IEngine::destroy(IVertexBuffer* vBuffer) noexcept { destroyResource(vBuffer, vBuffers_); }

void IEngine::destroy(IIndexBuffer* iBuffer) noexcept { destroyResource(iBuffer, iBuffers_); }

void IEngine::destroy(IRenderPrimitive* rPrimitive) noexcept
{
    destroyResource(rPrimitive, primitives_);
}

void IEngine::destroy(IRenderable* renderable) noexcept
{
    destroyResource(renderable, renderables_);
}

void IEngine::destroy(IMappedTexture* texture) noexcept
{
    destroyResource(texture, mappedTextures_);
}

void IEngine::destroy(ISkybox* skybox) noexcept { destroyResource(skybox, skyboxes_); }

void IEngine::destroy(IIndirectLight* idLight) noexcept
{
    destroyResource(idLight, indirectLights_);
}

void IEngine::destroy(ICamera* camera) noexcept { destroyResource(camera, cameras_); }

void IEngine::destroy(IWaveGenerator* wGen) noexcept { destroyResource(wGen, waterGens_); }

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

void IEngine::deleteRenderTarget(const vkapi::RenderTargetHandle& handle)
{
    driver_->deleteRenderTarget(handle);
}

} // namespace yave
