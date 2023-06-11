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
#include "samplerset.h"
#include "uniform_buffer.h"
#include "yave/texture_sampler.h"

#include <vulkan-api/driver.h>
#include <vulkan-api/program_manager.h>

#include <memory>
#include <string>

namespace yave
{
class IMappedTexture;
class IEngine;

class Compute
{
public:
    static constexpr int UboBindPoint = 0;
    static constexpr int SsboBindPoint = 1;
    static constexpr int MaxSsboCount = 5;

    Compute(IEngine& engine);
    ~Compute();

    void addStorageImage(
        vkapi::VkDriver& driver,
        const std::string& name,
        const vkapi::TextureHandle& handle,
        uint32_t binding,
        ImageStorageSet::StorageType storageType);

    void addImageSampler(
        vkapi::VkDriver& driver,
        const std::string& name,
        const vkapi::TextureHandle& texture,
        uint8_t binding,
        const TextureSampler& sampler);

    void addUboParam(
        const std::string& elementName,
        backend::BufferElementType type,
        void* value,
        size_t arrayCount = 1);

    void addSsbo(
        const std::string& elementName,
        backend::BufferElementType type,
        StorageBuffer::AccessType accessType,
        int binding,
        const std::string& aliasName,
        void* values = nullptr,
        uint32_t outerArraySize = 0,
        uint32_t innerArraySize = 1,
        const std::string& structName = "",
        bool destroy = false);

    void addPushConstantParam(
        const std::string& elementName,
        backend::BufferElementType type,
        void* value = nullptr);

    void updatePushConstantParam(const std::string& elementName, void* value);

    void updateGpuPush() noexcept;

    // add a previously declared ssbo as a reader/writer to another compute shader - must have been declared/written to
    // in a seperate dispatch call.
    void copySsbo(
        const Compute& fromCompute,
        int fromId,
        int toId,
        StorageBuffer::AccessType toAccessType,
        const std::string& toSsboName,
        const std::string& toAliasName,
        bool destroy = false);

    vkapi::ShaderProgramBundle* build(IEngine& engine, const std::string& compShader);

private:
    std::unique_ptr<StorageBuffer> ssbos_[MaxSsboCount];
    std::unique_ptr<UniformBuffer> ubo_;
    std::unique_ptr<PushBlock> pushBlock_;

    ImageStorageSet imageStorageSet_;
    SamplerSet samplerSet_;

    StorageBuffer* extSsboRead_;

    // ================ vulkan backend ===============

    vkapi::ShaderProgramBundle* bundle_;
};

} // namespace yave