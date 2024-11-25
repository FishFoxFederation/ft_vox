#include "VulkanAPI.hpp"
#include "logger.hpp"
#include "ft_format.hpp"

#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <unistd.h>

void VulkanAPI::_createRenderFrameRessources()
{
	m_shadow_visible_chunks.resize(shadow_maps_count);

	// Tuto pour Augustus:

	// Créer une texture de 100x100
	DebugGui::continentalness_texture_id = createImGuiTexture(DebugGui::NOISE_SIZE, DebugGui::NOISE_SIZE);
	DebugGui::erosion_texture_id = createImGuiTexture(DebugGui::NOISE_SIZE, DebugGui::NOISE_SIZE);
	DebugGui::PV_texture_id = createImGuiTexture(DebugGui::NOISE_SIZE, DebugGui::NOISE_SIZE);
	DebugGui::temperature_texture_id = createImGuiTexture(DebugGui::NOISE_SIZE, DebugGui::NOISE_SIZE);
	DebugGui::humidity_texture_id = createImGuiTexture(DebugGui::NOISE_SIZE, DebugGui::NOISE_SIZE);
	DebugGui::weirdness_texture_id = createImGuiTexture(DebugGui::NOISE_SIZE, DebugGui::NOISE_SIZE);
	DebugGui::biome_texture_id = createImGuiTexture(DebugGui::NOISE_SIZE, DebugGui::NOISE_SIZE);
	// imgui_texture_id est declaré dans DebugGui.hpp, rajoute d'autres id si tu veux plus de textures

	// Clear la texture
	// ImGuiTextureClear(DebugGui::continentalness_texture_id);


	for (int x = 0; x < DebugGui::NOISE_SIZE; x++)
	{
		for (int y = 0; y < DebugGui::NOISE_SIZE; y++)
		{
			ImGuiTexturePutPixel(DebugGui::continentalness_texture_id, x, y, 255, 255, 255);
			ImGuiTexturePutPixel(DebugGui::erosion_texture_id, x, y, 255, 255, 255);
			ImGuiTexturePutPixel(DebugGui::PV_texture_id, x, y, 255, 255, 255);
			ImGuiTexturePutPixel(DebugGui::temperature_texture_id, x, y, 255, 255, 255);
			ImGuiTexturePutPixel(DebugGui::humidity_texture_id, x, y, 255, 255, 255);
			ImGuiTexturePutPixel(DebugGui::weirdness_texture_id, x, y, 255, 255, 255);
			ImGuiTexturePutPixel(DebugGui::biome_texture_id, x, y, 255, 255, 255);
		}
	}

	// Draw la texture dans la fenêtre ImGui (voir ligne 136)
	// ImGuiTextureDraw(imgui_texture_id);

	// Normalement toutes ces fontions sont thread safe (mais j'avoue j'ai pas testé)

	// Fin du tuto pour Augustus
}

void VulkanAPI::renderFrame()
{
	ZoneScoped;
	std::string current_frame_str = "Frame " + std::to_string(m_current_frame);
	ZoneText(current_frame_str.c_str(), current_frame_str.size());

	//###########################################################################################################
	//																											#
	//							Do independent logic from the vulkan rendering here								#
	//																											#
	//###########################################################################################################

	_prepareFrame();

	//###########################################################################################################
	//																											#
	//								  Start the vulkan rendering process here									#
	//																											#
	//###########################################################################################################

	_startFrame();

	const std::chrono::nanoseconds start_cpu_rendering_time = std::chrono::steady_clock::now().time_since_epoch();

	memcpy(camera_ubo.mapped_memory[m_current_frame], &m_camera_matrices, sizeof(m_camera_matrices));
	memcpy(light_mat_ubo.mapped_memory[m_current_frame], &m_shadow_map_light, sizeof(m_shadow_map_light));
	memcpy(atmosphere_ubo.mapped_memory[m_current_frame], &m_atmosphere_params, sizeof(m_atmosphere_params));

	//###########################################################################################################
	//																											#
	//												Shadow pass													#
	//																											#
	//###########################################################################################################

	_shadowPass();

	VkSubmitInfo shadow_pass_submit_info = {};
	shadow_pass_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	shadow_pass_submit_info.commandBufferCount = 1;
	shadow_pass_submit_info.pCommandBuffers = &draw_shadow_pass_command_buffers[m_current_frame];
	shadow_pass_submit_info.signalSemaphoreCount = 1;
	shadow_pass_submit_info.pSignalSemaphores = &shadow_pass_finished_semaphores[m_current_frame];

	//###########################################################################################################
	//																											#
	//												Lighting pass												#
	//																											#
	//###########################################################################################################

	_lightingPass();

	VkSubmitInfo render_submit_info = {};
	render_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	render_submit_info.commandBufferCount = 1;
	render_submit_info.pCommandBuffers = &draw_command_buffers[m_current_frame];
	render_submit_info.signalSemaphoreCount = 1;
	render_submit_info.pSignalSemaphores = &main_render_finished_semaphores[m_current_frame];
	render_submit_info.waitSemaphoreCount = 1;
	render_submit_info.pWaitSemaphores = &shadow_pass_finished_semaphores[m_current_frame];
	const VkPipelineStageFlags render_submit_wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	render_submit_info.pWaitDstStageMask = &render_submit_wait_stage;

	//###########################################################################################################
	//																											#
	//						Copy the color image attachment to the swap chain image with blit					#
	//																											#
	//###########################################################################################################

	_copyToSwapchain();

	VkSubmitInfo copy_submit_info = {};
	copy_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copy_submit_info.commandBufferCount = 1;
	copy_submit_info.pCommandBuffers = &copy_command_buffers[m_current_frame];

	const std::array<VkSemaphore, 2> copy_wait_semaphores = {
		image_available_semaphores[m_current_frame],
		main_render_finished_semaphores[m_current_frame]
	};
	const std::array<VkPipelineStageFlags, 2> copy_wait_stages = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	};
	copy_submit_info.waitSemaphoreCount = static_cast<uint32_t>(copy_wait_semaphores.size());
	copy_submit_info.pWaitSemaphores = copy_wait_semaphores.data();
	copy_submit_info.pWaitDstStageMask = copy_wait_stages.data();

	copy_submit_info.signalSemaphoreCount = 1;
	copy_submit_info.pSignalSemaphores = &copy_finished_semaphores[m_current_frame];

	//###########################################################################################################
	//					 																						#
	//										Do the ImGui rendering here											#
	//					 																						#
	//###########################################################################################################

	_drawDebugGui();

	VkSubmitInfo imgui_submit_info = {};
	imgui_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	imgui_submit_info.commandBufferCount = 1;
	imgui_submit_info.pCommandBuffers = &imgui_command_buffers[m_current_frame];
	imgui_submit_info.waitSemaphoreCount = 1;
	imgui_submit_info.pWaitSemaphores = &copy_finished_semaphores[m_current_frame];
	const VkPipelineStageFlags vk_pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	imgui_submit_info.pWaitDstStageMask = &vk_pipeline_stage_flags;
	imgui_submit_info.signalSemaphoreCount = 1;
	imgui_submit_info.pSignalSemaphores = &imgui_render_finished_semaphores[m_current_frame];


	//###########################################################################################################
	//					 																						#
	//											  Submit to queue												#
	//					 																						#
	//###########################################################################################################

	{
		ZoneScopedN("Submit to queue");

		const std::vector<VkSubmitInfo> submit_infos = {
			shadow_pass_submit_info,
			render_submit_info,
			copy_submit_info,
			imgui_submit_info
		};

		VK_CHECK(
			vkQueueSubmit(
				graphics_queue,
				static_cast<uint32_t>(submit_infos.size()),
				submit_infos.data(),
				in_flight_fences[m_current_frame]
			),
			"Failed to submit all command buffers"
		);
	}

	//###########################################################################################################
	//					 																						#
	//							 Present the swap chain image to the present queue								#
	//					 																						#
	//###########################################################################################################

	{
		ZoneScopedN("Present the swap chain image");

		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &imgui_render_finished_semaphores[m_current_frame];
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &swapchain.swapchain;
		present_info.pImageIndices = &m_current_image_index;

		VkResult result = vkQueuePresentKHR(present_queue, &present_info);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			_recreateSwapChain(window);
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image");
		}
	}

	FrameMark;

	_endFrame();

	const std::chrono::nanoseconds end_cpu_rendering_time = std::chrono::steady_clock::now().time_since_epoch();
	DebugGui::cpu_time_history.push((end_cpu_rendering_time - start_cpu_rendering_time).count() / 1e6);
}

