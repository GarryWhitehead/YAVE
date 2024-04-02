#pragma once

#include <gtest/gtest.h>
#include <vulkan-api/driver.h>
#include <yave_app/window.h>

class VulkanHelper : public testing::Test
{
public:
    void initDriver() noexcept
    {
        driver_ = new vkapi::VkDriver;
        const auto& [ext, count] = yave::Window::getInstanceExt();
        driver_->createInstance(ext, count);
        bool success = driver_->init(VK_NULL_HANDLE);
        ASSERT_TRUE(success);
    }

    vkapi::VkDriver* getDriver() noexcept { return driver_; }

private:
    vkapi::VkDriver* driver_ = nullptr;
};