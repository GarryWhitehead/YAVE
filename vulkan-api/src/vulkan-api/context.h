/* Copyright (c) 2022 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "common.h"
#include "utility/compiler.h"

namespace vkapi
{

/**
 * @brief The current state of this vulkan instance. Encapsulates all
 * information extracted from the device and physical device.
 *
 */
class VkContext
{
public:
    struct Extensions
    {
        bool hasPhysicalDeviceProps2 = false;
        bool hasExternalCapabilities = false;
        bool hasDebugUtils = false;
        bool hasMultiView = false;
    };

    struct QueueInfo
    {
        uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
        uint32_t present = VK_QUEUE_FAMILY_IGNORED;
        uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
    };

    VkContext();
    ~VkContext();

    static bool
    findExtensionProperties(const char* name, std::vector<vk::ExtensionProperties>& properties);

    bool prepareExtensions(
        std::vector<const char*>& extensions,
        uint32_t extCount,
        std::vector<vk::ExtensionProperties>& extensionProps);

    /**
     * @brief Creates a new abstract instance of vulkan
     */
    bool createInstance(const char** glfwExtension, uint32_t extCount);

    /**
     * @brief Sets up all the vulkan devices and queues.
     */
    bool prepareDevice(vk::SurfaceKHR windowSurface);

    uint32_t selectMemoryType(uint32_t flags, vk::MemoryPropertyFlags reqs);

    static void GlobalBarrier(
        vk::CommandBuffer& cmds,
        vk::PipelineStageFlags srcStage,
        vk::PipelineStageFlags dstStage,
        vk::AccessFlags srcAccess,
        vk::AccessFlags dstAccess);

    static void writeReadComputeBarrier(vk::CommandBuffer& cmds);

    static void ExecutionBarrier(
        vk::CommandBuffer& cmds, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage);


    // All the getters
    [[nodiscard]] const vk::Instance& instance() const { return instance_; }
    [[nodiscard]] const vk::Device& device() const { return device_; }
    [[nodiscard]] const vk::PhysicalDevice& physical() const { return physical_; }
    [[nodiscard]] const vk::PhysicalDeviceFeatures& features() const { return features_; }
    [[nodiscard]] const QueueInfo& queueIndices() const { return queueFamilyIndex_; }
    [[nodiscard]] const vk::Queue& graphicsQueue() const { return graphicsQueue_; }
    [[nodiscard]] const vk::Queue& presentQueue() const { return presentQueue_; }

private:
    vk::Instance instance_;
    vk::Device device_;
    vk::PhysicalDevice physical_;
    vk::PhysicalDeviceFeatures features_;

    QueueInfo queueFamilyIndex_;

    vk::Queue graphicsQueue_;
    vk::Queue presentQueue_;
    vk::Queue computeQueue_;

    // supported extensions
    Extensions deviceExtensions_;

    // validation layers
    std::vector<const char*> requiredLayers_;

#ifdef VULKAN_VALIDATION_DEBUG

    vk::DebugReportCallbackEXT debugCallback;
    vk::DebugUtilsMessengerEXT debugMessenger;

#endif
};

} // namespace vkapi
