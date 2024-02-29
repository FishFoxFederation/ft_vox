#!/bin/bash

# Sript to build the project

# Download dependencies
./scripts/download_glm.sh
./scripts/download_imgui.sh

# Compile shaders
./scripts/compile_shaders.sh

# create build directory
mkdir -p build

# build the project
cd build
cmake ..
make -j
