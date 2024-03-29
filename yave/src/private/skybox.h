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

#include "render_graph/render_graph.h"
#include "render_primitive.h"
#include "samplerset.h"
#include "uniform_buffer.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/program_manager.h"
#include "yave/camera.h"
#include "yave/object.h"
#include "yave/scene.h"
#include "yave/skybox.h"
#include "yave/texture.h"

#include <utility/colour.h>

#include <cstdint>

namespace yave
{
class IEngine;
class IMappedTexture;
class IScene;
class ICamera;
class IMaterial;

class ISkybox : public Skybox
{
public:
    ISkybox(IEngine& engine, IScene& scene);

    void build(IScene& scene);

    void update(ICamera& cam) noexcept;

    ISkybox& setCubeMap(IMappedTexture* cubeMap);

    void setColour(const util::Colour4& col) noexcept { skyboxCol_ = col; }

    void renderSun(bool state) noexcept { showSun_ = state; }

    // ============= getters =====================

    IMappedTexture* getCubeMap() noexcept { return cubeTexture_; }

    [[maybe_unused]] void shutDown(vkapi::VkDriver& driver) { YAVE_UNUSED(driver); }

private:
    IEngine& engine_;

    IMappedTexture* cubeTexture_;

    IMaterial* material_;
    Object skyboxObj_;

    // not used if cube texture is specified
    util::Colour4 skyboxCol_;

    bool showSun_;
};

} // namespace yave
