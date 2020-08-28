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

import array
import random
import pynodegl as ngl
from pynodegl_utils.misc import scene
from pynodegl_utils.toolbox.colors import COLORS
from pynodegl_utils.tests.debug import get_debug_points
from pynodegl_utils.tests.cmp_cuepoints import test_cuepoints
from pynodegl_utils.tests.cmp_fingerprint import test_fingerprint


_PARTICULES_COMPUTE = 'test_particules.comp'
_PARTICULES_VERT = 'test_particules.vert'
_PARTICULES_FRAG = 'test_particules.frag'


@test_fingerprint(nb_keyframes=10, tolerance=1)
@scene()
def compute_particules(cfg):
    random.seed(0)
    cfg.duration = 10
    local_size = 4
    nb_particules = 128

    shader_version = '310 es' if cfg.backend == 'gles' else '430'
    shader_data = dict(
        version=shader_version,
        local_size=local_size,
        nb_particules=nb_particules,
    )
    compute_shader = _PARTICULES_COMPUTE
    vertex_shader = _PARTICULES_VERT
    fragment_shader = _PARTICULES_FRAG

    positions = array.array('f')
    velocities = array.array('f')
    for i in range(nb_particules):
        positions.extend([
            random.uniform(-2.0, 1.0),
            random.uniform(-1.0, 1.0),
            0.0,
        ])
        velocities.extend([
            random.uniform(1.0, 2.0),
            random.uniform(0.5, 1.5),
        ])

    ipositions = ngl.Block(
        fields={
            'ipositions':ngl.BufferVec3(data=positions),
            'ivelocities':ngl.BufferVec2(data=velocities),
        },
        layout='std430',
    )
    opositions = ngl.Block(fields={'opositions':ngl.BufferVec3(count=nb_particules)}, layout='std430')

    animkf = [
        ngl.AnimKeyFrameFloat(0, 0),
        ngl.AnimKeyFrameFloat(cfg.duration, 1.0),
    ]
    time = ngl.AnimatedFloat(animkf)
    duration = ngl.UniformFloat(cfg.duration)

    program = ngl.ComputeProgram(compute_shader)
    compute = ngl.Compute(nb_particules / ( local_size * local_size), 1, 1, program)
    compute.update_uniforms(time=time, duration=duration)
    compute.update_blocks(ipositions_buffer=ipositions, opositions_buffer=opositions)

    circle = ngl.Circle(radius=0.05)
    program = ngl.Program(vertex=vertex_shader, fragment=fragment_shader)
    render = ngl.Render(circle, program, nb_instances=nb_particules)
    render.update_uniforms(color=ngl.UniformVec4(value=COLORS['sgreen']))
    render.update_blocks(positions_buffer=opositions)

    group = ngl.Group()
    group.add_children(compute, render)
    return group


_COMPUTE_HISTOGRAM_CLEAR = 'test_histogram_clear.comp'
_COMPUTE_HISTOGRAM_EXEC = 'test_histogram.comp'
_RENDER_HISTOGRAM_VERT = 'test_histogram.vert'
_RENDER_HISTOGRAM_FRAG = 'test_histogram.frag'

_N = 8


def _get_compute_histogram_cuepoints():
    f = float(_N)
    off = 1 / (2 * f)
    c = lambda i: (i / f + off) * 2.0 - 1.0
    return dict(('%d%d' % (x, y), (c(x), c(y))) for y in range(_N) for x in range(_N))


