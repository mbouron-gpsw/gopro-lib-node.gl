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
#include "MediaPlayer.h"
#include "DebugUtil.h"
#include <string>
using namespace ngfx;

static void sxplayer_log_cb(void *arg, int level, const char *filename, int ln,
                                  const char *fn, const char *fmt, va_list vl) {
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, vl);
    LOG("[SXPLAYER %s:%d %s] %s", filename, ln, fn, buf);
}

MediaPlayer::MediaPlayer(const std::string& filename) {
    ctx = sxplayer_create(filename.c_str());
    sxplayer_set_log_callback(ctx, nullptr, sxplayer_log_cb);
    sxplayer_get_info(ctx, &info);
}

void MediaPlayer::start() {
    sxplayer_start(ctx);
}

MediaPlayer::Frame* MediaPlayer::getFrame(double time) {
    Frame* frame = new Frame();
    frame->v = sxplayer_get_frame(ctx, time);
    return frame;
}

MediaPlayer::~MediaPlayer() {
    sxplayer_free(&ctx);
}
