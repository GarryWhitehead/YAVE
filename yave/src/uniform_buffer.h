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

#include "utility/cstring.h"
#include "vulkan-api/buffer.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/resource_cache.h"

#include "backend/enums.h"

#include <string>
#include <vector>

namespace yave
{

class BufferBase
{
public:
    struct BackendBufferParams
    {
        vk::Buffer buffer;
        size_t size;
        uint32_t set;
        uint32_t binding;
        vk::DescriptorType type;
    };

    struct Info
    {
        // name of the member
        std::string name;
        // element type
        backend::BufferElementType type;
        // the member type size
        uint32_t size;
        // the member value as binary
        void* value;
        // the array size
        uint32_t arraySize;
        // name of the struct if the element type is "Struct"
        std::string structName;
    };

    BufferBase();
    virtual ~BufferBase();

    void pushElement(
        const std::string& name,
        backend::BufferElementType type,
        uint32_t size,
        void* value = nullptr,
        uint32_t arraySize = 1,
        const std::string& structName = "") noexcept;

    void updateElement(const std::string& name, void* data) noexcept;

    void* getBlockData() noexcept;

    size_t size() const noexcept { return accumSize_; }

    size_t getOffset(const std::string& name);

    bool empty() const noexcept { return elements_.empty(); }

    virtual std::string createShaderStr() noexcept { return ""; };

    virtual BackendBufferParams
    getBufferParams(vkapi::VkDriver& driver) noexcept
    {
        return {};
    }

protected:
    std::vector<Info> elements_;

    uint8_t* bufferData_;
    size_t currentBufferSize_;
    size_t accumSize_;
};

class UniformBuffer : public BufferBase
{
public:
    UniformBuffer(
        uint32_t set,
        uint32_t binding,
        const std::string& memberName,
        const std::string& aliasName = "");
    ~UniformBuffer();

    std::string createShaderStr() noexcept override;

    void createGpuBuffer(vkapi::VkDriver& driver, size_t size) noexcept;

    void createGpuBuffer(vkapi::VkDriver& driver) noexcept;

    void mapGpuBuffer(vkapi::VkDriver& driver, void* data) noexcept;

    void
    mapGpuBuffer(vkapi::VkDriver& driver, void* data, size_t size) noexcept;

    BackendBufferParams
    getBufferParams(vkapi::VkDriver& driver) noexcept override;

    uint8_t getBinding() const noexcept { return binding_; }

protected:
    std::string memberName_;
    std::string aliasName_;
    uint8_t binding_;
    uint32_t set_;
    size_t currentGpuBufferSize_;

    // =========== vulkan backend ============

    vkapi::BufferHandle vkHandle_;
};

class StorageBuffer : public UniformBuffer
{
public:
    enum class AccessType
    {
        ReadOnly,
        ReadWrite
    };

    StorageBuffer(
        AccessType type,
        uint32_t set,
        uint32_t binding,
        const std::string& memberName,
        const std::string& aliasName = "");

    ~StorageBuffer();

    virtual std::string createShaderStr() noexcept override;

    void createGpuBuffer(vkapi::VkDriver& driver, uint32_t size) noexcept;

    BackendBufferParams
    getBufferParams(vkapi::VkDriver& driver) noexcept override;

private:
    AccessType accessType_;
};

class PushBlock : public BufferBase
{
public:
    PushBlock(const std::string memberName, const std::string& aliasName = "");
    ~PushBlock();

    std::string createShaderStr() noexcept override;

private:
    std::string memberName_;
    std::string aliasName_;
};

} // namespace yave
