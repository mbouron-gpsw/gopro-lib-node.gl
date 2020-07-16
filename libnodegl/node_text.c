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

#include <stddef.h>
#include <string.h>

#include "memory.h"
#include "nodes.h"
#include "darray.h"
#include "drawutils.h"
#include "log.h"
#include "math_utils.h"
#include "pgcache.h"
#include "pgcraft.h"
#include "pipeline.h"
#include "type.h"
#include "topology.h"
#include "utils.h"

struct pipeline_desc {
    struct pgcraft *crafter;
    struct pipeline *pipeline;
    int modelview_matrix_index;
    int projection_matrix_index;
};

struct text_priv {
    char *text;
    float fg_color[4];
    float bg_color[4];
    float box_corner[3];
    float box_width[3];
    float box_height[3];
    int padding;
    double font_scale;
    int valign, halign;
    int aspect_ratio[2];
    int min_filter;
    int mag_filter;
    int mipmap_filter;

    struct texture *texture;
    struct canvas canvas;
    struct buffer *vertices;
    struct buffer *uvcoords;
    struct buffer *indices;
    int nb_indices;
    struct darray pipeline_descs;
};

#define VALIGN_CENTER 0
#define VALIGN_TOP    1
#define VALIGN_BOTTOM 2

#define HALIGN_CENTER 0
#define HALIGN_RIGHT  1
#define HALIGN_LEFT   2

static const struct param_choices valign_choices = {
    .name = "valign",
    .consts = {
        {"center", VALIGN_CENTER, .desc=NGLI_DOCSTRING("vertically centered")},
        {"bottom", VALIGN_BOTTOM, .desc=NGLI_DOCSTRING("bottom positioned")},
        {"top",    VALIGN_TOP,    .desc=NGLI_DOCSTRING("top positioned")},
        {NULL}
    }
};

static const struct param_choices halign_choices = {
    .name = "halign",
    .consts = {
        {"center", HALIGN_CENTER, .desc=NGLI_DOCSTRING("horizontally centered")},
        {"right",  HALIGN_RIGHT,  .desc=NGLI_DOCSTRING("right positioned")},
        {"left",   HALIGN_LEFT,   .desc=NGLI_DOCSTRING("left positioned")},
        {NULL}
    }
};

#define OFFSET(x) offsetof(struct text_priv, x)
static const struct node_param text_params[] = {
    {"text",         PARAM_TYPE_STR, OFFSET(text), .flags=PARAM_FLAG_CONSTRUCTOR,
                     .desc=NGLI_DOCSTRING("text string to rasterize")},
    {"fg_color",     PARAM_TYPE_VEC4, OFFSET(fg_color), {.vec={1.0, 1.0, 1.0, 1.0}},
                     .desc=NGLI_DOCSTRING("foreground text color")},
    {"bg_color",     PARAM_TYPE_VEC4, OFFSET(bg_color), {.vec={0.0, 0.0, 0.0, 0.8}},
                     .desc=NGLI_DOCSTRING("background text color")},
    {"box_corner",   PARAM_TYPE_VEC3, OFFSET(box_corner), {.vec={-1.0, -1.0, 0.0}},
                     .desc=NGLI_DOCSTRING("origin coordinates of `box_width` and `box_height` vectors")},
    {"box_width",    PARAM_TYPE_VEC3, OFFSET(box_width), {.vec={2.0, 0.0, 0.0}},
                     .desc=NGLI_DOCSTRING("box width vector")},
    {"box_height",   PARAM_TYPE_VEC3, OFFSET(box_height), {.vec={0.0, 2.0, 0.0}},
                     .desc=NGLI_DOCSTRING("box height vector")},
    {"padding",      PARAM_TYPE_INT, OFFSET(padding), {.i64=3},
                     .desc=NGLI_DOCSTRING("pixel padding around the text")},
    {"font_scale",   PARAM_TYPE_DBL, OFFSET(font_scale), {.dbl=1.0},
                     .desc=NGLI_DOCSTRING("scaling of the font")},
    {"valign",       PARAM_TYPE_SELECT, OFFSET(valign), {.i64=VALIGN_CENTER},
                     .choices=&valign_choices,
                     .desc=NGLI_DOCSTRING("vertical alignment of the text in the box")},
    {"halign",       PARAM_TYPE_SELECT, OFFSET(halign), {.i64=HALIGN_CENTER},
                     .choices=&halign_choices,
                     .desc=NGLI_DOCSTRING("horizontal alignment of the text in the box")},
    {"aspect_ratio", PARAM_TYPE_RATIONAL, OFFSET(aspect_ratio),
                     .desc=NGLI_DOCSTRING("box aspect ratio")},
    {"min_filter",   PARAM_TYPE_SELECT, OFFSET(min_filter), {.i64=NGLI_FILTER_LINEAR},
                     .choices=&ngli_filter_choices,
                     .desc=NGLI_DOCSTRING("rasterized text texture minifying function")},
    {"mag_filter",   PARAM_TYPE_SELECT, OFFSET(mag_filter), {.i64=NGLI_FILTER_NEAREST},
                     .choices=&ngli_filter_choices,
                     .desc=NGLI_DOCSTRING("rasterized text texture magnification function")},
    {"mipmap_filter", PARAM_TYPE_SELECT, OFFSET(mipmap_filter), {.i64=NGLI_MIPMAP_FILTER_LINEAR},
                      .choices=&ngli_mipmap_filter_choices,
                      .desc=NGLI_DOCSTRING("rasterized text texture minifying mipmap function")},
    {NULL}
};

