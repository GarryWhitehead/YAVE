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

#include "managers/component_manager.h"
#include "material.h"
#include "render_primitive.h"
#include "utility/bitset_enum.h"
#include "utility/cstring.h"
#include "utility/handle.h"
#include "utility/logger.h"
#include "vulkan-api/shader.h"
#include "yave/renderable_manager.h"
#include "yave/transform_manager.h"

#include <array>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

namespace yave
{
// forard declerations
class IObject;
class IEngine;
class IRenderable;
class IMaterialBuilder;

class IRenderableManager : public ComponentManager, public RenderableManager
{
public:
    constexpr static uint32_t MeshInitContainerSize = 50;

    IRenderableManager(IEngine& engine);
    ~IRenderableManager();

    /**
     * @brief Returns a instance of a mesh based on the specified IObject
     */
    IRenderable* getMesh(const IObject& obj);

    void buildI(
        IRenderable* renderable,
        IObject* obj,
        const ModelTransform& transform,
        const std::string& matShader);

    IMaterial* createMaterialI() noexcept;

    void destroyI(const IObject& obj);

    // ===================== client api ==================================

    void build(
        Renderable* renderable,
        Object* obj,
        const ModelTransform& transform,
        const std::string& matShader) override;

    Material* createMaterial() noexcept override;

    void destroy(const Object* obj) override;

private:
    IEngine& engine_;

    // the buffers containing all the model data
    std::vector<IRenderable> renderables_;

    // all the materials
    std::vector<std::unique_ptr<IMaterial>> materials_;
};

} // namespace yave
