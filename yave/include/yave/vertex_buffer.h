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

#include <cstdint>

namespace yave
{
class Engine;

class VertexBuffer :public YaveApi
{
public:
    enum class BindingType
    {
        Position,
        Uv,
        Normal,
        Colour,
        Weight,
        Bones,
        // this are not yet implemented - just here as a reminder!
        Custom0,
        Custom1,
        Custom2,
        __SENTINEL__
    };

    void addAttribute(BindingType bindType, backend::BufferElementType attrType);

    void build(Engine* engine, uint32_t vertexCount, void* vertexData);

protected:
    VertexBuffer() = default;
    ~VertexBuffer() = default;
};

} // namespace yave
