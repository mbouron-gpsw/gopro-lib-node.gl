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
#include "MediaPlayerApp.h"
#include "graphics/ShaderModule.h"
#include <memory>
using namespace ngfx;
using namespace glm;
using namespace std;

MediaPlayerApp::MediaPlayerApp(): Application("Texture")  {}

static std::map<int, ngfx::PixelFormat> pixFmtMap = {
    { SXPLAYER_PIXFMT_RGBA, PIXELFORMAT_RGBA8_UNORM },
    { SXPLAYER_PIXFMT_BGRA, PIXELFORMAT_BGRA8_UNORM }
};

void MediaPlayerApp::onInit() {
    const char* filename = "ngl-media-test.nut";
    mediaPlayer.reset(new MediaPlayer(filename));
    uint32_t w = mediaPlayer->info.width, h = mediaPlayer->info.height, size = w * h * 4;
    texture.reset(Texture::create(graphicsContext.get(), graphics.get(),
        nullptr, PIXELFORMAT_BGRA8_UNORM, size, w, h, 1, 1));
    drawTextureOp.reset(new DrawTextureOp(graphicsContext.get(), texture.get()));
}

void MediaPlayerApp::onRecordCommandBuffer(CommandBuffer* commandBuffer) {
    graphicsContext->beginRenderPass(commandBuffer, graphics.get());
    drawTextureOp->draw(commandBuffer, graphics.get());
    graphicsContext->endRenderPass(commandBuffer, graphics.get());
}

void MediaPlayerApp::onUpdate() {
    timer.update();
    time += timer.elapsed;
    std::unique_ptr<MediaPlayer::Frame> frame(mediaPlayer->getFrame(time));
    if (frame->v) texture->upload(frame->v->data, frame->v->linesize * frame->v->height);
}

int main() {
    MediaPlayerApp app;
    app.run();
    return 0;
}