@test_cuepoints(points=_get_compute_histogram_cuepoints(), tolerance=1)
@scene(show_dbg_points=scene.Bool())
def compute_histogram(cfg, show_dbg_points=False):
    random.seed(0)
    cfg.duration = 10
    cfg.aspect_ratio = (1, 1)
    hsize, size, local_size = _N * _N, _N, _N // 2
    data = array.array('f')
    for i in range(size * size):
        data.extend((
            random.uniform(0.0, 0.5),
            random.uniform(0.25, 0.75),
            random.uniform(0.5, 1.0),
            1.0,
        ))
    texture_buffer = ngl.BufferVec4(data=data)
    texture = ngl.Texture2D(width=size, height=size, data_src=texture_buffer)

    histogram_block = ngl.Block(layout='std430', label='histogram')
    histogram_block.update_fields({
            'histr':ngl.BufferUInt(hsize),
            'histg':ngl.BufferUInt(hsize),
            'histb':ngl.BufferUInt(hsize),
            'max':ngl.UniformVec3()
        }
    )

    shader_version = '310 es' if cfg.backend == 'gles' else '430'
    shader_header = '#version %s\n' % shader_version
    if cfg.backend == 'gles' and cfg.system == 'Android':
        shader_header += '#extension GL_ANDROID_extension_pack_es31a: require\n'
    shader_params = dict(hsize=hsize, size=size, local_size=local_size)

    group_size = hsize // local_size
    clear_histogram_shader = _COMPUTE_HISTOGRAM_CLEAR
    clear_histogram_program = ngl.ComputeProgram(clear_histogram_shader)
    clear_histogram = ngl.Compute(
        group_size,
        1,
        1,
        clear_histogram_program,
        label='clear_histogram',
    )
    clear_histogram.update_blocks(histogram=histogram_block)

    group_size = size // local_size
    exec_histogram_shader = _COMPUTE_HISTOGRAM_EXEC
    exec_histogram_program = ngl.ComputeProgram(exec_histogram_shader)
    exec_histogram = ngl.Compute(
        group_size,
        group_size,
        1,
        exec_histogram_program,
        label='compute_histogram'
    )
    exec_histogram.update_blocks(histogram=histogram_block)
    exec_histogram.update_textures(source=texture)

    quad = ngl.Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    program = ngl.Program(
        vertex=_RENDER_HISTOGRAM_VERT,
        fragment=_RENDER_HISTOGRAM_FRAG,
    )
    render = ngl.Render(quad, program, label='render_histogram')
    render.update_blocks(histogram=histogram_block)

    group = ngl.Group(children=(clear_histogram, exec_histogram, render,))
    if show_dbg_points:
        cuepoints = _get_compute_histogram_cuepoints()
        group.add_children(get_debug_points(cfg, cuepoints))
    return group


_ANIMATION_COMPUTE = 'test_animation.comp'
_ANIMATION_VERT = 'test_animation.vert'
_ANIMATION_FRAG = 'test_animation.frag'


@test_fingerprint(nb_keyframes=5, tolerance=1)
@scene()
def compute_animation(cfg):
    cfg.duration = 5
    cfg.aspect_ratio = (1, 1)
    local_size = 2

    shader_version = '310 es' if cfg.backend == 'gles' else '430'
    shader_data = dict(
        version=shader_version,
        local_size=local_size,
    )
    compute_shader = _ANIMATION_COMPUTE
    vertex_shader = _ANIMATION_VERT
    fragment_shader = _ANIMATION_FRAG

    vertices_data = array.array('f', [
        -0.5,  0.5, 0.0,
        -0.5, -0.5, 0.0,
         0.5,  0.5, 0.0,
        0.5,  -0.5, 0.0,
    ])
    nb_vertices = 4

    input_vertices = ngl.BufferVec3(data=vertices_data)
    output_vertices = ngl.BufferVec3(data=vertices_data)
    input_block = ngl.Block(fields={'vertices':input_vertices}, layout='std140')
    output_block = ngl.Block(fields={'vertices':output_vertices}, layout='std430')

    rotate_animkf = [ngl.AnimKeyFrameFloat(0, 0),
                     ngl.AnimKeyFrameFloat(cfg.duration, 360)]
    rotate = ngl.Rotate(ngl.Identity(), axis=(0, 0, 1), anim=ngl.AnimatedFloat(rotate_animkf))
    transform = ngl.UniformMat4(transform=rotate)

    program = ngl.ComputeProgram(compute_shader)
    compute = ngl.Compute(nb_vertices / (local_size ** 2), 1, 1, program)
    compute.update_uniforms(transform=transform)
    compute.update_blocks(input_block=input_block, output_block=output_block)

    quad_buffer = ngl.BufferVec3(block=output_block, block_field='vertices')
    geometry = ngl.Geometry(quad_buffer, topology='triangle_strip')
    program = ngl.Program(vertex=vertex_shader, fragment=fragment_shader)
    render = ngl.Render(geometry, program)
    render.update_uniforms(color=ngl.UniformVec4(value=COLORS['sgreen']))

    return ngl.Group(children=(compute, render))
