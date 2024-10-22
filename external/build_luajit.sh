#!/bin/bash

# echo "0: $0"
# echo "1: $1"
# echo "2: $2"
# echo "3: $3"
# echo "4: $4"
# echo "5: $5"
# echo "6: $6"
# echo "HOST_CC=cc $5"
# echo "CC=$2"

echo "PWD: $(pwd)"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
EXTRA=""

if [ ! -d ${DIR}/LuaJIT-2.0.4 ]; then
	wget "http://luajit.org/download/LuaJIT-2.0.4.tar.gz" -O ${DIR}/LuaJIT-2.0.4.tar.gz
	tar xf ${DIR}/LuaJIT-2.0.4.tar.gz -C ${DIR}
# 	rm ${DIR}/LuaJIT-2.0.4.tar.gz
fi

if [[ $2 == *"mingw"* ]]; then
	EXTRA="TARGET_SYS=Windows";
fi

# echo "ASM: $3"
# echo "EXTRA: $EXTRA"
# echo "STRIP: $4"
# echo "CFLAGS: $6"

if [ ! -f $1/libluajit_static.a ]; then
# 	echo "make clean libluajit.a -C ${DIR}/LuaJIT-2.0.4/src HOST_CC=\"cc $5\" BUILDMODE=static CC=\"$2\" ASM=\"$3\" STRIP=$4 $EXTRA XCFLAGS=-DLUAJIT_USE_SYSMALLOC CFLAGS=\"$6\""
	echo "make clean libluajit.a -C ${DIR}/LuaJIT-2.0.4/src HOST_CC=\"cc $5\" BUILDMODE=static CC=\"$2\" ASM=\"$3\" STRIP=$4 $EXTRA CFLAGS=\"$6\""
	make clean libluajit.a -C ${DIR}/LuaJIT-2.0.4/src HOST_CC="cc $5" BUILDMODE=static CC="$2" ASM="$3" STRIP=$4 $EXTRA CFLAGS="$6"
	cp ${DIR}/LuaJIT-2.0.4/src/libluajit.a $1/libluajit_static.a
fi
