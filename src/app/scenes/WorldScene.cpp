#include "WorldScene.hpp"
#include "logger.hpp"

WorldScene::WorldScene()
{
}

WorldScene::~WorldScene()
{
}

void WorldScene::addMeshData(uint32_t meshID, glm::mat4 model)
{
	std::unique_lock<std::mutex> lock(m_mesh_render_data_mutex);
	m_mesh_render_data.push_back({
		.id = meshID,
		.model = model
	});
}

void WorldScene::removeMesh(uint32_t meshID)
{
	std::unique_lock<std::mutex> lock(m_mesh_render_data_mutex);
	std::remove_if(m_mesh_render_data.begin(), m_mesh_render_data.end(), [meshID](MeshRenderData& mesh_render_data) {
		return mesh_render_data.id == meshID;
	});
}

std::vector<WorldScene::MeshRenderData> WorldScene::getMeshRenderData()
{
	std::unique_lock<std::mutex> lock(m_mesh_render_data_mutex);
	return m_mesh_render_data;
}



void WorldScene::moveCameraForward(float distance)
{
	std::unique_lock<std::mutex> lock(m_camera_mutex);
	m_camera.moveForward(distance);
}

void WorldScene::moveCameraRight(float distance)
{
	std::unique_lock<std::mutex> lock(m_camera_mutex);
	m_camera.moveRight(distance);
}

void WorldScene::moveCameraUp(float distance)
{
	std::unique_lock<std::mutex> lock(m_camera_mutex);
	m_camera.moveUp(distance);
}

void WorldScene::moveCameraDirection(float x, float y)
{
	std::unique_lock<std::mutex> lock(m_camera_mutex);
	m_camera.moveDirection(x, y);
}

WorldScene::Camera WorldScene::getCamera()
{
	std::unique_lock<std::mutex> lock(m_camera_mutex);
	return m_camera;
}