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
    void setSkybox(Skybox* skybox);

    void setIndirectLight(IndirectLight* il);

    void setCamera(Camera* camera);

    void setWaveGenerator(WaveGenerator* waterGen);

    Camera* getCurrentCamera();

    void addObject(Object obj);
    void destroyObject(Object obj);

    void usePostProcessing(bool state);

    void useGbuffer(bool state);

    void setBloomOptions(const BloomOptions& bloom);
    void setGbufferOptions(const GbufferOptions& gb);
    BloomOptions& getBloomOptions();
    GbufferOptions& getGbufferOptions();

protected:
    Scene() = default;
    ~Scene() = default;
};

} // namespace yave
