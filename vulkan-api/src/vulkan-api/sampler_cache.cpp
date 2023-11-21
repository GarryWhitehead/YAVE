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

#include "sampler_cache.h"

#include "context.h"
#include "driver.h"

namespace vkapi
{

SamplerCache::SamplerCache(VkDriver& driver) : driver_(driver) {}
SamplerCache::~SamplerCache() = default;

vk::Sampler SamplerCache::createSampler(const backend::TextureSamplerParams& params)
{
    auto iter = samplers_.find(params);
    if (iter != samplers_.end())
    {
        return iter->second;
    }

    vk::Sampler sampler;

    vk::SamplerCreateInfo samplerInfo(
        {},
        backend::samplerFilterToVk(params.mag),
        backend::samplerFilterToVk(params.min),
        vk::SamplerMipmapMode::eLinear, // TODO: should be user definable
        backend::samplerAddrModeToVk(params.addrU),
        backend::samplerAddrModeToVk(params.addrV),
        backend::samplerAddrModeToVk(params.addrW),
        0.0f,
        params.enableAnisotropy,
        params.anisotropy,
        params.enableCompare,
        backend::compareOpToVk(params.compareOp),
        0.0f,
        params.mipLevels == 0 ? 0.25f : static_cast<float>(params.mipLevels),
        vk::BorderColor::eFloatOpaqueWhite,
        VK_FALSE);

    VK_CHECK_RESULT(driver_.context().device().createSampler(&samplerInfo, nullptr, &sampler));

    samplers_.insert({params, sampler});
    return sampler;
}

void SamplerCache::clear()
{
    for (const auto& [_, sampler] : samplers_)
    {
        driver_.context().device().destroy(sampler);
    }
    samplers_.clear();
}

} // namespace vkapi
