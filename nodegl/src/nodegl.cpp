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

#include "nodegl.h"
#include "DebugUtil.h"
#include "NGL.h"
#include "NGLApplication.h"
#include "StringUtil.h"
#include <functional>
#include <glm/gtc/type_ptr.hpp>
using namespace NGL;

#define EASING_IN_OUT(t0, t1) \
    { #t1 "_in", EASING_##t0##_IN }, { #t1 "_out", EASING_##t0##_OUT }, { #t1 "_in_out", EASING_##t0##_IN_OUT }, { #t1 "_out_in", EASING_##t0##_OUT_IN }
static std::map<std::string, EasingId> EasingIdMap {
    { "linear", EASING_LINEAR },
    EASING_IN_OUT(QUADRATIC, quadratic),
    EASING_IN_OUT(CUBIC, cubic),
    EASING_IN_OUT(QUARTIC, quartic),
    EASING_IN_OUT(QUINTIC, quintic),
    EASING_IN_OUT(POWER, power),
    EASING_IN_OUT(SINUS, sinus),
    EASING_IN_OUT(EXP, exp),
    EASING_IN_OUT(CIRCULAR, circular),
    { "bounce_in", EASING_BOUNCE_IN }, { "bounce_out", EASING_BOUNCE_OUT },
    { "elastic_in", EASING_ELASTIC_IN }, { "elastic_out", EASING_ELASTIC_OUT },
    EASING_IN_OUT(BACK, back)
};

static std::map<std::string, NGL::BlendFactor> BlendFactorMap {
    { "zero", NGL::BlendFactor::BLEND_FACTOR_ZERO }, { "one", NGL::BlendFactor::BLEND_FACTOR_ONE },
    { "src_color", NGL::BlendFactor::BLEND_FACTOR_SRC_COLOR }, {"one_minus_src_color", NGL::BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_COLOR },
    { "dst_color", NGL::BlendFactor::BLEND_FACTOR_DST_COLOR }, {"one_minus_dst_color", NGL::BlendFactor::BLEND_FACTOR_ONE_MINUS_DST_COLOR },
    { "src_alpha", NGL::BlendFactor::BLEND_FACTOR_SRC_ALPHA }, { "one_minus_src_alpha", NGL::BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA },
    { "dst_alpha", NGL::BlendFactor::BLEND_FACTOR_DST_ALPHA }, { "one_minus_dst_alpha", NGL::BlendFactor::BLEND_FACTOR_ONE_MINUS_DST_ALPHA }
};

static std::map<std::string, NGL::BlendOp> BlendOpMap {
    { "add", BLEND_OP_ADD }, { "subtract", BLEND_OP_SUBTRACT },
    { "reverse_subtract", BLEND_OP_REVERSE_SUBTRACT },
    { "min", BLEND_OP_MIN }, {"max", BLEND_OP_MAX }
};

static struct {
    NGL::ColorComponentFlags at(const std::string& str) {
        NGL::ColorComponentFlags colorWriteMask = 0;
        if (str.find("r") != str.npos) colorWriteMask |= ColorComponentFlagBits::COLOR_COMPONENT_R_BIT;
        if (str.find("g") != str.npos) colorWriteMask |= ColorComponentFlagBits::COLOR_COMPONENT_G_BIT;
        if (str.find("b") != str.npos) colorWriteMask |= ColorComponentFlagBits::COLOR_COMPONENT_B_BIT;
        if (str.find("a") != str.npos) colorWriteMask |= ColorComponentFlagBits::COLOR_COMPONENT_A_BIT;
        return colorWriteMask;
    }
} ColorWriteMaskMap;

static std::map<std::string, NGL::PrimitiveTopology> PrimitiveTopologyMap {
    { "point_list", PRIMITIVE_TOPOLOGY_POINT_LIST },
    { "line_list", PRIMITIVE_TOPOLOGY_LINE_LIST },
    { "line_strip", PRIMITIVE_TOPOLOGY_LINE_STRIP },
    { "triangle_list", PRIMITIVE_TOPOLOGY_TRIANGLE_LIST },
    { "triangle_strip", PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP }
};

static std::string getFourCC(uint32_t type) {
    char* ptr = (char*)&type;
    char str[] = { ptr[3], ptr[2], ptr[1], ptr[0], '\0' };
    return str;
}

struct ngl_node {
    int type = 0;
    NGL::sp<NGL::Node> v;
    std::function<void(NGL::Node* node, const std::string key, int nb_elems, void* elems)> param_add = nullptr;
    std::function<void(NGL::Node* node, const std::string key, va_list &ap)> param_set = nullptr;
};

const std::string str_arg(va_list& ap) { return va_arg(ap, const char*); }
const void* voidp_arg(va_list& ap) { return va_arg(ap, void*); }
const int int_arg(va_list& ap) { return va_arg(ap, int); }
const double dbl_arg(va_list& ap) { return va_arg(ap, double); }
glm::vec2 vec2_arg(va_list& ap) { float* v = va_arg(ap, float*); return glm::make_vec2(v); }
glm::vec3 vec3_arg(va_list& ap) { float* v = va_arg(ap, float*); return glm::make_vec3(v); }
glm::vec4 vec4_arg(va_list& ap) { float* v = va_arg(ap, float*); return glm::make_vec4(v); }
glm::quat quat_arg(va_list& ap) { float* v = va_arg(ap, float*); return glm::make_quat(v); }
template <typename T> sp<T> node_arg(va_list& ap) { ngl_node* node = va_arg(ap, ngl_node*); return sp_cast<T>(node->v); }
glm::mat4 mat4_arg(va_list& ap) { float *v = va_arg(ap, float*); return glm::make_mat4(v); }
sp<Data> data_arg(va_list& ap) {
    auto data = make_shared<Data>();
    data->size = int_arg(ap);
    data->v = malloc(data->size);
    const void* src = voidp_arg(ap);
    memcpy(data->v, src, data->size);
    return data;
}
struct ngl_ctx {
    NGL::sp<NGL::Context> v;
};
struct ngl_ctx *ngl_create(void) {
    ngl_ctx* ctx = new ngl_ctx;
    auto app = make_shared<NGLApplication>();
    ctx->v.reset(new NGL::Context(app));
    return ctx;
}
int ngl_resize(struct ngl_ctx *s, int width, int height, const int *viewport) {
    TODO("w: %d h: %d", width, height);
    return -1;
}

char *ngl_node_dot(const struct ngl_node *node) {
    TODO(); return strdup("TODO");
}

int ngl_configure(struct ngl_ctx *s, struct ngl_config *c) {
    s->v->setConfig({
        bool(c->offscreen),
        c->width, c->height,
        glm::make_vec4(c->clear_color),
        c->capture_buffer,
        c->display,
        c->window
    });
    return 0;
}

int ngl_set_scene(struct ngl_ctx *s, struct ngl_node *scene) {
    s->v->setScene(scene->v);
    return 0;
}

void ngl_node_unrefp(struct ngl_node **nodep) {
    struct ngl_node *node = *nodep;
    if (!node)
        return;
    delete node;
}

int ngl_anim_evaluate(struct ngl_node *anim, void *dst, double t) {
    auto af = sp_cast<NGL::AnimatedNode>(anim->v);
    if (!af) { TODO(); return 1; }
    af->evaluate(t);
    memcpy(dst, af->data, af->size);
    return 0;

}

int ngl_node_param_add(struct ngl_node *node, const char *key,
                       int nb_elems, void *elems) {
    if (!node->param_add) LOG_TRACE("%s TODO type: %s", __FUNCTION__, getFourCC(node->type).c_str());
    else node->param_add(node->v.get(), key, nb_elems, elems);
    return 0;
}

static auto ngl_graphics_config_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::GraphicsConfig* grCfg = (NGL::GraphicsConfig*)node;
    if (key == "blend") grCfg->blend = int_arg(ap);
    else if (key == "blend_src_factor") {
        grCfg->blendSrcFactor = BlendFactorMap.at(str_arg(ap));
    }
    else if (key == "blend_dst_factor") {
        grCfg->blendDstFactor = BlendFactorMap.at(str_arg(ap));
    }
    else if (key == "blend_op") {
        grCfg->blendOp = BlendOpMap.at(str_arg(ap));
    }
    else if (key == "scissor_test") {
        grCfg->scissorTest = int_arg(ap);
    }
    else if (key == "scissor") {
        vec4 p = vec4_arg(ap);
        grCfg->scissorRect = { int32_t(p[0]), int32_t(p[1]), uint32_t(p[2]), uint32_t(p[3]) };
    }
    else if (key == "depth_test") {
        grCfg->depthTest = int_arg(ap);
    }
    else if (key == "color_write_mask") {
        grCfg->colorWriteMask = ColorWriteMaskMap.at(str_arg(ap));
    }
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_camera_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Camera* camera = (NGL::Camera*)node;
    if (key == "eye") camera->eye = vec3_arg(ap);
    else if (key == "center") camera->center = vec3_arg(ap);
    else if (key == "up") camera->up = vec3_arg(ap);
    else if (key == "perspective") {
        vec2 p = vec2_arg(ap);
        memcpy(&camera->perspectiveParams, value_ptr(p), sizeof(p));
        camera->perspectiveProj = true;
    }
    else if (key == "orthographic") {
        vec4 p = vec4_arg(ap);
        memcpy(&camera->orthographicParams, value_ptr(p), sizeof(p));
        camera->perspectiveProj = false;
    }
    else if (key == "clipping") {
        vec2 p = vec2_arg(ap);
        memcpy(&camera->clippingParams, value_ptr(p), sizeof(p));
    }
    else if (key == "eye_transform") {
        camera->eyeTransform = node_arg<Transform>(ap);
    }
    else if (key == "center_transform") {
        camera->centerTransform = node_arg<Transform>(ap);
    }
    else if (key == "up_transform") {
        camera->upTransform = node_arg<Transform>(ap);
    }
    else if (key == "fov_anim") {
        camera->fovAnim = node_arg<AnimatedFloat>(ap);
    }
    else { TODO("key: %s", key.c_str()); }
};


