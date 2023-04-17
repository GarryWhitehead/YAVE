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

#include <cgltf.h>
#include <mathfu/glsl_mappings.h>
#include <utility/cstring.h>

#include <memory>
#include <unordered_map>

namespace yave
{

// forward decleartions
class GltfModel;
class ModelMesh;
class SkinInstance;
class NodeInstance;

/**
 * @brief Keep the node heirachy obtained from the gltf file as this
 * will be needed for bone transforms
 */
struct NodeInfo
{
    NodeInfo() = default;
    ~NodeInfo();

    NodeInfo(const NodeInfo& rhs);
    NodeInfo& operator=(const NodeInfo& rhs);
    NodeInfo(NodeInfo&& rhs) = default;
    NodeInfo& operator=(NodeInfo&& rhs) = default;

    NodeInfo(const NodeInstance&) = delete;
    NodeInfo& operator=(const NodeInstance&) = delete;

    /// The id of the node is derived from the index - and is used to locate
    /// this node if it's a joint or animation target
    util::CString id;

    /// the index of the skin associated with this node
    size_t skinIndex = -1;

    /// the animation channel index
    size_t channelIndex = -1;

    /// a flag indicating whether this node contains a mesh
    /// the mesh is actaully stored outside the hierachy
    bool hasMesh = false;

    // the transform matrix transformed by the parent matrix
    mathfu::mat4 localTransform;

    // the transform matrix for this node = T*R*S
    mathfu::mat4 nodeTransform;

    // parent of this node. Null signifies the root
    NodeInfo* parent = nullptr;

    // children of this node
    std::vector<NodeInfo*> children;
};

class NodeInstance
{
public:
    NodeInstance();
    ~NodeInstance();

    void prepareTranslation(cgltf_node* node, NodeInfo* newNode);

    bool prepare(cgltf_node* node, GltfModel& model);

    NodeInfo* getNode(util::CString id);

    ModelMesh* getMesh();
    SkinInstance* getSkin();
    NodeInfo* getRootNode();

private:
    bool prepareNodeHierachy(
        cgltf_node* node,
        NodeInfo* newNode,
        NodeInfo* parent,
        mathfu::mat4& parentTransform,
        GltfModel& model,
        size_t& nodeIdx);

    NodeInfo* findNode(util::CString id, NodeInfo* node);

private:
    // we expect one mesh per node hierachy.
    std::unique_ptr<ModelMesh> mesh_;

    // the node hierachy
    std::unique_ptr<NodeInfo> rootNode_;

    // It is assumed that skins won't be used between different nodes in
    // multi-node models
    std::unique_ptr<SkinInstance> skin_;
};
} // namespace yave
