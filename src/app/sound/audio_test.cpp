#include <iostream>
#include <fstream>

#include "../../utils/logger.hpp"

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

int main(int argc, char **argv)
{
	logger.setTimestamp(false);
	
	if (argc < 2)
	{
		LOG_ERROR("No arguments provided");
		return 1;
	}

	const std::string file_name = argv[1];
	std::fstream file(file_name, std::ios::in | std::ios::binary);

	if (!file.is_open())
	{
		LOG_ERROR("Failed to open file: " + file_name);
		return 1;
	}

	RIFFChunk header;
	file.read(reinterpret_cast<char*>(&header), sizeof(RIFFChunk));

	LOG_INFO("File type: " + std::string(header.id, 4));
	LOG_INFO("File size: " + std::to_string(header.size));
	LOG_INFO("File format: " + std::string(header.format, 4));
	LOG_INFO(" ");


	FMTChunk fmt;
	file.read(reinterpret_cast<char*>(&fmt), sizeof(FMTChunk));

	LOG_INFO("FMT chunk ID: " + std::string(fmt.id, 4));
	LOG_INFO("FMT chunk size: " + std::to_string(fmt.size));
	LOG_INFO("Audio format: " + std::to_string(fmt.audio_format));
	LOG_INFO("Number of channels: " + std::to_string(fmt.num_channels));
	LOG_INFO("Sample rate: " + std::to_string(fmt.sample_rate));
	LOG_INFO("Byte rate: " + std::to_string(fmt.byte_rate));
	LOG_INFO("Block align: " + std::to_string(fmt.block_align));
	LOG_INFO("Bits per sample: " + std::to_string(fmt.bits_per_sample));
	LOG_INFO(" ");

	// Skip the rest of the fmt chunk
	const int bytes_to_skip = fmt.size - sizeof(FMTChunk) + 8;
	file.seekg(bytes_to_skip, std::ios::cur);
	LOG_INFO("Skipping " + std::to_string(bytes_to_skip) + " bytes");
	LOG_INFO(" ");


	DataChunk data;
	for (;;)
	{
		file.read(reinterpret_cast<char*>(&data), sizeof(DataChunk));

		LOG_INFO("Data chunk ID: " + std::string(data.id, 4));
		LOG_INFO("Data chunk size: " + std::to_string(data.size));
		LOG_INFO(" ");

		if (std::string(data.id, 4) == "data")
		{
			break;
		}
		
		// Skip the rest of the data chunk
		const int bytes_to_skip = data.size;
		file.seekg(bytes_to_skip, std::ios::cur);
		LOG_INFO("Skipping " + std::to_string(bytes_to_skip) + " bytes");
		LOG_INFO(" ");
	}


	file.close();
	return 0;
}