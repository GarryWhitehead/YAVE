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

#include "uniform_buffer.h"

#include "utility/assertion.h"

#include <spdlog/spdlog.h>

namespace yave
{

BufferBase::BufferBase() : bufferData_(nullptr), currentBufferSize_(0), accumSize_(0) {}

BufferBase::~BufferBase()
{
    if (bufferData_)
    {
        delete bufferData_;
    }
    for (const auto& element : elements_)
    {
        if (element.value)
        {
            delete reinterpret_cast<uint8_t*>(element.value);
        }
    }
}

auto elementTypeToStrAndSize(const BufferBase::Info& info)
{
    std::pair<std::string, int> output;
    switch (info.type)
    {
        case backend::BufferElementType::Int:
            output = std::make_pair("int", 4);
            break;
        case backend::BufferElementType::Int2:
            output = std::make_pair("vec2i", 8);
            break;
        case backend::BufferElementType::Int3:
            output = std::make_pair("vec3i", 12);
            break;
        case backend::BufferElementType::Float:
            output = std::make_pair("float", 4);
            break;
        case backend::BufferElementType::Float2:
            output = std::make_pair("vec2", 8);
            break;
        case backend::BufferElementType::Float3:
            output = std::make_pair("vec3", 12);
            break;
        case backend::BufferElementType::Float4:
            output = std::make_pair("vec4", 16);
            break;
        case backend::BufferElementType::Mat3:
            output = std::make_pair("mat3", 36);
            break;
        case backend::BufferElementType::Mat4:
            output = std::make_pair("mat4", 64);
            break;
        case backend::BufferElementType::Struct:
            ASSERT_FATAL(
                !info.structName.empty(),
                "Struct name must be defined when using a struct element.");
            output = std::make_pair(info.structName, 0);
            break;
        default:
            SPDLOG_CRITICAL("Unrecognised element type.");
    }
    return output;
}

vk::DescriptorType bufferTypeFromSet(uint32_t set)
{
    switch (set)
    {
        case vkapi::PipelineCache::UboSetValue:
            return vk::DescriptorType::eUniformBuffer;
            break;
        case vkapi::PipelineCache::UboDynamicSetValue:
            return vk::DescriptorType::eUniformBufferDynamic;
            break;
        case vkapi::PipelineCache::SsboSetValue:
            return vk::DescriptorType::eStorageBuffer;
            break;
        default:
            SPDLOG_WARN("Unrecognised buffer type when converting from set value.");
            break;
    }
    return vk::DescriptorType::eUniformBuffer;
}

void* BufferBase::getBlockData() noexcept
{
    ASSERT_LOG(elements_.size() > 0);

    if (bufferData_ && currentBufferSize_ < accumSize_)
    {
        delete bufferData_;
        bufferData_ = nullptr;
    }

    if (!bufferData_)
    {
        bufferData_ = new uint8_t[accumSize_];
    }
    currentBufferSize_ = accumSize_;

    uint32_t offset = 0;
    for (const auto& element : elements_)
    {
        size_t size = element.size * element.arraySize;
        // its OK to have null element values - i.e.
        // padding elements are usually not defined.
        if (element.value)
        {
            memcpy(bufferData_ + offset, element.value, size);
        }
        offset += size;
    }
    return (void*)bufferData_;
}

void BufferBase::pushElement(
    const std::string& name,
    backend::BufferElementType type,
    uint32_t size,
    void* value,
    uint32_t arraySize,
    const std::string& structName) noexcept
{
    uint32_t byteSize = size * arraySize;

    // if an element of the same name is already associated with the
    // buffer, as long as the value type is identical, this is not considered
    // an error, instead only the new value (if stated) will be updated.
    auto iter = std::find_if(elements_.begin(), elements_.end(), [&name](const Info& info) {
        return name == info.name;
    });
    if (iter != elements_.end())
    {
        ASSERT_FATAL(
            iter->type == type,
            "Element %s already associated with this buffer but trying to add "
            "an alternate element type.",
            iter->name.c_str());
        if (value)
        {
            if (!iter->value)
            {
                iter->value = new uint8_t[byteSize];
            }
            memcpy(iter->value, value, byteSize);
        }
        return;
    }

    uint8_t* newValue = nullptr;
    if (value)
    {
        newValue = new uint8_t[byteSize];
        memcpy(newValue, value, byteSize);
    }
    elements_.push_back({name, type, byteSize, newValue, arraySize, structName});
    accumSize_ += byteSize;
}

void BufferBase::updateElement(const std::string& name, void* data) noexcept
{
    auto iter =
        std::find_if(elements_.begin(), elements_.end(), [&name](const BufferBase::Info& info) {
            return info.name == name;
        });
    ASSERT_FATAL(
        iter != elements_.end(), "Uniform buffer name %s not found in elements list", name.c_str());

    size_t size = iter->size;
    if (iter->value)
    {
        delete reinterpret_cast<uint8_t*>(iter->value);
    }
    iter->value = new uint8_t[size];
    memcpy(iter->value, data, size);
}

size_t BufferBase::getOffset(const std::string& name)
{
    size_t offset = 0;
    for (const auto& element : elements_)
    {
        if (element.name == name)
        {
            return offset;
        }
        offset += element.size;
    }
    SPDLOG_ERROR(
        "Invalid offset call: Uniform buffer name {} not found in elements "
        "list",
        name.c_str());
    return -1;
}

PushBlock::PushBlock(const std::string memberName, const std::string& aliasName)
    : memberName_(memberName), aliasName_(aliasName)
{
}
PushBlock::~PushBlock() {}

std::string PushBlock::createShaderStr() noexcept
{
    if (elements_.empty())
    {
        return "";
    }

    uint32_t offset = 0;
    std::string output;
    output += "layout(push_constant) uniform " + memberName_ + "\n{\n";
    for (const auto& element : elements_)
    {
        const auto& [type, size] = elementTypeToStrAndSize(element);
        output += "\tlayout (offset = " + std::to_string(offset) + ") " + type + " " +
            element.name + ";\n";
        offset += size;
    }
    output += "}" + aliasName_ + ";\n";
    return output;
}

UniformBuffer::UniformBuffer(
    uint32_t set, uint32_t binding, const std::string& memberName, const std::string& aliasName)
    : memberName_(memberName),
      aliasName_(aliasName),
      binding_(binding),
      set_(set),
      currentGpuBufferSize_(0)
{
}

UniformBuffer::~UniformBuffer() {}

std::string UniformBuffer::createShaderStr() noexcept
{
    if (elements_.empty())
    {
        return "";
    }

    std::string output;
    output += "layout (set = " + std::to_string(set_) + ", binding = " + std::to_string(binding_) +
        ") uniform " + memberName_ + "\n{\n";

    for (const auto& element : elements_)
    {
        const auto& [type, size] = elementTypeToStrAndSize(element);
        output += "\t" + type + " " + element.name;
        if (element.arraySize > 1)
        {
            output += "[" + std::to_string(element.arraySize) + "]";
        }
        output += ";\n";
    }
    output += "} " + aliasName_ + ";\n";

    return output;
}

void UniformBuffer::createGpuBuffer(vkapi::VkDriver& driver, size_t size) noexcept
{
    ASSERT_FATAL(elements_.size() > 0, "This uniform has no elements added.");
    if (size > currentGpuBufferSize_)
    {
        vkHandle_ = driver.addUbo(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        currentGpuBufferSize_ = size;
    }
}

void UniformBuffer::createGpuBuffer(vkapi::VkDriver& driver) noexcept
{
    ASSERT_FATAL(elements_.size() > 0, "This uniform has no elements added.");
    if (accumSize_ > currentGpuBufferSize_)
    {
        createGpuBuffer(driver, accumSize_);
        currentGpuBufferSize_ = accumSize_;
    }
}

void UniformBuffer::mapGpuBuffer(vkapi::VkDriver& driver, void* data, size_t size) noexcept
{
    vkapi::Buffer* buffer = vkHandle_.getResource();
    ASSERT_LOG(buffer);
    buffer->mapToGpuBuffer(data, size);
}

void UniformBuffer::mapGpuBuffer(vkapi::VkDriver& driver, void* data) noexcept
{
    mapGpuBuffer(driver, data, accumSize_);
}

UniformBuffer::BackendBufferParams UniformBuffer::getBufferParams(vkapi::VkDriver& driver) noexcept
{
    return {vkHandle_.getResource()->get(), accumSize_, set_, binding_, bufferTypeFromSet(set_)};
}

StorageBuffer::StorageBuffer(
    AccessType type,
    uint32_t set,
    uint32_t binding,
    const std::string& memberName,
    const std::string& aliasName)
    : UniformBuffer(set, binding, memberName, aliasName), accessType_(type)
{
}

StorageBuffer::~StorageBuffer() {}

std::string StorageBuffer::createShaderStr() noexcept
{
    if (elements_.empty())
    {
        return "";
    }

    std::string output;
    std::string ssboType = accessType_ == AccessType::ReadOnly ? "readonly" : "";

    output += "layout (set = " + std::to_string(set_) + ", binding = " + std::to_string(binding_) +
        ") " + ssboType + " buffer " + memberName_ + "\n{\n";

    for (const auto& element : elements_)
    {
        const auto& [type, size] = elementTypeToStrAndSize(element);
        output += "\t" + type + " " + element.name;
        // An array size of zero denotes this as a unlimited array.
        if (!element.arraySize)
        {
            output += "[]";
        }
        else if (element.arraySize > 1)
        {
            output += "[" + std::to_string(element.arraySize) + "]";
        }
        output += ";\n";
    }
    output += "} " + aliasName_ + ";\n";

    return output;
}

void StorageBuffer::createGpuBuffer(vkapi::VkDriver& driver, uint32_t size) noexcept
{
    ASSERT_FATAL(elements_.size() > 0, "This storage buffer has no elements added.");
    ASSERT_LOG(size > 0);
    vkHandle_ = driver.addUbo(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

BufferBase::BackendBufferParams StorageBuffer::getBufferParams(vkapi::VkDriver& driver) noexcept
{
    return {vkHandle_.getResource()->get(), accumSize_, set_, binding_, bufferTypeFromSet(set_)};
}

} // namespace yave
