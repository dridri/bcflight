#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -f ${DIR}/LuaJIT-2.0.4 ]; then
	wget "http://luajit.org/download/LuaJIT-2.0.4.tar.gz" -O ${DIR}/LuaJIT-2.0.4.tar.gz
	tar xf ${DIR}/LuaJIT-2.0.4.tar.gz -C ${DIR}
	rm ${DIR}/LuaJIT-2.0.4.tar.gz
fi

if [ ! -f libluajit.a ]; then
	make clean libluajit.a -C ${DIR}/LuaJIT-2.0.4/src HOST_CC="gcc -m32" BUILDMODE=static CC=$1 ASM=$2 STRIP=$3 -j$4
	cp ${DIR}/LuaJIT-2.0.4/src/libluajit.a libluajit.a
fi
