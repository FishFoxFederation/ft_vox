#include "AudioData.hpp"
#include "logger.hpp"

#include <fstream>

namespace Sound
{

	struct RIFFChunk
	{
		char id[4];
		uint32_t size;
		char format[4];
	};

	struct FMTChunk
	{
		char id[4];
		uint32_t size;
		uint16_t audio_format;
		uint16_t num_channels;
		uint32_t sample_rate;
		uint32_t byte_rate;
		uint16_t block_align;
		uint16_t bits_per_sample;
	};

	struct DataChunk
	{
		char id[4];
		uint32_t size;
	};

	Data::Data(const std::string &file_name)
	{
		std::fstream file(file_name, std::ios::in | std::ios::binary);

		if (!file.is_open())
		{
			LOG_ERROR("Failed to open file: " + file_name);
			return;
		}

		RIFFChunk header;
		file.read(reinterpret_cast<char*>(&header), sizeof(RIFFChunk));

		// LOG_INFO("File type: " + std::string(header.id, 4));
		// LOG_INFO("File size: " + std::to_string(header.size));
		// LOG_INFO("File format: " + std::string(header.format, 4));
		// LOG_INFO(" ");


		FMTChunk fmt;
		file.read(reinterpret_cast<char*>(&fmt), sizeof(FMTChunk));

		audio_format = fmt.audio_format;
		num_channels = fmt.num_channels;
		sample_rate = fmt.sample_rate;
		byte_rate = fmt.byte_rate;
		block_align = fmt.block_align;
		bits_per_sample = fmt.bits_per_sample;

		// LOG_INFO("FMT chunk ID: " + std::string(fmt.id, 4));
		// LOG_INFO("FMT chunk size: " + std::to_string(fmt.size));
		// LOG_INFO("Audio format: " + std::to_string(fmt.audio_format));
		// LOG_INFO("Number of channels: " + std::to_string(fmt.num_channels));
		// LOG_INFO("Sample rate: " + std::to_string(fmt.sample_rate));
		// LOG_INFO("Byte rate: " + std::to_string(fmt.byte_rate));
		// LOG_INFO("Block align: " + std::to_string(fmt.block_align));
		// LOG_INFO("Bits per sample: " + std::to_string(fmt.bits_per_sample));
		// LOG_INFO(" ");

		// Skip the rest of the fmt chunk
		const int bytes_to_skip = fmt.size - sizeof(FMTChunk) + 8;
		file.seekg(bytes_to_skip, std::ios::cur);

		// Read chunks until we find the data chunk
		DataChunk chunk_data;
		for (;;)
		{
			file.read(reinterpret_cast<char*>(&chunk_data), sizeof(DataChunk));

			if (std::string(chunk_data.id, 4) == "data")
			{
				break;
			}

			// Skip the rest of the chunk
			const int bytes_to_skip = chunk_data.size;
			file.seekg(bytes_to_skip, std::ios::cur);
		}

		const uint32_t sample_size = fmt.bits_per_sample / 8;
		// const int sample_padding = fmt.block_align - fmt.num_channels * sample_size;
		num_samples = chunk_data.size / fmt.block_align;
		const float norm_sample = 1.0f / (glm::pow(2, fmt.bits_per_sample - 1) - 1);


		samples.resize(fmt.num_channels);
		for (uint32_t i = 0; i < fmt.num_channels; i++)
		{
			samples[i].resize(num_samples);
		}

		std::vector<uint8_t> bytes(sample_size);
		for (uint32_t i = 0; i < num_samples; i++)
		{
			for (uint32_t j = 0; j < fmt.num_channels; j++)
			{
				file.read(reinterpret_cast<char*>(bytes.data()), sample_size);
				samples[j][i] = bytesToFloat(bytes) * norm_sample;
			}
		}

		file.close();
	}

	Data::~Data()
	{
	}

	float Data::bytesToFloat(std::vector<uint8_t> &bytes)
	{
		if (audio_format == static_cast<uint16_t>(WaveFormat::WAVE_FORMAT_PCM))
		{
			switch (bytes.size())
			{
			case 1:
				return bytes[0] - 128;
				break;

			case 2:
				return *reinterpret_cast<int16_t*>(bytes.data());
				break;

			case 3:
				return (bytes[2] >> 7) * (0xFF << 24) | bytes[2] << 16 | bytes[1] << 8 | bytes[0];
				break;

			case 4:
				return *reinterpret_cast<int32_t*>(bytes.data());
				break;

			default:
				throw std::runtime_error("Unsupported size for PCM format: " + std::to_string(bytes.size()));
				break;
			}
		}
		else if (audio_format == static_cast<uint16_t>(WaveFormat::WAVE_FORMAT_IEEE_FLOAT))
		{
			if (bytes.size() != 4)
			{
				throw std::runtime_error("Unsupported size for IEEE float format: " + std::to_string(bytes.size()));
			}

			return *reinterpret_cast<float*>(bytes.data());
		}
		else
		{
			throw std::runtime_error("Unsupported audio format: " + std::to_string(audio_format));
		}
	}

} // namespace Sound
