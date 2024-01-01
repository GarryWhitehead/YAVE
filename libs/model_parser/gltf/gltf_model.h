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

#include "model_material.h"
#include "model_mesh.h"
#include "node_instance.h"
#include "skin_instance.h"

#include <cgltf.h>
#include <mathfu/glsl_mappings.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <string_view>
#include <filesystem>

namespace yave
{
// forward declerations
class NodeInstance;
class MeshInstance;
class SkinInstance;
class ModelMaterial;
struct NodeInfo;
;

class GltfExtension
{
public:
    // <extension name, value (as string)>
    using ExtensionData = std::unordered_map<const char*, const char*>;

    GltfExtension();
    ~GltfExtension();

    // utility functions for dealing with gltf json data
    bool build(const cgltf_extras& extras, cgltf_data& data);

    /// token to string converion functions
    static mathfu::vec3 tokenToVec3(const char* str);

    std::string_view find(std::string_view ext);

private:
    ExtensionData extensions_;
};

class GltfModel
{
public:
    GltfModel();
    ~GltfModel();

    /// atributes
    static uint8_t* getAttributeData(const cgltf_attribute* attrib, size_t& stride);

    /**
     * @brief Load a specified gltf file from disk.
     * @param filename Absolute path to the gltf model file. Binary data must
     * also be present in the directory.
     * @return Whether the file was succesfully loaded.
     */
    bool load(const std::filesystem::path& filename);

    /**
     * @brief Parses the file that was loaded in via  @p load
     * Note: You must call @p load before this function.
     */
    bool build();

    // =========== helper functions ================
    /**
     * @brief Find all of the node hierarchies based on the specified id.
     * @param id The id of the node to find.
     * @return If successful, returns a pointer to the node. Otherwise returns
     * nullptr.
     */
    NodeInfo* getNode(std::string_view id);

    GltfExtension& getExtensions();

    // ========= user front-end functions ==============

    /**
     * @brief Set the world translation for this model
     */
    GltfModel& setWorldTrans(const mathfu::vec3& trans);

    /**
     * @brief Set the world scale for this model
     */
    GltfModel& setWorldScale(const mathfu::vec3& scale);

    /**
     * @brief Set the world rotation for this model
     */
    GltfModel& setWorldRotation(const mathfu::quat& rot);

    void setDirectory(const std::filesystem::path&  dir);

private:
    void lineariseRecursive(cgltf_node& node, size_t index);
    void lineariseNodes(cgltf_data* data);


public:
    std::vector<std::unique_ptr<NodeInstance>> nodes;

    // materials and image paths pulled out of the nodes.
    std::vector<ModelMaterial> materials;

    // skeleton data also removed from the nodes
    std::vector<SkinInstance> skins;

    // std::vector<AnimInstance> animations;

private:
    cgltf_data* gltfData_ = nullptr;

    // linearised nodes - with the name updated to store an id
    // for linking to our own node hierachy
    std::vector<cgltf_node*> linearisedNodes_;

    // all the extensions available for this model
    std::unique_ptr<GltfExtension> extensions_;

    // world co-ords
    mathfu::vec3 wTrans_;
    mathfu::vec3 wScale_ = mathfu::vec3 {1.0f};
    mathfu::quat wRotation_;

    // user defined path to the model directory
    std::filesystem::path modelDir_;
};

} // namespace yave
