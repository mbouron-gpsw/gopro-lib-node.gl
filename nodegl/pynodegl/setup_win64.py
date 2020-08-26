from setuptools import Extension, setup
from Cython.Build import cythonize

extensions = [
	Extension("pynodegl", ["pynodegl.pyx"], 
		include_dirs=['../src','../porting/ngfx','../integ',f"../../ngfx", f"../../ngfx/src"],
		library_dirs=[f"../../cmake-build-debug/ngfx/Debug", f"../../cmake-build-debug/NodeGL/Debug"],
		define_macros=[('GRAPHICS_BACKEND_DIRECTX12', '1'), ('WINDOW_BACKEND_WINDOWS', '1')],
		libraries=['ngfx', 'NodeGL', 'd3d12','dxgi','d3dcompiler','kernel32','user32','gdi32','winspool','shell32','ole32','oleaut32','uuid','comdlg32','advapi32'],
		extra_compile_args=["-O0", "-std=c++17"]
	)
]

setup(
    ext_modules = cythonize(extensions, language_level=3)
)

