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

#include "app.h"

#include "camera_view.h"
#include "utility/timer.h"
#include "yave/engine.h"
#include "yave/renderer.h"
#include "yave/scene.h"

#include <filesystem>
#include <thread>

using namespace std::literals::chrono_literals;

namespace yave
{

Application::Application(const AppParams& params, bool showUI)
    : engine_(nullptr),
      showUI_(showUI),
      closeApp_(false),
      cameraFov_(90.0f),
      cameraNear_(0.1f),
      cameraFar_(256.0f)
{
    window_ = std::make_unique<Window>(
        *this, params.winTitle.c_str(), params.winWidth, params.winHeight, showUI);

    scene_->setCamera(window_->getCamera());
}

bool Application::run(Renderer* renderer, Scene* scene)
{
    // convert delta time to ms
    const NanoSeconds frameTime(33ms);
    util::Timer<NanoSeconds> timer;

    while (!closeApp_)
    {
        NanoSeconds startTime = timer.getCurrentTime();
        
        // check for any input from the window
        window_->poll();

        double now = glfwGetTime();
        double timeStep = time_ > 0.0 ? (float)(now - time_) : (float)(1.0f / 60.0f);
        time_ = timeStep;

        if (showUI_)
        {
            ImGuiIO& io = ImGui::GetIO();
            io.DeltaTime = time_;

            // this is carried out each frame just in case the window was resized
            GLFWwindow* win = window_->getWindow();
            int winWidth, winHeight;
            int displayWidth, displayHeight;
            glfwGetWindowSize(win, &winWidth, &winHeight);
            glfwGetFramebufferSize(win, &displayWidth, &displayHeight);

            float scaleX = winWidth > 0 ? static_cast<float>(displayWidth / winWidth) : 0;
            float scaleY = winHeight > 0 ? static_cast<float>(displayHeight / winHeight) : 0;
            imgui_->setDisplaySize(winWidth, winHeight, scaleX, scaleY);

            window_->updateUiMouseData();
            window_->updateUiMouseCursor();

            imgui_->beginFrame(*this);
        }

        // update the camera if any changes in key state have been detected
        window_->getCameraView()->updateKeyEvents(timeStep);
        mathfu::mat4 lookAt = window_->getCameraView()->getLookAt();
        window_->getCamera()->setViewMatrix(lookAt);

        renderer->beginFrame();

        // user define pre-render callback
        this->preRenderCallback();

        // begin the rendering for this frame - render the main scene
        renderer->render(engine_, scene, timeStep, timer);
        // and render the UI over this
        if (showUI_)
        {
            renderer->render(engine_, imgui_->getScene(), timeStep, timer, false);
        }

        // user defined post-render callback to be added here
        this->postRenderCallback();

        renderer->endFrame();

        // calculate whether we have any time remaining this frame
        NanoSeconds endTime = timer.getCurrentTime();
        NanoSeconds elapsedTime = endTime - startTime;

        // if we haven't used up the frame time, sleep for remainder
        if (elapsedTime < frameTime)
        {
            std::this_thread::sleep_for(frameTime - elapsedTime);
        }
    }
    return true;
}

} // namespace yave
