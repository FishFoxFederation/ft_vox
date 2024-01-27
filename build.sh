#!/bin/bash

# Sript to build the project

# Download the necessary external libraries
./scripts/download_cppVulkanAPI.sh

# create build directory
mkdir -p build

# build the project
cd build
cmake ..
make -j