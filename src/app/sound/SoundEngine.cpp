#include "SoundEngine.hpp"

namespace Sound
{

	int paCallback_stereo(
		const void *in_buffer,
		void *out_buffer,
		unsigned long frames_count,
		const PaStreamCallbackTimeInfo *time_info,
		PaStreamCallbackFlags status_flags,
		void *user_data
	)
	{
		Engine *e = static_cast<Engine*>(user_data);
		float *out = (float*)out_buffer;
		(void) in_buffer;
		(void) time_info;
		(void) status_flags;

		for(size_t i = 0; i < frames_count; i++)
		{
			float left = 0.0f;
			float right = 0.0f;

			std::unique_lock lock(e->m_sound_instances_mutex, std::try_to_lock);

			if (!lock.owns_lock() || e->m_sound_instances.empty())
			{
				*out++ = 0.0f;
				*out++ = 0.0f;
				continue;
			}

			for (auto &sound : e->m_sound_instances)
			{
				if (sound.cursor >= sound.audio_data->num_samples)
				{
					if (!sound.loop)
					{
						sound.ended = true;
						continue;
					}
					sound.cursor = 0;
				}

				const Data &audio_data = *sound.audio_data;

				if (audio_data.num_channels == 1)
				{
					left += audio_data.samples[0][sound.cursor];
					right += audio_data.samples[0][sound.cursor];
				}
				else
				{
					left += audio_data.samples[0][sound.cursor];
					right += audio_data.samples[1][sound.cursor];
				}

				sound.cursor++;
			}

			*out++ = left / e->m_audio_datas.size();
			*out++ = right / e->m_audio_datas.size();
		}
		return paContinue;
	}

	Engine::Engine()
	{
		for (const std::string &file_name : sound_files)
		{
			m_audio_datas.push_back(std::make_unique<Data>(sound_folder + file_name));
		}
		playSound(SoundName::CALM1, true);

		PA_THROW_CHECK(Pa_Initialize(), "Failed to initialize PortAudio");

		PaHostApiIndex hostApiCount = Pa_GetHostApiCount();
		PaHostApiIndex hostApiIndex = Pa_GetDefaultHostApi();

		if (hostApiCount > 0)
		{
			const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(hostApiIndex);
			LOG_INFO("PortAudio: Default host API name: " + std::string(hostApiInfo->name));
		}
		else
		{
			LOG_WARNING("PortAudio: No host APIs found: " + std::string(Pa_GetErrorText(hostApiIndex)));
			return;
		}


		PA_THROW_CHECK(
			Pa_OpenDefaultStream(
				&m_stream,
				0, // no input
				2, // stereo output
				paFloat32, // 32 bit floating point output
				48000, // sample rate
				paFramesPerBufferUnspecified, // frames per buffer
				paCallback_stereo,
				this
			),
			"Failed to open stream"
		);

		PA_THROW_CHECK(Pa_StartStream(m_stream), "Failed to start stream");
	}

	Engine::~Engine()
	{
		PA_WARNING_CHECK(Pa_StopStream(m_stream), "Failed to stop stream");
		PA_WARNING_CHECK(Pa_CloseStream(m_stream), "Failed to close stream");
		PA_WARNING_CHECK(Pa_Terminate(), "Failed to terminate PortAudio");
	}

	void Engine::playSound(const SoundName sound, bool loop)
	{
		std::lock_guard lock(m_sound_instances_mutex);
		m_sound_instances.push_back({m_audio_datas[int(sound)].get(), 0, loop});
	}

} // namespace Sound
