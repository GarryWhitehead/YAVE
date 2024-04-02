#include "vulkan_helper.h"

#include <compute.h>
#include <engine.h>
#include <gtest/gtest.h>
#include <mapped_texture.h>
#include <yave/texture_sampler.h>

TEST_F(VulkanHelper, ComputeShaderTest)
{
    std::string testShader = {R"(
        layout (local_Size_x = 16, local_Size_y = 1) in;

        void main()
        {
            uint idx = gl_GlobalInvocationID.x;
            if (idx > compute_ubo.N)
            {
                return;
            }
            output_ssbo.data[idx] = input_ssbo.data[idx];
        }
    )"};

    initDriver();
    auto* driver = getDriver();

    auto* engine = yave::IEngine::create(driver);

    yave::Compute compute {*engine, testShader.data()};

    constexpr int DataSize = 1000;
    std::array<int, DataSize> inputData;
    std::array<int, DataSize> outputData {5};
    outputData[3] = 5;
    for (size_t i = 0; i < DataSize; ++i)
    {
        inputData[i] = static_cast<int>(i) * 2;
    }

    auto& cmds = driver->getCommands();
    auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

    compute.addSsbo(
        "data",
        backend::BufferElementType::Int,
        yave::StorageBuffer::AccessType::ReadOnly,
        0,
        "input_ssbo",
        inputData.data(),
        DataSize);
    compute.addSsbo(
        "data",
        backend::BufferElementType::Int,
        yave::StorageBuffer::AccessType::ReadWrite,
        1,
        "output_ssbo",
        outputData.data(),
        DataSize);
    compute.addUboParam("N", backend::BufferElementType::Int, (void*)&DataSize);

    auto bundle = compute.build(*engine);
    driver->dispatchCompute(cmdBuffer, bundle, DataSize / 16 + 1, 1, 1);

    int hostData[DataSize];
    compute.downloadSsboData(*engine, 1, hostData);

    int diff = memcmp(hostData, inputData.data(), DataSize * sizeof(int));
    ASSERT_EQ(diff, 0);
}