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

#pragma once

#include "utility/cstring.h"
#include "vulkan/vulkan.hpp"
#include "yave/camera.h"

#include <mathfu/glsl_mappings.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <cstdint>
#include <unordered_map>

namespace yave
{
class CameraView;
class Engine;
class Application;

class Window
{
public:
    Window(Application& app, const char* title, uint32_t width, uint32_t height, bool showUI);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void poll() noexcept;

    void updateCameraForWindow();

    const std::pair<const char**, uint32_t> getInstanceExt() const noexcept;

    bool createSurfaceVk(const vk::Instance& instance);

    const vk::SurfaceKHR getSurface() const;

    void keyResponse(GLFWwindow* window, int key, int scan_code, int action, int mode);
    void mouseButtonResponse(GLFWwindow* window, int button, int action, int mods);
    void mouseMoveResponse(GLFWwindow* window, double xpos, double ypos);
    void scrollResponse(GLFWwindow* window, double xOffset, double yOffset);
    void enterResponse(GLFWwindow* window, int entered);

    static ImGuiKey glfwKeyCodeToImGui(int key);
    void updateUiMouseData();
    void updateUiMouseCursor();

    uint32_t width() const noexcept { return width_; }
    uint32_t height() const noexcept { return height_; }
    GLFWwindow* getWindow() noexcept { return window_; }
    CameraView* getCameraView() noexcept { return cameraView_.get(); }
    Camera* getCamera() noexcept { return camera_; }

private:
    Application& app_;

    uint32_t width_;
    uint32_t height_;

    std::unique_ptr<CameraView> cameraView_;
    Camera* camera_;

    // GLFW member variables
    GLFWwindow* window_;
    GLFWmonitor* monitor_;
    const GLFWvidmode* vmode_;

    // the vk surafce KHR for this window
    vk::SurfaceKHR surface_;

    std::unordered_map<int, int> events_;

    bool showUI_;

    // used by the enterCallback (only used by the UI)
    ImVec2 lastValidMousePos_;
    GLFWwindow* enterWindow_;
    GLFWcursor* mouseCursors[ImGuiMouseCursor_COUNT];
};

// Callbacks from GLFW
void keyCallback(GLFWwindow* window, int key, int scan_code, int action, int mode);

void mouseButtonPressCallback(GLFWwindow* window, int button, int action, int mods);

void mouseCallback(GLFWwindow* window, double xpos, double ypos);

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

void cursorEnterCallback(GLFWwindow* window, int entered);

} // namespace yave
