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

#include "uniform_buffer.h"

#include <vulkan-api/driver.h>

#include <memory>

namespace yave
{
class IEngine;
class ICamera;
class IIndirectLight;
class LightInstance;

/* A all-in-one ne uniform buffer that holds all the dynamic information
 *  that is required by the scene.
 */
class SceneUbo
{
public:
    static constexpr int SceneUboBindPoint = 3;

    SceneUbo(vkapi::VkDriver& driver);

    void updateCamera(ICamera& camera);

    void updateIbl(IIndirectLight* il);

    void updateDirLight(IEngine& engine, LightInstance* instance);

    void upload(IEngine& engine);

    UniformBuffer& get() noexcept { return *ubo_; }

private:
    std::unique_ptr<UniformBuffer> ubo_;

    uint32_t uboSize_;
};

} // namespace yave