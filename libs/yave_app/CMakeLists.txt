
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(YaveApp STATIC)

target_include_directories(
    YaveApp
    PUBLIC
    ${YAVE_UTILITY_ROOT_PATH}
)

target_link_libraries(
    YaveApp
    PUBLIC
    YAVE
    YaveUtility
    mathfu::mathfu
    imgui::imgui
    stb::stb
    KTX::ktx
)

target_compile_definitions(
    YaveApp
    PRIVATE
    YAVE_SHADER_DIRECTORY="${YAVE_SHADER_DIRECTORY}"
    YAVE_ASSETS_DIRECTORY="${YAVE_ASSETS_DIRECTORY}"
)

target_sources(
    YaveApp
    PRIVATE
    app.cpp
    imgui_helper.cpp
    models.cpp
    window.cpp
    camera_view.cpp
    asset_loader.cpp

    app.h
    imgui_helper.h
    models.h
    window.h
    camera_view.h
    asset_loader.h
)

# add common compiler flags
yave_add_compiler_flags(TARGET YaveApp)

# group source and header files
yave_source_group(
    TARGET YaveApp
    ROOT_DIR ${YAVE_UTILITY_ROOT_PATH}
)
