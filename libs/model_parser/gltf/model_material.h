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
#include <utility/colour.h>
#include <utility/cstring.h>

#include <cstdint>
#include <filesystem>

namespace yave
{
// forward declerations
class MeshInstance;
class GltfExtension;

class ModelMaterial
{
public:
    enum TextureType : uint32_t
    {
        BaseColour,
        Normal,
        MetallicRoughness,
        Emissive,
        Occlusion,
        Count
    };

    enum class PbrPipeline
    {
        SpecularGlosiness,
        MetallicRoughness,
        None
    };

    ModelMaterial();
    ~ModelMaterial();

    std::filesystem::path getTextureUri(cgltf_texture_view& view);

    bool create(cgltf_material& mat, GltfExtension& extensions);

    static float convertToAlpha(cgltf_alpha_mode mode);
    static std::string textureTypeToStr(TextureType type);
    /**
     * @brief The main attributes for this material.
     */
    struct Attributes
    {
        util::Colour4 baseColour = util::Colour4 {1.0f};
        util::Colour4 emissive = util::Colour4 {1.0f};
        util::Colour4 diffuse = util::Colour4 {1.0f};
        util::Colour4 specular = util::Colour4 {0.0f};
        float metallic = 1.0f;
        float roughness = 1.0f;
        float alphaMask = 1.0f;
        float alphaMaskCutOff = 1.0f;
    };

    /**
     @brief the sampler details for each texture
     */
    struct Sampler
    {
        enum Filter
        {
            Nearest,
            Linear,
            Cubic
        };

        enum AddressMode
        {
            Repeat,
            MirroredRepeat,
            ClampToEdge,
            ClampToBorder,
            MirrorClampToEdge
        };

        Filter magFilter = Filter::Linear;
        Filter minFilter = Filter::Linear;
        AddressMode addressModeU = AddressMode::Repeat;
        AddressMode addressModeV = AddressMode::Repeat;
        AddressMode addressModeW = AddressMode::Repeat;
    };

private:
    Sampler::Filter getSamplerFilter(int filter);
    Sampler::AddressMode getAddressMode(int mode);

public:
    // ==== material data (public) ========

    /// used to identify this material.
    util::CString name_;

    Attributes attributes_;
    Sampler sampler_;

    struct TextureInfo
    {
        std::filesystem::path texturePath;
        TextureType type;
    };
    /// the paths for all textures that are used by the material
    std::vector<TextureInfo> textures_;

    /// the material pipeline to use
    PbrPipeline pipeline_ = PbrPipeline::None;

    bool doubleSided_ = false;
};

} // namespace yave
