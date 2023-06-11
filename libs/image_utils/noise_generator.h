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

#include <utility/assertion.h>

#include <cstdint>
#include <vector>

namespace yave
{

class NoiseGenerator
{
public:
    NoiseGenerator(uint32_t seed);
    ~NoiseGenerator();

    double generateNoise(double x, double y, double z);

    template <typename TYPE>
    TYPE* generateImage(uint32_t imageWidth, uint32_t imageHeight, uint32_t channels)
    {
        ASSERT_FATAL(
            channels > 0 && channels <= 4, "Only r, rg or rgb channels supported for image gen.");
        TYPE* imageBuffer = new TYPE[imageWidth * imageHeight * channels];

        for (uint32_t i = 0; i < imageHeight; ++i)
        {
            for (uint32_t j = 0; j < imageWidth; j += channels)
            {
                double x = static_cast<double>(j) / static_cast<double>(imageWidth);
                double y = static_cast<double>(i) / static_cast<double>(imageHeight);

                double noise = generateNoise(10 * x, 10 * y, 0.8);

                for (uint32_t k = 0; k < channels; ++k)
                {
                    if (k == 3)
                    {
                        imageBuffer[(i * imageWidth + j) + 3] = (TYPE)1;
                        continue;
                    }
                    imageBuffer[(i * imageWidth + j) + k] = (TYPE)noise;
                }
            }
        }
        return imageBuffer;
    }

private:
    double fade(double t);

    double lerp(double t, double a, double b);

    double grad(int hash, double x, double y, double z);

private:
    std::vector<int> permutations_;
};

} // namespace yave