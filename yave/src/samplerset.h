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

#include <cstdint>
#include <string>
#include <vector>

namespace yave
{

class SamplerSet
{
public:
    enum class SamplerType
    {
        e2d,
        e3d,
        Cube
    };

    struct SamplerInfo
    {
        // name of the sampler
        std::string name;
        // the set for this sampler
        uint8_t set;
        // binding of the sampler;
        uint8_t binding;
        // type of the sampler
        SamplerType type;
    };

    SamplerSet() = default;

    static std::string samplerTypeToStr(SamplerSet::SamplerType type);

    void pushSampler(
        const std::string& name,
        uint8_t set,
        uint8_t binding,
        SamplerType type) noexcept;

    std::string createShaderStr() noexcept;

    bool empty() const noexcept { return samplers_.empty(); }

private:
    std::vector<SamplerInfo> samplers_;
};
} // namespace yave
