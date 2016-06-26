#!/bin/bash

ndk-build -j8 $@

mkdir -p assets
rm -rf assets/*
cp -R ../data assets/

ant release
adb install -r "bin/BC Flight VR-release.apk"
