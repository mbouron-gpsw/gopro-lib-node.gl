. scripts/env.sh
$PYSHELL $NGL_TEST rtt.py rtt_feature_depth refs/rtt_feature_depth.ref 
$PYSHELL $NGL_TEST rtt.py rtt_feature_depth_stencil refs/rtt_feature_depth_stencil.ref 
$PYSHELL $NGL_TEST rtt.py rtt_mipmap refs/rtt_mipmap.ref 
$PYSHELL $NGL_TEST rtt.py rtt_texture_depth_d16_unorm refs/rtt_texture_depth_d16_unorm.ref 
$PYSHELL $NGL_TEST rtt.py rtt_texture_depth_d24_unorm refs/rtt_texture_depth_d24_unorm.ref 
$PYSHELL $NGL_TEST rtt.py rtt_texture_depth_d24_unorm_s8_uint refs/rtt_texture_depth_d24_unorm_s8_uint.ref 
$PYSHELL $NGL_TEST rtt.py rtt_feature_depth_msaa refs/rtt_feature_depth_msaa.ref 
$PYSHELL $NGL_TEST rtt.py rtt_feature_depth_stencil_msaa refs/rtt_feature_depth_stencil_msaa.ref 
$PYSHELL $NGL_TEST rtt.py rtt_texture_depth_d16_unorm_msaa refs/rtt_texture_depth_d16_unorm_msaa.ref 
$PYSHELL $NGL_TEST rtt.py rtt_texture_depth_d24_unorm_msaa refs/rtt_texture_depth_d24_unorm_msaa.ref 
$PYSHELL $NGL_TEST rtt.py rtt_texture_depth_d24_unorm_s8_uint_msaa refs/rtt_texture_depth_d24_unorm_s8_uint_msaa.ref 
