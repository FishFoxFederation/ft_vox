#include "AThreadWrapper.hpp"
#include "logger.hpp"

AThreadWrapper::AThreadWrapper():
	m_thread(&AThreadWrapper::launch, this)
{

}

void AThreadWrapper::launch()
{
	init();

	while (!m_thread.get_stop_token().stop_requested())
	{
		loop();
	}
}
