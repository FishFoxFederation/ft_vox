#include "HitBox.hpp"
#include "logger.hpp"

HitBox::HitBox(
	const glm::vec3 & position,
	const glm::vec3 & size
):
	vertices({
		position + glm::vec3(0.0f,   0.0f,   0.0f),
		position + glm::vec3(size.x, 0.0f,   0.0f),
		position + glm::vec3(size.x, size.y, 0.0f),
		position + glm::vec3(0.0f,   size.y, 0.0f),
		position + glm::vec3(0.0f,   0.0f,   size.z),
		position + glm::vec3(size.x, 0.0f,   size.z),
		position + glm::vec3(size.x, size.y, size.z),
		position + glm::vec3(0.0f,   size.y, size.z)
	})
{
}

HitBox::HitBox(const std::vector<glm::vec3> & vertices):
	vertices(vertices)
{
}

HitBox::~HitBox()
{
}

HitBox::HitBox(const HitBox & other):
	vertices(other.vertices)
{
}

HitBox::HitBox(HitBox && other):
	vertices(std::move(other.vertices))
{
}

HitBox HitBox::transform(const glm::mat4 & model) const
{
	std::vector<glm::vec3> transformed_vertices;
	transformed_vertices.reserve(vertices.size());
	for (const auto & vertex : vertices)
	{
		transformed_vertices.push_back(glm::vec3(model * glm::vec4(vertex, 1.0f)));
	}
	return HitBox(transformed_vertices);
}

void HitBox::insertNormals(std::vector<glm::vec3> & normals) const
{
	glm::vec3 origin = vertices[0];
	glm::vec3 x_axis = vertices[1] - origin;
	glm::vec3 y_axis = vertices[3] - origin;
	glm::vec3 z_axis = vertices[4] - origin;

	normals.push_back(glm::normalize(glm::cross(x_axis, y_axis)));
	normals.push_back(glm::normalize(glm::cross(x_axis, z_axis)));
	normals.push_back(glm::normalize(glm::cross(y_axis, z_axis)));
}

Projection projection(const std::vector<glm::vec3> & vertices, const glm::vec3 & normal)
{
	float min = std::numeric_limits<float>::max();
	float max = std::numeric_limits<float>::min();
	for (const auto & vertex : vertices)
	{
		float projection = glm::dot(vertex, normal);
		min = std::min(min, projection);
		max = std::max(max, projection);
	}
	return Projection(min, max);
}

bool isColliding(
	const HitBox & hitbox1,
	const HitBox & hitbox2
)
{
	std::vector<glm::vec3> normals;
	hitbox1.insertNormals(normals);
	hitbox2.insertNormals(normals);

	for (const auto & normal : normals)
	{
		Projection projection1 = projection(hitbox1.vertices, normal);
		Projection projection2 = projection(hitbox2.vertices, normal);
		if (!projection1.isOverlapping(projection2))
		{
			return false;
		}
	}

	return true;
}
