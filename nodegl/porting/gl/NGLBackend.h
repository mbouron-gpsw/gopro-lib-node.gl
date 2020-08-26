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
#include "NGL.h"
#include "GLGraphicsEngine.h"
using namespace glGraphicsEngine;
using namespace std;

namespace NGL {
    struct NodePriv : NodeBackend {
        NodePriv(Node* thiz) : thiz(thiz) {}
        virtual ~NodePriv() {}
        virtual void init(GraphicsContext* ctx) {}
        virtual void draw(Graphics* graphics) {}
        Node* thiz = nullptr;
    };

    struct TexturePriv : NodePriv {
        TexturePriv(Texture* p) : NodePriv(p), p(p) {}
        virtual ~TexturePriv() {}
        unique_ptr<glGraphicsEngine::Texture> v;
        GraphicsContext* ctx = nullptr;
        Texture* p = nullptr;
    };

    struct Texture2DPriv : TexturePriv { 
        Texture2DPriv(Texture2D* p) : TexturePriv(p) {}
        virtual ~Texture2DPriv() {}
        virtual void init(GraphicsContext* ctx);
    };

    struct GeometryPriv : NodePriv {
        GeometryPriv(Geometry* p) : NodePriv(p), p(p) {}
        virtual ~GeometryPriv() {}
        unique_ptr<VertexBuffer> bPos, bTexCoord;
        Geometry* p = nullptr;
    };

    struct QuadPriv : GeometryPriv {
        QuadPriv(Quad* p) : GeometryPriv(p) {}
        virtual ~QuadPriv() {}
        virtual void init(GraphicsContext* ctx);
    };

    struct ProgramPriv : NodePriv {
        ProgramPriv(Program* p) : NodePriv(p), p(p) {}
        virtual ~ProgramPriv() {}
        virtual void createProgram() {}
        GraphicsContext* ctx = nullptr;
        unique_ptr<glGraphicsEngine::Program> program;
        Program* p = nullptr;
    };

    struct GraphicsProgramPriv : public ProgramPriv {
        GraphicsProgramPriv(GraphicsProgram* p) : ProgramPriv(p) {}
        virtual ~GraphicsProgramPriv() {}
        virtual void init(GraphicsContext* ctx);
        virtual void createProgram();
        glGraphicsEngine::GraphicsProgram* graphicsProgram() { return (glGraphicsEngine::GraphicsProgram*)program.get(); }
    };

    struct RenderPriv : public NodePriv {
        RenderPriv(Render* p) : NodePriv(p), p(p) {}
        virtual ~RenderPriv() {}
        void init(GraphicsContext* ctx, bool renderOffscreen);
        virtual void init(GraphicsContext* ctx);
        virtual void draw(Graphics* graphics);
        void updateUBO();
        uint32_t U_TEXTURE, B_POS, B_TEXCOORD, U_UBO;
        GraphicsContext* ctx = nullptr;
        unique_ptr<UniformBuffer> ubo;
        uint32_t uboSize = 0;
        Render* p = nullptr;
    };

    struct RenderToTexturePriv : public NodePriv {
        RenderToTexturePriv(RenderToTexture* p) : NodePriv(p), p(p) {}
        virtual ~RenderToTexturePriv() {}
        virtual void init(GraphicsContext* ctx);
        virtual void draw(Graphics* graphics);
        GraphicsContext* ctx = nullptr;
        unique_ptr<Framebuffer> outputFramebuffer;
        RenderToTexture* p = nullptr;
    };

    struct GroupPriv : NodePriv {
        GroupPriv(Group* p) : NodePriv(p), p(p) {}
        virtual ~GroupPriv() {}
        virtual void init(GraphicsContext* ctx) {
            for (auto& child : p->children) {
                ((NodePriv*)child->priv.get())->init(ctx);
            }
        }
        virtual void draw(Graphics* graphics) {
            for (auto& child : p->children) {
                ((NodePriv*)child->priv.get())->draw(graphics);
            }
        }
        Group* p = nullptr;
    };
};