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

#include "primitives.h"

#include "yave/camera.h"
#include "yave/engine.h"
#include "yave/index_buffer.h"
#include "yave/light_manager.h"
#include "yave/material.h"
#include "yave/object.h"
#include "yave/object_manager.h"
#include "yave/render_primitive.h"
#include "yave/renderable.h"
#include "yave/renderable_manager.h"
#include "yave/scene.h"
#include "yave/skybox.h"
#include "yave/texture.h"
#include "yave/texture_sampler.h"
#include "yave/transform_manager.h"
#include "yave/vertex_buffer.h"

#include <utility/colour.h>
#include <utility/logger.h>
#include <yave_app/app.h>
#include <yave_app/asset_loader.h>
#include <yave_app/models.h>

#include <memory>

PrimitiveApp::PrimitiveApp(const yave::AppParams& params, bool showUI)
    : yave::Application(params, showUI)
{
    sphereFactors_.baseColourFactor = {0.2f, 0.5f, 0.0f, 1.0f};
    capsuleFactors_.baseColourFactor = {0.0f, 0.8f, 0.5f, 1.0f};
    cubeFactors_.baseColourFactor = {0.4f, 0.1f, 0.6f, 1.0f};
}

void PrimitiveApp::buildPrimitive(
    yave::Engine* engine,
    PrimitiveType type,
    yave::Scene* scene,
    const mathfu::vec3& position,
    const mathfu::vec3& scale,
    const mathfu::quat& rotation)
{
    auto* rendManager = engine->getRenderManager();
    yave::ObjectManager* objManager = engine->getObjectManager();

    auto render = engine->createRenderable();
    auto vBuffer = engine->createVertexBuffer();
    auto iBuffer = engine->createIndexBuffer();
    auto prim = engine->createRenderPrimitive();

    yave::Material* mat;
    yave::Material::MaterialFactors factors;
    switch (type)
    {
        case PrimitiveType::Sphere: {
            yave::generateSphereMesh(engine, 20, vBuffer, iBuffer, prim);
            sphereMat_ = rendManager->createMaterial();
            mat = sphereMat_;
            factors = sphereFactors_;
            break;
        }
        case PrimitiveType::Capsule: {
            yave::generateCapsuleMesh(engine, 40, 3.0f, 3.0f, vBuffer, iBuffer, prim);
            capsuleMat_ = rendManager->createMaterial();
            mat = capsuleMat_;
            factors = capsuleFactors_;
            break;
        }
        case PrimitiveType::Cube: {
            yave::generateCubeMesh(engine, {3.0f, 3.0f, 3.0f}, vBuffer, iBuffer, prim);
            cubeMat_ = rendManager->createMaterial();
            mat = cubeMat_;
            factors = cubeFactors_;
            break;
        }
    }

    render->setPrimitiveCount(1);
    prim->setVertexBuffer(vBuffer);
    prim->setIndexBuffer(iBuffer);
    render->setPrimitive(prim, 0);

    yave::Object obj = objManager->createObject();
    scene->addObject(obj);

    mat->setPipeline(yave::Material::Pipeline::SpecularGlosiness);
    mat->setMaterialFactors(factors);

    mat->setCullMode(backend::CullMode::Back);
    mat->setDepthEnable(true, true);
    prim->setMaterial(mat);

    yave::ModelTransform transform;
    transform.translation = position;
    transform.rot = rotation;
    transform.scale = scale;
    rendManager->build(scene, render, obj, transform);
}

