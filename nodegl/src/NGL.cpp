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

#include "NGL.h"
#include "NGLBackend.h"
#include "FontData.h"
#include "MathUtils.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
using namespace NGL;
#define vp glm::value_ptr
#define COLOR_VEC4_TO_U32(vec) (((uint32_t)((vec)[0] * 255) & 0xff) << 24 | \
                                     ((uint32_t)((vec)[1] * 255) & 0xff) << 16 | \
                                     ((uint32_t)((vec)[2] * 255) & 0xff) <<  8 | \
                                     ((uint32_t)((vec)[3] * 255) & 0xff))
#ifdef GRAPHICS_BACKEND_VULKAN
#define GL_TO_VK_MAT mat4(1,0,0,0, 0,-1,0,0, 0,0,0.5f,0, 0,0,0.5f,1)
#endif

void Context::init() {
    priv = make_unique<ContextPriv>(this, app.get());
    modelViewMat = make_shared<UniformMat4>();
    projMat = make_shared<UniformMat4>();
    normalMat = make_shared<UniformMat4>();
    texCoord0Mat = make_shared<UniformMat4>();
#ifdef GRAPHICS_BACKEND_VULKAN
    projMat->value = GL_TO_VK_MAT;
#endif
}
void Context::setConfig(Config config) {
    auto ctxPriv = ((ContextPriv*)priv.get());
    this->config = config;
    ctxPriv->setConfig(&this->config);
}
void Context::draw(double t) {
    if (!scene) return;
    time = t;
    ((ContextPriv*)priv.get())->draw(t);
}

void Context::setScene(sp<Node> scene) {
    this->scene = scene;
    scene->setContext(this);
    this->scenePriv = scene->getBackend();
    ((ContextPriv*)priv.get())->setScene((NodePriv*)scenePriv.get());
}

Data* NGL::Buffer::getData() {
    if (block) {
        return sp_cast<Buffer>(block->fields[blockField])->data.get();
    }
    else return data.get();
}

void Render::setContext(Context* ctx) {
    this->ctx = ctx;
    uniforms[MODELVIEW_MATRIX_ID] = ctx->modelViewMat;
    uniforms[PROJECTION_MATRIX_ID] = ctx->projMat;
    uniforms[NORMAL_MATRIX_ID] = ctx->normalMat;
    uniforms[TEXCOORD_0_MATRIX_ID] = ctx->texCoord0Mat;
    if (geom) geom->setContext(ctx);
    if (program) program->setContext(ctx);
    for (auto& it : textures) it.second->setContext(ctx);
    for (auto& it : uniforms) it.second->setContext(ctx);
    for (auto& it : blocks) it.second->setContext(ctx);
}

void NGL::Compute::setContext(Context* ctx) {
    this->ctx = ctx;
    if (program) program->setContext(ctx);
    for (auto& it : uniforms) it.second->setContext(ctx);
    for (auto& it : blocks) it.second->setContext(ctx);
}

static map<DataType, uint32_t> strideMap = {
    { DataType::INT, 4 }, { DataType::FLOAT, 4 }, 
    { DataType::VEC2, 8 }, { DataType::VEC3, 12 }, { DataType::VEC4, 16 }, 
    { DataType::MAT4, 64 }, 
    { DataType::QUAT, sizeof(quat) }
};

void* UniformMat4::getData() {
    if (transform) {
        value = transform->getMat() * transform->getChildMat();
    }
    return data;
}

void UniformMat4::setContext(Context* ctx) {
    if (transform) transform->setContext(ctx);
}

void* UniformQuat::getData() {
    if (asMat4) {
        ngli_mat4_rotate_from_quat(vp(valueMat4), vp(value));
        return &valueMat4;
    }
    else return data;
}

void* AnimatedQuat::getData() {
    if (asMat4) {
        ngli_mat4_rotate_from_quat(vp(valueMat4), vp(value));
        return &valueMat4;
    }
    else return data;
}

AnimatedBuffer::AnimatedBuffer(DataType dataType): dataType(dataType) {
    LOG("dataType: %d", dataType); data = nullptr; size = 0;
    stride = strideMap.at(dataType);
}

mat4 Rotate::getMat() {
    if (needsUpdate) {
        float rad = angle * M_PI / 180.0f;
        vec3 normedAxis = normalize(axis);
        ngli_mat4_rotate(vp(mat), rad, vp(normedAxis));

        static const float zvec[3] = {0};
        bool useAnchor = memcmp(vp(anchor), zvec, sizeof(zvec));

        if (useAnchor) {
            mat4 transm;
            ngli_mat4_translate(vp(transm), anchor[0], anchor[1], anchor[2]);
            ngli_mat4_mul(vp(mat), vp(transm), vp(mat));
            ngli_mat4_translate(vp(transm), -anchor[0], -anchor[1], -anchor[2]);
            ngli_mat4_mul(vp(mat), vp(mat), vp(transm));
        }
        needsUpdate = false;
    }
    return mat;
}

