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
    static constexpr int SsboReadOnlyBindPoint = 1;
    static constexpr int SsboWriteOnlyBindPoint = 2;

    Compute(IEngine& engine);
    ~Compute();

    void addSamplerTexture(
        vkapi::VkDriver& driver,
        const std::string& name,
        const vkapi::TextureHandle& handle,
        const backend::TextureSamplerParams& params,
        uint32_t binding,
        ImageStorageSet::StorageType storageType);

    void addUboParam(
        const std::string& elementName,
        backend::BufferElementType type,
        void* value,
        size_t arrayCount = 1);

    // custom struct version
    void addSsboReadParam(
        const std::string& elementName,
        void* values,
        const std::string& structName,
        uint32_t arraySize = 0);

    void addSsboWriteParam(
        const std::string& elementName, const std::string& structName, uint32_t arraySize = 0);

    void addSsboReadParam(
        const std::string& elementName,
        backend::BufferElementType type,
        void* values,
        uint32_t arraySize = 0);

    void addSsboWriteParam(
        const std::string& elementName, backend::BufferElementType type, uint32_t arraySize = 0);

    // add the write-only ssbo as a reader to a compute shader - must have been written to
    // in a seperate dispatch call.
    void addExternalSsboRead(const Compute& compute);

    vkapi::ShaderProgramBundle* build(IEngine& engine, const std::string& compShader);

private:
    std::unique_ptr<StorageBuffer> readSsbo_;
    std::unique_ptr<StorageBuffer> writeSsbo_;
    std::unique_ptr<UniformBuffer> ubo_;
    ImageStorageSet imageStorageSet_;

    StorageBuffer* extSsboRead_;

    // ================ vulkan backend ===============

    vkapi::ShaderProgramBundle* bundle_;
};

} // namespace yave