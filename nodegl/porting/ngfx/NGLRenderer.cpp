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
#define _USE_MATH_DEFINES 
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/normal.hpp>
#include "graphics/BufferUtil.h"
#include "NGLBackend.h"
#include "NGLApplication.h"
#include "DebugUtil.h"
using namespace NGL;

static std::map<Node*, sp<NodeBackend>> backendCache;

template <typename T0, typename T1>
static sp<NodeBackend> getBackendFromCache(T0* thiz) {
    auto it = backendCache.find(thiz);
    if (it != backendCache.end()) return it->second;
    auto ptr = make_shared<T1>(thiz);
    backendCache[thiz] = ptr;
    return ptr;
}

#define DEFINE_GET_BACKEND(t0) \
sp<NodeBackend> t0::getBackend() { return make_shared<t0##Priv>(this); }
#define DEFINE_GET_BACKEND_WITH_CACHE(t0) \
sp<NodeBackend> t0::getBackend() { return getBackendFromCache<t0, t0##Priv>(this); }

DEFINE_GET_BACKEND(Render)
DEFINE_GET_BACKEND(Compute)
DEFINE_GET_BACKEND(Rotate) DEFINE_GET_BACKEND(RotateQuat)
DEFINE_GET_BACKEND(Scale) DEFINE_GET_BACKEND(Translate)
DEFINE_GET_BACKEND(Transform) DEFINE_GET_BACKEND(Camera)
DEFINE_GET_BACKEND(Group)
DEFINE_GET_BACKEND(GraphicsConfig)
DEFINE_GET_BACKEND(GraphicsProgram)
DEFINE_GET_BACKEND(ComputeProgram)

DEFINE_GET_BACKEND_WITH_CACHE(Text)
DEFINE_GET_BACKEND_WITH_CACHE(NGL::Buffer)
DEFINE_GET_BACKEND_WITH_CACHE(StreamedBuffer)
DEFINE_GET_BACKEND_WITH_CACHE(Geometry) DEFINE_GET_BACKEND_WITH_CACHE(Triangle)
DEFINE_GET_BACKEND_WITH_CACHE(Quad) DEFINE_GET_BACKEND_WITH_CACHE(Circle)

DEFINE_GET_BACKEND_WITH_CACHE(TimeRangeFilter)
DEFINE_GET_BACKEND_WITH_CACHE(TimeRangeModeCont)
DEFINE_GET_BACKEND_WITH_CACHE(TimeRangeModeNoOp)
DEFINE_GET_BACKEND_WITH_CACHE(TimeRangeModeOnce)

DEFINE_GET_BACKEND_WITH_CACHE(RenderToTexture)
DEFINE_GET_BACKEND_WITH_CACHE(Media)
DEFINE_GET_BACKEND_WITH_CACHE(NGL::Texture) DEFINE_GET_BACKEND_WITH_CACHE(Texture2D)
DEFINE_GET_BACKEND_WITH_CACHE(Texture3D) DEFINE_GET_BACKEND_WITH_CACHE(TextureCube)
DEFINE_GET_BACKEND_WITH_CACHE(Uniform)
DEFINE_GET_BACKEND_WITH_CACHE(UniformMat4)
DEFINE_GET_BACKEND_WITH_CACHE(Block)
DEFINE_GET_BACKEND_WITH_CACHE(AnimatedNode) DEFINE_GET_BACKEND_WITH_CACHE(AnimKeyFrame)

void UniformMat4Priv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    if (p0->transform) {
        transform = sp_cast<TransformPriv>(p0->transform->getBackend());
        transform->init(ctx, graphics, state);
    }
}

void UniformMat4Priv::update() {
    if (transform) transform->update();
}

static sp<ngfx::Buffer> getBufferRef(sp<Node> node) {
    if (sp<NGL::Buffer> buffer = sp_cast<NGL::Buffer>(node)) {
        if (buffer->block) {
            auto blockPriv = sp_cast<BlockPriv>(buffer->block->getBackend());
            return blockPriv->buffer;
        }
    }
    return nullptr;
}

static Data* getData(sp<Node> node) {
    if (sp<NGL::Buffer> buffer = sp_cast<NGL::Buffer>(node))
        return buffer->getData();
    else if (sp<AnimatedBuffer> buffer = sp_cast<AnimatedBuffer>(node)) {
        return buffer->value.get();
    }
    return nullptr;
}

static uint32_t getStride(sp<Node> node) {
    if (sp<NGL::Buffer> buffer = sp_cast<NGL::Buffer>(node))
        return buffer->stride;
    else if (sp<AnimatedBuffer> buffer = sp_cast<AnimatedBuffer>(node))
        return buffer->stride;
    return 0;
}

void GeometryPriv::init(GraphicsContext *ctx, Graphics* graphics, GraphicsState state) {
    if (!p->verts) ERR("verts is NULL");
    verts = sp_cast<NodePriv>(p->verts->getBackend());
    verts->init(ctx, graphics, state);
    if (p->normals) {
        normals = sp_cast<NodePriv>(p->normals->getBackend());
        normals->init(ctx, graphics, state);
    }
    if (p->uvCoords) {
        uvCoords = sp_cast<NodePriv>(p->uvCoords->getBackend());
        uvCoords->init(ctx, graphics, state);
    }
    if (p->indices) {
        indices = sp_cast<NodePriv>(p->indices->getBackend());
        indices->init(ctx, graphics, state);
    }

    auto vertsData = getData(p->verts);
    numVerts = vertsData->size / sizeof(vec3);
    auto bPosRef = getBufferRef(p->verts);
    if (bPosRef) bPos = bPosRef;
    else bPos.reset(createVertexBuffer(ctx, vertsData->v, vertsData->size, sizeof(vec3)));

    if (p->uvCoords) {
        auto uvCoordsData = getData(p->uvCoords);
        bTexCoord.reset(createVertexBuffer(ctx, uvCoordsData->v, uvCoordsData->size, sizeof(vec2)));
    }
    if (p->normals) {
        auto normalsData = getData(p->normals);
        bNormal.reset(createVertexBuffer(ctx, normalsData->v, normalsData->size, sizeof(vec3)));
    }
    if (p->indices) {
        auto indicesData = getData(p->indices);
        bIndices.reset(createIndexBuffer(ctx, indicesData->v, indicesData->size, getStride(p->indices)));
        numIndices = getData(p->indices)->size / getStride(p->indices);
    }
}

void GeometryPriv::update() {
    if (verts) verts->update();
    if (normals) normals->update();
    if (uvCoords) uvCoords->update();
    if (indices) indices->update();
    if (auto buffer = sp_cast<AnimatedBuffer>(p->verts)) {
        auto vertsData = buffer->value.get();
        bPos->upload(vertsData->v, vertsData->size);
    }
    if (auto buffer = sp_cast<AnimatedBuffer>(p->uvCoords)) {
        auto uvCoordsData = buffer->value.get();
        bTexCoord->upload(uvCoordsData->v, uvCoordsData->size);
    }
    if (auto buffer = sp_cast<AnimatedBuffer>(p->normals)) {
        auto normalsData = buffer->value.get();
        bNormal->upload(normalsData->v, normalsData->size);
    }
    if (auto buffer = sp_cast<AnimatedBuffer>(p->indices)) {
        auto indicesData = buffer->value.get();
        bIndices->upload(indicesData->v, indicesData->size);
    }
}

void TrianglePriv::init(GraphicsContext *ctx, Graphics* graphics, GraphicsState state) {
    vector<vec3> &pos = p0->edges;
    vector<vec2> &texCoord = p0->uvEdges;
    vec3 normal = triangleNormal(pos[0], pos[1], pos[2]);
    vector<vec3> normals(pos.size(), normal);
    for (auto& tc : texCoord) tc.y = 1.0f - tc.y;
    numVerts = uint32_t(pos.size());
    bPos.reset(createVertexBuffer(ctx, pos));
    bTexCoord.reset(createVertexBuffer(ctx, texCoord));
    bNormal.reset(createVertexBuffer(ctx, normals));
    p->topology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
}

void QuadPriv::init(GraphicsContext *ctx, Graphics* graphics, GraphicsState state) {
    vector<vec3> pos = { p0->corner + p0->height, p0->corner, p0->corner + p0->width + p0->height, p0->corner +p0->width };
    vector<vec2> texCoord = { p0->uv_corner + p0->uv_height, p0->uv_corner, p0->uv_corner + p0->uv_width + p0->uv_height, p0->uv_corner +p0->uv_width };
    vec3 normal = triangleNormal(pos[0], pos[1], pos[2]);
    vector<vec3> normals(pos.size(), normal);
    for (auto& tc : texCoord) tc.y = 1.0f - tc.y;
    numVerts = uint32_t(pos.size());
    bPos.reset(createVertexBuffer(ctx, pos));
    bTexCoord.reset(createVertexBuffer(ctx, texCoord));
    bNormal.reset(createVertexBuffer(ctx, normals));
    p->topology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
}

void CirclePriv::init(GraphicsContext *ctx, Graphics* graphics, GraphicsState state) {
    //TODO: use indexing
    numVerts = (p0->numPoints + 1) * 2;
    vector<vec3> pos(numVerts);
    vector<vec3> normals(numVerts);
    vector<vec2> texCoords(numVerts);

    float step = 2.0f * M_PI / float(p0->numPoints);
    float r = p0->radius;
    vec3 center(0.0f);
    auto posIter = pos.begin();
    auto texCoordsIter = texCoords.begin();
    for (uint32_t j = 0; j < (p0->numPoints + 1); j++) {
        float angle = j * -step;
        float x = r * sin(angle);
        float y = r * cos(angle);
        *posIter++ = vec3(x, y, 0.0f);
        *texCoordsIter++ = vec2((x + 1.0f) / 2.0f, (1.0f - y) / 2.0f);
        *posIter++ = center;
        *texCoordsIter++ = vec2(0.5f, 0.5f);
    }

    vec3 normal = triangleNormal(center, pos[0], pos[2]);
    for (auto& n : normals) {
        n = normal;
    }
    bPos.reset(createVertexBuffer(ctx, pos));
    bNormal.reset(createVertexBuffer(ctx, normals));
    bTexCoord.reset(createVertexBuffer(ctx, texCoords));
    p->topology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
}

void TextPriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    this->ctx = ctx;
    this->rtt = state.rttPass;
    p->initCanvas();
    vec3& c = p->boxCorner, &w = p->boxWidthVec, &h = p->boxHeightVec;
    vector<vec3> pos = { c + h, c, c + w + h, c + w };
    vector<vec2> texCoord = { vec2(0.0f, 1.0f), vec2(0.0f), vec2(1.0f), vec2(1.0f, 0.0f) };
    for (auto& tc : texCoord) tc.y = 1.0f - tc.y;
    numVerts = uint32_t(pos.size());
    bPos.reset(createVertexBuffer(ctx, pos));
    bTexCoord.reset(createVertexBuffer(ctx, texCoord));
    topology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    if (state.rttPass) createPipeline(ctx->defaultOffscreenRenderPass, state.colorFormat, state.depthFormat);
    else createPipeline(ctx->defaultRenderPass, state.colorFormat, state.depthFormat);
    graphicsPipeline->getBindings({ &U_UBO, &U_TEXTURE }, { &B_POS, &B_TEXCOORD } );
    texture.reset(ngfx::Texture::create(ctx, graphics, p->canvas->data, PIXELFORMAT_RGBA8_UNORM,
        p->canvas->w * p->canvas->h * 4, p->canvas->w, p->canvas->h, 1, 1));

    UBO_0_Data uboData = { p->ctx->modelViewMat->value, p->ctx->projMat->value };
    ubo.reset(createUniformBuffer(ctx, &uboData, sizeof(uboData)));
}

