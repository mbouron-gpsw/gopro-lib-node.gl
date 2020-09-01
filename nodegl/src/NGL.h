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
#include <vector>
#include <map>
#include <cmath>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <memory>
#include "DebugUtil.h"
#include "graphics/GraphicsCore.h" //TODO: add porting/NGLCore.h
#include "DrawUtils.h"
#include <typeindex>
using namespace std;
using namespace glm;

namespace NGL {
template <typename T> using sp = shared_ptr<T>;
#define sp_cast dynamic_pointer_cast

using FilterMode = ngfx::FilterMode;
enum MipmapFilterMode { MIPMAP_FILTER_NONE, MIPMAP_FILTER_NEAREST, MIPMAP_FILTER_LINEAR };
enum SamplerAddressMode { SAMPLER_ADDRESS_MODE_REPEAT, SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE };
using BlendFactor = ngfx::BlendFactor;
using ColorComponentFlags = ngfx::ColorComponentFlags;
using ColorComponentFlagBits = ngfx::ColorComponentFlagBits;
using BlendOp = ngfx::BlendOp;
using PrimitiveTopology  = ngfx::PrimitiveTopology;
using PixelFormat = ngfx::PixelFormat;
enum DataType {
    BYTE, BVEC2, BVEC3, BVEC4,
    INT, INT64, IVEC2, IVEC3, IVEC4,
    SHORT, SVEC2, SVEC3, SVEC4,
    UBYTE, UBVEC2, UBVEC3, UBVEC4,
    UINT, UIVEC2, UIVEC3, UIVEC4,
    USHORT, USVEC2, USVEC3, USVEC4,
    FLOAT, VEC2, VEC3, VEC4,
    MAT4,
    QUAT
};
enum EasingId {
    EASING_LINEAR,
    EASING_QUADRATIC_IN, EASING_QUADRATIC_OUT, EASING_QUADRATIC_IN_OUT, EASING_QUADRATIC_OUT_IN,
    EASING_CUBIC_IN, EASING_CUBIC_OUT, EASING_CUBIC_IN_OUT, EASING_CUBIC_OUT_IN,
    EASING_QUARTIC_IN, EASING_QUARTIC_OUT, EASING_QUARTIC_IN_OUT, EASING_QUARTIC_OUT_IN,
    EASING_QUINTIC_IN, EASING_QUINTIC_OUT, EASING_QUINTIC_IN_OUT, EASING_QUINTIC_OUT_IN,
    EASING_POWER_IN, EASING_POWER_OUT, EASING_POWER_IN_OUT, EASING_POWER_OUT_IN,
    EASING_SINUS_IN, EASING_SINUS_OUT, EASING_SINUS_IN_OUT, EASING_SINUS_OUT_IN,
    EASING_EXP_IN, EASING_EXP_OUT, EASING_EXP_IN_OUT, EASING_EXP_OUT_IN,
    EASING_CIRCULAR_IN, EASING_CIRCULAR_OUT, EASING_CIRCULAR_IN_OUT, EASING_CIRCULAR_OUT_IN,
    EASING_BOUNCE_IN, EASING_BOUNCE_OUT,
    EASING_ELASTIC_IN, EASING_ELASTIC_OUT,
    EASING_BACK_IN, EASING_BACK_OUT, EASING_BACK_IN_OUT, EASING_BACK_OUT_IN,
};

struct NodeBackend {
    virtual ~NodeBackend() {}
};
struct ContextBackend {
    virtual ~ContextBackend() {}
};

class Context;

class Node {
public:
    virtual ~Node() {}
    virtual void serialize(std::ostream &out) {}
    virtual void deserialize(std::istream &in) {}
    virtual void setContext(Context* ctx) {
        this->ctx = ctx;
    }
    virtual sp<NodeBackend> getBackend() { return nullptr; }
    Context* ctx = nullptr;
    string label;
};

class UniformMat4;

struct Config {
    bool offscreen = false;
    int width = 0, height = 0;
    vec4 clearColor = vec4(0.0f);
    uint8_t *captureBuffer = nullptr;
    uintptr_t displayHandle = 0;
    uintptr_t windowHandle  = 0;
};

class NGLApplication;
class GraphicsConfig;

class Context  {
public:
    Context(sp<NGLApplication> app): app(app) { init(); }
    virtual ~Context() {}
    void setConfig(Config config);
    void setScene(sp<Node> scene);
    void draw(double t);
    sp<NGLApplication> app;
    unique_ptr<ContextBackend> priv;
    Config config;
    sp<Node> scene;
    sp<NodeBackend> scenePriv;
    sp<UniformMat4> modelViewMat, projMat, normalMat, texCoord0Mat;
    GraphicsConfig* graphicsConfig = nullptr;
    double time = 0.0;
private:
    void init();
};

class Media : public Node {
public:
    Media() {}
    Media(const string& filename): filename(filename) {}
    virtual ~Media() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    string filename;
    uint32_t width = 0, height = 0;
};

class Text : public Node {
public:
    Text(string str = ""): str(str) {}
    virtual ~Text() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void initCanvas();
    void initCanvasDimensions(int &w, int &h);
    enum HAlign { HALIGN_CENTER, HALIGN_RIGHT, HALIGN_LEFT };
    enum VAlign { VALIGN_CENTER, VALIGN_TOP, VALIGN_BOTTOM };
    string str;
    vec4 fgColor = vec4(1.0f),
         bgColor = vec4(0.0f, 0.0f, 0.0f, 0.8f);
    int padding = 3;
    float fontScale = 1.0f;
    vec3 boxCorner = vec3(-1.0f, -1.0f, 0.0f),
         boxWidthVec = vec3(2.0f, 0.0f, 0.0f),
         boxHeightVec = vec3(0.0f, 2.0f, 0.0f);
    ivec2 aspectRatio = ivec2(1);
    HAlign hAlign = HALIGN_CENTER;
    VAlign vAlign = VALIGN_CENTER;
    sp<Canvas> canvas;
};

class Texture : public Node {
public:
    Texture(sp<Node> dataSrc = nullptr, uint32_t w = 0, uint32_t h = 0, uint32_t d = 1, uint32_t numLayers = 1, string label = "",
            FilterMode minFilter = FilterMode::FILTER_NEAREST, FilterMode magFilter = FilterMode::FILTER_NEAREST,
            SamplerAddressMode wrapS = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SamplerAddressMode wrapT = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                SamplerAddressMode wrapR = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            MipmapFilterMode mipmapFilter = MipmapFilterMode::MIPMAP_FILTER_NONE)
        : dataSrc(dataSrc), w(w), h(h), d(d), numLayers(numLayers), minFilter(minFilter), magFilter(magFilter),
          wrapS(wrapS), wrapT(wrapT), wrapR(wrapR), mipmapFilter(mipmapFilter) {
        this->label = label;
    }
    virtual ~Texture() {}
    sp<NodeBackend> getBackend() override;
    PixelFormat pixelFormat = PixelFormat::PIXELFORMAT_RGBA8_UNORM;
    uint32_t usageFlags = 0;
    sp<Node> dataSrc;
    uint32_t w = 0, h = 0, d = 1, numLayers = 1;
    FilterMode minFilter = FilterMode::FILTER_NEAREST, magFilter = FilterMode::FILTER_NEAREST;
    SamplerAddressMode wrapS = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, wrapT = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        wrapR = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    MipmapFilterMode mipmapFilter = MipmapFilterMode::MIPMAP_FILTER_NONE;
};

class Texture2D : public Texture {
public:
    Texture2D() {}
    Texture2D(sp<NGL::Node> dataSrc, uint32_t w, uint32_t h, string label,
              FilterMode minFilter = FilterMode::FILTER_NEAREST, FilterMode magFilter = FilterMode::FILTER_NEAREST,
              SamplerAddressMode wrapS = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SamplerAddressMode wrapT = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
              MipmapFilterMode mipmapFilter = MIPMAP_FILTER_NONE)
        :Texture(dataSrc, w, h, 1, 1, label, minFilter, magFilter, wrapS, wrapT, SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, mipmapFilter) {}
    virtual ~Texture2D() {}
    void setContext(Context* ctx) override {
        this->ctx = ctx;
        if (dataSrc) dataSrc->setContext(ctx);
    }
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
};

class Texture3D : public Texture {
public:
    Texture3D() {}
    Texture3D(sp<NGL::Node> dataSrc, uint32_t w, uint32_t h, uint32_t d, string label,
              FilterMode minFilter = FilterMode::FILTER_NEAREST, FilterMode magFilter = FilterMode::FILTER_NEAREST,
              SamplerAddressMode wrapS = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SamplerAddressMode wrapT = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
              SamplerAddressMode wrapR = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, MipmapFilterMode mipmapFilter = MIPMAP_FILTER_NONE)
            :Texture(nullptr, w, h, d, 1, label, minFilter, magFilter, wrapS, wrapT, wrapR, mipmapFilter) {}
    virtual ~Texture3D() {}
    void setContext(Context* ctx) override {
        this->ctx = ctx;
        if (dataSrc) dataSrc->setContext(ctx);
    }
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
};

class TextureCube : public Texture {
public:
    TextureCube() { numLayers = 6; }
    TextureCube(sp<NGL::Node> dataSrc, uint32_t dim, string label,
              FilterMode minFilter = FilterMode::FILTER_NEAREST, FilterMode magFilter = FilterMode::FILTER_NEAREST,
              SamplerAddressMode wrapS = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, SamplerAddressMode wrapT = SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
              MipmapFilterMode mipmapFilter = MIPMAP_FILTER_NONE)
            :Texture(dataSrc, dim,dim,1, 6, label, minFilter, magFilter, wrapS, wrapT, SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, mipmapFilter) {}
    virtual ~TextureCube() {}
    void setContext(Context* ctx) override {
        this->ctx = ctx;
        if (dataSrc) dataSrc->setContext(ctx);
    }
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    uint32_t dim() { return w; }
    void setDim(uint32_t dim) { w = dim; h = dim; d = 1; }
};

class Transform;

class Uniform : public Node {
public:
    virtual ~Uniform() {}
    sp<NodeBackend> getBackend() override;
    virtual void* getData() { return data; }
    virtual uint32_t getSize() { return size; }
    void* data = nullptr; uint32_t size = 0;
};

#define DEFINE_UNIFORM_CLASS(t0, t1, v0, ...) \
class Uniform##t0 : public Uniform { \
public: \
    Uniform##t0() { \
        data = &this->value; size = sizeof(t1); \
    } \
    Uniform##t0(t1 value): value(value) { \
        data = &this->value; size = sizeof(t1); \
    } \
    virtual ~Uniform##t0() {} \
    void serialize(std::ostream &out) override; \
    void deserialize(std::istream &in) override; \
    t1 value = v0; \
    __VA_ARGS__ \
}

