# https://github.com/google/shaderc/blob/main/downloads.md to download glslc

shader_files=$(find shaders -type f -name "*.geom" -o -name "*.vert" -o -name "*.frag" -o -name "*.rgen" -o -name "*.rchit" -o -name "*.rmiss" -o -name "*.comp")

for file in $shader_files
do
	./external/glslc/glslc $file -o $file.spv --target-spv=spv1.4
	if [ $? -ne 0 ]; then
		echo "Failed to compile $file"
		exit 1
	fi
done

# ./external/glslc/glslc shaders/simple_shader.vert -o shaders/simple_shader.vert.spv
# ./external/glslc/glslc shaders/simple_shader.frag -o shaders/simple_shader.frag.spv