static const char * const vertex_data =
    "void main()"                                                           "\n"
    "{"                                                                     "\n"
    "    ngl_out_pos = projection_matrix * modelview_matrix * position;"    "\n"
    "    var_tex_coord = uvcoord;"                                          "\n"
    "}";

static const char * const fragment_data =
    "void main()"                                                           "\n"
    "{"                                                                     "\n"
    "    float v = ngl_tex2d(tex, var_tex_coord).r;"                        "\n"
    "    ngl_out_color = mix(bg_color, fg_color, v);"                       "\n"
    "}";

static const struct pgcraft_iovar vert_out_vars[] = {
    {.name = "var_tex_coord", .type = NGLI_TYPE_VEC2},
};

#define BC(index) s->box_corner[index]
#define BW(index) s->box_width[index]
#define BH(index) s->box_height[index]

#define C(index) chr_corner[index]
#define W(index) chr_width[index]
#define H(index) chr_height[index]

static void get_char_box_dim(const char *s, int *wp, int *hp, int *np)
{
    int w = 0, h = 1;
    int cur_w = 0;
    int n = 0;
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\n') {
            cur_w = 0;
            h++;
        } else {
            cur_w++;
            w = NGLI_MAX(w, cur_w);
            n++;
        }
    }
    *wp = w;
    *hp = h;
    *np = n;
}

