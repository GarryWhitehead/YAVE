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

#include "window.h"

#include "app.h"
#include "camera_view.h"

#include <imgui.h>
#include <utility/assertion.h>
#include <yave/engine.h>

namespace yave
{

const char* imGuiGlfwGetClipboardText(void* user_data)
{
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

void imGuiGlfwSetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

Window::Window(Application& app, const char* title, uint32_t width, uint32_t height, bool showUI)
    : app_(app),
      width_(width),
      height_(height),
      window_(nullptr),
      monitor_(nullptr),
      showUI_(showUI),
      enterWindow_(nullptr)
{
    bool success = glfwInit();
    ASSERT_FATAL(success, "Failed to initialise GLFW.");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // if no title specified, no window decorations will be used
    if (!title)
    {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }

    // if dimensions set to zero, get the primary monitor which will create a
    // fullscreen, borderless window
    if (width == 0 && height == 0)
    {
        monitor_ = glfwGetPrimaryMonitor();
        vmode_ = glfwGetVideoMode(monitor_);
        width = vmode_->width;
        height = vmode_->height;
    }

    window_ = glfwCreateWindow(width, height, title, monitor_, nullptr);

    // set window inputs
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window_, mouseCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonPressCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetCursorEnterCallback(window_, cursorEnterCallback);

    // create the engine (dependent on the glfw window for creating the device)
    app_.engine_ = Engine::create(this);

    // create the camera view and main camera - we only have one for now
    cameraView_ = std::make_unique<CameraView>();
    cameraView_->setPosition({0.0f, 0.0f, -8.0f});
    camera_ = app.engine_->createCamera();

    // create a scene for our application
    app.scene_ = app.engine_->createScene();

    updateCameraForWindow();

    if (showUI_)
    {
        std::filesystem::path fontPath =
            std::string(YAVE_ASSETS_DIRECTORY) / std::filesystem::path("fonts/Roboto-Regular.ttf");
        app.imgui_ = std::make_unique<ImGuiHelper>(app.engine_, fontPath);

        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

        io.SetClipboardTextFn = imGuiGlfwSetClipboardText;
        io.GetClipboardTextFn = imGuiGlfwGetClipboardText;
        io.ClipboardUserData = window_;

        // if showing UI, setup the cursors used by imgui
        GLFWerrorfun prev_error_callback = glfwSetErrorCallback(nullptr);
        mouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        mouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        mouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
        mouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
        mouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
        mouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
        mouseCursors[ImGuiMouseCursor_ResizeNESW] =
            glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
        mouseCursors[ImGuiMouseCursor_ResizeNWSE] =
            glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
        mouseCursors[ImGuiMouseCursor_NotAllowed] =
            glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
        mouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        mouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        mouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        mouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
        glfwSetErrorCallback(prev_error_callback);
    }
}

Window::~Window() {}

void Window::updateCameraForWindow()
{
    // update the window dimensions
    uint32_t width, height;
    glfwGetWindowSize(window_, (int*)&width, (int*)&height);
    width_ = static_cast<uint32_t>(width);
    height_ = static_cast<uint32_t>(height);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);

    camera_->setProjection(
        app_.cameraFov_, static_cast<float>(width / height), app_.cameraNear_, app_.cameraFar_);
}

CameraView::Movement convertKeyCode(int code)
{
    CameraView::Movement dir;
    switch (code)
    {
        case GLFW_KEY_W:
            dir = CameraView::Movement::Forward;
            break;
        case GLFW_KEY_S:
            dir = CameraView::Movement::Backward;
            break;
        case GLFW_KEY_A:
            dir = CameraView::Movement::Left;
            break;
        case GLFW_KEY_D:
            dir = CameraView::Movement::Right;
            break;
        default:
            dir = CameraView::Movement::None;
    }
    return dir;
}

void Window::poll() noexcept { glfwPollEvents(); }

std::pair<const char**, uint32_t> Window::getInstanceExt() const noexcept
{
    uint32_t count = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

    return std::make_pair(glfwExtensions, count);
}

bool Window::createSurfaceVk(const vk::Instance& instance)
{
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(instance, window_, VK_NULL_HANDLE, &surface);
    surface_ = vk::SurfaceKHR(surface);
    if (err)
    {
        return false;
    }
    return true;
}

vk::SurfaceKHR Window::getSurface() const { return surface_; }

void Window::updateUiMouseData()
{
    if (!showUI_)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (glfwGetInputMode(window_, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        return;
    }

    const bool is_window_focused = glfwGetWindowAttrib(window_, GLFW_FOCUSED) != 0;
    if (is_window_focused)
    {
        if (io.WantSetMousePos)
        {
            glfwSetCursorPos(
                window_, static_cast<double>(io.MousePos.x), static_cast<double>(io.MousePos.y));

            if (!enterWindow_)
            {
                double mouseX, mouseY;
                glfwGetCursorPos(window_, &mouseX, &mouseY);
                lastValidMousePos_ = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
                io.AddMousePosEvent(static_cast<float>(mouseX), static_cast<float>(mouseY));
            }
        }
    }
}

void Window::updateUiMouseCursor()
{
    if (!showUI_)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) ||
        glfwGetInputMode(window_, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        return;
    }

    ImGuiMouseCursor uiCursor = ImGui::GetMouseCursor();
    if (uiCursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else
    {
        glfwSetCursor(
            window_,
            mouseCursors[uiCursor] ? mouseCursors[uiCursor] : mouseCursors[ImGuiMouseCursor_Arrow]);
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Window::keyResponse(GLFWwindow* window, int key, int scan_code, int action, int mode)
{
    if (action != GLFW_PRESS && action != GLFW_RELEASE)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiKey imguiKey = glfwKeyCodeToImGui(key);
    io.AddKeyEvent(imguiKey, (action == GLFW_PRESS));

    if (!io.WantCaptureMouse)
    {
        if (action == GLFW_PRESS)
        {
            cameraView_->keyDownEvent(convertKeyCode(key));
        }
        else if (action == GLFW_RELEASE)
        {
            cameraView_->keyUpEvent(convertKeyCode(key));
        }

        // check exit state
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }
}

void Window::mouseButtonResponse(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < ImGuiMouseButton_COUNT)
    {
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    }

    if (!io.WantCaptureMouse)
    {
        // check the left mouse button
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                cameraView_->mouseButtonDown(xpos, ypos);
            }
            else if (action == GLFW_RELEASE)
            {
                cameraView_->mouseButtonUp();
            }
        }
    }
}

void Window::mouseMoveResponse(GLFWwindow* window, double xpos, double ypos)
{
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(xpos), static_cast<float>(ypos));
    lastValidMousePos_ = ImVec2(static_cast<float>(xpos), static_cast<float>(ypos));