#define DEFINE_ANIMKEYFRAME_PARAM_SET(t0, t1, value_key, value_type) \
static auto ngl_animkeyframe_##t0##_param_set = [](NGL::Node* node, const std::string key, va_list &ap) { \
    NGL::AnimKeyFrame##t1* akf = (NGL::AnimKeyFrame##t1*)node; \
    if (key == "easing") { \
        akf->easingId = EasingIdMap.at(str_arg(ap)); \
    } \
    else if (key == "easing_start_offset") { \
        akf->easingOffsets[0] = dbl_arg(ap); \
    } \
    else if (key == "easing_end_offset") { \
        akf->easingOffsets[1] = dbl_arg(ap); \
    } \
    else if (key == #value_key) { \
        akf->v = value_type(ap); \
    } \
    else { TODO("key: %s", key.c_str()); } \
}
DEFINE_ANIMKEYFRAME_PARAM_SET(float, Float, value, dbl_arg);
DEFINE_ANIMKEYFRAME_PARAM_SET(vec2, Vec2, value, vec2_arg);
DEFINE_ANIMKEYFRAME_PARAM_SET(vec3, Vec3, value, vec3_arg);
DEFINE_ANIMKEYFRAME_PARAM_SET(vec4, Vec4, value, vec4_arg);
DEFINE_ANIMKEYFRAME_PARAM_SET(quat, Quat, quat, quat_arg);
DEFINE_ANIMKEYFRAME_PARAM_SET(buffer, Buffer, data, data_arg);

