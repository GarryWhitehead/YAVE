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

#include "gltf_model.h"

#include "backend/convert_to_vk.h"
#include "backend/convert_to_yave.h"
#include "backend/enums.h"
#include "yave/camera.h"
#include "yave/index_buffer.h"
#include "yave/render_primitive.h"
#include "yave/renderable.h"
#include "yave/renderable_manager.h"
#include "yave/scene.h"
#include "yave/skybox.h"
#include "yave/texture.h"
#include "yave/texture_sampler.h"
#include "yave/transform_manager.h"
#include "yave/vertex_buffer.h"

#include <imgui.h>
#include <model_parser/gltf/model_material.h>
#include <model_parser/gltf/model_mesh.h>
#include <utility/colour.h>
#include <utility/logger.h>
#include <ibl/ibl.h>
#include <yave_app/app.h>

#include <memory>

yave::Object* GltfModelApp::buildModel(
    const yave::GltfModel& model, yave::Engine* engine, yave::Scene* scene, yave::AssetLoader& loader)
{
    auto* rendManager = engine->getRenderManager();
    auto* renderable = engine->createRenderable();
    auto* obj = scene->createObject();

    size_t primCount = 0;
    for (auto& node : model.nodes)
    {
        primCount += node->getMesh()->primitives_.size();
    }
    renderable->setPrimitiveCount(primCount);

    materials.resize(primCount);
    for (size_t idx = 0; idx < primCount; ++idx)
    {
        materials[idx] = rendManager->createMaterial();
    }

    int primIdx = 0;
    for (auto& node : model.nodes)
    {
        yave::ModelMesh* mesh = node->getMesh();
        yave::ModelMaterial* material = mesh->material_.get();

        yave::Material* mat = materials[primIdx];
        mat->setPipeline(mat->convertPipeline(material->pipeline));

        yave::Material::MaterialFactors factors;
        factors.baseColourFactor = material->attributes.baseColour;
        factors.diffuseFactor = material->attributes.diffuse;
        factors.emissiveFactor = material->attributes.emissive;
        factors.specularFactor = material->attributes.specular;
        factors.metallicFactor = material->attributes.metallic;
        factors.roughnessFactor = material->attributes.roughness;
        factors.alphaMask = material->attributes.alphaMask;
        factors.alphaMaskCutOff = material->attributes.alphaMaskCutOff;
        mat->setMaterialFactors(factors);

        mat->setDoubleSidedState(material->doubleSided);

        // we use the same sampler for all textures
        yave::TextureSampler sampler(
            backend::samplerFilterToYave(material->sampler.magFilter),
            backend::samplerFilterToYave(material->sampler.minFilter),
            backend::samplerWrapModeToYave(material->sampler.addressModeU));

        // load the textures and upload to the gpu
        for (const auto& info : material->textures)
        {
            yave::Texture* tex =
                loader.loadFromFile(info.texturePath, backend::TextureFormat::RGBA8);
            mat->addTexture(engine, tex, mat->convertImageType(info.type), sampler);
        }

        yave::VertexBuffer* vBuffer = engine->createVertexBuffer();
        yave::IndexBuffer* iBuffer = engine->createIndexBuffer();
        yave::RenderPrimitive* prim = engine->createRenderPrimitive();

        vBuffer->addAttribute(
            yave::VertexBuffer::BindingType::Position, backend::BufferElementType::Float3);

        auto meshVariants = mesh->variantBits_;
        if (meshVariants.testBit(yave::ModelMesh::Variant::HasUv))
        {
            vBuffer->addAttribute(
                yave::VertexBuffer::BindingType::Uv, backend::BufferElementType::Float2);
        }
        if (meshVariants.testBit(yave::ModelMesh::Variant::HasNormal))
        {
            vBuffer->addAttribute(
                yave::VertexBuffer::BindingType::Normal, backend::BufferElementType::Float3);
        }
        if (meshVariants.testBit(yave::ModelMesh::Variant::HasWeight))
        {
            vBuffer->addAttribute(
                yave::VertexBuffer::BindingType::Weight, backend::BufferElementType::Float4);
        }
        if (meshVariants.testBit(yave::ModelMesh::Variant::HasJoint))
        {
            vBuffer->addAttribute(
                yave::VertexBuffer::BindingType::Bones, backend::BufferElementType::Float4);
        }
        vBuffer->build(engine, mesh->vertices_.size, mesh->vertices_.data);
        iBuffer->build(
            engine, mesh->indices_.size(), mesh->indices_.data(), backend::IndexBufferType::Uint32);
        prim->setVertexBuffer(vBuffer);
        prim->setIndexBuffer(iBuffer);

        prim->setTopology(backend::primitiveTopologyToYave(mesh->topology_));
        for (auto const& p : mesh->primitives_)
        {
            prim->addMeshDrawData(p.indexCount, p.indexPrimitiveOffset, 0);
        }
        prim->setMaterial(mat);
        renderable->setPrimitive(prim, primIdx);

        rendManager->build(renderable, obj, {});
    }

    return obj;
}

