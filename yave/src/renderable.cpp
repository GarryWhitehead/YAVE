#include "private/renderable.h"
#include "private/render_primitive.h"

namespace yave
{

void Renderable::setPrimitive(RenderPrimitive* prim, size_t idx)
{
    static_cast<IRenderable*>(this)->setPrimitive(static_cast<IRenderPrimitive*>(prim), idx);
}

void Renderable::setPrimitiveCount(size_t count) noexcept
{
    static_cast<IRenderable*>(this)->setPrimitiveCount(count);
}

void Renderable::skipVisibilityChecks()
{
    static_cast<IRenderable*>(this)->skipVisibilityChecks();
}

}