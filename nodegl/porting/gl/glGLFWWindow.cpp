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
#include "glGLFWWindow.h"
#include "GLDebugUtil.h"
using namespace glGraphicsEngine;

GLFWWindow::GLFWWindow(GraphicsContext* graphicsContext, const char* title, std::function<void(Window* thiz)> onWindowCreated, int w, int h) 
        : Window(graphicsContext, title, onWindowCreated, w, h) {
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef ENABLE_GL_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
    auto monitor = glfwGetPrimaryMonitor();
    auto videoMode = glfwGetVideoMode(monitor);
    if (w == Window::DISPLAY_WIDTH) w = videoMode->width;
    if (h == Window::DISPLAY_HEIGHT) h = videoMode->height;
    v = glfwCreateWindow(w, h, title, nullptr, nullptr);
    glfwGetFramebufferSize(v, &w, &h);
    this->w = w; this->h = h;
    glfwMakeContextCurrent(v);
    glfwSwapInterval(1);
    onWindowCreated(this);
}

GLFWWindow::~GLFWWindow() {}

bool GLFWWindow::shouldClose() {
    return glfwWindowShouldClose(v);
}
void GLFWWindow::pollEvents() {
    glfwPollEvents();
}

void GLFWWindow::swapBuffers() {
    glfwSwapBuffers(v);
}
