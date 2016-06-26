#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo $DIR

if [ ! -d ${DIR}/openh264-master ]; then
	wget "https://codeload.github.com/cisco/openh264/zip/master" -O ${DIR}/openh264-master.zip
	unzip ${DIR}/openh264-master.zip -d ${DIR}
# 	rm ${DIR}/openh264-master.zip
fi

if [ ! -f $1/libopenh264_static.a ]; then
	make -C $DIR/openh264-master
	cp $DIR/openh264-master/libdecoder.a $1/libdecoder_static.a
	cp $DIR/openh264-master/libopenh264.a $1/libopenh264_static.a
fi