void GltfModelApp::addLighting(yave::LightManager* lightManager, yave::Scene* scene)
{
    dirLightObj = scene->createObject();
    lightManager->create(dirLightParams, yave::LightManager::Type::Directional, dirLightObj);

    spotLightObj = scene->createObject();
    lightManager->create(spotLightParams, yave::LightManager::Type::Spot, spotLightObj);

    lightManager->prepare();
}

void GltfModelApp::uiCallback(yave::Engine* engine)
{
    auto* lightManager = engine->getLightManager();

    ImGui::SetNextWindowSize(ImVec2(300.0f, 500.0f));
    ImGui::Begin("Example settings");
    {
        if (ImGui::CollapsingHeader("Directional light"))
        {
            ImGui::Indent();
            ImGui::Checkbox("Display##dirlight", &showDirLight);
            ImGui::SliderFloat("fov##dirlight", &dirLightParams.fov, 0.1f, 90.0f);
            ImGui::SliderFloat("Intensity##dirlight", &dirLightParams.intensity, 1.0f, 1000.0f);
            ImGui::ColorEdit3("Colour##dirlight", &dirLightParams.colour.x);
            ImGui::SliderFloat3("Position##dirlight", &dirLightParams.position.x, 0.0f, 50.0f);
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Spot light"))
        {
            ImGui::Indent();
            ImGui::Checkbox("Display##spotlight", &showSpotLight);
            ImGui::SliderFloat("fov##spotlight", &spotLightParams.fov, 0.1f, 90.0f);
            ImGui::SliderFloat("Intensity##spotlight", &spotLightParams.intensity, 1.0f, 1000.0f);
            ImGui::ColorEdit3("Colour##spotlight", &spotLightParams.colour.x);
            ImGui::SliderFloat3("Position##spotlight", &spotLightParams.position.x, 0.0f, 50.0f);
            ImGui::Unindent();
        }
    }
    ImGui::End();

    lightManager->setPosition(dirLightParams.position, dirLightObj);
    lightManager->setColour(dirLightParams.colour, dirLightObj);
    lightManager->setFallout(dirLightParams.fallout, dirLightObj);
    lightManager->setIntensity(dirLightParams.intensity, dirLightObj);
    lightManager->setFov(dirLightParams.fov, dirLightObj);

    lightManager->setPosition(spotLightParams.position, spotLightObj);
    lightManager->setColour(spotLightParams.colour, spotLightObj);
    lightManager->setFallout(spotLightParams.fallout, spotLightObj);
    lightManager->setIntensity(spotLightParams.intensity, spotLightObj);
    lightManager->setFov(spotLightParams.fov, spotLightObj);
}

int main()
{
    yave::AppParams params {"gltf model", 1920, 1080};
    GltfModelApp app(params, true);

    yave::Engine* engine = app.getEngine();
    yave::Scene* scene = app.getScene();

    // create irradiance/specular maps
    yave::Ibl ibl(engine, YAVE_ASSETS_DIRECTORY);
    if(!ibl.loadEqirectImage("hdr/monoLake.hdr"))
    {
        exit(1);
    }

    engine->setCurrentScene(scene);

    // create the skybox
    yave::AssetLoader loader(engine);
    loader.setAssetFolder(YAVE_ASSETS_DIRECTORY);
    yave::Texture* skyboxTexture =
        loader.loadFromFile("textures/uffizi_rgba16f_cube.ktx", backend::TextureFormat::RGBA16);
    if (!skyboxTexture)
    {
        exit(1);
    }

    // add the skybox to the scene
    yave::Skybox* skybox = engine->createSkybox();
    skybox->setTexture(skyboxTexture);
    skybox->build(app.getWindow()->getCamera());

    scene->setSkybox(skybox);

    // create the renderer used to draw to the backbuffer
    auto handle = engine->createSwapchain(app.getWindow());
    engine->setCurrentSwapchain(handle);
    auto renderer = engine->createRenderer();

    // add a gltf model to the scene
    yave::GltfModel model;
    model.setDirectory(YAVE_ASSETS_DIRECTORY);
    if (!model.load("scenes/teapot.gltf"))
    {
        exit(1);
    }
    model.build();

    app.buildModel(model, engine, scene, loader);

    auto lightManager = engine->getLightManager();

    // add some lighting to the scene
    app.addLighting(lightManager, scene);

    app.run(renderer, scene);

    yave::Engine::destroy(engine);

    exit(0);
}
