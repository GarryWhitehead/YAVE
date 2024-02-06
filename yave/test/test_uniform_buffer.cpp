#include <gtest/gtest.h>
#include <uniform_buffer.h>

TEST(GpuBufferTests, UniformBuffer)
{
    const uint32_t set = 0;
    const uint32_t bind = 0;
    const std::string name = "TestUbo";
    const std::string aliasName = "test_ubo";
    yave::UniformBuffer ubo {set, bind, name, aliasName};

    ASSERT_TRUE(ubo.empty());

    float val = 2.0f;
    int val2 = 10;
    ubo.addElement("param1", backend::BufferElementType::Float, &val, 1, 1);
    ubo.addElement("param2", backend::BufferElementType::Int, &val2, 1, 1);
    // array type
    ubo.addElement("param3", backend::BufferElementType::Int, nullptr, 10, 1);

    // data size = float - 4bytes; integer - 4bytes; integer array - 4bytes * 10 = 44bytes.
    ASSERT_EQ(ubo.size(), 48);

    // Update an element value
    int newValue = 20;
    ubo.updateElement("param2", &newValue);

    std::string shaderStr = ubo.createShaderStr();

    std::string expected {"layout (set = 0, binding = 0) uniform TestUbo\n"
                          "{\n"
                          "\tfloat param1;\n"
                          "\tint param2;\n"
                          "\tint param3[10];\n"
                          "} test_ubo;\n"};
    ASSERT_STREQ(shaderStr.c_str(), expected.c_str());
}

TEST(GpuBufferTests, StorageBuffer)
{
    const uint32_t set = 0;
    const uint32_t bind = 0;
    const std::string name = "TestSSbo";
    const std::string aliasName = "test_ssbo";
    yave::StorageBuffer ssbo {
        yave::StorageBuffer::AccessType::ReadOnly, set, bind, name, aliasName};

    ASSERT_TRUE(ssbo.empty());

    ssbo.addElement("param1", backend::BufferElementType::Int, nullptr, 20, 1);
    // inner/outer array of zero should denote array of unlimited size
    ssbo.addElement("param2", backend::BufferElementType::Float, nullptr, 0, 0);

    // data size = integer array - 4bytes * 20 = 80bytes (param2 of unlimited size so isn't added).
    ASSERT_EQ(ssbo.size(), 80);

    std::string shaderStr = ssbo.createShaderStr();

    std::string expected {"layout (set = 0, binding = 0) readonly buffer TestSSbo\n"
                          "{\n"
                          "\tint param1[20];\n"
                          "\tfloat param2[];\n"
                          "} test_ssbo;\n"};
    ASSERT_STREQ(shaderStr.c_str(), expected.c_str());
}