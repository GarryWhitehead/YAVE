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

Compute::Compute(IEngine& engine)
{
    ubo_ = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboSetValue, UboBindPoint, "ComputeUbo", "compute_ubo");

    pushBlock_ = std::make_unique<PushBlock>("PushBlock", "push_params");

    bundle_ = engine.driver().progManager().createProgramBundle();
}
Compute::~Compute() = default;

void Compute::addStorageImage(
    const std::string& name,
    const vkapi::TextureHandle& texture,
    uint32_t binding,
    ImageStorageSet::StorageType storageType)
{
    ASSERT_FATAL(
        binding < vkapi::PipelineCache::MaxStorageImageBindCount,
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

    bundle_->setStorageImage(texture, binding);
}

void Compute::addImageSampler(
    vkapi::VkDriver& driver,
    const std::string& name,
    const vkapi::TextureHandle& texture,
    uint8_t binding,
    const TextureSampler& sampler)
{
    // all samplers use the same set
    samplerSet_.pushSampler(
        name, vkapi::PipelineCache::SamplerSetValue, binding, SamplerSet::SamplerType::e2d);

    bundle_->setImageSampler(
        texture, binding, driver.getSamplerCache().createSampler(sampler.get()));
}

void Compute::addUboParam(
    const std::string& elementName, backend::BufferElementType type, void* value, size_t arrayCount)
{
    ubo_->addElement(elementName, type, (void*)value, arrayCount);
}

void Compute::addSsbo(
    const std::string& elementName,
    backend::BufferElementType type,
    StorageBuffer::AccessType accessType,
    int binding,
    const std::string& aliasName,
    void* values,
    uint32_t outerArraySize,
    uint32_t innerArraySize,
    const std::string& structName,
    bool destroy)
{
    ASSERT_FATAL(binding < MaxSsboCount, "Binding out of range.");
    std::string bufferName = "SsboBuffer" + std::to_string(binding);
    if (ssbos_[binding] && destroy)
    {
        ssbos_[binding].reset();
    }
    if (!ssbos_[binding])
    {
        ssbos_[binding] = std::make_unique<StorageBuffer>(
            accessType, vkapi::PipelineCache::SsboSetValue, binding, bufferName, aliasName);
        ssbos_[binding]->addElement(
            elementName, type, values, outerArraySize, innerArraySize, structName);
    }
}

void Compute::copySsbo(
    const Compute& fromCompute,
    int fromId,
    int toId,
    StorageBuffer::AccessType toAccessType,
    const std::string& toSsboName,
    const std::string& toAliasName,
    bool destroy)
{
    ASSERT_FATAL(
        fromCompute.ssbos_[fromId] && !fromCompute.ssbos_[fromId]->empty(),
        "The write-only ssbo must have been written to in another compute call before being used "
        "as a reader.");
    ASSERT_FATAL(
        toId < MaxSsboCount, "Can not copy ssbo to id {} and exceed max bind count.", toId);

    // copy the write ssbo details to the read including the GPU buffer address that contains
    // the write contents from the last stage.
    if (destroy)
    {
        ssbos_[toId].reset();
    }
    if (!ssbos_[toId])
    {
        ssbos_[toId] = std::make_unique<StorageBuffer>(
            toAccessType,
            vkapi::PipelineCache::SsboSetValue,
            SsboBindPoint + toId,
            toSsboName,
            toAliasName);
        ssbos_[toId]->copyFrom(*fromCompute.ssbos_[fromId]);
    }
}

void Compute::addPushConstantParam(
    const std::string& elementName, backend::BufferElementType type, void* value)
{
    pushBlock_->addElement(elementName, type, (void*)value);
}

void Compute::updatePushConstantParam(const std::string& elementName, void* value)
{
    pushBlock_->updateElement(elementName, value);
}

void Compute::updateGpuPush() noexcept
{
    if (!pushBlock_->empty())
    {
        void* data = pushBlock_->getBlockData();
        ASSERT_LOG(data);
        bundle_->setPushBlockData(backend::ShaderStage::Compute, data);
    }
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
        ubo_->mapGpuBuffer(ubo_->getBlockData());
    }
    auto params = ubo_->getBufferParams(driver);
    bundle_->addDescriptorBinding(params.size, params.binding, params.buffer, params.type);

    // storage buffers
    for (const auto& ssbo : ssbos_)
    {
        if (ssbo)
        {
            ASSERT_LOG(!ssbo->empty());
            program->addAttributeBlock(ssbo->createShaderStr());
            ssbo->createGpuBuffer(driver);

            void* data = ssbo->getBlockData();
            if (data)
            {
                ssbo->mapGpuBuffer(data);
            }

            params = ssbo->getBufferParams(driver);
            bundle_->addDescriptorBinding(params.size, params.binding, params.buffer, params.type);
        }
    }

    // storage images
    if (!imageStorageSet_.empty())
    {
        program->addAttributeBlock(imageStorageSet_.createShaderStr());
    }

    // image samplers
    if (!samplerSet_.empty())
    {
        program->addAttributeBlock(samplerSet_.createShaderStr());
    }

    // push block
    program->addAttributeBlock(pushBlock_->createShaderStr());

    vkapi::Shader* shader = manager.findShaderVariantOrCreate(
        {}, backend::ShaderStage::Compute, vk::PrimitiveTopology::eTriangleList, bundle_);
    program->addShader(shader);

    return bundle_;
}

} // namespace yave