void TextPriv::createPipeline(RenderPass* renderPass, PixelFormat colorFormat, PixelFormat depthFormat) {
    const std::string key = "NGL::drawText";
    graphicsPipeline = (GraphicsPipeline*)ctx->pipelineCache->get(key);
    if (graphicsPipeline) return;
    GraphicsPipeline::State state;
    state.renderPass = renderPass;
    state.primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    auto device = ctx->device;
    graphicsPipeline = GraphicsPipeline::create(ctx, state,
        VertexShaderModule::create(device,   "drawText.vert").get(),
        FragmentShaderModule::create(device, "drawText.frag").get(),
        colorFormat, depthFormat
    );
    ctx->pipelineCache->add(key, graphicsPipeline);
}

void TextPriv::draw(CommandBuffer *commandBuffer, Graphics *graphics, GraphicsState state) {
    if (state.rttPass != rtt) return;
    graphics->bindGraphicsPipeline(commandBuffer, graphicsPipeline);
    graphics->bindVertexBuffer(commandBuffer, bPos.get(), B_POS);
    graphics->bindVertexBuffer(commandBuffer, bTexCoord.get(), B_TEXCOORD);
    graphics->bindTexture(commandBuffer, texture.get(), U_TEXTURE);
    graphics->bindUniformBuffer(commandBuffer, ubo.get(), U_UBO, SHADER_STAGE_VERTEX_BIT);
    graphics->draw(commandBuffer, numVerts, 1);
}

void TextPriv::update() {

}

void BlockPriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    for (auto& it : p->fields) {
         auto field = sp_cast<NodePriv>(it.second->getBackend());
         field->init(ctx, graphics, state);
         fields[it.first] = field;
    }
}

void BlockPriv::update() {
    for (auto& it : fields) {
        it.second->update();
    }
}

static map<DataType, PixelFormat> pixelFormatMap = {
    { DataType::UBYTE, PIXELFORMAT_R8_UNORM },
    { DataType::UBVEC2, PIXELFORMAT_RG8_UNORM },
    { DataType::UBVEC4, PIXELFORMAT_RGBA8_UNORM },
    { DataType::UINT, PIXELFORMAT_R32_UINT },
    { DataType::UIVEC2, PIXELFORMAT_RG32_UINT },
    { DataType::UIVEC4, PIXELFORMAT_RGBA32_UINT },
    { DataType::FLOAT, PIXELFORMAT_R32_SFLOAT },
    { DataType::VEC2, PIXELFORMAT_RG32_SFLOAT },
    { DataType::VEC4, PIXELFORMAT_RGBA32_SFLOAT }
};

template <typename T0, typename T1, typename T2> static void convertTextureData(
        T0* srcData, uint32_t count, T2 alpha, vector<T1> &dstData) {
    dstData.resize(count);
    for (uint32_t j = 0; j<count; j++) {
        dstData[j] = T1(srcData[j], alpha);
    }
}

