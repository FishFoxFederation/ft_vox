#include "HitBox.hpp"
#include "logger.hpp"

HitBox::HitBox(
	const glm::vec3 & position,
	const glm::vec3 & size
):
	transform(position, glm::vec3(0.0f), glm::vec3(0.0f), size)
{
}

HitBox::HitBox(
	const glm::vec3 & position,
	const glm::vec3 & size,
	const std::vector<glm::vec3> & vertices
):
	transform(position, glm::vec3(0.0f), glm::vec3(0.0f), size),
	m_vertices(vertices)
{
}

HitBox::~HitBox()
{
}

HitBox::HitBox(const HitBox & other):
	transform(other.transform),
	m_vertices(other.m_vertices)
{
}

HitBox::HitBox(HitBox && other):
	transform(std::move(other.transform)),
	m_vertices(std::move(other.m_vertices))
{
}

std::vector<glm::vec3> HitBox::transformedVertices() const
{
	const glm::mat4 model = transform.model();
	std::vector<glm::vec3> transformed_vertices;
	for (const auto & vertex : m_vertices)
	{
		transformed_vertices.push_back(glm::vec3(model * glm::vec4(vertex, 1.0f)));
	}

	return transformed_vertices;

}

void HitBox::insertNormals(std::vector<glm::vec3> & normals) const
{
	glm::vec3 origin = m_vertices[0];
	glm::vec3 x_axis = m_vertices[1] - origin;
	glm::vec3 y_axis = m_vertices[3] - origin;
	glm::vec3 z_axis = m_vertices[4] - origin;

	normals.push_back(glm::normalize(glm::cross(x_axis, y_axis)));
	normals.push_back(glm::normalize(glm::cross(x_axis, z_axis)));
	normals.push_back(glm::normalize(glm::cross(y_axis, z_axis)));
}

bool isColliding(
	const HitBox & hitbox1,
	const HitBox & hitbox2
)
{
	std::vector<glm::vec3> normals;
	hitbox1.insertNormals(normals);
	hitbox2.insertNormals(normals);
	std::vector<glm::vec3> transformed_vertices1 = hitbox1.transformedVertices();
	std::vector<glm::vec3> transformed_vertices2 = hitbox2.transformedVertices();

	for (const auto & normal : normals)
	{
		Projection projection1(transformed_vertices1, normal);
		Projection projection2(transformed_vertices2, normal);
		if (!projection1.isOverlapping(projection2))
		{
			return false;
		}
	}

	return true;
}
