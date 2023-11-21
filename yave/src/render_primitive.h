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

#include "aabox.h"

#include <backend/convert_to_vk.h>
#include <backend/enums.h>
#include <model_parser/gltf/model_mesh.h>
#include <vulkan-api/common.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/pipeline_cache.h>
#include <yave/engine.h>
#include <yave/render_primitive.h>

namespace yave
{
class VkDriver;
class IMaterial;
class IVertexBuffer;
class IIndexBuffer;

class IRenderPrimitive : public RenderPrimitive
{
public:
    enum class Variants
    {
        HasSkin,
        HasJoints,
        __SENTINEL__
    };

    struct MeshDrawData
    {
        size_t indexCount = 0;
        size_t indexPrimitiveOffset = 0;
        size_t vertexCount = 0;
    };

    IRenderPrimitive();
    ~IRenderPrimitive() override;

    void shutDown(vkapi::VkDriver& driver) noexcept;

    void addMeshDrawDataI(size_t indexCount, size_t offset, size_t vertexCount);

    vkapi::VDefinitions createVertexAttributeVariants();

    // =================== setters ============================

    void setTopologyI(backend::PrimitiveTopology topo);
    void setVertexBufferI(IVertexBuffer* vBuffer) noexcept;
    void setIndexBufferI(IIndexBuffer* iBuffer) noexcept;
    void setMaterialI(IMaterial* mat) noexcept;

    // =================== getters ============================

    [[nodiscard]] vk::PrimitiveTopology getTopology() const noexcept { return topology_; }
    [[nodiscard]] bool getPrimRestartState() const noexcept { return primitiveRestart_; }
    [[nodiscard]] const AABBox& getDimensions() const noexcept { return box_; }
    [[nodiscard]] const MeshDrawData& getDrawData() const noexcept { return drawData_; }
    util::BitSetEnum<Variants>& getVariantBits() noexcept { return variants_; }

    IVertexBuffer* getVertexBuffer() noexcept { return vertBuffer_; }
    IIndexBuffer* getIndexBuffer() noexcept { return indexBuffer_; }
    IMaterial* getMaterial() noexcept { return material_; }

    // =================== client api =======================

    void addMeshDrawData(size_t indexCount, size_t offset, size_t vertexCount) override;
    void setTopology(Topology topo) override;
    void enablePrimitiveRestart() noexcept override;
    void setVertexBuffer(VertexBuffer* vBuffer) override;
    void setIndexBuffer(IndexBuffer* iBuffer) override;
    void setMaterial(Material* mat) override;

private:
    vk::PrimitiveTopology topology_;

    bool primitiveRestart_;

    // The min and max extents of the primitive
    AABBox box_;

    util::BitSetEnum<Variants> variants_;

    IVertexBuffer* vertBuffer_;
    IIndexBuffer* indexBuffer_;

    // index offsets
    MeshDrawData drawData_;

    // the material for this primitive. This isn't owned by the
    // primitive - this is the "property" of the renderable manager.
    IMaterial* material_;
};

} // namespace yave