void PrimitiveApp::uiCallback(yave::Engine* engine)
{
    auto* lightManager = engine->getLightManager();

    ImGui::SetNextWindowSize(ImVec2(300.0f, 500.0f));
    ImGui::Begin("Primitive Settings");
    {
        if (ImGui::CollapsingHeader("Sphere Material"))
        {
            ImGui::Indent();
            ImGui::ColorEdit3("Colour##spheremat", sphereFactors_.baseColourFactor.getData());
            ImGui::SliderFloat(
                "Alpha Cutoff##spheremat", &sphereFactors_.alphaMaskCutOff, 0.0f, 1.0f);
            ImGui::SliderFloat("Metallic##spheremat", &sphereFactors_.metallicFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness##spheremat", &sphereFactors_.roughnessFactor, 0.0f, 1.0f);
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Capsule Material"))
        {
            ImGui::Indent();
            ImGui::ColorEdit3("Colour##capsulemat", capsuleFactors_.baseColourFactor.getData());
            ImGui::SliderFloat(
                "Alpha Cutoff##capsulemat", &capsuleFactors_.alphaMaskCutOff, 0.0f, 1.0f);
            ImGui::SliderFloat("Metallic##capsulemat", &capsuleFactors_.metallicFactor, 0.0f, 1.0f);
            ImGui::SliderFloat(
                "Roughness##capsulemat", &capsuleFactors_.roughnessFactor, 0.0f, 1.0f);
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Cube Material"))
        {
            ImGui::Indent();
            ImGui::ColorEdit3("Colour##cubemat", cubeFactors_.baseColourFactor.getData());
            ImGui::SliderFloat("Alpha Cutoff##cubemat", &cubeFactors_.alphaMaskCutOff, 0.0f, 1.0f);
            ImGui::SliderFloat("Metallic##cubemat", &cubeFactors_.metallicFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness##cubemat", &cubeFactors_.roughnessFactor, 0.0f, 1.0f);
            ImGui::Unindent();
        }
    }
    ImGui::End();

    sphereMat_->setColourBaseFactor(sphereFactors_.baseColourFactor);
    sphereMat_->setAlphaMaskCutOff(sphereFactors_.alphaMaskCutOff);
    sphereMat_->setMetallicFactor(sphereFactors_.metallicFactor);
    sphereMat_->setRoughnessFactor(sphereFactors_.roughnessFactor);

    capsuleMat_->setColourBaseFactor(capsuleFactors_.baseColourFactor);
    capsuleMat_->setAlphaMaskCutOff(capsuleFactors_.alphaMaskCutOff);
    capsuleMat_->setMetallicFactor(capsuleFactors_.metallicFactor);
    capsuleMat_->setRoughnessFactor(capsuleFactors_.roughnessFactor);

    cubeMat_->setColourBaseFactor(cubeFactors_.baseColourFactor);
    cubeMat_->setAlphaMaskCutOff(cubeFactors_.alphaMaskCutOff);
    cubeMat_->setMetallicFactor(cubeFactors_.metallicFactor);
    cubeMat_->setRoughnessFactor(cubeFactors_.roughnessFactor);
}

int main()
{
    yave::AppParams params {"primitives", 1920, 1080};
    PrimitiveApp app(params, true);

    yave::Engine* engine = app.getEngine();
    yave::Scene* scene = app.getScene();

    // create the skybox
    yave::AssetLoader loader(engine);
    loader.setAssetFolder(YAVE_ASSETS_DIRECTORY);
    yave::Texture* skyboxTexture =
        loader.loadFromFile("textures/uffizi_rgba16f_cube.ktx", backend::TextureFormat::RGBA16F);
    if (!skyboxTexture)
    {
        exit(1);
    }

    // add the skybox to the scene
    yave::Skybox* skybox = engine->createSkybox(scene);
    skybox->setTexture(skyboxTexture);
    skybox->build(scene);

    scene->setSkybox(skybox);

    // create the renderer used to draw to the backbuffer
    auto handle = engine->createSwapchain(app.getWindow());
    engine->setCurrentSwapchain(handle);
    auto renderer = engine->createRenderer();

    // create objects
    app.buildPrimitive(
        engine, PrimitiveApp::PrimitiveType::Sphere, scene, {0.0f, 0.0f, 0.0f}, {1.5f, 1.5f, 1.5f});
    app.buildPrimitive(engine, PrimitiveApp::PrimitiveType::Capsule, scene, {10.0f, 0.0f, 0.0f});
    app.buildPrimitive(engine, PrimitiveApp::PrimitiveType::Cube, scene, {20.0f, 0.0f, 0.0f});

    // add some lighting to the scene
    auto* lightManager = engine->getLightManager();
    yave::ObjectManager* objManager = engine->getObjectManager();

    yave::LightManager::CreateInfo ci {{2.0f, 2.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.8f, 0.3f, 1.0f}};
    yave::Object lightObj1 = objManager->createObject();
    scene->addObject(lightObj1);
    lightManager->create(ci, yave::LightManager::Type::Directional, lightObj1);

    ci.colour = {0.4f, 0.2f, 0.0f};
    ci.position = {0.0f, 0.3f, -2.0f};
    ci.fov = 45.0f;
    ci.radius = 100.0f;
    yave::Object lightObj2 = objManager->createObject();
    scene->addObject(lightObj2);
    lightManager->create(ci, yave::LightManager::Type::Spot, lightObj2);

    app.run(renderer, scene);

    yave::Engine::destroy(engine);

    exit(0);
}
