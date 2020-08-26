. scripts/env.sh
$PYSHELL $NGL_TEST blending.py blending_all_diamond refs/blending_all_diamond.ref 
$PYSHELL $NGL_TEST blending.py blending_all_timed_diamond refs/blending_all_timed_diamond.ref 
$PYSHELL $NGL_TEST blending.py blending_none refs/blending_none.ref 
$PYSHELL $NGL_TEST blending.py blending_multiply refs/blending_multiply.ref 
$PYSHELL $NGL_TEST blending.py blending_screen refs/blending_screen.ref 
$PYSHELL $NGL_TEST blending.py blending_darken refs/blending_darken.ref 
$PYSHELL $NGL_TEST blending.py blending_lighten refs/blending_lighten.ref 
