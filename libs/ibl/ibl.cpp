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

#include "ibl.h"

#include "prefilter.h"

#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <yave/engine.h>
#include <yave/texture.h>

namespace yave
{

Ibl::Ibl(Engine* engine, const std::filesystem::path& assetPath)
    : engine_(engine), assetPath_(assetPath)
{
}

Ibl::~Ibl() {}

bool Ibl::loadEqirectImage(const std::filesystem::path& path)
{
    std::filesystem::path imagePath = path;
    if (!assetPath_.empty())
    {
        imagePath = assetPath_.c_str() / imagePath;
    }
    auto platformPath = imagePath.make_preferred();

    int width, height, comp;
    float* data = stbi_loadf(platformPath.string().c_str(), &width, &height, &comp, 3);
    if (!data)
    {
        SPDLOG_CRITICAL("Unable to load image at path {}.", platformPath.string().c_str());
        return false;
    }

    if (!stbi_is_hdr(platformPath.string().c_str()))
    {
        SPDLOG_ERROR("Imge must be in the hdr format for ibl.");
        return false;
    }

    ASSERT_FATAL(comp == 3, "Image must contain 3 channels (rgb). This image contains: %d", comp);

    // We don't really want to work with RGB as this format isn't supported widely
    // by graphic card vendors so add an extra channel if we only have three.
    uint32_t oldSize = width * height * 3;
    uint32_t newSize = width * height * 4;
    float* newData = new float[newSize];
    ASSERT_LOG(newData);

    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            uint32_t oldPixelIdx = (i * width + j) * 3;
            uint32_t newPixelIdx = (i * width + j) * 4;

            float newPixel[4] = {
                data[oldPixelIdx], data[oldPixelIdx + 1], data[oldPixelIdx + 2], 1.0f};
            memcpy(newData + newPixelIdx, newPixel, sizeof(float) * 4);
        }
    }
    stbi_image_free(data);
    data = newData;

    uint32_t dataSizeBytes = width * height * 4 * sizeof(float);

    Texture* tex = engine_->createTexture();
    Texture::Params params {
        data,
        dataSizeBytes,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        backend::TextureFormat::RGBA32,
        backend::ImageUsage::Sampled};
    tex->setTexture(params);

    stbi_image_free(data);

    PreFilter preIbl(engine_, {});

    // create a cubemap from a eqirectangular env map
    cubeMap_ = preIbl.eqirectToCubemap(tex);
    irradianceMap_ = preIbl.createIrradianceEnvMap(cubeMap_);
    specularMap_ = preIbl.createSpecularEnvMap(cubeMap_);
    brdfLut_ = preIbl.createBrdfLut();

    return true;
}

} // namespace yave