static ngfx::Texture* createTexture(GraphicsContext* ctx, Graphics* graphics, void* data, uint32_t bpp,
        uint32_t w, uint32_t h, uint32_t d, uint32_t numLayers, DataType dataType,
        ImageUsageFlags imageUsageFlags,
        TextureType textureType,
        FilterMode minFilter, FilterMode magFilter, MipmapFilterMode mipmapFilter) {
    bool genMipmaps = (mipmapFilter != MIPMAP_FILTER_NONE);
    auto createTextureFn = [&](void* srcData) -> ngfx::Texture* {
        PixelFormat pixelFormat = pixelFormatMap.at(dataType);
        uint32_t size = bpp * w * h * d * numLayers;
        return ngfx::Texture::create(ctx, graphics, srcData, pixelFormat, size, w, h, d, numLayers, imageUsageFlags, textureType, genMipmaps,
            minFilter, magFilter, mipmapFilter == MIPMAP_FILTER_LINEAR ? FILTER_LINEAR : FILTER_NEAREST);
    };
    if (dataType == UBVEC3) {
        vector<u8vec4> v0;
        if (data) {
            convertTextureData<u8vec3, u8vec4, uint8_t>((u8vec3*)data, w * h * d * numLayers, 0xFF, v0);
            data = v0.data();
        }
        dataType = UBVEC4; bpp = sizeof(u8vec4);
        return createTextureFn(data);
    }
    else if (dataType == UIVEC3) {
        vector<uvec4> v0;
        if (data) {
            convertTextureData<uvec3, uvec4, uint32_t>((uvec3*)data, w * h * d * numLayers, 0xFFFFFFFF, v0);
            data = v0.data();
        }
        dataType = UIVEC4; bpp = sizeof(uvec4);
        return createTextureFn(data);
    }
    else if (dataType == VEC3) {
        vector<vec4> v0;
        if (data) {
            convertTextureData<vec3, vec4, float>((vec3*)data, w * h * d * numLayers, 1.0f, v0);
            data = v0.data();
        }
        dataType = VEC4; bpp = sizeof(vec4);
        return createTextureFn(data);
    }
    return createTextureFn(data);
}

static void updateTexture(void* data, uint32_t bpp, uint32_t w, uint32_t h, uint32_t d, uint32_t numLayers,
        DataType dataType, ngfx::Texture* texture) {
    auto updateTextureFn = [&](void* srcData) {
        uint32_t size = bpp * w * h * d * numLayers;
        texture->upload(srcData, size);
    };
    if (dataType == UBVEC3) {
        vector<u8vec4> v0;
        if (data) {
            convertTextureData<u8vec3, u8vec4, uint8_t>((u8vec3*)data, w * h * d * numLayers, 0xFF, v0);
            data = v0.data();
        }
        dataType = UBVEC4; bpp = sizeof(u8vec4);
        updateTextureFn(data);
        return;
    }
    else if (dataType == UIVEC3) {
        vector<uvec4> v0;
        if (data) {
            convertTextureData<uvec3, uvec4, uint32_t>((uvec3*)data, w * h * d * numLayers, 0xFFFFFFFF, v0);
            data = v0.data();
        }
        dataType = UIVEC4; bpp = sizeof(uvec4);
        updateTextureFn(data);
        return;
    }
    else if (dataType == VEC3) {
        vector<vec4> v0;
        if (data) {
            convertTextureData<vec3, vec4, float>((vec3*)data, w * h * d * numLayers, 1.0f, v0);
            data = v0.data();
        }
        dataType = VEC4; bpp = sizeof(vec4);
        updateTextureFn(data);
        return;
    }
    updateTextureFn(data);
}

void Texture2DPriv::init(GraphicsContext *ctx, Graphics* graphics, GraphicsState state) {
    if (v) return;
    this->ctx = ctx;
    if (p0->dataSrc) dataSrc = sp_cast<NodePriv>(p0->dataSrc->getBackend());
    if (dataSrc) dataSrc->init(ctx, graphics, state);
    if (auto media = sp_cast<Media>(p0->dataSrc)) {
        v.reset(ngfx::Texture::create(ctx, graphics, media->filename.c_str(),
                ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT)));
        media->width = p0->w;
        media->height = p0->h;
    } else if (auto buffer = sp_cast<Buffer>(p0->dataSrc)) {
        v.reset(createTexture(ctx, graphics, buffer->data->v, buffer->stride, p0->w, p0->h, 1, 1, buffer->dataType,
            ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT),
            TEXTURE_TYPE_2D, p0->minFilter, p0->magFilter, p0->mipmapFilter));
    } else if (auto buffer = sp_cast<AnimatedBuffer>(p0->dataSrc)) {
        v.reset(createTexture(ctx, graphics, nullptr, buffer->stride, p0->w, p0->h, 1, 1, buffer->dataType,
            ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT),
            TEXTURE_TYPE_2D, p0->minFilter, p0->magFilter, p0->mipmapFilter));
    } else {
        if (p0->pixelFormat == PIXELFORMAT_D16_UNORM) {
            v.reset(ngfx::Texture::create(ctx, graphics, nullptr, p0->pixelFormat, p0->w * p0->h * 2, p0->w, p0->h, 1, 1,
                ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)));
        }
        else v.reset(createTexture(ctx, graphics, nullptr, 4, p0->w, p0->h, 1, 1, UBVEC4,
            ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
            TEXTURE_TYPE_2D, p0->minFilter, p0->magFilter, p0->mipmapFilter));
    }
}

void Texture2DPriv::update() {
    if (dataSrc) dataSrc->update();
    if (auto buffer = sp_cast<AnimatedBuffer>(p0->dataSrc)) {
        auto bufferData = buffer->value.get();
        updateTexture(bufferData->v, buffer->stride, p0->w, p0->h, 1, 1, buffer->dataType, v.get());
    }
}

void Texture3DPriv::init(GraphicsContext *ctx, Graphics* graphics, GraphicsState state) {
    if (v) return;
    this->ctx = ctx;
    if (p0->dataSrc) dataSrc = sp_cast<NodePriv>(p0->dataSrc->getBackend());
    if (dataSrc) dataSrc->init(ctx, graphics, state);
    if (auto buffer = sp_cast<Buffer>(p0->dataSrc)) {
        v.reset(createTexture(ctx, graphics, buffer->data->v, buffer->stride, p0->w, p0->h, p0->d, 1, buffer->dataType,
              ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT),
              TextureType::TEXTURE_TYPE_3D, p0->minFilter, p0->magFilter, p0->mipmapFilter));
    } else if (auto buffer = sp_cast<AnimatedBuffer>(p0->dataSrc)) {
        v.reset(createTexture(ctx, graphics, nullptr, buffer->stride, p0->w, p0->h, p0->d, 1, buffer->dataType,
              ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT),
              TextureType::TEXTURE_TYPE_3D, p0->minFilter, p0->magFilter, p0->mipmapFilter));
    } else {
        v.reset(createTexture(ctx, graphics, nullptr, 4, p0->w, p0->h, p0->d, 1, UBVEC4,
            ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
            TextureType::TEXTURE_TYPE_3D, p0->minFilter, p0->magFilter, p0->mipmapFilter));
    }
}

void Texture3DPriv::update() {
    if (dataSrc) dataSrc->update();
    if (auto buffer = sp_cast<AnimatedBuffer>(p0->dataSrc)) {
        updateTexture(buffer->data, buffer->stride, p0->w, p0->h, p0->d, 1, buffer->dataType, v.get());
    }
}

