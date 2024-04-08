#include "AThreadWrapper.hpp"
#include "logger.hpp"

AThreadWrapper::AThreadWrapper():
	m_thread(&AThreadWrapper::launch, this)
{

}

void AThreadWrapper::launch()
{
	try {
	init();

	while (!m_thread.get_stop_token().stop_requested())
	{
		loop();
	}
	} catch (const std::exception & e) {
		LOG_ERROR("Thread exception: " << e.what());
	}
}
