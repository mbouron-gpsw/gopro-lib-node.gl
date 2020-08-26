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
#define NOMINMAX
#include "NGLBackend.h"
#include "DebugUtil.h"
using namespace NGL;

void QuadPriv::init(GraphicsContext *ctx) {
    Quad* q = (Quad*)p;
    vector<vec3> pos = { q->corner, q->corner + q->height, q->corner + q->width, q->corner + q->width + q->height };
    vector<vec2> texCoord = { q->uv_corner, q->uv_corner + q->uv_height, q->uv_corner + q->uv_width, q->uv_corner + q->uv_width + q->uv_height };
    p->numVerts = uint32_t(pos.size());
    bPos = VertexBuffer::create(pos);
    bTexCoord = VertexBuffer::create(texCoord);
}

void Texture2DPriv::init(GraphicsContext *ctx) {
    this->ctx = ctx;
    if (v) return;
    Texture2D* t = (Texture2D*)p;
    if (t->media) {
        v = glGraphicsEngine::Texture::create(t->media->filename.c_str());
        t->media->width = t->w;
        t->media->height = t->h;
    } else {
        uint32_t size = t->w * t->h * 4;
        v = make_unique<glGraphicsEngine::Texture>(nullptr, GL_RGBA8, size, t->w, t->h, 1);
    }
}

void GraphicsProgramPriv::init(GraphicsContext *ctx) {
    this->ctx = ctx;
    createProgram();
}

void GraphicsProgramPriv::createProgram() {
    auto* gp = (GraphicsProgram*)p;
    program = make_unique<glGraphicsEngine::GraphicsProgram>(
        make_unique<VertexShaderModule>(gp->vertexShaderFile).get(),
        make_unique<FragmentShaderModule>(gp->fragmentShaderFile).get()
    );
}

void RenderPriv::init(GraphicsContext *ctx) { init(ctx, false); }

void RenderPriv::init(GraphicsContext *ctx, bool renderOffscreen) {
    this->ctx = ctx;
    p->renderOffscreen = renderOffscreen;
    ((GeometryPriv*)p->geom->priv.get())->init(ctx);
    ((GraphicsProgramPriv*)p->program->priv.get())->init(ctx);
    for (auto texture : p->textures) ((TexturePriv*)texture->priv.get())->init(ctx);
    auto graphicsProgram = ((GraphicsProgramPriv*)p->program->priv.get())->graphicsProgram();
    auto& uniformInfos = graphicsProgram->uniformInfos;
    auto calcUBOSize = [&]() -> uint32_t {
        uint32_t uboSize = 0;
        for (auto it = uniformInfos.begin(); it != uniformInfos.end(); it++) {
            uboSize = glm::max(uboSize, it->second.offset + it->second.size);
        }
        return uboSize;
    };
    uboSize = calcUBOSize();
    if (uboSize > 0) {
        ubo = make_unique<UniformBuffer>(nullptr, uboSize);
        updateUBO();
    }
}

void RenderPriv::draw(Graphics* graphics) {
    auto graphicsProgram = ((GraphicsProgramPriv*)p->program->priv.get())->graphicsProgram();
    if (!p->renderOffscreen) {
        auto window = ctx->window;
        uint32_t w = window->w, h = window->h;
        graphics->setViewport(0, 0, w, h);
        graphics->setScissor(0, 0, w, h);
    }
    graphicsProgram->bind();
    uint32_t attrIndex = 0;
    ((GeometryPriv*)p->geom->priv.get())->bPos->bind(attrIndex++);
    ((GeometryPriv*)p->geom->priv.get())->bTexCoord->bind(attrIndex++);
    uint32_t uniformIndex = 0;
    for (auto & texture : p->textures) { 
        ((TexturePriv*)texture->priv.get())->v->bind(uniformIndex++); 
    }
    if (ubo) ubo->bind(uniformIndex++);
    graphics->draw(GL_TRIANGLE_STRIP, p->geom->numVerts);
}

void RenderPriv::updateUBO() {
    auto &uniformInfos = ((GraphicsProgramPriv*)p->program->priv.get())->graphicsProgram()->uniformInfos;
    vector<float> uboData(uboSize / sizeof(float));
    for (auto it = p->uniforms.begin(); it != p->uniforms.end(); it++) {
        auto& uniform = it->second;
        auto& uniformInfo = uniformInfos.at(it->first);
        memcpy(&uboData.data()[uniformInfo.offset / sizeof(float)], uniform->data, uniform->size);
    }
    ubo->update(uboData.data(), uboSize);
}

void Render::update(float elapsed) {
    for (auto it = uniforms.begin(); it != uniforms.end(); it++) it->second->update(elapsed);
    if (((RenderPriv*)priv.get())->ubo) ((RenderPriv*)priv.get())->updateUBO();
}

void RenderToTexturePriv::init(GraphicsContext* ctx) {
    this->ctx = ctx;
    auto& outputTexture = p->outputTextures[0];
    ((Texture2DPriv*)outputTexture->priv.get())->init(ctx);
    vector<glGraphicsEngine::Texture*> attachments = { 
        ((Texture2DPriv*)outputTexture->priv.get())->v.get() 
    };
    outputFramebuffer = make_unique<Framebuffer>(
        attachments, outputTexture->w, outputTexture->h);
    ((RenderPriv*)p->render->priv.get())->init(ctx, true);
}

void RenderToTexturePriv::draw(Graphics* graphics) {
    outputFramebuffer->bind();
    graphics->setViewport(0, 0, outputFramebuffer->w, outputFramebuffer->h);
    graphics->setScissor(0, 0, outputFramebuffer->w, outputFramebuffer->h);
    ((RenderPriv*)p->render->priv.get())->draw(graphics);
    outputFramebuffer->unbind();
}

void Texture2D::init() { priv = make_unique<Texture2DPriv>(this); }
void NGL::GraphicsProgram::init() { priv = make_unique<GraphicsProgramPriv>(this); }
void Quad::init() { priv = make_unique<QuadPriv>(this); }
void Render::init() { priv = make_unique<RenderPriv>(this); }
void RenderToTexture::init() { priv = make_unique<RenderToTexturePriv>(this); }
void Group::init() { priv = make_unique<GroupPriv>(this); }

