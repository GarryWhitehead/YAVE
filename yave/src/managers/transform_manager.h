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

#pragma once

#include "component_manager.h"
#include "uniform_buffer.h"
#include "vulkan-api/buffer.h"
#include "yave/transform_manager.h"

#include <mathfu/glsl_mappings.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace yave
{
// forward decleartions
class IObject;
class SkinInstance;
struct NodeInfo;
class NodeInstance;
class IEngine;

struct TransformInfo
{
    static constexpr uint32_t Uninitialised =
        std::numeric_limits<uint32_t>::max();

    NodeInfo* root = nullptr;

    // the transform of this model - calculated by calling updateTransform()
    mathfu::mat4 modelTransform;

    // the offset all skin indices will be adjusted by within this
    // node hierachy. We use this also to signify if this model has a skin
    uint32_t skinOffset = Uninitialised;

    // skinning data - set by calling updateTransform()
    std::vector<mathfu::mat4> jointMatrices;
};

class ITransformManager : public ComponentManager, public TransformManager
{
public:
    static constexpr uint32_t MaxBoneCount = 25;

    ITransformManager(IEngine& engine);
    ~ITransformManager();

 
    bool addNodeHierachy(NodeInstance& node, IObject& obj, SkinInstance* skin);

    void addTransformI(
        const mathfu::mat4& local,
        IObject* obj);

    mathfu::mat4 updateMatrix(NodeInfo* node);

    void updateModelTransform(NodeInfo* parent, TransformInfo& transInfo);

    void updateModel(const IObject& obj);

    // =================== getters ==========================

    TransformInfo* getTransform(const IObject& obj);

    // ======================== client API ===========================

    void
    addModelTransform(const ModelTransform& transform, Object* obj) override;

private:
    // transform data preserved in the node hierachal format
    // referenced by assoociated IObject
    std::vector<TransformInfo> nodes_;

    // skinned data - inverse bind matrices and bone info
    std::vector<SkinInstance> skins_;
};

} // namespace yave
