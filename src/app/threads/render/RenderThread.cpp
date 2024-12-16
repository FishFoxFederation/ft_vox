#include "define.hpp"
#include "RenderThread.hpp"
#include "logger.hpp"
#include "Perlin.hpp"

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
	std::chrono::nanoseconds start_time,
	std::exception_ptr & eptr_ref
):
	m_settings(settings),
	vk(vulkanAPI),
	m_world_scene(worldScene),
	m_debug_gui(),
	m_start_time(start_time),
	m_last_frame_time(start_time),
	m_frame_count(0),
	m_start_time_counting_fps(start_time),
	m_thread(&RenderThread::launch, this),
	m_eptr_ref(eptr_ref)
{
	(void)m_settings;
	(void)m_start_time;
}

RenderThread::~RenderThread()
{
}

void RenderThread::stop()
{
	this->m_thread.request_stop();
}

void RenderThread::launch()
{
	try
	{
		init();

		while (!m_thread.get_stop_token().stop_requested())
		{
			loop();
		}
	}
	catch (std::exception & e) {
		LOG_CRITICAL("RenderThread exception: " << e.what());
		m_eptr_ref = std::current_exception();
	} catch (...)
	{
		LOG_CRITICAL("RenderThread unkown exception");
		m_eptr_ref = std::current_exception();
	}
	m_running = false;
	LOG_INFO("RenderThread stopped");
}

void RenderThread::init()
{
	LOG_INFO("RenderThread launched :" << gettid());
}

