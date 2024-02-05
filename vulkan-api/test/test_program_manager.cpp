#include <vulkan-api/program_manager.h>

#include <gtest/gtest.h>

TEST(ProgramManagerTests, ShaderParseTest)
{
    std::string shaderCode = {R"(
        #include "/shader/path"
        #include "/shader/another/path"

        void randomFunc()
        {
        }

        void main()
        {
            randomFunc();
        }
    )"};

    std::string uboBlock = {R"(
        layout (set = 0, binding = 0) uniform_buffer Ubo
        {
            int param1;
        } test_ubo;
)"};

    vkapi::ShaderProgram prog;
    prog.parseShader(shaderCode.c_str());
    prog.addAttributeBlock(uboBlock);

    std::string output = prog.build();

    std::string expectedOutput = {R"(#version 460

       #include "/shader/path"

       #include "/shader/another/path"


        layout (set = 0, binding = 0) uniform_buffer Ubo
        {
            int param1;
        } test_ubo;

       void randomFunc()
       {
       }

       void main()
       {
           randomFunc();
       }
   )"};

    ASSERT_STREQ(output.c_str(), expectedOutput.c_str());
}