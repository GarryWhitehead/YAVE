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

#include "yave/engine.h"
#include "yave/light_manager.h"
#include "yave/material.h"
#include "yave/object.h"
#include "yave_app/app.h"

class PrimitiveApp : public yave::Application
{
public:
    enum class PrimitiveType
    {
        Sphere,
        Capsule,
        Cube
    };

    PrimitiveApp(const yave::AppParams& params, bool showUI);

    void buildPrimitive(
        yave::Engine* engine,
        PrimitiveType type,
        const mathfu::vec3& position,
        const mathfu::vec3& scale = {1.0f, 1.0f, 1.0f},
        const mathfu::quat& rotation = {1.0f, 0.0f, 0.0f, 0.0f});

    void uiCallback(yave::Engine* engine) override;

private:
    yave::Material::MaterialFactors sphereFactors_;
    yave::Material::MaterialFactors capsuleFactors_;
    yave::Material::MaterialFactors cubeFactors_;

    yave::Material* sphereMat_;
    yave::Material* capsuleMat_;
    yave::Material* cubeMat_;
};