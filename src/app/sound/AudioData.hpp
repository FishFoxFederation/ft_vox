#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Sound
{

	class Data
	{

	public:

		Data(const std::string &file_name);
		~Data();


		uint16_t audio_format = 0;
		uint16_t num_channels = 0;
		uint32_t sample_rate = 0;
		uint32_t byte_rate = 0;
		uint16_t block_align = 0;
		uint16_t bits_per_sample = 0;

		uint32_t num_samples = 0;

		std::vector<std::vector<float>> samples;

	private:

		enum class WaveFormat : uint16_t
		{
			WAVE_FORMAT_PCM = 0x0001,
			WAVE_FORMAT_IEEE_FLOAT = 0x0003,
			WAVE_FORMAT_ALAW = 0x0006,
			WAVE_FORMAT_MULAW = 0x0007,
			WAVE_FORMAT_EXTENSIBLE = 0xFFFE
		};

		float bytesToFloat(std::vector<uint8_t> &bytes);
	};

} // namespace Sound
