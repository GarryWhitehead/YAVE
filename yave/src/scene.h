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
#include "skybox.h"
#include "vulkan-api/buffer.h"
#include "yave/scene.h"

#include <mathfu/glsl_mappings.h>

#include <deque>
#include <memory>
#include <vector>

namespace yave
{
// forward declerations
class IObject;
class IRenderable;
struct TransformInfo;
class ICamera;
class Frustum;
class IEngine;
class ISkybox;

class IScene : public Scene
{
public:

    static constexpr int ModelBufferInitialSize = 20;

    /**
     * @brief A temp struct used to gather viable renderable object data ready
     * for visibilty checks and passing to the render queue
     */
    struct VisibleCandidate
    {
        IRenderable* renderable;
        TransformInfo* transform;
        AABBox worldAABB;
        mathfu::mat4 worldTransform;
    };

    IScene(IEngine& engine);
    ~IScene();

    bool update();

    void shutDown(vkapi::VkDriver& driver) noexcept;

    VisibleCandidate
    buildRendCandidate(IObject* obj, const mathfu::mat4& worldMatrix);

    void getVisibleRenderables(
        Frustum& frustum, std::vector<IScene::VisibleCandidate>& renderables);

    void getVisibleLights(
        Frustum& frustum,
        std::vector<LightInstance*>& candLightObjs);

    void updateTransformBuffer(
        const std::vector<IScene::VisibleCandidate>& candObjects,
        const size_t staticModelCount,
        const size_t skinnedModelCount);

    void updateCameraBuffer();

    void setSkyboxI(ISkybox* skybox) noexcept;

    void setCameraI(ICamera* cam) noexcept;

    // ============== getters ============================

    ISkybox* getSkybox() noexcept { return skybox_; }
    ICamera& getCurrentCameraI() noexcept { return *camera_; }
    RenderQueue& getRenderQueue() noexcept { return renderQueue_; }
    UniformBuffer& getTransUbo() noexcept { return *transUbo_; }
    UniformBuffer& getSkinUbo() noexcept { return *skinUbo_; }

    // ================== client api =======================

    void setSkybox(Skybox* skybox) override;
    void setCamera(Camera* cam) override;
    Camera* getCurrentCamera() override;

private:
    IEngine& engine_;

    /// Current camera used by this scene.
    ICamera* camera_;

    /// current skybox
    ISkybox* skybox_;

    std::vector<VisibleCandidate> candRenderableObjs_;

    RenderQueue renderQueue_;

    std::unique_ptr<UniformBuffer> transUbo_;
    std::unique_ptr<UniformBuffer> skinUbo_;
};


} // namespace yave