void TextureCubePriv::init(GraphicsContext *ctx, Graphics* graphics, GraphicsState state) {
    if (v) return;
    this->ctx = ctx;
    if (p0->dataSrc) dataSrc = sp_cast<NodePriv>(p0->dataSrc->getBackend());
    if (dataSrc) dataSrc->init(ctx, graphics, state);
    if (auto buffer = sp_cast<Buffer>(p0->dataSrc)) {
        v.reset(createTexture(ctx, graphics, buffer->data->v, buffer->stride, p0->dim(), p0->dim(), 1, p0->numLayers, buffer->dataType,
              ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT),
              TextureType::TEXTURE_TYPE_CUBE, p0->minFilter, p0->magFilter, p0->mipmapFilter));
    } else if (auto buffer = sp_cast<AnimatedBuffer>(p0->dataSrc)) {
        v.reset(createTexture(ctx, graphics, nullptr, buffer->stride, p0->dim(), p0->dim(), 1, p0->numLayers, buffer->dataType,
              ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT),
              TextureType::TEXTURE_TYPE_CUBE, p0->minFilter, p0->magFilter, p0->mipmapFilter));
    } else {
        v.reset(createTexture(ctx, graphics, nullptr, 4, p0->dim(), p0->dim(), 1, p0->numLayers, UBVEC4,
            ImageUsageFlags(p->usageFlags | IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
            TextureType::TEXTURE_TYPE_CUBE, p0->minFilter, p0->magFilter, p0->mipmapFilter));
    }
}

void TextureCubePriv::update() {
    if (dataSrc) dataSrc->update();
    if (auto buffer = sp_cast<AnimatedBuffer>(p0->dataSrc)) {
        updateTexture(buffer->data, buffer->stride, p0->dim(), p0->dim(), 1, p0->dim(), buffer->dataType, v.get());
    }
}

void ComputeProgramPriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    this->ctx = ctx;
    auto device = ctx->device;
    cs = ComputeShaderModule::create(device, p0->computeShaderFile);
    descriptorInfos = &cs->descriptors;
    uniformBufferInfos = &cs->uniformBufferInfos;
    shaderStorageBufferInfos = &cs->shaderStorageBufferInfos;
    createPipeline();
}

void ComputeProgramPriv::createPipeline() {
    computePipeline.reset(ComputePipeline::create(ctx, cs.get()));
}

int32_t ComputeProgramPriv::getDescriptorBinding(string name) {
    for (auto& desc : *descriptorInfos) {
        if (desc.name == name) return desc.set;
    }
    return -1;
}

void GraphicsProgramPriv::init(GraphicsContext *ctx, GraphicsState state, PrimitiveTopology topology,
        set<string> instanceAttributes) {
    this->ctx = ctx;
    this->topology = topology;
    this->instanceAttributes = instanceAttributes;
    auto device = ctx->device;
    vs = VertexShaderModule::create(device, p0->vertexShaderFile);
    fs = FragmentShaderModule::create(device, p0->fragmentShaderFile);
    initDescriptorInfos(vs->descriptors, fs->descriptors);
    initBufferInfos(vs->uniformBufferInfos, fs->uniformBufferInfos, uniformBufferInfos);
    initBufferInfos(vs->shaderStorageBufferInfos, fs->shaderStorageBufferInfos, shaderStorageBufferInfos);
    createPipeline(state);
}

int32_t GraphicsProgramPriv::getAttrLocation(string name) {
    for (auto& attr : vs->attributes) {
        if (attr.name == name) return attr.location;
    }
    return -1;
}

int32_t GraphicsProgramPriv::getDescriptorBinding(string name) {
    for (auto& desc : descriptorInfos) {
        if (desc.name == name) return desc.set;
    }
    return -1;
}

void GraphicsProgramPriv::initDescriptorInfos(
        DescriptorInfos &vsDescriptorInfos,
        DescriptorInfos &fsDescriptorInfos) {
    for (auto & desc : vsDescriptorInfos) {
        descriptorInfos.push_back(desc);
    }
    for (auto & desc : fsDescriptorInfos) {
        descriptorInfos.push_back(desc);
    }
}

void GraphicsProgramPriv::initBufferInfos(
        const BufferInfos &vsUboInfos,
        const BufferInfos &fsUboInfos,
        BufferInfos &bufferInfos) {
    for (auto & info : vsUboInfos) {
        auto& bufferInfo = bufferInfos[info.first];
        bufferInfo.set = info.second.set;
        bufferInfo.name = info.second.name;
        bufferInfo.memberInfos = info.second.memberInfos;
        bufferInfo.shaderStages |= SHADER_STAGE_VERTEX_BIT;
    }
    for (auto & info : fsUboInfos) {
        auto& bufferInfo = bufferInfos[info.first];
        bufferInfo.set = info.second.set;
        bufferInfo.name = info.second.name;
        bufferInfo.memberInfos = info.second.memberInfos;
        bufferInfo.shaderStages |= SHADER_STAGE_FRAGMENT_BIT;
    }
}

void GraphicsProgramPriv::createPipeline(GraphicsState state) {
    RenderPass* renderPass = state.rttPass ?
            ctx->getRenderPass({ true, state.enableDepthStencil, state.numSamples, state.numColorAttachments }) :
            ctx->defaultRenderPass;
    PixelFormat colorFormat = state.colorFormat;
    PixelFormat depthFormat = state.depthFormat;
    auto& nglCtx = p->ctx;
    GraphicsPipeline::State pipelineState;
    pipelineState.renderPass = renderPass;
    pipelineState.primitiveTopology = topology;
    auto gfxCfg = (nglCtx) ? nglCtx->graphicsConfig : nullptr;
    if (gfxCfg) {
        pipelineState.blendEnable = gfxCfg->blend;
        pipelineState.blendSrcFactor = gfxCfg->blendSrcFactor;
        pipelineState.blendDstFactor = gfxCfg->blendDstFactor;
        pipelineState.blendOp = gfxCfg->blendOp;
        pipelineState.colorWriteMask = gfxCfg->colorWriteMask;
        if (gfxCfg->depthTest) state.enableDepthStencil = true;
    }
    if (state.enableDepthStencil) {
        pipelineState.depthTestEnable = true;
        pipelineState.depthWriteEnable = true;
    }
    pipelineState.cullModeFlags = CULL_MODE_NONE;
    pipelineState.frontFace = FRONT_FACE_CLOCKWISE;
    pipelineState.numSamples = state.numSamples;
    pipelineState.numColorAttachments = state.numColorAttachments;
    graphicsPipeline.reset(GraphicsPipeline::create(ctx, pipelineState, vs.get(), fs.get(), colorFormat, depthFormat,
        instanceAttributes));
}

static uint32_t calcBufferSize(const BufferMemberInfos& memberInfos) {
    uint32_t bufferSize = 0;
    for (auto& it : memberInfos) {
        auto& p = it.second;
        bufferSize = glm::max(bufferSize, p.offset + (p.arrayCount ? p.arrayCount * p.arrayStride : p.size));
    }
#ifdef GRAPHICS_BACKEND_METAL
    bufferSize = (bufferSize + 15) / 16 * 16; //TODO: move to shader reflection map
#endif
    return bufferSize;
}

