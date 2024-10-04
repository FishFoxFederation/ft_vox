#pragma once

#include "AudioData.hpp"
#include "SoundList.hpp"
#include "logger.hpp"

#include <portaudio.h>

#include <memory>
#include <vector>
#include <mutex>

#define PA_THROW_CHECK(expr, msg) \
{ \
	PaError err = expr; \
	if (err != paNoError) \
	{ \
		throw std::runtime_error(std::string("Portaudio: ") + msg + " (" + std::string(Pa_GetErrorText(err)) + ")"); \
	} \
}

#define PA_WARNING_CHECK(expr, msg) \
{ \
	PaError err = expr; \
	if (err != paNoError) \
	{ \
		LOG_WARNING(std::string("Portaudio: ") + msg + " (" + std::string(Pa_GetErrorText(err)) + ")"); \
	} \
}

namespace Sound
{
	struct Instance
	{
		Data *audio_data = nullptr;
		uint32_t cursor = 0;
		bool loop = false;
		bool ended = false;
	};

	class Engine
	{

	public:

		Engine();
		~Engine();

		void playSound(const SoundName audio_index, bool loop = false);

	private:

		PaStream *m_stream = nullptr;

		std::vector<std::unique_ptr<Data>> m_audio_datas;

		std::mutex m_sound_instances_mutex;
		std::vector<Instance> m_sound_instances;

		friend int paCallback_stereo(
			const void *inputBuffer,
			void *outputBuffer,
			unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo *timeInfo,
			PaStreamCallbackFlags statusFlags,
			void *userData
		);
	};

}
