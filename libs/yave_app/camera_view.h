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

#include <mathfu/glsl_mappings.h>

#include <unordered_map>

namespace yave
{

class CameraView
{
public:
    enum class Movement
    {
        Forward,
        Backward,
        Left,
        Right,
        None
    };

    CameraView();
    ~CameraView();

    void keyUpEvent(Movement movement);
    void keyDownEvent(Movement movement);

    void mouseButtonDown(double x, double y);
    void mouseButtonUp();
    void mouseUpdate(double x, double y);

    void updateView();
    mathfu::vec3 frontVec() const; 
    mathfu::vec3 rightVec() const;

    void updateKeyEvents(float dt); 

    void setPosition(const mathfu::vec3& pos);

    mathfu::mat4 getLookAt() const noexcept;

private:

    mathfu::mat4 view_;
    mathfu::vec3 eye_;
    mathfu::vec3 rotation_;

    std::unordered_map<Movement, bool> keyEvent_;

    mathfu::vec2 mousePosition_;
    bool mouseButtonDown_;

    float moveSpeed_;
};

} // namespace yave