DEFINE_UNIFORM_CLASS(Float, float, float(0.0f));
DEFINE_UNIFORM_CLASS(Int, int, int(0.0f));
DEFINE_UNIFORM_CLASS(Vec2, glm::vec2, glm::vec2(0.0f));
DEFINE_UNIFORM_CLASS(Vec3, glm::vec3, glm::vec3(0.0f));
DEFINE_UNIFORM_CLASS(Vec4, glm::vec4, glm::vec4(0.0f));
DEFINE_UNIFORM_CLASS(Mat4, glm::mat4, glm::mat4(1.0f),
    void* getData() override;
    sp<NodeBackend> getBackend() override;
    void setContext(Context* ctx) override;
    sp<Transform> transform;
);
DEFINE_UNIFORM_CLASS(Quat, glm::quat, glm::quat(1.0f, vec3(0.0f)),
    void* getData() override;
    uint32_t getSize() override {
        if (asMat4) return sizeof(valueMat4);
        else return size;
    }
    glm::mat4 valueMat4 = glm::mat4(1.0f);
    bool asMat4 = false;
);

typedef double easingType;
typedef easingType (*EasingFunction)(easingType, int, const easingType *);

class AnimKeyFrame : public Node {
public:
    virtual ~AnimKeyFrame() {}
    virtual void initOnce();
    sp<NodeBackend> getBackend() override;
    EasingId easingId = EASING_LINEAR;
    double easingOffsets[2] = { 0.0, 1.0 };
    int scaleBoundaries = 0;
    double boundaries[2] = { 0.0, 0.0 };
    std::vector<double> easingArgs;
    EasingFunction easingFunction;
    EasingFunction easingResolution;
protected:
    bool isInitialized = false;
};