mat4 RotateQuat::getMat() {
    if (needsUpdate) {
        ngli_mat4_rotate_from_quat(vp(mat), vp(v));

        static const float zvec[3] = {0};
        bool useAnchor = memcmp(vp(anchor), zvec, sizeof(zvec));
        if (useAnchor) {
            mat4 transm;
            ngli_mat4_translate(vp(transm), anchor[0], anchor[1], anchor[2]);
            ngli_mat4_mul(vp(mat), vp(transm), vp(mat));
            ngli_mat4_translate(vp(transm), -anchor[0], -anchor[1], -anchor[2]);
            ngli_mat4_mul(vp(mat), vp(mat), vp(transm));
        }
        needsUpdate = false;
    }
    return mat;
}

mat4 Scale::getMat() {
    if (needsUpdate) {
        mat = translate(anchor) * scale(v) * translate(-anchor);
        needsUpdate = false;
    }
    return mat;
}

mat4 Translate::getMat() {
    if (needsUpdate) {
        mat = translate(v);
        needsUpdate = false;
    }
    return mat;
}

#define APPLY_CAMERA_TRANSFORM(p) \
    if (p##Transform) p##Param = vec3(p##Transform->getMat() * p##Transform->getChildMat() * vec4(p, 1.0));

mat4 Camera::getMat() {
    if (needsUpdate) {
        vec3 eyeParam = eye, centerParam = center, upParam = up;
        vec3 ground = cross(normalize(eye - center), normalize(up));
        APPLY_CAMERA_TRANSFORM(eye);
        APPLY_CAMERA_TRANSFORM(center);
        APPLY_CAMERA_TRANSFORM(up);
        if ((eyeTransform || centerTransform) && !upTransform) {
            upParam = cross(normalize(center - eye), ground);
        }
        ngli_mat4_look_at(vp(mat), vp(eyeParam), vp(centerParam), vp(upParam));
        auto& c = clippingParams;
        if (perspectiveProj) {
            auto& p = perspectiveParams;
            ngli_mat4_perspective(vp(projMat), p.fov, p.aspect, c.pNear, c.pFar);
        } else {
            auto& p = orthographicParams;
            ngli_mat4_orthographic(vp(projMat), p.left, p.right, p.bottom, p.top, c.pNear, c.pFar);
        }
    #ifdef GRAPHICS_BACKEND_VULKAN
        ngli_mat4_mul(vp(projMat), vp(GL_TO_VK_MAT), vp(projMat));
    #endif
        needsUpdate = false;
    }
    return mat;
}

void Text::initCanvas() {
    int canvasWidth, canvasHeight;

    /* Init canvas dimensions according to text (and user padding settings) */
    initCanvasDimensions(canvasWidth, canvasHeight);
    canvasWidth += 2 * padding;
    canvasHeight += 2 * padding;

    /* Pad it to match container ratio */
    const float boxWidth = length(boxWidthVec);
    const float boxHeight = length(boxHeightVec);
    const ivec2 ar = aspectRatio;
    const float boxRatio = ar[0] * boxWidth  / (float)(ar[1] * boxHeight );
    const float texRatio = canvasWidth / (float)canvasHeight;
    const int aspectPadW = (texRatio < boxRatio ? canvasHeight * boxRatio - canvasWidth : 0);
    const int aspectPadH = (texRatio < boxRatio ? 0 : canvasWidth / boxRatio - canvasHeight);

    /* Adjust canvas size to impact text size */
    const int texW = (canvasWidth + aspectPadW) / fontScale;
    const int texH = (canvasHeight + aspectPadH) / fontScale;
    const int padW = texW - canvasWidth;
    const int padH = texH - canvasHeight;
    canvasWidth = glm::max(1, texW);
    canvasHeight = glm::max(1, texH);

    /* Adjust text position according to alignment settings */
    const int tx = (hAlign == HALIGN_CENTER ? padW / 2 :
                    hAlign == HALIGN_RIGHT  ? padW     :
                    0) + padding;
    const int ty = (vAlign == VALIGN_CENTER ? padH / 2 :
                    vAlign == VALIGN_BOTTOM ? padH     :
                    0) + padding;

    /* Allocate, draw background, print text */
    canvas.reset(new Canvas(canvasWidth, canvasHeight));
    const uint32_t fg = COLOR_VEC4_TO_U32(fgColor);
    const uint32_t bg = COLOR_VEC4_TO_U32(bgColor);
    Rect rect = { 0, 0, canvasWidth, canvasHeight };
    canvas->drawRect(rect, bg);
    canvas->drawText(tx, ty, str, fg);
}

void Text::initCanvasDimensions(int &w, int &h) {
    w = 0;
    h = FONT_H;
    int cur_w = 0;
    for (int i = 0; i < str.size(); i++) {
        if (str[i] == '\n') {
            cur_w = 0;
            h += FONT_H;
        } else {
            cur_w += FONT_W;
            w = glm::max(w, cur_w);
        }
    }
}
