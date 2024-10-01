#!/bin/bash

url="https://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz"


if [ ! -d external/portaudio ]
then
	curl $url -o portaudio.tgz &&
	tar -xf portaudio.tgz -C external &&
	rm -rf portaudio.tgz

	# Add -fPIE to compile options
	echo "target_compile_options(portaudio_static PRIVATE -fPIE)" >> external/portaudio/CMakeLists.txt
fi
