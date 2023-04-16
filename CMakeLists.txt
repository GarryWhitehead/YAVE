cmake_minimum_required(VERSION 3.16)

project(YAVE LANGUAGES CXX)

# IDE setup
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY CMAKE_XCODE_GENERATE_SCHEME ON)

# set c and c++ standard versions to use throughout project
set(CMAKE_CXX_STANDARD              17)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_EXTENSIONS            OFF)

set(CMAKE_C_STANDARD                11)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              OFF)

# configuration or build type setup
set(configuration_types Debug Release RelWithDebInfo MinSizeRel CACHE STRING "Available configuration types." FORCE)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES ${configuration_types})
# If the user defines the build_type via the cmake cmd arg (or GUI), this value takes precedence
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

# path to the asset directory. This defaults to the assets directory at
# the root of yave but can be changed by the user
if(NOT YAVE_ASSETS_DIRECTORY)
    set(YAVE_ASSETS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" CACHE STRING "Path to the library assets.")
endif()

# global variables
set(YAVE_ROOT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(YAVE_SHADER_DIRECTORY "${YAVE_ROOT_DIRECTORY}/shaders")
set(YAVE_CMAKE_INCLUDE_DIRECTORY "${YAVE_ROOT_DIRECTORY}/cmake")

# find vulkan (hopefully!)
find_package(Vulkan REQUIRED)

# required dependencies for all libraries
find_package(VulkanHeaders REQUIRED)
find_package(shaderc REQUIRED)
find_package(vulkan-memory-allocator REQUIRED)
find_package(mathfu REQUIRED)
find_package(spirv-cross REQUIRED)
find_package(vulkan-validationlayers REQUIRED)
find_package(glfw3 REQUIRED)
find_package(TBB REQUIRED)
find_package(cgltf REQUIRED)
find_package(jsmn REQUIRED)
find_package(stb REQUIRED)
find_package(spdlog REQUIRED)
find_package(imgui REQUIRED)
find_package(Ktx REQUIRED)

if (APPLE)
    find_package(moltenvk REQUIRED)
endif()

# add libraries: 
# - libs are dependencies of vulkan-api and YAVE
# - vulkan-api is a dependency of YAVE
add_subdirectory(libs)
add_subdirectory(vulkan-api)
add_subdirectory(yave)

add_subdirectory(apps)