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
#include "yave/wave_generator.h"
#include "render_graph/render_graph.h"

#include <utility/timer.h>

#include <mathfu/glsl_mappings.h>

#include <cstdint>

namespace yave
{
class IEngine;
class IScene;
class Compute;
class IMappedTexture;

class IWaveGenerator : public WaveGenerator
{
public:

	// fixed resolution, maybe user defined at some point?
	static constexpr int Resolution = 256;
    // dxyz buffer offsets
    static constexpr int dxOffset = 0;
    static constexpr int dyOffset = Resolution * Resolution;
    static constexpr int dzOffset = dyOffset * 2;
    static constexpr int dxyzBufferSize = Resolution * Resolution * 3;
	
	// temp measure - move to scene!
	struct WaveOptions
    {
		int L = 1000;
        float A = 4.0f;
        mathfu::vec2 windDirection {4.0f, 2.0f};
        float windSpeed = 40.0f;
        float choppyFactor = 1.0f;
        float gridLength = 1024.0f;
	};

	IWaveGenerator(IEngine& engine);
    ~IWaveGenerator();

	void render(rg::RenderGraph& rGraph, IScene& scene, float dt, util::Timer<NanoSeconds>& timer);

private:

	IEngine& engine_;

	size_t log2N_;
    uint32_t reversedBits_[Resolution];
    // 4 channels for our noise texture
    float noiseMap_[Resolution * Resolution * 4];

	// inital spectrum compute
	// output textures
    IMappedTexture* h0kTexture_;
    IMappedTexture* h0minuskTexture_;

    std::unique_ptr<Compute> initialSpecCompute_;
    IMappedTexture* noiseTexture_;
	 
	// spectrum
    IMappedTexture* dztTexture_;
    IMappedTexture* dytTexture_;
    IMappedTexture* dxtTexture_;
    std::unique_ptr<Compute> specCompute_;
	
	// fft butterfly compute
	IMappedTexture* butterflyLut_;
    std::unique_ptr<Compute> butterflyCompute_;

	// fft compute
    std::unique_ptr<Compute> fftHorizCompute_;
    std::unique_ptr<Compute> fftVertCompute_;
    IMappedTexture* pingpongTexture_;

	// displacement
    IMappedTexture* fftOutputImage_;
    IMappedTexture* heightMap_;
    std::unique_ptr<Compute> displaceCompute_;

    // map generation
    IMappedTexture* gradientMap_;
    // height and displacement
    IMappedTexture* displacementMap_;
    std::unique_ptr<Compute> genMapCompute_;

	int pingpong_;

	WaveOptions options;

	bool updateSpectrum_;
};
}