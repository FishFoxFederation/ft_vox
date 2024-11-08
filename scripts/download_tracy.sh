#!/bin/bash

if [ ! -d external/tracy ]
then
	echo "cloning 'https://github.com/wolfpld/tracy.git' in external/tracy"

	mkdir -p external/tracy
	git clone https://github.com/wolfpld/tracy.git external/tracy
fi

if [ ! -d external/tracy-experimental ]
then
	echo "cloning 'https://github.com/Arpafaucon/tracy.git' in external/tracy"

	mkdir -p external/tracy-experimental
	git clone https://github.com/Arpafaucon/tracy.git external/tracy-experimental --branch multicapture-merge
fi
