
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(YaveUtility STATIC)

target_include_directories(
    YaveUtility
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(
    YaveUtility
    PRIVATE
    src/utility/cstring.cpp
    src/utility/assertion.cpp

    PUBLIC
    src/utility/bitset_enum.h
    src/utility/compiler.h
    src/utility/cstring.h
    src/utility/logger.h
    src/utility/colour.h
    src/utility/murmurhash.h
    src/utility/enum_cast.h
    src/utility/maths.h
    src/utility/timer.h
    src/utility/handle.h
    src/utility/assertion.h
    src/utility/aligned_alloc.h
    src/utility/soa.h
)

# add common compiler flags
yave_add_compiler_flags(TARGET YaveUtility)

# group source and header files
yave_source_group(
    TARGET YaveUtility
    ROOT_DIR ${YAVE_UTILITY_ROOT_PATH}
)

if (BUILD_TESTS)

    set (test_srcs
        test/main_test.cpp
        test/cstring_test.cpp
        test/soa_test.cpp
    )

    add_executable(UtilityTest ${test_srcs})
    target_link_libraries(UtilityTest PRIVATE GTest::GTest YaveUtility)
    set_target_properties(UtilityTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${YAVE_TEST_DIRECTORY})

    add_test(
            NAME UtilityTest
            COMMAND UtilityTest
            WORKING_DIRECTORY ${YAVE_TEST_DIRECTORY}
    )

endif()