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

#include "resource_cache.h"

#include "buffer.h"
#include "context.h"
#include "driver.h"
#include "garbage_collector.h"
#include "image.h"

namespace vkapi
{

ResourceCache::ResourceCache(VkContext& context, VkDriver& driver)
    : driver_(driver), context_(context)
{
}

TextureHandle ResourceCache::createTexture2d(
    vk::Format format,
    const uint32_t width,
    const uint32_t height,
    const uint8_t mipLevels,
    vk::ImageUsageFlags usageFlags,
    const uint8_t faceCount,
    const uint8_t arrayCount)
{
    auto tex = new Texture(context_);
    TextureHandle handle {tex};
    tex->createTexture2d(
        driver_, format, width, height, mipLevels, faceCount, arrayCount, usageFlags);
    textures_.insert(tex);
    return handle;
}

TextureHandle ResourceCache::createTexture2d(
    vk::Format format, const uint32_t width, const uint32_t height, vk::Image image)
{
    auto tex = new Texture(context_);
    TextureHandle handle {tex};
    tex->createTexture2d(driver_, format, width, height, image);
    textures_.insert(tex);
    return handle;
}

BufferHandle ResourceCache::createUbo(const size_t size, VkBufferUsageFlags usage)
{
    auto buffer = new Buffer;
    BufferHandle handle {buffer};
    buffer->prepare(driver_.vmaAlloc(), static_cast<VkDeviceSize>(size), usage);
    buffers_.insert(buffer);
    return handle;
}

void ResourceCache::deleteUbo(BufferHandle& handle)
{
    Buffer* buffer = handle.getResource();
    // If the handle is invalidated we assume that the resource has already been
    // deleted and allow this to fail silently.
    if (!buffer)
    {
        return;
    }
    auto iter = buffers_.find(const_cast<Buffer*>(buffer));
    if (iter != buffers_.end())
    {
        buffer->framesUntilGc_ = Commands::MaxCommandBufferSize;
        bufferGc_.insert(buffer);
        buffers_.erase(iter);
    }
}

void ResourceCache::deleteTexture(TextureHandle& handle)
{
    Texture* tex = handle.getResource();
    // If the handle is invalidated we assume that the resource has already been
    // deleted and allow this to fail silently.
    if (!tex)
    {
        return;
    }
    auto iter = textures_.find(const_cast<Texture*>(tex));
    if (iter != textures_.end())
    {
        tex->framesUntilGc_ = Commands::MaxCommandBufferSize;
        textureGc_.insert(tex);
        textures_.erase(iter);
    }
}

void ResourceCache::garbageCollection() noexcept
{
    std::unordered_set<Texture*> newTextureGc;
    for (auto* tex : textureGc_)
    {
        if (!--tex->framesUntilGc_)
        {
            tex->destroy();
        }
        else
        {
            newTextureGc.insert(tex);
        }
    }
    textureGc_.swap(newTextureGc);

    std::unordered_set<Buffer*> newBufferGc;
    for (auto* buffer : bufferGc_)
    {
        if (!--buffer->framesUntilGc_)
        {
            context_.device().destroy(buffer->get(), nullptr);
        }
        else
        {
            newBufferGc.insert(buffer);
        }
    }
    bufferGc_.swap(newBufferGc);
}

void ResourceCache::clear() noexcept
{
    for (auto* tex : textureGc_)
    {
        tex->destroy();
    }
    for (auto* buffer : bufferGc_)
    {
        context_.device().destroy(buffer->get(), nullptr);
    }
    textures_.clear();
}

} // namespace vkapi
