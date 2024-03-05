# https://github.com/google/shaderc/blob/main/downloads.md to download glslc

shader_files=$(find shaders -type f -name "*.vert" -o -name "*.frag")

for file in $shader_files
do
	./external/glslc/glslc $file -o $file.spv
	if [ $? -ne 0 ]; then
		echo "Failed to compile $file"
		exit 1
	fi
done

# ./external/glslc/glslc shaders/simple_shader.vert -o shaders/simple_shader.vert.spv
# ./external/glslc/glslc shaders/simple_shader.frag -o shaders/simple_shader.frag.spv
