/* Copyright (c) 2018-2020 Garry Whitehead
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

#pragma once

#include "imgui_helper.h"
#include "utility/assertion.h"
#include "utility/cstring.h"
#include "window.h"

#include <cstdint>

// forward declerations
namespace yave
{
class Engine;
class Scene;
class Renderer;
class Camera;
class ImGuiHelper;

struct AppParams
{
    std::string winTitle;
    uint32_t winWidth;
    uint32_t winHeight;
};

class Application
{
public:
    Application(const AppParams& params, bool showUI);
    virtual ~Application() {}

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool run(Renderer* renderer, Scene* scene);

    // user defined callback functions
    virtual void uiCallback(Engine* engine) { return; }

    virtual void preRenderCallback() { return; }

    virtual void postRenderCallback() { return; }

    // ================= getters/setters ========================

    Window* getWindow() noexcept { return window_.get(); }
    Engine* getEngine() noexcept { return engine_; }
    Scene* getScene() noexcept { return scene_; }

    void setCameraFov(float fovy) noexcept { cameraFov_ = fovy; }
    void setCameraNear(float near) noexcept { cameraNear_ = near; }
    void setCameraFar(float far) noexcept { cameraFar_ = far; }

    friend class Window;

private:
    // A engine instance. Only one permitted at the moment.
    Engine* engine_;
    Scene* scene_;

    std::unique_ptr<Window> window_;

    std::unique_ptr<ImGuiHelper> imgui_;

    bool showUI_;
    bool closeApp_;

    double time_;

    // camera paramters
    float cameraFov_;
    float cameraNear_;
    float cameraFar_;
};

} // namespace yave
