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
#include "GLGraphicsEngine.h"
#include "GLDebugUtil.h"
#include "glGLFWWindow.h"
#include "FPSCounter.h"
#include "DebugUtil.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
using namespace glGraphicsEngine;
using namespace std;

Window::Window(GraphicsContext* graphicsContext, const char* title, std::function<void(Window * thiz)> onWindowCreated, int w, int h) 
	:w(w), h(h) {}

GraphicsContext::GraphicsContext(const char* appName, bool debug) {}

static void onGLError(GLenum source, GLenum type, GLuint id, GLenum severity, 
		GLsizei length, const GLchar* message, const void* userParam) {
	if (severity == GL_DEBUG_SEVERITY_HIGH) { ERR("%s", message); }
	else { LOG("%s", message); }
}
void GraphicsContext::onWindowCreated(Window* window) {
	this->window = window;
#ifdef ENABLE_GL_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(onGLError, 0);
#endif
	glEnable(GL_CULL_FACE);
}

void Graphics::draw(PrimitiveTopology mode, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	glDrawArraysInstanced(mode, firstVertex, vertexCount, instanceCount);
}

void Graphics::drawIndexed(PrimitiveTopology mode, uint32_t indexCount, ValueType indexType, uint32_t instanceCount,
		uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
	glDrawElementsInstanced(mode, indexCount, indexType, nullptr, instanceCount);
}

void Graphics::setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
	glViewport(x, y, w, h);
}

void Graphics::setScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
	glScissor(x, y, w, h);
}

std::unique_ptr<Texture> Texture::create(const char* filename) {
	int w, h, channels;
	std::unique_ptr<stbi_uc> data(stbi_load(filename, &w, &h, &channels, 4));
	assert(data);
	return make_unique<Texture>(data.get(), GL_RGBA8, w * h * 4, w, h, 1);
}

Texture::Texture(void* data, Format internalFormat, uint32_t size, uint32_t w, uint32_t h, uint32_t d) {
	glGenTextures(1, &v);
	this->target = GL_TEXTURE_2D;
	this->internalFormat = internalFormat;
	if (internalFormat == GL_RGBA8) format = GL_RGBA;
	else { ERR("TODO: support format: %d", internalFormat); }
	glBindTexture(target, v);
	glTexStorage2D(target, 1, internalFormat, w, h);
	if (data) glTexSubImage2D(target, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void Texture::bind(uint32_t location) {
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0 + location);
	glBindTexture(target, v);
}

void Texture::update(void* data, uint32_t size, uint32_t x, uint32_t y, uint32_t z, int32_t w, int32_t h, int32_t d) {
	glBindTexture(target, v);
	if (data) glTexSubImage2D(target, 0, x, y, w, h, format, GL_UNSIGNED_BYTE, data);
}

IndexBuffer::IndexBuffer(const void* data, uint32_t size, uint32_t stride) {
	this->target = GL_ELEMENT_ARRAY_BUFFER;
	this->stride = stride;
	glGenBuffers(1, &v);
	update(data, size);
}

void IndexBuffer::update(const void* data, uint32_t size, uint32_t offset) {
	glBindBuffer(target, v);
	glBufferSubData(target, offset, size, data);
}

VertexBuffer::VertexBuffer(const void* data, uint32_t size, uint32_t numComponents, uint32_t type, uint32_t stride) {
	this->target = GL_ARRAY_BUFFER;
	this->size = size;
	this->numComponents = numComponents;
	this->type = type;
	this->stride = stride;
	glGenBuffers(1, &v);
	glBindBuffer(target, v);
	glBufferStorage(target, size, data, 0);
}

void VertexBuffer::bind(uint32_t location) {
	glEnableVertexAttribArray(location);
	glBindBuffer(target, v);
	glVertexAttribPointer(location, numComponents, GL_FLOAT, GL_FALSE, 0, 0);
}

void VertexBuffer::update(void* data, uint32_t size, uint32_t offset) {
	glBindBuffer(target, v);
	glBufferSubData(target, offset, size, data);
}

UniformBuffer::UniformBuffer(const void* data, uint32_t size) {
	this->target = GL_UNIFORM_BUFFER;
	this->stride = 0;
	glGenBuffers(1, &v);
	glBindBuffer(target, v);
	glBufferData(target, size, data, GL_STATIC_DRAW);
}

void UniformBuffer::bind(uint32_t binding) {
	glBindBufferBase(target, binding, v);
}

void UniformBuffer::update(const void* data, uint32_t size, uint32_t offset) {
	glBindBuffer(target, v);
	glBufferSubData(target, offset, size, data);
}

Framebuffer::Framebuffer(const std::vector<Texture*>& attachments, uint32_t w, uint32_t h, uint32_t layers) {
	this->target = GL_DRAW_FRAMEBUFFER;
	this->w = w; this->h = h;
	glGenFramebuffers(1, &v);
	glBindFramebuffer(target, v);
	auto& attachment = attachments[0];
	glFramebufferTexture2D(target, GL_COLOR_ATTACHMENT0, attachment->target, attachment->v, 0);
}

void Framebuffer::bind() {
	glBindFramebuffer(target, v);
}

void Framebuffer::unbind() {
	glBindFramebuffer(target, 0);
}

void Program::bind() {
	glUseProgram(v);
}

GLApplication::GLApplication(const std::string& appName) : appName(appName) {}

static void onError(int error, const char* desc) {
	ERR("%s", desc);
}
void GLApplication::init() {
	glfwInit();
	glfwSetErrorCallback(onError);
	graphicsContext = make_unique<GraphicsContext>(appName.c_str());
	window = make_unique<GLFWWindow>(graphicsContext.get(), appName.c_str(), [&](Window* thiz) {
		GLenum err = glewInit();
		if (err != GLEW_OK) ERR("%s", glewGetErrorString(err));
		LOG("GL Version: %s", glGetString(GL_VERSION));
		graphicsContext->onWindowCreated(thiz);
	});
	graphics = make_unique<Graphics>();
	window->onUpdate = std::bind(&GLApplication::onUpdate, this);
	window->onPaint = std::bind(&GLApplication::onPaint, this);
}

void GLApplication::run() {
	init();
	onInit();
	graphicsEngine::FPSCounter fpsCounter;
	while (!window->shouldClose()) {
		window->pollEvents();
		window->update();
		window->paint();
		fpsCounter.update();
	}
	close();
}

void GLApplication::close() {}

GraphicsProgram::GraphicsProgram(VertexShaderModule* vs, FragmentShaderModule* fs) {
	v = glCreateProgram();
	glAttachShader(v, vs->v);
	glAttachShader(v, fs->v);
	glLinkProgram(v);
	GLint linked;
	glGetProgramiv(v, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint infoLen = 0;
		GLchar infoLog[1024];
		glGetProgramInfoLog(v, sizeof(infoLog), &infoLen, infoLog);
		ERR("cannot link program: %s", infoLog);
	}
	for (auto& j : vs->uniformInfos) uniformInfos[j.first] = j.second;
	for (auto& j : fs->uniformInfos) uniformInfos[j.first] = j.second;
}

void GraphicsProgram::bind() {
	glUseProgram(v);
}