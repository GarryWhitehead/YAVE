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

#include "index_buffer.h"
#include "managers/transform_manager.h"
#include "object_manager.h"
#include "render_primitive.h"
#include "renderer.h"
#include "utility/compiler.h"
#include "utility/handle.h"
#include "vertex_buffer.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/swapchain.h"
#include "yave/camera.h"
#include "yave/engine.h"
#include "yave/light_manager.h"
#include "yave/renderable_manager.h"
#include "yave/wave_generator.h"

#include <deque>
#include <filesystem>
#include <memory>
#include <unordered_set>
#include <vector>

namespace yave
{
class Object;
class IScene;
class Window;
class IRenderable;
class IRenderableManager;
class PostProcess;
class ILightManager;
class IMappedTexture;
class ISkybox;
class IIndirectLight;
class ICamera;
class IWaveGenerator;

using SwapchainHandle = vkapi::SwapchainHandle;

class IEngine : public Engine
{
public:
    IEngine();
    ~IEngine();

    static IEngine* create(Window* window);
    static void destroy(IEngine* engine);

    void shutdown();

    SwapchainHandle createSwapchainI(Window* win);
    IRenderer* createRendererI();
    IScene* createSceneI();

    IVertexBuffer* createVertexBufferI() noexcept;
    IIndexBuffer* createIndexBufferI() noexcept;
    IRenderPrimitive* createRenderPrimitiveI() noexcept;
    IRenderable* createRenderableI() noexcept;
    IMappedTexture* createMappedTextureI() noexcept;
    ISkybox* createSkyboxI(IScene& scene) noexcept;
    IIndirectLight* createIndirectLightI() noexcept;
    ICamera* createCameraI() noexcept;
    IWaveGenerator* createWaveGeneratorI() noexcept;

    void setCurrentSwapchainI(const SwapchainHandle& handle) noexcept;
    vkapi::Swapchain* getCurrentSwapchain() noexcept;

    void init() noexcept;

    void flush();

    // ================= resource handling ===================

    template <typename RESOURCE, typename... ARGS>
    RESOURCE* createResource(std::unordered_set<RESOURCE*>& container, ARGS&&... args)
    {
        RESOURCE* resource = new RESOURCE(std::forward<ARGS>(args)...);
        if (resource)
        {
            container.insert(resource);
        }
        return resource;
    }

    template <typename RESOURCE>
    void destroyResource(RESOURCE* resource, std::unordered_set<RESOURCE*>& container);

    void deleteRenderTargetI(const vkapi::RenderTargetHandle& handle);

    // ==================== getters =======================

    vkapi::VkDriver& driver() noexcept { return *driver_; }
    const vkapi::VkDriver& driver() const noexcept { return *driver_; }

    IRenderableManager* getRenderableManagerI() noexcept { return rendManager_.get(); }
    ITransformManager* getTransformManagerI() noexcept { return transformManager_.get(); }
    ILightManager* getLightManagerI() noexcept { return lightManager_.get(); }
    IObjectManager* getObjManagerI() noexcept { return objManager_.get(); }
    PostProcess* getPostProcess() noexcept { return postProcess_.get(); }

    auto getQuadBuffers() noexcept { return std::make_pair(&quadVertexBuffer_, &quadIndexBuffer_); }
    IRenderPrimitive* getQuadPrimitive() noexcept { return &quadPrimitive_; }

    IMappedTexture* getDummyIrradianceMap() noexcept { return dummyIrradianceMap_; }
    IMappedTexture* getDummySpecularMap() noexcept { return dummySpecularMap_; }
    IMappedTexture* getDummyBrdfLut() noexcept { return dummyBrdfLut_; }

    // ==================== client api ========================

    Scene* createScene() override;
    vkapi::SwapchainHandle createSwapchain(Window* win) override;
    Renderer* createRenderer() override;
    VertexBuffer* createVertexBuffer() override;
    IndexBuffer* createIndexBuffer() override;
    RenderPrimitive* createRenderPrimitive() override;
    Renderable* createRenderable() override;
    void setCurrentSwapchain(const SwapchainHandle& handle) override;
    RenderableManager* getRenderManager() override;
    TransformManager* getTransformManager() override;
    LightManager* getLightManager() override;
    ObjectManager* getObjectManager() override;
    Texture* createTexture() override;
    Skybox* createSkybox(Scene* scene) override;
    IndirectLight* createIndirectLight() override;
    Camera* createCamera() override;
    WaveGenerator* createWaveGenerator() override;
    void flushCmds() override;

    void destroy(VertexBuffer* buffer) override;
    void destroy(IndexBuffer* buffer) override;
    void destroy(RenderPrimitive* buffer) override;
    void destroy(Renderable* buffer) override;
    void destroy(Scene* buffer) override;
    void destroy(Camera* buffer) override;
    void destroy(Renderer* renderer) override;

    void deleteRenderTarget(const vkapi::RenderTargetHandle& handle);

private:
    Window* currentWindow_;

    std::unique_ptr<IRenderableManager> rendManager_;
    std::unique_ptr<ITransformManager> transformManager_;
    std::unique_ptr<ILightManager> lightManager_;
    std::unique_ptr<IObjectManager> objManager_;
    std::unique_ptr<PostProcess> postProcess_;

    std::unordered_set<IVertexBuffer*> vBuffers_;
    std::unordered_set<IIndexBuffer*> iBuffers_;
    std::unordered_set<IRenderPrimitive*> primitives_;
    std::unordered_set<IScene*> scenes_;
    std::unordered_set<IRenderer*> renderers_;
    std::unordered_set<IRenderable*> renderables_;
    std::unordered_set<IMappedTexture*> mappedTextures_;
    std::unordered_set<ISkybox*> skyboxes_;
    std::unordered_set<IIndirectLight*> indirectLights_;
    std::unordered_set<ICamera*> cameras_;
    std::unordered_set<IWaveGenerator*> waterGens_;
    std::vector<vkapi::Swapchain*> swapchains_;

    vkapi::Swapchain* currentSwapchain_;

    // default quad verices/indices buffers
    IVertexBuffer quadVertexBuffer_;
    IIndexBuffer quadIndexBuffer_;
    IRenderPrimitive quadPrimitive_;

    // dummy textures
    IMappedTexture* dummyIrradianceMap_;
    IMappedTexture* dummySpecularMap_;
    IMappedTexture* dummyBrdfLut_;

    // =========== vk backend ============================

    std::unique_ptr<vkapi::VkDriver> driver_;
};

} // namespace yave
