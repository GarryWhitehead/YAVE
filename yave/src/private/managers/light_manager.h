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
#include "render_graph/render_graph.h"
#include "samplerset.h"
#include "uniform_buffer.h"
#include "yave/light_manager.h"

#include <mathfu/glsl_mappings.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace yave
{

// forward declerations
class Object;
class ICamera;
class IEngine;
class IVertexBuffer;
class IIndexBuffer;
class IRenderPrimitive;
class IIndirectLight;
class IScene;

struct LightInstance
{
    LightManager::Type type;

    // set by visibility checks
    bool isVisible;

    // Set by a call to update.
    mathfu::mat4 mvp;

    mathfu::vec3 position;
    mathfu::vec3 target;
    mathfu::vec3 colour;

    float fov;
    float intensity;

    struct SpotLightInfo
    {
        float scale;
        float offset;
        float cosOuterSquared;
        float outer;
        float radius;
    } spotLightInfo;
};

class ILightManager : public ComponentManager, public LightManager
{
public:
    static constexpr int MaxLightCount = 50;
    static constexpr int EndOfBufferSignal = 0xFF;

    static constexpr int SamplerPositionBinding = 0;
    static constexpr int SamplerColourBinding = 1;
    static constexpr int SamplerNormalBinding = 2;
    static constexpr int SamplerPbrBinding = 3;
    static constexpr int SamplerEmissiveBinding = 4;
    static constexpr int SamplerIrradianceBinding = 5;
    static constexpr int SamplerSpecularBinding = 6;
    static constexpr int SamplerBrdfBinding = 7;

    // This must mirror the lighting struct on the shader.
    struct LightSsbo
    {
        mathfu::mat4 viewMatrix;
        mathfu::vec4 pos;
        mathfu::vec4 direction;
        mathfu::vec4 colour;
        int type;
        float fallOut;
        float scale;
        float offset;
    };

    enum class Variants
    {
        IblContribution,
        __SENTINEL__
    };

    explicit ILightManager(IEngine& engine);
    ~ILightManager();

    void update(const ICamera& camera);

    void setVariant(Variants variant);
    [[maybe_unused]] void removeVariant(Variants variant);
    vkapi::VDefinitions createShaderVariants();

    void prepare(IScene* scene);

    void updateSsbo(std::vector<LightInstance*>& lights);

    static void setIntensity(float intensity, LightManager::Type type, LightInstance& light);
    static void calculateSpotCone(float outerCone, float innerCone, LightInstance& spotLight);
    static void setRadius(float fallout, LightInstance& light);

    void setIntensity(float intensity, Object& obj);

    void setFallout(float fallout, Object& obj);
    void setPosition(const mathfu::vec3& pos, Object& obj);
    void setTarget(const mathfu::vec3& target, Object& obj);
    void setColour(const mathfu::vec3& col, Object& obj);
    void setFov(float fov, Object& obj);

    void setSunAngularRadius(float radius, LightInstance& light);
    void setSunHaloSize(float size, LightInstance& light);
    void setSunHaloFalloff(float falloff, LightInstance& light);

    void createLight(const CreateInfo& ci, Object& obj, LightManager::Type type);

    [[maybe_unused]] size_t getLightCount() const;

    LightInstance* getLightInstance(Object& obj);

    void enableAmbientLight() noexcept;

    void destroy(const Object& handle);

    LightInstance* getDirLightParams() noexcept;

    rg::RenderGraphHandle render(
        rg::RenderGraph& rGraph,
        IScene& scene,
        uint32_t width,
        uint32_t height,
        vk::Format depthFormat);

    float getSunAngularRadius() const noexcept { return sunAngularRadius_; }
    float getSunHaloSize() const noexcept { return sunHaloSize_; }
    float getSunHaloFalloff() const noexcept { return sunHaloFalloff_; }

private:
    IEngine& engine_;

    std::vector<std::unique_ptr<LightInstance>> lights_;

    // Shader settings
    std::unique_ptr<StorageBuffer> ssbo_;
    SamplerSet samplerSets_;

    // Used for generating the ssbo light data per frame.
    LightSsbo ssboBuffer_[MaxLightCount + 1];

    util::BitSetEnum<Variants> variants_;

    // keep track of the scene the light manager was last prepared for
    IScene* currentScene_;

    // if a directional light is set then keep track of its object
    // as the light parameters are also held by the scene ubo
    Object dirLightObj_;

    float sunAngularRadius_;
    float sunHaloSize_;
    float sunHaloFalloff_;

    // ================= vulkan backend =======================

    vkapi::ShaderProgramBundle* programBundle_;
};

} // namespace yave