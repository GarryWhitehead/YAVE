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

#include "samplerset.h"

#include "mapped_texture.h"

namespace yave
{

std::string SamplerSet::samplerTypeToStr(SamplerSet::SamplerType type)
{
    std::string output;
    switch (type)
    {
        case SamplerSet::SamplerType::e2d:
            output = "sampler2D";
            break;
        case SamplerSet::SamplerType::e3d:
            output = "sampler3D";
            break;
        case SamplerSet::SamplerType::Cube:
            output = "samplerCube";
            break;
    }
    return output;
}

void SamplerSet::pushSampler(
    const std::string& name, uint8_t set, uint8_t binding, SamplerType type) noexcept
{
    samplers_.push_back({name, set, binding, type});
}

std::string SamplerSet::createShaderStr() noexcept
{
    std::string output;
    for (const auto& sampler : samplers_)
    {
        std::string type = samplerTypeToStr(sampler.type);
        output += "layout (set = " + std::to_string(sampler.set) +
            ", binding = " + std::to_string(sampler.binding) + ") uniform " + type + " " +
            sampler.name.c_str() + ";\n";
    }
    return output;
}

} // namespace yave