static auto ngl_media_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Media* media = (NGL::Media*)node;
    TODO("key: %s", key.c_str());
};

const static std::map<std::string, NGL::Text::HAlign> textHAlignMap =  {
    { "left" , NGL::Text::HALIGN_LEFT },
    { "center", NGL::Text::HALIGN_CENTER },
    { "right", NGL::Text::HALIGN_RIGHT }
};

const static std::map<std::string, NGL::Text::VAlign> textVAlignMap =  {
    { "bottom" , NGL::Text::VALIGN_BOTTOM },
    { "center", NGL::Text::VALIGN_CENTER },
    { "top", NGL::Text::VALIGN_TOP }
};

static auto ngl_text_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Text* text = (NGL::Text*)node;
    if (key == "fg_color") {
        text->fgColor = vec4_arg(ap);
    } else if (key == "bg_color") {
        text->bgColor = vec4_arg(ap);
    } else if (key == "padding") {
        text->padding = int_arg(ap);
    } else if (key == "font_scale") {
        text->fontScale = dbl_arg(ap);
    } else if (key == "halign") {
        text->hAlign = textHAlignMap.at(str_arg(ap));
    } else if (key == "valign") {
        text->vAlign = textVAlignMap.at(str_arg(ap));
    }
    else { TODO("key: %s", key.c_str()); }
};

const static std::map<std::string, NGL::FilterMode> filterModeMap =  {
    { "nearest" , NGL::FilterMode::FILTER_NEAREST },
    { "linear", NGL::FilterMode::FILTER_LINEAR }
};
const static std::map<std::string, NGL::MipmapFilterMode> mipmapFilterModeMap =  {
    { "none" , NGL::MIPMAP_FILTER_NONE },
    { "nearest" , NGL::MIPMAP_FILTER_NEAREST },
    { "linear", NGL::MIPMAP_FILTER_LINEAR }
};
const static std::map<std::string, NGL::SamplerAddressMode> samplerAddressModeMap =  {
    { "repeat", NGL::SAMPLER_ADDRESS_MODE_REPEAT },
    { "mirrored_repeat", NGL::SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
    { "clamp_to_edge", NGL::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE }
};

#define DEFINE_NGL_PIXELFORMATS(s, t0, t1) \
{ "r8_" #t0, PIXELFORMAT_R##s##_##t0 }, \
{ "r8g8_" #t1, PIXELFORMAT_RG##s##_##t0 }, \
{ "r8g8b8a8_" #t1, PIXELFORMAT_RGBA##s##_##t0 }

const static std::map<std::string, NGL::PixelFormat> PixelFormatMap = {
    DEFINE_NGL_PIXELFORMATS(8, UNORM, UNORM),
    DEFINE_NGL_PIXELFORMATS(16, UINT, UINT),
    DEFINE_NGL_PIXELFORMATS(16, SFLOAT, SFLOAT),
    DEFINE_NGL_PIXELFORMATS(32, UINT, UINT),
    DEFINE_NGL_PIXELFORMATS(32, SFLOAT, SFLOAT),
    { "b8g8r8a8_unorm", PIXELFORMAT_BGRA8_UNORM },
    { "d16_unorm", PIXELFORMAT_D16_UNORM },
    { "d24_unorm", PIXELFORMAT_D24_UNORM },
    { "d24_unorm_s8_uint", PIXELFORMAT_D24_UNORM_S8 }
};

#define TEXTURE_PARAM_SET_FN0() \
    if (key == "min_filter") { \
        tex->minFilter = filterModeMap.at(str_arg(ap)); \
    } \
    else if (key == "mag_filter") { \
        tex->magFilter = filterModeMap.at(str_arg(ap)); \
    } \
    else if (key == "mipmap_filter") { \
        tex->mipmapFilter = mipmapFilterModeMap.at(str_arg(ap)); \
    } \
    else if (key == "wrap_s") { \
        tex->wrapS = samplerAddressModeMap.at(str_arg(ap)); \
    } \
    else if (key == "wrap_t") { \
        tex->wrapT = samplerAddressModeMap.at(str_arg(ap)); \
    } \
    else if (key == "format") { \
        tex->pixelFormat = PixelFormatMap.at(str_arg(ap)); \
    }

static auto ngl_texture2d_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Texture2D* tex = (NGL::Texture2D*)node;
    TEXTURE_PARAM_SET_FN0()
    else if (key == "width") {
       tex->w = int_arg(ap);
    }
    else if (key == "height") {
       tex->h = int_arg(ap);
    }
    else if (key == "data_src") {
        tex->dataSrc = node_arg<Node>(ap);
    }
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_texture3d_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Texture3D* tex = (NGL::Texture3D*)node;
    TEXTURE_PARAM_SET_FN0()
    else if (key == "width") {
       tex->w = int_arg(ap);
    }
    else if (key == "height") {
       tex->h = int_arg(ap);
    }
    else if (key == "wrap_r") {
        tex->wrapR = samplerAddressModeMap.at(str_arg(ap));
    }
    else if (key == "data_src") {
        tex->dataSrc = node_arg<Node>(ap);
    }
    else if (key == "depth") {
        tex->d = int_arg(ap);
    }
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_texture_cube_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::TextureCube* tex = (NGL::TextureCube*)node;
    TEXTURE_PARAM_SET_FN0()
    else if (key == "wrap_r") {
        tex->wrapR = samplerAddressModeMap.at(str_arg(ap));
    }
    else if (key == "data_src") {
        tex->dataSrc = node_arg<Node>(ap);
    }
    else if (key == "size") {
        tex->setDim(int_arg(ap));
    }
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_group_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Group* grp = (NGL::Group*)node;
    TODO("key: %s", key.c_str());
};

static auto ngl_streamed_buffer_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::StreamedBuffer* sb = (NGL::StreamedBuffer*)node;
    if (key == "time_anim") sb->timeAnim = node_arg<AnimatedTime>(ap);
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_buffer_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Buffer* buffer = (NGL::Buffer*)node;
    if (key == "data") buffer->data = data_arg(ap);
    else if (key == "count") buffer->count = int_arg(ap);
    else if (key == "block") buffer->block = node_arg<Block>(ap);
    else if (key == "block_field") buffer->blockField = str_arg(ap);
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_block_param_add = [](NGL::Node* node, const std::string key, int nb_elems, void* elems) {
    TODO("key: %s", key.c_str());
};

static auto ngl_block_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Block* block= (NGL::Block*)node;
    if (key == "layout") {
        block->layout = str_arg(ap);
    } else if (key == "fields") {
        const std::string name = str_arg(ap);
        auto uniform = node_arg<Node>(ap);
        block->updateFields({ { name, uniform } });
    }
    else {
        TODO("key: %s", key.c_str());
    }
};


#define DEFINE_UNIFORM_PARAM_SET(t0, t1, p0) \
static auto ngl_uniform_##t0##_param_set = [](NGL::Node* node, const std::string key, va_list &ap) { \
    NGL::Uniform##t1* uniform##t1 = (NGL::Uniform##t1*)node; \
    if (key == "value") { \
        uniform##t1->value = p0; \
    } \
    else { TODO("key: %s", key.c_str()); } \
}
static auto ngl_uniform_mat4_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::UniformMat4* uniformMat4 = (NGL::UniformMat4*)node;
    if (key == "value") {
        uniformMat4->value = mat4_arg(ap);
    }
    else if (key == "transform") {
        uniformMat4->transform = node_arg<Transform>(ap);
    }
    else { TODO("key: %s", key.c_str()); }
};
static auto ngl_uniform_quat_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::UniformQuat* uniformQuat = (NGL::UniformQuat*)node;
    if (key == "value") {
        uniformQuat->value = quat_arg(ap);
    }
    else if (key == "as_mat4") {
        uniformQuat->asMat4 = int_arg(ap);
    }
    else { TODO("key: %s", key.c_str()); }
};

