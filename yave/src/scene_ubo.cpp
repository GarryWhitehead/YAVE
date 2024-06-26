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

#include "scene_ubo.h"

#include "camera.h"
#include "engine.h"
#include "indirect_light.h"
#include "managers/light_manager.h"

#include <mathfu/glsl_mappings.h>

namespace yave
{

SceneUbo::SceneUbo(vkapi::VkDriver& driver)
{
    ubo_ = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboSetValue, SceneUboBindPoint, "SceneUbo", "scene_ubo");

    // ================= camera elements =============================

    ubo_->addElement("mvp", backend::BufferElementType::Mat4);
    ubo_->addElement("project", backend::BufferElementType::Mat4);
    ubo_->addElement("model", backend::BufferElementType::Mat4);
    ubo_->addElement("view", backend::BufferElementType::Mat4);
    ubo_->addElement("position", backend::BufferElementType::Float3);
    ubo_->addElement("zNear", backend::BufferElementType::Float);
    ubo_->addElement("zFar", backend::BufferElementType::Float);

    // ================ indirect lighting elements ===================

    ubo_->addElement("iblMipLevels", backend::BufferElementType::Int);

    // =============== directional light =============================

    ubo_->addElement("padding0", backend::BufferElementType::Float2);
    ubo_->addElement("lightColourIntensity", backend::BufferElementType::Float4);
    ubo_->addElement("lightDirection", backend::BufferElementType::Float4);
    ubo_->addElement("sun", backend::BufferElementType::Float4);

    ubo_->createGpuBuffer(driver);
}

void SceneUbo::updateCamera(ICamera& camera)
{
    mathfu::mat4 proj = camera.projMatrix();
    mathfu::mat4 view = camera.viewMatrix();
    mathfu::mat4 model = camera.modelMatrix();
    mathfu::vec3 pos = camera.position();
    float n = camera.getNear();
    float f = camera.getFar();

    mathfu::mat4 vp = proj * view;

    ubo_->updateElement("mvp", &vp);
    ubo_->updateElement("project", &proj);
    ubo_->updateElement("view", &view);
    ubo_->updateElement("model", &model);
    ubo_->updateElement("position", &pos);
    ubo_->updateElement("zNear", &n);
    ubo_->updateElement("zFar", &f);
}

void SceneUbo::updateIbl(IIndirectLight* il)
{
    if (!il)
    {
        return;
    }
    uint32_t mips = il->getMipLevels();
    ubo_->updateElement("iblMipLevels", &mips);
}

void SceneUbo::updateDirLight(IEngine& engine, LightInstance* instance)
{
    auto* lm = engine.getLightManager();

    mathfu::vec4 sun {0.0f, 0.0f, 0.0f, 0.0f};
    mathfu::vec4 dir {mathfu::normalize(-instance->target), 1.0f};
    float exposure = 0.1f;

    if (instance)
    {
        mathfu::vec4 lightIntensity {instance->colour, instance->intensity * exposure};
        ubo_->updateElement("lightColourIntensity", &lightIntensity);
        ubo_->updateElement("lightDirection", &dir);

        float radius = lm->getSunAngularRadius();
        float size = lm->getSunHaloSize();
        float falloff = lm->getSunHaloFalloff();

        sun.x = std::sin(radius);
        sun.y = std::cos(radius);
        sun.z = 1.0f / (std::cos(radius * size) - sun.x);
        sun.w = falloff;
    }
    ubo_->updateElement("sun", &sun);
}

void SceneUbo::upload(IEngine& engine)
{
    ubo_->mapGpuBuffer(engine.driver(), ubo_->getBlockData());
}

} // namespace yave