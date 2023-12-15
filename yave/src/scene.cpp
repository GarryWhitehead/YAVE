#include "private/scene.h"
#include "private/camera.h"
#include "private/indirect_light.h"
#include "private/wave_generator.h"

namespace yave
{
void Scene::setSkybox(Skybox* skybox)
{
    static_cast<IScene*>(this)->setSkybox(static_cast<ISkybox*>(skybox));
}

void Scene::setIndirectLight(IndirectLight* il)
{
    static_cast<IScene*>(this)->setIndirectLight(static_cast<IIndirectLight*>(il));
}

void Scene::setCamera(Camera* camera)
{
    static_cast<IScene*>(this)->setCamera(static_cast<ICamera*>(camera));
}

void Scene::setWaveGenerator(WaveGenerator* waterGen)
{
    static_cast<IScene*>(this)->setWaveGenerator(static_cast<IWaveGenerator*>(waterGen));
}

Camera* Scene::getCurrentCamera()
{
    return static_cast<IScene*>(this)->getCurrentCamera();
}

void Scene::addObject(Object obj)
{
    static_cast<IScene*>(this)->addObject(obj);
}

void Scene::destroyObject(Object obj)
{
    static_cast<IScene*>(this)->destroyObject(obj);
}

void Scene::usePostProcessing(bool state)
{
    static_cast<IScene*>(this)->usePostProcessing(state);
}

void Scene::useGbuffer(bool state)
{
    static_cast<IScene*>(this)->useGbuffer(state);
}

void Scene::setBloomOptions(const BloomOptions& bloom)
{
    static_cast<IScene*>(this)->setBloomOptions(bloom);
}

void Scene::setGbufferOptions(const GbufferOptions& gb)
{
    static_cast<IScene*>(this)->setGbufferOptions(gb);
}
BloomOptions& Scene::getBloomOptions()
{
    return static_cast<IScene*>(this)->getBloomOptions();
}

GbufferOptions& Scene::getGbufferOptions()
{
    return static_cast<IScene*>(this)->getGbufferOptions();
}

}