#!/bin/bash

url="https://download.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.xz"


if [ ! -d external/freetype ]
then
	wget $url &&
	mkdir -p external/freetype &&
	tar -xf freetype-2.13.3.tar.xz -C external/freetype &&
	rm -rf freetype-2.13.3.tar.xz
fi