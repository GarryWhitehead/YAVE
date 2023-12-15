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

#include "aabox.h"
#include "managers/light_manager.h"
#include "render_queue.h"
#include "scene_ubo.h"
#include "skybox.h"
#include "yave/scene.h"

#include <mathfu/glsl_mappings.h>
#include <vulkan-api/buffer.h>

#include <deque>
#include <memory>
#include <vector>

namespace yave
{
// forward declarations
class Object;
class IRenderable;
struct TransformInfo;
class ICamera;
class Frustum;
class IEngine;
class ISkybox;
class IWaveGenerator;
class WaveGenerator;

class IScene : public Scene
{
public:
    static constexpr int ModelBufferInitialSize = 20;

    /**
     * @brief A temp struct used to gather viable renderable object data ready
     * for visibility checks and passing to the render queue
     */
    struct VisibleCandidate
    {
        IRenderable* renderable;
        TransformInfo* transform;
        AABBox worldAABB;
        mathfu::mat4 worldTransform;
    };

    explicit IScene(IEngine& engine);
    ~IScene();

    bool update();

    void shutDown(vkapi::VkDriver& driver) noexcept;

    VisibleCandidate buildRendCandidate(Object& obj, const mathfu::mat4& worldMatrix);

    static void
    getVisibleRenderables(Frustum& frustum, std::vector<IScene::VisibleCandidate>& renderables);

    static void getVisibleLights(Frustum& frustum, std::vector<LightInstance*>& candLightObjs);

    void updateTransformBuffer(
        const std::vector<IScene::VisibleCandidate>& candObjects,
        size_t staticModelCount,
        size_t skinnedModelCount);

    void setSkybox(ISkybox* skybox) noexcept;
    void setIndirectLight(IIndirectLight* il);
    void setBloomOptions(const BloomOptions& bloom);
    void setGbufferOptions(const GbufferOptions& gb);
    void setCamera(ICamera* cam) noexcept;
    void setWaveGenerator(IWaveGenerator* waterGen) noexcept;

    void addObject(Object obj);

    void destroyObject(Object obj);

    void usePostProcessing(bool state);
    void useGbuffer(bool state);

    // ============== getters ============================

    [[maybe_unused]] ISkybox* getSkybox() noexcept { return skybox_; }
    IIndirectLight* getIndirectLight() noexcept { return indirectLight_; }
    ICamera* getCurrentCamera() noexcept { return camera_; }
    RenderQueue& getRenderQueue() noexcept { return renderQueue_; }
    UniformBuffer& getTransUbo() noexcept { return *transUbo_; }
    [[maybe_unused]] UniformBuffer& getSkinUbo() noexcept { return *skinUbo_; }
    SceneUbo& getSceneUbo() noexcept { return *sceneUbo_; }
    IWaveGenerator* getWaveGenerator() noexcept { return waveGen_; }
    [[nodiscard]] bool withPostProcessing() const noexcept { return usePostProcessing_; }
    [[nodiscard]] bool withGbuffer() const noexcept { return useGbuffer_; }
    BloomOptions& getBloomOptions();
    GbufferOptions& getGbufferOptions();

private:
    IEngine& engine_;

    // Current camera used by this scene.
    ICamera* camera_;

    // current skybox
    ISkybox* skybox_;
    IWaveGenerator* waveGen_;

    IIndirectLight* indirectLight_;

    std::vector<VisibleCandidate> candRenderableObjs_;

    RenderQueue renderQueue_;

    std::unique_ptr<UniformBuffer> transUbo_;
    std::unique_ptr<UniformBuffer> skinUbo_;

    std::unique_ptr<SceneUbo> sceneUbo_;

    // the complete list of all objects associated with all registered scenes
    // using a vector here dor iteration purposes but not great for erasing
    // objects - find a more performant alternative?
    std::vector<Object> objects_;

    // options
    BloomOptions bloomOptions_;
    GbufferOptions gbufferOptions_;

    bool usePostProcessing_;
    bool useGbuffer_;
};


} // namespace yave
