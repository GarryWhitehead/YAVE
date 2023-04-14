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

#include "render_primitive.h"
#include "utility/assertion.h"
#include "utility/bitset_enum.h"
#include "vulkan-api/program_manager.h"
#include "vulkan-api/shader.h"
#include "yave/renderable.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace yave
{

class IRenderable : public Renderable
{
public:

    constexpr static uint32_t UNINITIALISED =
        std::numeric_limits<uint32_t>::max();

    IRenderable();
    ~IRenderable();

    /**
     * Bit flags for specifying the visibilty of renderables
     */
    enum class Visible : uint8_t
    {
        Render,
        Ignore,     // removes this renderable from the culling process
        Shadow,
        __SENTINEL__
    };

    void shutDown(vkapi::VkDriver& driver) noexcept;

    // ================= setters =======================

    void setProgram(vkapi::ShaderProgram* prog) noexcept { program_ = prog; }

    void setPrimitiveI(IRenderPrimitive* prim, size_t count) noexcept;
    void setPrimitiveCountI(size_t count) noexcept;

    void setMeshDynamicOffset(uint32_t offset) { meshDynamicOffset_ = offset; }
    void setSkinDynamicOffset(uint32_t offset) { skinDynamicOffset_ = offset; }

    void skipVisibilityChecksI();

    // ================= getters ========================

    IRenderPrimitive* getRenderPrimitive(size_t idx = 0) noexcept;
    std::vector<IRenderPrimitive*>& getAllRenderPrimitives() noexcept;
    
    util::BitSetEnum<Visible>& getVisibility() { return visibility_; }

    uint32_t getMeshDynamicOffset() const noexcept;
    uint32_t getSkinDynamicOffset() const noexcept;

    // ================ client api ==========================

    void setPrimitiveCount(size_t count) noexcept override;
    void setPrimitive(RenderPrimitive* prim, size_t idx) override;
    void skipVisibilityChecks() override;

    friend class IRenderableManager;

private:

    // visibilty of this renderable and their shadow
    util::BitSetEnum<Visible> visibility_;

    // ============ vulkan backend ========================
    vkapi::ShaderProgram* program_;

    // this is set by the transform manager but held here for convience reasons
    // when drawing
    uint32_t meshDynamicOffset_;
    uint32_t skinDynamicOffset_;

    std::vector<IRenderPrimitive*> primitives_;
};

} // namespace yave
