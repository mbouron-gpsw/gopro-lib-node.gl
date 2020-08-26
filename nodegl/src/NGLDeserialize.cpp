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

#include "NGLBackend.h"
#include "NGLSerializeTypes.h"
#include "NGLSerializeParams.h"
#include "DebugUtil.h"
#include <fstream>
#include <set>
#include <glm/gtx/string_cast.hpp>
using namespace NGL;

#define PARAM(v) in.read((char*)&v, sizeof(v))
#define PARAM_STR(v) { uint32_t strSize = 0; PARAM(strSize); v.resize(strSize); in.read(&v[0], strSize); }
#define PARAM_NODE(node, nodeType) { \
    bool nodeNotNull; PARAM(nodeNotNull); \
    if (nodeNotNull) node = sp_cast<nodeType>(deserializeNode(in)); \
}
#define PARAM_DATA(data) { \
    bool dataNotNull; \
    PARAM(dataNotNull); \
    if (dataNotNull) { \
        data = make_shared<Data>(); \
        in.read((char*)&data->size, sizeof(data->size)); \
        if (data->size) { \
            data->v = malloc(data->size); \
            in.read((char*)data->v, data->size); \
        } \
    } \
}
#define PARAM_NODE_MAP(nodes, t0) { \
    uint32_t numNodes; \
    PARAM(numNodes); \
    for (uint32_t j = 0; j < numNodes; j++) { \
        string key; \
        bool nodeNotNull = false; \
        PARAM_STR(key); \
        PARAM(nodeNotNull); \
        if (nodeNotNull) { \
            nodes[key] = sp_cast<NGL::t0>(deserializeNode(in)); \
        } \
    } \
}
#define PARAM_VECTOR(v, elementType) { \
    uint32_t count = 0; \
    PARAM(count); \
    v.resize(count); \
    in.read((char*)v.data(), v.size() * sizeof(v[0])); \
}
#define PARAM_NODE_VECTOR(nodes, t0) { \
    uint32_t numNodes = 0; \
    PARAM(numNodes); \
    nodes.resize(numNodes); \
    for (auto& node : nodes) node = sp_cast<NGL::t0>(deserializeNode(in)); \
}

static map<uint64_t, sp<Node>> deserializeCache;
template <typename T> static sp<T> deserializeNodeWithCache(std::istream& in) {
    auto& cache = deserializeCache;
    uint64_t thiz;
    PARAM(thiz);
    auto it = cache.find(thiz);
    if (it != cache.end()) return sp_cast<T>(it->second);
    auto node = make_shared<T>();
    PARAM_STR(node->label);
    node->deserialize(in);
    cache[thiz] = node;
    return node;
}

#define TYPE_FN(type) case NGL::type##Type: return deserializeNodeWithCache<NGL::type>(in); break;
static sp<Node> deserializeNode(std::istream& in) {
    Type type;
    PARAM(type);
    switch (type) {
        NODE_TYPES

    default: ERR("deserializeNode: unknown type"); break;
    };
}
#undef TYPE_FN

#define TYPE_FN(c0) \
void NGL::c0::deserialize(std::istream &in) { \
    c0##Params; \
}

NODE_TYPES

sp<Node> Deserialize::run() {
    std::ifstream in(filename.c_str(), std::ifstream::binary);
    if (!in.is_open()) ERR("cannot open file: %s", filename.c_str());
    auto scene = sp<Node>(deserializeNode(in));
    deserializeCache.clear();
    return scene;
}
