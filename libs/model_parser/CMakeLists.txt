
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(YaveModelParser STATIC)

target_include_directories(
    YaveModelParser
    PUBLIC
    ${YAVE_UTILITY_ROOT_PATH}
    ${VulkanHeaders_INCLUDE_DIRS}
)

target_link_libraries(
    YaveModelParser
    PUBLIC
    jsmn::jsmn
    mathfu::mathfu
    cgltf::cgltf
    YaveUtility
)

target_sources(
    YaveModelParser
    PRIVATE
    gltf/model_mesh.cpp
    gltf/model_material.cpp
    gltf/node_instance.cpp
    gltf/gltf_model.cpp
    gltf/skin_instance.cpp

    gltf/model_mesh.h
    gltf/model_material.h
    gltf/node_instance.h
    gltf/gltf_model.h
    gltf/skin_instance.h
)

# add common compiler flags
yave_add_compiler_flags(TARGET YaveModelParser)

# group source and header files
yave_source_group(
    TARGET YaveModelParser
    ROOT_DIR ${YAVE_UTILITY_ROOT_PATH}
)
