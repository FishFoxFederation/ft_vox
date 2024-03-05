#include "RenderThread.hpp"
#include "logger.hpp"
#include "Perlin.hpp"

#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <algorithm>

RenderThread::RenderThread(
	const Settings & settings,
	VulkanAPI & vulkanAPI,
	const WorldScene & worldScene,
	std::chrono::nanoseconds start_time
):
	AThreadWrapper(),
	m_settings(settings),
	vk(vulkanAPI),
	m_world_scene(worldScene),
	m_debug_gui(vk),
	m_start_time(start_time),
	m_last_frame_time(start_time),
	m_frame_count(0),
	m_start_time_counting_fps(start_time)
{
	(void)m_settings;
	(void)m_start_time;
}

RenderThread::~RenderThread()
{
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

	LOG_TRACE("Start rendering loop.");

	updateTime();

	int width, height;
	glfwGetFramebufferSize(vk.window, &width, &height);

	const double aspect_ratio = static_cast<double>(width) / static_cast<double>(height);

	const Camera::RenderInfo camera = m_world_scene.camera().getRenderInfo(aspect_ratio);

	CameraMatrices camera_matrices = {};
	camera_matrices.view = camera.view;
	camera_matrices.proj = camera.projection;
	camera_matrices.proj[1][1] *= -1;

	static std::vector<glm::dvec3> camera_position(vk.max_frames_in_flight, camera.position);
	camera_position[vk.current_frame] = camera.position;

	const std::vector<WorldScene::MeshRenderData> chunk_meshes = m_world_scene.getMeshRenderData();

	m_frame_count++;
	if (m_current_time - m_start_time_counting_fps >= std::chrono::seconds(1))
	{
		DebugGui::fps = static_cast<double>(m_frame_count) / std::chrono::duration_cast<std::chrono::seconds>(m_current_time - m_start_time_counting_fps).count();
		m_frame_count = 0;
		m_start_time_counting_fps = m_current_time;
	}

	DebugGui::frame_time_history.push(m_delta_time.count() / 1e6);


	//############################################################################################################
	//                     																                         #
	//                                  Start the vulkan rendering process here                                  #
	//                     																                         #
	//############################################################################################################

	LOG_TRACE("Start vulkan logic.");

	std::lock_guard<std::mutex> lock(vk.global_mutex);

	vkWaitForFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame]);

	const std::chrono::nanoseconds start_cpu_rendering_time = std::chrono::steady_clock::now().time_since_epoch();

	vkResetCommandBuffer(vk.draw_command_buffers[vk.current_frame], 0);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(vk.draw_command_buffers[vk.current_frame], &begin_info),
		"Failed to begin recording command buffer!"
	);

	vk.setImageLayout(
		vk.draw_command_buffers[vk.current_frame],
		vk.color_attachement_image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		0,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);

	std::array<VkRenderingAttachmentInfo, 1> color_attachments = {};
	color_attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	color_attachments[0].imageView = vk.color_attachement_view;
	color_attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachments[0].clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderingAttachmentInfo depth_attachment = {};
	depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depth_attachment.imageView = vk.depth_attachement_view;
	depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.clearValue = { 1.0f, 0 };

	VkRenderingInfo render_info = {};
	render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	render_info.renderArea = { 0, 0, vk.color_attachement_extent.width, vk.color_attachement_extent.height };
	render_info.layerCount = 1;
	render_info.colorAttachmentCount = static_cast<uint32_t>(color_attachments.size());
	render_info.pColorAttachments = color_attachments.data();
	render_info.pDepthAttachment = &depth_attachment;

	LOG_TRACE("Begin main rendering.");

	vkCmdBeginRendering(vk.draw_command_buffers[vk.current_frame], &render_info);

	memcpy(vk.camera_uniform_buffers_mapped_memory[vk.current_frame], &camera_matrices, sizeof(camera_matrices));

	// Draw the chunks
	vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.graphics_pipeline);

	vkCmdBindDescriptorSets(
		vk.draw_command_buffers[vk.current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk.pipeline_layout,
		0,
		1,
		&vk.camera_descriptor_set,
		0,
		nullptr
	);

	vkCmdBindDescriptorSets(
		vk.draw_command_buffers[vk.current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk.pipeline_layout,
		1,
		1,
		&vk.texture_array_descriptor_set,
		0,
		nullptr

	);

	uint32_t triangle_count = 0;

	for (auto & chunk_mesh : chunk_meshes)
	{
		glm::dvec3 pos = chunk_mesh.transform.position();
		if (!camera.view_frustum.sphereInFrustum(pos + glm::dvec3(CHUNK_SIZE / 2), CHUNK_SIZE / 2 * std::sqrt(3)))
		// if (!frustum.boxInFrustum(pos, pos + glm::dvec3(CHUNK_SIZE)))
		{
			continue;
		}

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
			vk.pipeline_layout,
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
	vkCmdBindPipeline(vk.draw_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.skybox_graphics_pipeline);

	vkCmdBindDescriptorSets(
		vk.draw_command_buffers[vk.current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk.skybox_pipeline_layout,
		0,
		1,
		&vk.camera_descriptor_set,
		0,
		nullptr
	);

	vkCmdBindDescriptorSets(
		vk.draw_command_buffers[vk.current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk.skybox_pipeline_layout,
		1,
		1,
		&vk.cube_map_descriptor_set,
		0,
		nullptr
	);

	ModelMatrice camera_model_matrice = {};
	// camera_model_matrice.model = glm::translate(glm::dmat4(1.0f), camera.position);
	camera_model_matrice.model = glm::translate(glm::dmat4(1.0), camera_position[1]);
	vkCmdPushConstants(
		vk.draw_command_buffers[vk.current_frame],
		vk.skybox_pipeline_layout,
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


	LOG_TRACE("End main rendering.");
	vkCmdEndRendering(vk.draw_command_buffers[vk.current_frame]);

	//############################################################################################################
	//                     																                         #
	//                              Submit the command buffer to the graphics queue                              #
	//                     																                         #
	//############################################################################################################

	LOG_TRACE("End render_command_buffers.");

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

	LOG_TRACE("Acquire the next swap chain image.");

	// Acquire the next swap chain image
	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(
		vk.device,
		vk.swap_chain,
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


	LOG_TRACE("Copy the color image to the swap chain image with blit.");
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
		vk.swap_chain_images[image_index],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		0,
		0,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);

	vk.setImageLayout(
		vk.copy_command_buffers[vk.current_frame],
		vk.color_attachement_image,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		0,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);

	VkImageBlit blit = {};
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcOffsets[1] = {
		static_cast<int32_t>(vk.color_attachement_extent.width),
		static_cast<int32_t>(vk.color_attachement_extent.height),
		1
	};
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;
	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = {
		static_cast<int32_t>(vk.swap_chain_extent.width),
		static_cast<int32_t>(vk.swap_chain_extent.height),
		1
	};
	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = 0;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	vkCmdBlitImage(
		vk.copy_command_buffers[vk.current_frame],
		vk.color_attachement_image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vk.swap_chain_images[image_index],
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

	LOG_TRACE("Begin ImGui command buffer.");
	VK_CHECK(
		vkBeginCommandBuffer(vk.imgui_command_buffers[vk.current_frame], &imgui_begin_info),
		"Failed to begin recording command buffer"
	);

	vk.setImageLayout(
		vk.imgui_command_buffers[vk.current_frame],
		vk.swap_chain_images[image_index],
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
	imgui_color_attachment.imageView = vk.swap_chain_image_views[image_index];
	imgui_color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imgui_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	imgui_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo imgui_render_info = {};
	imgui_render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	imgui_render_info.renderArea = { 0, 0, vk.swap_chain_extent.width, vk.swap_chain_extent.height };
	imgui_render_info.layerCount = 1;
	imgui_render_info.colorAttachmentCount = 1;
	imgui_render_info.pColorAttachments = &imgui_color_attachment;

	vkCmdBeginRendering(vk.imgui_command_buffers[vk.current_frame], &imgui_render_info);


	LOG_TRACE("Begin ImGui frame.");
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	m_debug_gui.updateImGui();

	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vk.imgui_command_buffers[vk.current_frame]);


	vkCmdEndRendering(vk.imgui_command_buffers[vk.current_frame]);

	vk.setImageLayout(
		vk.imgui_command_buffers[vk.current_frame],
		vk.swap_chain_images[image_index],
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

	LOG_TRACE("Submit all command buffers.");
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
	present_info.pSwapchains = &vk.swap_chain;
	present_info.pImageIndices = &image_index;

	LOG_TRACE("Present to screen.");
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
