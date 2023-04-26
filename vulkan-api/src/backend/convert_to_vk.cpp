
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

#include "convert_to_vk.h"

#include <spdlog/spdlog.h>

namespace backend
{

vk::BlendFactor blendFactorToVk(backend::BlendFactor factor)
{
    vk::BlendFactor output;
    switch (factor)
    {
        case backend::BlendFactor::Zero:
            output = vk::BlendFactor::eZero;
            break;
        case backend::BlendFactor::One:
            output = vk::BlendFactor::eOne;
            break;
        case backend::BlendFactor::SrcColour:
            output = vk::BlendFactor::eSrcColor;
            break;
        case backend::BlendFactor::OneMinusSrcColour:
            output = vk::BlendFactor::eOneMinusSrcColor;
            break;
        case backend::BlendFactor::DstColour:
            output = vk::BlendFactor::eDstColor;
            break;
        case backend::BlendFactor::OneMinusDstColour:
            output = vk::BlendFactor::eOneMinusDstColor;
            break;
        case backend::BlendFactor::SrcAlpha:
            output = vk::BlendFactor::eSrcAlpha;
            break;
        case backend::BlendFactor::OneMinusSrcAlpha:
            output = vk::BlendFactor::eOneMinusSrcAlpha;
            break;
        case backend::BlendFactor::DstAlpha:
            output = vk::BlendFactor::eDstAlpha;
            break;
        case backend::BlendFactor::OneMinusDstAlpha:
            output = vk::BlendFactor::eOneMinusDstAlpha;
            break;
        case backend::BlendFactor::ConstantColour:
            output = vk::BlendFactor::eConstantColor;
            break;
        case backend::BlendFactor::OneMinusConstantColour:
            output = vk::BlendFactor::eOneMinusConstantColor;
            break;
        case backend::BlendFactor::ConstantAlpha:
            output = vk::BlendFactor::eConstantAlpha;
            break;
        case backend::BlendFactor::OneMinusConstantAlpha:
            output = vk::BlendFactor::eOneMinusConstantAlpha;
            break;
        case backend::BlendFactor::SrcAlphaSaturate:
            output = vk::BlendFactor::eSrcAlphaSaturate;
            break;
        default:
            SPDLOG_WARN("Unrecognised blend factor when converting to Vk.");
            break;
    }
    return output;
}

vk::BlendOp blendOpToVk(backend::BlendOp op)
{
    vk::BlendOp output;
    switch (op)
    {
        case backend::BlendOp::Subtract:
            output = vk::BlendOp::eSubtract;
            break;
        case backend::BlendOp::ReverseSubtract:
            output = vk::BlendOp::eReverseSubtract;
            break;
        case backend::BlendOp::Add:
            output = vk::BlendOp::eAdd;
            break;
        case backend::BlendOp::Min:
            output = vk::BlendOp::eMin;
            break;
        case backend::BlendOp::Max:
            output = vk::BlendOp::eMax;
            break;
        default:
            SPDLOG_WARN("Unrecognised blend op when converting to Vk.");
            break;
    }
    return output;
}

vk::SamplerAddressMode samplerAddrModeToVk(SamplerAddressMode mode)
{
    vk::SamplerAddressMode output;
    switch (mode)
    {
        case backend::SamplerAddressMode::Repeat:
            output = vk::SamplerAddressMode::eRepeat;
            break;
        case backend::SamplerAddressMode::MirroredRepeat:
            output = vk::SamplerAddressMode::eMirroredRepeat;
            break;
        case backend::SamplerAddressMode::ClampToEdge:
            output = vk::SamplerAddressMode::eClampToEdge;
            break;
        case backend::SamplerAddressMode::ClampToBorder:
            output = vk::SamplerAddressMode::eClampToBorder;
            break;
        case backend::SamplerAddressMode::MirrorClampToEdge:
            output = vk::SamplerAddressMode::eMirrorClampToEdge;
            break;
    }
    return output;
}

vk::Filter samplerFilterToVk(SamplerFilter filter)
{
    vk::Filter output;
    switch (filter)
    {
        case backend::SamplerFilter::Nearest:
            output = vk::Filter::eNearest;
            break;
        case backend::SamplerFilter::Linear:
            output = vk::Filter::eLinear;
            break;
        case backend::SamplerFilter::Cubic:
            output = vk::Filter::eCubicIMG;
            break;
    }
    return output;
}

vk::CullModeFlagBits cullModeToVk(CullMode mode)
{
    vk::CullModeFlagBits output;
    switch (mode)
    {
        case backend::CullMode::Back:
            output = vk::CullModeFlagBits::eBack;
            break;
        case backend::CullMode::Front:
            output = vk::CullModeFlagBits::eFront;
            break;
        case backend::CullMode::None:
            output = vk::CullModeFlagBits::eNone;
            break;
    }
    return output;
}

vk::CompareOp compareOpToVk(CompareOp op)
{
    vk::CompareOp output;
    switch (op)
    {
        case CompareOp::Never:
            output = vk::CompareOp::eNever;
            break;
        case CompareOp::Always:
            output = vk::CompareOp::eAlways;
            break;
        case CompareOp::Equal:
            output = vk::CompareOp::eEqual;
            break;
        case CompareOp::Greater:
            output = vk::CompareOp::eGreater;
            break;
        case CompareOp::GreaterOrEqual:
            output = vk::CompareOp::eGreaterOrEqual;
            break;
        case CompareOp::Less:
            output = vk::CompareOp::eLess;
            break;
        case CompareOp::LessOrEqual:
            output = vk::CompareOp::eLessOrEqual;
            break;
        case CompareOp::NotEqual:
            output = vk::CompareOp::eNotEqual;
            break;
    }
    return output;
}

vk::PrimitiveTopology primitiveTopologyToVk(PrimitiveTopology topo)
{
    vk::PrimitiveTopology output;
    switch (topo)
    {
        case PrimitiveTopology::PointList:
            output = vk::PrimitiveTopology::ePointList;
            break;
        case PrimitiveTopology::LineList:
            output = vk::PrimitiveTopology::eLineList;
            break;
        case PrimitiveTopology::LineStrip:
            output = vk::PrimitiveTopology::eLineStrip;
            break;
        case PrimitiveTopology::TriangleList:
            output = vk::PrimitiveTopology::eTriangleList;
            break;
        case PrimitiveTopology::TriangleStrip:
            output = vk::PrimitiveTopology::eTriangleStrip;
            break;
        case PrimitiveTopology::TriangleFan:
            output = vk::PrimitiveTopology::eTriangleFan;
            break;
        case PrimitiveTopology::LineListWithAdjacency:
            output = vk::PrimitiveTopology::eLineListWithAdjacency;
            break;
        case PrimitiveTopology::LineStripWithAdjacency:
            output = vk::PrimitiveTopology::eLineStripWithAdjacency;
            break;
        case PrimitiveTopology::TriangleListWithAdjacency:
            output = vk::PrimitiveTopology::eTriangleListWithAdjacency;
            break;
        case PrimitiveTopology::TriangleStripWithAdjacency:
            output = vk::PrimitiveTopology::eTriangleStripWithAdjacency;
            break;
        case PrimitiveTopology::PatchList:
            output = vk::PrimitiveTopology::ePatchList;
            break;
    }
    return output;
}

vk::IndexType indexBufferTypeToVk(backend::IndexBufferType type)
{
    vk::IndexType output;
    switch (type)
    {
        case IndexBufferType::Uint32:
            output = vk::IndexType::eUint32;
            break;
        case IndexBufferType::Uint16:
            output = vk::IndexType::eUint16;
            break;
        default:
            SPDLOG_WARN("Unsupported index buffer type.");
            break;
    }
    return output;
}

vk::Format textureFormatToVk(backend::TextureFormat type)
{
    vk::Format output = vk::Format::eUndefined;

    switch (type)
    {
        case backend::TextureFormat::R8:
            output = vk::Format::eR8Unorm;
            break;
        case backend::TextureFormat::R16:
            output = vk::Format::eR16Sfloat;
            break;
        case backend::TextureFormat::R32:
            output = vk::Format::eR32Uint;
            break;
        case backend::TextureFormat::RG8:
            output = vk::Format::eR8G8Unorm;
            break;
        case backend::TextureFormat::RG16:
            output = vk::Format::eR16G16Sfloat;
            break;
        case backend::TextureFormat::RG32:
            output = vk::Format::eR32G32Uint;
            break;
        case backend::TextureFormat::RGB8:
            output = vk::Format::eR8G8B8Unorm;
            break;
        case backend::TextureFormat::RGB16:
            output = vk::Format::eR16G16B16Sfloat;
            break;
        case backend::TextureFormat::RGB32:
            output = vk::Format::eR32G32B32Uint;
            break;
        case backend::TextureFormat::RGBA8:
            output = vk::Format::eR8G8B8A8Unorm;
            break;
        case backend::TextureFormat::RGBA16:
            output = vk::Format::eR16G16B16A16Sfloat;
            break;
        case backend::TextureFormat::RGBA32:
            output = vk::Format::eR32G32B32A32Sfloat;
            break;
    }
    return output;
}

vk::ImageUsageFlags imageUsageToVk(uint32_t usageFlags)
{
    vk::ImageUsageFlags output;
    if (usageFlags & ImageUsage::Sampled)
    {
        output |= vk::ImageUsageFlagBits::eSampled;
    }
    if (usageFlags & ImageUsage::Storage)
    {
        output |= vk::ImageUsageFlagBits::eStorage;
    }
    if (usageFlags & ImageUsage::ColourAttach)
    {
        output |= vk::ImageUsageFlagBits::eColorAttachment;
    }
    if (usageFlags & ImageUsage::DepthAttach)
    {
        output |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
    }
    if (usageFlags & ImageUsage::InputAttach)
    {
        output |= vk::ImageUsageFlagBits::eInputAttachment;
    }

    return output;
}

vk::AttachmentLoadOp loadFlagsToVk(const LoadClearFlags flags)
{
    vk::AttachmentLoadOp result;
    switch (flags)
    {
        case LoadClearFlags::Clear:
            result = vk::AttachmentLoadOp::eClear;
            break;
        case LoadClearFlags::DontCare:
            result = vk::AttachmentLoadOp::eDontCare;
            break;
        case LoadClearFlags::Load:
            result = vk::AttachmentLoadOp::eLoad;
            break;
    }
    return result;
}

vk::AttachmentStoreOp storeFlagsToVk(const StoreClearFlags flags)
{
    vk::AttachmentStoreOp result;
    switch (flags)
    {
        case StoreClearFlags::DontCare:
            result = vk::AttachmentStoreOp::eDontCare;
            break;
        case StoreClearFlags::Store:
            result = vk::AttachmentStoreOp::eStore;
            break;
    }
    return result;
}

vk::SampleCountFlagBits samplesToVk(const uint32_t count)
{
    vk::SampleCountFlagBits result = vk::SampleCountFlagBits::e1;
    switch (count)
    {
        case 1:
            result = vk::SampleCountFlagBits::e1;
            break;
        case 2:
            result = vk::SampleCountFlagBits::e2;
            break;
        case 4:
            result = vk::SampleCountFlagBits::e4;
            break;
        case 8:
            result = vk::SampleCountFlagBits::e8;
            break;
        case 16:
            result = vk::SampleCountFlagBits::e16;
            break;
        case 32:
            result = vk::SampleCountFlagBits::e32;
            break;
        case 64:
            result = vk::SampleCountFlagBits::e64;
            break;
        default:
            SPDLOG_WARN("Unsupported sample count. Set to one.");
            break;
    }
    return result;
}

} // namespace backend
