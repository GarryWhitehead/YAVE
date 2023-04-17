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

#include "utility/bitset_enum.h"

#include <cgltf.h>
#include <mathfu/glsl_mappings.h>

#include <limits>
#include <memory>
#include <vector>

namespace yave
{
// forward declerations
class ModelMaterial;
class GltfModel;

class ModelMesh
{
public:
    struct Dimensions
    {
        Dimensions()
            : min(mathfu::vec3 {std::numeric_limits<float>::max()}),
              max(mathfu::vec3 {std::numeric_limits<float>::min()})
        {
        }
        mathfu::vec3 min;
        mathfu::vec3 max;
    };

    enum class Topology
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        TriangleFan,
        PatchList,
        Undefined
    };

    /**
     * @brief Specifies a variant to use when compiling the shader
     */
    enum class Variant : uint64_t
    {
        HasSkin,
        TangentInput,
        BiTangentInput,
        HasUv,
        HasNormal,
        HasWeight,
        HasJoint,
        __SENTINEL__
    };


    /**
     * @brief A abstract vertex buffer. Vertices are stored as blobs of data as
     * the shader system allows numerous variants in the vertex inputs. The blob
     * is described by the
     * **VertexDescriptor*
     */
    struct VertexBuffer
    {
        enum Attribute
        {
            Float,
            Int,
            Vec2,
            Vec3,
            Vec4,
            Mat3,
            Mat4
        };

        struct Descriptor
        {
            uint8_t stride;
            uint8_t width;
        };

        uint8_t* data = nullptr;
        uint64_t size = 0;
        uint32_t strideSize = 0;
        uint32_t vertCount = 0;
        std::vector<Attribute> attributes;
    };

    struct Primitive
    {
        Primitive() = default;

        // sub-mesh dimensions.
        Dimensions dimensions;

        // index offsets
        size_t indexBase = 0;
        size_t indexCount = 0;

        // ============ vulakn backend ==========================
        // set by calling **update**
        size_t indexPrimitiveOffset = 0;
    };

    ModelMesh();
    ~ModelMesh();

    /**
     * @brief Create a new mesh instance based on client defined vertex data.
     * This is useful when using the static factory models. This at present
     * doesn't support materials.
     */
    void create(
        std::vector<mathfu::vec4>& positions,
        std::vector<mathfu::vec2>& texCoords,
        std::vector<mathfu::vec3>& normals,
        const std::vector<int>& indices,
        const Topology& topo);

    bool build(const cgltf_mesh& mesh, GltfModel& model);

    // bool prepare(const cgltf_mesh& mesh, GltfModel& model);

    // bool prepare(aiScene* scene);

    // ======================= mesh data (public)
    // =======================================

    /// the material associated with this mesh (if it has one)
    std::unique_ptr<ModelMaterial> material_;

    /// defines the topology to use in the program state
    Topology topology_;

    /// the overall dimensions of this model. Sub-meshses contain their own
    /// dimensions
    Dimensions dimensions_;

    /// sub-meshes
    std::vector<Primitive> primitives_;

    /// All vertivces associated with the particular model
    VertexBuffer vertices_;

    /// All indices associated with this particular model
    std::vector<uint32_t> indices_;

    /// variation of the mesh shader
    util::BitSetEnum<Variant> variantBits_;

private:
};

} // namespace yave
