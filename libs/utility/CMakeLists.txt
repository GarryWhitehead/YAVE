
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(YaveUtility STATIC)

target_include_directories(
    YaveUtility
    PUBLIC
    ${YAVE_UTILITY_ROOT_PATH}
)

target_sources(
    YaveUtility
    PRIVATE
    cstring.cpp
    assertion.cpp

    bitset_enum.h
    compiler.h
    cstring.h
    logger.h
    colour.h
    murmurhash.h
    enum_cast.h
    maths.h
    timer.h
    handle.h
    assertion.h
    aligned_alloc.h
)

# add common compiler flags
yave_add_compiler_flags(TARGET YaveUtility)

# group source and header files
yave_source_group(
    TARGET YaveUtility
    ROOT_DIR ${YAVE_UTILITY_ROOT_PATH}
)
