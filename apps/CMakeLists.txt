project (Applications)

include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${YAVE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

function(create_app)
    set(single_value_args APP_NAME)
    set(multi_value_args FILES)
    cmake_parse_arguments(
        "PARAM"
        ""
        "${single_value_args}"
        "${multi_value_args}"
        "${ARGN}"
    )

    if(NOT PARAM_APP_NAME)
        message(FATAL "No app name parameter specified.")
    endif()

    add_executable(${PARAM_APP_NAME} ${PARAM_FILES})
    target_include_directories(${PARAM_APP_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${PARAM_APP_NAME} PUBLIC YAVE YaveApp)
    set_target_properties(${PARAM_APP_NAME} PROPERTIES FOLDER Apps)
    target_compile_definitions(
        ${PARAM_APP_NAME}
        PRIVATE
        YAVE_ASSETS_DIRECTORY="${YAVE_ASSETS_DIRECTORY}"
    )

    yave_add_compiler_flags(TARGET ${PARAM_APP_NAME})

    # group source and header files
    yave_source_group(
        TARGET ${PARAM_APP_NAME}
        ROOT_DIR "${YAVE_ROOT_DIRECTORY}/apps"
    )

endfunction()

create_app(APP_NAME primitives FILES primitives.cpp primitives.h)
create_app(APP_NAME gltf_model FILES gltf_model.cpp gltf_model.h)
create_app(APP_NAME triangle FILES triangle.cpp)
create_app(APP_NAME wave_demo FILES wave_demo.cpp wave_demo.h)

target_link_libraries(primitives PUBLIC mathfu::mathfu YaveUtility)
target_link_libraries(gltf_model PUBLIC mathfu::mathfu YaveUtility YaveIbl)
target_link_libraries(triangle PUBLIC mathfu::mathfu)
target_link_libraries(wave_demo PUBLIC mathfu::mathfu YaveUtility YaveIbl YaveImageUtils)

