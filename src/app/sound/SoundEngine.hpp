#pragma once

#include "logger.hpp"

#include <portaudio.h>

#define PA_THROW_CHECK(expr) \
{ \
	PaError err = expr; \
	if (err != paNoError) \
	{ \
		throw std::runtime_error("portaudio: " + std::string(Pa_GetErrorText(err))); \
	} \
}

#define PA_WARNING_CHECK(expr) \
{ \
	PaError err = expr; \
	if (err != paNoError) \
	{ \
		LOG_WARNING("portaudio: " + std::string(Pa_GetErrorText(err))); \
	} \
}

class SoundEngine
{

public:

	SoundEngine();
	~SoundEngine();

private:

	PaStream *m_stream = nullptr;

	float left_phase = 0;
    float right_phase = 0;

	friend int paCallback(
		const void *inputBuffer,
		void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo *timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData
	);
};