static int print(struct text_priv *s)
{
    int ret = 0;
    const char *str = s->text;
    const float cw = 1 / 16.f; // normalized character width in the atlas
    const float ch = 1 / 8.f;  // normalized character height in the atlas

    int text_cols, text_rows, text_nbchr;
    get_char_box_dim(str, &text_cols, &text_rows, &text_nbchr);

    const float box_width_len  = ngli_vec3_length(s->box_width);
    const float box_height_len = ngli_vec3_length(s->box_height);
    static const int default_ar[2] = {1, 1};
    const int *ar = s->aspect_ratio[0] && s->aspect_ratio[1] ? s->aspect_ratio : default_ar;
    const float box_ratio = ar[0] * box_width_len / (float)(ar[1] * box_height_len);

    const int text_width   = text_cols * NGLI_FONT_W + 2 * s->padding;
    const int text_height  = text_rows * NGLI_FONT_H + 2 * s->padding;
    const float text_ratio = ar[1] * text_width / (float)(ar[0] * text_height);

    float width[3];
    float height[3];

    if (text_ratio < box_ratio) {
        ngli_vec3_scale(width, s->box_width, text_ratio);
        memcpy(height, s->box_height, sizeof(height));
    } else {
        memcpy(width, s->box_width, sizeof(width));
        ngli_vec3_scale(height, s->box_height, 1.f / text_ratio);
    }

    ngli_vec3_scale(width, width, s->font_scale);
    ngli_vec3_scale(height, height, s->font_scale);

    float padw[3];
    float padh[3];
    ngli_vec3_scale(padw, width,  s->padding / (float)text_width);
    ngli_vec3_scale(padh, height, s->padding / (float)text_height);

    const float nopad_ratio_w = 1.f - 2 * s->padding / (float)text_width;
    const float nopad_ratio_h = 1.f - 2 * s->padding / (float)text_height;

    float width_nopad[3];
    float height_nopad[3];
    ngli_vec3_scale(width_nopad, width, nopad_ratio_w);
    ngli_vec3_scale(height_nopad, height, nopad_ratio_h);

    float chr_width[3];
    float chr_height[3];
    ngli_vec3_scale(chr_width,  width_nopad,  1.f / text_cols);
    ngli_vec3_scale(chr_height, height_nopad, 1.f / text_rows);

    const int nb_vertices = text_nbchr * 4 * 3;
    const int nb_uvcoords = text_nbchr * 4 * 2;
    const int nb_indices  = text_nbchr * 6;
    float *vertices = ngli_calloc(nb_vertices, sizeof(*vertices));
    float *uvcoords = ngli_calloc(nb_uvcoords, sizeof(*uvcoords));
    short *indices  = ngli_calloc(nb_indices, sizeof(*indices));
    s->nb_indices = nb_indices;
    if (!vertices || !uvcoords || !indices) {
        ret = NGL_ERROR_MEMORY;
        goto end;
    }

    /* Adjust text position according to alignment settings */
    float align_padw[3];
    float align_padh[3];
    ngli_vec3_sub(align_padw, s->box_width,  width);
    ngli_vec3_sub(align_padh, s->box_height, height);

    const float spx = (s->halign == HALIGN_CENTER ? .5f :
                       s->halign == HALIGN_RIGHT  ? 1.f :
                       0.f);
    const float spy = (s->valign == VALIGN_CENTER ? .5f :
                       s->valign == VALIGN_BOTTOM ? 1.f :
                       0.f);

    ngli_vec3_scale(align_padw, align_padw, spx);
    ngli_vec3_scale(align_padh, align_padh, spy);

    int px = 0, py = 0;
    int n = 0;

    for (int i = 0; str[i]; i++) {
        const char c = str[i] & 0x7f; // limit to atlas size of 16x8

        if (c == '\n') {
            py++;
            px = 0;
            continue;
        }

        /* quad vertices */
        float chr_corner_x[3];
        float chr_corner_y[3];
        float chr_corner[3] = {BC(0), BC(1), BC(2)};
        ngli_vec3_add(chr_corner, chr_corner, align_padw);
        ngli_vec3_add(chr_corner, chr_corner, align_padh);
        ngli_vec3_add(chr_corner, chr_corner, padw);
        ngli_vec3_add(chr_corner, chr_corner, padh);
        ngli_vec3_scale(chr_corner_x, chr_width, px);
        ngli_vec3_scale(chr_corner_y, chr_height, text_rows - py - 1);
        ngli_vec3_add(chr_corner, chr_corner, chr_corner_x);
        ngli_vec3_add(chr_corner, chr_corner, chr_corner_y);
        const float chr_vertices[] = {
            C(0),               C(1),               C(2),
            C(0) + W(0),        C(1) + W(1),        C(2) + W(2),
            C(0) + H(0) + W(0), C(1) + H(1) + W(1), C(2) + H(2) + W(2),
            C(0) + H(0),        C(1) + H(1),        C(2) + H(2),
        };
        memcpy(vertices + 4 * 3 * n, chr_vertices, sizeof(chr_vertices));

        /* focus uvcoords on the character in the atlas texture */
        const int chr_pos_x = c % 16;
        const int chr_pos_y = 7 - c / 16;
        const float cx = chr_pos_x * cw;
        const float cy = chr_pos_y * ch;
        const float chr_uvs[] = {
            cx,      1.f - cy,
            cx + cw, 1.f - cy,
            cx + cw, 1.f - cy - ch,
            cx,      1.f - cy - ch,
        };
        memcpy(uvcoords + 4 * 2 * n, chr_uvs, sizeof(chr_uvs));

        /* quad for each character is made of 2 triangles */
        const short chr_indices[] = { n*4 + 0, n*4 + 1, n*4 + 2, n*4 + 0, n*4 + 2, n*4 + 3 };
        memcpy(indices + n * NGLI_ARRAY_NB(chr_indices), chr_indices, sizeof(chr_indices));

        n++;
        px++;
    }

    ret = ngli_buffer_init(s->vertices, nb_vertices * sizeof(*vertices), NGLI_BUFFER_USAGE_STATIC);
    if (ret < 0)
        goto end;

    ret = ngli_buffer_upload(s->vertices, vertices, nb_vertices * sizeof(*vertices));
    if (ret < 0)
        goto end;

    ret = ngli_buffer_init(s->uvcoords, nb_uvcoords * sizeof(*uvcoords), NGLI_BUFFER_USAGE_STATIC);
    if (ret < 0)
        goto end;

    ret = ngli_buffer_upload(s->uvcoords, uvcoords, nb_uvcoords * sizeof(*uvcoords));
    if (ret < 0)
        goto end;

    ret = ngli_buffer_init(s->indices, nb_indices * sizeof(*indices), NGLI_BUFFER_USAGE_STATIC);
    if (ret < 0)
        goto end;

    ret = ngli_buffer_upload(s->indices, indices, nb_indices * sizeof(*indices));
    if (ret < 0)
        goto end;

end:
    ngli_free(vertices);
    ngli_free(uvcoords);
    ngli_free(indices);
    return ret;
}

