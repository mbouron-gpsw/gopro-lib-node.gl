#!/bin/bash
set -e
. ./scripts/env_win64.sh

sh scripts/uninstall_win64.sh
copyHeaders() {
	src_dir=$1
	dst_dir=$2
	cd $src_dir
	mkdir -p $GFX_AL_INSTALL_DIR/include/gfx_al/$dst_dir
	find . -name '*.h' | xargs -I {} cp --parents {} $GFX_AL_INSTALL_DIR/include/gfx_al/$dst_dir
	cd -
}

cp ./cmake-build-debug/Debug/ngfx.lib $GFX_AL_INSTALL_DIR/lib/gfx_al.lib
mkdir -p $GFX_AL_INSTALL_DIR/include/gfx_al
copyHeaders src .
copyHeaders porting porting
