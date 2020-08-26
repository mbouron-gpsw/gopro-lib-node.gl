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
#ifdef GRAPHICS_BACKEND_VULKAN
#if 0
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.h>
#include "porting/vulkan/VKGraphicsContext.h"
#endif
#include "NGLWindowWrapper.h"
using namespace NGL;
using namespace ngfx;

NGLWindowWrapper::NGLWindowWrapper(ngfx::GraphicsContext* ctx, uint32_t w, uint32_t h,
    uintptr_t displayHandle, uintptr_t windowHandle) {
    this->w = w; this->h = h;
#ifdef GRAPHICS_BACKEND_VULKAN
    vkSurface.w = w; vkSurface.h = h;
    VkResult vkResult;
#ifdef VK_USE_PLATFORM_XLIB_KHR
    if (!displayHandle) displayHandle = (uintptr_t)XOpenDisplay(NULL);
    VkXlibSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.dpy = (Display *)displayHandle;
    surfaceCreateInfo.window = windowHandle;
    V(vkCreateXlibSurfaceKHR(vk(ctx)->vkInstance.v, &surfaceCreateInfo, NULL, &vkSurface.v));
#endif
    surface = &vkSurface;
#endif
}

