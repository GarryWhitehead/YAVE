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

#include "models.h"

#include "yave/engine.h"
#include "yave/index_buffer.h"
#include "yave/render_primitive.h"
#include "yave/vertex_buffer.h"

#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace yave
{

float* generateInterleavedData(
    mathfu::vec3* positions, mathfu::vec2* texCoords, mathfu::vec3* normals, size_t vertexCount)
{
    if (!positions)
    {
        return nullptr;
    }

    const size_t totalBufferSize = 3 * vertexCount + 2 * vertexCount + 3 * vertexCount;
    float* buffer = new float[totalBufferSize];

    float* bufferPtr = buffer;
    mathfu::vec3* posPtr = positions;
    mathfu::vec2* texPtr = texCoords;
    mathfu::vec3* normPtr = normals;

    for (uint32_t idx = 0; idx < vertexCount; ++idx)
    {
        memcpy(bufferPtr, posPtr, sizeof(mathfu::vec3));
        bufferPtr += 3;
        ++posPtr;
        if (texCoords)
        {
            memcpy(bufferPtr, texPtr, sizeof(mathfu::vec2));
            bufferPtr += 2;
            ++texPtr;
        }
        if (normals)
        {
            memcpy(bufferPtr, normPtr, sizeof(mathfu::vec3));
            bufferPtr += 3;
            ++normPtr;
        }
    }

    return buffer;
}

void generateQuadMesh(
    yave::Engine* engine,
    const float size,
    yave::VertexBuffer* vBuffer,
    yave::IndexBuffer* iBuffer,
    yave::RenderPrimitive* prim)
{
    static constexpr int vertexCount = 4;
    static constexpr int totalVertexCount = vertexCount * 3;

    mathfu::vec3 positions[] = {
        mathfu::vec3 {size, size, 0.0f},
        mathfu::vec3 {-size, size, 0.0f},
        mathfu::vec3 {-size, -size, 0.0},
        mathfu::vec3 {size, -size, 0.0f}};

    mathfu::vec2 texCoords[] = {
        mathfu::vec2 {1.0f, 1.0f},
        mathfu::vec2 {0.0f, 1.0f},
        mathfu::vec2 {0.0f, 0.0f},
        mathfu::vec2 {1.0f, 0.0f}};

    mathfu::vec3 normals[] = {
        mathfu::vec3 {0.0f, 0.0f, -1.0f},
        mathfu::vec3 {0.0f, 0.0f, -1.0f},
        mathfu::vec3 {0.0f, 0.0f, -1.0f},
        mathfu::vec3 {0.0f, 0.0f, -1.0f}};

    // interleave data
    float* buffer = generateInterleavedData(positions, texCoords, normals, vertexCount);

    // quad made up of two triangles
    const std::vector<int> indices = {0, 1, 2, 2, 3, 0};

    vBuffer->addAttribute(
        yave::VertexBuffer::BindingType::Position, backend::BufferElementType::Float3);
    vBuffer->addAttribute(yave::VertexBuffer::BindingType::Uv, backend::BufferElementType::Float2);
    vBuffer->addAttribute(
        yave::VertexBuffer::BindingType::Normal, backend::BufferElementType::Float3);
    vBuffer->build(engine, totalVertexCount, (void*)buffer);

    iBuffer->build(
        engine,
        static_cast<uint32_t>(indices.size()),
        (void*)indices.data(),
        backend::IndexBufferType::Uint32);

    prim->addMeshDrawData(indices.size(), 0, 0);

    // we have uploaded the data to the device so no longer need the allocated
    // space
    delete[] buffer;
}

void generateSphereMesh(
    yave::Engine* engine,
    const uint32_t density,
    yave::VertexBuffer* vBuffer,
    yave::IndexBuffer* iBuffer,
    yave::RenderPrimitive* prim)
{
    std::vector<mathfu::vec3> positions;
    std::vector<mathfu::vec2> texCoords;

    positions.reserve(6 * density * density);
    texCoords.reserve(6 * density * density);

    std::vector<int> indices;
    indices.reserve(2 * density * density * 6);

    static const mathfu::vec3 basePosition[6] = {
        mathfu::vec3(1.0f, 1.0f, 1.0f),
        mathfu::vec3(-1.0f, 1.0f, -1.0f),
        mathfu::vec3(-1.0f, 1.0f, -1.0f),
        mathfu::vec3(-1.0f, -1.0f, +1.0f),
        mathfu::vec3(-1.0f, 1.0f, +1.0f),
        mathfu::vec3(+1.0f, 1.0f, -1.0f),
    };

    static const mathfu::vec3 dx[6] = {
        mathfu::vec3(0.0f, 0.0f, -2.0f),
        mathfu::vec3(0.0f, 0.0f, +2.0f),
        mathfu::vec3(2.0f, 0.0f, 0.0f),
        mathfu::vec3(2.0f, 0.0f, 0.0f),
        mathfu::vec3(2.0f, 0.0f, 0.0f),
        mathfu::vec3(-2.0f, 0.0f, 0.0f),
    };

    static const mathfu::vec3 dy[6] = {
        mathfu::vec3(0.0f, -2.0f, 0.0f),
        mathfu::vec3(0.0f, -2.0f, 0.0f),
        mathfu::vec3(0.0f, 0.0f, +2.0f),
        mathfu::vec3(0.0f, 0.0f, -2.0f),
        mathfu::vec3(0.0f, -2.0f, 0.0f),
        mathfu::vec3(0.0f, -2.0f, 0.0f),
    };

    const float densityMod = 1.0f / static_cast<float>(density - 1);

    for (uint32_t face = 0; face < 6; ++face)
    {
        uint32_t indexOffset = face * density * density;

        for (uint32_t y = 0; y < density; ++y)
        {
            for (uint32_t x = 0; x < density; ++x)
            {
                mathfu::vec2 uv {densityMod * x, densityMod * y};
                texCoords.push_back(uv);
                mathfu::vec3 pos =
                    mathfu::vec3 {basePosition[face] + dx[face] * uv.x + dy[face] * uv.y};
                mathfu::vec3 normPos = mathfu::NormalizedHelper(pos);
                positions.emplace_back(normPos);
            }
        }

        uint32_t strips = density - 1;
        for (uint32_t y = 0; y < strips; ++y)
        {
            uint32_t baseIndex = indexOffset + y * density;
            for (uint32_t x = 0; x < density; ++x)
            {
                indices.emplace_back(baseIndex + x);
                indices.emplace_back(baseIndex + x + density);
            }
            indices.emplace_back(UINT32_MAX);
        }
    }

    // interleave data
    const uint32_t vertexCount = static_cast<uint32_t>(positions.size());
    float* buffer =
        generateInterleavedData(positions.data(), texCoords.data(), nullptr, vertexCount);

    vBuffer->addAttribute(
        yave::VertexBuffer::BindingType::Position, backend::BufferElementType::Float3);
    vBuffer->addAttribute(yave::VertexBuffer::BindingType::Uv, backend::BufferElementType::Float2);
    vBuffer->build(engine, vertexCount * 20, (void*)buffer);

    iBuffer->build(
        engine,
        static_cast<uint32_t>(indices.size()),
        indices.data(),
        backend::IndexBufferType::Uint32);

    // using primitive rerstart here so when the 0xFFFF value is read, the most
    // recent indices values will be discarded
    prim->addMeshDrawData(indices.size(), 0, 0);
    prim->setTopology(backend::PrimitiveTopology::TriangleStrip);
    prim->enablePrimitiveRestart();

    // we have uploaded the data to the device so no longer need the allocated
    // space
    delete[] buffer;
}

void generateCapsuleMesh(
    yave::Engine* engine,
    const uint32_t density,
    const float height,
    const float radius,
    yave::VertexBuffer* vBuffer,
    yave::IndexBuffer* iBuffer,
    yave::RenderPrimitive* prim)
{
    const uint32_t innerSize = density / 2;
    const float halfHeight = 0.5f * height - 0.5f * radius;
    float invDensity = 1.0f / static_cast<float>(density);

    std::vector<mathfu::vec3> positions(2 * innerSize * density + 2);
    std::vector<mathfu::vec3> normals(2 * innerSize * density + 2);
    std::vector<mathfu::vec2> texCoords(2 * innerSize * density + 2);

    std::vector<int> indices;

    // Top center
    positions[0] = mathfu::vec3 {0.0f, halfHeight + radius, 0.0f};
    normals[0] = mathfu::vec3 {0.0f, 1.0f, 0.0f};

    // Bottom center
    positions[1] = mathfu::vec3 {0.0f, -halfHeight + radius, 0.0f};
    normals[1] = mathfu::vec3 {0.0f, -1.0f, 0.0f};

    // Top rings
    for (uint32_t i = 0; i < innerSize; ++i)
    {
        float w = float(i + 1) / static_cast<float>(innerSize);
        float extraHeight = radius * sqrtf(1.0f - w * w);
        uint32_t offset = i * density + 2;

        for (uint32_t j = 0; j < density; ++j)
        {
            float rad = 2.0f * static_cast<float>(M_PI) * (j + 0.5f) * invDensity;
            positions[offset + j] = mathfu::vec3 {
                w * radius * std::cos(rad), halfHeight + extraHeight, -w * radius * std::sin(rad)};

            mathfu::vec3 norm {positions[offset + j].x, extraHeight, positions[offset + j].z};
            normals[offset + j] = mathfu::NormalizedHelper(norm);
        }
    }

    // Bottom rings
    for (uint32_t i = 0; i < innerSize; ++i)
    {
        float w = static_cast<float>(innerSize - i) / static_cast<float>(innerSize);
        float extraHeight = radius * std::sqrt(1.0f - w * w);
        uint32_t offset = (i + innerSize) * density + 2;

        for (uint32_t j = 0; j < density; ++j)
        {
            float rad = 2.0f * static_cast<float>(M_PI) * (j + 0.5f) * invDensity;
            positions[offset + j] = mathfu::vec3 {
                w * radius * std::cos(rad), -halfHeight - extraHeight, -w * radius * std::sin(rad)};

            mathfu::vec3 norm {positions[offset + j].x, -extraHeight, positions[offset + j].z};
            normals[offset + j] = mathfu::NormalizedHelper(norm);
        }
    }

    // Link up top vertices.
    for (uint32_t i = 0; i < density; ++i)
    {
        indices.emplace_back(0);
        indices.emplace_back(i + 2);
        indices.emplace_back(((i + 1) % density) + 2);
    }

    // Link up bottom vertices.
    for (uint32_t i = 0; i < density; i++)
    {
        indices.emplace_back(1);
        indices.emplace_back((2 * innerSize - 1) * density + ((i + 1) % density) + 2);
        indices.emplace_back((2 * innerSize - 1) * density + i + 2);
    }

    // Link up rings.
    for (uint32_t i = 0; i < 2 * innerSize - 1; ++i)
    {
        uint32_t offset0 = i * density + 2;
        uint32_t offset1 = offset0 + density;

        for (uint32_t j = 0; j < density; ++j)
        {
            indices.emplace_back(offset0 + j);
            indices.emplace_back(offset1 + j);
            indices.emplace_back(offset0 + ((j + 1) % density));
            indices.emplace_back(offset1 + ((j + 1) % density));
            indices.emplace_back(offset0 + ((j + 1) % density));
            indices.emplace_back(offset1 + j);
        }
    }

    // interleave data
    const uint32_t vertexCount = static_cast<uint32_t>(positions.size());
    float* buffer =
        generateInterleavedData(positions.data(), texCoords.data(), normals.data(), vertexCount);

    vBuffer->addAttribute(
        yave::VertexBuffer::BindingType::Position, backend::BufferElementType::Float3);
    vBuffer->addAttribute(yave::VertexBuffer::BindingType::Uv, backend::BufferElementType::Float2);
    vBuffer->addAttribute(
        yave::VertexBuffer::BindingType::Normal, backend::BufferElementType::Float3);

    vBuffer->build(engine, vertexCount * 32, (void*)buffer);

    iBuffer->build(
        engine,
        static_cast<uint32_t>(indices.size()),
        (void*)indices.data(),
        backend::IndexBufferType::Uint32);

    prim->addMeshDrawData(indices.size(), 0, 0);

    // we have uploaded the data to the device so no longer need the allocated
    // space
    delete[] buffer;
}

void generateCubeMesh(
    yave::Engine* engine,
    const mathfu::vec3& size,
    yave::VertexBuffer* vBuffer,
    yave::IndexBuffer* iBuffer,
    yave::RenderPrimitive* prim)
{
    const float x = size.x / 2.0f;
    const float y = size.y / 2.0f;
    const float z = size.z / 2.0f;

    // cube vertices
    const mathfu::vec3 v0 {+x, +y, +z};
    const mathfu::vec3 v1 {-x, +y, +z};
    const mathfu::vec3 v2 {-x, -y, +z};
    const mathfu::vec3 v3 {+x, -y, +z};
    const mathfu::vec3 v4 {+x, +y, -z};
    const mathfu::vec3 v5 {-x, +y, -z};
    const mathfu::vec3 v6 {-x, -y, -z};
    const mathfu::vec3 v7 {+x, -y, -z};

    // cube uvs
    const mathfu::vec2 uv0 {1.0f, 1.0f};
    const mathfu::vec2 uv1 {0.0f, 1.0f};
    const mathfu::vec2 uv2 {0.0f, 0.0f};
    const mathfu::vec2 uv3 {1.0f, 0.0f};
    const mathfu::vec2 uv4 {0.0f, 1.0f};
    const mathfu::vec2 uv5 {1.0f, 1.0f};
    const mathfu::vec2 uv6 {0.0f, 0.0f};
    const mathfu::vec2 uv7 {1.0f, 0.0f};


    mathfu::vec3 positions[] = {v1, v2, v3, v3, v0, v1, v2, v6, v7, v7, v3, v2,
                                v6, v5, v4, v4, v7, v6, v5, v1, v0, v0, v4, v5,
                                v0, v3, v7, v7, v4, v0, v5, v6, v2, v2, v1, v5};

    mathfu::vec2 texCoords[] = {uv1, uv2, uv3, uv3, uv0, uv1, uv2, uv6, uv7, uv7, uv3, uv2,
                                uv6, uv5, uv4, uv4, uv7, uv6, uv5, uv1, uv0, uv0, uv4, uv5,
                                uv0, uv3, uv7, uv7, uv4, uv0, uv5, uv6, uv2, uv2, uv1, uv5};

    mathfu::vec3 normalsPerFace[] = {
        mathfu::vec3 {0.0f, 0.0f, +1.0f},
        mathfu::vec3 {-1.0f, 0.0f, 0.0f},
        mathfu::vec3 {0.0f, 0.0f, -1.0f},
        mathfu::vec3 {+1.0f, 0.0f, 0.0f},
        mathfu::vec3 {0.0f, -1.0f, 0.0f},
        mathfu::vec3 {0.0f, +1.0f, 0.0f}};

    // cube indices
    static const std::vector<uint32_t> indices {// front
                                                0,
                                                1,
                                                2,
                                                3,
                                                4,
                                                5,
                                                // right side
                                                6,
                                                7,
                                                8,
                                                9,
                                                10,
                                                11,
                                                // back
                                                12,
                                                13,
                                                14,
                                                15,
                                                16,
                                                17,
                                                // left side
                                                18,
                                                19,
                                                20,
                                                21,
                                                22,
                                                23,
                                                // bottom
                                                24,
                                                25,
                                                26,
                                                27,
                                                28,
                                                29,
                                                // top
                                                30,
                                                31,
                                                32,
                                                33,
                                                34,
                                                35};


    // sort the normal - per face
    std::vector<mathfu::vec3> normals(36);
    uint32_t vertexBase = 0;
    for (uint32_t i = 0; i < 6; ++i)
    {
        for (uint32_t j = 0; j < 6; ++j)
        {
            normals[vertexBase + j] = normalsPerFace[i];
        }
        vertexBase += 6;
    }

    // interleave data
    const uint32_t vertexCount = static_cast<uint32_t>(normals.size());
    float* buffer = generateInterleavedData(positions, texCoords, normals.data(), vertexCount);

    vBuffer->addAttribute(
        yave::VertexBuffer::BindingType::Position, backend::BufferElementType::Float3);
    vBuffer->addAttribute(yave::VertexBuffer::BindingType::Uv, backend::BufferElementType::Float2);
    vBuffer->addAttribute(
        yave::VertexBuffer::BindingType::Normal, backend::BufferElementType::Float3);
    vBuffer->build(engine, vertexCount * 32, (void*)buffer);

    iBuffer->build(
        engine,
        static_cast<uint32_t>(indices.size()),
        (void*)indices.data(),
        backend::IndexBufferType::Uint32);

    prim->addMeshDrawData(indices.size(), 0, 0);

    // we have uploaded the data to the device so no longer need the allocated
    // space
    delete[] buffer;
}


} // namespace yave
