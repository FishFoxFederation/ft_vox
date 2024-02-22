#pragma once

#include "define.hpp"

#include <mutex>

/**
 * @brief Class to hold the settings of the application.
 * 
 */
class Settings
{

public:

	Settings();
	~Settings();

	Settings(Settings & other) = delete;
	Settings(Settings && other) = delete;
	Settings & operator=(Settings & other) = delete;
	Settings & operator=(Settings && other) = delete;

private:


};