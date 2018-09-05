/*
 * Copyright 2018 GoPro Inc.
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

#include "renderer.h"
#include "nodes.h"

struct rendererbuffer *ngli_renderer_create_buffer(struct glcontext *glcontext, uint32_t size, uint32_t usage)
{
    struct rendererbuffer *handle = malloc(sizeof(struct rendererbuffer));
    if (!handle)
        return NULL;

    // TODO: base alignement on these values
    // VkDeviceSize          minUniformBufferOffsetAlignment;
    // VkDeviceSize          minStorageBufferOffsetAlignment;
    uint32_t aligned_size = NGLI_ALIGN(size, 32);
    handle->size = aligned_size;
    handle->usage = usage;

    // create the buffer
    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    handle->buffers = calloc(glcontext->nb_frames, sizeof(*handle->buffers));
    if (!handle->buffers)
        return NULL;

    VkResult vkret = VK_SUCCESS;
    for (uint32_t i = 0; i < glcontext->nb_frames; i++){
        vkret = vkCreateBuffer(glcontext->device, &buffer_create_info, NULL, &handle->buffers[i]);
        if (vkret != VK_SUCCESS)
            return NULL;
    }

    /* alloc madness */
    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(glcontext->device, handle->buffers[0], &mem_req);

    uint32_t memory_type_index = -1;
    VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (int i = 0; i < glcontext->phydev_mem_props.memoryTypeCount; i++) {
        if ((mem_req.memoryTypeBits & (1<<i)) && (glcontext->phydev_mem_props.memoryTypes[i].propertyFlags & props) == props) {
            memory_type_index = i;
            break;
        }
    }

    VkMemoryAllocateInfo memory_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = glcontext->nb_frames * aligned_size,
        .memoryTypeIndex = memory_type_index,
    };

    vkret = vkAllocateMemory(glcontext->device, &memory_allocate_info, NULL, &handle->allocation);
    if (vkret != VK_SUCCESS)
        return NULL;

    for (uint32_t i = 0; i < glcontext->nb_frames; i++){
        vkret = vkBindBufferMemory(glcontext->device, handle->buffers[i], handle->allocation, i * aligned_size);
        if (vkret != VK_SUCCESS)
            return NULL;
    }

    return handle;
}

void ngli_renderer_destroy_buffer(struct glcontext *glcontext, struct rendererbuffer *handle)
{
    for (uint32_t i = 0; i < glcontext->nb_frames; i++)
        vkDestroyBuffer(glcontext->device, handle->buffers[i], NULL);
    free(handle->buffers);

    // TODO: allocation could be reuse
    vkFreeMemory(glcontext->device, handle->allocation, NULL);
    free(handle);
}

void ngli_renderer_bind_buffer(struct glcontext *glcontext, struct program *p, struct rendererbuffer *rb, uint32_t offset, uint32_t size, uint32_t index, uint32_t type)
{
    for (uint32_t i = 0; i < glcontext->nb_frames; i++){
        VkDescriptorBufferInfo descriptor_buffer_info = {
            .buffer = rb->buffers[i],
            .offset = offset,
            .range = size,
        };
        VkWriteDescriptorSet write_descriptor_set = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = p->descriptor_sets[i],
            .dstBinding = index,
            .dstArrayElement = 0,
            .descriptorType = type,
            .descriptorCount = 1,
            .pBufferInfo = &descriptor_buffer_info,
            .pImageInfo = NULL,
            .pTexelBufferView = NULL,
        };
        vkUpdateDescriptorSets(glcontext->device, 1, &write_descriptor_set, 0, NULL);
    }
}

void *ngli_renderer_map_buffer(struct glcontext *glcontext, struct rendererbuffer* handle)
{
    void *mapped_memory;
    vkMapMemory(glcontext->device, handle->allocation, glcontext->frame_index * handle->size, handle->size, 0, &mapped_memory);
    return mapped_memory;
}

void ngli_renderer_unmap_buffer(struct glcontext *glcontext, struct rendererbuffer* handle)
{
    vkUnmapMemory(glcontext->device, handle->allocation);
}

void *ngli_renderer_create_shader(struct glcontext *glcontext, const uint8_t *data, uint32_t data_size)
{
    VkShaderModuleCreateInfo shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = data_size,
        .pCode = (const uint32_t *)data,
    };

    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(glcontext->device, &shader_module_create_info, NULL, &module) != VK_SUCCESS)
        return NULL;

    return module;
}

void ngli_renderer_destroy_shader(struct glcontext *glcontext, void *handle)
{
    vkDestroyShaderModule(glcontext->device, handle, NULL);
}

void ngli_renderer_start_time(struct glcontext *glcontext)
{
    // TODO: could be build once
    VkCommandBuffer command_buffer = glcontext->query_cmd_buf[2*glcontext->frame_index];
    VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    };

    VkResult ret = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    if (ret != VK_SUCCESS)
        return;
    vkCmdResetQueryPool(command_buffer, glcontext->query_pool, 0, 1);

    vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, glcontext->query_pool, 0);
    ret = vkEndCommandBuffer(command_buffer);
    if (ret != VK_SUCCESS)
        return;

    glcontext->command_buffers[glcontext->nb_command_buffers++] = command_buffer;
}

void ngli_renderer_stop_time(struct glcontext *glcontext)
{
    // TODO: could be build once
    VkCommandBuffer command_buffer = glcontext->query_cmd_buf[2*glcontext->frame_index+1];
    VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    };

    VkResult ret = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    if (ret != VK_SUCCESS)
        return;
    vkCmdResetQueryPool(command_buffer, glcontext->query_pool, 1, 1);

    vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, glcontext->query_pool, 1);
    ret = vkEndCommandBuffer(command_buffer);
    if (ret != VK_SUCCESS)
        return;
    
    glcontext->command_buffers[glcontext->nb_command_buffers++] = command_buffer;
}

uint64_t ngli_renderer_get_time(struct glcontext *glcontext)
{
    uint64_t timestamps[2];
    vkGetQueryPoolResults(glcontext->device,
			glcontext->query_pool,
			0,
			2,
            sizeof(timestamps),
			timestamps,
			sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT);

    return timestamps[1] - timestamps[0];
}

void ngli_renderer_marker_begin(struct glcontext *glcontext, const char *name)
{
    static PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin = NULL;
    if(!vkCmdDebugMarkerBegin) {
        void *function = vkGetInstanceProcAddr(glcontext->instance, "vkCmdDebugMarkerBeginEXT");
        vkCmdDebugMarkerBegin = function;
    }

    if (vkCmdDebugMarkerBegin) {
        VkDebugMarkerMarkerInfoEXT debug_marker_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,
            .pMarkerName = name,
        };

        VkCommandBuffer command_buffer = glcontext->command_buffers[glcontext->nb_command_buffers];
        if (command_buffer != VK_NULL_HANDLE)
            vkCmdDebugMarkerBegin(command_buffer, &debug_marker_info);
    }
}

void ngli_renderer_marker_end(struct glcontext *glcontext)
{
    static PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd = NULL;
    if(!vkCmdDebugMarkerEnd) {
        void *function = vkGetInstanceProcAddr(glcontext->instance, "vkCmdDebugMarkerEndEXT");
        vkCmdDebugMarkerEnd = function;
    }
    
    if (vkCmdDebugMarkerEnd) {
        VkCommandBuffer command_buffer = glcontext->command_buffers[glcontext->nb_command_buffers];
        if (command_buffer != VK_NULL_HANDLE)
            vkCmdDebugMarkerEnd(command_buffer);
    }
}