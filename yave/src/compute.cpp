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

#include "compute.h"

#include "engine.h"
#include "mapped_texture.h"

#include <vulkan-api/common.h>
#include <vulkan-api/sampler_cache.h>

namespace yave
{

Compute::Compute(IEngine& engine) : extSsboRead_(nullptr)
{
    ubo_ = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboSetValue, UboBindPoint, "ComputeUbo", "compute_ubo");

    readSsbo_ = std::make_unique<StorageBuffer>(
        StorageBuffer::AccessType::ReadWrite,
        vkapi::PipelineCache::SsboSetValue,
        SsboReadOnlyBindPoint,
        "ComputeReadSsbo",
        "input_ssbo");

    writeSsbo_ = std::make_unique<StorageBuffer>(
        StorageBuffer::AccessType::ReadWrite,
        vkapi::PipelineCache::SsboSetValue,
        SsboWriteOnlyBindPoint,
        "ComputeWriteSsbo",
        "output_ssbo");

    bundle_ = engine.driver().progManager().createProgramBundle();
}
Compute::~Compute() {}

void Compute::addSamplerTexture(
    vkapi::VkDriver& driver,
    const std::string& name,
    const vkapi::TextureHandle& texture,
    const backend::TextureSamplerParams& params,
    uint32_t binding,
    ImageStorageSet::StorageType storageType)
{
    ASSERT_FATAL(
        binding < vkapi::PipelineCache::MaxSamplerBindCount,
        "Out of range for texture binding (=%d). Max allowed count is %d\n",
        binding,
        vkapi::PipelineCache::MaxSamplerBindCount);

    imageStorageSet_.addStorageImage(
        name,
        vkapi::PipelineCache::StorageImageSetValue,
        binding,
        ImageStorageSet::SamplerType::e2d, // TODO: make a paramater
        storageType,
        ImageStorageSet::texFormatToFormatLayout(texture.getResource()->context().format));

    bundle_->setTexture(texture, binding, driver.getSamplerCache().createSampler(params));
}

void Compute::addUboParam(
    const std::string& elementName, backend::BufferElementType type, void* value, size_t arrayCount)
{
    ubo_->addElement(elementName, type, (void*)value, arrayCount);
}

void Compute::addSsboReadParam(
    const std::string& elementName, void* values, const std::string& structName, uint32_t arraySize)
{
    readSsbo_->addElement(
        elementName, backend::BufferElementType::Struct, values, arraySize, structName);
}

void Compute::addSsboWriteParam(
    const std::string& elementName, const std::string& structName, uint32_t arraySize)
{
    writeSsbo_->addElement(
        elementName, backend::BufferElementType::Struct, nullptr, arraySize, structName);
}

void Compute::addSsboReadParam(
    const std::string& elementName,
    backend::BufferElementType type,
    void* values,
    uint32_t arraySize)
{
    readSsbo_->addElement(elementName, type, values, arraySize);
}

void Compute::addSsboWriteParam(
    const std::string& elementName, backend::BufferElementType type, uint32_t arraySize)
{
    writeSsbo_->addElement(elementName, type, nullptr, arraySize);
}

void Compute::addExternalSsboRead(const Compute& compute)
{
    ASSERT_FATAL(
        !compute.writeSsbo_->empty(),
        "The write-only ssbo must have been written to in another compute call before being used "
        "as a reader.");
    // copy the write ssbo details to the read including the GPU buffer address that contains
    // the write contents from the last stage.
    readSsbo_->copyFrom(*compute.writeSsbo_);
}

vkapi::ShaderProgramBundle* Compute::build(IEngine& engine, const std::string& compShader)
{
    auto& manager = engine.driver().progManager();
    auto& driver = engine.driver();

    bundle_->buildShaders(compShader);

    auto* program = bundle_->getProgram(backend::ShaderStage::Compute);

    // ubo
    if (!ubo_->empty())
    {
        program->addAttributeBlock(ubo_->createShaderStr());
        ubo_->createGpuBuffer(driver);
        ubo_->mapGpuBuffer(driver, ubo_->getBlockData());
    }
    auto params = ubo_->getBufferParams(driver);
    bundle_->addDescriptorBinding(params.size, params.binding, params.buffer, params.type);

    // read ssbo
    // Note: we don't map storage buffers used for compute as we assume the write ssbo will
    // be written by the compute and this will then be read. This might not always be the
    // case so may need changing at some point.
    if (!readSsbo_->empty())
    {
        program->addAttributeBlock(readSsbo_->createShaderStr());
        readSsbo_->createGpuBuffer(driver);

        params = readSsbo_->getBufferParams(driver);
        bundle_->addDescriptorBinding(params.size, params.binding, params.buffer, params.type);
    }

    // write ssbo
    if (!writeSsbo_->empty())
    {
        program->addAttributeBlock(writeSsbo_->createShaderStr());
        writeSsbo_->createGpuBuffer(driver);

        params = writeSsbo_->getBufferParams(driver);
        bundle_->addDescriptorBinding(params.size, params.binding, params.buffer, params.type);
    }

    // samplers
    if (!imageStorageSet_.empty())
    {
        program->addAttributeBlock(imageStorageSet_.createShaderStr());
    }

    vkapi::Shader* shader = manager.findShaderVariantOrCreate(
        {}, backend::ShaderStage::Compute, vk::PrimitiveTopology::eTriangleList, bundle_);
    program->addShader(shader);

    return bundle_;
}

} // namespace yave