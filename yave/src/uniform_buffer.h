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
#include "utility/cstring.h"
#include "vulkan-api/buffer.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/resource_cache.h"

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
        size_t size = 0;
        uint32_t set = 0;
        uint32_t binding = 0;
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
        // the inner array size ( >1 indicates a 2d array)
        uint32_t innerArraySize;
        // the outer array size
        uint32_t outerArraySize;
        // name of the struct if the element type is "Struct"
        std::string structName;
    };

    BufferBase();
    virtual ~BufferBase();

    // Note: if both inner and outer array are > 1 then indicates a 2d array.
    // id inner array == 1 then treated as a 1d array and only the outer array size considered.
    void addElement(
        const std::string& name,
        backend::BufferElementType type,
        void* value = nullptr,
        uint32_t outerArraySize = 1,
        uint32_t innerArraySize = 1,
        const std::string& structName = "") noexcept;

    void updateElement(const std::string& name, void* data) noexcept;

    void* getBlockData() noexcept;

    [[nodiscard]] size_t size() const noexcept { return accumSize_; }

    [[nodiscard]] bool empty() const noexcept { return elements_.empty(); }

    virtual std::string createShaderStr() noexcept { return ""; };

    virtual BackendBufferParams getBufferParams(vkapi::VkDriver& driver) noexcept { return {}; }

protected:
    std::vector<Info> elements_;

    uint8_t* bufferData_;
    size_t currentBufferSize_;
    size_t accumSize_;
};

class UniformBuffer : public BufferBase
{
public:
    UniformBuffer(uint32_t set, uint32_t binding, std::string memberName, std::string aliasName);
    ~UniformBuffer() override;

    std::string createShaderStr() noexcept override;

    void createGpuBuffer(vkapi::VkDriver& driver, size_t size) noexcept;

    void createGpuBuffer(vkapi::VkDriver& driver) noexcept;

    void mapGpuBuffer(void* data) noexcept;

    void mapGpuBuffer(void* data, size_t size) noexcept;

    BackendBufferParams getBufferParams(vkapi::VkDriver& driver) noexcept override;

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
        ReadWrite,
    };

    StorageBuffer(
        AccessType type,
        uint32_t set,
        uint32_t binding,
        const std::string& memberName,
        const std::string& aliasName);

    ~StorageBuffer() override;

    std::string createShaderStr() noexcept override;

    void createGpuBuffer(vkapi::VkDriver& driver, uint32_t size) noexcept;

    void createGpuBuffer(vkapi::VkDriver& driver) noexcept;

    void copyFrom(const StorageBuffer& other) noexcept;

    BackendBufferParams getBufferParams(vkapi::VkDriver& driver) noexcept override;

private:
    AccessType accessType_;
};

class PushBlock : public BufferBase
{
public:
    PushBlock(std::string memberName, std::string aliasName);
    ~PushBlock() override;

    std::string createShaderStr() noexcept override;

private:
    std::string memberName_;
    std::string aliasName_;
};

} // namespace yave
