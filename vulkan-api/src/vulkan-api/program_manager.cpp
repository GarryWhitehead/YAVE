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

#include "program_manager.h"

#include "driver.h"
#include "pipeline.h"
#include "utility/assertion.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>

namespace vkapi
{

void ShaderProgram::addAttributeBlock(const std::string& block)
{
    attributeBlocks_.push_back(block);
}

std::string ShaderProgram::build()
{
    std::string output = "#version 460\n\n";

    // add any include files - these are added at the top of the
    // shader file as the contents may be required by other members
    // declared within the rest of the code block.
    for (const auto& include : includes_)
    {
        output += include + "\n";
    }
    // append any additional blocks to the shader code. This consists of Ubos,
    // samplers, etc.
    for (const auto& block : attributeBlocks_)
    {
        output += block + "\n";
    }
    // add the main attributes
    output += attributeDescriptorBlock_;

    // add the material shader code if defined.
    if (!materialShaderBlock_.empty())
    {
        output += "\n" + materialShaderBlock_ + "\n";
    }
    // append the main shader code block
    output += mainStageBlock_;

    return output;
}

void ShaderProgram::parseMaterialShaderBlock(const std::vector<std::string>& lines, size_t& index)
{
    for (; index < lines.size(); ++index)
    {
        const std::string& line = lines[index];
        if (line.find("[[") != std::string::npos && line.find("]]") != std::string::npos)
        {
            break;
        }
        if (line.find("#include \"") != std::string::npos)
        {
            includes_.push_back(line);
            continue;
        }
        materialShaderBlock_ += lines[index] + "\n";
    }
}

void ShaderProgram::parseShader(const util::CString& shaderCode)
{
    // Its safe to assume that any descriptors will be before the
    // main code block.
    auto readLine = [&shaderCode](size_t& idx, const size_t charCount) -> std::string {
        std::string line;
        while (idx < charCount)
        {
            if (shaderCode[idx] == '\n')
            {
                ++idx;
                break;
            }
            line += shaderCode[idx++];
        }
        return line;
    };

    size_t idx = 0;
    size_t charCount = shaderCode.size();
    ASSERT_FATAL(charCount > 0, "Shader input code block has no code!");

    while (idx < charCount)
    {
        // Get the next line from the shader code block (gather all characters until the newline)
        std::string line = readLine(idx, charCount);

        if (line.find("#include \"") != std::string::npos)
        {
            // skip adding this to the code block. Include statements will
            // instead be added to the top of the completed text block.
            includes_.push_back(line);
            continue;
        }
        if (line.find("void main()") != std::string::npos)
        {
            mainStageBlock_ += "\n" + line;
            break;
        }
        attributeDescriptorBlock_ += line;
    }

    ASSERT_FATAL(idx != charCount, "Shader code block contains no main() source.");

    while (idx < charCount)
    {
        mainStageBlock_ += readLine(idx, charCount);
    }
}

void ShaderProgram::clearAttributes() noexcept { attributeBlocks_.clear(); }

ShaderProgramBundle::ShaderProgramBundle()
    : shaderId_(0), pipelineLayout_(std::make_unique<PipelineLayout>()), tesselationVertCount_(0)
{
}

ShaderProgramBundle::~ShaderProgramBundle() = default;

void ShaderProgramBundle::parseMaterialShader(const std::filesystem::path& shaderPath)
{
    std::string absolutePath = YAVE_SHADER_DIRECTORY "/materials/" + shaderPath.string();
    std::fstream file(absolutePath, std::ios::in);
    if (!file.is_open())
    {
        throw std::runtime_error("Error whilst loading material shader: " + shaderPath.string());
    }

    // we use the material shader as the hash for the key
    shaderId_ = util::murmurHash3(
        (const uint32_t*)shaderPath.string().c_str(), shaderPath.string().size(), 0);

    std::string line;
    std::vector<std::string> shaderLines;
    shaderLines.reserve(200);
    while (std::getline(file, line))
    {
        shaderLines.push_back(line);
    }

    size_t idx = 0;
    while (idx < shaderLines.size())
    {
        line = shaderLines[idx++];
        if (line.find("[[vertex]]") != std::string::npos)
        {
            ShaderProgram* prog = createProgram(backend::ShaderStage::Vertex);
            prog->parseMaterialShaderBlock(shaderLines, idx);
        }
        else if (line.find("[[fragment]]") != std::string::npos)
        {
            ShaderProgram* prog = createProgram(backend::ShaderStage::Fragment);
            prog->parseMaterialShaderBlock(shaderLines, idx);
        }
        else if (line.find("[[tesse-eval]]") != std::string::npos)
        {
            ShaderProgram* prog = createProgram(backend::ShaderStage::TesselationEval);
            prog->parseMaterialShaderBlock(shaderLines, idx);
        }
        else if (line.find("[[tesse-control]]") != std::string::npos)
        {
            ShaderProgram* prog = createProgram(backend::ShaderStage::TesselationCon);
            prog->parseMaterialShaderBlock(shaderLines, idx);
        }
    }
}

util::CString ShaderProgramBundle::loadShader(const util::CString& filename)
{
    std::filesystem::path absolutePath = YAVE_SHADER_DIRECTORY "/" + std::string(filename.c_str());

    std::fstream file(absolutePath, std::ios::in);
    if (!file.is_open())
    {
        SPDLOG_ERROR("Error whilst loading shader: {}", filename.c_str());
        return {};
    }

    std::string shaderExt = absolutePath.extension().string();
    if (shaderExt != ".frag" && shaderExt != ".vert" && shaderExt != ".comp" &&
        shaderExt != ".tesse" && shaderExt != ".tessc")
    {
        SPDLOG_ERROR(
            "Unsupported shader extension type: {} for file path: {}",
            shaderExt,
            absolutePath.c_str());
        return {};
    }

    std::string line;
    std::string finalCode;
    while (std::getline(file, line))
    {
        finalCode += line + "\n";
    }

    return finalCode.c_str();
}

void ShaderProgramBundle::buildShader(
    const util::CString& shaderCode, backend::ShaderStage shaderType)
{
    // prefer the material shader filename as the hash key. If this isn't set,
    // use the main shader filename.
    shaderId_ = !shaderId_
        ? util::murmurHash3((const uint32_t*)shaderCode.c_str(), shaderCode.size(), 0)
        : shaderId_;

    ShaderProgram* prog = createProgram(shaderType);
    prog->parseShader(shaderCode);
}

ShaderProgram* ShaderProgramBundle::createProgram(const backend::ShaderStage& type)
{
    size_t idx = static_cast<uint32_t>(type);
    if (programs_[idx])
    {
        return programs_[idx].get();
    }

    auto program = std::make_unique<ShaderProgram>();
    programs_[idx] = std::move(program);
    return programs_.back().get();
}

std::vector<vk::PipelineShaderStageCreateInfo> ShaderProgramBundle::getShaderStagesCreateInfo()
{
    std::vector<vk::PipelineShaderStageCreateInfo> output;
    output.reserve(6);

    for (size_t i = 0; i < static_cast<size_t>(backend::ShaderStage::Count); ++i)
    {
        if (!programs_[i])
        {
            output.emplace_back();
        }
        else
        {
            Shader* shader = programs_[i]->getShader();
            output.push_back(shader->getCreateInfo());
        }
    }
    return output;
}

void ShaderProgramBundle::addDescriptorBinding(
    uint32_t size, uint32_t binding, vk::Buffer buffer, vk::DescriptorType type)
{
    ASSERT_FATAL(buffer, "VkBuffer has not been initialised.");
    ASSERT_LOG(size > 0);
    descBindInfo_.push_back({binding, buffer, size, type});
}

ShaderProgram* ShaderProgramBundle::getProgram(backend::ShaderStage type) noexcept
{
    ShaderProgram* prog = programs_[util::ecast(type)].get();
    if (prog)
    {
        return prog;
    }
    // If no program has been registered for this stage, create a new one.
    return createProgram(type);
}

bool ShaderProgramBundle::hasProgram(backend::ShaderStage type) noexcept
{
    ShaderProgram* prog = programs_[util::ecast(type)].get();
    if (prog)
    {
        return true;
    }
    return false;
}

void ShaderProgramBundle::setImageSampler(
    const TextureHandle& handle, uint8_t binding, vk::Sampler sampler)
{
    ASSERT_FATAL(handle, "Invalid texture handle.");
    ASSERT_FATAL(
        binding < PipelineCache::MaxSamplerBindCount, "Binding of %d is out of bounds.", binding);
    imageSamplers_[binding] = {handle, sampler};
}

void ShaderProgramBundle::setStorageImage(const TextureHandle& handle, uint8_t binding)
{
    ASSERT_FATAL(handle, "Invalid texture handle.");
    ASSERT_FATAL(
        binding < PipelineCache::MaxStorageImageBindCount,
        "Binding of %d is out of bounds.",
        binding);
    storageImages_[binding] = handle;
}

void ShaderProgramBundle::setPushBlockData(backend::ShaderStage stage, void* data)
{
    ASSERT_FATAL(data, "Pushblock data is NULL.");
    ASSERT_FATAL(pushBlock_, "Trying to set push block data when it's not initialised.");
    pushBlock_[util::ecast(stage)]->data = data;
}

void ShaderProgramBundle::addRenderPrimitive(
    vk::PrimitiveTopology topo,
    vk::IndexType indexBufferType,
    uint32_t indicesCount,
    uint32_t indicesOffset,
    VkBool32 primRestart)
{
    renderPrim_.primitiveRestart = primRestart;
    renderPrim_.topology = topo;
    renderPrim_.indexBufferType = indexBufferType;
    renderPrim_.indicesCount = indicesCount;
    renderPrim_.offset = indicesOffset;
}

void ShaderProgramBundle::addRenderPrimitive(
    vk::PrimitiveTopology topo, uint32_t vertexCount, VkBool32 primRestart)
{
    renderPrim_.primitiveRestart = primRestart;
    renderPrim_.topology = topo;
    renderPrim_.vertexCount = vertexCount;
}

void ShaderProgramBundle::addRenderPrimitive(uint32_t vertexCount)
{
    renderPrim_.vertexCount = vertexCount;
}

void ShaderProgramBundle::setTesselationVertCount(size_t count) noexcept
{
    tesselationVertCount_ = count;
}

void ShaderProgramBundle::setScissor(
    uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset)
{
    scissor_ = vk::Rect2D {
        {static_cast<int32_t>(xOffset), static_cast<int32_t>(yOffset)}, {width, height}};
}

void ShaderProgramBundle::setViewport(
    uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    ASSERT_LOG(width > 0);
    ASSERT_LOG(height > 0);
    viewport_ =
        vk::Viewport {static_cast<float>(width), static_cast<float>(height), minDepth, maxDepth};
}

void ShaderProgramBundle::addTextureSampler(vk::Sampler sampler, uint32_t binding)
{
    ASSERT_FATAL(
        binding < PipelineCache::MaxSamplerBindCount,
        "Binding value of %d exceeds the max binding count.",
        binding);
    imageSamplers_[binding].sampler = sampler;
}

void ShaderProgramBundle::createPushBlock(size_t size, backend::ShaderStage stage)
{
    uint32_t stageValue = util::ecast(stage);
    if (!pushBlock_[stageValue])
    {
        pushBlock_[stageValue] = std::make_unique<PipelineLayout::PushBlockBindParams>();
    }
    pushBlock_[stageValue]->stage = Shader::getStageFlags(stage);
    pushBlock_[stageValue]->size = size;
}

void ShaderProgramBundle::clear() noexcept
{
    descBindInfo_.clear();
    pipelineLayout_->clearDescriptors();

    for (size_t i = 0; i < util::ecast(backend::ShaderStage::Count); ++i)
    {
        if (programs_[i])
        {
            programs_[i]->clearAttributes();
        }
    }
}

PipelineLayout& ShaderProgramBundle::getPipelineLayout() noexcept { return *pipelineLayout_; }

ProgramManager::ProgramManager(VkDriver& driver) : driver_(driver) {}
ProgramManager::~ProgramManager() = default;

Shader* ProgramManager::findCachedShaderVariant(const CachedKey& key)
{
    auto iter = shaderCache_.find({key});
    if (iter != shaderCache_.end())
    {
        return iter->second.get();
    }
    return nullptr;
}

Shader* ProgramManager::compileShader(
    const std::string& shaderCode,
    const backend::ShaderStage& type,
    const VDefinitions& variants,
    const CachedKey& key)
{
    std::unique_ptr<Shader> shader = std::make_unique<Shader>(driver_.context(), type);
    if (!shader->compile(shaderCode, variants))
    {
        return nullptr;
    }

    // save for returning back to the caller
    Shader* output = shader.get();

    // cache for later use
    shaderCache_.insert({key, std::move(shader)});

    return output;
}

ShaderProgramBundle* ProgramManager::createProgramBundle()
{
    auto bundle = std::make_unique<ShaderProgramBundle>();
    ASSERT_LOG(bundle);
    programBundles_.emplace_back(std::move(bundle));
    return programBundles_.back().get();
}

Shader* ProgramManager::findShaderVariantOrCreate(
    const VDefinitions& variants,
    backend::ShaderStage type,
    vk::PrimitiveTopology topo,
    ShaderProgramBundle* bundle,
    uint64_t variantBits)
{
    // check whether the required variant shader is in the cache and use this if so.
    CachedKey key {};
    key.shaderId = bundle->getShaderId();
    key.variantBits = variantBits;
    key.topology = (uint32_t)topo;
    key.shaderStage = (uint32_t)type;

    Shader* shader = findCachedShaderVariant(key);
    if (!shader)
    {
        auto shaderCodeBlock = bundle->getProgram(type)->build();
        shader = compileShader(shaderCodeBlock, type, variants, key);
        if (!shader)
        {
            throw std::runtime_error("Error whilst compiling shader.");
        }
    }

    // update the pipeline layout
    ShaderBinding& binding = shader->getShaderBinding();
    auto& plineLayout = bundle->getPipelineLayout();

    for (const auto& layout : binding.descLayouts)
    {
        plineLayout.addDescriptorLayout(layout.set, layout.binding, layout.type, layout.stage);
    }

    if (binding.pushBlockSize > 0)
    {
        // Push constant details for the pipeline layout
        plineLayout.addPushConstant(type, binding.pushBlockSize);
        bundle->createPushBlock(binding.pushBlockSize, type);
    }
    return shader;
}


} // namespace vkapi