class AnimatedNode : public Node {
public:
    virtual ~AnimatedNode() {}
    virtual void evaluate(float t) {}
    sp<NodeBackend> getBackend() override;
    virtual void* getData() { return data; }
    virtual uint32_t getSize() { return size; }
    void* data = nullptr; uint32_t size = 0;
};

struct Data {
    Data() {}
    Data(uint32_t size): size(size) { v = malloc(size); }
    ~Data() { if (v) free(v); }
    void* v = nullptr; uint32_t size = 0;
};

#define DEFINE_ANIMATED_KEYFRAME_CLASS(t0, t1, v0, d0, s0, ...) \
class AnimKeyFrame##t0 : public AnimKeyFrame { \
public: \
    AnimKeyFrame##t0() {} \
    AnimKeyFrame##t0(float t, t1 v): t(t), v(v) {} \
    virtual ~AnimKeyFrame##t0() {} \
    void serialize(std::ostream &out) override; \
    void deserialize(std::istream &in) override; \
    float t = 0.0f; t1 v = v0; \
}
#define DEFINE_ANIMATED_NODE_CLASS(t0, kf_t0, t1, v0, d0, s0, ...) \
class Animated##t0 : public AnimatedNode { \
public: \
    Animated##t0() { data = d0; size = s0; } \
    Animated##t0(const vector<sp<AnimKeyFrame##kf_t0>>& kf): kf(kf) { \
        data = d0; size = s0; \
    } \
    virtual ~Animated##t0() {} \
    void serialize(std::ostream &out) override; \
    void deserialize(std::istream &in) override; \
    void evaluate(float t) override; \
    vector<sp<AnimKeyFrame##kf_t0>> kf; \
    t1 value = v0; \
    __VA_ARGS__ \
}

