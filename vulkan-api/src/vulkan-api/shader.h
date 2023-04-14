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

#include "buffer.h"
#include "common.h"
#include "utility/compiler.h"
#include "utility/cstring.h"
#include "backend/enums.h"

#include <shaderc/shaderc.hpp>
#include <shaderc_util/file_finder.h>
#include <spirv_glsl.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vkapi
{
// forward decleartions
class VkContext;

using VDefinitions = std::unordered_map<std::string, uint8_t>;

// This is a callback class to impelement shaderc's include interface
class IncludeInterface : public shaderc::CompileOptions::IncluderInterface
{
public:
    explicit IncludeInterface(const shaderc_util::FileFinder* fileFinder)
        : fileFinder(*fileFinder)
    {
    }

    ~IncludeInterface() override;

    shaderc_include_result* GetInclude(
        const char* requested_source,
        shaderc_include_type type,
        const char* requesting_source,
        size_t include_depth) override;

    // Releases an include result.
    void ReleaseInclude(shaderc_include_result* includeResult) override;

    // Returns a reference to the member storing the set of included files.
    const std::unordered_set<std::string>& filePathTrace() const
    {
        return includedFiles;
    }

private:
    // The full path and content of a source file.
    struct FileInfo
    {
        const std::string full_path;
        std::vector<char> contents;
    };

    // The set of full paths of included files.
    std::unordered_set<std::string> includedFiles;

    // required for dealing with #include
    shaderc_util::FileFinder fileFinder;
};

/**
 * @brief Filled through reflection of the shader code, this struct holds
 * information required to create the relevant Vulkan objects.
 */
struct ShaderBinding
{
    static constexpr int MAX_DESCRIPTOR_SET_COUNT = 20;

    struct Attribute
    {
        uint32_t location;
        uint32_t stride;
        vk::Format format;
    };

    struct DescriptorLayout
    {
        ::util::CString name;
        uint32_t binding;
        uint32_t set;
        uint32_t range;
        vk::DescriptorType type;
        vk::ShaderStageFlags stage;
    };

    std::vector<Attribute> stageInputs;
    std::vector<Attribute> stageOutputs;
    std::vector<DescriptorLayout> descLayouts;
    size_t pushBlockSize = 0;
};

class Shader
{
public:
    
    Shader(VkContext& context, const backend::ShaderStage type);
    ~Shader();

    bool loadAsBinary(const std::filesystem::path shaderPath, uint32_t* output);

    /**
     * Gathers the createInfo for all shaders into one blob in a format needed
     * by the pipeline
     */
    vk::PipelineShaderStageCreateInfo& get() { return createInfo_; }

    /**
     * @brief Converts the StageType enum into a vulkan recognisible format
     */
    static vk::ShaderStageFlagBits getStageFlags(backend::ShaderStage type);

    static util::CString shaderTypeToString(backend::ShaderStage type);

    /**
     * @brief From the width and vecsize derive the vk format
     * @param width The width of each vec in bytes i.e. int32, float32
     * @param vecSize the number of each type i.e. vec3
     * @param type From spirv reflection - the type - i.e. float, integer
     */
    static std::tuple<vk::Format, uint32_t> getVkFormatFromSize(
        uint32_t width, uint32_t vecSize, const spirv_cross::SPIRType type);

    /**
     * @brief compiles the specified code into glsl bytecode, and then creates
     * a shader module and createInfo ready for using with a vulkan pipeline
     */
    bool compile(std::string shaderCode, const VDefinitions& variants);

    /**
     * @brief Performs reflection on the shader code and fills the shader
     * binding information. This will be used to create the relevant Vulkan
     * objects.
     * @param shaderPath A pointer to a compiled shader data blob.
     */
    void reflect(const uint32_t* shaderCode, uint32_t size);

    // ================== getters ========================

    ShaderBinding& getShaderBinding() { return resourceBinding_; }
    vk::ShaderModule getShaderModule() { return module_; }
    vk::PipelineShaderStageCreateInfo& getCreateInfo() { return createInfo_; }
    backend::ShaderStage getStageType() const noexcept { return type_; }

private:
    VkContext& context_;

    /// all the bindings for this shader - generated via the @p reflect call.
    ShaderBinding resourceBinding_;

    /// a vulkan shader module object for use with a pipeline
    vk::ShaderModule module_;

    /// The type of this shader see @p backend::ShaderStage
    backend::ShaderStage type_;

    vk::PipelineShaderStageCreateInfo createInfo_;
};

class ShaderCompiler
{
public:
    ShaderCompiler(std::string shaderCode, const backend::ShaderStage type);
    ~ShaderCompiler();

    bool compile(bool optimise);

    void addVariant(const std::string& variant, const uint8_t value)
    {
        definitions_.emplace(variant.c_str(), value);
    }

    uint32_t* getData() { return output_.data(); }

    size_t getSize() const { return output_.size(); }

    size_t getByteSize() const { return output_.size() * sizeof(uint32_t); }

private:
    std::vector<uint32_t> output_;

    shaderc_shader_kind kind_;
    std::string source_;
    std::string sourceName_;
    VDefinitions definitions_;

    // required for dealing with #include
    shaderc_util::FileFinder fileFinder_;
};

} // namespace vkapi
