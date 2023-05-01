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

#include "backend/enums.h"

namespace yave
{

class TextureSampler
{
public:
    TextureSampler() = default;

    TextureSampler(backend::SamplerFilter min, backend::SamplerFilter mag) { params_ = {min, mag}; }

    TextureSampler(
        backend::SamplerFilter min, backend::SamplerFilter mag, backend::SamplerAddressMode addr)
    {
        params_ = {min, mag, addr, addr, addr};
    }

    TextureSampler(
        backend::SamplerFilter min,
        backend::SamplerFilter mag,
        backend::SamplerAddressMode addr,
        float ansiotropy)
    {
        params_ = {min, mag, addr, addr, addr};
        params_.anisotropy = ansiotropy;
        params_.enableAnisotropy = true;
    }

    TextureSampler(
        backend::SamplerFilter min,
        backend::SamplerFilter mag,
        backend::SamplerAddressMode addrU,
        backend::SamplerAddressMode addrV,
        backend::SamplerAddressMode addrW)
    {
        params_ = {min, mag, addrU, addrV, addrW};
    }

    backend::TextureSamplerParams& get() noexcept { return params_; }

private:
    backend::TextureSamplerParams params_;
};

} // namespace yave