#define DEFINE_ANIMATED_CLASS(t0, t1, v0, d0, s0, ...) \
    DEFINE_ANIMATED_KEYFRAME_CLASS(t0, t1, v0, d0, s0, __VA_ARGS__); \
    DEFINE_ANIMATED_NODE_CLASS(t0, t0, t1, v0, d0, s0, __VA_ARGS__)

DEFINE_ANIMATED_CLASS(Buffer, sp<Data>, nullptr, nullptr, 0,
    AnimatedBuffer(DataType dataType);
    void* getData() override {
        return value->v;
    }
    uint32_t getSize() override {
        return value->size;
    }
    DataType dataType;
    uint32_t stride = 0;);
DEFINE_ANIMATED_CLASS(Int, int, 0, &value, sizeof(int));
DEFINE_ANIMATED_CLASS(Float, float, 0.0f, &value, sizeof(float));
DEFINE_ANIMATED_NODE_CLASS(Time, Float, float, 0.0f, &value, sizeof(float));
DEFINE_ANIMATED_CLASS(Vec2, vec2, vec2(0.0f), &value, sizeof(vec2));
DEFINE_ANIMATED_CLASS(Vec3, vec3, vec3(0.0f), &value, sizeof(vec3));
DEFINE_ANIMATED_CLASS(Vec4, vec4, vec4(0.0f), &value, sizeof(vec4));
DEFINE_ANIMATED_CLASS(Mat4, mat4, mat4(1.0f), &value, sizeof(mat4));
DEFINE_ANIMATED_CLASS(Quat, quat, quat(1.0f, vec3(0.0f)), &value, sizeof(quat),
    void* getData() override;
    uint32_t getSize() override {
      if (asMat4) return sizeof(valueMat4);
      else return size;
    }
    glm::mat4 valueMat4 = glm::mat4(1.0f);
    bool asMat4 = false;
);

class Block;

class Buffer : public Node {
public:
    Buffer() {}
    Buffer(DataType dataType, uint32_t stride = 0, sp<Data> data = nullptr)
        : dataType(dataType), stride(stride), data(data) {}
    virtual ~Buffer() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    Data* getData();
    DataType dataType;
    uint32_t stride = 0;
    sp<Data> data;
    uint32_t count = 1;
    sp<Block> block;
    std::string blockField;
};

class StreamedBuffer : public Node {
public:
    StreamedBuffer() {}
    StreamedBuffer(DataType dataType, uint32_t stride = 0, uint32_t count = 0, sp<Buffer> timestamps = nullptr, sp<Buffer> buffer = nullptr)
        : dataType(dataType), stride(stride), count(count), timestamps(timestamps), buffer(buffer) {}
    virtual ~StreamedBuffer() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void setContext(Context* ctx) override {
        this->ctx = ctx;
        if (timeAnim) timeAnim->setContext(ctx);
    }
    uint32_t getOffset() {
        int64_t* timestampsData = (int64_t*)timestamps->data->v;
        uint32_t numTimestamps = timestamps->data->size / sizeof(int64_t);
        double time = timeAnim ? timeAnim->value : ctx->time;
        uint32_t offset = 0;
        for (uint32_t j = 0; j<numTimestamps; j++) {
            double timestamp = timestampsData[j] * timeBase;
            if (timestamp > time) break;
            offset = j;
        }
        return offset * stride * count;
    }
    DataType dataType;
    uint32_t stride = 0;
    uint32_t count = 0;
    sp<Buffer> timestamps;
    sp<Buffer> buffer;
    double timeBase = 1.0 / 1000000.0;
    sp<AnimatedTime> timeAnim;
};

