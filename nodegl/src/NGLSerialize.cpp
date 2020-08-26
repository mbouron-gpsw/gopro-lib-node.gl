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
#include "NGLSerializeTypes.h"
#include "NGLSerializeParams.h"
#include "DebugUtil.h"
#include <fstream>
#include <set>
#include <glm/gtx/string_cast.hpp>
using namespace NGL;

#define PARAM(v) { out.write((char*)&v, sizeof(v)); }
#define PARAM_STR(v) { uint32_t strSize = uint32_t(v.size()); PARAM(strSize); out.write(&v[0], strSize); }
#define PARAM_ID() if (serializeNodeIDWithCache(out, this)) return;
#define PARAM_NODE(node, nodeType) { \
    bool nodeNotNull = (node != nullptr); \
    PARAM(nodeNotNull); \
    if (node) node->serialize(out); \
}
#define PARAM_DATA(data) { \
    bool dataNotNull = (data != nullptr); \
    PARAM(dataNotNull); \
    if (dataNotNull) { \
        out.write((char*)&data->size, sizeof(data->size)); \
        if (data->v) out.write((char*)data->v, data->size); \
    } \
}
#define PARAM_HEADER(t0) Type type = t0; PARAM(type); PARAM_ID(); PARAM_STR(label)
#define PARAM_VECTOR(v, elementType) { \
    uint32_t count = v.size(); \
    out.write((char*)&count, sizeof(count)); \
    out.write((char*)v.data(), v.size() * sizeof(v[0])); \
}
#define PARAM_NODE_MAP(nodes, nodeType) { \
    uint32_t numNodes = uint32_t(nodes.size()); \
    PARAM(numNodes); \
    for (auto it = nodes.begin(); it != nodes.end(); it++) { \
        PARAM_STR(it->first); \
        PARAM_NODE(it->second, nodeType); \
    } \
}
#define PARAM_NODE_VECTOR(nodes, nodeType) { \
    uint32_t numNodes = uint32_t(nodes.size()); \
    PARAM(numNodes); \
    for (auto& node : nodes) node->serialize(out); \
}


bool serializeNodeIDWithCache(std::ostream& out, void* thiz) {
    static set<uint64_t> cache;
    uint64_t id = (uint64_t)thiz;
    PARAM(id);
    if (cache.find(id) != cache.end()) return true;
    cache.insert(id);
    return false;
}

#define TYPE_FN(c0) \
void c0::serialize(std::ostream &out) { \
    PARAM_HEADER(c0##Type); \
    c0##Params; \
}

NODE_TYPES

void Serialize::run() {
    std::ofstream out(filename.c_str(), std::ofstream::binary);
    if (!out.is_open()) ERR("cannot open file: %s", filename.c_str());
    LOG("serialize to %s", filename.c_str());
	scene->serialize(out);
}
