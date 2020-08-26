#!/bin/bash
CP=/usr/local/Cellar/coreutils/8.32/libexec/gnubin/cp
sh scripts/uninstall.sh
copyHeaders() {
	src_dir=$1
	dst_dir=$2
	cd $src_dir
	mkdir -p /usr/local/include/gfx_al/$dst_dir
	find . -name '*.h' | xargs -I {} $CP --parents {} /usr/local/include/gfx_al/$dst_dir
	cd -
}

$CP ./cmake-build-debug/Debug/libngfx.a /usr/local/lib/libgfx_al.a
mkdir -p /usr/local/include/gfx_al
copyHeaders src .
copyHeaders porting porting
