#include "RenderThread.hpp"
#include "logger.hpp"
#include "Model.hpp"

#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <unistd.h>

RenderThread::RenderThread(
	const Settings & settings,
	VulkanAPI & vulkanAPI,
	const WorldScene & worldScene,
	std::chrono::nanoseconds start_time
):
	m_settings(settings),
	vk(vulkanAPI),
	m_world_scene(worldScene),
	m_debug_gui(),
	m_start_time(start_time),
	m_last_frame_time(start_time),
	m_frame_count(0),
	m_start_time_counting_fps(start_time),
	m_thread(&RenderThread::launch, this)
{
	(void)m_settings;
	(void)m_start_time;
}

RenderThread::~RenderThread()
{
}

void RenderThread::launch()
{
	try
	{
		init();

		while (!m_thread.get_stop_token().stop_requested())
		{
			auto start_time = std::chrono::high_resolution_clock::now();
			loop();
			auto end_time = std::chrono::high_resolution_clock::now();

			//i want 60fps so if not enough time passed wait a bit
			std::chrono::nanoseconds frame_time = end_time - start_time;
			if (frame_time < std::chrono::nanoseconds(16666666))
			{
				std::this_thread::sleep_for(std::chrono::nanoseconds(16666666) - frame_time);
			}
		}
	}
	catch (const std::exception & e)
	{
		LOG_ERROR("RENDER THREAD Thread exception: " << e.what());
	}
	LOG_DEBUG("Thread stopped");
}

void RenderThread::init()
{
	LOG_INFO("RenderThread launched :" << gettid());
	tracy::SetThreadName(str_render_thread);
}

