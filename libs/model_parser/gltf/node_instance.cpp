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

#include "node_instance.h"

#include "gltf_model.h"
#include "model_mesh.h"
#include "skin_instance.h"
#include "utility/assertion.h"
#include "utility/logger.h"

namespace yave
{

NodeInfo::~NodeInfo()
{
    for (auto& child : children)
    {
        if (child)
        {
            delete child;
            child = nullptr;
        }
    }
}

NodeInfo::NodeInfo(const NodeInfo& rhs)
    : id(rhs.id),
      skinIndex(rhs.skinIndex),
      channelIndex(rhs.channelIndex),
      hasMesh(rhs.hasMesh),
      localTransform(rhs.localTransform),
      nodeTransform(rhs.nodeTransform)
{
    // if there already children, delete them
    children.clear();

    for (auto& child : rhs.children)
    {
        NodeInfo* newChild = new NodeInfo(*child);
        children.push_back(newChild);
        children.back()->parent = this;
    }
}

NodeInfo& NodeInfo::operator=(const NodeInfo& rhs)
{
    if (this != &rhs)
    {
        id = rhs.id;
        skinIndex = rhs.skinIndex;
        channelIndex = rhs.channelIndex;
        hasMesh = rhs.hasMesh;
        localTransform = rhs.localTransform;
        nodeTransform = rhs.nodeTransform;

        // if there already children, delete them
        children.clear();

        for (auto& child : rhs.children)
        {
            NodeInfo* newChild = new NodeInfo(*child);
            children.push_back(newChild);
            children.back()->parent = this;
        }
    }
    return *this;
}

// ================================================================================================================================

NodeInstance::NodeInstance() {}

NodeInstance::~NodeInstance() {}

NodeInfo* NodeInstance::findNode(util::CString id, NodeInfo* node)
{
    NodeInfo* result = nullptr;
    if (node->id.compare(id))
    {
        return node;
    }
    for (NodeInfo* child : node->children)
    {
        result = findNode(id, child);
        if (result)
        {
            break;
        }
    }
    return result;
}

NodeInfo* NodeInstance::getNode(util::CString id) { return findNode(id, rootNode_.get()); }

bool NodeInstance::prepareNodeHierachy(
    cgltf_node* node,
    NodeInfo* newNode,
    NodeInfo* parent,
    mathfu::mat4& parentTransform,
    GltfModel& model,
    size_t& nodeIdx)
{
    ASSERT_LOG(newNode);
    newNode->parent = parent;
    // newNode->id = nodeIdx++;

    if (node->mesh)
    {
        mesh_ = std::make_unique<ModelMesh>();
        mesh_->build(*node->mesh, model);
        newNode->hasMesh = true;

        if (node->skin)
        {
            skin_ = std::make_unique<SkinInstance>();
            skin_->prepare(*node->skin, *this);
        }

        // propogate transforms through node list
        prepareTranslation(node, newNode);
        newNode->localTransform = parentTransform * newNode->localTransform;
    }

    // now for the children of this node
    cgltf_node* const* childEnd = node->children + node->children_count;
    for (cgltf_node* const* child = node->children; child < childEnd; ++child)
    {
        NodeInfo* childNode = new NodeInfo();
        if (!prepareNodeHierachy(node, childNode, newNode, newNode->nodeTransform, model, nodeIdx))
        {
            return false;
        }
        newNode->children.emplace_back(childNode);
    }

    return true;
}

void NodeInstance::prepareTranslation(cgltf_node* node, NodeInfo* newNode)
{
    // usually the gltf file will have a baked matrix or trs data
    if (node->has_matrix)
    {
        newNode->localTransform = mathfu::mat4(node->matrix);
    }
    else
    {
        mathfu::vec3 translation = {0.0f, 0.0f, 0.0f};
        mathfu::vec3 scale = {0.0f, 0.0f, 0.0f};
        mathfu::quat rot = {1.0f, 0.0f, 0.0f, 0.0f};

        if (node->has_translation)
        {
            translation =
                mathfu::vec3 {node->translation[0], node->translation[1], node->translation[2]};
        }
        if (node->has_rotation)
        {
            rot = mathfu::quat {
                node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]};
        }
        if (node->has_scale)
        {
            scale = mathfu::vec3 {node->scale[0], node->scale[1], node->scale[2]};
        }

        newNode->nodeTransform = mathfu::mat4::FromTranslationVector(translation) *
            rot.ToMatrix4() * mathfu::mat4::FromScaleVector(scale);
    }
}

bool NodeInstance::prepare(cgltf_node* node, GltfModel& model)
{
    size_t nodeId = 0;
    mathfu::mat4 transform;
    rootNode_ = std::make_unique<NodeInfo>();

    if (!prepareNodeHierachy(node, rootNode_.get(), nullptr, transform, model, nodeId))
    {
        return false;
    }

    return true;
}

ModelMesh* NodeInstance::getMesh()
{
    assert(mesh_);
    return mesh_.get();
}

SkinInstance* NodeInstance::getSkin()
{
    ASSERT_LOG(skin_);
    return skin_.get();
}

NodeInfo* NodeInstance::getRootNode()
{
    assert(rootNode_);
    return rootNode_.get();
}

} // namespace yave
