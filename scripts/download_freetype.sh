#!/bin/bash

url="https://download.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.xz"


if [ ! -d external/freetype ]
then
	# mkdir -p tmp &&
	# curl -o ./tmp/freetype.tar.xz $url &&
	# tar -xf ./tmp/freetype.tar.xz -C ./tmp &&
	# mkdir -p external/freetype &&
	# mv ./tmp/freetype external/freetype &&
	# rm -rf ./tmp/freetype.tar.xz ./tmp/freetype

	wget $url &&
	mkdir -p external/freetype &&
	tar -xf freetype-2.13.3.tar.xz -C external/freetype &&
	rm -rf freetype-2.13.3.tar.xz
fi