class Geometry : public Node {
public:
    Geometry(sp<Node> verts = nullptr): verts(verts) {}
    virtual ~Geometry() {}
    sp<NodeBackend> getBackend() override;
    void setContext(Context* ctx) override {
        if (verts) verts->setContext(ctx);
        if (normals) normals->setContext(ctx);
        if (uvCoords) uvCoords->setContext(ctx);
        if (indices) indices->setContext(ctx);
    }
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    sp<Node> verts, normals, uvCoords, indices;
    PrimitiveTopology topology = PrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
};

class Triangle : public Geometry {
public:
    Triangle(vec3 edge0 = vec3(0.0f), vec3 edge1 = vec3(0.0f), vec3 edge2 = vec3(0.0f),
             vec2 uvEdge0 = vec2(0.0f, 0.0f), vec2 uvEdge1 = vec2(0.0f, 1.0f), vec2 uvEdge2 = vec2(1.0f, 1.0f))
            :edges({ edge0, edge1, edge2 }), uvEdges({ uvEdge0, uvEdge1, uvEdge2 }) {
    }
    virtual ~Triangle() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    std::vector<vec3> edges;
    std::vector<vec2> uvEdges;
};

class Quad : public Geometry {
public:
    Quad(vec3 corner = { -0.5f, -0.5f, 0.0f }, vec3 width = { 1.0f, 0.0f, 0.0f }, vec3 height = { 0.0f, 1.0f, 0.0f },
            vec2 uv_corner = { 0.0f, 0.0f }, vec2 uv_width = { 1.0f, 0.0f }, vec2 uv_height = { 0.0f, 1.0f })
            :corner(corner), width(width), height(height), uv_corner(uv_corner), uv_width(uv_width), uv_height(uv_height) {
    }
    virtual ~Quad() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    vec3 corner, width, height;
    vec2 uv_corner, uv_width, uv_height;
};

class Circle : public Geometry {
public:
    Circle(float radius = 1.0f, uint32_t numPoints = 16)
            :radius(radius), numPoints(numPoints) {
    }
    virtual ~Circle() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    float radius;
    uint32_t numPoints;
};

class Program : public Node {
public:
    virtual ~Program() {}

};
class GraphicsProgram : public Program {
public:
    GraphicsProgram() {}
    GraphicsProgram(const string& vertexShaderFile, const string& fragmentShaderFile)
            :GraphicsProgram() {
        this->vertexShaderFile = vertexShaderFile;
        this->fragmentShaderFile = fragmentShaderFile;
    }
    virtual ~GraphicsProgram() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    string vertexShaderFile, fragmentShaderFile;
};
class ComputeProgram : public Program {
public:
    ComputeProgram() {}
    ComputeProgram(const string& computeShaderFile)
            :ComputeProgram() {
        this->computeShaderFile = computeShaderFile;
    }
    virtual ~ComputeProgram() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    string computeShaderFile;
};

