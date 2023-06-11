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

#include "context.h"

#include "utility/compiler.h"

#include <spdlog/spdlog.h>
#include <string.h>

#include <set>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT obj_type,
    uint64_t obj,
    size_t loc,
    int32_t code,
    const char* layer_prefix,
    const char* msg,
    void* data)
{
    YAVE_UNUSED(obj_type);
    YAVE_UNUSED(obj);
    YAVE_UNUSED(loc);
    YAVE_UNUSED(data);

    // ignore access mask false positive
    if (std::strcmp(layer_prefix, "DS") == 0 && code == 10)
    {
        return VK_FALSE;
    }

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        SPDLOG_INFO("Vulkan Error: {}: {}\n", layer_prefix, msg);
        return VK_FALSE;
    }
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        SPDLOG_INFO("Vulkan Warning: {}: {}\n", layer_prefix, msg);
        return VK_FALSE;
    }
    else
    {
        // just output this as a log
        SPDLOG_INFO("Vulkan Information: %s: %s\n", layer_prefix, msg);
    }
    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data)
{
    YAVE_UNUSED(user_data);

    switch (severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            {
                SPDLOG_INFO("Validation Error: {}\n", data->pMessage);
            }
            else
            {
                SPDLOG_INFO("Other Error: {}\n", data->pMessage);
            }
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            {
                SPDLOG_INFO("Validation Warning: {}\n", data->pMessage);
            }
            else
            {
                SPDLOG_INFO("Other Warning: {}\n", data->pMessage);
            }
            break;

        default:
            break;
    }

    bool logObjectNames = false;
    for (uint32_t i = 0; i < data->objectCount; i++)
    {
        auto* name = data->pObjects[i].pObjectName;
        if (name)
        {
            logObjectNames = true;
            break;
        }
    }

    if (logObjectNames)
    {
        for (uint32_t i = 0; i < data->objectCount; i++)
        {
            auto* name = data->pObjects[i].pObjectName;
            SPDLOG_INFO("  Object #%u: %s\n", i, name ? name : "N/A");
        }
    }

    return VK_FALSE;
}

namespace vkapi
{

VkContext::VkContext() = default;
VkContext::~VkContext() = default;

bool VkContext::findExtensionProperties(
    const char* name, std::vector<vk::ExtensionProperties>& properties)
{
    for (auto& ext : properties)
    {
        if (std::strcmp(ext.extensionName, name) == 0)
        {
            return true;
        }
    }
    return false;
};

bool VkContext::prepareExtensions(
    std::vector<const char*>& extensions,
    uint32_t extCount,
    std::vector<vk::ExtensionProperties>& extensionProps)
{
    for (uint32_t i = 0; i < extCount; ++i)
    {
        if (!findExtensionProperties(extensions[i], extensionProps))
        {
            SPDLOG_ERROR("Unable to find required extension properties for GLFW.");
            return false;
        }
    }

    if (findExtensionProperties(
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, extensionProps))
    {
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        deviceExtensions_.hasPhysicalDeviceProps2 = true;

        if (findExtensionProperties(
                VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, extensionProps) &&
            findExtensionProperties(
                VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, extensionProps))
        {
            extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
            extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
            deviceExtensions_.hasExternalCapabilities = true;
        }
    }
    if (findExtensionProperties(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensionProps))
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        deviceExtensions_.hasDebugUtils = true;
    }
    if (findExtensionProperties(VK_KHR_MULTIVIEW_EXTENSION_NAME, extensionProps))
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        deviceExtensions_.hasMultiView = true;
    }


#ifdef VULKAN_VALIDATION_DEBUG
    // if debug utils isn't supported, try debug report
    if (!deviceExtensions_.hasDebugUtils &&
        findExtensionProperties(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extensionProps))
    {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
#endif
    return true;
}

