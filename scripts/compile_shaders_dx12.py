from shader_tools import *

paths = ['ngfx/data/shaders', 'NodeGL/data/shaders', 'NodeGL/pynodegl-utils/pynodegl_utils/examples/shaders']
extensions=['.vert', '.frag', '.comp']
glslFiles = addFiles(paths, extensions)
outDir = 'cmake-build-debug'

defines = '-DGRAPHICS_BACKEND_DIRECTX12=1'
spvFiles = compileShaders(glslFiles, defines, outDir, 'glsl')
spvMapFiles = generateShaderMaps(glslFiles, outDir, 'glsl')
hlslFiles = convertShaders(spvFiles, outDir, 'hlsl')
hlsllibFiles = compileShaders(hlslFiles, defines, outDir, 'hlsl')
hlslMapFiles = generateShaderMaps(hlslFiles, outDir, 'hlsl')
