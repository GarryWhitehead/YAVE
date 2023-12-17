#include "private/render_primitive.h"

#include "private/index_buffer.h"
#include "private/material.h"
#include "private/vertex_buffer.h"

namespace yave
{

void RenderPrimitive::addMeshDrawData(size_t indexCount, size_t offset, size_t vertexCount)
{
    static_cast<IRenderPrimitive*>(this)->addMeshDrawData(indexCount, offset, vertexCount);
}

void RenderPrimitive::setTopology(Topology topo)
{
    static_cast<IRenderPrimitive*>(this)->setTopology(topo);
}

void RenderPrimitive::enablePrimitiveRestart() noexcept
{
    static_cast<IRenderPrimitive*>(this)->enablePrimitiveRestart();
}

void RenderPrimitive::setVertexBuffer(VertexBuffer* vBuffer)
{
    static_cast<IRenderPrimitive*>(this)->setVertexBuffer(static_cast<IVertexBuffer*>(vBuffer));
}

void RenderPrimitive::setIndexBuffer(IndexBuffer* iBuffer)
{
    static_cast<IRenderPrimitive*>(this)->setIndexBuffer(static_cast<IIndexBuffer*>(iBuffer));
}

void RenderPrimitive::setMaterial(Material* mat)
{
    static_cast<IRenderPrimitive*>(this)->setMaterial(static_cast<IMaterial*>(mat));
}

} // namespace yave