static int text_init(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct gctx *gctx = ctx->gctx;
    struct text_priv *s = node->priv_data;

    s->vertices = ngli_buffer_create(gctx);
    if (!s->vertices)
        return NGL_ERROR_MEMORY;

    s->uvcoords = ngli_buffer_create(gctx);
    if (!s->uvcoords)
        return NGL_ERROR_MEMORY;

    s->indices = ngli_buffer_create(gctx);
    if (!s->indices)
        return NGL_ERROR_MEMORY;

    int ret = print(s);
    if (ret < 0)
        return ret;

    ret = ngli_drawutils_get_font_atlas(&s->canvas);
    if (ret < 0)
        return ret;

    struct texture_params tex_params = NGLI_TEXTURE_PARAM_DEFAULTS;
    tex_params.width = s->canvas.w;
    tex_params.height = s->canvas.h;
    tex_params.format = NGLI_FORMAT_R8_UNORM;
    tex_params.min_filter = s->min_filter;
    tex_params.mag_filter = s->mag_filter;
    s->texture = ngli_texture_create(gctx);
    if (!s->texture)
        return NGL_ERROR_MEMORY;
    ret = ngli_texture_init(s->texture, &tex_params);
    if (ret < 0)
        return ret;

    ret = ngli_texture_upload(s->texture, s->canvas.buf, 0);
    if (ret < 0)
        return ret;

    ngli_darray_init(&s->pipeline_descs, sizeof(struct pipeline_desc), 0);

    return 0;
}

