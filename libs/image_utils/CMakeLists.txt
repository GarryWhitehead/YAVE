
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(YaveImageUtils STATIC)

target_include_directories(
    YaveImageUtils
    PUBLIC
    ${YAVE_UTILITY_ROOT_PATH}
)

target_link_libraries(
    YaveImageUtils
    PUBLIC
    mathfu::mathfu
    YaveUtility
)

target_sources(
    YaveImageUtils
    PRIVATE
    noise_generator.cpp
    cubemap.cpp
    
    noise_generator.h
    cubemap.h
)

# add common compiler flags
yave_add_compiler_flags(TARGET YaveImageUtils)

# group source and header files
yave_source_group(
    TARGET YaveImageUtils
    ROOT_DIR ${YAVE_UTILITY_ROOT_PATH}
)