void RenderThread::loop()
{
	ZoneScoped;
	std::string current_frame = "Frame " + std::to_string(vk.current_frame);
	ZoneText(current_frame.c_str(), current_frame.size());

	//############################################################################################################
	//                     																                         #
	//                            Do independent logic from the vulkan rendering here                            #
	//                     																                         #
	//############################################################################################################

	int window_width, window_height;
	double aspect_ratio;
	glm::mat4 clip;
	Camera::RenderInfo camera;
	ViewProjMatrices camera_matrices = {};
	std::vector<WorldScene::MeshRenderData> chunk_meshes;
	std::vector<WorldScene::MeshRenderData> entity_meshes;
	std::vector<WorldScene::PlayerRenderData> players;
	ViewProjMatrices sun = {};
	glm::dvec3 sun_position;
	std::optional<glm::vec3> target_block;
	std::vector<WorldScene::DebugBlock> debug_blocks;
	AtmosphereParams atmosphere_params = {};

	{
		ZoneScopedN("Prepare frame");

		updateTime();

		glfwGetFramebufferSize(vk.window, &window_width, &window_height);

		aspect_ratio = static_cast<double>(window_width) / static_cast<double>(window_height);

		clip = glm::mat4(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f,-1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.0f, 0.0f, 0.5f, 1.0f
		);

		camera = m_world_scene.camera().getRenderInfo(aspect_ratio);

		camera_matrices.view = camera.view;
		camera_matrices.proj = clip * camera.projection;

		chunk_meshes = m_world_scene.chunk_mesh_list.values();
		entity_meshes = m_world_scene.entity_mesh_list.values();
		players = m_world_scene.getPlayers();

		m_frame_count++;
		if (m_current_time - m_start_time_counting_fps >= std::chrono::seconds(1))
		{
			DebugGui::fps = static_cast<double>(m_frame_count) / std::chrono::duration_cast<std::chrono::seconds>(m_current_time - m_start_time_counting_fps).count();
			m_frame_count = 0;
			m_start_time_counting_fps = m_current_time;
		}

		DebugGui::frame_time_history.push(m_delta_time.count() / 1e6);

		// const glm::dvec3 sun_offset = glm::dvec3(
		// 	0.0f,
		// 	100.0 * glm::cos(glm::radians(20.0) * m_current_time.count() / 1e9),
		// 	100.0 * glm::sin(glm::radians(20.0) * m_current_time.count() / 1e9)
		// );
		const glm::dvec3 sun_offset = glm::dvec3(
			0.0f,
			100.0 * glm::cos(glm::radians(DebugGui::sun_theta.load())),
			100.0 * glm::sin(glm::radians(DebugGui::sun_theta.load()))
		);
		sun_position = camera.position + sun_offset;
		const float sun_size = 300.0f;
		const float sun_near = 10.0f;
		const float sun_far = 1000.0f;

		sun = camera_matrices;
		sun.view = glm::lookAt(
			sun_position,
			camera.position,
			glm::dvec3(0.0f, 1.0f, 0.0f)
		);
		sun.proj = clip * glm::ortho(
			-sun_size, sun_size,
			-sun_size, sun_size,
			sun_near, sun_far
		);

		target_block = m_world_scene.targetBlock();
		debug_blocks = m_world_scene.debugBlocks();

		atmosphere_params.earth_radius = DebugGui::earth_radius;
		atmosphere_params.atmosphere_radius = DebugGui::atmosphere_radius;
		atmosphere_params.beta_rayleigh = DebugGui::beta_rayleigh;
		atmosphere_params.beta_mie = DebugGui::beta_mie;
		atmosphere_params.sun_intensity = DebugGui::sun_intensity;
		atmosphere_params.h_rayleigh = DebugGui::h_rayleigh;
		atmosphere_params.h_mie = DebugGui::h_mie;
		atmosphere_params.g = DebugGui::g;
		atmosphere_params.n_samples = DebugGui::n_samples;
		atmosphere_params.n_light_samples = DebugGui::n_light_samples;
	}

	//############################################################################################################
	//                     																                         #
	//                                  Start the vulkan rendering process here                                  #
	//                     																                         #
	//############################################################################################################

	std::lock_guard lock(vk.global_mutex);

	DebugGui::chunk_mesh_count = vk.mesh_map.size();

	{
		ZoneScopedN("Wait for in_flight_fences");

		VK_CHECK(
			vkWaitForFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max()),
			"Failed to wait for in flight fence"
		);
	}
	vkResetFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame]);

	const std::chrono::nanoseconds start_cpu_rendering_time = std::chrono::steady_clock::now().time_since_epoch();

	{ // reset mesh usage by frame info
		std::lock_guard lock(vk.mesh_map_mutex);
		for (auto & [id, mesh] : vk.mesh_map)
		{
			mesh.used_by_frame[vk.current_frame] = false;
		}
	}

	VK_CHECK(
		vkResetCommandBuffer(vk.draw_command_buffers[vk.current_frame], 0),
		"Failed to reset draw command buffer"
	);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(vk.draw_command_buffers[vk.current_frame], &begin_info),
		"Failed to begin recording command buffer"
	);

	memcpy(vk.camera_ubo.mapped_memory[vk.current_frame], &camera_matrices, sizeof(camera_matrices));
	memcpy(vk.sun_ubo.mapped_memory[vk.current_frame], &sun, sizeof(sun));
	memcpy(vk.atmosphere_ubo.mapped_memory[vk.current_frame], &atmosphere_params, sizeof(atmosphere_params));


	shadowPass(chunk_meshes);
	lightingPass(camera, chunk_meshes, entity_meshes, players, target_block, debug_blocks, sun_position);

	TracyVkCollect(vk.ctx, vk.draw_command_buffers[vk.current_frame]);

	VK_CHECK(
		vkEndCommandBuffer(vk.draw_command_buffers[vk.current_frame]),
		"Failed to record command buffer"
	);

	VkSubmitInfo render_submit_info = {};
	render_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	render_submit_info.commandBufferCount = 1;
	render_submit_info.pCommandBuffers = &vk.draw_command_buffers[vk.current_frame];
	render_submit_info.signalSemaphoreCount = 1;
	render_submit_info.pSignalSemaphores = &vk.main_render_finished_semaphores[vk.current_frame];

	//############################################################################################################
	//                     																                         #
	//                     Copy the color image attachment to the swap chain image with blit                     #
	//                     																                         #
	//############################################################################################################

	copyToSwapchain();

	VkSubmitInfo copy_submit_info = {};
	copy_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copy_submit_info.commandBufferCount = 1;
	copy_submit_info.pCommandBuffers = &vk.copy_command_buffers[vk.current_frame];

	const std::array<VkSemaphore, 2> copy_wait_semaphores = {
		vk.image_available_semaphores[vk.current_frame],
		vk.main_render_finished_semaphores[vk.current_frame]
	};
	const std::array<VkPipelineStageFlags, 2> copy_wait_stages = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	};
	copy_submit_info.waitSemaphoreCount = static_cast<uint32_t>(copy_wait_semaphores.size());
	copy_submit_info.pWaitSemaphores = copy_wait_semaphores.data();
	copy_submit_info.pWaitDstStageMask = copy_wait_stages.data();

	copy_submit_info.signalSemaphoreCount = 1;
	copy_submit_info.pSignalSemaphores = &vk.copy_finished_semaphores[vk.current_frame];

	//############################################################################################################
	//                     																                         #
	//                                        Do the ImGui rendering here                                        #
	//                     																                         #
	//############################################################################################################

	drawDebugGui();

	VkSubmitInfo imgui_submit_info = {};
	imgui_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	imgui_submit_info.commandBufferCount = 1;
	imgui_submit_info.pCommandBuffers = &vk.imgui_command_buffers[vk.current_frame];
	imgui_submit_info.waitSemaphoreCount = 1;
	imgui_submit_info.pWaitSemaphores = &vk.copy_finished_semaphores[vk.current_frame];
	const VkPipelineStageFlags vk_pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	imgui_submit_info.pWaitDstStageMask = &vk_pipeline_stage_flags;
	imgui_submit_info.signalSemaphoreCount = 1;
	imgui_submit_info.pSignalSemaphores = &vk.imgui_render_finished_semaphores[vk.current_frame];


	const std::array<VkSubmitInfo, 3> submit_infos = {
		render_submit_info,
		copy_submit_info,
		imgui_submit_info
	};

	VK_CHECK(
		vkQueueSubmit(vk.graphics_queue, static_cast<uint32_t>(submit_infos.size()), submit_infos.data(), vk.in_flight_fences[vk.current_frame]),
		"Failed to submit all command buffers"
	);

	//############################################################################################################
	//                     																                         #
	//                             Present the swap chain image to the present queue                             #
	//                     																                         #
	//############################################################################################################

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &vk.imgui_render_finished_semaphores[vk.current_frame];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &vk.swapchain.swapchain;
	present_info.pImageIndices = &vk.current_image_index;

	VkResult result = vkQueuePresentKHR(vk.present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		vk.recreateSwapChain(vk.window);
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image");
	}

	FrameMark;

	// Increment the current frame
	vk.current_frame = (vk.current_frame + 1) % vk.max_frames_in_flight;

	const std::chrono::nanoseconds end_cpu_rendering_time = std::chrono::steady_clock::now().time_since_epoch();
	DebugGui::cpu_time_history.push((end_cpu_rendering_time - start_cpu_rendering_time).count() / 1e6);
}

