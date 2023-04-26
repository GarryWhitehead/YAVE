
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(YaveIbl STATIC)

target_include_directories(
    YaveIbl
    PUBLIC
    ${YAVE_UTILITY_ROOT_PATH}
)

target_link_libraries(
    YaveIbl
    PUBLIC
    YAVE
    YaveImageUtils
    mathfu::mathfu
    stb::stb
    spdlog::spdlog
)

target_sources(
    YaveIbl
    PRIVATE
    ibl.cpp
    prefilter.cpp
    ibl.h
    prefilter.h
)

# add common compiler flags
yave_add_compiler_flags(TARGET YaveIbl)

# group source and header files
yave_source_group(
    TARGET YaveIbl
    ROOT_DIR ${YAVE_UTILITY_ROOT_PATH}
)
