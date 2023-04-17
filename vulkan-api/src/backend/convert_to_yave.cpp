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

#include "convert_to_yave.h"

namespace backend
{

backend::SamplerFilter samplerFilterToYave(yave::ModelMaterial::Sampler::Filter filter)
{
    backend::SamplerFilter output = backend::SamplerFilter::Linear;
    switch (filter)
    {
        case yave::ModelMaterial::Sampler::Filter::Cubic:
            output = backend::SamplerFilter::Cubic;
            break;
        case yave::ModelMaterial::Sampler::Filter::Nearest:
            output = backend::SamplerFilter::Nearest;
            break;
    }
    return output;
}

backend::SamplerAddressMode samplerWrapModeToYave(yave::ModelMaterial::Sampler::AddressMode addr)
{
    backend::SamplerAddressMode output = backend::SamplerAddressMode::ClampToEdge;
    switch (addr)
    {
        case yave::ModelMaterial::Sampler::AddressMode::ClampToBorder:
            output = backend::SamplerAddressMode::ClampToBorder;
            break;
        case yave::ModelMaterial::Sampler::AddressMode::MirrorClampToEdge:
            output = backend::SamplerAddressMode::MirrorClampToEdge;
            break;
        case yave::ModelMaterial::Sampler::AddressMode::MirroredRepeat:
            output = backend::SamplerAddressMode::MirroredRepeat;
            break;
    }
    return output;
}

backend::PrimitiveTopology primitiveTopologyToYave(yave::ModelMesh::Topology topo)
{
    backend::PrimitiveTopology output;
    switch (topo)
    {
        case yave::ModelMesh::Topology::TriangleFan:
            output = PrimitiveTopology::TriangleFan;
            break;
        case yave::ModelMesh::Topology::TriangleList:
            output = PrimitiveTopology::TriangleList;
            break;
        case yave::ModelMesh::Topology::TriangleStrip:
            output = PrimitiveTopology::TriangleStrip;
            break;
        case yave::ModelMesh::Topology::LineList:
            output = PrimitiveTopology::LineList;
            break;
        case yave::ModelMesh::Topology::LineStrip:
            output = PrimitiveTopology::LineStrip;
            break;
        case yave::ModelMesh::Topology::PointList:
            output = PrimitiveTopology::PointList;
            break;
        case yave::ModelMesh::Topology::PatchList:
            output = PrimitiveTopology::PatchList;
            break;
    }
    return output;
}

} // namespace backend
