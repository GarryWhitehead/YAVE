
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

target_sources(
    YAVE
    PUBLIC
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
    
    PRIVATE
    src/engine.cpp
    src/camera.cpp
    src/scene.cpp
    src/renderer.cpp
    src/frustum.cpp
    src/skybox.cpp
    src/material.cpp
    src/render_primitive.cpp
    src/uniform_buffer.cpp
    src/samplerset.cpp
    src/mapped_texture.cpp
    src/renderable.cpp
    src/object_instance.cpp
    src/colour_pass.cpp
    src/render_queue.cpp
    src/vertex_buffer.cpp
    src/index_buffer.cpp
    src/managers/renderable_manager.cpp
    src/managers/component_manager.cpp
    src/managers/renderable_manager.cpp
    src/managers/transform_manager.cpp
    src/managers/light_manager.cpp
    src/render_graph/render_graph.cpp  
    src/render_graph/render_graph_builder.cpp  
    src/render_graph/resources.cpp
    src/render_graph/render_pass_node.cpp
    src/render_graph/resource_node.cpp
    src/render_graph/dependency_graph.cpp
    src/render_graph/rendergraph_resource.cpp
    src/render_graph/backboard.cpp

    src/camera.h
    src/frustum.h
    src/aabox.h
    src/skybox.h
    src/material.h
    src/object_instance.h
    src/uniform_buffer.h
    src/samplerset.h
    src/mapped_texture.h
    src/engine.h
    src/renderable.h
    src/scene.h
    src/renderable.h
    src/renderer.h
    src/render_primitive.h
    src/colour_pass.h
    src/render_queue.h
    src/index_buffer.h
    src/vertex_buffer.h
    src/managers/component_manager.h
    src/managers/renderable_manager.h
    src/managers/transform_manager.h
    src/managers/light_manager.h
    src/render_graph/render_graph.h
    src/render_graph/render_graph_pass.h 
    src/render_graph/render_graph_builder.h
    src/render_graph/resources.h
    src/render_graph/render_pass_node.h
    src/render_graph/resource_node.h
    src/render_graph/dependency_graph.h
    src/render_graph/rendergraph_resource.h
    src/render_graph/backboard.h
    src/render_graph/render_graph_handle.h
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
    "$<$<PLATFORM_ID:Windows>:_USE_MATH_DEFINES;NOMINMAX;_CRT_SECURE_NO_WARNINGS>"  
)

# add common compile flags
yave_add_compiler_flags(TARGET YAVE)

# group source and header files
yave_source_group(
    TARGET YAVE
    ROOT_DIR "${YAVE_ROOT_DIRECTORY}/yave"
)
