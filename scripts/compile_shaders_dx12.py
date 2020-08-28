from shader_tools import *

def applyPatches(patchFiles, outDir):
    patch='patch.exe' if PLATFORM_WIN32 else 'patch'
    for patchFile in patchFiles:
        filename = os.path.basename(patchFile)[:-6]
        print('filename: ',filename)
        outFile = os.path.normpath(f"{outDir}/{filename}")
        if os.path.exists(outFile):
            print(f"applying patch: {patchFile}")
            cmdStr = f"{patch} -N -u {outFile} -i {patchFile}"
            cmd(cmdStr)
            
paths = ['ngfx/data/shaders', 'NodeGL/data/shaders', 'NodeGL/pynodegl-utils/pynodegl_utils/examples/shaders']
extensions=['.vert', '.frag', '.comp']
glslFiles = addFiles(paths, extensions)
outDir = 'cmake-build-debug'

defines = '-DGRAPHICS_BACKEND_DIRECTX12=1'
spvFiles = compileShaders(glslFiles, defines, outDir, 'glsl')
spvMapFiles = generateShaderMaps(glslFiles, outDir, 'glsl')
hlslFiles = convertShaders(spvFiles, outDir, 'hlsl')
patchFiles = glob.glob(f"patches/*.patch")
applyPatches(patchFiles, outDir)
hlsllibFiles = compileShaders(hlslFiles, defines, outDir, 'hlsl')
hlslMapFiles = generateShaderMaps(hlslFiles, outDir, 'hlsl')
