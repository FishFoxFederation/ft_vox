#!/bin/bash

if [ ! -d external/tracy ]
then
	echo "cloning 'https://github.com/wolfpld/tracy.git' in external/tracy"

	mkdir -p external/tracy
	git clone https://github.com/wolfpld/tracy.git external/tracy
fi