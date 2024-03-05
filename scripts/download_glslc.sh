#!/bin/bash

url="https://storage.googleapis.com/shaderc/artifacts/prod/graphics_shader_compiler/shaderc/linux/continuous_gcc_release/447/20240212-124340/install.tgz"

if [ ! -d external/glslc ]
then
	if [ ! -d ~/goinfre ]
	then
		echo "There is no ~/goinfre directory, please create it and run the script again or download glslc manually"
		exit 1
	fi

	curl -o ~/goinfre/install.tgz $url &&
	tar -xf ~/goinfre/install.tgz -C ~/goinfre &&
	mkdir -p external/glslc &&
	mv ~/goinfre/install/bin/glslc external/glslc &&
	rm -rf ~/goinfre/install.tgz ~/goinfre/install
fi
