#pragma once

#include "define.hpp"

#include <shared_mutex>

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

	void setMouseSensitivity(double sensitivity);
	double mouseSensitivity() const;


private:

	mutable std::shared_mutex m_mutex;

	double m_mouse_sensitivity{0.2};

};