static void findBufferMembers(
        const std::string& bufferName,
        BufferMemberInfos &memberInfos,
        std::map<std::string, sp<Node>> &vars,
        std::map<std::string, sp<Node>> &bufferMembers) {
    for (auto &memberInfo : memberInfos) {
        auto varsIt = vars.find(memberInfo.first);
        if (varsIt != vars.end()) {
            bufferMembers[varsIt->first] = varsIt->second;
        } else {
            LOG("cannot find member: %s", memberInfo.first.c_str());
        }
    }
}

static sp<Block> findBlock(
        const std::string& bufferName,
        std::map<std::string, sp<Block>> &blocks) {
    auto blockIt = blocks.find(bufferName);
    if (blockIt != blocks.end()) { return blockIt->second; }
    return nullptr;
}

static void updateUBO(const std::string& bufferName,
    BufferData& bufferData,
    BufferMemberInfos &memberInfos,
    std::map<std::string, sp<Node>> &members);

static void initUBOs(GraphicsContext* ctx,
        DescriptorInfos& descriptorInfos,
        BufferInfos &bufferInfos,
        std::map<std::string, sp<Node>> &uniforms,
        std::map<std::string, sp<Block>> &blocks,
        std::map<std::string, BufferData> &ubos) {
    for (auto& bufferInfoIt : bufferInfos ) {
        auto& bufferName = bufferInfoIt.first;
        auto& bufferInfo = bufferInfoIt.second;
        uint32_t bufferSize = calcBufferSize(bufferInfo.memberInfos);
        if (bufferSize > 0) {
            sp<Block> block = findBlock(bufferName, blocks);
            sp<ngfx::Buffer> buffer;
            sp<BlockPriv> blockPriv;
            if (block) {
                blockPriv = sp_cast<BlockPriv>(block->getBackend());
                buffer = blockPriv->buffer;
            }
            if (!buffer) buffer.reset(createUniformBuffer(ctx, nullptr, bufferSize));
            BufferData bufferData = { buffer, bufferSize, &bufferInfo };
            if (blockPriv) blockPriv->buffer = buffer;
            std::map<std::string, sp<Node>> bufferMembers;
            if (block) {
                findBufferMembers(bufferName, bufferInfo.memberInfos, block->fields, bufferMembers);
            } else {
                findBufferMembers(bufferName, bufferInfo.memberInfos, uniforms, bufferMembers);
            }
            updateUBO(bufferName, bufferData, bufferInfo.memberInfos, bufferMembers);
            ubos[bufferName] = std::move(bufferData);
        }
    }
}

#define UPDATE_BUFFER_MEMBER_FN1() \
    memcpy(&dstData[memberInfo.offset], uniform->getData(), uniform->getSize());

#define UPDATE_BUFFER_MEMBER_FN2(stride0, d0, offset0, numElements0) \
    uint32_t srcStride = stride0, \
        dstStride = memberInfo.arrayStride; \
    auto data = d0; \
    if (!data) return; \
    uint32_t numElements = numElements0; \
    uint32_t offset = memberInfo.offset; \
    uint8_t *src = &((uint8_t*)data->v)[offset0], *dst = &dstData[offset]; \
    for (uint32_t j = 0; j<numElements; j++) { \
        memcpy(dst, src, srcStride); \
        dst += dstStride; src += srcStride; \
    }

#define UPDATE_BUFFER_MEMBER_FN0(d0) \
    UPDATE_BUFFER_MEMBER_FN2(buffer->stride, d0, 0, d0->size / buffer->stride)

static void updateBufferMember(uint8_t* dstData, string key, sp<Node> node,
        const BufferMemberInfos &memberInfos) {
    auto &memberInfo = memberInfos.at(key);
    if (sp<Uniform> uniform = sp_cast<Uniform>(node)) {
        UPDATE_BUFFER_MEMBER_FN1();
    }
    else if (sp<NGL::Buffer> buffer = sp_cast<NGL::Buffer>(node)) {
        UPDATE_BUFFER_MEMBER_FN0(buffer->data);
    }
    else if (sp<AnimatedBuffer> buffer = sp_cast<AnimatedBuffer>(node)) {
        UPDATE_BUFFER_MEMBER_FN0(buffer->value);
    }
    else if (sp<StreamedBuffer> streamedBuffer = sp_cast<StreamedBuffer>(node)) {
        UPDATE_BUFFER_MEMBER_FN2(streamedBuffer->stride, streamedBuffer->buffer->data,
            streamedBuffer->getOffset(), streamedBuffer->count);
    }
    else if (sp<AnimatedNode> uniform = sp_cast<AnimatedNode>(node)) {
        UPDATE_BUFFER_MEMBER_FN1();
    }
}

static void updateUBO(const std::string& bufferName,
        BufferData& bufferData,
        BufferMemberInfos &memberInfos,
        std::map<std::string, sp<Node>> &members) {
    uint8_t* dstData = (uint8_t*)bufferData.buffer->map();
    for (auto& it: members) {
        updateBufferMember(dstData, it.first, it.second, memberInfos);
    }
    bufferData.buffer->unmap();
}

static void updateSSBO(const std::string& bufferName,
    BufferData& bufferData, BufferMemberInfos &memberInfos,
    std::map<std::string, sp<Node>> &members);

static void initSSBOs(GraphicsContext* ctx, DescriptorInfos& descriptorInfos, BufferInfos& bufferInfos,
        std::map<std::string, sp<Node>> &uniforms, std::map<std::string, sp<Block>> &blocks,
        std::map<std::string, BufferData> &ssbos) {
    for (auto& bufferInfoIt : bufferInfos ) {
        auto& bufferName = bufferInfoIt.first;
        auto& bufferInfo = bufferInfoIt.second;
        uint32_t bufferSize = calcBufferSize(bufferInfo.memberInfos);
        if (bufferSize == 0) ERR("buffer size is 0");
        sp<Block> block = findBlock(bufferName, blocks);
        if (!block) ERR("cannot find block: %s", bufferName.c_str());
        sp<BlockPriv> blockPriv = sp_cast<BlockPriv>(block->getBackend());
        auto& buffer = blockPriv->buffer;
        if (!buffer) buffer.reset(ngfx::Buffer::create(ctx, nullptr, bufferSize, 0,
            BUFFER_USAGE_STORAGE_BUFFER_BIT | BUFFER_USAGE_VERTEX_BUFFER_BIT));
        BufferData bufferData = { buffer, bufferSize, &bufferInfo };
        std::map<std::string, sp<Node>> bufferMembers;
        findBufferMembers(bufferName, bufferInfo.memberInfos, block->fields, bufferMembers);
        updateSSBO(bufferName, bufferData, bufferInfo.memberInfos, bufferMembers);
        ssbos[bufferName] = std::move(bufferData);
    }
}

static void updateSSBO(const std::string& bufferName,
        BufferData& bufferData, BufferMemberInfos &memberInfos,
        std::map<std::string, sp<Node>> &members) {
    uint8_t* dstData = (uint8_t*)bufferData.buffer->map();
    for (auto& it : members) {
        updateBufferMember(dstData, it.first, it.second, memberInfos);
    }
    bufferData.buffer->unmap();
}

