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
#include "graphics/Graphics.h"
#include "graphics/GraphicsContext.h"
#include <set>
extern "C" {
#include <sxplayer.h>
}
using namespace ngfx;
using namespace std;

namespace NGL {
    class NGLApplication;

    struct GraphicsState {
        bool rttPass = false;
        bool enableDepthStencil = false;
        uint32_t numSamples = 1, numColorAttachments = 1;
        PixelFormat colorFormat, depthFormat;
    };

    struct NodePriv : public NodeBackend {
        NodePriv(Node* thiz) : thiz(thiz) {}
        virtual ~NodePriv() {}
        virtual void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state = {}) {}
        virtual void compute(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) {}
        virtual void draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) {}
        virtual void update() {}
        Node* thiz = nullptr;
    };

    struct ContextPriv : public ContextBackend {
        ContextPriv(Context* thiz, NGLApplication* app);
        virtual ~ContextPriv();
        virtual void draw(double t);
        virtual void setScene(NodePriv* scene);
        virtual void setConfig(Config* cfg);
        Context* thiz = nullptr;
        NGLApplication* app = nullptr;
        Config* cfg = nullptr;
    };

    struct BufferPriv : public NodePriv {
        BufferPriv(Buffer* p) : NodePriv(p), p(p) {}
        virtual ~BufferPriv() {}
        Buffer* p = nullptr;
    };

    struct AnimatedNodePriv;

    struct StreamedBufferPriv : public NodePriv {
        StreamedBufferPriv(StreamedBuffer* p) : NodePriv(p), p(p) {}
        virtual ~StreamedBufferPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void update() override;
        StreamedBuffer* p = nullptr;
        sp<AnimatedNodePriv> timeAnim;
    };

    struct BlockPriv : public NodePriv {
        BlockPriv(Block* p): NodePriv(p), p(p) {}
        virtual ~BlockPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void update() override;
        Block* p = nullptr;
        map<string, sp<NodePriv>> fields;
        sp<ngfx::Buffer> buffer;
    };

    struct TextPriv : public NodePriv {
        TextPriv(Text* p): NodePriv(p), p(p) {}
        virtual ~TextPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void draw(CommandBuffer *commandBuffer, Graphics *graphics, GraphicsState state) override;
        void update() override;
        void createPipeline(RenderPass *renderPass, PixelFormat colorFormat, PixelFormat depthFormat);
        GraphicsContext* ctx = nullptr;
        bool rtt = false;
        Text* p = nullptr;
        uint32_t numVerts = 0;
        unique_ptr<ngfx::Buffer> bPos, bTexCoord;
        PrimitiveTopology topology;
        uint32_t B_POS, B_TEXCOORD, U_UBO, U_TEXTURE;
        GraphicsPipeline* graphicsPipeline = nullptr;
        unique_ptr<ngfx::Texture> texture;
        unique_ptr<ngfx::Buffer> ubo;
        struct UBO_0_Data {
            mat4 ngl_modelview_matrix;
            mat4 ngl_projection_matrix;
        };
    };

    struct TexturePriv : public NodePriv {
        TexturePriv(Texture* p) : NodePriv(p), p(p) {}
        virtual ~TexturePriv() {}
        sp<ngfx::Texture> v;
        GraphicsContext* ctx = nullptr;
        Texture* p = nullptr;
        uint32_t width() { return p->w; }
        uint32_t height() { return p->h; }
        uint32_t depth() { return p->d; }
        uint32_t numLayers() { return p->numLayers; }
    };

    struct Texture2DPriv : public TexturePriv {
        Texture2DPriv(Texture2D* p) : TexturePriv(p), p0(p) {}
        virtual ~Texture2DPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void update() override;
        Texture2D* p0 = nullptr;
        sp<NodePriv> dataSrc;
    };

    struct Texture3DPriv : public TexturePriv {
        Texture3DPriv(Texture3D* p) : TexturePriv(p), p0(p) {}
        virtual ~Texture3DPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void update() override;
        Texture3D* p0 = nullptr;
        sp<NodePriv> dataSrc;
    };

    struct TextureCubePriv : public TexturePriv {
        TextureCubePriv(TextureCube* p) : TexturePriv(p), p0(p) {}
        virtual ~TextureCubePriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void update() override;
        uint32_t dim() { return p0->dim(); }
        TextureCube* p0 = nullptr;
        sp<NodePriv> dataSrc;
    };

    struct GeometryPriv : public NodePriv {
        GeometryPriv(Geometry* p) : NodePriv(p), p(p) {}
        virtual ~GeometryPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void update() override;
        sp<ngfx::Buffer> bPos, bNormal, bTexCoord;
        sp<ngfx::Buffer> bIndices;
        Geometry* p = nullptr;
        uint32_t numVerts = 0;
        uint32_t numIndices = 0;
        sp<NodePriv> verts, normals, uvCoords, indices;
    };

    struct TrianglePriv : public GeometryPriv {
        TrianglePriv(Triangle* p) : GeometryPriv(p), p0(p) {}
        virtual ~TrianglePriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        Triangle* p0 = nullptr;
    };

    struct QuadPriv : public GeometryPriv {
        QuadPriv(Quad* p) : GeometryPriv(p), p0(p) {}
        virtual ~QuadPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        Quad* p0 = nullptr;
    };

    struct CirclePriv : public GeometryPriv {
        CirclePriv(Circle* p) : GeometryPriv(p), p0(p) {}
        virtual ~CirclePriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        Circle* p0 = nullptr;
    };

    struct ProgramPriv : public NodePriv {
        ProgramPriv(Program* p) : NodePriv(p), p(p) {}
        virtual ~ProgramPriv() {}
        GraphicsContext* ctx = nullptr;
        Program* p = nullptr;

    };
    using DescriptorInfo = ShaderModule::DescriptorInfo;
    using DescriptorInfos = ShaderModule::DescriptorInfos;
    using BufferInfos = ShaderModule::BufferInfos;
    using BufferMemberInfos = ShaderModule::BufferMemberInfos;

    struct GraphicsProgramPriv : public ProgramPriv {
        GraphicsProgramPriv(GraphicsProgram* p) : ProgramPriv(p), p0(p) {}
        virtual ~GraphicsProgramPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override {
            init(ctx, state, PrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        }
        virtual void init(GraphicsContext* ctx, GraphicsState state, PrimitiveTopology topology,
            set<string> instanceAttributes = {});
        virtual void createPipeline(GraphicsState state);
        void initBufferInfos(
                const BufferInfos &vsUboInfos,
                const BufferInfos &fsUboInfos,
                BufferInfos& bufferInfos);
        void initDescriptorInfos(
                DescriptorInfos& vsDescriptorInfos,
                DescriptorInfos& fsDescriptorInfos);
        int32_t getAttrLocation(string name);
        int32_t getDescriptorBinding(string name);
        unique_ptr<VertexShaderModule> vs;
        unique_ptr<FragmentShaderModule> fs;
        GraphicsProgram* p0 = nullptr;
        PrimitiveTopology topology;
        set<string> instanceAttributes;
        BufferInfos uniformBufferInfos, shaderStorageBufferInfos;
        DescriptorInfos descriptorInfos;
        unique_ptr<GraphicsPipeline> graphicsPipeline;
    };

    struct ComputeProgramPriv : public ProgramPriv {
        ComputeProgramPriv(ComputeProgram* p) : ProgramPriv(p), p0(p) {}
        virtual ~ComputeProgramPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        virtual void createPipeline();
        int32_t getDescriptorBinding(string name);
        unique_ptr<ComputeShaderModule> cs;
        ComputeProgram* p0 = nullptr;
        BufferInfos *uniformBufferInfos = nullptr, *shaderStorageBufferInfos = nullptr;
        DescriptorInfos *descriptorInfos = nullptr;
        unique_ptr<ComputePipeline> computePipeline;
    };

    struct UniformPriv;

    struct BufferData {
        sp<ngfx::Buffer> buffer;
        uint32_t size;
        ShaderModule::BufferInfo* bufferInfo = nullptr;
    };
    
    struct TextureData {
        sp<TexturePriv> texture;
        uint32_t set;
    };

    struct RenderPriv : public NodePriv {
        RenderPriv(Render* p) : NodePriv(p), p(p) {}
        virtual ~RenderPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) override;
        void update() override;
        GraphicsContext* ctx = nullptr;
        Render* p = nullptr;
        sp<GeometryPriv> geom;
        sp<GraphicsProgramPriv> graphicsProgram;
        std::map<std::string, BufferData> ssbos;
        std::map<std::string, BufferData> ubos;
        std::map<std::string, TextureData> textures;
        std::map<std::string, sp<NodePriv>> uniforms;
        std::map<std::string, sp<BlockPriv>> blocks;
        map<uint32_t, unique_ptr<ngfx::Buffer>> instanceAttributes;
        bool rtt = false;
        int32_t B_POS = -1, B_NORMAL = -1, B_TEXCOORD = -1;
    };

    struct ComputePriv : public NodePriv {
        ComputePriv(Compute* p) : NodePriv(p), p(p) {}
        virtual ~ComputePriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void compute(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) override;
        void update() override;
        GraphicsContext* ctx = nullptr;
        std::map<std::string, BufferData> ssbos;
        std::map<std::string, BufferData> ubos;
        Compute* p = nullptr;
        sp<ComputeProgramPriv> computeProgram;
        std::map<std::string, TextureData> textures;
        std::map<std::string, sp<NodePriv>> uniforms;
        std::map<std::string, sp<BlockPriv>> blocks;
        bool rtt = false;
    };

    struct RenderToTexturePriv : public NodePriv {
        RenderToTexturePriv(RenderToTexture* p) : NodePriv(p), p(p) {}
        virtual ~RenderToTexturePriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) override;
        void update() override {
            if (child) child->update();
        }
        GraphicsContext* ctx = nullptr;
        sp<Texture2DPriv> depthTextureRef;
        sp<Framebuffer> outputFramebuffer;
        RenderToTexture* p = nullptr;
        RenderPass* renderPass = nullptr;
        sp<NodePriv> child;
        vector<sp<TexturePriv>> outputTextures;
        sp<ngfx::Texture> multisampleColorTexture;
        sp<ngfx::Texture> depthTexture, multisampleDepthTexture;

    };

    struct GroupPriv : public NodePriv {
        GroupPriv(Group* p) : NodePriv(p), p(p) {}
        virtual ~GroupPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void compute(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) override {
            for (auto& child : children) {
                child->compute(commandBuffer, graphics, state);
            }
        }
        void draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) override {
            for (auto& child : children) {
                child->draw(commandBuffer, graphics, state);
            }
        }
        void update() override {
            for (auto child : children) child->update();
        }
        Group* p = nullptr;
        std::vector<sp<NodePriv>> children;
    };

    struct GraphicsConfigPriv : public NodePriv {
        GraphicsConfigPriv(GraphicsConfig* p): NodePriv(p), p(p) {}
        virtual ~GraphicsConfigPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) override;
        void update() override {
            child->update();
        }
        GraphicsConfig* p = nullptr;
        sp<NodePriv> child;
    };

    struct MediaPriv : public NodePriv {
        MediaPriv(Media* p) : NodePriv(p), p(p) {}
        virtual ~MediaPriv();
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void update() override;
        struct Frame {
            ~Frame() { if (v) { sxplayer_release_frame(v); v = nullptr; } }
            sxplayer_frame *v = nullptr;
        };
        void getFrame();
        Media* p = nullptr;
        sxplayer_ctx* playerCtx = nullptr;
        std::unique_ptr<Frame> frame;
        double time = 0.0;
        sp<AnimatedNodePriv> timeAnim;
    };

    struct TimeRangeFilterPriv : public NodePriv {
        TimeRangeFilterPriv(TimeRangeFilter* p): NodePriv(p), p(p) {}
        virtual ~TimeRangeFilterPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) override;
        void update() override;
        TimeRangeFilter* p = nullptr;
        sp<NodePriv> child;
    };

    struct TimeRangeModeContPriv : public NodePriv {
        TimeRangeModeContPriv(TimeRangeModeCont* p): NodePriv(p), p(p) {}
        virtual ~TimeRangeModeContPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override {}
        TimeRangeModeCont* p = nullptr;
    };

    struct TimeRangeModeNoOpPriv : public NodePriv {
        TimeRangeModeNoOpPriv(TimeRangeModeNoOp* p): NodePriv(p), p(p) {}
        virtual ~TimeRangeModeNoOpPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override {}
        TimeRangeModeNoOp* p = nullptr;
    };

    struct TimeRangeModeOncePriv : public NodePriv {
        TimeRangeModeOncePriv(TimeRangeModeOnce* p): NodePriv(p), p(p) {}
        virtual ~TimeRangeModeOncePriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override {}
        TimeRangeModeOnce* p = nullptr;
    };

    struct AnimatedNodePriv;

    struct TransformPriv : public NodePriv {
        TransformPriv(Transform* p): NodePriv(p), p(p) {}
        virtual ~TransformPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) override {
            child->draw(commandBuffer, graphics, state);
        }
        void update() override;
        virtual void beginTransform();
        virtual void endTransform();
        virtual void initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {}
        virtual void updateAnim() {}
        Transform* p = nullptr;
        sp<NodePriv> child;
        mat4 currentModelViewMat, currentNormalMat;
    };

    struct CameraPriv : public TransformPriv {
        CameraPriv(Camera* p): TransformPriv(p), p0(p){}
        virtual ~CameraPriv() {}
        void beginTransform() override;
        void endTransform() override;
        void initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void updateAnim() override;
        Camera* p0 = nullptr;
        sp<AnimatedNodePriv> fovAnim;
        sp<TransformPriv> eyeTransform, centerTransform, upTransform;
        mat4 currentProjMat;
    };

    struct RotatePriv : public TransformPriv {
        RotatePriv(Rotate* p): TransformPriv(p), p0(p) {}
        virtual ~RotatePriv() {}
        void initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void updateAnim() override;
        Rotate* p0 = nullptr;
        sp<AnimatedNodePriv> anim;
    };

    struct RotateQuatPriv : public TransformPriv {
        RotateQuatPriv(RotateQuat* p): TransformPriv(p), p0(p) {}
        virtual ~RotateQuatPriv() {}
        void initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void updateAnim() override;
        RotateQuat* p0 = nullptr;
        sp<AnimatedNodePriv> anim;
    };

    struct ScalePriv : public TransformPriv {
        ScalePriv(Scale* p): TransformPriv(p), p0(p) {}
        virtual ~ScalePriv() {}
        void initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void updateAnim() override;
        Scale* p0 = nullptr;
        sp<AnimatedNodePriv> anim;
    };

    struct TranslatePriv : public TransformPriv {
        TranslatePriv(Translate* p): TransformPriv(p), p0(p) {}
        virtual ~TranslatePriv() {}
        void initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void updateAnim() override;
        Translate* p0 = nullptr;
        sp<AnimatedNodePriv> anim;
    };

    struct UniformPriv : public NodePriv {
        UniformPriv(Uniform* p) : NodePriv(p), p(p) {}
        virtual ~UniformPriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override {}
        void update() override {}
        Uniform *p = nullptr;
    };

    struct UniformMat4Priv : public UniformPriv {
        UniformMat4Priv(UniformMat4* p) : UniformPriv(p), p0(p) {}
        virtual ~UniformMat4Priv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override;
        void update() override;
        UniformMat4 *p0 = nullptr;
        sp<TransformPriv> transform;
    };

    struct AnimatedNodePriv : public NodePriv {
        AnimatedNodePriv(AnimatedNode* p) : NodePriv(p), p0(p) {}
        virtual ~AnimatedNodePriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override {
            p0->evaluate(p0->ctx->time);
        }
        void update() override {
            p0->evaluate(p0->ctx->time);
        }
        AnimatedNode* p0 = nullptr;
    };

    struct AnimKeyFramePriv : public NodePriv {
        AnimKeyFramePriv(AnimKeyFrame* p) : NodePriv(p), p(p) {}
        virtual ~AnimKeyFramePriv() {}
        void init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) override {}
        void update() override {}
        AnimKeyFrame* p = nullptr;
    };
};
