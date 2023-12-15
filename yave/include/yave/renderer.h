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

#include "yave_api.h"

#include <backend/enums.h>
#include <utility/colour.h>
#include <utility/cstring.h>
#include <utility/timer.h>
#include <vulkan-api/renderpass.h>

#include <memory>

namespace yave
{
class Engine;
class Scene;
class Texture;

class RenderTarget
{
public:
    static const int MaxAttachCount = vkapi::RenderTarget::MaxAttachmentCount;
    static const int DepthAttachIdx = vkapi::RenderTarget::DepthIndex;

    struct Attachment
    {
        Texture* texture = nullptr;
        uint8_t mipLevel = 0;
        uint8_t layer = 0;
    };

    RenderTarget() = default;

    void setColourTexture(Texture* tex, uint8_t attachIndx);

    void setDepthTexture(Texture* tex);

    void setMipLevel(uint8_t level, uint8_t attachIdx);

    void setLayer(uint8_t layer, uint8_t attachIdx);

    void setLoadFlags(backend::LoadClearFlags flags, uint8_t attachIdx);

    void setStoreFlags(backend::StoreClearFlags flags, uint8_t attachIdx);

    void build(Engine* engine, const util::CString& name, bool multiView = false);

    [[nodiscard]] vkapi::RenderTargetHandle getHandle() const { return handle_; }

    backend::LoadClearFlags* getLoadFlags() noexcept { return loadFlags_; }
    backend::StoreClearFlags* getStoreFlags() noexcept { return storeFlags_; }

    [[nodiscard]] uint32_t getWidth() const { return width_; }
    [[nodiscard]] uint32_t getHeight() const { return height_; }

private:
    Attachment attachments_[MaxAttachCount] = {};
    uint8_t samples_ = 1;
    vkapi::RenderTargetHandle handle_;
    util::Colour4 clearCol_ {0.0f};
    backend::LoadClearFlags loadFlags_[MaxAttachCount];
    backend::StoreClearFlags storeFlags_[MaxAttachCount];
    uint32_t width_;
    uint32_t height_;
};

class Renderer : public YaveApi
{
public:

    void beginFrame();

    void endFrame();

    void render(
        Engine* engine,
        Scene* scene,
        float dt,
        util::Timer<NanoSeconds>& timer,
        bool clearSwap = true);

    void renderSingleScene(Engine* engine, Scene* scene, RenderTarget& rTarget);

protected:

    Renderer() = default;
    ~Renderer() = default;
};

} // namespace yave
