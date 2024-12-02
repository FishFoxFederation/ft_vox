#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <string>
#include <stdexcept>

#define VK_ERR_STR(result) (std::string(string_VkResult(result)))

#define VK_CHECK(function, message) \
	{ \
		VkResult result = function; \
		if (result != VK_SUCCESS) \
		{ \
			throw std::runtime_error( \
				__PRETTY_FUNCTION__ + std::string(": ") \
				+ std::string(message) + " (" + std::string(string_VkResult(result)) + ")" \
			); \
		} \
	}
