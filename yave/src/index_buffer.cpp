#include "private/index_buffer.h"
#include "private/engine.h"

namespace yave
{

void IndexBuffer::build(
    Engine* engine,
    uint32_t indicesCount,
    void* indicesData,
    backend::IndexBufferType type)
{
    static_cast<IIndexBuffer*>(this)->build(static_cast<IEngine*>(engine)->driver(), indicesCount, indicesData, type);
}

}