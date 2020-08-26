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
#include "DebugUtil.h"
#include "File.h"
#include <fstream>
using namespace glGraphicsEngine;
using namespace std;

#define ITEM(s) { #s, s }
static map<string, Format> formatMap = {
    { "FORMAT_R32_SFLOAT", GL_R32F },
    { "FORMAT_R32G32_SFLOAT", GL_RG32F },
    { "FORMAT_R32G32B32_SFLOAT", GL_RGB32F },
    { "FORMAT_R32G32B32_SFLOAT", GL_RGBA32F },
    { "FORMAT_R8G8B8A8_UNORM", GL_RGBA8 }
};

static map<string, DescriptorType> descriptorTypeMap = {
    { "DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER", GL_TEXTURE_2D },
    { "DESCRIPTOR_TYPE_UNIFORM_BUFFER", GL_UNIFORM_BUFFER }
};

static void parseAttributes(ifstream& in, vector<VertexShaderModule::AttributeDescription>& attrs) {
    string token;
    uint32_t numAttributes;
    in >> token >> numAttributes;
    attrs.resize(numAttributes);
    for (uint32_t j = 0; j < numAttributes; j++) {
        auto& attr = attrs[j];
        string formatStr;
        in >> attr.semantic >> attr.location >> attr.binding >> formatStr >> attr.offset;
        attr.format = formatMap.at(formatStr);
    }
}

static void parseUniforms(ifstream& in, vector<ShaderModule::UniformDescription>& descs) {
    string token;
    uint32_t numDescriptors;
    in >> token >> numDescriptors;
    descs.resize(numDescriptors);
    for (uint32_t j = 0; j < numDescriptors; j++) {
        auto& desc = descs[j];
        string descriptorTypeStr;
        in >> desc.set >> descriptorTypeStr;
        desc.type = descriptorTypeMap.at(descriptorTypeStr);
    }
}

static void parseUniformInfos(ifstream& in, map<string, ShaderModule::UniformInfo>& uniformInfos) {
    string token;
    in >> token;
    if (token != "UNIFORMS_INFO") return;
    uint32_t numUniformInfos;
    in >> numUniformInfos;
    for (uint32_t j = 0; j < numUniformInfos; j++) {
        ShaderModule::UniformInfo uniformInfo;
        string uniformName;
        in >> uniformName >> uniformInfo.set >> uniformInfo.offset >> uniformInfo.size;
        uniformInfos[uniformName] = uniformInfo;
    }
}

ShaderModule::ShaderModule(const std::string& filename, uint32_t type) {
    //TODO: add shader module cache
    graphicsEngine::File file;
    file.read(filename);
    v = glCreateShader(type);
    const char* src = file.data.get();
    glShaderSource(v, 1, &src, &file.size);
    LOG("compiling shader: %s", filename.c_str());
    glCompileShader(v);
    GLint compiled;
    glGetShaderiv(v, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(v, GL_INFO_LOG_LENGTH, &infoLen);
        string infoLog;
        if (infoLen > 0) {
            infoLog.resize(infoLen);
            glGetShaderInfoLog(v, infoLen, NULL, &infoLog[0]);
        }
        ERR("cannot compile: %s\n%s", filename.c_str(), infoLog.c_str());
    }
}

VertexShaderModule::VertexShaderModule(const std::string& filename)
	    : ShaderModule(filename, GL_VERTEX_SHADER) {
    initBindings(filename + ".map");
}

void VertexShaderModule::initBindings(const std::string& filename) {
	ifstream in(filename);
	if (!in.is_open()) ERR("cannot open file: %s", filename.c_str());
	string token;
	parseAttributes(in, attributes);
	parseUniforms(in, uniforms);
	parseUniformInfos(in, uniformInfos);
	in.close();
}

FragmentShaderModule::FragmentShaderModule(const std::string& filename)
	    : ShaderModule(filename, GL_FRAGMENT_SHADER) {
    initBindings(filename+".map");
}

void FragmentShaderModule::initBindings(const std::string& filename) {
	ifstream in(filename);
	if (!in.is_open()) ERR("cannot open file: %s", filename.c_str());
	parseUniforms(in, uniforms);
	parseUniformInfos(in, uniformInfos);
	in.close();
}