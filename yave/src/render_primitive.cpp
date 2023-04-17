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

#include "render_primitive.h"

#include "backend/enums.h"
#include "engine.h"
#include "material.h"
#include "utility/assertion.h"
#include "vertex_buffer.h"
#include "yave/engine.h"
#include "yave/render_primitive.h"

#include <spdlog/spdlog.h>

#include <tuple>

namespace yave
{

IRenderPrimitive::IRenderPrimitive()
    : topology_(vk::PrimitiveTopology::eTriangleList), primitiveRestart_(false)
{
}
IRenderPrimitive::~IRenderPrimitive() {}

void IRenderPrimitive::shutDown(vkapi::VkDriver& driver) noexcept {}

vkapi::VDefinitions IRenderPrimitive::createVertexAttributeVariants()
{
    vkapi::VDefinitions map;
    auto bits = vertBuffer_->getAtrributeBits();

    ASSERT_FATAL(
        bits.testBit(VertexBuffer::BindingType::Position),
        "It is essential that the position vertex input attribute is set.");

    if (bits.testBit(VertexBuffer::BindingType::Normal))
    {
        map.emplace("HAS_NORMAL_ATTR_INPUT", 1);
    }
    if (bits.testBit(VertexBuffer::BindingType::Uv))
    {
        map.emplace("HAS_UV_ATTR_INPUT", 1);
    }
    if (bits.testBit(VertexBuffer::BindingType::Colour))
    {
        map.emplace("HAS_COLOUR_ATTR_INPUT", 1);
    }
    if (bits.testBit(VertexBuffer::BindingType::Weight))
    {
        map.emplace("HAS_WEIGHT_ATTR_INPUT", 1);
    }
    if (bits.testBit(VertexBuffer::BindingType::Bones))
    {
        map.emplace("HAS_BONES_ATTR_INPUT", 1);
    }
    return map;
}

void IRenderPrimitive::addMeshDrawDataI(size_t indexCount, size_t offset)
{
    ASSERT_LOG(indexCount > 0);
    drawData_ = {indexCount, offset};
}

void IRenderPrimitive::setTopologyI(backend::PrimitiveTopology topo)
{
    topology_ = backend::primitiveTopologyToVk(topo);
}

void IRenderPrimitive::setVertexBufferI(IVertexBuffer* vBuffer) noexcept { vertBuffer_ = vBuffer; }

void IRenderPrimitive::setIndexBufferI(IIndexBuffer* iBuffer) noexcept { indexBuffer_ = iBuffer; }

void IRenderPrimitive::setMaterialI(IMaterial* mat) noexcept { material_ = mat; }

// =============== client api virtual =======================

RenderPrimitive::RenderPrimitive() {}
RenderPrimitive::~RenderPrimitive() {}

void IRenderPrimitive::addMeshDrawData(size_t indexCount, size_t offset)
{
    addMeshDrawDataI(indexCount, offset);
}

void IRenderPrimitive::setTopology(Topology topo) { setTopologyI(topo); }

void IRenderPrimitive::enablePrimitiveRestart() noexcept { primitiveRestart_ = true; }

void IRenderPrimitive::setVertexBuffer(VertexBuffer* vBuffer)
{
    setVertexBufferI(reinterpret_cast<IVertexBuffer*>(vBuffer));
}

void IRenderPrimitive::setIndexBuffer(IndexBuffer* iBuffer)
{
    setIndexBufferI(reinterpret_cast<IIndexBuffer*>(iBuffer));
}

void IRenderPrimitive::setMaterial(Material* mat)
{
    setMaterialI(reinterpret_cast<IMaterial*>(mat));
}

} // namespace yave