DEFINE_UNIFORM_PARAM_SET(int, Int, int_arg(ap));
DEFINE_UNIFORM_PARAM_SET(float, Float, dbl_arg(ap));
DEFINE_UNIFORM_PARAM_SET(vec2, Vec2, vec2_arg(ap));
DEFINE_UNIFORM_PARAM_SET(vec3, Vec3, vec4(vec3_arg(ap), 0.0f));
DEFINE_UNIFORM_PARAM_SET(vec4, Vec4, vec4_arg(ap));

static auto ngl_graphics_program_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::GraphicsProgram* program = (NGL::GraphicsProgram*)node;
    if (key == "vertex") {
        program->vertexShaderFile = str_arg(ap);
    }
    else if (key == "fragment") {
        program->fragmentShaderFile = str_arg(ap);
    }
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_geometry_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Geometry* geom = (NGL::Geometry*)node;
    if (key == "uvcoords") geom->uvCoords = node_arg<NGL::Buffer>(ap);
    else if (key == "normals") geom->normals = node_arg<NGL::Buffer>(ap);
    else if (key == "indices") geom->indices = node_arg<NGL::Buffer>(ap);
    else if (key == "topology") geom->topology = PrimitiveTopologyMap.at(str_arg(ap));
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_triangle_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Triangle* triangle = (NGL::Triangle*)node;
    if (key == "uv_edge0") triangle->uvEdges[0] = vec3_arg(ap);
    else if (key == "uv_edge1") triangle->uvEdges[1] = vec3_arg(ap);
    else if (key == "uv_edge2") triangle->uvEdges[2] = vec3_arg(ap);

    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_quad_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Quad* quad = (NGL::Quad*)node;
    if (key == "corner") {
        quad->corner = vec3_arg(ap);
    }
    else if (key == "width") {
        quad->width = vec3_arg(ap);
    }
    else if (key == "height") {
        quad->height = vec3_arg(ap);
    }
    else if (key == "uv_corner") {
        quad->uv_corner = vec2_arg(ap);
    }
    else if (key == "uv_width") {
        quad->uv_width = vec2_arg(ap);
    }
    else if (key == "uv_height") {
        quad->uv_height = vec2_arg(ap);
    }
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_circle_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Circle* circle = (NGL::Circle*)node;
    if (key == "radius") circle->radius = dbl_arg(ap);
    else if (key == "npoints") circle->numPoints = int_arg(ap);
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_timerangemodecont_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    { TODO("key: %s", key.c_str()); }
};

static auto ngl_timerangemodenoop_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    { TODO("key: %s", key.c_str()); }
};

