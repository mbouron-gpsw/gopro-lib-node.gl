from setuptools import Extension, setup
from Cython.Build import cythonize


extensions = [
	Extension("pynodegl", ["pynodegl.pyx"], 
		include_dirs=['../src','../porting/ngfx','../integ',f"../../ngfx", f"../../ngfx/src", '/usr/include/glm'],
		library_dirs=[f"../../cmake-build-debug/ngfx/Debug", f"../../cmake-build-debug/nodegl/Debug"],
		define_macros=[('GRAPHICS_BACKEND_METAL', '1'), ('WINDOW_BACKEND_APPKIT', '1')],
		libraries=['ngfx', 'NodeGL'],
		extra_compile_args=["-O0", "-std=c++17"] #debug build #"-fsanitize=address"
	)
]

setup(
    ext_modules = cythonize(extensions, language_level=3)
)
