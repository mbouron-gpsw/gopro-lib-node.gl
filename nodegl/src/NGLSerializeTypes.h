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

#pragma once

#define NODE_TYPES \
    TYPE_FN(ComputeProgram) TYPE_FN(GraphicsProgram) TYPE_FN(GraphicsConfig) \
    TYPE_FN(Camera) TYPE_FN(Buffer) TYPE_FN(StreamedBuffer) \
    TYPE_FN(Geometry) TYPE_FN(Circle) TYPE_FN(Triangle) TYPE_FN(Quad) \
    TYPE_FN(Identity) TYPE_FN(Rotate) TYPE_FN(RotateQuat) TYPE_FN(Scale) TYPE_FN(Translate) TYPE_FN(Transform) \
    TYPE_FN(TimeRangeFilter) TYPE_FN(TimeRangeModeCont) TYPE_FN(TimeRangeModeNoOp) TYPE_FN(TimeRangeModeOnce) \
    TYPE_FN(RenderToTexture) TYPE_FN(Media) \
    TYPE_FN(Compute) TYPE_FN(Render) \
    TYPE_FN(Texture2D) TYPE_FN(Texture3D) TYPE_FN(TextureCube) \
    TYPE_FN(Block) TYPE_FN(UniformInt) TYPE_FN(UniformFloat) TYPE_FN(UniformVec2) TYPE_FN(UniformVec3) \
    TYPE_FN(UniformVec4) TYPE_FN(UniformMat4) TYPE_FN(UniformQuat) \
    TYPE_FN(AnimatedBuffer) TYPE_FN(AnimKeyFrameBuffer) \
    TYPE_FN(AnimatedTime) TYPE_FN(AnimatedFloat) TYPE_FN(AnimKeyFrameFloat) TYPE_FN(AnimatedVec2) TYPE_FN(AnimKeyFrameVec2) \
    TYPE_FN(AnimatedVec3) TYPE_FN(AnimKeyFrameVec3) TYPE_FN(AnimatedVec4) TYPE_FN(AnimKeyFrameVec4) TYPE_FN(AnimatedQuat) TYPE_FN(AnimKeyFrameQuat) \
    TYPE_FN(Group) TYPE_FN(Text)

#define TYPE_FN(s) s##Type,
namespace NGL {
    enum Type { NODE_TYPES };
}
#undef TYPE_FN
