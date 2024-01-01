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

#include "gltf_model.h"

#include "model_material.h"
#include "utility/assertion.h"
#include "utility/logger.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <jsmn.h>

#include <filesystem>


namespace yave
{

GltfExtension::GltfExtension() {}
GltfExtension::~GltfExtension() {}

mathfu::vec3 GltfExtension::tokenToVec3(const char* str)
{
    std::string vecStr {str};
    std::vector<const char*> splitStrs;
    size_t pos = 0;
    while ((pos = vecStr.find(" ")) != std::string::npos) {
        std::string token = vecStr.substr(0, pos);
        splitStrs.emplace_back(token.c_str());
        vecStr.erase(0, pos + 1);
    }
    ASSERT_FATAL(
        splitStrs.size() == 3, "String must be of vec3 type - %i elements found.", splitStrs.size());
    return {
        std::strtof(splitStrs[0], nullptr),
        std::strtof(splitStrs[1], nullptr),
        std::strtof(splitStrs[2], nullptr)};
}

std::string_view GltfExtension::find(std::string_view ext)
{
    auto iter = extensions_.find(ext.data());
    if (iter == extensions_.end())
    {
        return "";
    }
    return iter->second;
}

bool GltfExtension::build(const cgltf_extras& extras, cgltf_data& data)
{
    // first check whther there are any extensions
    cgltf_size extSize;
    cgltf_copy_extras_json(&data, &extras, nullptr, &extSize);
    if (!extSize)
    {
        return true;
    }

    // alloc mem for the json extension data
    char* jsonData = new char[extSize + 1];

    cgltf_result res = cgltf_copy_extras_json(&data, &extras, jsonData, &extSize);
    if (res != cgltf_result_success)
    {
        LOGGER_ERROR("Unable to prepare extension data. Error code: %d\n", res);
        return false;
    }

    jsonData[extSize] = '\0';

    // create json file from data blob - using cgltf implementation here
    jsmn_parser jsonParser;
    jsmn_init(&jsonParser);
    const int tokenCount = jsmn_parse(&jsonParser, jsonData, extSize + 1, nullptr, 0);
    if (tokenCount < 1)
    {
        // not an error - just no data
        return true;
    }

    // we know the token size, so allocate memory for this and get the tokens
    std::vector<jsmntok_t> tokenData;
    jsmn_parse(&jsonParser, jsonData, extSize, tokenData.data(), tokenCount);

    // now convert everything to a string for easier storage
    std::vector<std::string_view> temp;
    for (const jsmntok_t& token : tokenData)
    {
        std::string_view startStr {reinterpret_cast<const char*>(jsonData[token.start])};
        std::string_view endStr {std::to_string(token.end - token.start).c_str()};
        temp.emplace_back(startStr);
        temp.emplace_back(endStr);
    }

    // should be divisible by 2 otherwise we will overflow when creating the
    // output
    if ((temp.size() % 2) != 0)
    {
        LOGGER_ERROR("Error while parsing tokens. Invalid size.\n");
        return false;
    }

    // create the output - <extension name, value>
    for (size_t i = 0; i < temp.size(); i += 2)
    {
        extensions_.emplace(temp[i], temp[i + 1]);
    }

    return true;
}

// =====================================================================================================================================================

GltfModel::GltfModel() : extensions_(std::make_unique<GltfExtension>()) {}
GltfModel::~GltfModel() {}

NodeInfo* GltfModel::getNode(std::string_view id)
{
    NodeInfo* foundNode = nullptr;
    for (auto& node : nodes)
    {
        foundNode = node->getNode(id);
        if (foundNode)
        {
            break;
        }
    }
    return foundNode;
}

uint8_t* GltfModel::getAttributeData(const cgltf_attribute* attrib, size_t& stride)
{
    const cgltf_accessor* accessor = attrib->data;
    stride = accessor->buffer_view->stride; // the size of each sub blob
    if (!stride)
    {
        stride = accessor->stride;
    }
    assert(stride);
    return static_cast<uint8_t*>(accessor->buffer_view->buffer->data) + accessor->offset +
        accessor->buffer_view->offset;
}

void GltfModel::lineariseRecursive(cgltf_node& node, size_t index)
{
    // nodes a lot of the time don't possess a name, so we can't rely on this
    // for identifying nodes. So. we will use a stringifyed id instead
    node.name = const_cast<char*>(std::to_string(index).c_str());

    linearisedNodes_.emplace_back(&node);

    cgltf_node** childEnd = node.children + node.children_count;
    for (cgltf_node* const* child = node.children; child < childEnd; ++child)
    {
        lineariseRecursive(**child, index);
    }
}

void GltfModel::lineariseNodes(cgltf_data* data)
{
    size_t index = 0;

    cgltf_scene* sceneEnd = data->scenes + data->scenes_count;
    for (const cgltf_scene* scene = data->scenes; scene < sceneEnd; ++scene)
    {
        cgltf_node* const* nodeEnd = scene->nodes + scene->nodes_count;
        for (cgltf_node* const* node = scene->nodes; node < nodeEnd; ++node)
        {
            lineariseRecursive(**node, index);
        }
    }
}

bool GltfModel::load(const std::filesystem::path& filename)
{
    // no additional options required
    cgltf_options options = {};

    std::filesystem::path modelPath = filename;
    if (!modelDir_.empty())
    {
        modelPath = modelDir_.c_str() / modelPath;
    }
    auto platformPath = modelPath.make_preferred();

    cgltf_result res = cgltf_parse_file(&options, platformPath.string().c_str(), &gltfData_);
    if (res != cgltf_result_success)
    {
        LOGGER_ERROR("Unable to open gltf file %s. Error code: %d\n", platformPath.c_str(), res);
        return false;
    }

    // the buffers need parsing separately
    res = cgltf_load_buffers(&options, gltfData_, (const char*)modelPath.c_str());
    if (res != cgltf_result_success)
    {
        LOGGER_ERROR(
            "Unable to open gltf file data for %s. Error code: %d\n", filename.c_str(), res);
        return false;
    }

    return true;
}

bool GltfModel::build()
{
    if (!gltfData_)
    {
        LOGGER_ERROR("Looks like you need to load a gltf file before calling "
                     "this function!\n");
        return false;
    }

    // joints and animation samplers point at nodes in the hierachy. To link our
    // node hierachy the model nodes have there ids. We also linearise the cgltf
    // nodes in a map, along with an id which matches the pattern of the model
    // nodes. To find a model node from a cgltf node - find the id of the cgltf
    // in the linear map, and then search for this id in the model node hierachy
    // and return the pointer.
    lineariseNodes(gltfData_);

    // prepare any extensions which may be included with this model
    extensions_->build(gltfData_->extras, *gltfData_);

    // for each scene, visit each node in that scene
    cgltf_scene* sceneEnd = gltfData_->scenes + gltfData_->scenes_count;
    for (const cgltf_scene* scene = gltfData_->scenes; scene < sceneEnd; ++scene)
    {
        cgltf_node* const* nodeEnd = scene->nodes + scene->nodes_count;
        for (cgltf_node* const* node = scene->nodes; node < nodeEnd; ++node)
        {
            auto newNode = std::make_unique<NodeInstance>();
            if (!newNode->prepare(*node, *this))
            {
                return false;
            }

            nodes.emplace_back(std::move(newNode));
        }
    }

    return true;
}

GltfExtension& GltfModel::getExtensions() { return *extensions_; }

// ================ user front-end functions =========================

GltfModel& GltfModel::setWorldTrans(const mathfu::vec3& trans)
{
    wTrans_ = trans;
    return *this;
}


GltfModel& GltfModel::setWorldScale(const mathfu::vec3& scale)
{
    wScale_ = scale;
    return *this;
}

GltfModel& GltfModel::setWorldRotation(const mathfu::quat& rot)
{
    wRotation_ = rot;
    return *this;
}

void GltfModel::setDirectory(const std::filesystem::path& dirPath) { modelDir_ = dirPath; }

} // namespace yave
