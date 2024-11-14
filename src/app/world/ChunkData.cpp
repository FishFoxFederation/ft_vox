#include "Chunk.hpp"
#include <cstring>
#include "RLE_TEST.hpp"

ChunkData::ChunkData(const Chunk & chunk)
{
	blocks = chunk.getBlocks();
	light = chunk.getLight();
	biome = chunk.getBiomes();
	height = chunk.getHeights();
	gen_level = chunk.getGenLevel();
	position = chunk.getPosition();
}

std::vector<char> ChunkData::serialize() const
{
	std::array<char, DATA_SIZE> data = {};

	size_t index = 0;

	std::memcpy(data.data(), blocks.data(), sizeof(blocks));
	index += sizeof(blocks);

	std::memcpy(data.data() + index, light.data(), sizeof(light));
	index += sizeof(light);

	std::memcpy(data.data() + index, biome.data(), sizeof(biome));
	index += sizeof(biome);

	std::memcpy(data.data() + index, height.data(), sizeof(height));
	index += sizeof(height);

	std::memcpy(data.data() + index, &gen_level, sizeof(gen_level));
	index += sizeof(gen_level);

	std::memcpy(data.data() + index, &position, sizeof(position));
	index += sizeof(position);

	RLE_TEST<char> rle;
	rle.compressData(data.data(), data.size());
	// LOG_INFO("Serialized chunk:" << position.x << " " << position.z << " size: " << rle.getRawSize());

	std::vector<char> ret(rle.getRawSize());
	std::memcpy(ret.data(), rle.getRaw().data(), rle.getRawSize());
	return ret;
};

void ChunkData::deserialize(const char * data, const size_t & size)
{
	RLE_TEST<char> rle;
	rle.setContent(data, size);
	std::vector<char> uncompressedData = rle.getData();

	size_t index = 0;
	
	std::memcpy(blocks.data(), uncompressedData.data(), sizeof(blocks));
	index += sizeof(blocks);

	std::memcpy(light.data(), uncompressedData.data() + index, sizeof(light));
	index += sizeof(light);

	std::memcpy(biome.data(), uncompressedData.data() + index, sizeof(biome));
	index += sizeof(biome);

	std::memcpy(height.data(), uncompressedData.data() + index, sizeof(height));
	index += sizeof(height);

	std::memcpy(&gen_level, uncompressedData.data() + index, sizeof(gen_level));
	index += sizeof(gen_level);

	std::memcpy(&position, uncompressedData.data() + index, sizeof(position));
	index += sizeof(position);

	// LOG_INFO("Deserialized chunk:" << position.x << " " << position.z << " size: " << size);
}
