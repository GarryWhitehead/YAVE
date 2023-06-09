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

#include "options.h"

#include <memory>

namespace yave
{
class Skybox;
class Camera;
class Object;
class IndirectLight;
class WaveGenerator;

class Scene
{
public:
    virtual ~Scene();

    virtual void setSkybox(Skybox* skybox) = 0;

    virtual void setIndirectLight(IndirectLight* il) = 0;

    virtual void setCamera(Camera* camera) = 0;

    virtual void setWaveGenerator(WaveGenerator* waterGen) = 0;

    virtual Camera* getCurrentCamera() = 0;

    virtual void addObject(Object obj) = 0;
    virtual void destroyObject(Object obj) = 0;

    virtual void usePostProcessing(bool state) = 0;

    virtual void useGbuffer(bool state) = 0;

    virtual void setBloomOptions(const BloomOptions& bloom) = 0;
    virtual void setGbufferOptions(const GbufferOptions& gb) = 0;
    virtual BloomOptions& getBloomOptions() = 0;
    virtual GbufferOptions& getGbufferOptions() = 0;

protected:
    Scene();
};

} // namespace yave