bool VkContext::createInstance(const char** glfwExtension, uint32_t extCount)
{
    vk::ApplicationInfo appInfo(
        "YAVE", VK_MAKE_VERSION(1, 2, 0), "", VK_MAKE_VERSION(1, 2, 0), VK_API_VERSION_1_2);

    // glfw extensions
    std::vector<const char*> extensions;
    for (uint32_t c = 0; c < extCount; ++c)
    {
        extensions.push_back(glfwExtension[c]);
    }

    // extension properties
    std::vector<vk::ExtensionProperties> extensionProps =
        vk::enumerateInstanceExtensionProperties();

    if (!prepareExtensions(extensions, extCount, extensionProps))
    {
        return false;
    }

    // layer extensions, if any
    std::vector<vk::LayerProperties> layerExt = vk::enumerateInstanceLayerProperties();

    auto findLayerExtension = [&](const char* name) -> bool {
        for (auto& ext : layerExt)
        {
            if (std::strcmp(ext.layerName, name) == 0)
            {
                return true;
            }
        }
        return false;
    };

#ifdef VULKAN_VALIDATION_DEBUG
    if (findLayerExtension("VK_LAYER_KHRONOS_validation"))
    {
        requiredLayers_.push_back("VK_LAYER_KHRONOS_validation");
    }
    else
    {
        SPDLOG_WARN("Unable to find validation standard layers.");
    }
#endif
    vk::InstanceCreateInfo createInfo(
        {},
        &appInfo,
        static_cast<uint32_t>(requiredLayers_.size()),
        requiredLayers_.data(),
        static_cast<uint32_t>(extensions.size()),
        extensions.data());
    VK_CHECK_RESULT(vk::createInstance(&createInfo, nullptr, &instance_));

#ifdef VULKAN_VALIDATION_DEBUG
    // For sone reason, on windows the instance needs to be NULL to successfully
    // get the proc address, only Mac and Linux the VkInstance needs to be
    // passed.
#ifdef WIN32
    auto addr =
        PFN_vkGetInstanceProcAddr(vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkGetInstanceProcAddr"));
#else
    auto addr =
        PFN_vkGetInstanceProcAddr(vkGetInstanceProcAddr(instance_, "vkGetInstanceProcAddr"));
#endif
    vk::DispatchLoaderDynamic dldi(instance_, addr);

    if (deviceExtensions_.hasDebugUtils)
    {
        vk::DebugUtilsMessengerCreateInfoEXT dbgCreateInfo(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            DebugMessenger,
            this);

        auto debugReport =
            instance_.createDebugUtilsMessengerEXT(&dbgCreateInfo, nullptr, &debugMessenger, dldi);
    }
    else if (findExtensionProperties(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extensionProps))
    {
        vk::DebugReportCallbackCreateInfoEXT cbCreateInfo(
            vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning |
                vk::DebugReportFlagBitsEXT::ePerformanceWarning,
            DebugCallback,
            this);

        VK_CHECK_RESULT(
            instance_.createDebugReportCallbackEXT(&cbCreateInfo, nullptr, &debugCallback, dldi));
    }
#endif

    return true;
}

bool VkContext::prepareDevice(const vk::SurfaceKHR windowSurface)
{
    if (!instance_)
    {
        SPDLOG_ERROR("You must first create a vulkan instance before creating "
                     "the device!");
        return false;
    }

    // find a suitable gpu - at the moment this is pretty basic - find a gpu and
    // that will do. In the future, find the best match
    std::vector<vk::PhysicalDevice> gpus = instance_.enumeratePhysicalDevices();
    for (auto const& gpu : gpus)
    {
        if (gpu)
        {
            physical_ = gpu;
            // prefer discrete GPU over integrated.
            // TODO: make this an option.
            auto props = gpu.getProperties();
            if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                break;
            }
        }
    }

    if (!physical_)
    {
        SPDLOG_ERROR("No Vulkan supported GPU devices were found.");
        return false;
    }

    // Also get all the device extensions for querying later
    std::vector<vk::ExtensionProperties> extensions =
        physical_.enumerateDeviceExtensionProperties();

    // find queues for this gpu
    std::vector<vk::QueueFamilyProperties> queues = physical_.getQueueFamilyProperties();

    // graphics queue
    for (uint32_t c = 0; c < queues.size(); ++c)
    {
        if (queues[c].queueCount == 0)
        {
            continue;
        }
        if (queues[c].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            queueFamilyIndex_.graphics = c;
            break;
        }
    }

    if (queueFamilyIndex_.graphics == VK_QUEUE_FAMILY_IGNORED)
    {
        SPDLOG_ERROR("Unable to find a suitable graphics queue on the device.");
        return false;
    }

    // the ideal situtaion is if the graphics and presentation queues are the
    // same.
    VkBool32 hasPresentionQueue = false;
    VK_CHECK_RESULT(physical_.getSurfaceSupportKHR(
        queueFamilyIndex_.graphics, windowSurface, &hasPresentionQueue));
    if (hasPresentionQueue)
    {
        queueFamilyIndex_.present = queueFamilyIndex_.graphics;
    }

    // else use a seperate presentation queue
    for (uint32_t c = 0; c < queues.size(); ++c)
    {
        VK_CHECK_RESULT(physical_.getSurfaceSupportKHR(c, windowSurface, &hasPresentionQueue));
        if (queues[c].queueCount > 0 && hasPresentionQueue)
        {
            queueFamilyIndex_.present = c;
            break;
        }
    }

    // presentation queue is compulsory
    if (queueFamilyIndex_.present == VK_QUEUE_FAMILY_IGNORED)
    {
        SPDLOG_ERROR("Physical device does not support a presentation queue.");
        return false;
    }

    // compute queue
    for (uint32_t c = 0; c < queues.size(); ++c)
    {
        if (queues[c].queueCount > 0 && c != queueFamilyIndex_.present &&
            queues[c].queueFlags & vk::QueueFlagBits::eCompute)
        {
            queueFamilyIndex_.compute = c;
            break;
        }
    }

    // The preference is a sepearte compute queue as this will be faster, though
    // if not found, use the graphics queue for compute shaders
    if (queueFamilyIndex_.compute == VK_QUEUE_FAMILY_IGNORED)
    {
        queueFamilyIndex_.compute = queueFamilyIndex_.graphics;
    }

    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueInfo = {};
    std::set<uint32_t> uniqueQueues = {
        queueFamilyIndex_.graphics, queueFamilyIndex_.present, queueFamilyIndex_.compute};

    for (auto& queue : uniqueQueues)
    {
        vk::DeviceQueueCreateInfo createInfo({}, queue, 1, &queuePriority);
        queueInfo.push_back(createInfo);
    }

    // enable required device features
    vk::PhysicalDeviceFeatures2 reqFeatures2;
    vk::PhysicalDeviceMultiviewFeatures mvFeatures;
    mvFeatures.multiview = VK_TRUE;
    reqFeatures2.pNext = &mvFeatures;

    vk::PhysicalDeviceFeatures devFeatures = physical_.getFeatures();
    if (devFeatures.textureCompressionETC2)
    {
        reqFeatures2.features.textureCompressionETC2 = VK_TRUE;
    }
    if (devFeatures.textureCompressionBC)
    {
        reqFeatures2.features.textureCompressionBC = VK_TRUE;
    }
    if (devFeatures.samplerAnisotropy)
    {
        reqFeatures2.features.samplerAnisotropy = VK_TRUE;
    }
    if (devFeatures.tessellationShader)
    {
        reqFeatures2.features.tessellationShader = VK_TRUE;
    }
    if (devFeatures.geometryShader)
    {
        reqFeatures2.features.geometryShader = VK_TRUE;
    }
    if (devFeatures.shaderStorageImageExtendedFormats)
    {
        reqFeatures2.features.shaderStorageImageExtendedFormats = VK_TRUE;
    }
    if (devFeatures.multiViewport)
    {
        reqFeatures2.features.multiViewport = VK_TRUE;
    }

    // a swapchain extension must be present
    if (!findExtensionProperties(VK_KHR_SWAPCHAIN_EXTENSION_NAME, extensions))
    {
        SPDLOG_ERROR("Swap chain extension not found.");
        return false;
    }

    const std::vector<const char*> reqExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
#if __APPLE__
        ,
        "VK_KHR_portability_subset"
#endif
    };

    vk::DeviceCreateInfo createInfo(
        {},
        static_cast<uint32_t>(queueInfo.size()),
        queueInfo.data(),
        static_cast<uint32_t>(requiredLayers_.size()),
        requiredLayers_.empty() ? nullptr : requiredLayers_.data(),
        static_cast<uint32_t>(reqExtensions.size()),
        reqExtensions.data(),
        nullptr,
        &reqFeatures2);

    VK_CHECK_RESULT(physical_.createDevice(&createInfo, nullptr, &device_));

    // print out specifications of the selected device
    auto props = physical_.getProperties();

    const uint32_t driverVersion = props.driverVersion;
    const uint32_t vendorID = props.vendorID;
    const uint32_t deviceID = props.deviceID;

    SPDLOG_INFO(
        "\n\nDevice name: {}\nDriver version: {}\nVendor ID: {:0x}\n",
        props.deviceName,
        driverVersion,
        vendorID);

    device_.getQueue(queueFamilyIndex_.compute, 0, &computeQueue_);
    device_.getQueue(queueFamilyIndex_.graphics, 0, &graphicsQueue_);
    device_.getQueue(queueFamilyIndex_.present, 0, &presentQueue_);

    return true;
}

