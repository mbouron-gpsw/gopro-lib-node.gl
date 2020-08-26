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
#include <functional>
#include <GL/glew.h>
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace glGraphicsEngine {
    typedef GLuint PrimitiveTopology;
    typedef GLuint Format;
    typedef GLuint ValueType;
    typedef GLuint DescriptorType;

    class GraphicsContext;
    class Window {
    public:
        Window(GraphicsContext* graphicsContext, const char* title, std::function<void(Window * thiz)> onWindowCreated,
            int w = DISPLAY_WIDTH, int h = DISPLAY_HEIGHT);
        virtual ~Window() {}
        virtual bool shouldClose() = 0;
        virtual void pollEvents() = 0;
        virtual void update() { if (onUpdate) onUpdate(); }
        virtual void paint() { if (onPaint) onPaint(); }
        virtual void swapBuffers() = 0;
        enum { DISPLAY_WIDTH = -1, DISPLAY_HEIGHT = -1 };
        uint32_t w, h;
        std::function<void()> onUpdate = nullptr, onPaint = nullptr;
    };
    class Graphics {
    public:
        void draw(PrimitiveTopology mode, uint32_t vertexCount,
            uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
        void drawIndexed(PrimitiveTopology mode, uint32_t indexCount, ValueType indexType = GL_UNSIGNED_INT,
            uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
        void setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
        void setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    };
    class GraphicsContext {
    public:
        GraphicsContext(const char* appName, bool debug = true);
        virtual void onWindowCreated(Window* window);
        Window* window;
    };
    class Texture {
    public:
        static std::unique_ptr<Texture> create(const char* filename);
        Texture(void* data, Format internalFormat, uint32_t size, uint32_t w, uint32_t h, uint32_t d);
        void bind(uint32_t location);
        void update(void* data, uint32_t size, uint32_t x = 0, uint32_t y = 0, uint32_t z = 0, int32_t w = -1, int32_t h = -1, int32_t d = -1);
        uint32_t v;
        uint32_t target;
        Format format, internalFormat;
    };
    class IndexBuffer {
    public:
        IndexBuffer(const void* data, uint32_t size, uint32_t stride);
        template <typename T> static std::unique_ptr<IndexBuffer> create(const std::vector<T>& v) {
            return std::make_unique<IndexBuffer>(v.data(), uint32_t(v.size() * sizeof(v[0])), sizeof(T));
        }
        void update(const void* data, uint32_t size, uint32_t offset = 0);
        uint32_t v;
        uint32_t target;
        uint32_t stride;
    };
    class VertexBuffer {
    public: 
        VertexBuffer(const void* data, uint32_t size, uint32_t numComponents, uint32_t type, uint32_t stride);
        template <typename T> static std::unique_ptr<VertexBuffer> create(const std::vector<T>& v) {
            return std::make_unique<VertexBuffer>(v.data(), uint32_t(v.size() * sizeof(v[0])), sizeof(v[0]) / sizeof(float), GL_FLOAT, 0);
        }
        void bind(uint32_t location);
        void update(void* data, uint32_t size, uint32_t offset = 0);
        uint32_t v;
        uint32_t target;
        uint32_t size, numComponents, type, stride;
    };
    class UniformBuffer {
    public:
        UniformBuffer(const void* data, uint32_t size);
        void bind(uint32_t binding);
        void update(const void* data, uint32_t size, uint32_t offset = 0);
        uint32_t v;
        uint32_t target;
        uint32_t stride;
    };
    class Framebuffer {
    public:
        Framebuffer(const std::vector<Texture*>& attachments, 
            uint32_t w, uint32_t h, uint32_t layers = 1);
        void bind();
        void unbind();
        uint32_t w, h;
        uint32_t v;
        uint32_t target;
    };
    class ShaderModule {
    public:
        ShaderModule(const std::string& filename, uint32_t type);
        virtual ~ShaderModule() {}
        struct UniformDescription {
            uint32_t set;
            DescriptorType type;
        };
        struct UniformInfo { uint32_t set, offset, size; };
        uint32_t v;
    };
    class VertexShaderModule : public ShaderModule {
    public:
        VertexShaderModule(const std::string& filename);
        virtual ~VertexShaderModule() {}
        void initBindings(const std::string& filename);
        struct AttributeDescription {
            std::string semantic;
            uint32_t    location;
            uint32_t    binding;
            Format      format;
            uint32_t    offset;
        };
        std::vector<AttributeDescription> attributes;
        std::vector<UniformDescription> uniforms;
        std::map<std::string, UniformInfo> uniformInfos;
    };
    class FragmentShaderModule : public ShaderModule {
    public:
        FragmentShaderModule(const std::string& filename);
        virtual ~FragmentShaderModule() {}
        void initBindings(const std::string& filename);
        std::vector<UniformDescription> uniforms;
        std::map<std::string, UniformInfo> uniformInfos;
    };
    class ComputeShaderModule : public ShaderModule {
    public:
        ComputeShaderModule(const std::string& filename)
            :ShaderModule(filename, GL_COMPUTE_SHADER) {}
        virtual ~ComputeShaderModule() {}
        void initBindings(const std::string& filename) {}
    };
    class Program {
    public:
        virtual ~Program() {}
        virtual void bind() = 0;
        uint32_t v;
    };
    class GraphicsProgram : public Program {
    public:
        GraphicsProgram(VertexShaderModule* vs, FragmentShaderModule* fs);
        virtual ~GraphicsProgram() {}
        virtual void bind();
        std::map<std::string, ShaderModule::UniformInfo> uniformInfos;
    };
    class GLApplication {
    public:
        GLApplication(const std::string& appName);
        virtual ~GLApplication() {}
        virtual void onInit() {}
        virtual void onPaint() {}
        virtual void onUpdate() {}
        virtual void run();
    protected:
        virtual void init();
        virtual void close();
        std::unique_ptr<Graphics> graphics;
        std::unique_ptr<Window> window;
        std::unique_ptr<GraphicsContext> graphicsContext;
        std::string appName;
    };
};
