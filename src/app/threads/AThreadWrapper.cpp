#include "AThreadWrapper.hpp"

AThreadWrapper::AThreadWrapper()
	:m_thread(&AThreadWrapper::launch)
{

}

void AThreadWrapper::launch()
{
	init();

	while (!m_thread.request_stop())
	{
		loop();
	}
}
