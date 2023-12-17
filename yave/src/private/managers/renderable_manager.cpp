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
#include "renderable.h"
#include "transform_manager.h"
#include "utility/assertion.h"
#include "yave/object.h"
#include "yave/renderable.h"
#include "yave/renderable_manager.h"

namespace yave
{

IRenderableManager::IRenderableManager(IEngine& engine) : engine_(engine)
{
    // for performance purposes
    renderables_.reserve(MeshInitContainerSize);
}

IRenderableManager::~IRenderableManager() = default;

void IRenderableManager::build(
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
        renderables_.emplace_back(renderable);
    }
    else
    {
        renderables_[objHandle.get()] = renderable;
    }
}

IMaterial* IRenderableManager::createMaterial() noexcept
{
    auto* mat = new IMaterial(engine_);
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
    return renderables_[handle.get()];
}

void IRenderableManager::destroy(const Object& obj)
{
    engine_.getTransformManager()->removeObject(obj);
    removeObject(obj);
}

void IRenderableManager::destroy(IMaterial* mat)
{
    auto iter = materials_.find(mat);
    ASSERT_FATAL(iter != materials_.end(), "Material not found in set.");
    delete mat;
    materials_.erase(iter);
}

} // namespace yave
