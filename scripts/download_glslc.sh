#!/bin/bash

url="https://storage.googleapis.com/shaderc/artifacts/prod/graphics_shader_compiler/shaderc/linux/continuous_gcc_release/447/20240212-124340/install.tgz"

if [ ! -d external/glslc ]
then
	if [ ! -d ~/goinfre ]
	then
		mkdir -p tmp &&
		curl -o ./tmp/install.tgz $url &&
		tar -xf ./tmp/install.tgz -C ./tmp &&
		mkdir -p external/glslc &&
		mv ./tmp/install/bin/glslc external/glslc &&
		rm -rf ./tmp/install.tgz ./tmp/install
	fi

	curl -o ~/goinfre/install.tgz $url &&
	tar -xf ~/goinfre/install.tgz -C ~/goinfre &&
	mkdir -p external/glslc &&
	mv ~/goinfre/install/bin/glslc external/glslc &&
	rm -rf ~/goinfre/install.tgz ~/goinfre/install
fi
