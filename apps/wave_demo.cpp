
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

#include "wave_demo.h"

#include "backend/convert_to_vk.h"
#include "backend/convert_to_yave.h"
#include "backend/enums.h"
#include "yave/camera.h"
#include "yave/index_buffer.h"
#include "yave/indirect_light.h"
#include "yave/object_manager.h"
#include "yave/render_primitive.h"
#include "yave/renderable.h"
#include "yave/renderable_manager.h"
#include "yave/skybox.h"
#include "yave/texture.h"
#include "yave/texture_sampler.h"
#include "yave/vertex_buffer.h"
#include "yave/wave_generator.h"

#include <ibl/ibl.h>
#include <imgui.h>
#include <utility/colour.h>
#include <yave_app/app.h>

#include <memory>

void WaveApp::uiCallback(yave::Engine* engine) {}

int main()
{
    yave::AppParams params {"wave demo", 1920, 1080};
    WaveApp app(params, true);

    app.engine_ = app.getEngine();
    app.scene_ = app.getScene();

    // create irradiance/specular maps
    /* yave::Ibl ibl(app.engine_, YAVE_ASSETS_DIRECTORY);
    if (!ibl.loadEqirectImage("hdr/monoLake.hdr"))
    {
        exit(1);
    }
    yave::IndirectLight* il = app.engine_->createIndirectLight();
    il->setIrrandianceMap(ibl.getIrradianceMap());
    il->setSpecularMap(ibl.getSpecularMap(), ibl.getBrdfLut());
    app.scene_->setIndirectLight(il);*/

    yave::AssetLoader loader(app.engine_);
    loader.setAssetFolder(YAVE_ASSETS_DIRECTORY);

    // add the skybox to the scene
    yave::Skybox* skybox = app.engine_->createSkybox(app.scene_);
    skybox->setColour({0.1f, 0.2f, 0.8f, 1.0f});
    skybox->renderSun(true);
    //skybox->setTexture(ibl.getCubeMap());
    skybox->build(app.scene_, app.getWindow()->getCamera());
    app.scene_->setSkybox(skybox);

    // add the sun (directional light)
    yave::ObjectManager* objManager = app.engine_->getObjectManager();
    yave::LightManager* lm = app.engine_->getLightManager();

    app.sunObj_ = objManager->createObject();
    app.scene_->addObject(app.sunObj_);

    yave::LightManager::CreateInfo sunParams {
        {1.0f, 80.0f, 1.0f}, {0.7f, -1.0f, -0.8f}, {0.1f, 0.9f, 0.1f}};
    sunParams.intensity = 1100;
    sunParams.sunAngularRadius = 0.5f;
    sunParams.sunHaloSize = 20.0f;
    sunParams.sunHaloFalloff = 5.0f;
    lm->create(sunParams, yave::LightManager::Type::Directional, app.sunObj_);

    // add some waves....
    yave::WaveGenerator* waveGen = app.engine_->createWaveGenerator(app.scene_);
    app.scene_->setWaveGenerator(waveGen);

    // create the renderer used to draw to the backbuffer
    auto handle = app.engine_->createSwapchain(app.getWindow());
    app.engine_->setCurrentSwapchain(handle);
    auto renderer = app.engine_->createRenderer();

    app.run(renderer, app.scene_);

    yave::Engine::destroy(app.engine_);

    exit(0);
}