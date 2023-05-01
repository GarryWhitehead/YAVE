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

#include "shader.h"

#include "context.h"
#include "pipeline.h"
#include "pipeline_cache.h"
#include "program_manager.h"
#include "utility/assertion.h"

#include <shaderc_util/io_shaderc.h>
#include <spdlog/spdlog.h>

#include <fstream>

namespace vkapi
{

IncludeInterface::~IncludeInterface() {}

shaderc_include_result* IncludeInterface::GetInclude(
    const char* requested_source,
    shaderc_include_type include_type,
    const char* requesting_source,
    size_t)
{
    const std::string full_path = (include_type == shaderc_include_type_relative)
        ? fileFinder.FindRelativeReadableFilepath(requesting_source, requested_source)
        : fileFinder.FindReadableFilepath(requested_source);

    if (full_path.empty())
    {
        SPDLOG_CRITICAL("Unable to find or open include file: {}\n", requested_source);
        return nullptr;
    }

    // Read the file and save its full path and contents into stable addresses.
    FileInfo* new_file_info = new FileInfo {full_path, {}};
    if (!shaderc_util::ReadFile(full_path, &(new_file_info->contents)))
    {
        SPDLOG_CRITICAL("Unable to read include file: {}\n", requested_source);
        return nullptr;
    }

    includedFiles.insert(full_path);

    return new shaderc_include_result {
        new_file_info->full_path.data(),
        new_file_info->full_path.length(),
        new_file_info->contents.data(),
        new_file_info->contents.size(),
        new_file_info};
}

void IncludeInterface::ReleaseInclude(shaderc_include_result* include_result)
{
    FileInfo* info = static_cast<FileInfo*>(include_result->user_data);
    delete info;
    delete include_result;
}

// ============================================================================================================================

shaderc_shader_kind getShaderKind(const backend::ShaderStage type)
{
    shaderc_shader_kind result;
    switch (type)
    {
        case backend::ShaderStage::Vertex:
            result = shaderc_shader_kind::shaderc_vertex_shader;
            break;
        case backend::ShaderStage::Fragment:
            result = shaderc_shader_kind::shaderc_fragment_shader;
            break;
        case backend::ShaderStage::Geometry:
            result = shaderc_shader_kind::shaderc_geometry_shader;
            break;
        case backend::ShaderStage::Compute:
            result = shaderc_shader_kind::shaderc_compute_shader;
            break;
    }

    return result;
}

void printShader(std::string code)
{
    std::stringstream ss(code);
    std::string line;
    uint32_t lineNum = 1;

    while (std::getline(ss, line, '\n'))
    {
        printf("%d:  %s\n", lineNum++, line.c_str());
    }
}

ShaderCompiler::ShaderCompiler(std::string shaderCode, const backend::ShaderStage type)
    : kind_(getShaderKind(type)), source_(shaderCode), sourceName_(YAVE_SHADER_DIRECTORY)
{
}

ShaderCompiler::~ShaderCompiler() {}

bool ShaderCompiler::compile(bool optimise)
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetGenerateDebugInfo();

    for (auto& define : definitions_)
    {
        options.AddMacroDefinition(define.first, std::to_string(define.second));
    }

    if (optimise)
    {
        options.SetOptimizationLevel(shaderc_optimization_level_size);
    }

    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    // we need to create a new instantation of the includer interface - using
    // the one from shaerc - though could use our own.
    std::unique_ptr<IncludeInterface> includer(new IncludeInterface(&fileFinder_));
    const auto& used_source_files = includer->filePathTrace();
    options.SetIncluder(std::move(includer));

    // shaderc requires a slash on the end of the source name string otherwise
    // it incorrectly resolves the path.
    // TODO: Should probably check whether the path already ends with a slash.
    sourceName_ += "/";

    auto result = compiler.CompileGlslToSpv(source_, kind_, sourceName_.c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        SPDLOG_CRITICAL(
            "Number of errors: {}; Number of warnings: {}",
            result.GetNumErrors(),
            result.GetNumWarnings());
        SPDLOG_CRITICAL("{}\n", result.GetErrorMessage().c_str());

        return false;
    }

    std::copy(result.cbegin(), result.cend(), back_inserter(output_));

    return true;
}

// ==================== Shader =========================

Shader::Shader(VkContext& context, const backend::ShaderStage type) : context_(context), type_(type)
{
}

Shader::~Shader() {}

