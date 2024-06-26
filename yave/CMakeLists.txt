
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(YAVE STATIC)

target_include_directories(
    YAVE 
    PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/private"
    
    PRIVATE
    ${glfw3_INCLUDE_DIRS}
    ${VulkanHeaders_INCLUDE_DIRS}
)

set (public_headers 
    include/yave/engine.h
    include/yave/scene.h
    include/yave/renderer.h
    include/yave/renderable.h
    include/yave/render_primitive.h
    include/yave/renderable_manager.h
    include/yave/transform_manager.h
    include/yave/light_manager.h
    include/yave/object.h
    include/yave/skybox.h
    include/yave/texture.h
    include/yave/camera.h
    include/yave/texture_sampler.h
    include/yave/material.h
    include/yave/vertex_buffer.h
    include/yave/index_buffer.h
    include/yave/object_manager.h
    include/yave/indirect_light.h
    include/yave/options.h
    include/yave/wave_generator.h
    include/yave/yave_api.h
)

set (src_files
    src/camera.cpp
    src/engine.cpp
    src/index_buffer.cpp
    src/indirect_light.cpp
    src/light_manager.cpp
    src/renderable.cpp
    src/material.cpp
    src/object_manager.cpp
    src/render_primitive.cpp
    src/renderable_manager.cpp
    src/scene.cpp
    src/renderer.cpp
    src/skybox.cpp
    src/texture.cpp
    src/compute.cpp
    src/scene_ubo.cpp
    src/uniform_buffer.cpp
    src/transform_manager.cpp
    src/vertex_buffer.cpp
    src/samplerset.cpp
    src/colour_pass.cpp
    src/render_queue.cpp
    src/frustum.cpp

    src/private/engine.cpp
    src/private/camera.cpp
    src/private/scene.cpp
    src/private/renderer.cpp
    src/private/skybox.cpp
    src/private/material.cpp
    src/private/render_primitive.cpp
    src/private/mapped_texture.cpp
    src/private/renderable.cpp
    src/private/vertex_buffer.cpp
    src/private/index_buffer.cpp
    src/private/object_manager.cpp
    src/private/indirect_light.cpp
    src/private/post_process.cpp
    src/private/wave_generator.cpp
    src/private/managers/renderable_manager.cpp
    src/private/managers/component_manager.cpp
    src/private/managers/renderable_manager.cpp
    src/private/managers/transform_manager.cpp
    src/private/managers/light_manager.cpp
    src/private/render_graph/render_graph.cpp  
    src/private/render_graph/render_graph_builder.cpp  
    src/private/render_graph/resources.cpp
    src/private/render_graph/render_pass_node.cpp
    src/private/render_graph/resource_node.cpp
    src/private/render_graph/dependency_graph.cpp
    src/private/render_graph/rendergraph_resource.cpp
    src/private/render_graph/backboard.cpp
)

set (hdr_files
    src/compute.h
    src/frustum.h
    src/aabox.h
    src/uniform_buffer.h
    src/colour_pass.h
    src/render_queue.h
    src/scene_ubo.h
    src/samplerset.h
    src/object_instance.h

    src/private/camera.h
    src/private/skybox.h
    src/private/material.h
    src/private/mapped_texture.h
    src/private/engine.h
    src/private/renderable.h
    src/private/scene.h
    src/private/renderable.h
    src/private/renderer.h
    src/private/render_primitive.h
    src/private/index_buffer.h
    src/private/vertex_buffer.h
    src/private/object_manager.h
    src/private/indirect_light.h
    src/private/post_process.h
    src/private/wave_generator.h
    src/private/managers/component_manager.h
    src/private/managers/renderable_manager.h
    src/private/managers/transform_manager.h
    src/private/managers/light_manager.h
    src/private/render_graph/render_graph.h
    src/private/render_graph/render_graph_pass.h 
    src/private/render_graph/render_graph_builder.h
    src/private/render_graph/resources.h
    src/private/render_graph/render_pass_node.h
    src/private/render_graph/resource_node.h
    src/private/render_graph/dependency_graph.h
    src/private/render_graph/rendergraph_resource.h
    src/private/render_graph/backboard.h
    src/private/render_graph/render_graph_handle.h
)

target_sources(
    YAVE
    PUBLIC
    ${public_headers}
    PRIVATE
    ${src_files}
    ${hdr_files}
)

target_link_libraries(
    YAVE 
    PUBLIC
    YaveModelParser
    YaveVulkanApi
    YaveUtility

    vectorial::vectorial
    mathfu::mathfu
    TBB::tbb
    spdlog::spdlog
    ${glfw3_LIBRARIES}
)

target_compile_definitions(
    YAVE
    PRIVATE
    YAVE_SHADER_DIRECTORY="${YAVE_SHADER_DIRECTORY}"
    YAVE_VERBOSE_OUTPUT=${VERBOSE_OUTPUT}
    "$<$<PLATFORM_ID:Windows>:_USE_MATH_DEFINES;NOMINMAX;_CRT_SECURE_NO_WARNINGS>"  
)

# add common compile flags
yave_add_compiler_flags(TARGET YAVE)

# group source and header files
yave_source_group(
    TARGET YAVE
    ROOT_DIR "${YAVE_ROOT_DIRECTORY}/yave"
)

if (BUILD_TESTS)

    set (test_srcs
        test/main_test.cpp
        test/vulkan_helper.h
        test/vulkan_helper.cpp
        test/test_uniform_buffer.cpp
        test/test_compute.cpp
    )

    add_executable(YaveTest ${test_srcs})
    target_link_libraries(YaveTest PRIVATE GTest::GTest YAVE YaveVulkanApi YaveApp)
    set_target_properties(YaveTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${YAVE_TEST_DIRECTORY})

    add_test(
            NAME YaveTest
            COMMAND YaveTest
            WORKING_DIRECTORY ${YAVE_TEST_DIRECTORY}
    )

endif()
