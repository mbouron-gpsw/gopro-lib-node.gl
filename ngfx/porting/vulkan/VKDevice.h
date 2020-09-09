/*
 * Copyright 2016 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#pragma once
#include <string>
#include <vector>
#include "graphics/Device.h"
#include <vulkan/vulkan.h>
#include "porting/vulkan/VKPhysicalDevice.h"
#include "porting/vulkan/VKUtil.h"

namespace ngfx {
    class VKDevice : public Device {
    public:
        void create(VKPhysicalDevice* vkPhysicalDevice);
        virtual ~VKDevice();
        void waitIdle();
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        struct {
            uint32_t graphics;
            uint32_t compute;
            uint32_t transfer;
        } queueFamilyIndices;
        VkDevice v = VK_NULL_HANDLE;
        bool enableDebugMarkers = false;
        std::vector<std::string> deviceExtensions;
        VKPhysicalDevice* vkPhysicalDevice;
        VkDeviceCreateInfo createInfo;
        std::vector<const char*> enabledDeviceExtensions;
    private:
        uint32_t getQueueFamilyIndex(VkQueueFlags queueFlags);
        void getQueueCreateInfos(VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
        void getDeviceExtensions();
    };
    VK_CAST(Device);
}