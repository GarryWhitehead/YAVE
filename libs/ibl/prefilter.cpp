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

#include "prefilter.h"

#include <backend/enums.h>

#include <yave/engine.h>
#include <yave/texture.h>
#include <yave/material.h>
#include <yave/renderable_manager.h>
#include <yave/renderable.h>
#include <yave/renderer.h>
#include <yave/render_primitive.h>
#include <yave/vertex_buffer.h>
#include <yave/index_buffer.h>
#include <yave/object.h>
#include <yave/transform_manager.h>
#include <yave/texture_sampler.h>
#include <yave/scene.h>
#include <yave/camera.h>

#include <utility/assertion.h>
#include <image_utils/cubemap.h>

namespace yave
{

PreFilter::PreFilter(Engine* engine) : engine_(engine)
{
    scene_ = engine_->createScene();
    engine_->setCurrentScene(scene_);
    camera_ = engine->createCamera();
    scene_->setCamera(camera_);
    renderer_ = engine_->createRenderer();

    vBuffer = engine_->createVertexBuffer();
    iBuffer = engine_->createIndexBuffer();
    vBuffer->addAttribute(VertexBuffer::BindingType::Position, backend::BufferElementType::Float3);
    vBuffer->build(
        engine_, CubeMap::Vertices.size() * sizeof(float), (void*)CubeMap::Vertices.data());
    iBuffer->build(
        engine_,
        CubeMap::Indices.size(),
        (void*)CubeMap::Indices.data(),
        backend::IndexBufferType::Uint32);
}

PreFilter::~PreFilter() {}

Texture* PreFilter::eqirectToCubemap(Texture* hdrImage)
{
    ASSERT_FATAL(hdrImage, "Input image is nullptr!");

    RenderableManager* rManager = engine_->getRenderManager();
    Object* obj = scene_->createObject();
    Material* mat = rManager->createMaterial();
    Renderable* render = engine_->createRenderable();
    RenderPrimitive* prim = engine_->createRenderPrimitive();
    render->setPrimitiveCount(1);

    TextureSampler sampler(
        backend::SamplerFilter::Linear,
        backend::SamplerFilter::Linear,
        backend::SamplerAddressMode::ClampToEdge);
    mat->addTexture(engine_, hdrImage, Material::ImageType::BaseColour, sampler);
    
    mathfu::mat4* faceViews = CubeMap::createFaceViews();
    mat->addUboArrayParam(
        "faceViews", backend::BufferElementType::Mat4, 6, backend::ShaderStage::Vertex, faceViews);
    camera_->setProjection(2.0f / M_PI, 1.0f, 1.0f, 512.0f);

    prim->setVertexBuffer(vBuffer);
    prim->setIndexBuffer(iBuffer);
    prim->setMaterial(mat);
    prim->addMeshDrawData(CubeMap::Indices.size(), 0, 0);

    render->setPrimitive(prim, 0);
    render->skipVisibilityChecks();
    render->disableGBuffer();

    rManager->build(render, obj, {}, "eqirect_to_cubemap.glsl");

    // create the cube texture to render into
    Texture* cubeTex = engine_->createTexture();
    cubeTex->setEmptyTexture(
        512,
        512,
        backend::TextureFormat::RGBA16,
        backend::ImageUsage::Sampled | backend::ImageUsage::ColourAttach,
        1, 6);

    // set the empty cube map as the render target for our draws
    RenderTarget rt;
    rt.setColourTexture(cubeTex, 0);
    // Note: for cube map targets we use a multi-view renderpass to render all faces with one draw call
    rt.build(engine_, "eqicube_target", true);
    renderer_->renderSingleScene(engine_, scene_, rt);
    
    return cubeTex;
}

} // namespace yave