void VulkanAPI::_prepareFrame()
{
	ZoneScoped;

	_updateTime();
	m_frame_count++;
	if (m_current_time - m_start_time_counting_fps >= std::chrono::seconds(1))
	{
		DebugGui::fps = static_cast<double>(m_frame_count) / std::chrono::duration_cast<std::chrono::seconds>(m_current_time - m_start_time_counting_fps).count();
		m_frame_count = 0;
		m_start_time_counting_fps = m_current_time;
	}

	_updateRenderData();


	DebugGui::frame_time_history.push(m_delta_time.count() / 1e6);

	const glm::dvec3 sun_offset = glm::dvec3(
		10.0f,
		100.0 * glm::cos(glm::radians(DebugGui::sun_theta.load())),
		100.0 * glm::sin(glm::radians(DebugGui::sun_theta.load()))
	);
	m_sun_position = m_camera_render_info.position + sun_offset;
	const float sun_size = 50.0f;
	const float sun_near = 10.0f;
	const float sun_far = 1000.0f;

	m_sun_matrices = m_camera_matrices;
	m_sun_matrices.view = glm::lookAt(
		m_sun_position,
		m_camera_render_info.position,
		glm::dvec3(0.0f, 1.0f, 0.0f)
	);
	m_sun_matrices.proj = m_clip * glm::ortho(
		-sun_size, sun_size,
		-sun_size, sun_size,
		sun_near, sun_far
	);

	m_atmosphere_params.sun_direction = glm::normalize(m_sun_position - m_camera_render_info.position);
	m_atmosphere_params.earth_radius = DebugGui::earth_radius;
	m_atmosphere_params.atmosphere_radius = DebugGui::atmosphere_radius;
	m_atmosphere_params.beta_rayleigh = DebugGui::beta_rayleigh;
	m_atmosphere_params.beta_mie = DebugGui::beta_mie;
	m_atmosphere_params.sun_intensity = DebugGui::sun_intensity;
	m_atmosphere_params.h_rayleigh = DebugGui::h_rayleigh;
	m_atmosphere_params.h_mie = DebugGui::h_mie;
	m_atmosphere_params.g = DebugGui::g;
	m_atmosphere_params.n_samples = DebugGui::n_samples;
	m_atmosphere_params.n_light_samples = DebugGui::n_light_samples;


	std::vector<float> frustum_split = { 0.0f, 0.01f, 0.05f, 0.1f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };
	// std::vector<float> frustum_split = { 0.0f, 0.1f, 0.2f, 0.6f, 1.0f };
	std::vector<float> far_plane_distances;
	if (frustum_split.size() != shadow_maps_count + 1)
	{
		LOG_ERROR("frustum_split.size() != shadow_maps_count + 1");
	}
	m_shadow_map_light.light_dir = glm::normalize(m_sun_position - m_camera_render_info.position);
	m_shadow_map_light.blend_distance = 5.0f;
	m_light_view_proj_matrices = _getCSMLightViewProjMatrices(
		m_shadow_map_light.light_dir,
		frustum_split,
		m_shadow_map_light.blend_distance,
		m_camera_render_info.view,
		m_camera_render_info.fov,
		m_aspect_ratio,
		m_camera_render_info.near_plane,
		m_camera_render_info.far_plane,
		far_plane_distances
	);
	for (size_t i = 0; i < shadow_maps_count; i++)
	{
		m_shadow_map_light.view_proj[i] = m_clip * m_light_view_proj_matrices[i];
		m_shadow_map_light.plane_distances[i].x = far_plane_distances[i];
	}

	_updateVisibleChunks();
	_updateDebugText();
}

void VulkanAPI::_updateTime()
{
	ZoneScoped;

	m_current_time = std::chrono::steady_clock::now().time_since_epoch();
	m_delta_time = m_current_time - m_last_frame_time;
	m_last_frame_time = m_current_time;
}

void VulkanAPI::_updateDebugText()
{
	m_debug_text = ft_format(
		"fps: %d\n"
		"xyz: %.2f %.2f %.2f\n"
		"Chunk count: %d\n",
		DebugGui::fps.load(),
		DebugGui::player_position.get().x, DebugGui::player_position.get().y, DebugGui::player_position.get().z,
		m_visible_chunks.size()
	);

	m_debug_text += "Shadow chunk count:";
	for (size_t i = 0; i < m_shadow_visible_chunks.size(); i++)
	{
		m_debug_text += ft_format(" %d", m_shadow_visible_chunks[i].size());
	}
	m_debug_text += "\n";

	m_debug_text += ft_format(
		"C: %.2f; E: %.2f; H: %.2f; T: %.2f; PV %.2f; W %.2f\n ",
		DebugGui::continentalness.load(),
		DebugGui::erosion.load(),
		DebugGui::humidity.load(),
		DebugGui::temperature.load(),
		DebugGui::PV.load(),
		DebugGui::weirdness.load()
	);

	switch(DebugGui::biome.load())
	{
		case 0: m_debug_text += "Forest\n"; break;
		case 1: m_debug_text += "Plain\n"; break;
		case 2: m_debug_text += "Mountain\n"; break;
		case 3: m_debug_text += "Ocean\n"; break;
		case 4: m_debug_text += "Coast\n"; break;
		case 5: m_debug_text += "Desert\n"; break;
		case 6: m_debug_text += "River\n"; break;
	};
}