void RenderThread::updateTime()
{
	ZoneScoped;

	m_current_time = std::chrono::steady_clock::now().time_since_epoch();
	m_delta_time = m_current_time - m_last_frame_time;
	m_last_frame_time = m_current_time;
}

void RenderThread::shadowPass(
	const std::vector<WorldScene::MeshRenderData> & chunk_meshes
)
{
	ZoneScoped;
	TracyVkZone(vk.ctx, vk.draw_command_buffers[vk.current_frame], "Shadow pass");

	VkRenderPassBeginInfo shadow_render_pass_begin_info = {};
	shadow_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	shadow_render_pass_begin_info.renderPass = vk.shadow_render_pass;
	shadow_render_pass_begin_info.framebuffer = vk.shadow_framebuffers[vk.current_frame];
	shadow_render_pass_begin_info.renderArea.offset = { 0, 0 };
	shadow_render_pass_begin_info.renderArea.extent = vk.shadow_map_depth_attachement.extent2D;
	std::vector<VkClearValue> shadow_clear_values = {
		{ 1.0f, 0 }
	};
	shadow_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(shadow_clear_values.size());
	shadow_render_pass_begin_info.pClearValues = shadow_clear_values.data();

	vkCmdBeginRenderPass(
		vk.draw_command_buffers[vk.current_frame],
		&shadow_render_pass_begin_info,
		VK_SUBPASS_CONTENTS_INLINE
	);

	// Draw the chunks
	// vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.shadow_pipeline.pipeline);

	// const std::vector<VkDescriptorSet> shadow_descriptor_sets = {
	// 	vk.sun_descriptor.sets[vk.current_frame]
	// };

	// vkCmdBindDescriptorSets(
	// 	vk.draw_command_buffers[vk.current_frame],
	// 	VK_PIPELINE_BIND_POINT_GRAPHICS,
	// 	vk.shadow_pipeline.layout,
	// 	0,
	// 	static_cast<uint32_t>(shadow_descriptor_sets.size()),
	// 	shadow_descriptor_sets.data(),
	// 	0,
	// 	nullptr
	// );

	// for (auto & chunk_mesh : chunk_meshes)
	// {
	// 	ModelMatrice model_matrice = {};
	// 	model_matrice.model = chunk_mesh.model;

	// 	vk.drawMesh(
	// 		vk.shadow_pipeline,
	// 		chunk_mesh.id,
	// 		&model_matrice,
	// 		sizeof(ModelMatrice),
	// 		VK_SHADER_STAGE_VERTEX_BIT
	// 	);
	// }

	vkCmdEndRenderPass(vk.draw_command_buffers[vk.current_frame]);
}

