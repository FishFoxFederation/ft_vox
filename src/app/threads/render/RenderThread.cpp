#include "RenderThread.hpp"
#include "logger.hpp"

#include <iostream>

RenderThread::RenderThread(
	vk::RenderAPI & renderAPI,
	const WorldScene & worldScene
):
	AThreadWrapper(),
	m_renderAPI(renderAPI),
	m_world_scene(worldScene)
{
}

RenderThread::~RenderThread()
{
}

void RenderThread::init()
{
	vk::Mesh::ID mesh_id = m_renderAPI.loadModel("assets/models/cube.obj");
	LOG_DEBUG("Mesh ID: " << mesh_id);


	vk::UniformBuffer::CreateInfo uniform_buffer_info{};
	uniform_buffer_info.size = sizeof(ViewProj_UBO);
	uniform_buffer_info.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	m_proj_view_ubo_id = m_renderAPI.createUniformBuffer(uniform_buffer_info);

	vk::Texture::CreateInfo texture_info{};
	texture_info.filepath = "assets/textures/grass.jpg";
	texture_info.mipLevel = 1;
	texture_info.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	m_texture_id = m_renderAPI.loadTexture(texture_info);


	vk::Pipeline::CreateInfo pipeline_create_info{};
	pipeline_create_info.vertexShaderPath = "shaders/simple_shader.vert.spv";
	pipeline_create_info.fragmentShaderPath = "shaders/simple_shader.frag.spv";
	pipeline_create_info.descriptorSetLayouts = {
		m_renderAPI.getUniformBuffer(m_proj_view_ubo_id).descriptor()->layout(),
		m_renderAPI.getTexture(m_texture_id)->descriptor()->layout()
	};
	pipeline_create_info.pushConstantRanges = {
		{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrix_push_constant)}
	};

	m_simple_shader_pipeline_id = m_renderAPI.createPipeline(pipeline_create_info);
}

void RenderThread::loop()
{
	std::vector<WorldScene::MeshRenderData> mesh_render_data = m_world_scene.getMeshRenderData();

	m_renderAPI.startDraw();
	m_renderAPI.startRendering();

	//############################################################################

	int width, height;
	glfwGetFramebufferSize(m_renderAPI.getWindow(), &width, &height);

	ViewProj_UBO ubo{};
	ubo.view = m_world_scene.camera().getViewMatrix();
	ubo.proj = m_world_scene.camera().getProjectionMatrix(width / (float) height);
	ubo.proj[1][1] *= -1;

	m_renderAPI.getUniformBuffer(m_proj_view_ubo_id).buffer(m_renderAPI.currentFrame())->write(&ubo, sizeof(ubo));

	m_renderAPI.bindPipeline(m_simple_shader_pipeline_id);

	m_renderAPI.bindDescriptor(
		m_simple_shader_pipeline_id,
		0, 1,
		m_renderAPI.getUniformBuffer(m_proj_view_ubo_id).descriptor()->pSet(m_renderAPI.currentFrame())
	);

	m_renderAPI.bindDescriptor(
		m_simple_shader_pipeline_id,
		1, 1,
		m_renderAPI.getTexture(m_texture_id)->descriptor()->pSet(m_renderAPI.currentFrame())
	);


	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(width);
	viewport.height = static_cast<float>(height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	m_renderAPI.setViewport(viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	m_renderAPI.setScissor(scissor);

	for (auto& data : mesh_render_data)
	{
		ModelMatrix_push_constant pushConstant{};
		pushConstant.model = data.model;
		m_renderAPI.pushConstant(
			m_simple_shader_pipeline_id,
			VK_SHADER_STAGE_VERTEX_BIT,
			sizeof(ModelMatrix_push_constant),
			&pushConstant
		);

		m_renderAPI.drawMesh(data.id);
	}

	//############################################################################

	m_renderAPI.endRendering();
	m_renderAPI.endDraw();
}