uint32_t VkContext::selectMemoryType(uint32_t flags, vk::MemoryPropertyFlags reqs)
{
    auto memProps = physical_.getMemoryProperties();
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
    {
        if (flags & 1)
        {
            if ((memProps.memoryTypes[i].propertyFlags & reqs) == reqs)
            {
                return i;
            }
        }
        flags >>= 1;
    }
    SPDLOG_ERROR("Unable to find required memory type.");
    return UINT32_MAX;
}

void VkContext::GlobalBarrier(
    vk::CommandBuffer& cmds,
    vk::PipelineStageFlags srcStage,
    vk::PipelineStageFlags dstStage,
    vk::AccessFlags srcAccess,
    vk::AccessFlags dstAccess)
{
    vk::MemoryBarrier barrier {srcAccess, dstAccess};
    cmds.pipelineBarrier(
        srcStage, dstStage, (vk::DependencyFlags)0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void VkContext::writeReadComputeBarrier(vk::CommandBuffer& cmds)
{
    GlobalBarrier(
        cmds,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::AccessFlagBits::eShaderWrite,
        vk::AccessFlagBits::eShaderRead);
}

void VkContext::ExecutionBarrier(
    vk::CommandBuffer& cmds, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage)
{
    cmds.pipelineBarrier(
        srcStage, dstStage, (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 0, nullptr);
}

} // namespace vkapi