void RenderThread::loop()
{
	//############################################################################################################
	//                     																                         #
	//                            Do independent logic from the vulkan rendering here                            #
	//                     																                         #
	//############################################################################################################

	updateTime();

	int width, height;
	glfwGetFramebufferSize(vk.window, &width, &height);

	const double aspect_ratio = static_cast<double>(width) / static_cast<double>(height);

	const glm::mat4 clip = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f,-1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f
	);

	const Camera::RenderInfo camera = m_world_scene.camera().getRenderInfo(aspect_ratio);

	// DebugGui::player_position = camera.position;

	ViewProjMatrices camera_matrices = {};
	camera_matrices.view = camera.view;
	camera_matrices.proj = clip * camera.projection;

	const std::vector<WorldScene::MeshRenderData> chunk_meshes = m_world_scene.getMeshRenderData();

	m_frame_count++;
	if (m_current_time - m_start_time_counting_fps >= std::chrono::seconds(1))
	{
		DebugGui::fps = static_cast<double>(m_frame_count) / std::chrono::duration_cast<std::chrono::seconds>(m_current_time - m_start_time_counting_fps).count();
		m_frame_count = 0;
		m_start_time_counting_fps = m_current_time;
	}

	DebugGui::frame_time_history.push(m_delta_time.count() / 1e6);

	const glm::dvec3 sun_offset = glm::dvec3(100.0f, 70.0f, 100.0f);
	const glm::dvec3 sun_pos = camera.position + sun_offset;
	const float sun_size = 300.0f;
	const float sun_near = 10.0f;
	const float sun_far = 1000.0f;

	ViewProjMatrices sun = camera_matrices;
	sun.view = glm::lookAt(
		sun_pos,
		camera.position,
		glm::dvec3(0.0f, 1.0f, 0.0f)
	);
	sun.proj = clip * glm::ortho(
		-sun_size, sun_size,
		-sun_size, sun_size,
		sun_near, sun_far
	);


	//############################################################################################################
	//                     																                         #
	//                                  Start the vulkan rendering process here                                  #
	//                     																                         #
	//############################################################################################################

	std::lock_guard<std::mutex> lock(vk.global_mutex);

	{
		std::lock_guard<std::mutex> lock(vk.mesh_mutex);
		DebugGui::chunk_mesh_count = vk.meshes.size();
	}

	VK_CHECK(
		vkWaitForFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max()),
		"Failed to wait for in flight fence"
	);
	vkResetFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame]);

	const std::chrono::nanoseconds start_cpu_rendering_time = std::chrono::steady_clock::now().time_since_epoch();

	// reset mesh usage by frame info
	{
		std::lock_guard<std::mutex> lock(vk.mesh_mutex);
		for (auto & mesh : vk.meshes)
		{
			mesh.second.used_by_frame[vk.current_frame] = false;
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

	memcpy(vk.camera_uniform_buffers_mapped_memory[vk.current_frame], &camera_matrices, sizeof(camera_matrices));
	memcpy(vk.sun_uniform_buffers_mapped_memory[vk.current_frame], &sun, sizeof(sun));

	//############################################################################################################
	//                     																                         #
	//                                     Start shadow pass rendering here                                      #
	//                     																                         #
	//############################################################################################################

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
	vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.shadow_pipeline.pipeline);

	const std::vector<VkDescriptorSet> shadow_descriptor_sets = {
		vk.sun_descriptor.sets[vk.current_frame]
	};

	vkCmdBindDescriptorSets(
		vk.draw_command_buffers[vk.current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk.shadow_pipeline.layout,
		0,
		static_cast<uint32_t>(shadow_descriptor_sets.size()),
		shadow_descriptor_sets.data(),
		0,
		nullptr
	);

	for (auto & chunk_mesh : chunk_meshes)
	{
		std::lock_guard<std::mutex> lock(vk.mesh_mutex);

		if (vk.meshes.find(chunk_mesh.id) == vk.meshes.end())
		{
			LOG_WARNING("Mesh " << chunk_mesh.id << " not found in the mesh map.");
			continue;
		}

		if (vk.meshes[chunk_mesh.id].buffer == VK_NULL_HANDLE)
		{
			LOG_WARNING("Mesh " << chunk_mesh.id << " has a null buffer.");
			continue;
		}

		vk.meshes[chunk_mesh.id].used_by_frame[vk.current_frame] = true;

		const VkBuffer vertex_buffers[] = { vk.meshes[chunk_mesh.id].buffer };
		const VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(
			vk.draw_command_buffers[vk.current_frame],
			0, 1,
			vertex_buffers,
			offsets
		);

		vkCmdBindIndexBuffer(
			vk.draw_command_buffers[vk.current_frame],
			vk.meshes[chunk_mesh.id].buffer,
			vk.meshes[chunk_mesh.id].index_offset,
			VK_INDEX_TYPE_UINT32
		);

		ModelMatrice model_matrice = {};
		model_matrice.model = chunk_mesh.transform.model();
		vkCmdPushConstants(
			vk.draw_command_buffers[vk.current_frame],
			vk.shadow_pipeline.layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(ModelMatrice),
			&model_matrice
		);

		vkCmdDrawIndexed(
			vk.draw_command_buffers[vk.current_frame],
			static_cast<uint32_t>(vk.meshes[chunk_mesh.id].index_count),
			1, 0, 0, 0
		);
	}

	vkCmdEndRenderPass(vk.draw_command_buffers[vk.current_frame]);

	//############################################################################################################
	//                     																                         #
	//                                    Start lighting pass rendering here                                     #
	//                     																                         #
	//############################################################################################################

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


	// Draw the chunks
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

	uint32_t triangle_count = 0;

	for (auto & chunk_mesh : chunk_meshes)
	{
		std::lock_guard<std::mutex> lock(vk.mesh_mutex);

		if (vk.meshes.find(chunk_mesh.id) == vk.meshes.end())
		{
			LOG_WARNING("Mesh " << chunk_mesh.id << " not found in the mesh map.");
			continue;
		}

		if (vk.meshes[chunk_mesh.id].buffer == VK_NULL_HANDLE)
		{
			LOG_WARNING("Mesh " << chunk_mesh.id << " has a null buffer.");
			continue;
		}

		vk.meshes[chunk_mesh.id].used_by_frame[vk.current_frame] = true;

		// glm::dvec3 pos = chunk_mesh.transform.position();
		// if (!camera.view_frustum.sphereInFrustum(pos + glm::dvec3(CHUNK_SIZE / 2), CHUNK_SIZE / 2 * std::sqrt(3)))
		// {
		// 	continue;
		// }

		const VkBuffer vertex_buffers[] = { vk.meshes[chunk_mesh.id].buffer };
		const VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(
			vk.draw_command_buffers[vk.current_frame],
			0, 1,
			vertex_buffers,
			offsets
		);

		vkCmdBindIndexBuffer(
			vk.draw_command_buffers[vk.current_frame],
			vk.meshes[chunk_mesh.id].buffer,
			vk.meshes[chunk_mesh.id].index_offset,
			VK_INDEX_TYPE_UINT32
		);

		ModelMatrice model_matrice = {};
		model_matrice.model = chunk_mesh.transform.model();
		vkCmdPushConstants(
			vk.draw_command_buffers[vk.current_frame],
			vk.chunk_pipeline.layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(ModelMatrice),
			&model_matrice
		);

		vkCmdDrawIndexed(
			vk.draw_command_buffers[vk.current_frame],
			static_cast<uint32_t>(vk.meshes[chunk_mesh.id].index_count),
			1, 0, 0, 0
		);

		triangle_count += (vk.meshes[chunk_mesh.id].index_count) / 3;
	}
	DebugGui::rendered_triangles = triangle_count;


	// Draw the skybox
	vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.skybox_pipeline.pipeline);

	const std::array<VkDescriptorSet, 2> skybox_descriptor_sets = {
		vk.camera_descriptor.sets[vk.current_frame],
		vk.cube_map_descriptor.set
	};

	vkCmdBindDescriptorSets(
		vk.draw_command_buffers[vk.current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk.skybox_pipeline.layout,
		0,
		static_cast<uint32_t>(skybox_descriptor_sets.size()),
		skybox_descriptor_sets.data(),
		0,
		nullptr
	);

	ModelMatrice camera_model_matrice = {};
	camera_model_matrice.model = glm::translate(glm::dmat4(1.0f), camera.position);
	vkCmdPushConstants(
		vk.draw_command_buffers[vk.current_frame],
		vk.skybox_pipeline.layout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(ModelMatrice),
		&camera_model_matrice
	);

	vkCmdDraw(
		vk.draw_command_buffers[vk.current_frame],
		36,
		1,
		0,
		0
	);


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

	//############################################################################################################
	//                     																                         #
	//                              Submit the command buffer to the graphics queue                              #
	//                     																                         #
	//############################################################################################################

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

	// Acquire the next swap chain image
	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(
		vk.device,
		vk.swapchain.swapchain,
		std::numeric_limits<uint64_t>::max(),
		vk.image_available_semaphores[vk.current_frame],
		VK_NULL_HANDLE,
		&image_index
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
		vk.swapchain.images[image_index],
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
		vk.swapchain.images[image_index],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&blit,
		VK_FILTER_LINEAR
	);

	VK_CHECK(
		vkEndCommandBuffer(vk.copy_command_buffers[vk.current_frame]),
		"Failed to record copy command buffer"
	);

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

	vkResetCommandBuffer(vk.imgui_command_buffers[vk.current_frame], 0);

	VkCommandBufferBeginInfo imgui_begin_info = {};
	imgui_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(vk.imgui_command_buffers[vk.current_frame], &imgui_begin_info),
		"Failed to begin recording command buffer"
	);

	vk.setImageLayout(
		vk.imgui_command_buffers[vk.current_frame],
		vk.swapchain.images[image_index],
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
	imgui_color_attachment.imageView = vk.swapchain.image_views[image_index];
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
		vk.swapchain.images[image_index],
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
	present_info.pImageIndices = &image_index;

	result = vkQueuePresentKHR(vk.present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		vk.recreateSwapChain(vk.window);
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image");
	}

	// Increment the current frame
	vk.current_frame = (vk.current_frame + 1) % vk.max_frames_in_flight;

	const std::chrono::nanoseconds end_cpu_rendering_time = std::chrono::steady_clock::now().time_since_epoch();
	DebugGui::cpu_time_history.push((end_cpu_rendering_time - start_cpu_rendering_time).count() / 1e6);
}

void RenderThread::updateTime()
{
	m_current_time = std::chrono::steady_clock::now().time_since_epoch();
	m_delta_time = m_current_time - m_last_frame_time;
	m_last_frame_time = m_current_time;
}
