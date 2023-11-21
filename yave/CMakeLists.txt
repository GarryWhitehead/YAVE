
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(YAVE STATIC)

target_include_directories(
    YAVE 
    PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${YAVE_ROOT_DIRECTORY}/YaveVulkanApi/src"
    "${YAVE_ROOT_DIRECTORY}/libs"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    
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
)

set (src_files
    src/camera.cpp
    src/engine.cpp
    src/index_buffer.cpp
    src/indirect_light.cpp
    src/private/engine.cpp
    src/private/camera.cpp
    src/private/scene.cpp
    src/private/renderer.cpp
    src/private/frustum.cpp
    src/private/skybox.cpp
    src/private/material.cpp
    src/private/render_primitive.cpp
    src/private/uniform_buffer.cpp
    src/private/samplerset.cpp
    src/private/mapped_texture.cpp
    src/private/renderable.cpp
    src/private/colour_pass.cpp
    src/private/render_queue.cpp
    src/private/vertex_buffer.cpp
    src/private/index_buffer.cpp
    src/private/object_manager.cpp
    src/private/indirect_light.cpp
    src/private/scene_ubo.cpp
    src/private/post_process.cpp
    src/private/compute.cpp
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
    src/private/camera.h
    src/private/frustum.h
    src/private/aabox.h
    src/private/skybox.h
    src/private/material.h
    src/private/object_instance.h
    src/private/uniform_buffer.h
    src/private/samplerset.h
    src/private/mapped_texture.h
    src/private/engine.h
    src/private/renderable.h
    src/private/scene.h
    src/private/renderable.h
    src/private/renderer.h
    src/private/render_primitive.h
    src/private/colour_pass.h
    src/private/render_queue.h
    src/private/index_buffer.h
    src/private/vertex_buffer.h
    src/private/object_manager.h
    src/private/indirect_light.h
    src/private/scene_ubo.h
    src/private/post_process.h
    src/private/compute.h
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
    YaveModelParser
    YaveUtility

    vectorial::vectorial
    mathfu::mathfu
    TBB::tbb
    spdlog::spdlog
    ${glfw3_LIBRARIES}
    YaveApp
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