void VulkanAPI::_updateVisibleChunks()
{
	ZoneScoped;

	m_visible_chunks.clear();
	for (auto & [id, model]: m_chunks_in_scene_rendered)
	{
		if (!_isInsideFrustum_planes(m_camera_render_info.projection * m_camera_render_info.view, model, CHUNK_SIZE_VEC3))
		{
			continue;
		}

		m_visible_chunks.push_back(id);
	}

	for (size_t i = 0; i < shadow_maps_count; i++)
	{
		m_shadow_visible_chunks[i].clear();
		for (auto & [id, model]: m_chunks_in_scene_rendered)
		{
			if (!_isInsideFrustum_planes(m_light_view_proj_matrices[i], model, CHUNK_SIZE_VEC3))
			{
				continue;
			}

			m_shadow_visible_chunks[i].push_back(id);
		}
	}
}

void VulkanAPI::_shadowPass()
{
	ZoneScoped;

	VK_CHECK(
		vkResetCommandBuffer(draw_shadow_pass_command_buffers[m_current_frame], 0),
		"Failed to reset shadow pass command buffer"
	);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(draw_shadow_pass_command_buffers[m_current_frame], &begin_info),
		"Failed to begin recording command buffer"
	);

	{
		TracyVkZone(draw_ctx, draw_shadow_pass_command_buffers[m_current_frame], "Shadow pass");

		vkCmdBindPipeline(draw_shadow_pass_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline.pipeline);

		const std::vector<VkDescriptorSet> shadow_descriptor_sets = {
			global_descriptor.sets[m_current_frame]
		};

		vkCmdBindDescriptorSets(
			draw_shadow_pass_command_buffers[m_current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			shadow_pipeline.layout,
			0,
			static_cast<uint32_t>(shadow_descriptor_sets.size()),
			shadow_descriptor_sets.data(),
			0,
			nullptr
		);

		for (size_t shadow_map_index = 0; shadow_map_index < shadow_maps_count; shadow_map_index++)
		{
			TracyVkZone(draw_ctx, draw_shadow_pass_command_buffers[m_current_frame], "Shadow sub pass");

			VkRenderPassBeginInfo shadow_render_pass_begin_info = {};
			shadow_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			shadow_render_pass_begin_info.renderPass = shadow_render_pass;
			shadow_render_pass_begin_info.framebuffer = shadow_framebuffers[m_current_frame * shadow_maps_count + shadow_map_index];
			shadow_render_pass_begin_info.renderArea.offset = { 0, 0 };
			shadow_render_pass_begin_info.renderArea.extent = shadow_map_depth_attachement.extent2D;
			std::vector<VkClearValue> shadow_clear_values = {
				{ 1.0f, 0 }
			};
			shadow_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(shadow_clear_values.size());
			shadow_render_pass_begin_info.pClearValues = shadow_clear_values.data();

			vkCmdBeginRenderPass(
				draw_shadow_pass_command_buffers[m_current_frame],
				&shadow_render_pass_begin_info,
				VK_SUBPASS_CONTENTS_INLINE
			);

			vkCmdPushConstants(
				draw_shadow_pass_command_buffers[m_current_frame],
				shadow_pipeline.layout,
				VK_SHADER_STAGE_ALL,
				offsetof(GlobalPushConstant, layer),
				sizeof(GlobalPushConstant::layer),
				&shadow_map_index
			);

			{ // Draw the chunks
				ZoneScopedN("Draw chunks");
				TracyVkZone(draw_ctx, draw_shadow_pass_command_buffers[m_current_frame], "Draw chunks");

				_drawChunksBlock(
					draw_shadow_pass_command_buffers[m_current_frame],
					m_draw_chunk_block_shadow_pass_buffer[m_current_frame][shadow_map_index],
					m_shadow_visible_chunks[shadow_map_index]
				);
			}

			vkCmdEndRenderPass(draw_shadow_pass_command_buffers[m_current_frame]);
		}
	}

	VK_CHECK(
		vkEndCommandBuffer(draw_shadow_pass_command_buffers[m_current_frame]),
		"Failed to record command buffer"
	);
}

