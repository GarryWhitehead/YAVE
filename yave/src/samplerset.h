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

#include <vulkan-api/common.h>

#include <cstdint>
#include <string>
#include <vector>

namespace yave
{

// Groups combined image samplers used by the graphics pipeline
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
        std::string name;
        // the set for this sampler
        uint8_t set;
        // binding of the sampler;
        uint8_t binding;
        // type of the sampler
        SamplerType type;
    };

    SamplerSet() = default;

    static std::string samplerTypeToStr(SamplerType type);

    void
    pushSampler(const std::string& name, uint8_t set, uint8_t binding, SamplerType type) noexcept;

    std::string createShaderStr() noexcept;

    uint32_t getSamplerBinding(const std::string& name);

    [[nodiscard]] bool empty() const noexcept { return samplers_.empty(); }

private:
    std::vector<SamplerInfo> samplers_;
};

// Groups storage image samplers used by the compute pipeline
class ImageStorageSet
{
public:
    enum class SamplerType
    {
        e2d,
        e3d,
        Cube
    };

    enum class StorageType
    {
        WriteOnly,
        ReadOnly,
        ReadWrite
    };

    struct SamplerInfo
    {
        std::string name;
        // the set for this sampler
        uint8_t set;
        // binding of the sampler;
        uint8_t binding;
        // type of the sampler
        SamplerType type;
        // whether the image is read or write
        StorageType storageType;
        // the texture format i.e. rgba8
        std::string formatLayout;
    };

    ImageStorageSet() = default;

    static std::string samplerTypeToStr(SamplerType type);

    static std::string storageTypeToStr(ImageStorageSet::StorageType type);

    static std::string texFormatToFormatLayout(vk::Format format);

    void addStorageImage(
        const std::string& name,
        uint8_t set,
        uint8_t binding,
        SamplerType type,
        StorageType storageType,
        const std::string& formatLayout) noexcept;

    std::string createShaderStr() noexcept;

    [[maybe_unused]] uint32_t getSamplerBinding(const std::string& name);

    [[nodiscard]] bool empty() const noexcept { return samplers_.empty(); }

private:
    std::vector<SamplerInfo> samplers_;
};

} // namespace yave
