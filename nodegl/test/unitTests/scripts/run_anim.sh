. scripts/env.sh
$PYSHELL $NGL_TEST anim.py anim_forward_api refs/anim_forward_api.ref 
$PYSHELL $NGL_TEST anim.py anim_forward_float refs/anim_forward_float.ref 
$PYSHELL $NGL_TEST anim.py anim_forward_vec2 refs/anim_forward_vec2.ref 
$PYSHELL $NGL_TEST anim.py anim_forward_vec3 refs/anim_forward_vec3.ref 
$PYSHELL $NGL_TEST anim.py anim_forward_vec4 refs/anim_forward_vec4.ref 
$PYSHELL $NGL_TEST anim.py anim_forward_quat refs/anim_forward_quat.ref 
$PYSHELL $NGL_TEST anim.py anim_resolution_api refs/anim_resolution_api.ref 
