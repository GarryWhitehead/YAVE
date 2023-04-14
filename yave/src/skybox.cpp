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

#include "skybox.h"

#include "backend/enums.h"
#include "camera.h"
#include "engine.h"
#include "mapped_texture.h"
#include "render_primitive.h"
#include "renderable.h"
#include "scene.h"
#include "material.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "managers/renderable_manager.h"
#include "utility/assertion.h"
#include "utility/murmurhash.h"
#include "vulkan-api/image.h"
#include "vulkan-api/pipeline.h"
#include "vulkan-api/program_manager.h"
#include "vulkan-api/resource_cache.h"
#include "vulkan-api/texture.h"
#include "vulkan-api/utility.h"
#include "yave/texture_sampler.h"

#include <mathfu/glsl_mappings.h>

#include <array>

namespace yave
{

ISkybox::ISkybox(IEngine& engine)
    : engine_(engine), blurFactor_(0.0f)
{
    auto* rendManager = engine_.getRenderableManagerI();
    skyboxObj_ = engine_.createObjectI();
    material_ = rendManager->createMaterialI();
}

void ISkybox::buildI(ICamera& cam)
{
    // cube vertices
    const std::array<mathfu::vec3, 8> vertices {
        mathfu::vec3 {-1.0f, -1.0f, 1.0f},
        mathfu::vec3 {1.0f, -1.0f, 1.0f},
        mathfu::vec3 {1.0f, 1.0f, 1.0f},
        mathfu::vec3 {-1.0f, 1.0f, 1.0f},
        mathfu::vec3 {-1.0f, -1.0f, -1.0f},
        mathfu::vec3 {1.0f, -1.0f, -1.0f},
        mathfu::vec3 {1.0f, 1.0f, -1.0f},
        mathfu::vec3 {-1.0f, 1.0f, -1.0f}};

    // cube indices
    // clang-format off
    constexpr std::array<uint32_t, 36> indices {
        0, 1, 2, 2, 3, 0,       // front
        1, 5, 6, 6, 2, 1,       // right side
        7, 6, 5, 5, 4, 7,       // left side
        4, 0, 3, 3, 7, 4,       // bottom
        4, 5, 1, 1, 0, 4,       // back
        3, 2, 6, 6, 7, 3        // top
    };
    // clang-format on

    auto& driver = engine_.driver();
    auto* rendManager = engine_.getRenderableManagerI();

    TextureSampler sampler(
        backend::SamplerFilter::Linear,
        backend::SamplerFilter::Linear,
        backend::SamplerAddressMode::ClampToEdge,
        1,
        10);

    material_->addImageTexture(
        engine_.driver(),
        cubeTexture_,
        Material::ImageType::BaseColour,
        sampler.get(), 0);

    auto render = engine_.createRenderableI();
    auto vBuffer = engine_.createVertexBufferI();
    auto iBuffer = engine_.createIndexBufferI();
    auto prim = engine_.createRenderPrimitiveI();
    render->setPrimitiveCount(1);
    render->skipVisibilityChecks();

    vBuffer->addAttribute(
        VertexBuffer::BindingType::Position,
        backend::BufferElementType::Float3);
    vBuffer->buildI(driver, vertices.size() * sizeof(mathfu::vec3), (void*)vertices.data());
    iBuffer->buildI(
        driver, indices.size(), (void*)indices.data(), backend::IndexBufferType::Uint32);
    prim->addMeshDrawDataI(indices.size(), 0);

    prim->setVertexBuffer(vBuffer);
    prim->setIndexBuffer(iBuffer);
    render->setPrimitive(prim, 0);

    material_->addUboParam(
        "blurFactor",
        backend::BufferElementType::Float,
        sizeof(float),
        &blurFactor_);

    material_->setCullMode(backend::CullMode::Front);
    material_->setViewLayer(0x4);
    prim->setMaterialI(material_);

    rendManager->buildI(render, skyboxObj_, {}, "skybox.glsl");
}

ISkybox& ISkybox::setCubeMap(IMappedTexture* cubeTexture)
{
    ASSERT_FATAL(cubeTexture, "The cube texture is nullptr.");
    cubeTexture_ = cubeTexture;

    return *this;
}

void ISkybox::update(ICamera& camera) noexcept
{
    // update the push constant values
    material_->updateUboParam("blurFactor", (void*)&blurFactor_);
}

// ======================== client api =======================

Skybox::Skybox() {}
Skybox::~Skybox() {}

void ISkybox::setTexture(Texture* texture) noexcept
{
    setCubeMap(reinterpret_cast<IMappedTexture*>(texture));
}

void ISkybox::setBlurFactor(float blur) noexcept { blurFactor_ = blur; }

void ISkybox::build(Camera* camera) 
{ 
    buildI(*(reinterpret_cast<ICamera*>(camera))); 
}

} // namespace yave