void VulkanAPI::_lightingPass()
{
	ZoneScoped;

	VK_CHECK(
		vkResetCommandBuffer(draw_command_buffers[m_current_frame], 0),
		"Failed to reset draw command buffer"
	);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(draw_command_buffers[m_current_frame], &begin_info),
		"Failed to begin recording command buffer"
	);

	const std::vector<VkDescriptorSet> descriptor_sets = {
		global_descriptor.sets[m_current_frame]
	};

	vkCmdBindDescriptorSets(
		draw_command_buffers[m_current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		chunk_pipeline.layout,
		0,
		static_cast<uint32_t>(descriptor_sets.size()),
		descriptor_sets.data(),
		0,
		nullptr
	);

	{
		TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Lighting pass");

		VkRenderPassBeginInfo lighting_render_pass_begin_info = {};
		lighting_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		lighting_render_pass_begin_info.renderPass = lighting_render_pass;
		lighting_render_pass_begin_info.framebuffer = lighting_framebuffers[m_current_frame];
		lighting_render_pass_begin_info.renderArea.offset = { 0, 0 };
		lighting_render_pass_begin_info.renderArea.extent = output_attachement.extent2D;
		std::vector<VkClearValue> lighting_clear_values = {
			{ 0.0f, 0.0f, 0.0f, 1.0f },
			{ 1.0f, 0 },
			{ 0.0f, 0.0f, 0.0f, 1.0f }
		};
		lighting_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(lighting_clear_values.size());
		lighting_render_pass_begin_info.pClearValues = lighting_clear_values.data();

		vkCmdBeginRenderPass(
			draw_command_buffers[m_current_frame],
			&lighting_render_pass_begin_info,
			VK_SUBPASS_CONTENTS_INLINE
		);
		{
			TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Opaque render pass");

			{ // Draw the chunks
				ZoneScopedN("Draw chunks");
				TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Draw chunks");

				vkCmdBindPipeline(draw_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, chunk_pipeline.pipeline);

				_drawChunksBlock(
					draw_command_buffers[m_current_frame],
					m_draw_chunk_block_light_pass_buffer[m_current_frame],
					m_visible_chunks
				);
			}

			{ // Draw the entities
				ZoneScopedN("Draw entities");
				TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Draw entities");

				vkCmdBindPipeline(draw_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, entity_pipeline.pipeline);

				for (const auto & [id, entity_mesh] : m_entities)
				{
					GlobalPushConstant entity_matrice = {};
					entity_matrice.matrice = entity_mesh.model;
					entity_matrice.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

					_drawMesh(
						draw_command_buffers[m_current_frame],
						entity_pipeline,
						entity_mesh.id,
						&entity_matrice,
						sizeof(GlobalPushConstant),
						VK_SHADER_STAGE_ALL
					);
				}
			}

			{ // Draw the players
				ZoneScopedN("Draw players");
				TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Draw players");

				vkCmdBindPipeline(draw_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, player_pipeline.pipeline);

				for (const auto & [id, player] : m_players)
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
					_drawPlayerBodyPart(
						player_chest_mesh_id,
						body_model * chest_model
					);

					// Head
					const glm::mat4 head_model = Mat4()
						.translate(PlayerModel::head_pos)
						.rotate(glm::radians(player.pitch), glm::dvec3(1.0f, 0.0f, 0.0f))
						.mat();
					_drawPlayerBodyPart(
						player_head_mesh_id,
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
					_drawPlayerBodyPart(
						player_left_leg_mesh_id,
						body_model * chest_model * left_leg_model
					);

					// Right leg
					const glm::mat4 right_leg_model = Mat4()
						.rotate(-legs_angle, glm::dvec3(1.0f, 0.0f, 0.0f))
						.translate(PlayerModel::right_leg_pos)
						.mat();
					_drawPlayerBodyPart(
						player_right_leg_mesh_id,
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
					_drawPlayerBodyPart(
						player_right_arm_mesh_id,
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
					_drawPlayerBodyPart(
						player_right_arm_mesh_id,
						body_model * chest_model * right_arm_model
					);
				}
			}

			// Draw the targeted block
			if (m_target_block.has_value())
			{
				vkCmdBindPipeline(draw_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, line_pipeline.pipeline);

				Mesh mesh;
				{
					std::lock_guard lock(mesh_map_mutex);
					mesh = mesh_map.at(cube_mesh_id);
				}

				const VkBuffer vertex_buffers[] = { mesh.buffer };
				const VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(
					draw_command_buffers[m_current_frame],
					0, 1,
					vertex_buffers,
					offsets
				);

				vkCmdBindIndexBuffer(
					draw_command_buffers[m_current_frame],
					mesh.buffer,
					mesh.index_offset,
					VK_INDEX_TYPE_UINT32
				);

				const float scale_factor = 1.001f;
				const glm::mat4 target_block_model = glm::translate(glm::mat4(1.0f), m_target_block.value() - glm::vec3((scale_factor - 1.0f) / 2.0f));
				const glm::mat4 target_block_scale = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));
				const GlobalPushConstant target_block_push_constant = {
					target_block_model * target_block_scale,
					glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
				};
				vkCmdPushConstants(
					draw_command_buffers[m_current_frame],
					line_pipeline.layout,
					VK_SHADER_STAGE_ALL,
					0,
					sizeof(GlobalPushConstant),
					&target_block_push_constant
				);

				vkCmdSetLineWidth(draw_command_buffers[m_current_frame], 2.0f);

				vkCmdDrawIndexed(
					draw_command_buffers[m_current_frame],
					static_cast<uint32_t>(mesh.index_count),
					1, 0, 0, 0
				);
			}

			{ // Draw the skybox
				// vkCmdBindPipeline(draw_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline.pipeline);

				// GlobalPushConstant skybox_matrices = {};
				// skybox_matrices.matrice = glm::translate(glm::dmat4(1.0f), camera.position);
				// vkCmdPushConstants(
				// 	draw_command_buffers[current_frame],
				// 	skybox_pipeline.layout,
				// 	VK_SHADER_STAGE_ALL,
				// 	0,
				// 	sizeof(GlobalPushConstant),
				// 	&skybox_matrices
				// );

				// vkCmdDraw(
				// 	draw_command_buffers[current_frame],
				// 	36,
				// 	1,
				// 	0,
				// 	0
				// );
			}

			{ // Draw the sun
				ZoneScopedN("Draw sun");
				TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Draw sun");

				vkCmdBindPipeline(draw_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, sun_pipeline.pipeline);

				GlobalPushConstant sky_shader_data = {};
				sky_shader_data.matrice = glm::translate(glm::dmat4(1.0f), m_camera_render_info.position);

				_drawMesh(
					draw_command_buffers[m_current_frame],
					sun_pipeline,
					icosphere_mesh_id,
					&sky_shader_data,
					sizeof(GlobalPushConstant),
					VK_SHADER_STAGE_ALL
				);
			}
		}
		vkCmdEndRenderPass(draw_command_buffers[m_current_frame]);


		// copy the color attachment to the output attachement
		VkImageCopy copy_region = {};
		copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_region.srcSubresource.layerCount = 1;
		copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_region.dstSubresource.layerCount = 1;
		copy_region.extent = output_attachement.extent3D;

		{
			ZoneScopedN("Copy image");
			TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Copy image");

			vkCmdCopyImage(
				draw_command_buffers[m_current_frame],
				color_attachement.image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				output_attachement.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copy_region
			);
		}

		VkRenderPassBeginInfo water_render_pass_begin_info = {};
		water_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		water_render_pass_begin_info.renderPass = water_render_pass;
		water_render_pass_begin_info.framebuffer = water_framebuffers[m_current_frame];
		water_render_pass_begin_info.renderArea.offset = { 0, 0 };
		water_render_pass_begin_info.renderArea.extent = output_attachement.extent2D;
		water_render_pass_begin_info.clearValueCount = 0;
		water_render_pass_begin_info.pClearValues = nullptr;

		vkCmdBeginRenderPass(
			draw_command_buffers[m_current_frame],
			&water_render_pass_begin_info,
			VK_SUBPASS_CONTENTS_INLINE
		);
		{
			TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Water render pass");

			{ // Draw water
				ZoneScopedN("Draw water");
				TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Draw water");

				vkCmdBindPipeline(draw_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, water_pipeline.pipeline);

				_bindChunkIndexBuffer(draw_command_buffers[m_current_frame]);

				for (auto & id: m_visible_chunks)
				{
					_drawChunkWater(draw_command_buffers[m_current_frame], id);
				}
			}
		}
		vkCmdEndRenderPass(draw_command_buffers[m_current_frame]);


		// write the debug text on the texture only if the debug text is enabled
		if (m_show_debug_text)
		{
			ZoneScopedN("Write debug text");
			TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Write debug text");

			_writeTextToDebugImage(draw_command_buffers[m_current_frame], m_debug_text, 10, 10, 32);
		}


		VkRenderPassBeginInfo hud_render_pass_begin_info = {};
		hud_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		hud_render_pass_begin_info.renderPass = hud_render_pass;
		hud_render_pass_begin_info.framebuffer = hud_framebuffers[m_current_frame];
		hud_render_pass_begin_info.renderArea.offset = { 0, 0 };
		hud_render_pass_begin_info.renderArea.extent = output_attachement.extent2D;
		hud_render_pass_begin_info.clearValueCount = 0;
		hud_render_pass_begin_info.pClearValues = nullptr;

		vkCmdBeginRenderPass(
			draw_command_buffers[m_current_frame],
			&hud_render_pass_begin_info,
			VK_SUBPASS_CONTENTS_INLINE
		);
		{
			TracyVkZone(draw_ctx, draw_command_buffers[m_current_frame], "Hud render pass");

			{ // Draw hud
				ZoneScopedN("Draw hud");

				vkCmdBindPipeline(draw_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, hud_pipeline.pipeline);

				{ // Crosshair
					float min_size = std::min(output_attachement.extent2D.width, output_attachement.extent2D.height);
					float size = min_size / 40.0f;

					VkViewport viewport = {};
					viewport.x = static_cast<float>(output_attachement.extent2D.width / 2 - (size / 2));
					viewport.y = static_cast<float>(output_attachement.extent2D.height / 2 - (size / 2));
					viewport.width = size;
					viewport.height = size;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;

					_drawHudImage(crosshair_image_descriptor, viewport);
				}

				const glm::vec2 toolbar_pos = {
					static_cast<float>(output_attachement.extent2D.width / 2 - (toolbar_image.extent2D.width / 2)),
					static_cast<float>(output_attachement.extent2D.height - toolbar_image.extent2D.height - 10)
				};

				{ // Toolbar
					VkExtent2D size = toolbar_image.extent2D;
					VkViewport viewport = {};
					viewport.x = toolbar_pos.x;
					viewport.y = toolbar_pos.y;
					viewport.width = size.width;
					viewport.height = size.height;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;

					_drawHudImage(toolbar_image_descriptor, viewport);
				}


				vkCmdBindPipeline(draw_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, item_icon_pipeline.pipeline);

				{ // Toolbar items
					for (size_t i = 0; i < 9; i++)
					{
						if (m_toolbar_items[i] == ItemInfo::Type::None)
						{
							continue;
						}

						const glm::vec2 toolbar_item_pos = {
							toolbar_pos.x + (i * 64),
							toolbar_pos.y
						};

						VkViewport viewport = {};
						viewport.x = toolbar_item_pos.x + 3;
						viewport.y = toolbar_item_pos.y + 3;
						viewport.width = 64 - 6;
						viewport.height = 64 - 6;
						viewport.minDepth = 0.0f;
						viewport.maxDepth = 1.0f;

						_drawItemIcon(viewport, static_cast<uint32_t>(m_toolbar_items[i]));
					}
				}

				vkCmdBindPipeline(draw_command_buffers[m_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, hud_pipeline.pipeline);

				const glm::vec2 toolbar_cursor_pos = {
					toolbar_pos.x + (m_toolbar_cursor_index * toolbar_cursor_image.extent2D.width),
					toolbar_pos.y
				};

				{ // Toolbar cursor
					VkExtent2D size = toolbar_cursor_image.extent2D;
					VkViewport viewport = {};
					viewport.x = toolbar_cursor_pos.x;
					viewport.y = toolbar_cursor_pos.y;
					viewport.width = size.width;
					viewport.height = size.height;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;

					_drawHudImage(toolbar_cursor_image_descriptor, viewport);
				}

				if (m_show_debug_text) // Debug info
				{
					float width = 2048.0f;
					float height = 512.0f;

					VkViewport viewport = {};
					viewport.x = 0;
					viewport.y = 0;
					viewport.width = debug_info_image.extent2D.width;
					viewport.height = debug_info_image.extent2D.height;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;

					_drawHudImage(debug_info_image_descriptor, viewport);
				}
			}

			// { // Draw test image
			// 	vkCmdBindPipeline(draw_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, test_image_pipeline.pipeline);
			// 	vkCmdDraw(
			// 		draw_command_buffers[current_frame],
			// 		6,
			// 		1,
			// 		0,
			// 		0
			// 	);
			// }
		}
		vkCmdEndRenderPass(draw_command_buffers[m_current_frame]);
	}

	TracyVkCollect(draw_ctx, draw_command_buffers[m_current_frame]);

	VK_CHECK(
		vkEndCommandBuffer(draw_command_buffers[m_current_frame]),
		"Failed to record command buffer"
	);
}

void VulkanAPI::_drawPlayerBodyPart(
	const uint64_t mesh_id,
	const glm::mat4 & model
)
{
	GlobalPushConstant player_matrice = {};
	player_matrice.matrice = model;

	_drawMesh(
		draw_command_buffers[m_current_frame],
		player_pipeline,
		mesh_id,
		&player_matrice,
		sizeof(GlobalPushConstant),
		VK_SHADER_STAGE_ALL
	);
}

void VulkanAPI::_copyToSwapchain()
{
	ZoneScoped;

	// Acquire the next swap chain image
	VkResult result = vkAcquireNextImageKHR(
		device,
		swapchain.swapchain,
		std::numeric_limits<uint64_t>::max(),
		image_available_semaphores[m_current_frame],
		VK_NULL_HANDLE,
		&m_current_image_index
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		_recreateSwapChain(window);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}


	// Copy the color image to the swap chain image with blit
	VK_CHECK(
		vkResetCommandBuffer(copy_command_buffers[m_current_frame], 0),
		"Failed to reset copy command buffer"
	);

	VkCommandBufferBeginInfo copy_begin_info = {};
	copy_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(copy_command_buffers[m_current_frame], &copy_begin_info),
		"Failed to begin recording copy command buffer"
	);

	{ // Scope for Tracy
		TracyVkZone(draw_ctx, copy_command_buffers[m_current_frame], "Copy to swapchain");

		_setImageLayout(
			copy_command_buffers[m_current_frame],
			swapchain.images[m_current_image_index],
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
			static_cast<int32_t>(output_attachement.extent2D.width),
			static_cast<int32_t>(output_attachement.extent2D.height),
			1
		};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = 0;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = {
			static_cast<int32_t>(swapchain.extent.width),
			static_cast<int32_t>(swapchain.extent.height),
			1
		};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = 0;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(
			copy_command_buffers[m_current_frame],
			output_attachement.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			swapchain.images[m_current_image_index],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blit,
			VK_FILTER_LINEAR
		);
	}

	VK_CHECK(
		vkEndCommandBuffer(copy_command_buffers[m_current_frame]),
		"Failed to record copy command buffer"
	);
}

