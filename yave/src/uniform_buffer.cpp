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

auto elementTypeSizeof(backend::BufferElementType type)
{
    int output;
    switch (type)
    {
        case backend::BufferElementType::Uint:
            output = sizeof(uint32_t);
            break;
        case backend::BufferElementType::Int:
            output = sizeof(int);
            break;
        case backend::BufferElementType::Int2:
            output = sizeof(int) * 2;
            break;
        case backend::BufferElementType::Int3:
            output = sizeof(int) * 3;
            break;
        case backend::BufferElementType::Float:
            output = sizeof(float);
            break;
        case backend::BufferElementType::Float2:
            output = sizeof(float) * 2;
            break;
        case backend::BufferElementType::Float3:
            output = sizeof(float) * 3;
            break;
        case backend::BufferElementType::Float4:
            output = sizeof(float) * 4;
            break;
        case backend::BufferElementType::Mat3:
            output = sizeof(float) * 3 * 3;
            break;
        case backend::BufferElementType::Mat4:
            output = sizeof(float) * 4 * 4;
            break;
        case backend::BufferElementType::Struct:
            break;
        default:
            SPDLOG_CRITICAL("Unrecognised element type.");
    }
    return output;
}

auto elementTypeToStrAndSize(const BufferBase::Info& info)
{
    std::pair<std::string, int> output;
    switch (info.type)
    {
        case backend::BufferElementType::Uint:
            output = std::make_pair("uint", 4);
            break;
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
        // its OK to have null element values - i.e.
        // padding elements are usually not defined.
        if (element.value)
        {
            memcpy(bufferData_ + offset, element.value, element.size);
        }
        offset += element.size;
    }
    return (void*)bufferData_;
}

void BufferBase::addElement(
    const std::string& name,
    backend::BufferElementType type,
    void* value,
    uint32_t outerArraySize,
    uint32_t innerArraySize,
    const std::string& structName) noexcept
{
    uint32_t byteSize = elementTypeSizeof(type) * (innerArraySize * outerArraySize);

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
    elements_.push_back(
        {name, type, byteSize, newValue, innerArraySize, outerArraySize, structName});
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
    if (!iter->value)
    {
        iter->value = new uint8_t[size];
    }
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
        // TODO: add support for 2d arrays
        if (element.outerArraySize > 1)
        {
            output += "[" + std::to_string(element.outerArraySize) + "]";
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
    createGpuBuffer(driver, accumSize_);
}

void UniformBuffer::mapGpuBuffer(vkapi::VkDriver& driver, void* data, size_t size) noexcept
{
    vkapi::Buffer* buffer = vkHandle_.getResource();
    ASSERT_FATAL(
        buffer, "Buffer is nullptr - have you created the buffer before trying to map data?");
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

        // check for 2d array
        if (element.innerArraySize > 1)
        {
            ASSERT_FATAL(
                element.outerArraySize > 1,
                "When specifying a 2d array, the outer array size must be greater than one.");
            output += "[" + std::to_string(element.innerArraySize) + "]";
        }
        // An array size of zero denotes the outer array is of unlimited size.
        if (!element.outerArraySize)
        {
            output += "[]";
        }
        else if (element.outerArraySize > 1)
        {
            output += "[" + std::to_string(element.outerArraySize) + "]";
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

    if (size > currentGpuBufferSize_)
    {
        vkHandle_ = driver.addUbo(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        currentGpuBufferSize_ = size;
    }
}

void StorageBuffer::createGpuBuffer(vkapi::VkDriver& driver) noexcept
{
    createGpuBuffer(driver, accumSize_);
}

void StorageBuffer::copyFrom(const StorageBuffer& other) noexcept
{
    elements_ = other.elements_;
    bufferData_ = other.bufferData_;
    currentBufferSize_ = other.currentBufferSize_;
    accumSize_ = other.accumSize_;
    currentGpuBufferSize_ = other.currentGpuBufferSize_;
    vkHandle_ = other.vkHandle_;
}

BufferBase::BackendBufferParams StorageBuffer::getBufferParams(vkapi::VkDriver& driver) noexcept
{
    return {vkHandle_.getResource()->get(), accumSize_, set_, binding_, bufferTypeFromSet(set_)};
}

} // namespace yave
