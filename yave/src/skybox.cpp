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

#include "skybox.h"

#include "backend/enums.h"
#include "camera.h"
#include "engine.h"
#include "index_buffer.h"
#include "managers/renderable_manager.h"
#include "mapped_texture.h"
#include "material.h"
#include "render_primitive.h"
#include "renderable.h"
#include "scene.h"
#include "utility/assertion.h"
#include "vertex_buffer.h"
#include "yave/object_manager.h"
#include "yave/scene.h"
#include "yave/texture_sampler.h"

#include <image_utils/cubemap.h>
#include <mathfu/glsl_mappings.h>

#include <array>

namespace yave
{

ISkybox::ISkybox(IEngine& engine, IScene& scene) : engine_(engine)
{
    IRenderableManager* rm = engine_.getRenderableManagerI();
    IObjectManager* om = engine_.getObjManagerI();
    skyboxObj_ = om->createObjectI();
    scene.addObject(skyboxObj_);

    material_ = rm->createMaterialI();
}

void ISkybox::buildI(IScene& scene, ICamera& cam)
{
    auto& driver = engine_.driver();
    IRenderableManager* rm = engine_.getRenderableManagerI();

    TextureSampler sampler(
        backend::SamplerFilter::Linear,
        backend::SamplerFilter::Linear,
        backend::SamplerAddressMode::ClampToEdge,
        16);

    material_->addImageTexture(
        engine_.driver(), cubeTexture_, Material::ImageType::BaseColour, sampler.get(), 0);

    IRenderable* render = engine_.createRenderableI();
    IVertexBuffer* vBuffer = engine_.createVertexBufferI();
    IIndexBuffer* iBuffer = engine_.createIndexBufferI();
    IRenderPrimitive* prim = engine_.createRenderPrimitiveI();
    render->setPrimitiveCount(1);
    render->skipVisibilityChecks();

    vBuffer->addAttribute(VertexBuffer::BindingType::Position, backend::BufferElementType::Float3);
    vBuffer->buildI(
        driver, CubeMap::Vertices.size() * sizeof(float), (void*)CubeMap::Vertices.data());
    iBuffer->buildI(
        driver,
        CubeMap::Indices.size(),
        (void*)CubeMap::Indices.data(),
        backend::IndexBufferType::Uint32);
    prim->addMeshDrawDataI(CubeMap::Indices.size(), 0, 0);

    prim->setVertexBuffer(vBuffer);
    prim->setIndexBuffer(iBuffer);
    render->setPrimitive(prim, 0);

    material_->setCullMode(backend::CullMode::Front);
    material_->setViewLayer(0x4);
    prim->setMaterialI(material_);

    rm->buildI(scene, render, skyboxObj_, {}, "skybox.glsl");
}

ISkybox& ISkybox::setCubeMap(IMappedTexture* cubeTexture)
{
    ASSERT_FATAL(cubeTexture, "The cube texture is nullptr.");
    cubeTexture_ = cubeTexture;

    return *this;
}

void ISkybox::update(ICamera& camera) noexcept {}

// ======================== client api =======================

Skybox::Skybox() {}
Skybox::~Skybox() {}

void ISkybox::setTexture(Texture* texture) noexcept
{
    setCubeMap(reinterpret_cast<IMappedTexture*>(texture));
}

void ISkybox::build(Scene* scene, Camera* camera)
{
    buildI(*(reinterpret_cast<IScene*>(scene)), *(reinterpret_cast<ICamera*>(camera)));
}

} // namespace yave
