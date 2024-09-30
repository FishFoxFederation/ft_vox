#pragma once

#include <string>
#include <cstdio>

template<typename... Args>
std::string ft_format(const std::string & format, Args... args)
{
	size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
	std::string str(size, 0);
	snprintf(str.data(), size, format.c_str(), args...);
	str.pop_back();
	return str;
}
