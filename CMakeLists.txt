cmake_minimum_required(VERSION 3.15)

project(ft_vox
	VERSION 0.0.1
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME})

target_compile_options(${PROJECT_NAME}
	PRIVATE
		-O3
		-Wall
		-Wextra
		-Werror
		-Wpedantic
		-Wno-missing-braces
)

target_sources(${PROJECT_NAME}
	PRIVATE
		src/main.cpp

		src/utils/logger.cpp

		src/app/perlin/Perlin.cpp

		src/app/application.cpp
		src/app/DebugGui.cpp
		src/app/Settings.cpp
		src/app/scenes/Camera.cpp
		src/app/scenes/WorldScene.cpp
		src/app/threads/AThreadWrapper.cpp
		src/app/threads/render/RenderThread.cpp
		src/app/threads/update/UpdateThread.cpp
		src/app/threads/pool/ThreadPool.cpp
		src/app/threads/block_update/BlockUpdateThread.cpp
		src/app/vulkan/VulkanAPI.cpp
		src/app/vulkan/VulkanMemoryAllocator.cpp
		src/app/vulkan/chunk_mesh.cpp
		src/app/vulkan/vk_helper.cpp
		src/app/vulkan/Image.cpp
		src/app/vulkan/Command.cpp
		src/app/vulkan/Descriptor.cpp
		src/app/window/window.cpp
		src/app/window/input.cpp
		src/app/world/Chunk.cpp
		src/app/world/World.cpp
		src/app/world/WorldGenerator.cpp
		src/app/world/Player.cpp

		external/imgui/imgui.cpp
		external/imgui/imgui_draw.cpp
		external/imgui/imgui_widgets.cpp
		external/imgui/imgui_tables.cpp
		external/imgui/imgui_demo.cpp
		external/imgui/backends/imgui_impl_glfw.cpp
		external/imgui/backends/imgui_impl_vulkan.cpp
)

# include directories (equivalent to -I)
target_include_directories(${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}/src/utils
		${CMAKE_CURRENT_SOURCE_DIR}/src/app
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/scenes
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/threads
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/threads/pool
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/threads/render
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/threads/update
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/threads/block_update
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/vulkan
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/window
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/perlin
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/world

		${CMAKE_CURRENT_SOURCE_DIR}/external/glm
		${CMAKE_CURRENT_SOURCE_DIR}/external/stb
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
)

target_link_libraries(${PROJECT_NAME} glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)

