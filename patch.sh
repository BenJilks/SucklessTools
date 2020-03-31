#!/bin/sh

git submodule update --init
patch -d feh < patches/feh.patch