static void updateSSBOs(
        DescriptorInfos& descriptorInfos,
        BufferInfos& bufferInfos,
        std::map<std::string, sp<Node>> &uniforms,
        std::map<std::string, sp<Block>> &blocks,
        std::map<std::string, BufferData> &ssbos) {
    for (auto& bufferInfoIt : bufferInfos) {
        auto& bufferInfo = bufferInfoIt.second;
        auto& bufferName = bufferInfo.name;
        auto& bufferData = ssbos.at(bufferName);
        std::map<std::string, sp<Node>> bufferMembers;
        sp<Block> block = findBlock(bufferName, blocks);
        if (!block) ERR("cannot find block: %s", bufferName.c_str());
        findBufferMembers(bufferName, bufferInfo.memberInfos, block->fields, bufferMembers);
        updateSSBO(bufferName, bufferData, bufferInfo.memberInfos, bufferMembers);
    }
}

static void updateUBOs(
        DescriptorInfos& descriptorInfos,
        BufferInfos& bufferInfos,
        std::map<std::string, sp<Node>> &uniforms,
        std::map<std::string, sp<Block>> &blocks,
        std::map<std::string, BufferData> &ubos) {
    for (auto& bufferInfoIt : bufferInfos) {
        auto& bufferInfo = bufferInfoIt.second;
        auto& bufferName = bufferInfo.name;
        auto& bufferData = ubos.at(bufferName);
        std::map<std::string, sp<Node>> bufferMembers;
        sp<Block> block = findBlock(bufferName, blocks);
        if (block) {
            findBufferMembers(bufferName, bufferInfo.memberInfos, block->fields, bufferMembers);
        } else {
            findBufferMembers(bufferName, bufferInfo.memberInfos, uniforms, bufferMembers);
        }
        updateUBO(bufferName, bufferData, bufferInfo.memberInfos, bufferMembers);
    }
}

void RenderPriv::init(GraphicsContext *ctx, Graphics* graphics, GraphicsState state) {
    this->ctx = ctx;
    this->rtt = state.rttPass;
    if (!state.rttPass) {
        state.colorFormat = ctx->surfaceFormat;
        state.depthFormat = ctx->depthFormat;
    }
    geom = sp_cast<GeometryPriv>(p->geom->getBackend());
    if (!p->program) p->program.reset(
        new NGL::GraphicsProgram(
            "default.vert",
            "default.frag"
        ));
    graphicsProgram = sp_cast<GraphicsProgramPriv>(p->program->getBackend());
    for (auto it : p->uniforms) {
        uniforms[it.first] = sp_cast<NodePriv>(it.second->getBackend());
    }
    for (auto it : p->blocks) {
        auto& block = blocks[it.first];
        block = sp_cast<BlockPriv>(it.second->getBackend());
        block->init(ctx, graphics, state);
    }
    geom->init(ctx, graphics, state);
    set<string> instanceAttributesSet;
    for (auto& it: p->instanceAttributes) instanceAttributesSet.insert(it.first);
    graphicsProgram->init(ctx, state, p->geom->topology, instanceAttributesSet);
    for (auto it : p->textures) {
        string descriptorName = it.first + "_sampler";
        int32_t set = graphicsProgram->getDescriptorBinding(descriptorName);
        if (set == -1) ERR("cannot find texture: %s", descriptorName.c_str());
        auto texture = sp_cast<TexturePriv>(it.second->getBackend());
        texture->init(ctx, graphics, state);
        textures[descriptorName] = { texture, uint32_t(set) };
    }
    initSSBOs(ctx, graphicsProgram->descriptorInfos, graphicsProgram->shaderStorageBufferInfos, p->uniforms, p->blocks, ssbos);
    initUBOs(ctx, graphicsProgram->descriptorInfos, graphicsProgram->uniformBufferInfos, p->uniforms, p->blocks, ubos);
    auto initInstanceAttrs = [&] {
        for (auto& it : p->instanceAttributes) {
            auto& buffer = it.second;
            uint32_t location = graphicsProgram->getAttrLocation(it.first);
            instanceAttributes[location].reset(
                createVertexBuffer(ctx, buffer->data->v, buffer->data->size, buffer->stride)
            );
        }
    };
    initInstanceAttrs();
    B_POS = graphicsProgram->getAttrLocation("ngl_position");
    assert(B_POS != -1);
    B_NORMAL = graphicsProgram->getAttrLocation("ngl_normal");
    B_TEXCOORD = graphicsProgram->getAttrLocation("ngl_uvcoord");
    auto& vsAttr = graphicsProgram->vs->attributes;
    if (geom->bPos->stride == 0) geom->bPos->stride = vsAttr[B_POS].elementSize;
    if (geom->bNormal && geom->bNormal->stride == 0) geom->bNormal->stride = vsAttr[B_NORMAL].elementSize;
    if (geom->bTexCoord && geom->bTexCoord->stride == 0) geom->bTexCoord->stride = vsAttr[B_TEXCOORD].elementSize;
}

void RenderPriv::draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) {
    if (state.rttPass != rtt) return;
    GraphicsPipeline* graphicsPipeline = graphicsProgram->graphicsPipeline.get();
    graphics->bindGraphicsPipeline(commandBuffer, graphicsPipeline);
    graphics->bindVertexBuffer(commandBuffer, geom->bPos.get(), B_POS);
    if (geom->bTexCoord && B_TEXCOORD != -1) graphics->bindVertexBuffer(commandBuffer, geom->bTexCoord.get(), B_TEXCOORD);
    if (geom->bNormal && B_NORMAL != -1) graphics->bindVertexBuffer(commandBuffer, geom->bNormal.get(), B_NORMAL);
    for (auto& it : instanceAttributes) {
        uint32_t location = it.first;
        auto& buffer = it.second;
        graphics->bindVertexBuffer(commandBuffer, buffer.get(), location);
    }
    for (auto& it : textures) {
        auto& textureData = it.second;
        graphics->bindTexture(commandBuffer, textureData.texture->v.get(), textureData.set);
    }
    for (auto& it : ubos) {
        auto &bufferData = it.second;
        graphics->bindUniformBuffer(commandBuffer, bufferData.buffer.get(), bufferData.bufferInfo->set, bufferData.bufferInfo->shaderStages);
    }
    for (auto& it : ssbos) {
        auto &bufferData = it.second;
        graphics->bindStorageBuffer(commandBuffer, bufferData.buffer.get(), bufferData.bufferInfo->set, bufferData.bufferInfo->shaderStages);
    }
    auto nglCtx = p->ctx;
    auto gfxCfg = (nglCtx) ? nglCtx->graphicsConfig : nullptr;
    Rect2D currentScissorRect = graphics->scissorRect;
    if (gfxCfg && gfxCfg->scissorTest) {
        graphics->setScissor(commandBuffer, gfxCfg->scissorRect);
    }
    if (geom->bIndices) {
        graphics->bindIndexBuffer(commandBuffer, geom->bIndices.get());
        graphics->drawIndexed(commandBuffer, geom->numIndices, p->numInstances);
    }
    else graphics->draw(commandBuffer, geom->numVerts, p->numInstances);
    if (gfxCfg && gfxCfg->scissorTest) {
        graphics->setScissor(commandBuffer, currentScissorRect);
    }
}

void RenderPriv::update() {
    geom->update();
    for (auto& it: uniforms) it.second->update();
    for (auto& it: blocks) it.second->update();
    for (auto& it: textures) it.second.texture->update();
    updateSSBOs(graphicsProgram->descriptorInfos, graphicsProgram->shaderStorageBufferInfos, p->uniforms, p->blocks, ssbos);
    updateUBOs(graphicsProgram->descriptorInfos, graphicsProgram->uniformBufferInfos, p->uniforms, p->blocks, ubos);
}

void ComputePriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    this->ctx = ctx;
    this->rtt = state.rttPass;
    computeProgram = sp_cast<ComputeProgramPriv>(p->program->getBackend());
    for (auto it : p->uniforms) {
        auto uniform = sp_cast<NodePriv>(it.second->getBackend());
        uniform->init(ctx, graphics, state);
        uniforms[it.first] = uniform;
    }
    for (auto it : p->blocks) {
        auto& block = blocks[it.first];
        block = sp_cast<BlockPriv>(it.second->getBackend());
        block->init(ctx, graphics, state);
    }
    computeProgram->init(ctx, graphics, state);
    for (auto it : p->textures) {
        string descriptorName = it.first + "_sampler";
        int32_t set = computeProgram->getDescriptorBinding(descriptorName);
        if (set == -1) ERR("cannot find texture: %s", descriptorName.c_str());
        auto texture = sp_cast<TexturePriv>(it.second->getBackend());
        texture->init(ctx, graphics, state);
        textures[descriptorName] = { texture, uint32_t(set) };
    }
    initSSBOs(ctx, *computeProgram->descriptorInfos, *computeProgram->shaderStorageBufferInfos, p->uniforms, p->blocks, ssbos);
    initUBOs(ctx, *computeProgram->descriptorInfos, *computeProgram->uniformBufferInfos, p->uniforms, p->blocks, ubos);
}
void ComputePriv::compute(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) {
    ComputePipeline* computePipeline = computeProgram->computePipeline.get();
    graphics->bindComputePipeline(commandBuffer, computePipeline);
    for (auto& it : textures) {
        auto& textureData = it.second;
        graphics->bindTexture(commandBuffer, textureData.texture->v.get(), textureData.set);
    }
    for (auto& it : ubos) {
        auto &bufferData = it.second;
        graphics->bindUniformBuffer(commandBuffer, bufferData.buffer.get(), bufferData.bufferInfo->set, bufferData.bufferInfo->shaderStages);
    }
    for (auto& it : ssbos) {
        auto &bufferData = it.second;
        graphics->bindStorageBuffer(commandBuffer, bufferData.buffer.get(), bufferData.bufferInfo->set, bufferData.bufferInfo->shaderStages);
    }
    graphics->dispatch(commandBuffer, p->groupCountX, p->groupCountY, p->groupCountZ, p->threadsPerGroupX, p->threadsPerGroupY, p->threadsPerGroupZ);
}

void ComputePriv::update() {
    for (auto& it: uniforms) it.second->update();
    for (auto& it: blocks) it.second->update();
    updateSSBOs(*computeProgram->descriptorInfos, *computeProgram->shaderStorageBufferInfos, p->uniforms, p->blocks, ssbos);
    updateUBOs(*computeProgram->descriptorInfos, *computeProgram->uniformBufferInfos, p->uniforms, p->blocks, ubos);
}

void TransformPriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    if (p->child) child = sp_cast<NodePriv>(p->child->getBackend());
    initAnim(ctx, graphics, state);
    if (child) {
        beginTransform();
        child->init(ctx, graphics, state);
        endTransform();
    }
}

void TransformPriv::beginTransform() {
    auto& modelViewMat = p->ctx->modelViewMat->value;
    auto& normalMat = p->ctx->normalMat->value;
    currentModelViewMat = modelViewMat;
    currentNormalMat = normalMat;
    modelViewMat = currentModelViewMat * p->getMat();
    normalMat = transpose(inverse(modelViewMat));
}

void TransformPriv::endTransform() {
    auto& modelViewMat = p->ctx->modelViewMat->value;
    auto& normalMat = p->ctx->normalMat->value;
    modelViewMat = currentModelViewMat;
    normalMat = currentNormalMat;
}

void CameraPriv::beginTransform() {
    TransformPriv::beginTransform();
    auto& projMat = p->ctx->projMat->value;
    currentProjMat = projMat;
    projMat = p0->projMat;
}

void CameraPriv::endTransform() {
   TransformPriv::endTransform();
   auto& projMat = p->ctx->projMat->value;
   projMat = currentProjMat;
}

