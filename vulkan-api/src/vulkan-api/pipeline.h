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

#include "common.h"
#include "shader.h"
#include "pipeline_cache.h"
#include "utility/compiler.h"

#include <vulkan/vulkan_hash.hpp>

#include <array>
#include <unordered_map>
#include <vector>

namespace vkapi
{
// forward declearions
class VkContext;
class VkDriver;
class RenderPass;
class FrameBuffer;
class ShaderProgram;

/**
 * @brief all the data required to create a pipeline layout
 */
class PipelineLayout
{
public:
    struct PushBlockBindParams
    {
        vk::ShaderStageFlags stage;
        uint32_t size = 0;
        void* data = nullptr;
    };

    PipelineLayout();
    ~PipelineLayout();

    void build(VkContext& context);

    void createDescriptorLayouts(VkContext& context);

    void addPushConstant(backend::ShaderStage type, size_t size);

    void bindPushBlock(
        const vk::CommandBuffer& cmdBuffer, const PushBlockBindParams& pushBlock);

    void addDescriptorLayout(
        uint8_t set,
        uint32_t binding,
        vk::DescriptorType descType,
        vk::ShaderStageFlags stageFlags);

    void clearDescriptors() noexcept { descriptorBindings_.clear(); }

    // ================= getters =========================
    const vk::PipelineLayout& get() const noexcept { return layout_; }
    vk::PipelineLayout& get() noexcept { return layout_; }

    vk::DescriptorSetLayout* getDescSetLayout() noexcept
    {
        return descriptorLayouts_;
    }

    using SetValue = uint8_t;
    using DescriptorBindingMap = std::
        unordered_map<SetValue, std::vector<vk::DescriptorSetLayoutBinding>>;

private:
    // each descriptor type has its own set
    DescriptorBindingMap descriptorBindings_;

    vk::DescriptorSetLayout
        descriptorLayouts_[PipelineCache::MaxDescriptorTypeCount] {};

    // the shader stage the push constant refers to and its size
    std::unordered_map<vk::ShaderStageFlags, size_t> pConstantSizes_;
    vk::PipelineLayout layout_;
};


class Pipeline
{
public:

    static constexpr int LifetimeFrameCount = 10;

    enum class Type
    {
        Graphics,
        Compute
    };

    Pipeline(VkContext& context, const Pipeline::Type type);
    ~Pipeline();

    static vk::PipelineBindPoint createBindPoint(Pipeline::Type type);

    /**
     * Creates a pipeline using render data from the shader program and
     * associates it with the declared renderpass
     */
    void create(
        const PipelineCache::PipelineKey& key, PipelineLayout& pipelineLayout);

    Pipeline::Type getType() const { return type_; }

    const vk::Pipeline& get() const { return pipeline_; }

    uint64_t lastUsedFrameStamp_;

private:
    VkContext& context_;

    // dynamic states to be used with this pipeline - by default the viewport
    // and scissor dynamic states are set
    std::vector<vk::DynamicState> dynamicStates_ {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    Type type_;

    vk::Pipeline pipeline_;

};


} // namespace vkapi
