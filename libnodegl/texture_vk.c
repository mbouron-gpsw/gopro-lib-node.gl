/*
 * Copyright 2019 GoPro Inc.
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

#include <string.h>

#include "log.h"
#include "utils.h"
#include "format.h"
#include "format_vk.h"
#include "memory.h"
#include "texture_vk.h"
#include "nodes.h"
#include "gctx_vk.h"

static const VkFilter vk_filter_map[NGLI_NB_FILTER] = {
    [NGLI_FILTER_NEAREST] = VK_FILTER_NEAREST,
    [NGLI_FILTER_LINEAR]  = VK_FILTER_LINEAR,
};

VkFilter ngli_texture_get_vk_filter(int filter)
{
    return vk_filter_map[filter];
}

static const VkSamplerMipmapMode vk_mipmap_mode_map[NGLI_NB_MIPMAP] = {
    [NGLI_MIPMAP_FILTER_NONE]    = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    [NGLI_MIPMAP_FILTER_NEAREST] = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    [NGLI_MIPMAP_FILTER_LINEAR]  = VK_SAMPLER_MIPMAP_MODE_LINEAR,
};

VkSamplerMipmapMode ngli_texture_get_vk_mipmap_mode(int mipmap_filter)
{
    return vk_mipmap_mode_map[mipmap_filter];
}

static const VkSamplerAddressMode vk_wrap_map[NGLI_NB_WRAP] = {
    [NGLI_WRAP_CLAMP_TO_EDGE]   = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    [NGLI_WRAP_MIRRORED_REPEAT] = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    [NGLI_WRAP_REPEAT]          = VK_SAMPLER_ADDRESS_MODE_REPEAT,
};

VkSamplerAddressMode ngli_texture_get_vk_wrap(int wrap)
{
    return vk_wrap_map[wrap];
}

static int is_depth_format(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return 1;
    default:
        return 0;
    }
}

static VkImageAspectFlags get_vk_image_aspect_flags(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static VkResult create_buffer(struct vkcontext *vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory)
{
    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkResult ret = vkCreateBuffer(vk->device, &buffer_create_info, NULL, buffer);
    if (ret != VK_SUCCESS)
        return ret;

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(vk->device, *buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = ngli_vk_find_memory_type(vk, mem_requirements.memoryTypeBits, properties),
    };

    ret = vkAllocateMemory(vk->device, &alloc_info, NULL, buffer_memory);
    if (ret != VK_SUCCESS)
        return ret;

    vkBindBufferMemory(vk->device, *buffer, *buffer_memory, 0);

    return VK_SUCCESS;
}

static VkResult transition_image_layout(struct texture *s, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);

struct texture *ngli_texture_vk_create(struct gctx *gctx)
{
    struct texture_vk *s = ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->parent.gctx = gctx;
    return (struct texture *)s;
}

int ngli_texture_vk_init(struct texture *s,
                      const struct texture_params *params)
{
    s->params = *params;

    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct texture_vk *s_priv = (struct texture_vk *)s;

    int ret = ngli_format_get_vk_format(vk, s->params.format, &s_priv->format);
    if (ret < 0)
        return ret;

    VkFormatFeatureFlags features =
                    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                    VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
                    VK_FORMAT_FEATURE_BLIT_SRC_BIT |
                    VK_FORMAT_FEATURE_BLIT_DST_BIT |
                    VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
                    VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
                    VK_FORMAT_FEATURE_TRANSFER_DST_BIT;

    VkImageUsageFlagBits usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (!params->staging) {
    if (is_depth_format(s_priv->format)) {
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        features |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
    } else {
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    }

    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(vk->physical_device, s_priv->format, &properties);
    LOG(ERROR, "features = 0x%x 0x%x", properties.optimalTilingFeatures, properties.linearTilingFeatures);
    if (!(properties.optimalTilingFeatures & features))
        tiling = VK_IMAGE_TILING_LINEAR;

    VkMemoryPropertyFlags memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (params->staging) {
        tiling = VK_IMAGE_TILING_LINEAR;
        memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    }

    s_priv->mipmap_levels = 1;
    if (params->mipmap_filter != NGLI_MIPMAP_FILTER_NONE) {
        while ((params->width | params->height) >> s_priv->mipmap_levels)
            s_priv->mipmap_levels += 1;
    }

    VkImageType image_type;
    VkImageViewType view_type;
    int depth = 1;
    int array_layers = 1;
    int flags = 0;

    if (params->type == NGLI_TEXTURE_TYPE_2D) {
        image_type = VK_IMAGE_TYPE_2D;
        view_type = VK_IMAGE_VIEW_TYPE_2D;
    } else if (params->type == NGLI_TEXTURE_TYPE_3D) {
        image_type = VK_IMAGE_TYPE_3D;
        view_type = VK_IMAGE_VIEW_TYPE_3D;
        depth = params->depth;
    } else if (params->type == NGLI_TEXTURE_TYPE_CUBE) {
        image_type = VK_IMAGE_TYPE_2D;
        view_type = VK_IMAGE_VIEW_TYPE_CUBE;
        array_layers = 6;
        flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    } else {
        ngli_assert(0);
    }

    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    switch (params->samples) {
    case 2:
        samples = VK_SAMPLE_COUNT_2_BIT;
        break;
    case 4:
        samples = VK_SAMPLE_COUNT_4_BIT;
        break;
    case 8:
        samples = VK_SAMPLE_COUNT_8_BIT;
        break;
    default:
        samples = VK_SAMPLE_COUNT_1_BIT;
    }

    VkImageCreateInfo image_create_info = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = image_type,
        .extent.width  = params->width,
        .extent.height = params->height,
        .extent.depth  = depth,
        .mipLevels     = s_priv->mipmap_levels,
        .arrayLayers   = array_layers,
        .format        = s_priv->format,
        .tiling        = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage,
        .samples       = samples,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .flags         = flags,
    };

    VkResult vkret = vkCreateImage(vk->device, &image_create_info, NULL, &s_priv->image);
    if (vkret != VK_SUCCESS)
        return -1;

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(vk->device, s_priv->image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = ngli_vk_find_memory_type(vk, mem_requirements.memoryTypeBits, memory_property_flags),
    };

    vkret = vkAllocateMemory(vk->device, &alloc_info, NULL, &s_priv->image_memory);
    if (vkret != VK_SUCCESS)
        return -1;

    vkret = vkBindImageMemory(vk->device, s_priv->image, s_priv->image_memory, 0);
    if (vkret != VK_SUCCESS)
        return -1;

    s_priv->image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    s_priv->image_size = s->params.width * s->params.height * ngli_format_get_bytes_per_pixel(s->params.format) * array_layers * (params->type == NGLI_TEXTURE_TYPE_3D ? params->depth : 1);

    /* FIXME */
    transition_image_layout(s, s_priv->image, s_priv->format, s_priv->image_layout, VK_IMAGE_LAYOUT_GENERAL);

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = s_priv->image,
        .viewType = view_type,
        .format = s_priv->format,
        .subresourceRange.aspectMask = get_vk_image_aspect_flags(s_priv->format),
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = s_priv->mipmap_levels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    vkret = vkCreateImageView(vk->device, &view_info, NULL, &s_priv->image_view);
    if (vkret != VK_SUCCESS)
        return -1;

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = ngli_texture_get_vk_filter(s->params.mag_filter),
        .minFilter = ngli_texture_get_vk_filter(s->params.min_filter),
        .addressModeU = ngli_texture_get_vk_wrap(s->params.wrap_s),
        .addressModeV = ngli_texture_get_vk_wrap(s->params.wrap_t),
        .addressModeW = ngli_texture_get_vk_wrap(s->params.wrap_r),
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = ngli_texture_get_vk_mipmap_mode(s->params.mipmap_filter),
        .minLod = 0.0f,
        .maxLod = s_priv->mipmap_levels,
        .mipLodBias = 0.0f,
    };

    vkret = vkCreateSampler(vk->device, &sampler_info, NULL, &s_priv->image_sampler);
    if (vkret != VK_SUCCESS)
        return -1;

    if (!(params->usage & NGLI_TEXTURE_USAGE_ATTACHMENT_ONLY)) {
        create_buffer(vk, s_priv->image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &s_priv->staging_buffer, &s_priv->staging_buffer_memory);
    }

    return 0;
}

