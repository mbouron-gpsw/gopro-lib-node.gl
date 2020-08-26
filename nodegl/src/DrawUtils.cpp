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

#include "DrawUtils.h"
#include <glm/glm.hpp>
#include "FontData.h"
using namespace NGL;
using namespace glm;

static inline void setColor(uint8_t *p, uint32_t rgba)
{
    p[0] = rgba >> 24;
    p[1] = rgba >> 16 & 0xff;
    p[2] = rgba >>  8 & 0xff;
    p[3] = rgba       & 0xff;
}

void Canvas::drawRect(Rect rect, uint32_t color) {
    uint8_t *dst = data + (rect.y * w + rect.x) * bpp;
    uint32_t stride = w * bpp;
    for (int y = 0; y < rect.h; y++) {
        for (int x = 0; x < rect.w; x++) {
            setColor(dst + x * bpp, color);
        }
        dst += stride;
    }
}

void Canvas::drawText(int x, int y, const std::string &str, uint32_t color) {
    int px = 0, py = 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\n') {
            py++;
            px = 0;
            continue;
        }
        for (int char_y = 0; char_y < FONT_H; char_y++) {
            for (int char_x = 0; char_x < FONT_W; char_x++) {
                const int pix_x = x + px * FONT_W + char_x;
                const int pix_y = y + py * FONT_H + char_y;
                if (pix_x < 0 || pix_y < 0 || pix_x >= w || pix_y >= h)
                    continue;
                uint8_t *p =  data + (pix_y * w + pix_x) * bpp;
                if (FONT8_DATA[str[i] & 0x7f][char_y] & (1 << char_x))
                    setColor(p, color);
            }
        }
        px++;
    }
}