class Block : public Node {
public:
    Block(string layout = ""): layout(layout) {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void setContext(Context* ctx) override {
        for (auto& it: fields) it.second->setContext(ctx);
    }
    void updateFields(const map<string, sp<Node>>& fields) {
        for (auto& it: fields) {
            this->fields[it.first] = it.second;
        }
    }
    map<string, sp<Node>> fields;
    string layout;
};

class Render : public Node {
public:
    Render() {}
    Render(sp<Geometry> geom, sp<GraphicsProgram> program = nullptr)
            :geom(geom), program(program) {
    }
    virtual ~Render() {}
    void updateTextures(const map<string, sp<Texture>>& textures) {
        for (auto& it : textures) {
            this->textures[it.first] = it.second;
        }
	}
    void updateUniforms(const map<string, sp<Node>>& uniforms) {
        for (auto& it: uniforms) {
            this->uniforms[it.first] = it.second;
		}
	}
    void updateBlocks(const map<string, sp<Block>>& blocks) {
        for (auto& it: blocks) {
            this->blocks[it.first] = it.second;
        }
    }
    void updateInstanceAttributes(const map<string, sp<Buffer>>& attrs) {
        for (auto& it: attrs) {
            this->instanceAttributes[it.first] = it.second;
        }
    }
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void setContext(Context* ctx) override;
    sp<Geometry> geom;
    sp<GraphicsProgram> program;
    map<string, sp<Texture>> textures;
    const std::string
        MODELVIEW_MATRIX_ID = "ngl_modelview_matrix",
        PROJECTION_MATRIX_ID = "ngl_projection_matrix",
        NORMAL_MATRIX_ID = "ngl_normal_matrix",
        TEXCOORD_0_MATRIX_ID = "ngl_tex0_coord_matrix";
    map<string, sp<Node>> uniforms;
    map<string, sp<Block>> blocks;
    map<string, sp<Buffer>> instanceAttributes;
    uint32_t numInstances = 1;
};

class Compute : public Node {
public:
    Compute() {}
    Compute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
            uint32_t threadsPerGroupX, uint32_t threadsPerGroupY, uint32_t threadsPerGroupZ,
            sp<ComputeProgram> program)
        :groupCountX(groupCountX), groupCountY(groupCountY), groupCountZ(groupCountZ),
         threadsPerGroupX(threadsPerGroupX), threadsPerGroupY(threadsPerGroupY), threadsPerGroupZ(threadsPerGroupZ),
         program(program) {}
    virtual ~Compute() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void setContext(Context* ctx) override;
    void updateUniforms(const map<string, sp<Node>>& uniforms) {
        for (auto& it: uniforms) {
            this->uniforms[it.first] = it.second;
        }
    }
    void updateBlocks(const map<string, sp<Block>>& blocks) {
        for (auto& it: blocks) {
            this->blocks[it.first] = it.second;
        }
    }
    void updateTextures(const map<string, sp<Texture>>& textures) {
        for (auto& it: textures) {
            this->textures[it.first] = it.second;
            it.second->usageFlags |= ngfx::IMAGE_USAGE_STORAGE_BIT;
        }
    }
    map<string, sp<Node>> uniforms;
    map<string, sp<Block>> blocks;
    map<string, sp<Texture>> textures;
    uint32_t groupCountX = 0, groupCountY = 0, groupCountZ = 0,
        threadsPerGroupX = 0, threadsPerGroupY = 0, threadsPerGroupZ = 0;
    sp<ComputeProgram> program;
};

class RenderToTexture : public Node {
public:
    RenderToTexture() {}
    RenderToTexture(sp<Node> child, const vector<sp<Texture>>& outputTextures = {})
            :child(child), outputTextures(outputTextures) {
    }
    virtual ~RenderToTexture() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void setContext(Context* ctx) override {
        this->ctx = ctx;
        if (child) child->setContext(ctx);
        for (auto texture: outputTextures) texture->setContext(ctx);
    }
    sp<Node> child;
    vector<sp<Texture>> outputTextures;
    sp<Texture2D> depthTexture;
    bool setClearColor = false;
    vec4 clearColor;
    uint32_t numSamples = 1;
    enum Flags { DEPTH = 1, STENCIL = 2, NO_CLEAR = 4 };
    uint32_t flags = 0;
};

class Group : public Node {
public:
    Group() {}
    virtual ~Group() {}
    void addChildren(const vector<sp<Node>>& children) {
        for (auto& child: children) this->children.push_back(child);
    }
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void setContext(Context *ctx) override {
        this->ctx = ctx;
        for (auto& child: children) child->setContext(ctx);
    }
    vector<sp<Node>> children;
};