void VulkanAPI::_drawDebugGui()
{
	ZoneScoped;

	vkResetCommandBuffer(imgui_command_buffers[m_current_frame], 0);

	VkCommandBufferBeginInfo imgui_begin_info = {};
	imgui_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(imgui_command_buffers[m_current_frame], &imgui_begin_info),
		"Failed to begin recording command buffer"
	);

	{ // Scope for Tracy
		TracyVkZone(draw_ctx, imgui_command_buffers[m_current_frame], "Draw debug gui");

		{
			std::lock_guard lock(imgui_textures_mutex);
			for (auto & [id, texture]: imgui_textures)
			{
				_setImageLayout(
					imgui_command_buffers[m_current_frame],
					texture.image,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
					0,
					0,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
				);
			}
		}
		_setImageLayout(
			imgui_command_buffers[m_current_frame],
			swapchain.images[m_current_image_index],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			0,
			0,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		);

		VkRenderingAttachmentInfo imgui_color_attachment = {};
		imgui_color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		imgui_color_attachment.imageView = swapchain.image_views[m_current_image_index];
		imgui_color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgui_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		imgui_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkRenderingInfo imgui_render_info = {};
		imgui_render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		imgui_render_info.renderArea = { 0, 0, swapchain.extent.width, swapchain.extent.height };
		imgui_render_info.layerCount = 1;
		imgui_render_info.colorAttachmentCount = 1;
		imgui_render_info.pColorAttachments = &imgui_color_attachment;

		vkCmdBeginRendering(imgui_command_buffers[m_current_frame], &imgui_render_info);


		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		_updateImGui();

		ImGui::Render();

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imgui_command_buffers[m_current_frame]);


		vkCmdEndRendering(imgui_command_buffers[m_current_frame]);

		_setImageLayout(
			imgui_command_buffers[m_current_frame],
			swapchain.images[m_current_image_index],
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			0,
			0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
		);

		{
			std::lock_guard lock(imgui_textures_mutex);
			for (auto & [id, texture]: imgui_textures)
			{
				_setImageLayout(
					imgui_command_buffers[m_current_frame],
					texture.image,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_GENERAL,
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
					0,
					0,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
				);
			}
		}
	}

	VK_CHECK(
		vkEndCommandBuffer(imgui_command_buffers[m_current_frame]),
		"Failed to record imgui command buffer"
	);
}

