#!/bin/bash

if [ ! -d external/stb ]
then
	echo "cloning 'https://github.com/nothings/stb.git' in external/stb"

	git clone https://github.com/nothings/stb.git external/stb
fi
