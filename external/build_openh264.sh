#!/bin/bash

echo "$0 $1 $2"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
EXTRA=""
echo $DIR

if [ ! -d ${DIR}/openh264-master ]; then
	wget "https://codeload.github.com/cisco/openh264/zip/master" -O ${DIR}/openh264-master.zip
	unzip ${DIR}/openh264-master.zip -d ${DIR}
# 	rm ${DIR}/openh264-master.zip
fi

if [[ $2 == *"mingw"* ]]; then
	EXTRA="OS=mingw_nt";
fi

if [ ! -f $1/libopenh264_static.a ]; then
	make CC=$2 -C $DIR/openh264-master $EXTRA
	cp $DIR/openh264-master/libdecoder.a $1/libdecoder_static.a
	cp $DIR/openh264-master/libopenh264.a $1/libopenh264_static.a
fi