static auto ngl_timerangemodeonce_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    { TODO("key: %s", key.c_str()); }
};

static auto ngl_timerangefilter_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    { TODO("key: %s", key.c_str()); }
};

static auto ngl_rotate_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Rotate* rotate = (NGL::Rotate*)node;
    if (key == "angle") rotate->set(dbl_arg(ap));
    else if (key == "axis") rotate->setAxis(vec3_arg(ap));
    else if (key == "anchor") rotate->setAnchor(vec3_arg(ap));
    else if (key == "anim") rotate->setAnim(node_arg<AnimatedFloat>(ap));
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_rotate_quat_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::RotateQuat* rotateQuat= (NGL::RotateQuat*)node;
    if (key == "quat") rotateQuat->set(quat_arg(ap));
    else if (key == "anchor") rotateQuat->setAnchor(vec3_arg(ap));
    else if (key == "anim") rotateQuat->setAnim(node_arg<AnimatedQuat>(ap));
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_scale_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Scale* scale= (NGL::Scale*)node;
    if (key == "factors") scale->set(vec3_arg(ap));
    else if (key == "anchor") scale->setAnchor(vec3_arg(ap));
    else if (key == "anim") scale->setAnim(node_arg<AnimatedVec3>(ap));
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_translate_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Translate* translate = (NGL::Translate*)node;
    if (key == "vector") translate->set(vec3_arg(ap));
    else if (key == "anim") translate->setAnim(node_arg<AnimatedVec3>(ap));
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_transform_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Transform* transform = (NGL::Transform*)node;
    if (key == "matrix") transform->set(mat4_arg(ap));
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_render_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Render* render  = (NGL::Render*)node;
    if (key == "program") {
        render->program = node_arg<GraphicsProgram>(ap);
    }
    else if (key == "textures") {
        render->updateTextures({ { str_arg(ap), node_arg<NGL::Texture>(ap) } });
    }
    else if (key == "uniforms") {
        render->updateUniforms({ { str_arg(ap), node_arg<Node>(ap) } });
    }
    else if (key == "nb_instances") {
        render->numInstances = int_arg(ap);
    }
    else if (key == "instance_attributes") {
        const std::string name = str_arg(ap);
        auto attr = node_arg<NGL::Buffer>(ap);
        render->updateInstanceAttributes({ { name, attr } });
    }
    else if (key == "blocks") {
        render->updateBlocks({ { str_arg(ap), node_arg<Block>(ap) }});
    }
    else { TODO("key: %s", key.c_str()); }
};

static map<string, uint32_t> rttFlagsMap = {
    { "depth", RenderToTexture::Flags::DEPTH },
    { "stencil", RenderToTexture::Flags::STENCIL },
    { "no_clear", RenderToTexture::Flags::NO_CLEAR }
};

static auto ngl_compute_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::Compute* compute= (NGL::Compute*)node;
    if (key == "uniforms") {
        compute->updateUniforms({ { str_arg(ap), node_arg<Node>(ap) } });
    }
    else if (key == "blocks") {
        compute->updateBlocks({ { str_arg(ap), node_arg<Block>(ap) }});
    }
    else if (key == "textures") {
        compute->updateTextures({ { str_arg(ap),node_arg<NGL::Texture>(ap) } });
    }
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_rtt_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::RenderToTexture* rtt= (NGL::RenderToTexture*)node;
    if (key == "clear_color") {
        rtt->setClearColor = true;
        rtt->clearColor = vec4_arg(ap);
    }
    else if (key == "samples") {
        rtt->numSamples = int_arg(ap);
    }
    else if (key == "features") {
        string flagsStr = str_arg(ap);
        vector<string> flagsVec;
        StringUtil::split(flagsStr, "+", flagsVec);
        uint32_t flags = 0;
        for (auto& flagStr : flagsVec) {
            flags |= rttFlagsMap.at(flagStr);
        }
        rtt->flags = flags;
    }
    else if (key == "depth_texture") {
        rtt->depthTexture = node_arg<Texture2D>(ap);
    }
    else { TODO("key: %s", key.c_str()); }
};

static auto ngl_animated_quat_param_set = [](NGL::Node* node, const std::string key, va_list &ap) {
    NGL::AnimatedQuat* aq = (NGL::AnimatedQuat*)node;
    if (key == "as_mat4") aq->asMat4 = int_arg(ap);
    else { TODO("key: %s", key.c_str()); }
};

#define DEFINE_ANIMATED_PARAM_ADD(t0, t1) \
static auto ngl_animated_##t0##_param_add = [](NGL::Node* node, const std::string key, int nb_elems, void* elems) { \
    NGL::Animated##t1* a0 = (NGL::Animated##t1*)node; \
    if (key == "keyframes") { \
        ngl_node** node_ptr = (ngl_node**)elems; \
        for (uint32_t j = 0; j<uint32_t(nb_elems); j++) { \
            auto akf1 = sp_cast<NGL::AnimKeyFrame##t1>((*node_ptr++)->v); \
            a0->kf.push_back(akf1); \
        } \
    } \
    else { TODO("key: %s", key.c_str()); } \
}


DEFINE_ANIMATED_PARAM_ADD(buffer, Buffer);
DEFINE_ANIMATED_PARAM_ADD(float, Float);
DEFINE_ANIMATED_PARAM_ADD(time, Float);
DEFINE_ANIMATED_PARAM_ADD(vec2, Vec2);
DEFINE_ANIMATED_PARAM_ADD(vec3, Vec3);
DEFINE_ANIMATED_PARAM_ADD(vec4, Vec4);
DEFINE_ANIMATED_PARAM_ADD(quat, Quat);