static int text_prepare(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct gctx *gctx = ctx->gctx;
    struct text_priv *s = node->priv_data;

    const struct pgcraft_uniform uniforms[] = {
        {.name = "modelview_matrix",  .type = NGLI_TYPE_MAT4, .stage = NGLI_PROGRAM_SHADER_VERT, .data = NULL},
        {.name = "projection_matrix", .type = NGLI_TYPE_MAT4, .stage = NGLI_PROGRAM_SHADER_VERT, .data = NULL},
        {.name = "bg_color",          .type = NGLI_TYPE_VEC4, .stage = NGLI_PROGRAM_SHADER_FRAG, .data = s->bg_color},
        {.name = "fg_color",          .type = NGLI_TYPE_VEC4, .stage = NGLI_PROGRAM_SHADER_FRAG, .data = s->fg_color},
    };

    const struct pgcraft_texture textures[] = {
        {
            .name     = "tex",
            .type     = NGLI_PGCRAFT_SHADER_TEX_TYPE_TEXTURE2D,
            .stage    = NGLI_PROGRAM_SHADER_FRAG,
            .texture  = s->texture,
        },
    };

    const struct pgcraft_attribute attributes[] = {
        {
            .name     = "position",
            .type     = NGLI_TYPE_VEC4,
            .format   = NGLI_FORMAT_R32G32B32_SFLOAT,
            .stride   = 3 * 4,
            .buffer   = s->vertices,
        },
        {
            .name     = "uvcoord",
            .type     = NGLI_TYPE_VEC2,
            .format   = NGLI_FORMAT_R32G32_SFLOAT,
            .stride   = 2 * 4,
            .buffer   = s->uvcoords,
        },
    };

    struct pipeline_params pipeline_params = {
        .type          = NGLI_PIPELINE_TYPE_GRAPHICS,
        .graphics      = {
            .topology       = NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .indices        = s->indices,
            .indices_format = NGLI_FORMAT_R16_UNORM,
            .nb_indices     = s->nb_indices,
            .state          = ctx->graphicstate,
            .rt_desc        = *ctx->rendertarget_desc,
        }
    };

    const struct pgcraft_params crafter_params = {
        .vert_base        = vertex_data,
        .frag_base        = fragment_data,
        .uniforms         = uniforms,
        .nb_uniforms      = NGLI_ARRAY_NB(uniforms),
        .textures         = textures,
        .nb_textures      = NGLI_ARRAY_NB(textures),
        .attributes       = attributes,
        .nb_attributes    = NGLI_ARRAY_NB(attributes),
        .vert_out_vars    = vert_out_vars,
        .nb_vert_out_vars = NGLI_ARRAY_NB(vert_out_vars),
    };

    struct pipeline_desc *desc = ngli_darray_push(&s->pipeline_descs, NULL);
    if (!desc)
        return NGL_ERROR_MEMORY;
    ctx->rnode_pos->id = ngli_darray_count(&s->pipeline_descs) - 1;

    memset(desc, 0, sizeof(*desc));

    desc->crafter = ngli_pgcraft_create(ctx);
    if (!desc->crafter)
        return NGL_ERROR_MEMORY;

    int ret = ngli_pgcraft_craft(desc->crafter, &pipeline_params, &crafter_params);
    if (ret < 0)
        return ret;

    desc->pipeline = ngli_pipeline_create(gctx);
    if (!desc->pipeline)
        return NGL_ERROR_MEMORY;

    ret = ngli_pipeline_init(desc->pipeline, &pipeline_params);
    if (ret < 0)
        return ret;

    desc->modelview_matrix_index = ngli_pgcraft_get_uniform_index(desc->crafter, "modelview_matrix", NGLI_PROGRAM_SHADER_VERT);
    desc->projection_matrix_index = ngli_pgcraft_get_uniform_index(desc->crafter, "projection_matrix", NGLI_PROGRAM_SHADER_VERT);

    return 0;
}

static void text_draw(struct ngl_node *node)
{
    struct ngl_ctx *ctx = node->ctx;
    struct text_priv *s = node->priv_data;

    const float *modelview_matrix  = ngli_darray_tail(&ctx->modelview_matrix_stack);
    const float *projection_matrix = ngli_darray_tail(&ctx->projection_matrix_stack);

    struct pipeline_desc *descs = ngli_darray_data(&s->pipeline_descs);
    struct pipeline_desc *desc = &descs[ctx->rnode_pos->id];

    ngli_pipeline_update_uniform(desc->pipeline, desc->modelview_matrix_index, modelview_matrix);
    ngli_pipeline_update_uniform(desc->pipeline, desc->projection_matrix_index, projection_matrix);

    ngli_pipeline_exec(desc->pipeline);
}

static void text_uninit(struct ngl_node *node)
{
    struct text_priv *s = node->priv_data;
    struct pipeline_desc *descs = ngli_darray_data(&s->pipeline_descs);
    const int nb_descs = ngli_darray_count(&s->pipeline_descs);
    for (int i = 0; i < nb_descs; i++) {
        struct pipeline_desc *desc = &descs[i];
        ngli_pipeline_freep(&desc->pipeline);
        ngli_pgcraft_freep(&desc->crafter);
    }
    ngli_darray_reset(&s->pipeline_descs);
    ngli_texture_freep(&s->texture);
    ngli_buffer_freep(&s->vertices);
    ngli_buffer_freep(&s->uvcoords);
    ngli_buffer_freep(&s->indices);
    ngli_free(s->canvas.buf);
}

const struct node_class ngli_text_class = {
    .id        = NGL_NODE_TEXT,
    .name      = "Text",
    .init      = text_init,
    .prepare   = text_prepare,
    .draw      = text_draw,
    .uninit    = text_uninit,
    .priv_size = sizeof(struct text_priv),
    .params    = text_params,
    .file      = __FILE__,
};