void VulkanAPI::_updateImGui()
{
	
#define FLOAT_SLIDER(name, min, max) float name ## _f = name; ImGui::SliderFloat(#name, &name ## _f, min, max); name = name ## _f;
#define INT_SLIDER(name, min, max) int name ## _i = name; ImGui::SliderInt(#name, &name ## _i, min, max); name = name ## _i;

#define VEC3_SLIDER(name, min_x, max_x, min_y, max_y, min_z, max_z) \
	float name ## _x = name.x; \
	float name ## _y = name.y; \
	float name ## _z = name.z; \
	ImGui::SliderFloat(#name " x", &name ## _x, min_x, max_x); \
	ImGui::SliderFloat(#name " y", &name ## _y, min_y, max_y); \
	ImGui::SliderFloat(#name " z", &name ## _z, min_z, max_z); \
	name = glm::vec3(name ## _x, name ## _y, name ## _z);


	if (ImGui::Begin("Debug"))
	{
		if (ImGui::BeginTabBar("HeadTabBar"))
		{
			if (ImGui::BeginTabItem("Game"))
			{
				ImGui::Text("Fps: %d", DebugGui::fps.load());
				ImGui::Text("XYZ: %.3f %.3f %.3f", DebugGui::player_position.get().x, DebugGui::player_position.get().y, DebugGui::player_position.get().z);
				ImGui::Text("V XYZ: %.3f %.3f %.3f", DebugGui::player_velocity_vec.get().x, DebugGui::player_velocity_vec.get().y, DebugGui::player_velocity_vec.get().z);
				ImGui::Text("Velocity: %.3f", DebugGui::player_velocity.load());
				ImGui::Text("Chunk: %f %f %f", std::floor(DebugGui::player_position.get().x / 16) , std::floor(DebugGui::player_position.get().y / 256), std::floor(DebugGui::player_position.get().z / 16));
				ImGui::Text("Looked face sky light: %d", DebugGui::looked_face_sky_light.load());
				ImGui::Text("Looked face block light: %d", DebugGui::looked_face_block_light.load());

				ImGui::Separator();

				ImGui::Text("Rendered triangles: %ld", DebugGui::rendered_triangles.load());
				ImGui::Text("Chunk meshes count: %d", DebugGui::chunk_mesh_count.load());

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Performance"))
			{
				if (ImGui::CollapsingHeader("Render Thread"))
				{
					ImGui::Text("Fps: %d", DebugGui::fps.load());

					{
						std::string average = "Average: " + std::to_string(DebugGui::frame_time_history.average()) + " ms";
						auto lock = DebugGui::frame_time_history.lock();
						ImGui::PlotHistogram("Frame time", DebugGui::frame_time_history.data(), DebugGui::frame_time_history.size(), 0, average.c_str(), 0.0f, 3000.0f, ImVec2(200, 80));
					}

					{
						std::string average = "Average: " + std::to_string(DebugGui::cpu_time_history.average()) + " ms";
						auto lock = DebugGui::cpu_time_history.lock();
						ImGui::PlotHistogram("Cpu time", DebugGui::cpu_time_history.data(), DebugGui::cpu_time_history.size(), 0, average.c_str(), 0.0f, 3000.0f, ImVec2(200, 80));
					}

					{
						std::string average = "Average: " + std::to_string(DebugGui::cpu_wait_time_history.average()) + " ms";
						auto lock = DebugGui::cpu_wait_time_history.lock();
						ImGui::PlotHistogram("Cpu wait time", DebugGui::cpu_wait_time_history.data(), DebugGui::cpu_wait_time_history.size(), 0, average.c_str(), 0.0f, 3000.0f, ImVec2(200, 80));
					}
				}
				if (ImGui::CollapsingHeader("Update Thread"))
				{
					ImGui::Text("Ups: %d", DebugGui::ups.load());
				}
				if (ImGui::CollapsingHeader("Block Update Thread"))
				{
					ImGui::Text("Fps: %d", DebugGui::fps.load());

					ImGui::Text("Chunk gen time:     %f µs", DebugGui::chunk_gen_time_history.average());
					ImGui::Text("Chunk unload time:  %f µs", DebugGui::chunk_unload_time_history.average());
					ImGui::Text("Chunk render time:  %f µs", DebugGui::chunk_render_time_history.average());
					ImGui::Text("  Chunk mesh create time:  %f µs", DebugGui::create_mesh_time.load());
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Memory"))
			{
				ImGui::Text("Gpu memory: %ld MB", DebugGui::gpu_allocated_memory / 1024 / 1024);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Network"))
			{
				ImGui::Text("Send buffer size: %ld", DebugGui::send_buffer_size.load());
				ImGui::Text("Recv buffer size: %ld", DebugGui::recv_buffer_size.load());

				{
					// auto lock = send_history.lock();
					// ImGui::PlotHistogram("Send usage", send_history.data(), send_history.size());
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Textures for Augustus"))
			{
				ImGui::Text("Continentalness");
				ImGuiTextureDraw(DebugGui::continentalness_texture_id);
				ImGui::Text("Erosion");
				ImGuiTextureDraw(DebugGui::erosion_texture_id);
				ImGui::Text("Weirdness");
				ImGuiTextureDraw(DebugGui::weirdness_texture_id);
				ImGui::Text("PV");
				ImGuiTextureDraw(DebugGui::PV_texture_id);
				ImGui::Text("Temperature");
				ImGuiTextureDraw(DebugGui::temperature_texture_id);
				ImGui::Text("Humidity");
				ImGuiTextureDraw(DebugGui::humidity_texture_id);
				ImGui::Text("Biome");
				ImGuiTextureDraw(DebugGui::biome_texture_id);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Graphic"))
			{
				// slidders for atmosphere parrams
				// FLOAT_SLIDER(sun_theta, 0.0f, 360.0f)
				// FLOAT_SLIDER(earth_radius, 0.0f, 10000000.0f)
				// FLOAT_SLIDER(atmosphere_radius, 0.0f, 10000000.0f)
				// FLOAT_SLIDER(player_height, 0.0f, 10000.0f)

				// glm::vec3 beta_r = beta_rayleigh.get() * 1e6f;
				// VEC3_SLIDER(beta_r, 0.0f, 100.0f, 0.0f, 100.0f, 0.0f, 100.0f)
				// beta_rayleigh = beta_r / 1e6f;

				// float beta_m = beta_mie.get().x * 1e6f;
				// ImGui::SliderFloat("Beta mie", &beta_m, 0.0f, 100.0f);
				// beta_mie = glm::vec3(beta_m) / 1e6f;

				// FLOAT_SLIDER(sun_intensity, 0.0f, 100.0f)
				// FLOAT_SLIDER(h_rayleigh, 0.0f, 10000.0f)
				// FLOAT_SLIDER(h_mie, 0.0f, 10000.0f)
				// FLOAT_SLIDER(g, 0.0f, 1.0f)
				// INT_SLIDER(n_samples, 0.0f, 20.0f)
				// INT_SLIDER(n_light_samples, 0.0f, 20.0f)

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

	}
	ImGui::End();
}

bool VulkanAPI::_isInsideFrustum_ndcSpace(const glm::mat4 & model, const glm::vec3 & size) const
{
	const std::vector<glm::vec4> corners = {
		glm::vec4(0,      0,      0,      1),
		glm::vec4(size.x, 0,      0,      1),
		glm::vec4(0,      size.y, 0,      1),
		glm::vec4(size.x, size.y, 0,      1),
		glm::vec4(0,      0,      size.z, 1),
		glm::vec4(size.x, 0,      size.z, 1),
		glm::vec4(0,      size.y, size.z, 1),
		glm::vec4(size.x, size.y, size.z, 1)
	};

	const glm::mat4 MVP = m_camera_matrices_fc.proj * m_camera_matrices_fc.view * model;

	glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 max = glm::vec3(std::numeric_limits<float>::min());
	for (const auto & corner : corners)
	{
		const glm::vec4 clip_space = MVP * corner;
		const glm::vec3 ndc_space = glm::vec3(clip_space) / clip_space.w;

		min.x = std::min(min.x, ndc_space.x);
		min.y = std::min(min.y, ndc_space.y);
		min.z = std::min(min.z, ndc_space.z);
		max.x = std::max(max.x, ndc_space.x);
		max.y = std::max(max.y, ndc_space.y);
		max.z = std::max(max.z, ndc_space.z);
	}

	return (
		min.x <= 1.0f && max.x >= -1.0f &&
		min.y <= 1.0f && max.y >= -1.0f &&
		min.z <= 1.0f && max.z >= 0.0f
	);

	// bool inside = false;
	// for (const auto & corner : corners)
	// {
	// 	const glm::vec4 clip_space = MVP * corner;

	// 	inside = inside || (
	// 		clip_space.x >= -clip_space.w &&
	// 		clip_space.x <= clip_space.w &&
	// 		clip_space.y >= -clip_space.w &&
	// 		clip_space.y <= clip_space.w &&
	// 		clip_space.z >= 0.0f &&
	// 		clip_space.z <= clip_space.w
	// 	);
	// }

	// return inside;
}

static std::array<glm::vec4, 6> getFrustumPlnes(const glm::mat4 & VP)
{
	std::array<glm::vec4, 6> planes;

	// Left clipping plane
	planes[0] = glm::vec4(
		VP[0][3] + VP[0][0],
		VP[1][3] + VP[1][0],
		VP[2][3] + VP[2][0],
		VP[3][3] + VP[3][0]
	);
	// Right clipping plane
	planes[1] = glm::vec4(
		VP[0][3] - VP[0][0],
		VP[1][3] - VP[1][0],
		VP[2][3] - VP[2][0],
		VP[3][3] - VP[3][0]
	);
	// Bottom clipping plane
	planes[2] = glm::vec4(
		VP[0][3] + VP[0][1],
		VP[1][3] + VP[1][1],
		VP[2][3] + VP[2][1],
		VP[3][3] + VP[3][1]
	);
	// Top clipping plane
	planes[3] = glm::vec4(
		VP[0][3] - VP[0][1],
		VP[1][3] - VP[1][1],
		VP[2][3] - VP[2][1],
		VP[3][3] - VP[3][1]
	);
	// Near clipping plane
	planes[4] = glm::vec4(
		VP[0][3] + VP[0][2],
		VP[1][3] + VP[1][2],
		VP[2][3] + VP[2][2],
		VP[3][3] + VP[3][2]
	);
	// Far clipping plane
	planes[5] = glm::vec4(
		VP[0][3] - VP[0][2],
		VP[1][3] - VP[1][2],
		VP[2][3] - VP[2][2],
		VP[3][3] - VP[3][2]
	);

	for (auto & plane : planes)
	{
		const float length = glm::length(glm::vec3(plane));
		plane /= length;
	}

	return planes;
}

bool VulkanAPI::_isInsideFrustum_planes(
	const glm::mat4 & view_proj,
	const glm::mat4 & model,
	const glm::vec3 & size
) const
{
	// https://iquilezles.org/articles/frustumcorrect/
	// https://www.cse.chalmers.se/~uffe/vfc_bbox.pdf
	// https://www.braynzarsoft.net/viewtutorial/q16390-34-aabb-cpu-side-frustum-culling

	const std::vector<glm::vec4> aabb_corners = {
		model * glm::vec4(0,      0,      0,      1),
		model * glm::vec4(size.x, 0,      0,      1),
		model * glm::vec4(0,      size.y, 0,      1),
		model * glm::vec4(size.x, size.y, 0,      1),
		model * glm::vec4(0,      0,      size.z, 1),
		model * glm::vec4(size.x, 0,      size.z, 1),
		model * glm::vec4(0,      size.y, size.z, 1),
		model * glm::vec4(size.x, size.y, size.z, 1)
	};

	const auto frustum_planes = getFrustumPlnes(view_proj);

	// check box outside/inside of frustum
	for (const auto & plane : frustum_planes)
	{
		int outside = 0;
		outside += glm::dot(plane, aabb_corners[0]) < 0.0f ? 1 : 0;
		outside += glm::dot(plane, aabb_corners[1]) < 0.0f ? 1 : 0;
		outside += glm::dot(plane, aabb_corners[2]) < 0.0f ? 1 : 0;
		outside += glm::dot(plane, aabb_corners[3]) < 0.0f ? 1 : 0;
		outside += glm::dot(plane, aabb_corners[4]) < 0.0f ? 1 : 0;
		outside += glm::dot(plane, aabb_corners[5]) < 0.0f ? 1 : 0;
		outside += glm::dot(plane, aabb_corners[6]) < 0.0f ? 1 : 0;
		outside += glm::dot(plane, aabb_corners[7]) < 0.0f ? 1 : 0;
		if (outside == 8) return false;
	}

	// const glm::mat4 inv_proj_view = glm::inverse(view_proj);
	// const std::vector<glm::vec4> frustum_corners = {
	// 	inv_proj_view * glm::vec4(-1, -1, -1, 1),
	// 	inv_proj_view * glm::vec4( 1, -1, -1, 1),
	// 	inv_proj_view * glm::vec4(-1,  1, -1, 1),
	// 	inv_proj_view * glm::vec4( 1,  1, -1, 1),
	// 	inv_proj_view * glm::vec4(-1, -1,  1, 1),
	// 	inv_proj_view * glm::vec4( 1, -1,  1, 1),
	// 	inv_proj_view * glm::vec4(-1,  1,  1, 1),
	// 	inv_proj_view * glm::vec4( 1,  1,  1, 1)
	// };

	// check frustum outside/inside box
	// int out;
	// out = 0; for (const auto & corner : frustum_corners) out += corner.x > aabb_corners[7].x ? 1 : 0; if (out == 8) return false;
	// out = 0; for (const auto & corner : frustum_corners) out += corner.x < aabb_corners[0].x ? 1 : 0; if (out == 8) return false;
	// out = 0; for (const auto & corner : frustum_corners) out += corner.y > aabb_corners[7].y ? 1 : 0; if (out == 8) return false;
	// out = 0; for (const auto & corner : frustum_corners) out += corner.y < aabb_corners[0].y ? 1 : 0; if (out == 8) return false;
	// out = 0; for (const auto & corner : frustum_corners) out += corner.z > aabb_corners[7].z ? 1 : 0; if (out == 8) return false;
	// out = 0; for (const auto & corner : frustum_corners) out += corner.z < aabb_corners[0].z ? 1 : 0; if (out == 8) return false;

	return true;
}


static std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4 proj, const glm::mat4 view)
{
    const auto inv = glm::inverse(proj * view);

    std::vector<glm::vec4> frustum_corners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt =
                    inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f);
                frustum_corners.push_back(pt / pt.w);
            }
        }
    }

    return frustum_corners;
}

std::vector<glm::mat4> VulkanAPI::_getCSMLightViewProjMatrices(
	const glm::vec3 & light_dir,
	const std::vector<float> & split,
	const float blend_distance,
	const glm::mat4 & camera_view,
	const float cam_fov,
	const float cam_ratio,
	const float cam_near_plane,
	const float cam_far_plane,
	std::vector<float> & far_plane_distances
)
{
	const float near_far_diff = cam_far_plane - cam_near_plane;

	std::vector<glm::mat4> light_view_proj_matrices;
	for (size_t i = 0; i + 1 < split.size(); i++)
	{
		float sub_frustum_near_plane = cam_near_plane + near_far_diff * split[i];
		float sub_frustum_far_plane = cam_near_plane + near_far_diff * split[i + 1];
		if (i + 2 < split.size()) // Extend the far plane (except for the last cascade)
		{
			sub_frustum_far_plane += blend_distance;
		}
		far_plane_distances.push_back(sub_frustum_far_plane);

		std::vector<glm::vec4> frustum_corners = getFrustumCornersWorldSpace(
			glm::perspective(cam_fov, cam_ratio, sub_frustum_near_plane, sub_frustum_far_plane),
			camera_view
		);

		glm::vec3 sub_frustum_center = glm::vec3(0.0f);
		for (const auto & corner : frustum_corners)
		{
			sub_frustum_center += glm::vec3(corner);
		}
		sub_frustum_center /= static_cast<float>(frustum_corners.size());


		const glm::mat4 light_view = glm::lookAt(
			sub_frustum_center + light_dir,
			sub_frustum_center,
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();
		for (const glm::vec4 & v : frustum_corners)
		{
			const glm::vec4 trf = light_view * v;
			minX = std::min(minX, trf.x);
			maxX = std::max(maxX, trf.x);
			minY = std::min(minY, trf.y);
			maxY = std::max(maxY, trf.y);
			minZ = std::min(minZ, trf.z);
			maxZ = std::max(maxZ, trf.z);
		}

		// Tune this parameter according to the scene
		constexpr float zMult = 10.0f;
		if (minZ < 0)
		{
			minZ *= zMult;
		}
		else
		{
			minZ /= zMult;
		}
		if (maxZ < 0)
		{
			maxZ /= zMult;
		}
		else
		{
			maxZ *= zMult;
		}

		// LOG_DEBUG(i << " minX: " << minX << " maxX: " << maxX << " minY: " << minY << " maxY: " << maxY << " minZ: " << minZ << " maxZ: " << maxZ);

		const glm::mat4 light_proj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

		light_view_proj_matrices.push_back(light_proj * light_view);
	}

	return light_view_proj_matrices;
}
