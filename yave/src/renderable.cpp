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

#include "renderable.h"

namespace yave
{

IRenderable::IRenderable()
    : program_(nullptr), meshDynamicOffset_(0), skinDynamicOffset_(IRenderable::UNINITIALISED)
{
}

IRenderable::~IRenderable() {}

void IRenderable::shutDown(vkapi::VkDriver& driver) noexcept {}

uint32_t IRenderable::getMeshDynamicOffset() const noexcept { return meshDynamicOffset_; }

uint32_t IRenderable::getSkinDynamicOffset() const noexcept { return skinDynamicOffset_; }

void IRenderable::skipVisibilityChecksI() { visibility_.setBit(IRenderable::Visible::Ignore); }

void IRenderable::setPrimitiveI(IRenderPrimitive* prim, size_t idx) noexcept
{
    ASSERT_FATAL(
        idx < primitives_.size(),
        "Primitive index of %d is greater than the allocated primitive count "
        "of %d",
        idx,
        primitives_.size());
    primitives_[idx] = prim;
}

void IRenderable::setPrimitiveCountI(size_t count) noexcept
{
    ASSERT_LOG(count > 0);
    primitives_.resize(count);
}

IRenderPrimitive* IRenderable::getRenderPrimitive(size_t idx) noexcept
{
    ASSERT_FATAL(idx < primitives_.size(), "Primitive handle is out of bounds.");
    return primitives_[idx];
}

std::vector<IRenderPrimitive*>& IRenderable::getAllRenderPrimitives() noexcept
{
    ASSERT_LOG(!primitives_.empty());
    return primitives_;
}

// ========================== client api ============================

Renderable::Renderable() {}
Renderable::~Renderable() {}

void IRenderable::setPrimitive(RenderPrimitive* prim, size_t idx)
{
    setPrimitiveI(reinterpret_cast<IRenderPrimitive*>(prim), idx);
}

void IRenderable::setPrimitiveCount(size_t count) noexcept { setPrimitiveCountI(count); }

void IRenderable::skipVisibilityChecks() { skipVisibilityChecksI(); }

} // namespace yave
