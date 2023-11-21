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

#include "backend/convert_to_vk.h"
#include "backend/enums.h"
#include "common.h"

#include <memory>
#include <unordered_map>

namespace vkapi
{
// forward declarations
class VkContext;
class VkDriver;

/**
 * @brief Used to keep track of samplers dispersed between textures
 * with all samplers kept track of in one place.
 *
 */
class SamplerCache
{
public:
    explicit SamplerCache(VkDriver& driver);
    ~SamplerCache();

    vk::Sampler createSampler(const backend::TextureSamplerParams& params);

    void clear();

    using SamplerCacheMap = std::
        unordered_map<backend::TextureSamplerParams, vk::Sampler, backend::TextureSamplerHasher>;

private:
    VkDriver& driver_;

    SamplerCacheMap samplers_;
};

} // namespace vkapi
