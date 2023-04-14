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

#include "model_material.h"

#include "gltf_model.h"

namespace yave
{

ModelMaterial::ModelMaterial() {}

ModelMaterial::~ModelMaterial() {}

ModelMaterial::Sampler::Filter ModelMaterial::getSamplerFilter(int filter)
{
    Sampler::Filter result;
    switch (filter)
    {
        case 9728:
            result = Sampler::Filter::Nearest;
            break;
        case 9729:
            result = Sampler::Filter::Linear;
            break;
        case 9984:
            result = Sampler::Filter::Nearest;
            break;
        case 9985:
            result = Sampler::Filter::Nearest;
            break;
        case 9986:
            result = Sampler::Filter::Linear;
            break;
        case 9987:
            result = Sampler::Filter::Linear;
            break;
    }
    return result;
}

ModelMaterial::Sampler::AddressMode ModelMaterial::getAddressMode(int mode)
{
    Sampler::AddressMode result;

    switch (mode)
    {
        case 10497:
            result = Sampler::AddressMode::Repeat;
            break;
        case 33071:
            result = Sampler::AddressMode::ClampToEdge;
            break;
        case 33648:
            result = Sampler::MirroredRepeat;
            break;
    }
    return result;
}

float ModelMaterial::convertToAlpha(const cgltf_alpha_mode mode)
{
    float result;
    switch (mode)
    {
        case cgltf_alpha_mode_opaque:
            result = 0.0f;
            break;
        case cgltf_alpha_mode_mask:
            result = 1.0f;
            break;
        case cgltf_alpha_mode_blend:
            result = 2.0f;
            break;
    }
    return result;
}

std::string ModelMaterial::textureTypeToStr(TextureType type)
{
    std::string output;
    switch (type)
    {
        case ModelMaterial::TextureType::BaseColour:
            output = "BaseColour";
            break;
        case ModelMaterial::TextureType::Normal:
            output = "Normal";
            break;
        case ModelMaterial::TextureType::MetallicRoughness:
            output = "MetallicRoughness";
            break;
        case ModelMaterial::TextureType::Emissive:
            output = "Emissive";
            break;
        case ModelMaterial::TextureType::Occlusion:
            output = "Occlusion";
            break;
    }
    return output + "Sampler";
}


std::filesystem::path ModelMaterial::getTextureUri(cgltf_texture_view& view)
{
    // not guaranteed to have a texture or uri
    if (view.texture && view.texture->image)
    {
        // check whether this texture has a sampler. Otherwise use defeult
        // values
        if (view.texture->sampler)
        {
            sampler.magFilter =
                getSamplerFilter(view.texture->sampler->mag_filter);
            sampler.minFilter =
                getSamplerFilter(view.texture->sampler->min_filter);
            sampler.addressModeU =
                getAddressMode(view.texture->sampler->wrap_s);
            sampler.addressModeV =
                getAddressMode(view.texture->sampler->wrap_t);
        }

        // also set variant bit
        return std::filesystem::path {view.texture->image->uri};
    }
    return "";
}

bool ModelMaterial::create(
    cgltf_material& mat, GltfExtension& extensions)
{
    name = mat.name;

    // two pipelines, either specular glosiness or metallic roughness
    // according to the spec, metallic roughness shoule be preferred
    if (mat.has_pbr_specular_glossiness)
    {
        pipeline = PbrPipeline::SpecularGlosiness;

        {
            TextureInfo colourInfo {
                getTextureUri(mat.pbr_specular_glossiness.diffuse_texture),
                TextureType::BaseColour};
            textures.emplace_back(colourInfo);
        }

        {
            // instead of having a seperate entry for mr or specular gloss, the
            // two share the same slot. Should maybe be renamed to be more
            // transparent?
            TextureInfo mrInfo {
                getTextureUri(
                    mat.pbr_specular_glossiness.specular_glossiness_texture),
                TextureType::MetallicRoughness};
            textures.emplace_back(mrInfo);
        }
        // block.specularGlossiness =
        // mat.pbr_specular_glossiness.glossiness_factor;

        auto* df = mat.pbr_specular_glossiness.diffuse_factor;
        attributes.baseColour = {df[0], df[1], df[2], df[3]};
    }
    else if (mat.has_pbr_metallic_roughness)
    {
        pipeline = PbrPipeline::MetallicRoughness;

        {
            auto path =
                getTextureUri(mat.pbr_metallic_roughness.base_color_texture);
            if (!path.empty())
            {
                TextureInfo colourInfo {path, TextureType::BaseColour};
                textures.emplace_back(colourInfo);
            }
        }
        {
            auto path = getTextureUri(
                mat.pbr_metallic_roughness.metallic_roughness_texture);
            if (!path.empty())
            {
                TextureInfo mrInfo {path, TextureType::MetallicRoughness};
                textures.emplace_back(mrInfo);
            }
        }
        attributes.roughness = mat.pbr_metallic_roughness.roughness_factor;
        attributes.metallic = mat.pbr_metallic_roughness.metallic_factor;

        auto* bcf = mat.pbr_metallic_roughness.base_color_factor;
        attributes.baseColour = {bcf[0], bcf[1], bcf[2], bcf[3]};
    }

    {
        // normal texture
        auto path = getTextureUri(mat.normal_texture);
        if (!path.empty())
        {
            TextureInfo normInfo {path, TextureType::Normal};
            textures.emplace_back(normInfo);
        }
    }
    {
        // occlusion texture
        auto path = getTextureUri(mat.occlusion_texture);
        if (!path.empty())
        {
            TextureInfo occInfo {path, TextureType::Occlusion};
            textures.emplace_back(occInfo);
        }
    }
    {
        // emissive texture
        auto path = getTextureUri(mat.emissive_texture);
        if (!path.empty())
        {
            TextureInfo emInfo {path, TextureType::Emissive};
            textures.emplace_back(emInfo);
        }
    }
    // emissive factor
    auto* ef = mat.emissive_factor;
    attributes.emissive = {ef[0], ef[1], ef[2], 1.0f};

    // specular - extension
    util::CString data = extensions.find("specular");
    if (!data.empty())
    {
        mathfu::vec3 vec = GltfExtension::tokenToVec3(data);
        attributes.specular = {vec.x, vec.y, vec.z, 1.0f};
    }

    // alpha blending
    attributes.alphaMaskCutOff = mat.alpha_cutoff;
    attributes.alphaMask = convertToAlpha(mat.alpha_mode);

    // determines the type of culling required
    doubleSided = mat.double_sided;

    return true;
}

} // namespace yave
