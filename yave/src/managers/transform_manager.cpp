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

#include "transform_manager.h"

#include "engine.h"

#include <model_parser/gltf/node_instance.h>
#include <model_parser/gltf/skin_instance.h>
#include <utility/assertion.h>
#include <utility/logger.h>

#include <algorithm>

namespace yave
{

ITransformManager::ITransformManager(IEngine& engine) {}

ITransformManager::~ITransformManager() = default;

bool ITransformManager::addNodeHierachy(NodeInstance& node, Object& obj, SkinInstance* skin)
{
    if (!node.getRootNode())
    {
        LOGGER_ERROR("Trying to add a root node that is null.\n");
        return false;
    }

    TransformInfo info;
    info.root = new NodeInfo(*node.getRootNode());

    // add skins to the manager - these don't require a slot to be requested as
    // there may be numerous skins per mesh. Instead, the starting index of this
    // group will be used to offset the skin indices to point at the correct
    // skin
    if (skin)
    {
        info.skinOffset = static_cast<uint32_t>(skins_.size());
        skins_.emplace_back(*skin);
    }

    // update the model transform, and if skinned, joint matrices
    updateModelTransform(info.root, info);

    // request a slot for this Object
    ObjectHandle handle = addObject(obj);

    if (handle.get() >= nodes_.size())
    {
        nodes_.emplace_back(std::move(info));
    }
    else
    {
        nodes_[handle.get()] = std::move(info);
    }

    return true;
}

void ITransformManager::addTransformI(const mathfu::mat4& local, Object& obj)
{
    TransformInfo info;
    info.root = new NodeInfo();
    info.root->nodeTransform = local;
    info.root->hasMesh = true;

    // update the model transform, and if skinned, joint matrices
    updateModelTransform(info.root, info);

    // request a slot for this Object
    ObjectHandle handle = addObject(obj);

    if (handle.get() >= nodes_.size())
    {
        nodes_.emplace_back(std::move(info));
    }
    else
    {
        nodes_[handle.get()] = std::move(info);
    }
}

mathfu::mat4 ITransformManager::updateMatrix(NodeInfo* node)
{
    mathfu::mat4 mat = node->nodeTransform;
    NodeInfo* parent = node->parent;
    while (parent)
    {
        mat = parent->nodeTransform * mat;
        parent = parent->parent;
    }
    return mat;
}

void ITransformManager::updateModelTransform(NodeInfo* parent, TransformInfo& transInfo)
{
    // we need to find the mesh node first - we will then update matrices
    // working back towards the root node
    if (parent->hasMesh)
    {
        mathfu::mat4 mat;

        // update the matrices - child node transform * parent transform
        mat = updateMatrix(parent);

        // add updated local transfrom to the ubo buffer
        transInfo.modelTransform = mat;

        if (transInfo.skinOffset != TransformInfo::Uninitialised)
        {
            // the transform data index for this Object is stored on the
            // component
            auto skinIndex = static_cast<uint32_t>(parent->skinIndex);
            ASSERT_LOG(skinIndex >= 0);
            SkinInstance& skin = skins_[skinIndex];

            // get the number of joints in the skeleton
            uint32_t jointCount =
                std::min(static_cast<uint32_t>(skin.jointNodes.size()), MaxBoneCount);

            // transform to local space
            mathfu::mat4 inverseMat = mat.Inverse();

            for (uint32_t i = 0; i < jointCount; ++i)
            {
                // get a pointer to the joint a.k.a transform node
                NodeInfo* jointNode = skin.jointNodes[i];

                // the joint matrix is the local matrix * inverse bin matrix
                mathfu::mat4 jointMatrix = updateMatrix(jointNode) * skin.invBindMatrices[i];

                // transform joint to local (joint) space
                mathfu::mat4 localMatrix = inverseMat * jointMatrix;
                transInfo.jointMatrices[i] = localMatrix;
            }
        }

        // one mesh per node is required. So don't bother with the child nodes
        return;
    }

    // now work up the child nodes - until we find a mesh
    for (NodeInfo* child : parent->children)
    {
        updateModelTransform(child, transInfo);
    }
}

void ITransformManager::updateModel(const Object& obj)
{
    ObjectHandle handle = getObjIndex(obj);
    TransformInfo& info = nodes_[handle.get()];
    updateModelTransform(info.root->parent, info);
}

TransformInfo* ITransformManager::getTransform(const Object& obj)
{
    ObjectHandle handle = getObjIndex(obj);
    ASSERT_FATAL(
        handle.get() <= nodes_.size(),
        "Handle index is out of range for transform nodes (idx=%i)",
        handle.get());
    return &nodes_[handle.get()];
}

// ===================== client api ==============================

TransformManager::TransformManager() = default;
TransformManager::~TransformManager() = default;

void ITransformManager::addModelTransform(const ModelTransform& transform, Object& obj)
{
    mathfu::mat4 r = transform.rot.ToMatrix4();
    mathfu::mat4 s = mathfu::mat4::FromScaleVector(transform.scale);
    mathfu::mat4 t = mathfu::mat4::FromTranslationVector(transform.translation);
    mathfu::mat4 local = t * r * s;
    addTransformI(local, obj);
}

} // namespace yave