static auto ngl_group_param_add = [](NGL::Node* node, const std::string key, int nb_elems, void* elems) {
    NGL::Group* grp = (NGL::Group*)node;
    if (key == "children") {
        auto &children = grp->children;
        ngl_node** node_ptr = (ngl_node**)elems;
        for (uint32_t j = 0; j<uint32_t(nb_elems); j++) {
            children.push_back((*node_ptr++)->v);
        }
    }
    else {
        TODO("key: %s nb_elems: %d", key.c_str(), nb_elems);
    }
};

static auto ngl_timerangefilter_param_add = [](NGL::Node* node, const std::string key, int nb_elems, void* elems) {
    NGL::TimeRangeFilter* filter = (NGL::TimeRangeFilter*)node;
    if (key == "ranges") {
        auto& ranges = filter->ranges;
        ngl_node** node_ptr = (ngl_node**)elems;
        for (uint32_t j = 0; j<uint32_t(nb_elems); j++) {
            ranges.push_back(sp_cast<TimeRangeMode>((*node_ptr++)->v));
        }
    } else {
        TODO("key: %s nb_elems: %d", key.c_str(), nb_elems);
    }
};

static auto ngl_rtt_param_add = [](NGL::Node* node, const std::string key, int nb_elems, void* elems) {
    NGL::RenderToTexture* rtt = (NGL::RenderToTexture*)node;
    if (key == "color_textures") {
        ngl_node** nodes = (ngl_node**)elems;
        auto &textures = rtt->outputTextures;
        for (uint32_t j = 0; j<uint32_t(nb_elems); j++) {
            textures.push_back(sp_cast<NGL::Texture>(nodes[j]->v));
        }
    }
    else ERR("TODO: key: %s nb_elems: %d", key.c_str(), nb_elems);
};

#define NODE_CREATE_FN(t0, t1, p0, p1, ...) \
    case NGL_NODE_##t0: { \
        __VA_ARGS__ \
        node->param_set = p0; \
        node->param_add = p1; \
    } break
#define NODE_CREATE_FN0(t0, t1, p0, p1) \
    NODE_CREATE_FN(t0, t1, p0, p1, node->v.reset(new NGL::t1());)
#define NODE_CREATE_FN1(t0, t1, p0, p1, param) \
    NODE_CREATE_FN(t0, t1, p0, p1, node->v.reset(new NGL::t1(param));)
