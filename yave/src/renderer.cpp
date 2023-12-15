#include "private/renderer.h"

#include "private/engine.h"
#include "private/scene.h"

namespace yave
{

void Renderer::beginFrame() { static_cast<IRenderer*>(this)->beginFrame(); }

void Renderer::endFrame() { static_cast<IRenderer*>(this)->endFrame(); }

void Renderer::render(
    Engine* engine, Scene* scene, float dt, util::Timer<NanoSeconds>& timer, bool clearSwap)
{
    static_cast<IRenderer*>(this)->render(
        static_cast<IEngine*>(engine)->driver(), static_cast<IScene*>(scene), dt, timer, clearSwap);
}

void Renderer::renderSingleScene(Engine* engine, Scene* scene, RenderTarget& rTarget)
{
    static_cast<IRenderer*>(this)->renderSingleScene(
        static_cast<IEngine*>(engine)->driver(), static_cast<IScene*>(scene), rTarget);
}

} // namespace yave