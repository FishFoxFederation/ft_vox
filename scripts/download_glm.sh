#!/bin/bash

if [ ! -d external/glm ]
then
	echo "cloning 'https://github.com/g-truc/glm.git' in external/glm"

	mkdir -p external/glm
	git clone https://github.com/g-truc/glm.git external/glm
fi