#define INIT_TRANSFORM_ANIM(t0, anim0, setFn) \
    if (p0->anim0) { \
        anim0 = sp_cast<AnimatedNodePriv>(p0->anim0->getBackend()); \
        anim0->init(ctx, graphics, state); \
        p0->setFn(((Animated##t0*)anim0->p0)->value); \
    }
#define UPDATE_TRANSFORM_ANIM(t0, anim0, setFn) \
    if (anim0) { \
        anim0->update(); \
        p0->setFn(((Animated##t0*)anim0->p0)->value); \
    }

void RotateQuatPriv::initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    INIT_TRANSFORM_ANIM(Quat, anim, set);
}

void RotateQuatPriv::updateAnim() {
    UPDATE_TRANSFORM_ANIM(Quat, anim, set);
}

void CameraPriv::initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    INIT_TRANSFORM_ANIM(Float, fovAnim, setFov);
    if (p0->eyeTransform) {
        eyeTransform = sp_cast<TransformPriv>(p0->eyeTransform->getBackend());
        eyeTransform->init(ctx, graphics, state);
    }
    if (p0->centerTransform) {
        centerTransform = sp_cast<TransformPriv>(p0->centerTransform->getBackend());
        centerTransform->init(ctx, graphics, state);
    }
    if (p0->upTransform) {
        upTransform = sp_cast<TransformPriv>(p0->upTransform->getBackend());
        upTransform->init(ctx, graphics, state);
    }
}

void CameraPriv::updateAnim() {
    UPDATE_TRANSFORM_ANIM(Float, fovAnim, setFov);
    if (eyeTransform) {
        eyeTransform->update(); p0->needsUpdate = true;
    }
    if (centerTransform) {
        centerTransform->update(); p0->needsUpdate = true;
    }
    if (upTransform) {
        upTransform->update(); p0->needsUpdate = true;
    }
}

void RotatePriv::initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    INIT_TRANSFORM_ANIM(Float, anim, set);
}

void RotatePriv::updateAnim() {
    UPDATE_TRANSFORM_ANIM(Float, anim, set);
}

void ScalePriv::initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    INIT_TRANSFORM_ANIM(Vec3, anim, set);
}

void ScalePriv::updateAnim() {
    UPDATE_TRANSFORM_ANIM(Vec3, anim, set);
}

void TranslatePriv::initAnim(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    INIT_TRANSFORM_ANIM(Vec3, anim, set);
}

void TranslatePriv::updateAnim() {
    UPDATE_TRANSFORM_ANIM(Vec3, anim, set);
}

void TransformPriv::update() {
    updateAnim();
    if (child) {
        beginTransform();
        child->update();
        endTransform();
    }
}

void RenderToTexturePriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    this->ctx = ctx;
    state.rttPass = true;
    state.colorFormat = ctx->defaultOffscreenSurfaceFormat;
    outputTextures.resize(p->outputTextures.size());
    for (uint32_t j = 0; j<p->outputTextures.size(); j++) {
        outputTextures[j] = sp_cast<TexturePriv>(p->outputTextures[j]->getBackend());
    }
    auto& outputTexture = outputTextures[0];
    child = sp_cast<NodePriv>(p->child->getBackend());
    outputTexture->init(ctx, graphics, state);

    state.numSamples = glm::max(p->numSamples, uint32_t(1));
    std::vector<ngfx::Framebuffer::Attachment> attachments;
    state.numColorAttachments = 1;
    uint32_t w = outputTexture->width(), h = outputTexture->height(), size = w * h * 4;
    if (sp_cast<TextureCubePriv>(outputTexture)) {
        state.numColorAttachments = 6;
    }
    else {
        state.numColorAttachments = 1;
    }
    if (state.numSamples != 1) {
        multisampleColorTexture.reset(ngfx::Texture::create(ctx, graphics, nullptr, ctx->defaultOffscreenSurfaceFormat, size, w, h, 1, state.numColorAttachments,
            IMAGE_USAGE_COLOR_ATTACHMENT_BIT | IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, TEXTURE_TYPE_2D_ARRAY, false,
            FILTER_NEAREST, FILTER_NEAREST, FILTER_NEAREST, state.numSamples));
    }

    for (uint32_t j = 0; j<state.numColorAttachments; j++) {
        if (state.numSamples != 1) {
            attachments.push_back({ multisampleColorTexture.get(), 0, j });
        }
        attachments.push_back({ outputTexture->v.get(), 0, j });
    }

    if (p->flags & RenderToTexture::Flags::DEPTH) {
        state.depthFormat = PIXELFORMAT_D16_UNORM;
        if (state.numSamples != 1) {
            multisampleDepthTexture.reset(ngfx::Texture::create(ctx, graphics, nullptr, state.depthFormat, size, w, h, 1, 1,
                IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, TEXTURE_TYPE_2D, false,
                FILTER_NEAREST, FILTER_NEAREST, FILTER_NEAREST, state.numSamples));
            attachments.push_back({ multisampleDepthTexture.get() });
        }
        depthTexture.reset(ngfx::Texture::create(ctx, graphics, nullptr, state.depthFormat, size, w, h, 1, 1,
            IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));
        attachments.push_back({ depthTexture.get() });
        state.enableDepthStencil = true;
    }
    else if (p->depthTexture) {
        depthTextureRef = sp_cast<Texture2DPriv>(p->depthTexture->getBackend());
        depthTextureRef->init(ctx, graphics, state);
        depthTexture = depthTextureRef->v;
        attachments.push_back({ depthTexture.get() });
        state.enableDepthStencil = true;
        state.depthFormat = depthTexture->format;
    }
    else {
        state.depthFormat = PIXELFORMAT_UNDEFINED;
    }
    renderPass = ctx->getRenderPass({ true, state.enableDepthStencil, state.numSamples, state.numColorAttachments });
    outputFramebuffer.reset(Framebuffer::create(ctx->device, renderPass,
        attachments, outputTexture->width(), outputTexture->height()));
    if (child) child->init(ctx, graphics, state);
}

void RenderToTexturePriv::draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) {
    if (!state.rttPass) return;
    vec4 clearColor = (p->setClearColor) ? p->clearColor : ctx->clearColor;

    if (multisampleColorTexture) multisampleColorTexture->changeLayout(commandBuffer, IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    for (auto& outputTexture : outputTextures) outputTexture->v->changeLayout(commandBuffer, IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    if (multisampleDepthTexture) multisampleDepthTexture->changeLayout(commandBuffer, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    if (depthTexture) depthTexture->changeLayout(commandBuffer, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    graphics->beginRenderPass(commandBuffer, renderPass, outputFramebuffer.get(), clearColor);
    graphics->setViewport(commandBuffer, { 0, 0, outputFramebuffer->w, outputFramebuffer->h });
    auto nglCtx = p->ctx;
    auto gfxCfg = (nglCtx) ? nglCtx->graphicsConfig : nullptr;
    Rect2D scissorRect;
    if (gfxCfg && gfxCfg->scissorTest) {
        scissorRect = gfxCfg->scissorRect;
    } else {
        scissorRect = { 0, 0, outputFramebuffer->w, outputFramebuffer->h };
    }
    graphics->setScissor(commandBuffer, scissorRect);
    if (child) child->draw(commandBuffer, graphics, state);
    graphics->endRenderPass(commandBuffer);

    for (auto& outputTexture: outputTextures) {
        if (auto outputTexture2D = sp_cast<Texture2DPriv>(outputTexture)) {
            bool genMipmaps = outputTexture2D->p0->mipmapFilter != MIPMAP_FILTER_NONE;
            if (genMipmaps) {
                outputTexture2D->v->generateMipmaps(commandBuffer);
            }
        }
        if (outputTexture->v->imageUsageFlags & IMAGE_USAGE_SAMPLED_BIT)
            outputTexture->v->changeLayout(commandBuffer, IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    if (depthTexture && depthTexture->imageUsageFlags & IMAGE_USAGE_SAMPLED_BIT) {
        depthTexture->changeLayout(commandBuffer, IMAGE_LAYOUT_GENERAL); //TODO: remove
        depthTexture->changeLayout(commandBuffer, IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

ContextPriv::ContextPriv(Context* thiz, NGLApplication* app)
    : thiz(thiz), app(app) {}

ContextPriv::~ContextPriv() {
    backendCache.clear();
}

void ContextPriv::setConfig(Config* cfg) {
    app->cfg = cfg;
}
void ContextPriv::draw(double t) {
    if (getenv("DEBUG_ONSCREEN")) {
        app->cfg->offscreen = false;
        app->run();
    }
    else app->drawFrame();
}

void ContextPriv::setScene(NGL::NodePriv* scene) {
    app->scene = scene;
}

void GroupPriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    children.resize(p->children.size());
    for (uint32_t j = 0; j<p->children.size(); j++) {
        children[j] = sp_cast<NodePriv>(p->children[j]->getBackend());
    }
    for (auto& child : children) {
        child->init(ctx, graphics, state);
    }
}

void GraphicsConfigPriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    auto& graphicsConfig = p->ctx->graphicsConfig;
    auto currentGraphicsConfig = graphicsConfig;
    graphicsConfig = p;
    child = sp_cast<NodePriv>(p->child->getBackend());
    child->init(ctx, graphics, state);
    graphicsConfig = currentGraphicsConfig;
}

void GraphicsConfigPriv::draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) {
    auto& graphicsConfig = p->ctx->graphicsConfig;
    auto currentGraphicsConfig = graphicsConfig;
    graphicsConfig = p;
    child->draw(commandBuffer, graphics, state);
    graphicsConfig = currentGraphicsConfig;
}

void TimeRangeFilterPriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    child = sp_cast<NodePriv>(p->child->getBackend());
    child->init(ctx, graphics, state);
}

void TimeRangeFilterPriv::draw(CommandBuffer* commandBuffer, Graphics* graphics, GraphicsState state) {
    sp<TimeRangeMode> pRange = p->findRange(p->ctx->time);
    if (auto range = sp_cast<TimeRangeModeNoOp>(pRange)) {
        return;
    }
    child->draw(commandBuffer, graphics, state);
}

void TimeRangeFilterPriv::update() {
    sp<TimeRangeMode> pRange = p->findRange(p->ctx->time);
    if (auto range = sp_cast<TimeRangeModeNoOp>(pRange)) {
        return;
    }
    child->update();
}

void StreamedBufferPriv::init(GraphicsContext* ctx, Graphics* graphics, GraphicsState state) {
    if (p->timeAnim) {
        timeAnim = sp_cast<AnimatedNodePriv>(p->timeAnim->getBackend());
        timeAnim->init(ctx, graphics, state);
    }
}

void StreamedBufferPriv::update() {
    if (timeAnim) timeAnim->update();
}
