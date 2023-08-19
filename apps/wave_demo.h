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
#include "yave/scene.h"
#include "yave/transform_manager.h"
#include "yave_app/app.h"

#include <model_parser/gltf/gltf_model.h>
#include <yave_app/asset_loader.h>

class WaveApp : public yave::Application
{
public:

    WaveApp(const yave::AppParams& params, bool showUI) : yave::Application(params, showUI) {}

    void uiCallback(yave::Engine* engine) override;

    yave::Scene* scene_ = nullptr;
    yave::Engine* engine_ = nullptr;

    yave::Object sunObj_;

};