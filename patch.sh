#!/bin/sh

git submodule update --init
patch -d feh < patches/feh.patch
patch -d xclip < patches/xclip.patch

cd xclip
autoconf
./configure
cd ..

