#pragma once

#include "define.hpp"

#include <cppVulkanAPI.hpp>

class Renderer
{

public:

	Renderer();
	~Renderer();

	Renderer(Renderer& renderer) = delete;
	Renderer(Renderer&& renderer) = delete;
	Renderer& operator=(Renderer& renderer) = delete;

private:

};