util::CString Shader::shaderTypeToString(backend::ShaderStage type)
{
    util::CString result;
    switch (type)
    {
        case backend::ShaderStage::Vertex:
            result = "Vertex";
            break;
        case backend::ShaderStage::Fragment:
            result = "Fragment";
            break;
        case backend::ShaderStage::TesselationCon:
            result = "TesselationCon";
            break;
        case backend::ShaderStage::TesselationEval:
            result = "TesselationEval";
            break;
        case backend::ShaderStage::Geometry:
            result = "Geometry";
            break;
        case backend::ShaderStage::Compute:
            result = "Compute";
            break;
    }
    return result;
}

vk::ShaderStageFlagBits Shader::getStageFlags(backend::ShaderStage type)
{
    vk::ShaderStageFlagBits ret;
    switch (type)
    {
        case backend::ShaderStage::Vertex:
            ret = vk::ShaderStageFlagBits::eVertex;
            break;
        case backend::ShaderStage::Fragment:
            ret = vk::ShaderStageFlagBits::eFragment;
            break;
        case backend::ShaderStage::TesselationCon:
            ret = vk::ShaderStageFlagBits::eTessellationControl;
            break;
        case backend::ShaderStage::TesselationEval:
            ret = vk::ShaderStageFlagBits::eTessellationEvaluation;
            break;
        case backend::ShaderStage::Geometry:
            ret = vk::ShaderStageFlagBits::eGeometry;
            break;
        case backend::ShaderStage::Compute:
            ret = vk::ShaderStageFlagBits::eCompute;
            break;
    }
    return ret;
}

std::tuple<vk::Format, uint32_t>
Shader::getVkFormatFromSize(uint32_t width, uint32_t vecSize, const spirv_cross::SPIRType type)
{
    // TODO: add other base types and widths
    vk::Format format;
    uint32_t size = 0;

    spirv_cross::SPIRType::BaseType baseType = type.basetype;

    // floats
    if (baseType == spirv_cross::SPIRType::Float)
    {
        if (width == 32)
        {
            if (vecSize == 1)
            {
                format = vk::Format::eR32Sfloat;
                size = 4;
            }
            if (vecSize == 2)
            {
                format = vk::Format::eR32G32Sfloat;
                size = 8;
            }
            if (vecSize == 3)
            {
                format = vk::Format::eR32G32B32Sfloat;
                size = 12;
            }
            if (vecSize == 4)
            {
                format = vk::Format::eR32G32B32A32Sfloat;
                size = 16;
            }
        }
    }

    // signed integers
    else if (baseType == spirv_cross::SPIRType::Int)
    {
        if (width == 32)
        {
            if (vecSize == 1)
            {
                format = vk::Format::eR32Sint;
                size = 4;
            }
            if (vecSize == 2)
            {
                format = vk::Format::eR32G32Sint;
                size = 8;
            }
            if (vecSize == 3)
            {
                format = vk::Format::eR32G32B32Sint;
                size = 12;
            }
            if (vecSize == 4)
            {
                format = vk::Format::eR32G32B32A32Sint;
                size = 16;
            }
        }
    }

    return std::make_tuple(format, size);
}

bool Shader::compile(std::string shaderCode, const VDefinitions& variants)
{
    if (shaderCode.empty())
    {
        SPDLOG_ERROR("There is no shader code to process!");
        return false;
    }

    // compile into bytecode
    ShaderCompiler compiler(shaderCode, type_);

    // add definitions to compiler
    for (auto& [key, value] : variants)
    {
        compiler.addVariant(key, value);
    }

    // compile into bytecode ready for wrapping
    // TODO: Maybe make the optimisation boolean accessable outside of
    // this class.
    if (!compiler.compile(false))
    {
        printShader(shaderCode);
        return false;
    }

    reflect(
        reinterpret_cast<uint32_t*>(compiler.getData()),
        static_cast<uint32_t>(compiler.getByteSize()) / sizeof(uint32_t));

    // create the shader module
    vk::ShaderModuleCreateInfo shaderInfo({}, compiler.getByteSize(), compiler.getData());

    VK_CHECK_RESULT(context_.device().createShaderModule(&shaderInfo, nullptr, &module_));

    // create the wrapper - this will be used by the pipeline
    vk::ShaderStageFlagBits stage = getStageFlags(type_);
    createInfo_ = vk::PipelineShaderStageCreateInfo({}, stage, module_, "main", nullptr);

    return true;
}

bool Shader::loadAsBinary(const std::filesystem::path shaderPath, uint32_t* output)
{
    ASSERT_LOG(output);
    std::ifstream file(shaderPath, std::ios::in | std::ios_base::binary);
    if (file.is_open())
    {
        const size_t size = std::filesystem::file_size(shaderPath);
        file.read(reinterpret_cast<char*>(output), size);
        return true;
    }
    return false;
}

