#!/bin/bash
sh scripts/uninstall.sh
copyHeaders() {
	src_dir=$1
	dst_dir=$2
	cd $src_dir
	mkdir -p /usr/local/include/ngfx/$dst_dir
	find . -name '*.h' | xargs -I {} cp --parents {} /usr/local/include/ngfx/$dst_dir
	cd -
}

cp ./cmake-build-debug/libngfx.so /usr/local/lib/.
mkdir -p /usr/local/include/ngfx
copyHeaders src .
copyHeaders porting porting
