#include "RenderThread.hpp"
#include "logger.hpp"
#include "Perlin.hpp"

#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>

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
	m_start_time(start_time),
	m_last_frame_time(start_time),
	m_frame_count(0),
	m_start_time_counting_fps(start_time)
{
	(void)m_settings;
	(void)m_world_scene;

}

RenderThread::~RenderThread()
{
}

void RenderThread::init()
{
	
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

	CameraMatrices camera_matrices = {};
	camera_matrices.view = m_world_scene.camera().getViewMatrix();
	camera_matrices.proj = m_world_scene.camera().getProjectionMatrix(static_cast<float>(width) / static_cast<float>(height));
	camera_matrices.proj[1][1] *= -1;

	m_frame_count++;
	if (m_current_time - m_start_time_counting_fps >= std::chrono::seconds(1))
	{
		m_fps = static_cast<float>(m_frame_count) / std::chrono::duration_cast<std::chrono::seconds>(m_current_time - m_start_time_counting_fps).count();
		m_frame_count = 0;
		m_start_time_counting_fps = m_current_time;
	}

	//############################################################################################################
	//                     																                         #
	//                                  Start the vulkan rendering process here                                  #
	//                     																                         #
	//############################################################################################################

	std::lock_guard<std::mutex> lock(vk.global_mutex);

	vkWaitForFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame]);

	vkResetCommandBuffer(vk.render_command_buffers[vk.current_frame], 0);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(vk.render_command_buffers[vk.current_frame], &begin_info),
		"Failed to begin recording command buffer!"
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

	vkCmdBeginRendering(vk.render_command_buffers[vk.current_frame], &render_info);
	m_start_cpu_rendering_time = std::chrono::steady_clock::now().time_since_epoch();


	vkCmdBindPipeline(vk.render_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.graphics_pipeline);

	memcpy(vk.uniform_buffers_mapped_memory[vk.current_frame], &camera_matrices, sizeof(camera_matrices));
	vkCmdBindDescriptorSets(
		vk.render_command_buffers[vk.current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk.pipeline_layout,
		0,
		1,
		&vk.descriptor_set,
		0,
		nullptr
	);

	m_triangle_count = 0;

	auto mesh_render_data = m_world_scene.getMeshRenderData();
	
	for (auto & mesh_data : mesh_render_data)
	{
		VkBuffer vertex_buffers[] = { vk.meshes[mesh_data.id].buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(vk.render_command_buffers[vk.current_frame], 0, 1, vertex_buffers, offsets);

		vkCmdBindIndexBuffer(vk.render_command_buffers[vk.current_frame], vk.meshes[mesh_data.id].buffer, vk.meshes[mesh_data.id].index_offset, VK_INDEX_TYPE_UINT32);

		ModelMatrice model_matrice = {};
		model_matrice.model = mesh_data.transform.model();
		vkCmdPushConstants(
			vk.render_command_buffers[vk.current_frame],
			vk.pipeline_layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(ModelMatrice),
			&model_matrice
		);

		vkCmdDrawIndexed(vk.render_command_buffers[vk.current_frame], static_cast<uint32_t>(vk.meshes[mesh_data.id].index_count), 1, 0, 0, 0);

		m_triangle_count += static_cast<int>(vk.meshes[mesh_data.id].index_count) / 3;
	}

	m_end_cpu_rendering_time = std::chrono::steady_clock::now().time_since_epoch();
	vkCmdEndRendering(vk.render_command_buffers[vk.current_frame]);

	//############################################################################################################
	//                     																                         #
	//                              Submit the command buffer to the graphics queue                              #
	//                     																                         #
	//############################################################################################################

	VK_CHECK(
		vkEndCommandBuffer(vk.render_command_buffers[vk.current_frame]),
		"Failed to record command buffer"
	);

	VkSubmitInfo render_submit_info = {};
	render_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	render_submit_info.commandBufferCount = 1;
	render_submit_info.pCommandBuffers = &vk.render_command_buffers[vk.current_frame];
	render_submit_info.signalSemaphoreCount = 1;
	render_submit_info.pSignalSemaphores = &vk.render_finished_semaphores[vk.current_frame];

	VK_CHECK(
		vkQueueSubmit(vk.graphics_queue, 1, &render_submit_info, vk.in_flight_fences[vk.current_frame]),
		"Failed to submit draw command buffer"
	);

	//############################################################################################################
	//                     																                         #
	//                     Copy the color image attachment to the swap chain image with blit                     #
	//                     																                         #
	//############################################################################################################

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


	// Transition the swap chain image from present to transfer destination
	vk.transitionImageLayout(
		vk.swap_chain_images[image_index],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);

	// Transition the color image from color attachment to transfer source
	vk.transitionImageLayout(
		vk.color_attachement_image,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);

	// Copy the color image to the swap chain image with blit
	vkResetCommandBuffer(vk.copy_command_buffers[vk.current_frame], 0);

	VkCommandBufferBeginInfo copy_begin_info = {};
	copy_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(vk.copy_command_buffers[vk.current_frame], &copy_begin_info),
		"Failed to begin recording copy command buffer"
	);

	VkImageBlit blit = {};
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcOffsets[1] = {
		static_cast<int32_t>(vk.color_attachement_extent.width),
		static_cast<int32_t>(vk.color_attachement_extent.height),
		// static_cast<int32_t>(vk.draw_image_extent.width),
		// static_cast<int32_t>(vk.draw_image_extent.height),
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
		// vk.draw_image,
		// VK_IMAGE_LAYOUT_GENERAL,
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

	std::array<VkSemaphore, 2> copy_wait_semaphores = {
		vk.image_available_semaphores[vk.current_frame],
		vk.render_finished_semaphores[vk.current_frame]
	};
	std::array<VkPipelineStageFlags, 2> copy_wait_stages = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	};
	copy_submit_info.waitSemaphoreCount = static_cast<uint32_t>(copy_wait_semaphores.size());
	copy_submit_info.pWaitSemaphores = copy_wait_semaphores.data();
	copy_submit_info.pWaitDstStageMask = copy_wait_stages.data();

	copy_submit_info.signalSemaphoreCount = 1;
	copy_submit_info.pSignalSemaphores = &vk.swap_chain_updated_semaphores[vk.current_frame];

	VK_CHECK(
		vkQueueSubmit(vk.graphics_queue, 1, &copy_submit_info, VK_NULL_HANDLE),
		"Failed to submit copy command buffer"
	);

	VK_CHECK(
		vkQueueWaitIdle(vk.graphics_queue),
		"Failed to wait for queue to become idle"
	);

	// Transition the color image from transfer source to color attachment
	vk.transitionImageLayout(
		vk.color_attachement_image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	);

	//############################################################################################################
	//                     																                         #
	//                                        Do the ImGui rendering here                                        #
	//                     																                         #
	//############################################################################################################

	// Transition the swap chain image from transfer destination to color attachment
	vk.transitionImageLayout(
		vk.swap_chain_images[image_index],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	);

	// Transition the imgui texture from general to shader read only optimal
	vk.transitionImageLayout(
		vk.imgui_texture.image,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	updateImGui();

	ImGui::Render();

	VkCommandBuffer imgui_command_buffer;
	VkCommandBufferAllocateInfo imgui_command_buffer_info = {};
	imgui_command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	imgui_command_buffer_info.commandPool = vk.command_pool;
	imgui_command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	imgui_command_buffer_info.commandBufferCount = 1;

	VK_CHECK(
		vkAllocateCommandBuffers(vk.device, &imgui_command_buffer_info, &imgui_command_buffer),
		"Failed to allocate command buffer"
	);

	VkCommandBufferBeginInfo imgui_begin_info = {};
	imgui_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(
		vkBeginCommandBuffer(imgui_command_buffer, &imgui_begin_info),
		"Failed to begin recording command buffer"
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

	vkCmdBeginRendering(imgui_command_buffer, &imgui_render_info);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imgui_command_buffer);

	vkCmdEndRendering(imgui_command_buffer);

	VK_CHECK(
		vkEndCommandBuffer(imgui_command_buffer),
		"Failed to record command buffer"
	);

	VkSubmitInfo imgui_submit_info = {};
	imgui_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	imgui_submit_info.commandBufferCount = 1;
	imgui_submit_info.pCommandBuffers = &imgui_command_buffer;
	imgui_submit_info.waitSemaphoreCount = 1;
	imgui_submit_info.pWaitSemaphores = &vk.swap_chain_updated_semaphores[vk.current_frame];
	VkPipelineStageFlags vk_pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	imgui_submit_info.pWaitDstStageMask = &vk_pipeline_stage_flags;
	imgui_submit_info.signalSemaphoreCount = 1;
	imgui_submit_info.pSignalSemaphores = &vk.imgui_render_finished_semaphores[vk.current_frame];

	VK_CHECK(
		vkQueueSubmit(vk.graphics_queue, 1, &imgui_submit_info, VK_NULL_HANDLE),
		"Failed to submit command buffer"
	);

	VK_CHECK(
		vkQueueWaitIdle(vk.graphics_queue),
		"Failed to wait for queue to become idle"
	);

	// Transition the imgui texture from shader read only optimal to general
	vk.transitionImageLayout(
		vk.imgui_texture.image,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
	);

	//############################################################################################################
	//                     																                         #
	//                             Present the swap chain image to the present queue                             #
	//                     																                         #
	//############################################################################################################

	// Transition the swap chain image from transfer destination to present
	vk.transitionImageLayout(
		vk.swap_chain_images[image_index],
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
	);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &vk.imgui_render_finished_semaphores[vk.current_frame];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &vk.swap_chain;
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
}

void RenderThread::updateTime()
{
	m_current_time = std::chrono::steady_clock::now().time_since_epoch();
	m_delta_time = m_current_time - m_last_frame_time;
	m_last_frame_time = m_current_time;
}
