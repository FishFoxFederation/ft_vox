#!/bin/bash

if [ ! -d external/imgui ]
then
	echo "cloning 'https://github.com/ocornut/imgui.git' in external/imgui"

	mkdir -p external/imgui
	git clone https://github.com/ocornut/imgui.git external/imgui
fi