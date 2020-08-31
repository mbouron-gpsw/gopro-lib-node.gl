#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2020 GoPro Inc.
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
import pynodegl as ngl
from shader_tools import *
import hashlib
import os

GRAPHICS_BACKEND=os.getenv("GRAPHICS_BACKEND")
NGFX_DIR= os.path.abspath('../../..')
TMP_DIR = os.getenv("TEMP")

def compile_program(vertex, fragment):
    vertexKey = hashlib.md5(bytes(vertex, 'utf-8')).hexdigest()
    fragmentKey = hashlib.md5(bytes(fragment, 'utf-8')).hexdigest()
    vertexFile = 'tmp_'+vertexKey+'.vert'
    fragmentFile = 'tmp_'+fragmentKey+'.frag'
    outDir =  f"{NGFX_DIR}/cmake-build-debug"
    if GRAPHICS_BACKEND == 'vulkan':
        defines='-DGRAPHICS_BACKEND_VULKAN=1'
    elif GRAPHICS_BACKEND == 'metal':
        defines='-DGRAPHICS_BACKEND_METAL=1'
    elif GRAPHICS_BACKEND == 'direct3d12':
        defines='-DGRAPHICS_BACKEND_DIRECT3D12'
    else:
        print('ERROR: GRAPHICS_BACKEND environment variable not set')
        return 1
    if not TMP_DIR:
        print('ERROR: TEMP environment variable not set')
        return 1
    if not (os.path.exists(f"{TMP_DIR}/{vertexFile}") and os.path.exists(f"{TMP_DIR}/{fragmentFile}")):
        outFile = open(f"{TMP_DIR}/{vertexFile}", 'w')
        outFile.write(vertex)
        outFile.close()
        outFile = open(f"{TMP_DIR}/{fragmentFile}", 'w')
        outFile.write(fragment)
        outFile.close()
    cur_dir = os.getcwd()
    os.chdir(NGFX_DIR)
    glslFiles=[f"{TMP_DIR}/{vertexFile}", f"{TMP_DIR}/{fragmentFile}"]
    spvFiles = compileShaders(glslFiles, defines, outDir, 'glsl')
    spvMapFiles = generateShaderMaps(glslFiles, outDir, 'glsl')
    if GRAPHICS_BACKEND == 'metal':
        metalFiles = convertShaders(spvFiles, outDir, 'msl')
        metallibFiles = compileShaders(metalFiles, defines, outDir, 'msl')
        metalMapFiles = generateShaderMaps(metalFiles, outDir, 'msl')
    elif GRAPHICS_BACKEND == 'direct3d12':
        hlslFiles = convertShaders(spvFiles, outDir, 'hlsl')
        hlsllibFiles = compileShaders(hlslFiles, defines, outDir, 'hlsl')
        hlslMapFiles = generateShaderMaps(hlslFiles, outDir, 'hlsl')
    os.chdir(cur_dir)
    print(f"ngl.Program {vertexFile} {fragmentFile}")
    return ngl.Program(f"{vertexFile}", f"{fragmentFile}")
