. scripts/env.sh
$PYSHELL $NGL_TEST transform.py transform_animated_camera refs/transform_animated_camera.ref 
$PYSHELL $NGL_TEST transform.py transform_matrix refs/transform_matrix.ref 
$PYSHELL $NGL_TEST transform.py transform_translate refs/transform_translate.ref 
$PYSHELL $NGL_TEST transform.py transform_translate_animated refs/transform_translate_animated.ref 
$PYSHELL $NGL_TEST transform.py transform_scale refs/transform_scale.ref 
$PYSHELL $NGL_TEST transform.py transform_scale_animated refs/transform_scale_animated.ref 
$PYSHELL $NGL_TEST transform.py transform_scale_anchor refs/transform_scale_anchor.ref 
$PYSHELL $NGL_TEST transform.py transform_scale_anchor_animated refs/transform_scale_anchor_animated.ref 
$PYSHELL $NGL_TEST transform.py transform_rotate refs/transform_rotate.ref 
$PYSHELL $NGL_TEST transform.py transform_rotate_anchor refs/transform_rotate_anchor.ref 
$PYSHELL $NGL_TEST transform.py transform_rotate_quat refs/transform_rotate_quat.ref 
$PYSHELL $NGL_TEST transform.py transform_rotate_quat_anchor refs/transform_rotate_quat_anchor.ref 
$PYSHELL $NGL_TEST transform.py transform_rotate_quat_animated refs/transform_rotate_quat_animated.ref
