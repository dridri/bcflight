#!/bin/bash

echo "$0 $1 $2 $3 $4 $5"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -d ${DIR}/LuaJIT-2.0.4 ]; then
	wget "http://luajit.org/download/LuaJIT-2.0.4.tar.gz" -O ${DIR}/LuaJIT-2.0.4.tar.gz
	tar xf ${DIR}/LuaJIT-2.0.4.tar.gz -C ${DIR}
	rm ${DIR}/LuaJIT-2.0.4.tar.gz
fi

if [ ! -f $1/libluajit_static.a ]; then
	make clean libluajit.a -C ${DIR}/LuaJIT-2.0.4/src HOST_CC="cc $5" BUILDMODE=static CC="$2" ASM="$3" STRIP=$4
	cp ${DIR}/LuaJIT-2.0.4/src/libluajit.a libluajit_static.a
fi
