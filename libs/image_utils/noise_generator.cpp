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

#include "noise_generator.h"

#include <utility/colour.h>

#include <cstdlib>

namespace yave
{
NoiseGenerator::NoiseGenerator(uint32_t noiseWidth, uint32_t noiseHeight)
    : noiseBuffer_(nullptr), noiseWidth_(noiseWidth), noiseHeight_(noiseHeight)
{
    noiseBuffer_ = new double[noiseWidth * noiseHeight];
    generateNoise(noiseBuffer_, noiseWidth, noiseHeight);
}

NoiseGenerator::~NoiseGenerator() 
{ 
    delete noiseBuffer_; 
} 

void NoiseGenerator::generateNoise(
    double* noiseBuffer, uint32_t noiseWidth, uint32_t noiseHeight)
{
    for (uint32_t y = 0; y < noiseHeight; y++)
    {
        for (uint32_t x = 0; x < noiseWidth; x++)
        {
            noiseBuffer[y * noiseWidth + x] = (rand() % 32768) / 32768.0;
        }
    }
}

double NoiseGenerator::smoothNoise(
    double x,
    double y,
    double* noiseBuffer,
    uint32_t noiseWidth,
    uint32_t noiseHeight)
{
    // get fractional part of x and y
    double fractX = x - static_cast<int>(x);
    double fractY = y - static_cast<int>(y);

    // wrap around
    uint32_t x1 = (static_cast<uint32_t>(x) + noiseWidth) % noiseWidth;
    uint32_t y1 = (static_cast<uint32_t>(y) + noiseHeight) % noiseHeight;

    // neighbor values
    uint32_t x2 = (x1 + noiseWidth - 1) % noiseWidth;
    uint32_t y2 = (y1 + noiseHeight - 1) % noiseHeight;

    // smooth the noise with bilinear interpolation
    double value = 0.0;
    value += fractX * fractY * noiseBuffer[y1 * noiseWidth + x1];
    value += (1 - fractX) * fractY * noiseBuffer[y1 * noiseWidth + x2];
    value += fractX * (1 - fractY) * noiseBuffer[y2 * noiseWidth + x1];
    value += (1 - fractX) * (1 - fractY) * noiseBuffer[y2 * noiseWidth + x2];

    return value;
}

uint8_t* NoiseGenerator::generateImage(
    uint32_t imageWidth,
    uint32_t imageHeight,
    uint32_t noiseWidth,
    uint32_t noiseHeight)
{
    uint8_t* imageBuffer = new uint8_t[imageWidth * imageHeight * 4];

    for (uint32_t y = 0; y < imageHeight; y++)
    {
        for (uint32_t x = 0; x < imageWidth; x++)
        {
            util::Colour4 colour {static_cast<float>(
                256 * smoothNoise(x, y, noiseBuffer_, noiseWidth, noiseHeight))};
            imageBuffer[y * imageWidth + x] = static_cast<uint8_t>(colour.r());
            imageBuffer[2 * (y * imageWidth + x)] =
                static_cast<uint8_t>(colour.g());
            imageBuffer[3 * (y * imageWidth + x)] =
                static_cast<uint8_t>(colour.b());
            imageBuffer[4 * (y * imageWidth + x)] = 1;
        }
    }
    return imageBuffer;
}


}