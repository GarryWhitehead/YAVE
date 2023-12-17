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
#include "yave/scene.h"
#include "yave/texture_sampler.h"

#include <image_utils/cubemap.h>
#include <utility/enum_cast.h>

namespace yave
{

ISkybox::ISkybox(IEngine& engine, IScene& scene)
    : engine_(engine), cubeTexture_(nullptr), skyboxCol_ {0.0f}, showSun_(false)
{
    IRenderableManager* rm = engine_.getRenderableManager();
    IObjectManager* om = engine_.getObjManager();
    skyboxObj_ = om->createObject();
    scene.addObject(skyboxObj_);

    material_ = rm->createMaterial();
}

void ISkybox::build(IScene& scene)
{
    auto& driver = engine_.driver();
    IRenderableManager* rm = engine_.getRenderableManager();

    TextureSampler sampler(
        backend::SamplerFilter::Linear,
        backend::SamplerFilter::Linear,
        backend::SamplerAddressMode::ClampToEdge,
        16);

    int useColour = static_cast<int>(!cubeTexture_);
    int showSun = static_cast<int>(showSun_);

    IMappedTexture* tex = cubeTexture_ ? cubeTexture_ : engine_.getDummyCubeMap();
    material_->addImageTexture(
        engine_.driver(),
        tex,
        Material::ImageType::BaseColour,
        backend::ShaderStage::Fragment,
        sampler.get(),
        0);

    material_->addUboParam(
        "colour",
        backend::BufferElementType::Float4,
        1,
        backend::ShaderStage::Fragment,
        (void*)&skyboxCol_);
    material_->addUboParam(
        "useColour",
        backend::BufferElementType::Int,
        1,
        backend::ShaderStage::Fragment,
        &useColour);
    material_->addUboParam(
        "renderSun", backend::BufferElementType::Int, 1, backend::ShaderStage::Fragment, &showSun);

    IRenderable* render = engine_.createRenderable();
    IVertexBuffer* vBuffer = engine_.createVertexBuffer();
    IIndexBuffer* iBuffer = engine_.createIndexBuffer();
    IRenderPrimitive* prim = engine_.createRenderPrimitive();
    render->setPrimitiveCount(1);
    render->skipVisibilityChecks();

    vBuffer->addAttribute(
        util::ecast(VertexBuffer::BindingType::Position), backend::BufferElementType::Float3);
    vBuffer->build(
        driver, CubeMap::Vertices.size() * sizeof(float), (void*)CubeMap::Vertices.data());
    iBuffer->build(
        driver,
        CubeMap::Indices.size(),
        (void*)CubeMap::Indices.data(),
        backend::IndexBufferType::Uint32);
    prim->addMeshDrawData(CubeMap::Indices.size(), 0, 0);

    prim->setVertexBuffer(vBuffer);
    prim->setIndexBuffer(iBuffer);
    render->setPrimitive(prim, 0);

    material_->setCullMode(backend::cullModeToVk(backend::CullMode::Front));
    material_->setViewLayer(0x4);
    prim->setMaterial(material_);

    rm->build(scene, render, skyboxObj_, {}, "skybox.glsl");
}

ISkybox& ISkybox::setCubeMap(IMappedTexture* cubeTexture)
{
    ASSERT_FATAL(cubeTexture, "The cube texture is nullptr.");
    cubeTexture_ = cubeTexture;

    return *this;
}

void ISkybox::update(ICamera& camera) noexcept {}

} // namespace yave
