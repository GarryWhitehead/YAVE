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

#include <spdlog/spdlog.h>

namespace yave
{

std::string SamplerSet::samplerTypeToStr(SamplerType type)
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

uint32_t SamplerSet::getSamplerBinding(const std::string& name)
{
    auto iter = std::find_if(samplers_.begin(), samplers_.end(), [&name](const SamplerInfo& info) {
        return name == info.name;
    });
    ASSERT_FATAL(
        iter != samplers_.end(), "Sampler with name %s not found in sampler set.", name.c_str());
    return iter->binding;
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

void ImageStorageSet::addStorageImage(
    const std::string& name,
    uint8_t set,
    uint8_t binding,
    SamplerType samplerType,
    StorageType storageType,
    const std::string& formatLayout) noexcept
{
    samplers_.push_back({name, set, binding, samplerType, storageType, formatLayout});
}

std::string ImageStorageSet::samplerTypeToStr(SamplerType type)
{
    std::string output;
    switch (type)
    {
        case ImageStorageSet::SamplerType::e2d:
            output = "image2D";
            break;
        case ImageStorageSet::SamplerType::e3d:
            output = "image3D";
            break;
        case ImageStorageSet::SamplerType::Cube:
            output = "imageCube";
            break;
    }
    return output;
}

std::string ImageStorageSet::storageTypeToStr(ImageStorageSet::StorageType type)
{
    std::string output;
    switch (type)
    {
        case ImageStorageSet::StorageType::ReadOnly:
            output = "readonly";
            break;
        case ImageStorageSet::StorageType::WriteOnly:
            output = "writeonly";
            break;
        case ImageStorageSet::StorageType::ReadWrite:
            output = "";
            break;
    }
    return output;
}

std::string ImageStorageSet::texFormatToFormatLayout(vk::Format format)
{
    std::string output;
    switch (format)
    {
        case vk::Format::eR8Unorm:
        case vk::Format::eR8Uint:
            output = "r8";
            break;
        case vk::Format::eR8G8Unorm:
        case vk::Format::eR8G8Uint:
            output = "rg8";
            break;
        case vk::Format::eR8G8B8Unorm:
        case vk::Format::eR8G8B8Uint:
            output = "rgb8";
            break;
        case vk::Format::eR8G8B8A8Unorm:
        case vk::Format::eR8G8B8A8Uint:
            output = "rgba8";
            break;
        case vk::Format::eR16Unorm:
        case vk::Format::eR16Uint:
        case vk::Format::eR16Sfloat:
            output = "r16f";
            break;
        case vk::Format::eR16G16Unorm:
        case vk::Format::eR16G16Uint:
        case vk::Format::eR16G16Sfloat:
            output = "rg16f";
            break;
        case vk::Format::eR16G16B16Unorm:
        case vk::Format::eR16G16B16Uint:
        case vk::Format::eR16G16B16Sfloat:
            output = "rgb16f";
            break;
        case vk::Format::eR16G16B16A16Unorm:
        case vk::Format::eR16G16B16A16Uint:
        case vk::Format::eR16G16B16A16Sfloat:
            output = "rgba16f";
            break;
        case vk::Format::eR32Uint:
        case vk::Format::eR32Sfloat:
            output = "r32f";
            break;
        case vk::Format::eR32G32Uint:
        case vk::Format::eR32G32Sfloat:
            output = "rg32f";
            break;
        case vk::Format::eR32G32B32Uint:
        case vk::Format::eR32G32B32Sfloat:
            output = "rgb32f";
            break;
        case vk::Format::eR32G32B32A32Uint:
        case vk::Format::eR32G32B32A32Sfloat:
            output = "rgba32f";
            break;
        default:
            SPDLOG_WARN(
                "Unsupported texture format for compute image storage format layout conversion.");
    }
    return output;
}

std::string ImageStorageSet::createShaderStr() noexcept
{
    std::string output;
    for (const auto& sampler : samplers_)
    {
        std::string imageType = samplerTypeToStr(sampler.type);
        std::string storageType = storageTypeToStr(sampler.storageType);

        output += "layout (set = " + std::to_string(sampler.set) +
            ", binding = " + std::to_string(sampler.binding) + ", " + sampler.formatLayout +
            ") uniform " + storageType + " " + imageType + " " + sampler.name.c_str() + ";\n";
    }
    return output;
}

uint32_t ImageStorageSet::getSamplerBinding(const std::string& name)
{
    auto iter = std::find_if(samplers_.begin(), samplers_.end(), [&name](const SamplerInfo& info) {
        return name == info.name;
    });
    ASSERT_FATAL(
        iter != samplers_.end(), "Sampler with name %s not found in sampler set.", name.c_str());
    return iter->binding;
}


} // namespace yave
