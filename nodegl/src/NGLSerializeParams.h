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

#define BufferParams PARAM(dataType); PARAM(stride); PARAM_DATA(data); PARAM(count); PARAM_NODE(block, Block); PARAM_STR(blockField)
#define StreamedBufferParams PARAM(dataType); PARAM(stride); PARAM(count); PARAM_NODE(timestamps, Buffer); PARAM_NODE(buffer, Buffer); \
    PARAM(timeBase); PARAM_NODE(timeAnim, AnimatedTime)
#define GraphicsProgramParams PARAM_STR(vertexShaderFile); PARAM_STR(fragmentShaderFile)
#define ComputeProgramParams PARAM_STR(computeShaderFile)
#define GraphicsConfigParams PARAM_NODE(child, Node); PARAM(blend); PARAM(blendSrcFactor); PARAM(blendDstFactor); \
    PARAM(blendOp); PARAM(colorWriteMask); PARAM(scissorTest); PARAM(scissorRect); PARAM(depthTest)
#define GeometryParams PARAM_NODE(verts, Node); PARAM_NODE(normals, Node); PARAM_NODE(uvCoords, Node); \
    PARAM_NODE(indices, Node); PARAM(topology)
#define TriangleParams PARAM_VECTOR(edges, vec3); PARAM_VECTOR(uvEdges, vec2)
#define QuadParams PARAM(corner); PARAM(width); PARAM(height); PARAM(uv_corner); PARAM(uv_width); PARAM(uv_height)
#define IdentityParams
#define TransformParams PARAM_NODE(child, Node); PARAM(mat)
#define CameraParams PARAM_NODE(child, Node); PARAM(eye); PARAM(center); PARAM(up); \
    PARAM(eyeTransform); PARAM(centerTransform); PARAM(upTransform); \
    PARAM(perspectiveProj); PARAM(perspectiveParams); PARAM(orthographicParams); PARAM(clippingParams); PARAM_NODE(fovAnim, AnimatedFloat)
#define RotateParams PARAM_NODE(child, Node); PARAM(axis); PARAM(anchor); PARAM(angle); PARAM_NODE(anim, AnimatedFloat)
#define RotateQuatParams PARAM_NODE(child, Node); PARAM(v); PARAM(anchor); PARAM_NODE(anim, AnimatedQuat)
#define ScaleParams PARAM_NODE(child, Node); PARAM(v); PARAM(anchor); PARAM_NODE(anim, AnimatedVec3)
#define TranslateParams PARAM_NODE(child, Node); PARAM(v); PARAM_NODE(anim, AnimatedVec3)
#define TimeRangeFilterParams PARAM_NODE(child, Node); PARAM_NODE_VECTOR(ranges, TimeRangeMode)
#define TimeRangeModeParams PARAM(startTime)
#define TimeRangeModeContParams TimeRangeModeParams
#define TimeRangeModeNoOpParams TimeRangeModeParams
#define TimeRangeModeOnceParams TimeRangeModeParams; PARAM(renderTime)
#define CircleParams PARAM(radius); PARAM(numPoints)
#define BlockParams PARAM_NODE_MAP(fields, Node); PARAM_STR(layout)
#define MediaParams PARAM_STR(filename); PARAM(width); PARAM(height)
#define GroupParams PARAM_NODE_VECTOR(children, Node);

#define AnimKeyFrameBufferParams PARAM(easingId); PARAM(t); PARAM_DATA(v)
#define AnimatedBufferParams PARAM_NODE_VECTOR(kf, AnimKeyFrameBuffer);  PARAM(dataType); PARAM(stride)
#define AnimKeyFrameFloatParams PARAM(easingId); PARAM(t); PARAM(v)
#define AnimatedFloatParams PARAM_NODE_VECTOR(kf, AnimKeyFrameFloat)
#define AnimatedTimeParams PARAM_NODE_VECTOR(kf, AnimKeyFrameFloat)
#define AnimKeyFrameVec2Params PARAM(easingId); PARAM(t); PARAM(v)
#define AnimatedVec2Params PARAM_NODE_VECTOR(kf, AnimKeyFrameVec2)
#define AnimKeyFrameVec3Params PARAM(easingId); PARAM(t); PARAM(v)
#define AnimatedVec3Params PARAM_NODE_VECTOR(kf, AnimKeyFrameVec3)
#define AnimKeyFrameVec4Params PARAM(easingId); PARAM(t); PARAM(v)
#define AnimatedVec4Params PARAM_NODE_VECTOR(kf, AnimKeyFrameVec4)
#define AnimKeyFrameQuatParams PARAM(easingId); PARAM(t); PARAM(v)
#define AnimatedQuatParams PARAM_NODE_VECTOR(kf, AnimKeyFrameQuat); PARAM(valueMat4); PARAM(asMat4)

#define UniformFloatParams PARAM(value)
#define UniformIntParams PARAM(value)
#define UniformVec2Params PARAM(value)
#define UniformVec3Params PARAM(value)
#define UniformVec4Params PARAM(value)
#define UniformMat4Params PARAM(value); PARAM_NODE(transform, Transform)
#define UniformQuatParams PARAM(value); PARAM(valueMat4); PARAM(asMat4)

#define TextureParams PARAM(pixelFormat); PARAM(usageFlags); PARAM_NODE(dataSrc, Node); \
    PARAM(w); PARAM(h); PARAM(d); PARAM(numLayers); \
    PARAM(minFilter); PARAM(magFilter); PARAM(wrapS); PARAM(wrapT); PARAM(wrapR); PARAM(mipmapFilter)
#define Texture2DParams TextureParams
#define Texture3DParams TextureParams
#define TextureCubeParams TextureParams
#define RenderParams PARAM_NODE(geom, Geometry); PARAM_NODE(program, GraphicsProgram); \
    PARAM_NODE_MAP(textures, Texture); PARAM_NODE_MAP(uniforms, Node); PARAM_NODE_MAP(blocks, Block); \
    PARAM_NODE_MAP(instanceAttributes, Buffer); PARAM(numInstances)
#define ComputeParams PARAM(groupCountX); PARAM(groupCountY); PARAM(groupCountZ); \
    PARAM(threadsPerGroupX); PARAM(threadsPerGroupY); PARAM(threadsPerGroupZ); \
    PARAM_NODE(program, ComputeProgram); \
    PARAM_NODE_MAP(textures, Texture); PARAM_NODE_MAP(uniforms, Node); PARAM_NODE_MAP(blocks, Block)
#define RenderToTextureParams PARAM_NODE(child, Node); PARAM_NODE_VECTOR(outputTextures, Texture); \
    PARAM_NODE(depthTexture, Texture2D); PARAM(setClearColor); PARAM(clearColor); PARAM(numSamples); PARAM(flags)

#define TextParams PARAM_STR(str); PARAM(fgColor); PARAM(bgColor); PARAM(padding); PARAM(fontScale); \
    PARAM(boxCorner); PARAM(boxWidthVec); PARAM(boxHeightVec); PARAM(aspectRatio); PARAM(hAlign); PARAM(vAlign)

