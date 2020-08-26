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
#include "StringUtil.h"
#include <cstring>
#include <memory>
using namespace NGL;
using namespace std;

void StringUtil::split(string str, string delim, vector<string> &r) {
    unique_ptr<char> buffer;
    buffer.reset(strdup(str.c_str()));
    char* token = strtok(buffer.get(), delim.c_str());
    while (token) {
        r.push_back(token);
        token = strtok(nullptr, delim.c_str());
    }
}