void RenderThread::lightingPass(
	const Camera::RenderInfo & camera,
	const std::vector<WorldScene::MeshRenderData> & chunk_meshes,
	const std::vector<WorldScene::MeshRenderData> & entity_meshes,
	const std::vector<WorldScene::PlayerRenderData> & players,
	const std::optional<glm::vec3> & target_block,
	const std::vector<WorldScene::DebugBlock> & debug_blocks,
	const glm::dvec3 & sun_position
)
{
	ZoneScoped;
	TracyVkZone(vk.ctx, vk.draw_command_buffers[vk.current_frame], "Lighting pass");

	VkRenderPassBeginInfo lighting_render_pass_begin_info = {};
	lighting_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	lighting_render_pass_begin_info.renderPass = vk.lighting_render_pass;
	lighting_render_pass_begin_info.framebuffer = vk.lighting_framebuffers[vk.current_frame];
	lighting_render_pass_begin_info.renderArea.offset = { 0, 0 };
	lighting_render_pass_begin_info.renderArea.extent = vk.color_attachement.extent2D;
	std::vector<VkClearValue> lighting_clear_values = {
		{ 0.0f, 0.0f, 0.0f, 1.0f },
		{ 1.0f, 0 }
	};
	lighting_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(lighting_clear_values.size());
	lighting_render_pass_begin_info.pClearValues = lighting_clear_values.data();

	vkCmdBeginRenderPass(
		vk.draw_command_buffers[vk.current_frame],
		&lighting_render_pass_begin_info,
		VK_SUBPASS_CONTENTS_INLINE
	);

	{ // Draw the chunks
		ZoneScopedN("Draw chunks");

		vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.chunk_pipeline.pipeline);

		const std::vector<VkDescriptorSet> descriptor_sets = {
			vk.camera_descriptor.sets[vk.current_frame],
			vk.sun_descriptor.sets[vk.current_frame],
			vk.block_textures_descriptor.set,
			vk.shadow_map_descriptor.set
		};

		vkCmdBindDescriptorSets(
			vk.draw_command_buffers[vk.current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk.chunk_pipeline.layout,
			0,
			static_cast<uint32_t>(descriptor_sets.size()),
			descriptor_sets.data(),
			0,
			nullptr
		);

		for (auto & chunk_mesh: chunk_meshes)
		{
			ModelMatrice model_matrice = {};
			model_matrice.model = chunk_mesh.model;

			vk.drawMesh(
				vk.chunk_pipeline,
				chunk_mesh.id,
				&model_matrice,
				sizeof(ModelMatrice),
				VK_SHADER_STAGE_VERTEX_BIT
			);
		}
	}

	{ // Draw the entities
		ZoneScopedN("Draw entities");

		vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.entity_pipeline.pipeline);

		const std::vector<VkDescriptorSet> entity_descriptor_sets = {
			vk.camera_descriptor.sets[vk.current_frame]
		};

		vkCmdBindDescriptorSets(
			vk.draw_command_buffers[vk.current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk.entity_pipeline.layout,
			0,
			static_cast<uint32_t>(entity_descriptor_sets.size()),
			entity_descriptor_sets.data(),
			0,
			nullptr
		);

		for (const auto & entity_mesh : entity_meshes)
		{
			EntityMatrices entity_matrice = {};
			entity_matrice.model = entity_mesh.model;
			entity_matrice.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

			vk.drawMesh(
				vk.entity_pipeline,
				entity_mesh.id,
				&entity_matrice,
				sizeof(EntityMatrices),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
			);
		}

		for (auto & data: debug_blocks)
		{
			const glm::vec3 size = glm::vec3(data.size);
			const glm::mat4 block_scale = glm::scale(glm::mat4(1.0f), size);
			const glm::mat4 block_model = glm::translate(glm::mat4(1.0f), data.position - size / 2.0f) * block_scale;

			EntityMatrices block_matrice = {};
			block_matrice.model = block_model;
			block_matrice.color = data.color;
			vkCmdPushConstants(
				vk.draw_command_buffers[vk.current_frame],
				vk.entity_pipeline.layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(EntityMatrices),
				&block_matrice
			);

			vkCmdDrawIndexed(
				vk.draw_command_buffers[vk.current_frame],
				36,
				1,
				0,
				0,
				0
			);
		}
	}

	{ // Draw the players
		ZoneScopedN("Draw players");

		vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.player_pipeline.pipeline);

		const std::vector<VkDescriptorSet> player_descriptor_sets = {
			vk.camera_descriptor.sets[vk.current_frame],
			vk.player_skin_image_descriptor.set
		};

		vkCmdBindDescriptorSets(
			vk.draw_command_buffers[vk.current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk.player_pipeline.layout,
			0,
			static_cast<uint32_t>(player_descriptor_sets.size()),
			player_descriptor_sets.data(),
			0,
			nullptr
		);

		for (const auto & player : players)
		{
			if (!player.visible)
			{
				continue;
			}

			// Body
			const glm::mat4 body_model = Mat4()
				.translate(player.position)
				.rotate(-glm::radians(player.yaw + 90), glm::dvec3(0.0f, 1.0f, 0.0f))
				.mat();

			// Chest
			const glm::mat4 chest_model = Mat4()
				.translate(PlayerModel::chest_pos)
				.mat();
			drawPlayerBodyPart(
				vk.player_chest_mesh_id,
				body_model * chest_model
			);

			// Head
			const glm::mat4 head_model = Mat4()
				.translate(PlayerModel::head_pos)
				.rotate(glm::radians(player.pitch), glm::dvec3(1.0f, 0.0f, 0.0f))
				.mat();
			drawPlayerBodyPart(
				vk.player_head_mesh_id,
				body_model * chest_model * head_model
			);

			// Legs animation angle
			double legs_angle = 0.0;
			if (player.walk_animation.isActive())
			{
				legs_angle = player.walk_animation.angle();
			}

			// Left leg
			const glm::mat4 left_leg_model = Mat4()
				.rotate(legs_angle, glm::dvec3(1.0f, 0.0f, 0.0f))
				.translate(PlayerModel::left_leg_pos)
				.mat();
			drawPlayerBodyPart(
				vk.player_left_leg_mesh_id,
				body_model * chest_model * left_leg_model
			);

			// Right leg
			const glm::mat4 right_leg_model = Mat4()
				.rotate(-legs_angle, glm::dvec3(1.0f, 0.0f, 0.0f))
				.translate(PlayerModel::right_leg_pos)
				.mat();
			drawPlayerBodyPart(
				vk.player_right_leg_mesh_id,
				body_model * chest_model * right_leg_model
			);

			// Arm animation angle
			double arms_angle_x = 0.0;
			double arms_angle_y = 0.0;
			double arms_angle_z = 0.0;
			if (player.walk_animation.isActive())
			{
				arms_angle_x = player.walk_animation.angle();
			}

			// Left arm
			const glm::mat4 left_arm_model = Mat4()
				.translate(PlayerModel::left_arm_pos)
				.translate({0.0f, PlayerModel::arm_size.y, 0.0f})
				.rotate(-arms_angle_x, glm::dvec3(1.0f, 0.0f, 0.0f))
				.rotate(-glm::radians(2.0), glm::dvec3(0.0f, 0.0f, 1.0f))
				.mat();
			drawPlayerBodyPart(
				vk.player_right_arm_mesh_id,
				body_model * chest_model * left_arm_model
			);

			if (player.attack_animation.isActive())
			{
				arms_angle_x = player.attack_animation.angleX();
				arms_angle_y = player.attack_animation.angleY();
				arms_angle_z = player.attack_animation.angleZ();
			}

			// Right arm
			const glm::mat4 right_arm_model = Mat4()
				.translate(PlayerModel::right_arm_pos)
				.translate({0.0f, PlayerModel::arm_size.y, 0.0f})
				.rotate(arms_angle_x, glm::dvec3(1.0f, 0.0f, 0.0f))
				.rotate(arms_angle_z, glm::dvec3(0.0f, 0.0f, 1.0f))
				.rotate(glm::radians(2.0), glm::dvec3(0.0f, 0.0f, 1.0f))
				.rotate(arms_angle_y, glm::dvec3(0.0f, 1.0f, 0.0f))
				.mat();
			drawPlayerBodyPart(
				vk.player_right_arm_mesh_id,
				body_model * chest_model * right_arm_model
			);
		}
	}

	// Draw the targeted block
	if (target_block.has_value())
	{
		vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.line_pipeline.pipeline);

		const std::vector<VkDescriptorSet> line_descriptor_sets = {
			vk.camera_descriptor.sets[vk.current_frame]
		};

		vkCmdBindDescriptorSets(
			vk.draw_command_buffers[vk.current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk.line_pipeline.layout,
			0,
			static_cast<uint32_t>(line_descriptor_sets.size()),
			line_descriptor_sets.data(),
			0,
			nullptr
		);

		Mesh mesh;
		{
			std::lock_guard lock(vk.mesh_map_mutex);
			mesh = vk.mesh_map.at(vk.cube_mesh_id);
		}

		const VkBuffer vertex_buffers[] = { mesh.buffer };
		const VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(
			vk.draw_command_buffers[vk.current_frame],
			0, 1,
			vertex_buffers,
			offsets
		);

		vkCmdBindIndexBuffer(
			vk.draw_command_buffers[vk.current_frame],
			mesh.buffer,
			mesh.index_offset,
			VK_INDEX_TYPE_UINT32
		);

		ModelMatrice target_block_matrice = {};
		target_block_matrice.model = glm::translate(glm::mat4(1.0f), target_block.value()) * glm::scale(glm::mat4(1.0f), glm::vec3(1.001f));
		vkCmdPushConstants(
			vk.draw_command_buffers[vk.current_frame],
			vk.line_pipeline.layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(ModelMatrice),
			&target_block_matrice
		);

		vkCmdSetLineWidth(vk.draw_command_buffers[vk.current_frame], 2.0f);

		vkCmdDrawIndexed(
			vk.draw_command_buffers[vk.current_frame],
			static_cast<uint32_t>(mesh.index_count),
			1, 0, 0, 0
		);
	}


	{ // Draw the skybox
		// vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.skybox_pipeline.pipeline);

		// const std::array<VkDescriptorSet, 2> skybox_descriptor_sets = {
		// 	vk.camera_descriptor.sets[vk.current_frame],
		// 	vk.cube_map_descriptor.set
		// };

		// vkCmdBindDescriptorSets(
		// 	vk.draw_command_buffers[vk.current_frame],
		// 	VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 	vk.skybox_pipeline.layout,
		// 	0,
		// 	static_cast<uint32_t>(skybox_descriptor_sets.size()),
		// 	skybox_descriptor_sets.data(),
		// 	0,
		// 	nullptr
		// );

		// ModelMatrice skybox_matrices = {};
		// skybox_matrices.model = glm::translate(glm::dmat4(1.0f), camera.position);
		// vkCmdPushConstants(
		// 	vk.draw_command_buffers[vk.current_frame],
		// 	vk.skybox_pipeline.layout,
		// 	VK_SHADER_STAGE_VERTEX_BIT,
		// 	0,
		// 	sizeof(ModelMatrice),
		// 	&skybox_matrices
		// );

		// vkCmdDraw(
		// 	vk.draw_command_buffers[vk.current_frame],
		// 	36,
		// 	1,
		// 	0,
		// 	0
		// );
	}

	{ // Draw the sun
		vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.sun_pipeline.pipeline);

		const std::vector<VkDescriptorSet> sun_descriptor_sets = {
			vk.camera_descriptor.sets[vk.current_frame],
			vk.atmosphere_descriptor.sets[vk.current_frame]
		};

		vkCmdBindDescriptorSets(
			vk.draw_command_buffers[vk.current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk.sun_pipeline.layout,
			0,
			static_cast<uint32_t>(sun_descriptor_sets.size()),
			sun_descriptor_sets.data(),
			0,
			nullptr
		);

		SkyShaderData sky_shader_data = {};
		sky_shader_data.model = glm::translate(glm::dmat4(1.0f), camera.position);
		sky_shader_data.sun_direction = glm::normalize(sun_position - camera.position);

		vk.drawMesh(
			vk.sun_pipeline,
			vk.icosphere_mesh_id,
			&sky_shader_data,
			sizeof(SkyShaderData),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
		);
	}


	{ // Draw gui
		vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.gui_pipeline.pipeline);

		const std::vector<VkDescriptorSet> gui_descriptor_sets = {
			vk.crosshair_image_descriptor.set
		};

		vkCmdBindDescriptorSets(
			vk.draw_command_buffers[vk.current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk.gui_pipeline.layout,
			0,
			static_cast<uint32_t>(gui_descriptor_sets.size()),
			gui_descriptor_sets.data(),
			0,
			nullptr
		);

		float min_size = std::min(vk.color_attachement.extent2D.width, vk.color_attachement.extent2D.height);
		float size = min_size / 40.0f;

		VkViewport viewport = {};
		viewport.x = static_cast<float>(vk.color_attachement.extent2D.width / 2 - (size / 2));
		viewport.y = static_cast<float>(vk.color_attachement.extent2D.height / 2 - (size / 2));
		viewport.width = size;
		viewport.height = size;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(vk.draw_command_buffers[vk.current_frame], 0, 1, &viewport);

		vkCmdDraw(
			vk.draw_command_buffers[vk.current_frame],
			6,
			1,
			0,
			0
		);
	}


	// Draw test image
	// vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.test_image_pipeline.pipeline);

	// const std::vector<VkDescriptorSet> test_image_descriptor_sets = {
	// 	vk.test_image_descriptor.set
	// };

	// vkCmdBindDescriptorSets(
	// 	vk.draw_command_buffers[vk.current_frame],
	// 	VK_PIPELINE_BIND_POINT_GRAPHICS,
	// 	vk.test_image_pipeline.layout,
	// 	0,
	// 	static_cast<uint32_t>(test_image_descriptor_sets.size()),
	// 	test_image_descriptor_sets.data(),
	// 	0,
	// 	nullptr
	// );

	// vkCmdDraw(
	// 	vk.draw_command_buffers[vk.current_frame],
	// 	6,
	// 	1,
	// 	0,
	// 	0
	// );


	vkCmdEndRenderPass(vk.draw_command_buffers[vk.current_frame]);
}

