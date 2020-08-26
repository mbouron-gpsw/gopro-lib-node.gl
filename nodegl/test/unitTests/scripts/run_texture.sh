. scripts/env.sh
$PYSHELL $NGL_TEST texture.py texture_3d refs/texture_3d.ref 
$PYSHELL $NGL_TEST texture.py texture_clear_and_scissor refs/texture_clear_and_scissor.ref 
$PYSHELL $NGL_TEST texture.py texture_cubemap refs/texture_cubemap.ref 
$PYSHELL $NGL_TEST texture.py texture_cubemap_from_mrt refs/texture_cubemap_from_mrt.ref 
$PYSHELL $NGL_TEST texture.py texture_data refs/texture_data.ref 
$PYSHELL $NGL_TEST texture.py texture_data_animated refs/texture_data_animated.ref 
$PYSHELL $NGL_TEST texture.py texture_data_unaligned_row refs/texture_data_unaligned_row.ref 
$PYSHELL $NGL_TEST texture.py texture_mipmap refs/texture_mipmap.ref 
$PYSHELL $NGL_TEST texture.py texture_scissor refs/texture_scissor.ref 
$PYSHELL $NGL_TEST texture.py texture_cubemap_from_mrt_msaa refs/texture_cubemap_from_mrt_msaa.ref 
