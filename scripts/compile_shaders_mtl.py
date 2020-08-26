#!/usr/bin/python3
from shader_tools import *

paths = ['data/shaders', 'NodeGL/data/shaders', 'NodeGL/pynodegl-utils/pynodegl_utils/examples/shaders']
extensions=['.vert', '.frag', '.comp']
glslFiles = addFiles(paths, extensions)
outDir = 'cmake-build-debug'

defines = '-DGRAPHICS_BACKEND_METAL=1'
spvFiles = compileShaders(glslFiles, defines, outDir, 'glsl')
spvMapFiles = generateShaderMaps(glslFiles, outDir, 'glsl')
metalFiles = convertShaders(spvFiles, outDir, 'msl')
metallibFiles = compileShaders(metalFiles, defines, outDir, 'msl')
metalMapFiles = generateShaderMaps(metalFiles, outDir, 'msl')