void RenderThread::drawPlayerBodyPart(
	const uint64_t mesh_id,
	const glm::mat4 & model
)
{
	ModelMatrice player_matrice = {};
	player_matrice.model = model;

	vk.drawMesh(
		vk.player_pipeline,
		mesh_id,
		&player_matrice,
		sizeof(ModelMatrice),
		VK_SHADER_STAGE_VERTEX_BIT
	);
}

void RenderThread::copyToSwapchain()
{
	ZoneScoped;

	// Acquire the next swap chain image
	VkResult result = vkAcquireNextImageKHR(
		vk.device,
		vk.swapchain.swapchain,
		std::numeric_limits<uint64_t>::max(),
		vk.image_available_semaphores[vk.current_frame],
		VK_NULL_HANDLE,
		&vk.current_image_index
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		vk.recreateSwapChain(vk.window);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}


	// Copy the color image to the swap chain image with blit
	vkResetCommandBuffer(vk.copy_command_buffers[vk.current_frame], 0);

	VkCommandBufferBeginInfo copy_begin_info = {};
	copy_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(vk.copy_command_buffers[vk.current_frame], &copy_begin_info),
		"Failed to begin recording copy command buffer"
	);

	vk.setImageLayout(
		vk.copy_command_buffers[vk.current_frame],
		vk.swapchain.images[vk.current_image_index],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		0,
		0,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);

	VkImageBlit blit = {};
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcOffsets[1] = {
		static_cast<int32_t>(vk.color_attachement.extent2D.width),
		static_cast<int32_t>(vk.color_attachement.extent2D.height),
		1
	};
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;
	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = {
		static_cast<int32_t>(vk.swapchain.extent.width),
		static_cast<int32_t>(vk.swapchain.extent.height),
		1
	};
	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = 0;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	vkCmdBlitImage(
		vk.copy_command_buffers[vk.current_frame],
		vk.color_attachement.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vk.swapchain.images[vk.current_image_index],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&blit,
		VK_FILTER_LINEAR
	);

	VK_CHECK(
		vkEndCommandBuffer(vk.copy_command_buffers[vk.current_frame]),
		"Failed to record copy command buffer"
	);
}

