/*
 * Copyright 2019 GoPro Inc.
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
#include "pass_vk.h"

int ngli_pass_vk_init(struct pass *s, struct ngl_ctx *ctx, const struct pass_params *params) {
    return pass_init(s, ctx, params);
}
int ngli_pass_vk_prepare(struct pass *s) {
    return pass_prepare(s);
}
void ngli_pass_vk_uninit(struct pass *s) {
    return pass_uninit(s);
}
int ngli_pass_vk_update(struct pass *s, double t) {
    return pass_update(s, t);
}
int ngli_pass_vk_exec(struct pass *s) {
    return pass_exec(s);
}