int ngli_texture_vk_has_mipmap(const struct texture *s)
{
    return s->params.mipmap_filter != NGLI_MIPMAP_FILTER_NONE;
}

int ngli_texture_vk_match_dimensions(const struct texture *s, int width, int height, int depth)
{
    const struct texture_params *params = &s->params;
    return params->width == width && params->height == height && params->depth == depth;
}

static VkAccessFlagBits get_vk_access_mask_from_image_layout(VkImageLayout layout, int dst_mask)
{
    VkPipelineStageFlags access_mask = 0;
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        if (dst_mask)
            ngli_assert(0);
        break;
    case VK_IMAGE_LAYOUT_GENERAL:
        access_mask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        access_mask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        access_mask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        if (!dst_mask)
            access_mask = VK_ACCESS_HOST_WRITE_BIT;
        else
            ngli_assert(0);
        break;
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
        access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
        access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        /* TODO */
        break;
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
        /* TODO */
        break;
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
        /* TODO */
        break;
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
        break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        access_mask = VK_ACCESS_MEMORY_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
        /* TODO */
        break;
    default:
        LOG(ERROR, "unexpected image layout: 0x%x", layout);
        ngli_assert(0);
    }

    return access_mask;
}

void ngli_vk_transition_image_layout(struct vkcontext *vk,
                                     VkCommandBuffer command_buffer,
                                     VkImage image,
                                     VkImageLayout old_layout,
                                     VkImageLayout new_layout,
                                     const VkImageSubresourceRange *subres_range)
{
    const VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    const VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    const VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = get_vk_access_mask_from_image_layout(old_layout, 0),
        .dstAccessMask = get_vk_access_mask_from_image_layout(new_layout, 1),
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = *subres_range,
    };

    vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

