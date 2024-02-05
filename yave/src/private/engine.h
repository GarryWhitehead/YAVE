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

    // Note: the engine takes ownership of the Vulkan driver
    static IEngine* create(vkapi::VkDriver* driver);
    static void destroy(IEngine* engine);

    void shutdown();

    SwapchainHandle createSwapchain(const vk::SurfaceKHR& surface, uint32_t width, uint32_t height);
    IRenderer* createRenderer();
    IScene* createScene();

    IVertexBuffer* createVertexBuffer() noexcept;
    IIndexBuffer* createIndexBuffer() noexcept;
    IRenderPrimitive* createRenderPrimitive() noexcept;
    IRenderable* createRenderable() noexcept;
    IMappedTexture* createMappedTexture() noexcept;
    ISkybox* createSkybox(IScene& scene) noexcept;
    IIndirectLight* createIndirectLight() noexcept;
    ICamera* createCamera() noexcept;
    IWaveGenerator* createWaveGenerator(IScene& scene) noexcept;

    void destroy(IRenderer* renderer);
    void destroy(IScene* scene);
    void destroy(IVertexBuffer* vBuffer) noexcept;
    void destroy(IIndexBuffer* iBuffer) noexcept;
    void destroy(IRenderPrimitive* rPrimitive) noexcept;
    void destroy(IRenderable* renderable) noexcept;
    void destroy(IMappedTexture* texture) noexcept;
    void destroy(ISkybox* skybox) noexcept;
    void destroy(IIndirectLight* idLight) noexcept;
    void destroy(ICamera* camera) noexcept;
    void destroy(IWaveGenerator* wGen) noexcept;

    void setCurrentSwapchain(const SwapchainHandle& handle) noexcept;
    vkapi::Swapchain* getCurrentSwapchain() noexcept;

    void init() noexcept;

    void flush();

    // ================= resource handling ===================

    template <typename RESOURCE, typename... ARGS>
    RESOURCE* createResource(std::unordered_set<RESOURCE*>& container, ARGS&&... args)
    {
        auto* resource = new RESOURCE(std::forward<ARGS>(args)...);
        if (resource)
        {
            container.insert(resource);
        }
        return resource;
    }

    template <typename RESOURCE>
    void destroyResource(RESOURCE* resource, std::unordered_set<RESOURCE*>& container);

    void deleteRenderTarget(const vkapi::RenderTargetHandle& handle);

    // ==================== getters =======================

    vkapi::VkDriver& driver() noexcept { return *driver_; }
    const vkapi::VkDriver& driver() const noexcept { return *driver_; }

    IRenderableManager* getRenderableManager() noexcept { return rendManager_.get(); }
    ITransformManager* getTransformManager() noexcept { return transformManager_.get(); }
    ILightManager* getLightManager() noexcept { return lightManager_.get(); }
    IObjectManager* getObjManager() noexcept { return objManager_.get(); }
    PostProcess* getPostProcess() noexcept { return postProcess_.get(); }

    [[maybe_unused]] auto getQuadBuffers() noexcept
    {
        return std::make_pair(&quadVertexBuffer_, &quadIndexBuffer_);
    }
    [[maybe_unused]] IRenderPrimitive* getQuadPrimitive() noexcept { return &quadPrimitive_; }

    IMappedTexture* getDummyCubeMap() noexcept { return dummyCubeMap_; }
    IMappedTexture* getDummyTexture() noexcept { return dummyTexture_; }

private:

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
    IMappedTexture* dummyCubeMap_;
    IMappedTexture* dummyTexture_;

    // =========== vk backend ============================

    std::unique_ptr<vkapi::VkDriver> driver_;
};

} // namespace yave
