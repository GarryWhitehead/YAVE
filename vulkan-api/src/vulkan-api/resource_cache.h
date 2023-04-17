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
#include "texture.h"
#include "unordered_map"
#include "utility/assertion.h"
#include "utility/compiler.h"
#include "utility/cstring.h"
#include "utility/handle.h"
#include "utility/murmurhash.h"

#include <memory>
#include <unordered_set>
#include <vector>

namespace vkapi
{
// forward declerations
class Buffer;
class VkDriver;
class VkContext;
class GarbageCollector;

template <typename T>
class ResourceHandle
{
public:
    explicit operator bool() const noexcept { return resource_ != nullptr; }

    bool operator==(const ResourceHandle& rhs) const { return resource_ == rhs.getResource(); }
    bool operator!=(const ResourceHandle& rhs) const { return resource_ != rhs.getResource(); }

    ResourceHandle() : resource_(nullptr) {}
    ResourceHandle(T* key) : resource_(key) {}

    ~ResourceHandle() {}

    ResourceHandle(const ResourceHandle&) = default;
    ResourceHandle& operator=(const ResourceHandle&) = default;
    ResourceHandle(ResourceHandle&&) = default;
    ResourceHandle& operator=(ResourceHandle&&) = default;

    const T* getResource() const noexcept { return resource_; }

    T* getResource() noexcept { return resource_; }

    void invalidate() noexcept { resource_ = nullptr; }

private:
    T* resource_ = nullptr;
};

using TextureHandle = ResourceHandle<Texture>;
using BufferHandle = ResourceHandle<Buffer>;

class ResourceCache
{
public:
    ResourceCache(VkContext& context_, VkDriver& driver);

    TextureHandle createTexture2d(
        vk::Format format,
        const uint32_t width,
        const uint32_t height,
        const uint8_t mipLevels,
        vk::ImageUsageFlags usageFlags,
        const uint8_t faceCount = 1,
        const uint8_t arrayCount = 1);

    TextureHandle createTexture2d(
        vk::Format format, const uint32_t width, const uint32_t height, vk::Image image);

    BufferHandle createUbo(const size_t size, VkBufferUsageFlags usage);

    void deleteUbo(BufferHandle& handle, GarbageCollector& gc);

    void deleteTexture(TextureHandle& handle, GarbageCollector& gc);

    void garbageCollection() noexcept;

    void clear() noexcept;

    using TextureMap = std::unordered_set<Texture*>;
    using BufferMap = std::unordered_set<Buffer*>;

private:
    VkDriver& driver_;
    VkContext& context_;

    // texture/buffer resources
    TextureMap textures_;
    BufferMap buffers_;

    std::unordered_set<Texture*> textureGc_;
    std::unordered_set<Buffer*> bufferGc_;
};

} // namespace vkapi