static VkResult transition_image_layout(struct texture *s, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct texture_vk *s_priv = (struct texture_vk *)s;

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    int ret = ngli_vk_begin_transient_command(vk, &command_buffer);
    if (ret < 0)
        return ret;

    const VkImageSubresourceRange subres_range = {
        .aspectMask = get_vk_image_aspect_flags(s_priv->format),
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    ngli_vk_transition_image_layout(vk, command_buffer, image, old_layout, new_layout, &subres_range);

    ret = ngli_vk_execute_transient_command(vk, command_buffer);
    if (ret < 0)
        return ret;

    s_priv->image_layout = new_layout;

    return 0;
}

int ngli_texture_vk_transition_layout(struct texture *s, VkImageLayout layout)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct texture_vk *s_priv = (struct texture_vk *)s;

    if (s_priv->image_layout == layout)
        return 0;

    VkCommandBuffer command_buffer = vk->cur_command_buffer;

    const VkImageSubresourceRange subres_range = {
        .aspectMask = get_vk_image_aspect_flags(s_priv->format),
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    ngli_vk_transition_image_layout(vk, command_buffer, s_priv->image, s_priv->image_layout, layout, &subres_range);

    s_priv->image_layout = layout;

    return 0;
}

int ngli_texture_vk_upload(struct texture *s, const uint8_t *data, int linesize)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    const struct texture_params *params = &s->params;
    struct texture_vk *s_priv = (struct texture_vk *)s;

    /* texture with external storage (including wrapped textures and render
     * buffers) cannot update their content with this function */
    ngli_assert(!s->external_storage && !(params->usage & NGLI_TEXTURE_USAGE_ATTACHMENT_ONLY));

    if (data) {
        int face = 1;
        if (params->type == NGLI_TEXTURE_TYPE_CUBE)
            face = 6;

        /* TODO: check return */
        LOG(ERROR, "data=%d", face);
        void *mapped_data;
        vkMapMemory(vk->device, s_priv->staging_buffer_memory, 0, s_priv->image_size, 0, &mapped_data);
        memcpy(mapped_data, data, s_priv->image_size);
        vkUnmapMemory(vk->device, s_priv->staging_buffer_memory);

        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        int ret = ngli_vk_begin_transient_command(vk, &command_buffer);
        if (ret < 0)
            return ret;

        const VkImageSubresourceRange subres_range = {
            .aspectMask = get_vk_image_aspect_flags(s_priv->format),
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        };
        ngli_vk_transition_image_layout(vk, command_buffer, s_priv->image, s_priv->image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &subres_range);

        struct darray copy_regions;
        ngli_darray_init(&copy_regions, sizeof(VkBufferImageCopy), 0);

        int layer_size = s->params.width * s->params.height * ngli_format_get_bytes_per_pixel(s->params.format);
        for (int i = 0; i < face; i++) {
            VkDeviceSize offset = i * layer_size;
            VkBufferImageCopy region = {
                .bufferOffset = offset,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource.aspectMask = get_vk_image_aspect_flags(s_priv->format),
                .imageSubresource.mipLevel = 0,
                .imageSubresource.baseArrayLayer = i,
                .imageSubresource.layerCount = 1,
                .imageOffset = {0, 0, 0},
                .imageExtent = {
                    params->width,
                    params->height,
                    params->depth ? params->depth : 1,
                }
            };

            if (!ngli_darray_push(&copy_regions, &region)) {
                /* FIXME */
                return -1;
            }
        }

        vkCmdCopyBufferToImage(command_buffer, s_priv->staging_buffer, s_priv->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ngli_darray_count(&copy_regions), ngli_darray_data(&copy_regions));

        ngli_vk_transition_image_layout(vk, command_buffer, s_priv->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, s_priv->image_layout, &subres_range);

        ret = ngli_vk_execute_transient_command(vk, command_buffer);
        if (ret < 0)
            return ret;

        if (ngli_texture_has_mipmap(s))
            ngli_texture_generate_mipmap(s);
    }

    return 0;
}

