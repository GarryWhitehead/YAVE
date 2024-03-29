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

#include "backend/enums.h"
#include "yave_api.h"

#include <model_parser/gltf/model_mesh.h>

namespace yave
{
class Engine;
class Material;

class RenderPrimitive : public YaveApi
{
public:
    using Topology = backend::PrimitiveTopology;

    void addMeshDrawData(size_t indexCount, size_t offset, size_t vertexCount);

    void setTopology(Topology topo);

    void enablePrimitiveRestart() noexcept;

    void setVertexBuffer(VertexBuffer* vBuffer);

    void setIndexBuffer(IndexBuffer* iBuffer);

    void setMaterial(Material* mat);

protected:
    RenderPrimitive() = default;
    ~RenderPrimitive() = default;
};

} // namespace yave
