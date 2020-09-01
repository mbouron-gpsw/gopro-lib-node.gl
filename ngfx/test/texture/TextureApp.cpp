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
#include "TextureApp.h"
#include "graphics/ShaderModule.h"
#include <memory>
using namespace ngfx;
using namespace glm;
using namespace std;

TextureApp::TextureApp(): Application("Texture")  {}

void TextureApp::onInit() {
    texture.reset(Texture::create(graphicsContext.get(), graphics.get(), "GP0056_BACK.JPG"));
    drawTextureOp.reset(new DrawTextureOp(graphicsContext.get(), texture.get()));
}

void TextureApp::onRecordCommandBuffer(CommandBuffer* commandBuffer) {
    graphicsContext->beginRenderPass(commandBuffer, graphics.get());
    drawTextureOp->draw(commandBuffer, graphics.get());
    graphicsContext->endRenderPass(commandBuffer, graphics.get());
}

int main() {
    TextureApp app;
    app.run();
    return 0;
}