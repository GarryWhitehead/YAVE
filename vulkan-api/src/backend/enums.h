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

#include "utility/compiler.h"
#include "utility/murmurhash.h"
#include "vulkan/vulkan.hpp"

#include <type_traits>
#include <cstring>

namespace backend
{

enum class BlendFactor
{
    Zero,
    One,
    SrcColour,
    OneMinusSrcColour,
    DstColour,
    OneMinusDstColour,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColour,
    OneMinusConstantColour,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate
};

enum class BlendOp
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class BlendFactorPresets
{
    Translucent
};

enum class SamplerAddressMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge
};

enum class SamplerFilter
{
    Nearest,
    Linear,
    Cubic
};

enum class CullMode
{
    Back,
    Front,
    None
};

enum class CompareOp
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

enum class PrimitiveTopology
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan,
    LineListWithAdjacency,
    LineStripWithAdjacency,
    TriangleListWithAdjacency,
    TriangleStripWithAdjacency,
    PatchList
};

enum class BufferElementType
{
    Int,
    Int2,
    Int3,
    Int4,
    Float,
    Float2,
    Float3,
    Float4,
    Mat3,
    Mat4,
    Struct
};

enum class ShaderStage
{
    Vertex,
    Fragment,
    TesselationCon,
    TesselationEval,
    Geometry,
    Compute,
    Count
};

enum class TextureFormat
{
    R8,
    R16,
    R32,
    RG8,
    RG16,
    RG32,
    RGB8,
    RGB16,
    RGB32,
    RGBA8,
    RGBA16,
    RGBA32
};

enum class IndexBufferType
{
    Uint32,
    Uint16
};

// objects used by the public api but held here for easier
// access between the different libraries.

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wpadded"

struct TextureSamplerParams
{
    SamplerFilter min = SamplerFilter::Nearest;
    SamplerFilter mag = SamplerFilter::Nearest;
    SamplerAddressMode addrU = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addrV = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addrW = SamplerAddressMode::ClampToEdge;
    VkBool32 enableAnisotropy = VK_TRUE;
    float anisotropy = 1.0f;
    VkBool32 enableCompare = VK_FALSE;
    CompareOp compareOp = CompareOp::Never;
    uint32_t mipLevels = 1;

    bool operator==(const TextureSamplerParams& rhs) const noexcept 
    {
        return memcmp(this, &rhs, sizeof(TextureSamplerParams)) == 0;
    }
};

using TextureSamplerHasher = util::Murmur3Hasher<TextureSamplerParams>;

#pragma clang diagnostic pop

static_assert(
    std::is_trivially_copyable<TextureSamplerParams>::value,
    "TextureSamplerParams must be a POD for hashing to work correctly");

} // namespace backend
