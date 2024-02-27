#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>


class Perlin
{
public:
	Perlin(unsigned int seed);
	Perlin(unsigned int seed, int octaves, int frequency, float persistence, float lacunarity);

	float			noise(float x) const;
	float			noise(glm::vec2 v) const;
	float			noise(glm::vec3 v) const;
private:
	unsigned int	_seed;
	int 			_octaves = 1;
	int				_frequency = 1;
	float			_persistence = 0.5;
	float			_lacunarity = 2.0;


	float 			insideNoise(glm::vec3 v, unsigned int seed) const;
	float			interpolate(float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8, glm::vec3 t) const;

	unsigned int	hash(unsigned int x, unsigned int seed) const;
	unsigned int 	hash(glm::uvec3 v, unsigned int seed) const;

	glm::vec3		getHashedGradient(glm::uvec3 v, unsigned int seed) const;
	glm::vec3		getAngleGradient(glm::uvec3 v, unsigned int seed) const;
};
