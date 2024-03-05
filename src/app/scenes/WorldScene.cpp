#include "WorldScene.hpp"
#include "logger.hpp"
#include "DebugGui.hpp"

#include <algorithm>

WorldScene::WorldScene()
{
}

WorldScene::~WorldScene()
{
}

void WorldScene::addMeshData(uint64_t meshID, const Transform & transform)
{
	std::unique_lock<std::mutex> lock(m_mesh_render_data_mutex);
	m_mesh_render_data.push_back({
		.id = meshID,
		.transform = transform
	});
}

void WorldScene::removeMesh(uint64_t meshID)
{
	std::unique_lock<std::mutex> lock(m_mesh_render_data_mutex);
	std::erase_if(m_mesh_render_data, [meshID](MeshRenderData& mesh_render_data) {
		return mesh_render_data.id == meshID;
	});
}

std::vector<WorldScene::MeshRenderData> WorldScene::getMeshRenderData() const
{
	std::unique_lock<std::mutex> lock(m_mesh_render_data_mutex);
	return m_mesh_render_data;
}



glm::mat4 WorldScene::Camera::getViewMatrix() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return glm::lookAt(m_position, m_position + direction(), up);
}

glm::mat4 WorldScene::Camera::getProjectionMatrix(float aspect_ratio) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return glm::perspective(glm::radians(fov), aspect_ratio, 0.1f, far_plane);
}

void WorldScene::Camera::moveForward(float distance)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_position += distance * glm::normalize(glm::vec3(direction().x, 0.0f, direction().z));
}

void WorldScene::Camera::moveRight(float distance)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_position += distance * glm::normalize(glm::cross(direction(), up));
}

void WorldScene::Camera::moveUp(float distance)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_position += distance * up;
}

void WorldScene::Camera::movePosition(const glm::vec3 & move)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_position += move.z * glm::normalize(glm::vec3(direction().x, 0.0f, direction().z));
	m_position += move.x * glm::normalize(glm::cross(direction(), up));
	m_position += move.y * up;
}

void WorldScene::Camera::moveDirection(float x_offset, float y_offset)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// update the pitch and yaw
	pitch += -y_offset;
	yaw += x_offset;
}

void WorldScene::Camera::setPosition(const glm::vec3& position)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	this->m_position = position;
}

void WorldScene::Camera::lookAt(const glm::vec3& target)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	glm::vec3 direction = glm::normalize(target - m_position);
	pitch = glm::degrees(asin(direction.y));
	yaw = glm::degrees(atan2(direction.z, direction.x));
}

glm::vec3 WorldScene::Camera::direction() const
{
	return glm::vec3(
		cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
		sin(glm::radians(pitch)),
		cos(glm::radians(pitch)) * sin(glm::radians(yaw))
	);
}

glm::vec3 WorldScene::Camera::position()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_position;
}
