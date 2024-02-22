#include "Settings.hpp"

Settings::Settings()
{
}

Settings::~Settings()
{
}

void Settings::setMouseSensitivity(double sensitivity)
{
	std::lock_guard<std::shared_mutex> lock(m_mutex);
	m_mouse_sensitivity = sensitivity;
}

double Settings::mouseSensitivity() const
{
	std::shared_lock<std::shared_mutex> lock(m_mutex);
	return m_mouse_sensitivity;
}