    if (!io.WantCaptureMouse)
    {
        cameraView_->mouseUpdate(xpos, ypos);
    }
}

void Window::scrollResponse(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(static_cast<float>(xoffset), static_cast<float>(yoffset));

    if (!io.WantCaptureMouse)
    {
        app_.cameraFov_ -= yoffset;
        app_.cameraFov_ = std::clamp(app_.cameraFov_, 1.0f, 90.0f);
        camera_->setFov(app_.cameraFov_);
    }
}
void Window::enterResponse(GLFWwindow* window, int entered)
{
    if (!showUI_ || glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (entered)
    {
        enterWindow_ = window;
        io.AddMousePosEvent(lastValidMousePos_.x, lastValidMousePos_.y);
    }
    else if (!entered && enterWindow_)
    {
        lastValidMousePos_ = io.MousePos;
        enterWindow_ = nullptr;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
}

void keyCallback(GLFWwindow* window, int key, int scan_code, int action, int mode)
{
    Window* inputSys = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    inputSys->keyResponse(window, key, scan_code, action, mode);
}

void mouseButtonPressCallback(GLFWwindow* window, int button, int action, int mods)
{
    Window* inputSys = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    inputSys->mouseButtonResponse(window, button, action, mods);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    Window* inputSys = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    inputSys->mouseMoveResponse(window, xpos, ypos);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Window* inputSys = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    inputSys->scrollResponse(window, xoffset, yoffset);
}

void cursorEnterCallback(GLFWwindow* window, int entered)
{
    Window* inputSys = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    inputSys->enterResponse(window, entered);
}

ImGuiKey Window::glfwKeyCodeToImGui(int key)
{
    switch (key)
    {
        case GLFW_KEY_TAB:
            return ImGuiKey_Tab;
        case GLFW_KEY_LEFT:
            return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT:
            return ImGuiKey_RightArrow;
        case GLFW_KEY_UP:
            return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN:
            return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP:
            return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN:
            return ImGuiKey_PageDown;
        case GLFW_KEY_HOME:
            return ImGuiKey_Home;
        case GLFW_KEY_END:
            return ImGuiKey_End;
        case GLFW_KEY_INSERT:
            return ImGuiKey_Insert;
        case GLFW_KEY_DELETE:
            return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE:
            return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE:
            return ImGuiKey_Space;
        case GLFW_KEY_ENTER:
            return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE:
            return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE:
            return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA:
            return ImGuiKey_Comma;
        case GLFW_KEY_MINUS:
            return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD:
            return ImGuiKey_Period;
        case GLFW_KEY_SLASH:
            return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON:
            return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL:
            return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET:
            return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH:
            return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET:
            return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:
            return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK:
            return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK:
            return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK:
            return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN:
            return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE:
            return ImGuiKey_Pause;
        case GLFW_KEY_KP_0:
            return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1:
            return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2:
            return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3:
            return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4:
            return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5:
            return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6:
            return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7:
            return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8:
            return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9:
            return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL:
            return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE:
            return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY:
            return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT:
            return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD:
            return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER:
            return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL:
            return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT:
            return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL:
            return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT:
            return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER:
            return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT:
            return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL:
            return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT:
            return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER:
            return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU:
            return ImGuiKey_Menu;
        case GLFW_KEY_0:
            return ImGuiKey_0;
        case GLFW_KEY_1:
            return ImGuiKey_1;
        case GLFW_KEY_2:
            return ImGuiKey_2;
        case GLFW_KEY_3:
            return ImGuiKey_3;
        case GLFW_KEY_4:
            return ImGuiKey_4;
        case GLFW_KEY_5:
            return ImGuiKey_5;
        case GLFW_KEY_6:
            return ImGuiKey_6;
        case GLFW_KEY_7:
            return ImGuiKey_7;
        case GLFW_KEY_8:
            return ImGuiKey_8;
        case GLFW_KEY_9:
            return ImGuiKey_9;
        case GLFW_KEY_A:
            return ImGuiKey_A;
        case GLFW_KEY_B:
            return ImGuiKey_B;
        case GLFW_KEY_C:
            return ImGuiKey_C;
        case GLFW_KEY_D:
            return ImGuiKey_D;
        case GLFW_KEY_E:
            return ImGuiKey_E;
        case GLFW_KEY_F:
            return ImGuiKey_F;
        case GLFW_KEY_G:
            return ImGuiKey_G;
        case GLFW_KEY_H:
            return ImGuiKey_H;
        case GLFW_KEY_I:
            return ImGuiKey_I;
        case GLFW_KEY_J:
            return ImGuiKey_J;
        case GLFW_KEY_K:
            return ImGuiKey_K;
        case GLFW_KEY_L:
            return ImGuiKey_L;
        case GLFW_KEY_M:
            return ImGuiKey_M;
        case GLFW_KEY_N:
            return ImGuiKey_N;
        case GLFW_KEY_O:
            return ImGuiKey_O;
        case GLFW_KEY_P:
            return ImGuiKey_P;
        case GLFW_KEY_Q:
            return ImGuiKey_Q;
        case GLFW_KEY_R:
            return ImGuiKey_R;
        case GLFW_KEY_S:
            return ImGuiKey_S;
        case GLFW_KEY_T:
            return ImGuiKey_T;
        case GLFW_KEY_U:
            return ImGuiKey_U;
        case GLFW_KEY_V:
            return ImGuiKey_V;
        case GLFW_KEY_W:
            return ImGuiKey_W;
        case GLFW_KEY_X:
            return ImGuiKey_X;
        case GLFW_KEY_Y:
            return ImGuiKey_Y;
        case GLFW_KEY_Z:
            return ImGuiKey_Z;
        case GLFW_KEY_F1:
            return ImGuiKey_F1;
        case GLFW_KEY_F2:
            return ImGuiKey_F2;
        case GLFW_KEY_F3:
            return ImGuiKey_F3;
        case GLFW_KEY_F4:
            return ImGuiKey_F4;
        case GLFW_KEY_F5:
            return ImGuiKey_F5;
        case GLFW_KEY_F6:
            return ImGuiKey_F6;
        case GLFW_KEY_F7:
            return ImGuiKey_F7;
        case GLFW_KEY_F8:
            return ImGuiKey_F8;
        case GLFW_KEY_F9:
            return ImGuiKey_F9;
        case GLFW_KEY_F10:
            return ImGuiKey_F10;
        case GLFW_KEY_F11:
            return ImGuiKey_F11;
        case GLFW_KEY_F12:
            return ImGuiKey_F12;
        default:
            return ImGuiKey_None;
    }
}

} // namespace yave
