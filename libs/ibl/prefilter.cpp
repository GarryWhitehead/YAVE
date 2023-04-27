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
#include <image_utils/cubemap.h>
#include <utility/assertion.h>
#include <yave/camera.h>
#include <yave/engine.h>
#include <yave/index_buffer.h>
#include <yave/material.h>
#include <yave/object.h>
#include <yave/render_primitive.h>
#include <yave/renderable.h>
#include <yave/renderable_manager.h>
#include <yave/renderer.h>
#include <yave/scene.h>
#include <yave/texture.h>
#include <yave/texture_sampler.h>
#include <yave/transform_manager.h>
#include <yave/vertex_buffer.h>

namespace yave
{

PreFilter::PreFilter(Engine* engine, const Options& options) : engine_(engine), options_(options)
{
    scene_ = engine_->createScene();
    engine_->setCurrentScene(scene_);

    camera_ = engine->createCamera();
    camera_->setProjection(90.0f, 1.0f, 1.0f, 512.0f);
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

    prim_ = engine_->createRenderPrimitive();
    prim_->setVertexBuffer(vBuffer);
    prim_->setIndexBuffer(iBuffer);
    prim_->addMeshDrawData(CubeMap::Indices.size(), 0, 0);

    render_ = engine_->createRenderable();
    render_->setPrimitiveCount(1);
    render_->setPrimitive(prim_, 0);
    render_->skipVisibilityChecks();
    render_->disableGBuffer();
}

PreFilter::~PreFilter() 
{ 
    engine_->destroy(prim_);
    engine_->destroy(camera_);
    engine_->destroy(render_);
    engine_->destroy(renderer_);
    engine_->destroy(vBuffer);
    engine_->destroy(iBuffer);
    engine_->destroy(scene_);
}

Texture* PreFilter::eqirectToCubemap(Texture* hdrImage)
{
    ASSERT_FATAL(hdrImage, "Input image is nullptr!");

    RenderableManager* rManager = engine_->getRenderManager();
    Object* obj = scene_->createObject();
    Material* mat = rManager->createMaterial();
   
    TextureSampler sampler(
        backend::SamplerFilter::Linear,
        backend::SamplerFilter::Linear,
        backend::SamplerAddressMode::ClampToEdge,
        16.0f);
    mat->addTexture(engine_, hdrImage, Material::ImageType::BaseColour, sampler);

    std::array<mathfu::mat4, 6> faceViews;
    CubeMap::createFaceViews(faceViews);
    mat->addUboArrayParam(
        "faceViews",
        backend::BufferElementType::Mat4,
        6,
        backend::ShaderStage::Vertex,
        faceViews.data());
    prim_->setMaterial(mat);

    rManager->build(render_, obj, {}, "eqirect_to_cubemap.glsl");

    // create the cube texture to render into
    Texture* cubeTex = engine_->createTexture();
    cubeTex->setEmptyTexture(
        512,
        512,
        backend::TextureFormat::RGBA16,
        backend::ImageUsage::ColourAttach | backend::ImageUsage::Sampled,
        1,
        6);

    // set the empty cube map as the render target for our draws
    RenderTarget rt;
    rt.setColourTexture(cubeTex, 0);
    rt.setLoadFlags(backend::LoadClearFlags::Clear, 0);
    rt.setStoreFlags(backend::StoreClearFlags::Store, 0);

    // Note: for cube map targets we use a multi-view renderpass to render all faces with one draw
    // call
    rt.build(engine_, "eqicube_target", true);
    renderer_->renderSingleScene(engine_, scene_, rt);

    engine_->deleteRenderTarget(rt.getHandle());

    scene_->destroy(obj);
    rManager->destroy(mat);

    return cubeTex;
}

Texture* PreFilter::createIrradianceEnvMap(Texture* cubeMap) 
{
    ASSERT_FATAL(cubeMap, "Cubemap image is nullptr!");

    RenderableManager* rManager = engine_->getRenderManager();
    Object* obj = scene_->createObject();
    Material* mat = rManager->createMaterial();

    TextureSampler cubeSampler(backend::SamplerFilter::Linear, backend::SamplerFilter::Linear);

    std::array<mathfu::mat4, 6> faceViews;
    CubeMap::createFaceViews(faceViews);
    mat->addUboArrayParam(
        "faceViews",
        backend::BufferElementType::Mat4,
        6,
        backend::ShaderStage::Vertex,
        faceViews.data());
    mat->addTexture(engine_, cubeMap, Material::ImageType::BaseColour, cubeSampler);
    prim_->setMaterial(mat);
 
    rManager->build(render_, obj, {}, "irradiance.glsl");

    // create the irradiance cubemap texture to render into
    Texture* cubeTex = engine_->createTexture();
    cubeTex->setEmptyTexture(
        64,
        64,
        backend::TextureFormat::RGBA32,
        backend::ImageUsage::ColourAttach | backend::ImageUsage::Sampled,
        0xFFFF,
        6);

    // set the empty cube map as the render target for our draws
    RenderTarget rt;
    rt.setColourTexture(cubeTex, 0);
    rt.setLoadFlags(backend::LoadClearFlags::Clear, 0);
    rt.setStoreFlags(backend::StoreClearFlags::Store, 0);

    // Render each cubemap mip level (faces are drawn in one call with multiview)
    uint32_t maxMipLevels = cubeTex->getTextureParams().levels;
    uint32_t dim = cubeTex->getTextureParams().width;

    for (int level = 0; level < maxMipLevels; ++level)
    {
        rt.setMipLevel(level, 0);
        rt.build(engine_, "irradiance_target", true);
        mat->setViewport(dim, dim, 0, 0);
        renderer_->renderSingleScene(engine_, scene_, rt);
        dim >>= 1;

        engine_->deleteRenderTarget(rt.getHandle());
    }

    scene_->destroy(obj);
    rManager->destroy(mat);

    return cubeTex;
}

Texture* PreFilter::createSpecularEnvMap(Texture* cubeMap) 
{
    ASSERT_FATAL(cubeMap, "Cubemap image is nullptr!");

    float roughness = 0.0f;

    RenderableManager* rManager = engine_->getRenderManager();
    Object* obj = scene_->createObject();
    Material* mat = rManager->createMaterial();

    TextureSampler cubeSampler(backend::SamplerFilter::Linear, backend::SamplerFilter::Linear);

    std::array<mathfu::mat4, 6> faceViews;
    CubeMap::createFaceViews(faceViews);
    mat->addUboArrayParam(
        "faceViews",
        backend::BufferElementType::Mat4,
        6,
        backend::ShaderStage::Vertex,
        faceViews.data());
    mat->addUboParam(
        "sampleCount",
        backend::BufferElementType::Float,
        backend::ShaderStage::Fragment,
        &options_.sampleCount);
    mat->addUboParam(
        "roughness", backend::BufferElementType::Float, backend::ShaderStage::Fragment, &roughness);
    mat->addTexture(engine_, cubeMap, Material::ImageType::BaseColour, cubeSampler);    
    prim_->setMaterial(mat);

    rManager->build(render_, obj, {}, "specular.glsl");

    // create the irradiance cubemap texture to render into
    Texture* cubeTex = engine_->createTexture();
    cubeTex->setEmptyTexture(
        512,
        512,
        backend::TextureFormat::RGBA16,
        backend::ImageUsage::ColourAttach | backend::ImageUsage::Sampled,
        0xFFFF,
        6);

    // set the empty cube map as the render target for our draws
    RenderTarget rt;
    rt.setColourTexture(cubeTex, 0);
    rt.setLoadFlags(backend::LoadClearFlags::Clear, 0);
    rt.setStoreFlags(backend::StoreClearFlags::Store, 0);

    // Render each cubemap mip level (faces are drawn in one call with multiview)
    uint32_t maxMipLevels = cubeTex->getTextureParams().levels;
    uint32_t dim = cubeTex->getTextureParams().width;

    for (int level = 0; level < maxMipLevels; ++level)
    {
        rt.setMipLevel(level, 0);
        rt.build(engine_, "specular_target", true);
        mat->setViewport(dim, dim, 0, 0);

        roughness = static_cast<float>(level) / static_cast<float>(maxMipLevels) - 1.0f;
        mat->updateUboParam("roughness", backend::ShaderStage::Fragment, &roughness);

        renderer_->renderSingleScene(engine_, scene_, rt);
        engine_->deleteRenderTarget(rt.getHandle());
        dim >>= 1;
    }

    scene_->destroy(obj);
    rManager->destroy(mat);

    return cubeTex;
}


} // namespace yave