class GraphicsConfig : public Node {
public:
    GraphicsConfig() {}
    GraphicsConfig(sp<Node> child): child(child) {}
    virtual ~GraphicsConfig() {}
    void setChild(sp<Node> child) {
        this->child = child;
    }
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void setContext(Context *ctx) override {
        this->ctx = ctx;
        child->setContext(ctx);
    }
    sp<Node> child;
    bool blend = false;
    bool scissorTest = false;
    ngfx::Rect2D scissorRect;
    bool depthTest = false;
    BlendFactor blendSrcFactor = BlendFactor::BLEND_FACTOR_ONE;
    BlendFactor blendDstFactor = BlendFactor::BLEND_FACTOR_ZERO;
    BlendOp blendOp = BlendOp::BLEND_OP_ADD;
    ColorComponentFlags colorWriteMask =
        ColorComponentFlagBits::COLOR_COMPONENT_R_BIT |
        ColorComponentFlagBits::COLOR_COMPONENT_G_BIT |
        ColorComponentFlagBits::COLOR_COMPONENT_B_BIT |
        ColorComponentFlagBits::COLOR_COMPONENT_A_BIT;
};

class Transform : public Node {
public:
    Transform() {}
    Transform(sp<Node> child): child(child) {}
    virtual ~Transform() {}
    virtual void setChild(sp<Node> child) {
        this->child = child;
    }
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void setContext(Context *ctx) override {
        this->ctx = ctx;
        child->setContext(ctx);
    }
    virtual mat4 getMat() {
        if (needsUpdate) {
            needsUpdate = false;
        }
        return mat;
    }
    mat4 getChildMat() {
        if (auto trChild = sp_cast<Transform>(child)) {
            return trChild->getMat() * trChild->getChildMat();
        }
        return mat4(1.0f);
    }
    void set(mat4 mat) {
        this->mat = mat;
        needsUpdate = true;
    }
    sp<Node> child;
    mat4 mat;
    bool needsUpdate = true;
};

class Camera : public Transform {
public:
    Camera(sp<Node> child = nullptr): Transform(child) {}
    virtual ~Camera() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    mat4 getMat() override;
    void setContext(Context *ctx) override {
        Transform::setContext(ctx);
        if (fovAnim) fovAnim->setContext(ctx);
        if (eyeTransform) eyeTransform->setContext(ctx);
        if (centerTransform) centerTransform->setContext(ctx);
        if (upTransform) upTransform->setContext(ctx);
    }
    void setEye(vec3 eye) { this->eye = eye; needsUpdate = true; }
    void setCenter(vec3 center) { this->center = center; needsUpdate = true; }
    void setUp(vec3 up) { this->up = up; needsUpdate = true; }
    void setFov(float fov) { this->perspectiveParams.fov = fov; needsUpdate = true; }
    struct PerspectiveProjectionParams { float fov, aspect; };
    struct OrthographicProjectionParams { float left, right, bottom, top; };
    struct ClippingParams { float pNear, pFar; };
    void setPerspectiveProjectionParams(PerspectiveProjectionParams params) {
        perspectiveProj = true;
        perspectiveParams = params;
    }
    void setOrthographicProjectionParams(OrthographicProjectionParams params) {
        perspectiveProj = false;
        orthographicParams = params;
    }
    vec3 eye = vec3(0.0f),
         center = vec3(0.0f, 0.0f, -1.0f),
         up = vec3(0.0f, 1.0f, 0.0f);
    bool perspectiveProj = false;
    PerspectiveProjectionParams perspectiveParams = {};
    OrthographicProjectionParams orthographicParams = {};
    ClippingParams clippingParams = {};
    sp<AnimatedFloat> fovAnim;
    sp<Transform> eyeTransform, centerTransform, upTransform;
    mat4 projMat;
};

class Identity : public Node {
public:
    Identity() {}
    virtual ~Identity() {}
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
};

class Rotate : public Transform {
public:
    Rotate() {}
    Rotate(sp<Node> child): Transform(child) {}
    virtual ~Rotate() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    mat4 getMat() override;
    void setAxis(vec3 axis) { this->axis = axis; needsUpdate = true; }
    void setAnchor(vec3 anchor) { this->anchor = anchor; needsUpdate = true; }
    void set(float angle) { this->angle = angle; needsUpdate = true; }
    void setAnim(sp<AnimatedFloat> anim) { this->anim = anim; needsUpdate = true; }
    void setContext(Context *ctx) override {
        Transform::setContext(ctx);
        if (anim) anim->setContext(ctx);
    }
    vec3 axis = vec3(0.0f, 0.0f, 1.0f);
    vec3 anchor = vec3(0.0f);
    float angle = 0.0f;
    sp<AnimatedFloat> anim;
};