void Shader::reflect(const uint32_t* shaderCode, uint32_t size)
{
    // perform reflection on the shader
    spirv_cross::Compiler glsl(shaderCode, size);

    auto resources = glsl.get_shader_resources();

    // input attributes
    for (auto& attrib : resources.stage_inputs)
    {
        ShaderBinding::Attribute stageInput;
        stageInput.location = glsl.get_decoration(attrib.id, spv::DecorationLocation);

        auto& base_type = glsl.get_type(attrib.base_type_id);
        auto& member = glsl.get_type(base_type.self);

        uint32_t vecSize = member.vecsize;
        uint32_t width = glsl.get_type(attrib.type_id).width;

        const auto& [format, stride] = getVkFormatFromSize(width, vecSize, base_type);
        stageInput.stride = stride;
        stageInput.format = format;
        resourceBinding_.stageInputs.push_back(stageInput);
    }

    // output attributes
    for (auto& attrib : resources.stage_outputs)
    {
        ShaderBinding::Attribute stageOutput;
        stageOutput.location = glsl.get_decoration(attrib.id, spv::DecorationLocation);

        auto& base_type = glsl.get_type(attrib.base_type_id);
        auto& member = glsl.get_type(base_type.self);
        uint32_t vecSize = member.vecsize;
        uint32_t width = glsl.get_type(attrib.type_id).width;

        const auto& [format, stride] = getVkFormatFromSize(width, vecSize, base_type);
        stageOutput.stride = stride;
        stageOutput.format = format;
        resourceBinding_.stageOutputs.push_back(stageOutput);
    }

    // image samplers
    for (auto& sample : resources.sampled_images)
    {
        const uint32_t set = glsl.get_decoration(sample.id, spv::DecorationDescriptorSet);
        ASSERT_LOG(PipelineCache::SamplerSetValue == set);

        const uint32_t binding = glsl.get_decoration(sample.id, spv::DecorationBinding);
        ShaderBinding::DescriptorLayout desc;
        desc.set = set;
        desc.binding = binding;
        desc.type = vk::DescriptorType::eCombinedImageSampler;
        desc.name = ::util::CString {sample.name.c_str()};
        desc.stage = getStageFlags(type_);
        resourceBinding_.descLayouts.emplace_back(desc);
    }
    // unifom buffers
    for (auto& buffer : resources.uniform_buffers)
    {
        uint32_t set = glsl.get_decoration(buffer.id, spv::DecorationDescriptorSet);
        uint32_t binding = glsl.get_decoration(buffer.id, spv::DecorationBinding);
        ShaderBinding::DescriptorLayout desc;
        desc.set = set;
        desc.binding = binding;
        desc.type = set == PipelineCache::UboSetValue ? vk::DescriptorType::eUniformBuffer
                                                      : vk::DescriptorType::eUniformBufferDynamic;
        desc.name = ::util::CString {buffer.name.c_str()};
        desc.stage = getStageFlags(type_);
        desc.range = glsl.get_declared_struct_size(glsl.get_type(buffer.base_type_id));
        resourceBinding_.descLayouts.emplace_back(desc);
    }
    // storage buffers
    for (auto& buffer : resources.storage_buffers)
    {
        uint32_t set = glsl.get_decoration(buffer.id, spv::DecorationDescriptorSet);
        ASSERT_FATAL(
            set == PipelineCache::SsboSetValue,
            "Mismatch between reflected readout and defined set value for "
            "storage buffers.");
        uint32_t binding = glsl.get_decoration(buffer.id, spv::DecorationBinding);
        ShaderBinding::DescriptorLayout desc;
        desc.set = set;
        desc.binding = binding;
        // we don't yet support dynamic storage buffers
        desc.type = vk::DescriptorType::eStorageBuffer;
        desc.name = ::util::CString {buffer.name.c_str()};
        desc.stage = getStageFlags(type_);
        desc.range = glsl.get_declared_struct_size(glsl.get_type(buffer.base_type_id));
        resourceBinding_.descLayouts.emplace_back(desc);
    }

    // push blocks
    uint32_t id = resources.push_constant_buffers[0].id;
    auto elements = glsl.get_active_buffer_ranges(id);

    for (const auto& element : elements)
    {
        resourceBinding_.pushBlockSize += element.range;
    }

    // specialisation constants
    auto spec_constants = glsl.get_specialization_constants();
    for (auto& spec_constant : spec_constants)
    {
        // TODO
    }
}

} // namespace vkapi
