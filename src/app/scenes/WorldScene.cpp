#include "WorldScene.hpp"
#include "logger.hpp"

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