void RenderThread::drawDebugGui()
{
	ZoneScoped;

	vkResetCommandBuffer(vk.imgui_command_buffers[vk.current_frame], 0);

	VkCommandBufferBeginInfo imgui_begin_info = {};
	imgui_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(vk.imgui_command_buffers[vk.current_frame], &imgui_begin_info),
		"Failed to begin recording command buffer"
	);

	vk.setImageLayout(
		vk.imgui_command_buffers[vk.current_frame],
		vk.swapchain.images[vk.current_image_index],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		0,
		0,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	);

	vk.setImageLayout(
		vk.imgui_command_buffers[vk.current_frame],
		vk.imgui_texture.image,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		0,
		0,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);

	VkRenderingAttachmentInfo imgui_color_attachment = {};
	imgui_color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	imgui_color_attachment.imageView = vk.swapchain.image_views[vk.current_image_index];
	imgui_color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imgui_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	imgui_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo imgui_render_info = {};
	imgui_render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	imgui_render_info.renderArea = { 0, 0, vk.swapchain.extent.width, vk.swapchain.extent.height };
	imgui_render_info.layerCount = 1;
	imgui_render_info.colorAttachmentCount = 1;
	imgui_render_info.pColorAttachments = &imgui_color_attachment;

	vkCmdBeginRendering(vk.imgui_command_buffers[vk.current_frame], &imgui_render_info);


	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	m_debug_gui.updateImGui();

	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vk.imgui_command_buffers[vk.current_frame]);


	vkCmdEndRendering(vk.imgui_command_buffers[vk.current_frame]);

	vk.setImageLayout(
		vk.imgui_command_buffers[vk.current_frame],
		vk.swapchain.images[vk.current_image_index],
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		0,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
	);

	vk.setImageLayout(
		vk.imgui_command_buffers[vk.current_frame],
		vk.imgui_texture.image,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		0,
		0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
	);

	VK_CHECK(
		vkEndCommandBuffer(vk.imgui_command_buffers[vk.current_frame]),
		"Failed to record imgui command buffer"
	);
}
