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
		-g
		-Wall
		-Wextra
		-Werror
		-Wpedantic
		-Wno-missing-braces
)

# download cppVulkanAPI
add_custom_target(
	download_cppVulkanAPI
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/download_cppVulkanAPI.sh
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_dependencies(${PROJECT_NAME} download_cppVulkanAPI)

target_sources(${PROJECT_NAME}
	PRIVATE
		src/main.cpp
		src/app/application.cpp
		src/app/render/renderer.cpp
)

# include directories (equivalent to -I)
target_include_directories(${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}/src/app
		${CMAKE_CURRENT_SOURCE_DIR}/src/app/render
)

# link libraries
add_subdirectory(external/cppVulkanAPI)
target_link_libraries(${PROJECT_NAME}
	PRIVATE
		cppVulkanAPI
)
