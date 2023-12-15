#include "private/vertex_buffer.h"

#include "private/engine.h"

namespace yave
{

void VertexBuffer::addAttribute(BindingType bindType, backend::BufferElementType attrType)
{
    static_cast<IVertexBuffer*>(this)->addAttribute(static_cast<uint32_t>(bindType), attrType);
}

void VertexBuffer::build(Engine* engine, uint32_t vertexCount, void* vertexData)
{
    static_cast<IVertexBuffer*>(this)->build(
        static_cast<IEngine*>(engine)->driver(), vertexCount, vertexData);
}

} // namespace yave