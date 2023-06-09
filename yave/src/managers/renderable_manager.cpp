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

#include "renderable_manager.h"

#include "engine.h"
#include "mapped_texture.h"
#include "render_primitive.h"
#include "renderable.h"
#include "transform_manager.h"
#include "utility/assertion.h"
#include "utility/murmurhash.h"
#include "vulkan-api/commands.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/pipeline_cache.h"
#include "vulkan-api/program_manager.h"
#include "vulkan-api/texture.h"
#include "vulkan-api/utility.h"
#include "yave/object.h"
#include "yave/renderable.h"
#include "yave/renderable_manager.h"

#include <spdlog/spdlog.h>

namespace yave
{

IRenderableManager::IRenderableManager(IEngine& engine) : engine_(engine)
{
    // for performance purposes
    renderables_.reserve(MeshInitContainerSize);
}

IRenderableManager::~IRenderableManager() {}

void IRenderableManager::buildI(
    IScene& scene,
    IRenderable* renderable,
    Object& obj,
    const ModelTransform& transform,
    const std::string& matShader,
    const std::string& mainShaderPath)
{
    for (const auto& prim : renderable->primitives_)
    {
        ASSERT_FATAL(prim->getMaterial(), "Material must be initialised for a render primitive.");
        prim->getMaterial()->build(engine_, scene, *renderable, prim, matShader, mainShaderPath);
    }

    engine_.getTransformManager()->addModelTransform(transform, obj);

    // first add the Object which will give us a free slot
    ObjectHandle objHandle = addObject(obj);

    // check whether we just add to the back or use a freed slot
    if (objHandle.get() >= renderables_.size())
    {
        renderables_.emplace_back(*renderable);
    }
    else
    {
        renderables_[objHandle.get()] = std::move(*renderable);
    }
}

IMaterial* IRenderableManager::createMaterialI() noexcept
{
    IMaterial* mat = new IMaterial(engine_);
    ASSERT_LOG(mat);
    materials_.insert(mat);
    return mat;
}

IRenderable* IRenderableManager::getMesh(const Object& obj)
{
    ObjectHandle handle = getObjIndex(obj);
    ASSERT_FATAL(
        handle.get() <= renderables_.size(),
        "Handle index out of range for renderable mesh (idx=%i)",
        handle.get());
    return &renderables_[handle.get()];
}

void IRenderableManager::destroyI(const Object& obj)
{
    engine_.getTransformManagerI()->removeObject(obj);
    removeObject(obj);
}

void IRenderableManager::destroyI(IMaterial* mat)
{
    auto iter = materials_.find(mat);
    ASSERT_FATAL(iter != materials_.end(), "Material not found in set.");
    delete mat;
    materials_.erase(iter);
}

// ==================== client api ========================

RenderableManager::RenderableManager() {}
RenderableManager::~RenderableManager() {}

void IRenderableManager::build(
    Scene* scene,
    Renderable* renderable,
    Object& obj,
    const ModelTransform& transform,
    const std::string& matShader)
{
    buildI(
        *(reinterpret_cast<IScene*>(scene)),
        reinterpret_cast<IRenderable*>(renderable),
        obj,
        transform,
        matShader);
}

Material* IRenderableManager::createMaterial() noexcept
{
    return reinterpret_cast<Material*>(createMaterialI());
}

void IRenderableManager::destroy(const Object& obj) { destroyI(obj); }

void IRenderableManager::destroy(Material* mat) { destroyI(reinterpret_cast<IMaterial*>(mat)); }

} // namespace yave