int ngli_texture_vk_generate_mipmap(struct texture *s)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    const struct texture_params *params = &s->params;
    struct texture_vk *s_priv = (struct texture_vk *)s;

    ngli_assert(!(params->usage & NGLI_TEXTURE_USAGE_ATTACHMENT_ONLY));

    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(vk->physical_device, s_priv->format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        LOG(ERROR, "texture image format does not support linear blitting");
        return -1;
    }

    VkCommandBuffer command_buffer;
    if (vk->cur_command_buffer)
        command_buffer = vk->cur_command_buffer;
    else {
        int ret = ngli_vk_begin_transient_command(vk, &command_buffer);
        if (ret < 0)
            return ret;
    }

    const VkImageSubresourceRange subres_range = {
        .aspectMask = get_vk_image_aspect_flags(s_priv->format),
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    ngli_vk_transition_image_layout(vk, command_buffer, s_priv->image, s_priv->image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &subres_range);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = s_priv->image,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = params->type == NGLI_TEXTURE_TYPE_CUBE ? 6 : 1,
        .subresourceRange.levelCount = 1,
    };

    int32_t mipWidth = s->params.width;
    int32_t mipHeight = s->params.height;
    int mipmap_levels = 1;
    while ((params->width | params->height) >> mipmap_levels)
        mipmap_levels += 1;
    int mipLevels = mipmap_levels;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, NULL,
            0, NULL,
            1, &barrier);

        VkImageBlit blit = {
            .srcOffsets[0] = (VkOffset3D){0, 0, 0},
            .srcOffsets[1] = (VkOffset3D){mipWidth, mipHeight, 1},
            .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .srcSubresource.mipLevel = i - 1,
            .srcSubresource.baseArrayLayer = 0,
            .srcSubresource.layerCount = params->type == NGLI_TEXTURE_TYPE_CUBE ? 6 : 1,
            .dstOffsets[0] = (VkOffset3D){0, 0, 0},
            .dstOffsets[1] = (VkOffset3D){ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 },
            .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .dstSubresource.mipLevel = i,
            .dstSubresource.baseArrayLayer = 0,
            .dstSubresource.layerCount = params->type == NGLI_TEXTURE_TYPE_CUBE ? 6 : 1,
        };

        vkCmdBlitImage(command_buffer,
            s_priv->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            s_priv->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = s_priv->image_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, NULL,
            0, NULL,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = s_priv->image_layout;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, NULL,
        0, NULL,
        1, &barrier);

    if (!vk->cur_command_buffer)
        ngli_vk_execute_transient_command(vk, command_buffer);

    return 0;
}

void ngli_texture_vk_freep(struct texture **sp)
{
    if (!*sp)
        return;

    struct texture *s = *sp;
    struct texture_vk *s_priv = (struct texture_vk *)s;
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;

    vkDestroySampler(vk->device, s_priv->image_sampler, NULL);
    vkDestroyImageView(vk->device, s_priv->image_view, NULL);
    vkDestroyImage(vk->device, s_priv->image, NULL);
    vkDestroyBuffer(vk->device, s_priv->staging_buffer, NULL);
    vkFreeMemory(vk->device, s_priv->staging_buffer_memory, NULL);
    vkFreeMemory(vk->device, s_priv->image_memory, NULL);

    ngli_freep(sp);
}
