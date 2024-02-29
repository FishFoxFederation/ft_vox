# https://github.com/google/shaderc/blob/main/downloads.md to download glslc

/usr/local/bin/glslc shaders/simple_shader.vert -o shaders/simple_shader.vert.spv
/usr/local/bin/glslc shaders/simple_shader.frag -o shaders/simple_shader.frag.spv

# ./glslc shaders/simple_shader.vert -o shaders/simple_shader.vert.spv
# ./glslc shaders/simple_shader.frag -o shaders/simple_shader.frag.spv

# ./external/glslc/glslc shaders/simple_shader.vert -o shaders/simple_shader.vert.spv
# ./external/glslc/glslc shaders/simple_shader.frag -o shaders/simple_shader.frag.spv

# glslang -V shaders/simple_shader.vert -o shaders/simple_shader.vert.spv
# glslang -V shaders/simple_shader.frag -o shaders/simple_shader.frag.spv
