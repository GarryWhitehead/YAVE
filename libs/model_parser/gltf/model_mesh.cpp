/* Copyright (c) 2018-2020 Garry Whitehead
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

#include "model_mesh.h"

#include "gltf_model.h"
#include "model_material.h"
#include "utility/logger.h"

#include <unordered_map>

namespace yave
{

ModelMesh::ModelMesh() : material_(nullptr), topology_(Topology::TriangleList) {}
ModelMesh::~ModelMesh() {}

void ModelMesh::create(
    std::vector<mathfu::vec4>& positions,
    std::vector<mathfu::vec2>& texCoords,
    std::vector<mathfu::vec3>& normals,
    const std::vector<int>& indices,
    const Topology& topo)
{
    uint8_t* dataPtr = vertices_.data;

    uint8_t* posPtr = reinterpret_cast<uint8_t*>(positions.data());
    uint8_t* normPtr = reinterpret_cast<uint8_t*>(normals.data());
    uint8_t* uvPtr = reinterpret_cast<uint8_t*>(texCoords.data());

    size_t posStride = sizeof(mathfu::vec4);
    size_t normStride = sizeof(mathfu::vec3);
    size_t uvStride = sizeof(mathfu::vec2);

    vertices_.vertCount = static_cast<uint32_t>(positions.size());

    // stride size in bytes - also set variant bit for use with shaders
    size_t strideCount = sizeof(mathfu::vec4);
    if (normals.size() > 0)
    {
        strideCount += sizeof(mathfu::vec3);
        variantBits_ |= ModelMesh::Variant::HasNormal;
    }
    if (texCoords.size() > 0)
    {
        strideCount += sizeof(mathfu::vec2);
        variantBits_ |= ModelMesh::Variant::HasUv;
    }
    vertices_.strideSize = static_cast<uint32_t>(strideCount);
    vertices_.data = new uint8_t[vertices_.vertCount * vertices_.strideSize];

    // now contruct the interleaved vertex data - in the format:
    // position -- normal -- texCoord
    for (size_t i = 0; i < vertices_.vertCount; ++i)
    {
        // positions
        memcpy(dataPtr, posPtr, posStride);
        posPtr += posStride;
        dataPtr += posStride;

        // normals
        if (normals.size() > 0)
        {
            memcpy(dataPtr, normPtr, normStride);
            normPtr += normStride;
            dataPtr += normStride;
        }

        // uvs
        if (texCoords.size() > 0)
        {
            memcpy(dataPtr, uvPtr, uvStride);
            uvPtr += uvStride;
            dataPtr += uvStride;
        }
    }

    // copy the indices
    memcpy(indices_.data(), indices.data(), sizeof(int) * indices.size());

    // create the primitive info
    primitives_.push_back({{}, 0, indices.size(), 0});

    topology_ = topo;
}

bool ModelMesh::build(const cgltf_mesh& mesh, GltfModel& model)
{
    cgltf_primitive* meshEnd = mesh.primitives + mesh.primitives_count;
    for (const cgltf_primitive* primitive = mesh.primitives; primitive < meshEnd; ++primitive)
    {
        Primitive newPrimitive;

        // must have indices otherwise got to the next primitive
        if (!primitive->indices || primitive->indices->count == 0)
        {
            continue;
        }

        // must be triangle type otherwise we won't be able to deal with it
        if (primitive->type != cgltf_primitive_type_triangles)
        {
            LOGGER_ERROR("At the moment only triangles are supported by the "
                         "gltf parser.\n");
            return false;
        }

        // only one material per mesh is allowed which is the case 99% of the
        // time cache the cgltf pointer and use to ensure this is the case
        if (!material_)
        {
            material_ = std::make_unique<ModelMaterial>();
            material_->create(*primitive->material, model.getExtensions());
        }

        // get the number of vertices to process
        size_t vertCount = primitive->attributes[0].data->count;

        // ================ vertices =====================
        // somewhere to store the base pointers and stride for each attribute
        uint8_t* posBase = nullptr;
        uint8_t* normBase = nullptr;
        uint8_t* uvBase = nullptr;
        uint8_t* weightsBase = nullptr;
        uint8_t* jointsBase = nullptr;

        size_t posStride, normStride, uvStride, weightsStride, jointsStride;
        size_t attribCount = primitive->attributes_count;
        size_t attribStride = 0;

        cgltf_attribute* attribEnd = primitive->attributes + attribCount;
        for (const cgltf_attribute* attrib = primitive->attributes; attrib < attribEnd; ++attrib)
        {
            if (attrib->type == cgltf_attribute_type_position)
            {
                posBase = GltfModel::getAttributeData(attrib, posStride);
                assert(posStride == 12);
                attribStride += posStride;
            }

            else if (attrib->type == cgltf_attribute_type_normal)
            {
                normBase = GltfModel::getAttributeData(attrib, normStride);
                assert(normStride == 12);
                attribStride += normStride;
                variantBits_ |= ModelMesh::Variant::HasNormal;
            }

            else if (attrib->type == cgltf_attribute_type_texcoord)
            {
                uvBase = GltfModel::getAttributeData(attrib, uvStride);
                assert(uvStride == 8);
                attribStride += uvStride;
                variantBits_ |= ModelMesh::Variant::HasUv;
            }

            else if (attrib->type == cgltf_attribute_type_joints)
            {
                jointsBase = GltfModel::getAttributeData(attrib, jointsStride);
                attribStride += jointsStride;
                variantBits_ |= ModelMesh::Variant::HasJoint;
            }

            else if (attrib->type == cgltf_attribute_type_weights)
            {
                weightsBase = GltfModel::getAttributeData(attrib, weightsStride);
                attribStride += weightsStride;
                variantBits_ |= ModelMesh::Variant::HasWeight;
            }
            else
            {
                LOGGER_INFO("Gltf attribute not supported - %s. Will skip.\n", attrib->name);
            }
        }

        // must have position data otherwise we can't continue
        if (!posBase)
        {
            LOGGER_ERROR("Gltf file contains no vertex position data. Unable "
                         "to continue.\n");
            return false;
        }

        // this has to been done here and not above as the we expect these to be
        // in a specific order
        vertices_.attributes.emplace_back(VertexBuffer::Attribute::Vec3);
        if (uvBase)
        {
            vertices_.attributes.emplace_back(VertexBuffer::Attribute::Vec2);
        }
        if (normBase)
        {
            vertices_.attributes.emplace_back(VertexBuffer::Attribute::Vec3);
        }
        if (weightsBase)
        {
            vertices_.attributes.emplace_back(VertexBuffer::Attribute::Float);
        }
        if (jointsBase)
        {
            vertices_.attributes.emplace_back(VertexBuffer::Attribute::Vec4);
        }

        // store vertex as a blob of data
        vertices_.vertCount = static_cast<uint32_t>(vertCount);
        vertices_.strideSize = static_cast<uint32_t>(attribStride);
        vertices_.size = attribStride * vertCount;
        vertices_.data = new uint8_t[vertices_.size];

        uint8_t* dataPtr = vertices_.data;

        // now contruct the interleaved vertex data
        for (size_t i = 0; i < vertCount; ++i)
        {
            // we know the positional data exsists - it's mandatory
            // sort out min/max boundaries of the sub-mesh
            float* posPtr = reinterpret_cast<float*>(posBase);
            mathfu::vec3 pos {*posPtr, *(posPtr + 1), *(posPtr + 2)};
            newPrimitive.dimensions.min = mathfu::MinHelper(newPrimitive.dimensions.min, pos);
            newPrimitive.dimensions.max = mathfu::MaxHelper(newPrimitive.dimensions.max, pos);

            memcpy(dataPtr, posBase, posStride);
            posBase += posStride;
            dataPtr += posStride;

            if (normBase)
            {
                memcpy(dataPtr, normBase, normStride);
                normBase += normStride;
                dataPtr += normStride;
            }
            if (uvBase)
            {
                memcpy(dataPtr, uvBase, uvStride);
                uvBase += uvStride;
                dataPtr += uvStride;
            }
            if (weightsBase)
            {
                memcpy(dataPtr, weightsBase, weightsStride);
                weightsBase += weightsStride;
                dataPtr += weightsStride;
            }
            if (jointsBase)
            {
                memcpy(dataPtr, jointsBase, jointsStride);
                jointsBase += jointsStride;
                dataPtr += jointsStride;
            }
        }

        // ================= indices ===================
        size_t indicesCount = primitive->indices->count;
        newPrimitive.indexBase = indices_.size();
        newPrimitive.indexCount = indicesCount;
        indices_.reserve(indicesCount);

        // now for the indices - Note: if the idices aren't 32-bit ints, they
        // will be casted into this format
        if ((indicesCount % 3) != 0)
        {
            LOGGER_ERROR("Indices data is of incorrect size.\n");
            return false;
        }

        const uint8_t* base =
            static_cast<const uint8_t*>(primitive->indices->buffer_view->buffer->data) +
            primitive->indices->offset + primitive->indices->buffer_view->offset;

        for (size_t i = 0; i < primitive->indices->count; ++i)
        {
            uint32_t index = 0;
            if (primitive->indices->component_type == cgltf_component_type_r_32u)
            {
                index = *reinterpret_cast<const uint32_t*>(base + sizeof(uint32_t) * i);
            }
            else if (primitive->indices->component_type == cgltf_component_type_r_16u)
            {
                index = *reinterpret_cast<const uint16_t*>(base + sizeof(uint16_t) * i);
            }
            else
            {
                LOGGER_ERROR("Unsupported indices type. Unable to proceed.\n");
                return false;
            }

            indices_.emplace_back(index);
        }

        // adjust the overall model boundaries based on the sub mesh
        dimensions_.min = mathfu::MinHelper(dimensions_.min, newPrimitive.dimensions.min);
        dimensions_.max = mathfu::MaxHelper(dimensions_.max, newPrimitive.dimensions.max);

        primitives_.emplace_back(newPrimitive);
    }
    return true;
}

} // namespace yave