#define BUFFER_CREATE_FN(t0, t1) \
    NODE_CREATE_FN(BUFFER##t0, Buffer, ngl_buffer_param_set, nullptr, node->v.reset(new NGL::Buffer(DataType::t0, sizeof(t1))); )
#define STREAMED_BUFFER_CREATE_FN(t0, t1) \
    NODE_CREATE_FN(STREAMEDBUFFER##t0, StreamedBuffer, ngl_streamed_buffer_param_set, nullptr, \
        auto p0 = int_arg(ap); auto p1 = node_arg<NGL::Buffer>(ap), p2 = node_arg<NGL::Buffer>(ap); \
        node->v.reset(new NGL::StreamedBuffer(DataType::t0, sizeof(t1), p0, p1, p2)); )

struct ngl_node *ngl_node_create(int type, ...) {
    ngl_node *node = new ngl_node;
    node->type = type;
    va_list ap;
    va_start(ap, type);
    switch (type) {
    NODE_CREATE_FN1(CAMERA, Camera, ngl_camera_param_set, nullptr, node_arg<Node>(ap));
    NODE_CREATE_FN1(MEDIA, Media, ngl_media_param_set, nullptr, str_arg(ap)); 
    NODE_CREATE_FN0(GROUP, Group, ngl_group_param_set, ngl_group_param_add);
    NODE_CREATE_FN1(TEXT, Text, ngl_text_param_set, nullptr, str_arg(ap));
    NODE_CREATE_FN0(TEXTURE2D, Texture2D, ngl_texture2d_param_set, nullptr);
    NODE_CREATE_FN0(TEXTURE3D, Texture3D, ngl_texture3d_param_set, nullptr);
    NODE_CREATE_FN0(TEXTURECUBE, TextureCube, ngl_texture_cube_param_set, nullptr);
    BUFFER_CREATE_FN(BYTE, int8_t);
    BUFFER_CREATE_FN(BVEC2, bvec2);
    BUFFER_CREATE_FN(BVEC3, bvec3);
    BUFFER_CREATE_FN(BVEC4, bvec4);
    BUFFER_CREATE_FN(INT, int);
    BUFFER_CREATE_FN(INT64, int64_t);
    BUFFER_CREATE_FN(IVEC2, ivec2);
    BUFFER_CREATE_FN(IVEC3, ivec3);
    BUFFER_CREATE_FN(IVEC4, ivec4);
    BUFFER_CREATE_FN(SHORT, int16_t);
    BUFFER_CREATE_FN(SVEC2, i16vec2);
    BUFFER_CREATE_FN(SVEC3, i16vec3);
    BUFFER_CREATE_FN(SVEC4, i16vec4);
    BUFFER_CREATE_FN(UBYTE, uint8_t);
    BUFFER_CREATE_FN(UBVEC2, u8vec2);
    BUFFER_CREATE_FN(UBVEC3, u8vec3);
    BUFFER_CREATE_FN(UBVEC4, u8vec4);
    BUFFER_CREATE_FN(UINT, uint32_t);
    BUFFER_CREATE_FN(UIVEC2, uvec2);
    BUFFER_CREATE_FN(UIVEC3, uvec3);
    BUFFER_CREATE_FN(UIVEC4, uvec4);
    BUFFER_CREATE_FN(USHORT, uint16_t);
    BUFFER_CREATE_FN(USVEC2, u16vec2);
    BUFFER_CREATE_FN(USVEC3, u16vec3);
    BUFFER_CREATE_FN(USVEC4, u16vec4);
    BUFFER_CREATE_FN(FLOAT, float);
    BUFFER_CREATE_FN(VEC2, vec2);
    BUFFER_CREATE_FN(VEC3, vec3);
    BUFFER_CREATE_FN(VEC4, vec4);
    BUFFER_CREATE_FN(MAT4, mat4);
    STREAMED_BUFFER_CREATE_FN(INT, int);
    STREAMED_BUFFER_CREATE_FN(FLOAT, float);
    STREAMED_BUFFER_CREATE_FN(VEC2, vec2);
    STREAMED_BUFFER_CREATE_FN(VEC3, vec3);
    STREAMED_BUFFER_CREATE_FN(VEC4, vec4);
    STREAMED_BUFFER_CREATE_FN(MAT4, mat4);
    NODE_CREATE_FN0(BLOCK, Block, ngl_block_param_set, ngl_block_param_add);
    NODE_CREATE_FN0(UNIFORMINT, UniformInt, ngl_uniform_int_param_set, nullptr);
    NODE_CREATE_FN0(UNIFORMFLOAT, UniformFloat, ngl_uniform_float_param_set, nullptr);
    NODE_CREATE_FN0(UNIFORMVEC2, UniformVec2, ngl_uniform_vec2_param_set, nullptr);
    NODE_CREATE_FN0(UNIFORMVEC3, UniformVec3, ngl_uniform_vec3_param_set, nullptr);
    NODE_CREATE_FN0(UNIFORMVEC4, UniformVec4, ngl_uniform_vec4_param_set, nullptr);
    NODE_CREATE_FN0(UNIFORMQUAT, UniformQuat, ngl_uniform_quat_param_set, nullptr);
    NODE_CREATE_FN0(UNIFORMMAT4, UniformMat4, ngl_uniform_mat4_param_set, nullptr);
    NODE_CREATE_FN0(PROGRAM, GraphicsProgram, ngl_graphics_program_param_set, nullptr);
    NODE_CREATE_FN1(COMPUTEPROGRAM, ComputeProgram, nullptr, nullptr, str_arg(ap));
    NODE_CREATE_FN1(GEOMETRY, Geometry, ngl_geometry_param_set, nullptr,node_arg<Node>(ap));
    NODE_CREATE_FN(TRIANGLE, Triangle, ngl_triangle_param_set, nullptr,
        auto p0 = vec3_arg(ap), p1 = vec3_arg(ap), p2 = vec3_arg(ap);
        node->v.reset(new NGL::Triangle(p0, p1, p2));
    );
    NODE_CREATE_FN0(QUAD, Quad, ngl_quad_param_set, nullptr);
    NODE_CREATE_FN0(CIRCLE, Circle, ngl_circle_param_set, nullptr);
    NODE_CREATE_FN1(TIMERANGEFILTER, TimeRangeFilter, ngl_timerangefilter_param_set, ngl_timerangefilter_param_add, node_arg<Node>(ap));
    NODE_CREATE_FN1(TIMERANGEMODECONT, TimeRangeModeCont, ngl_timerangemodecont_param_set, nullptr, dbl_arg(ap));
    NODE_CREATE_FN1(TIMERANGEMODENOOP, TimeRangeModeNoOp, ngl_timerangemodenoop_param_set, nullptr, dbl_arg(ap));
    NODE_CREATE_FN(TIMERANGEMODEONCE, TimeRangeModeOnce, ngl_timerangemodeonce_param_set, nullptr,
        auto p0 = dbl_arg(ap), p1 = dbl_arg(ap);
        node->v.reset(new NGL::TimeRangeModeOnce(p0, p1));
    );
    NODE_CREATE_FN0(IDENTITY, Identity, nullptr, nullptr);
    NODE_CREATE_FN1(ROTATEQUAT, RotateQuat, ngl_rotate_quat_param_set, nullptr, node_arg<Node>(ap));
    NODE_CREATE_FN1(ROTATE, Rotate, ngl_rotate_param_set, nullptr, node_arg<Node>(ap));
    NODE_CREATE_FN1(SCALE, Scale, ngl_scale_param_set, nullptr, node_arg<Node>(ap));
    NODE_CREATE_FN1(TRANSLATE, Translate, ngl_translate_param_set, nullptr, node_arg<Node>(ap));
    NODE_CREATE_FN1(TRANSFORM, Transform, ngl_transform_param_set, nullptr, node_arg<Node>(ap));
    NODE_CREATE_FN1(RENDER, Render, ngl_render_param_set, nullptr, node_arg<Geometry>(ap));
    NODE_CREATE_FN(COMPUTE, Compute, ngl_compute_param_set, nullptr,
        auto p0 = int_arg(ap), p1 = int_arg(ap), p2 = int_arg(ap); auto p3 = node_arg<ComputeProgram>(ap);
        node->v.reset(new NGL::Compute(p0, p1, p2, p3));
    );
    NODE_CREATE_FN1(RENDERTOTEXTURE, RenderToTexture, ngl_rtt_param_set, ngl_rtt_param_add, node_arg<Node>(ap));
    NODE_CREATE_FN1(GRAPHICCONFIG, GraphicsConfig, ngl_graphics_config_param_set, nullptr, node_arg<Node>(ap));
    NODE_CREATE_FN(ANIMKEYFRAMEBUFFER, AnimKeyFrameBuffer, ngl_animkeyframe_buffer_param_set, nullptr,
        node->v.reset(new NGL::AnimKeyFrameBuffer(dbl_arg(ap), nullptr));
    );
    NODE_CREATE_FN(ANIMKEYFRAMEFLOAT, AnimKeyFrameFloat, ngl_animkeyframe_float_param_set, nullptr,
        auto p0 = dbl_arg(ap), p1 = dbl_arg(ap);
        node->v.reset(new NGL::AnimKeyFrameFloat(p0, p1)); 
    );
    NODE_CREATE_FN(ANIMKEYFRAMEVEC2, AnimKeyFrameVec2, ngl_animkeyframe_vec2_param_set, nullptr,
        auto p0 = dbl_arg(ap); auto p1 = vec2_arg(ap);
        node->v.reset(new NGL::AnimKeyFrameVec2(p0, p1));
    );
    NODE_CREATE_FN(ANIMKEYFRAMEVEC3, AnimKeyFrameVec3, ngl_animkeyframe_vec3_param_set, nullptr,
        auto p0 = dbl_arg(ap); auto p1 = vec3_arg(ap);
        node->v.reset(new NGL::AnimKeyFrameVec3(p0, p1));
    );
    NODE_CREATE_FN(ANIMKEYFRAMEVEC4, AnimKeyFrameVec4, ngl_animkeyframe_vec4_param_set, nullptr,
        auto p0 = dbl_arg(ap); auto p1 = vec4_arg(ap);
        node->v.reset(new NGL::AnimKeyFrameVec4(p0, p1));
    );
    NODE_CREATE_FN(ANIMKEYFRAMEQUAT, AnimKeyFrameQuat, ngl_animkeyframe_quat_param_set, nullptr,
        auto p0 = dbl_arg(ap); auto p1 = quat_arg(ap);
        node->v.reset(new NGL::AnimKeyFrameQuat(p0, p1));
    );
    NODE_CREATE_FN1(ANIMATEDBUFFERFLOAT, AnimatedBuffer, nullptr, ngl_animated_buffer_param_add, DataType::FLOAT);
    NODE_CREATE_FN1(ANIMATEDBUFFERVEC2, AnimatedBuffer, nullptr, ngl_animated_buffer_param_add, DataType::VEC2);
    NODE_CREATE_FN1(ANIMATEDBUFFERVEC3, AnimatedBuffer, nullptr, ngl_animated_buffer_param_add, DataType::VEC3);
    NODE_CREATE_FN1(ANIMATEDBUFFERVEC4, AnimatedBuffer, nullptr, ngl_animated_buffer_param_add, DataType::VEC4);
    NODE_CREATE_FN0(ANIMATEDFLOAT, AnimatedFloat, nullptr, ngl_animated_float_param_add);
    NODE_CREATE_FN0(ANIMATEDTIME, AnimatedTime, nullptr, ngl_animated_time_param_add);
    NODE_CREATE_FN0(ANIMATEDVEC2, AnimatedVec2, nullptr, ngl_animated_vec2_param_add);
    NODE_CREATE_FN0(ANIMATEDVEC3, AnimatedVec3, nullptr, ngl_animated_vec3_param_add);
    NODE_CREATE_FN0(ANIMATEDVEC4, AnimatedVec4, nullptr, ngl_animated_vec4_param_add);
    NODE_CREATE_FN0(ANIMATEDQUAT, AnimatedQuat, ngl_animated_quat_param_set, ngl_animated_quat_param_add);
    default:
        TODO("type: %s", getFourCC(type).c_str());
        break;
    }
    va_end(ap);
    return node;
}

int ngl_node_param_set(struct ngl_node *node, const char *key, ...) {
    int ret = 0;
    va_list ap;
    va_start(ap, key);
    if (std::string(key) == "label") {
        node->v->label = str_arg(ap);
    }
    else if (!node->param_set) TODO("type: %s", getFourCC(node->type).c_str());
    else node->param_set(node->v.get(), key, ap);
    va_end(ap);
    return ret;
}

char *ngl_node_serialize(const struct ngl_node *node, const char* cFileName) {
    std::string filename = cFileName;
    if (filename == "") {
        filename = tmpnam(nullptr);
    }
    NGL::Serialize(node->v, filename).run();
    return strdup(filename.c_str());
}

struct ngl_node *ngl_node_deserialize(const char *cFileName) {
    ngl_node* node = new ngl_node;
    node->v = NGL::Deserialize(cFileName).run();
    return node;
}

void ngl_log_set_min_level(int level) { TODO();
}

int ngl_draw(struct ngl_ctx *s, double t) {
    s->v->draw(t);
    return 0;
}

void ngl_freep(struct ngl_ctx **ss) {
    ngl_ctx *ctx = *ss;
    if (!ctx)
        return;
    delete ctx;
}

char *ngl_dot(struct ngl_ctx *s, double t) { TODO();
    return strdup("");
}

int ngl_easing_evaluate(const char *name, double *args, int nb_args,
                        double *offsets, double t, double *v) {
    return NGL::AnimationUtil::evaluateEasing(EasingIdMap.at(name), args, nb_args, offsets, t, v);
}

int ngl_easing_solve(const char *name, double *args, int nb_args,
                     double *offsets, double v, double *t) {
    return NGL::AnimationUtil::solveEasing(EasingIdMap.at(name), args, nb_args, offsets, v, t);
}

