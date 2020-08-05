/*
 * Copyright 2020 GoPro Inc.
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

#include <dlfcn.h>
#include <jni.h>
#include <stdlib.h>

#include "android_api.h"
#include "log.h"

#define NDK_LOAD_FUNC(handle, name) do {   \
    s->name = dlsym(handle, #name);        \
    if (!s->name) {                        \
        ret = NGL_ERROR_UNSUPPORTED;       \
        LOG(INFO, "missing #name symbol"); \
        goto done;                         \
    }                                      \
} while (0)

static int load_media_api(struct android_api *s)
{
    int ret = 0;

    void *media = dlopen("libmediandk.so", RTLD_NOW);
    if (!media) {
        LOG(ERROR, "could not open libmediandk.so");
        return NGL_ERROR_UNSUPPORTED;
    }

    NDK_LOAD_FUNC(media, AImage_delete);
    NDK_LOAD_FUNC(media, AImage_getHardwareBuffer);
    NDK_LOAD_FUNC(media, AImageReader_new);
    NDK_LOAD_FUNC(media, AImageReader_getWindow);
    NDK_LOAD_FUNC(media, AImageReader_acquireNextImage);
    NDK_LOAD_FUNC(media, AImageReader_setImageListener);
    NDK_LOAD_FUNC(media, AImageReader_delete);

done:
    dlclose(media);
    return ret;
}

static int load_surface_api(struct android_api *s)
{
    int ret = 0;

    void *android = dlopen("libandroid.so", RTLD_NOW);
    if (!android) {
        LOG(ERROR, "could not open libandroid.so");
        return NGL_ERROR_UNSUPPORTED;
    }

    NDK_LOAD_FUNC(android, ANativeWindow_toSurface);

done:
    dlclose(android);
    return ret;
}

int ngli_android_api_init(struct android_api *s)
{
    if (load_media_api(s) < 0 || load_surface_api(s) < 0)
        s->has_native_imagereader_api = 0;
    else
        s->has_native_imagereader_api = 1;

    return 0;
}
