from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import collect_libs

import os


class VkSceneEditor3dPackage(ConanFile):
    name = "vk-scene-editor"
    license = "MIT"
    author = "garry"
    url = ""
    description = "A Vulkan 3D scene editor."
    topics = ("Vulkan", "graphics", "3D")

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_validation_layers": [True, False],
        "verbose": [True, False]
    }
    default_options = {"shared": False, "fPIC": True, "with_validation_layers": True, "verbose": False}
    generators = "CMakeDeps"
    _cmake = None

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        self.requires("vulkan-headers/1.3.236.0")
        self.requires("mathfu/1.1.0")
        self.requires("vulkan-loader/1.3.236.0")
        self.requires("vulkan-memory-allocator/3.0.1")
        self.requires("vulkan-validationlayers/1.3.236.0")
        self.requires("glfw/3.3.8")
        self.requires("shaderc/2021.1")
        self.requires("glslang/1.3.236.0")
        self.requires("spirv-tools/1.3.236.0")
        self.requires("spirv-cross/1.3.236.0")
        self.requires("stb/cci.20210910")
        self.requires("tbb/2020.3")
        self.requires("cgltf/1.12")
        self.requires("jsmn/1.1.0")
        self.requires("spdlog/1.11.0")
        self.requires("imgui/1.89.4")
        self.requires("ktx/4.0.0")

        if self.settings.os == "Macos":
            self.requires("moltenvk/1.2.2")

    def layout(self):
        self.folders.source = "./src"
        self.folders.build = os.getcwd()
        self.folders.generators = f"{self.folders.build}/Conan"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["WITH_VALIDATION_LAYERS"] = self.options.with_validation_layers
        tc.variables["BUILD_SHARED"] = self.options.shared
        tc.variables["VERBOSE_OUTPUT"] = self.options.verbose
        tc.generate()

    def build(self):
        self._cmake = CMake(self)
        self._cmake.configure()
        self._cmake.build()

    def package(self):
        self.copy("LICENSE", dst="licenses")

        if self._cmake is None:
            self._cmake = CMake(self)
            self._cmake.configure()
        self._cmake.install()

    def package_info(self):
        self.cpp_info.libs = collect_libs(self)
        self.cpp_info.set_property("cmake_file_name", "vk-scene-editor")
        self.cpp_info.set_property("cmake_target_name", "vse::vse")