class RotateQuat : public Transform {
public:
    RotateQuat() {}
    RotateQuat(sp<Node> child): Transform(child) {}
    virtual ~RotateQuat() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    mat4 getMat() override;
    void set(quat v) { this->v = v; needsUpdate = true; }
    void setAnchor(vec3 anchor) { this->anchor = anchor; needsUpdate = true; }
    void setAnim(sp<AnimatedQuat> anim) { this->anim = anim; needsUpdate = true; }
    void setContext(Context *ctx) override {
        Transform::setContext(ctx);
        if (anim) anim->setContext(ctx);
    }
    quat v = quat(0.0f, vec3(0.0f));
    vec3 anchor = vec3(0.0f);
    sp<AnimatedQuat> anim;
};

class Scale : public Transform {
public:
    Scale() {}
    Scale(sp<Node> child): Transform(child) {}
    virtual ~Scale() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    mat4 getMat() override;
    void set(vec3 v) { this->v = v; needsUpdate = true; }
    void setAnim(sp<AnimatedVec3> anim) { this->anim = anim; needsUpdate = true; }
    void setAnchor(vec3 anchor) { this->anchor = anchor; needsUpdate = true; }
    void setContext(Context *ctx) override {
        Transform::setContext(ctx);
        if (anim) anim->setContext(ctx);
    }
    vec3 v = vec3(1.0f), anchor = vec3(0.0f);
    sp<AnimatedVec3> anim;
};

class Translate : public Transform {
public:
    Translate() {}
    Translate(sp<Node> child): Transform(child) {}
    virtual ~Translate() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    mat4 getMat() override;
    void set(vec3 v) { this->v = v; needsUpdate = true; }
    void setAnim(sp<AnimatedVec3> anim) { this->anim = anim; needsUpdate = true; }
    void setContext(Context *ctx) override {
        Transform::setContext(ctx);
        if (anim) anim->setContext(ctx);
    }
    vec3 v = vec3(0.0f);
    sp<AnimatedVec3> anim;
};

class TimeRangeMode : public Node {
public:
    TimeRangeMode() {}
    TimeRangeMode(double startTime): startTime(startTime) {}
    virtual ~TimeRangeMode() {}
    double startTime = 0.0;
};

class TimeRangeModeCont : public TimeRangeMode {
public:
    TimeRangeModeCont() {}
    TimeRangeModeCont(double startTime): TimeRangeMode(startTime) {}
    virtual ~TimeRangeModeCont() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
};

class TimeRangeModeNoOp : public TimeRangeMode {
public:
    TimeRangeModeNoOp() {}
    TimeRangeModeNoOp(double startTime): TimeRangeMode(startTime) {}
    virtual ~TimeRangeModeNoOp() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
};

class TimeRangeModeOnce : public TimeRangeMode {
public:
    TimeRangeModeOnce() {}
    TimeRangeModeOnce(double startTime, double renderTime)
        : TimeRangeMode(startTime), renderTime(renderTime) {}
    virtual ~TimeRangeModeOnce() {}
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    double renderTime = 0.0;
};

class TimeRangeFilter : public Node {
public:
    TimeRangeFilter() {}
    TimeRangeFilter(sp<Node> child): child(child) {}
    virtual ~TimeRangeFilter() {}
    void setChild(sp<Node> child) {
        this->child = child;
    }
    sp<NodeBackend> getBackend() override;
    void serialize(std::ostream &out) override;
    void deserialize(std::istream &in) override;
    void setContext(Context *ctx) override {
        this->ctx = ctx;
        child->setContext(ctx);
    }
    sp<TimeRangeMode> findRange(double t) {
        sp<TimeRangeMode> r;
        for (auto& range: ranges) {
            if (range->startTime > t) continue;
            r = range;
        }
        return r;
    }
    sp<Node> child;
    vector<sp<TimeRangeMode>> ranges;
};

class Deserialize {
public:
    Deserialize(const std::string& filename)
        :filename(filename) {}
    sp<Node> run();
private:
    std::string filename;
};
class Serialize {
public:
    Serialize(sp<Node> scene, const std::string& filename)
        :scene(scene), filename(filename) {}
	void run();
private:
    sp<Node> scene;
    std::string filename;
};

struct AnimationUtil {
    static int evaluateEasing(EasingId easingId, double* args, int nb_args, double* offsets, double t, double* v);
    static int solveEasing(EasingId easingId, double *args, int nb_args,
                         double *offsets, double v, double *t);
};
}

