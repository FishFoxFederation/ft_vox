#!/bin/bash

# Sript to build the project

# Download dependencies
./scripts/download_glm.sh
./scripts/download_imgui.sh
./scripts/download_glslc.sh
./scripts/download_tracy.sh

# Compile shaders
./scripts/compile_shaders.sh
if [ $? -ne 0 ]; then
	echo "Failed to compile shaders"
	exit 1
fi

# create build directory
mkdir -p build

# build the project
cd build
cmake ..
make -j
