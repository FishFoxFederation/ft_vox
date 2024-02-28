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
	const WorldScene & worldScene
):
	AThreadWrapper(),
	m_settings(settings),
	vk(vulkanAPI),
	m_world_scene(worldScene)
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

	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	int width, height;
	glfwGetFramebufferSize(vk.window, &width, &height);

	CameraMatrices camera_matrices = {};
	camera_matrices.view = m_world_scene.camera().getViewMatrix();
	camera_matrices.proj = m_world_scene.camera().getProjectionMatrix(static_cast<float>(width) / static_cast<float>(height));
	camera_matrices.proj[1][1] *= -1;

	//############################################################################################################
	//                     																                         #
	//                                  Start the vulkan rendering process here                                  #
	//                     																                         #
	//############################################################################################################

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

	VkRenderingInfo render_info = {};
	render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	render_info.renderArea = { 0, 0, vk.color_attachement_extent.width, vk.color_attachement_extent.height };
	render_info.layerCount = 1;
	render_info.colorAttachmentCount = static_cast<uint32_t>(color_attachments.size());
	render_info.pColorAttachments = color_attachments.data();

	vkCmdBeginRendering(vk.render_command_buffers[vk.current_frame], &render_info);


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

	ModelMatrice model_matrice = {};
	model_matrice.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	vkCmdPushConstants(
		vk.render_command_buffers[vk.current_frame],
		vk.pipeline_layout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(ModelMatrice),
		&model_matrice
	);

	VkBuffer vertex_buffers[] = { vk.mesh.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(vk.render_command_buffers[vk.current_frame], 0, 1, vertex_buffers, offsets);

	vkCmdBindIndexBuffer(vk.render_command_buffers[vk.current_frame], vk.mesh.buffer, vk.mesh.index_offset, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(vk.render_command_buffers[vk.current_frame], static_cast<uint32_t>(vk.mesh.index_count), 1, 0, 0, 0);


	vkCmdEndRendering(vk.render_command_buffers[vk.current_frame]);

	//############################################################################################################
	//                     																                         #
	//                                          Auguste tu commence ici                                          #
	//                     																                         #
	//############################################################################################################

	/*
	 * vk.width() = the width of the image
	 * vk.height() = the height of the image
	 * vk.clearPixels() = clear the draw image
	 * vk.putPixel(x, y, r, g, b, a = 255) = put a pixel at the position (x, y) with the color (r, g, b, a)
	 */

	static Perlin perlin_pinpin(0, 5, 1, 0.5, 2.0);

	(void)time;
	vk.clearPixels();
	// int nope;
	// std::cin >> nope; std::cin.clear();

	for(size_t i = 0; i < vk.height(); i++)
	{
		for(size_t j = 0; j < vk.width(); j++)
		{
			float value = perlin_pinpin.noise(glm::vec2(j * 0.01f, i * 0.01f));

			//normalize to range [0, 1]
			value += 1;
			value /= 2;

			int light = value * 255;
			light &= 0xFF;

			vk.putPixel(j, i, light, light, light);
			// std::cout << std::setw(3) << light << " ";
		}
		// std::cout << std::endl;
	}

	//############################################################################################################
	//                     																                         #
	//                                           Et tu t'arrêtes la :)                                           #
	//                     																                         #
	//############################################################################################################


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

	// Transition the swap chain image from transfer destination to present
	vk.transitionImageLayout(
		vk.swap_chain_images[image_index],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		0,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
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
	//                             Present the swap chain image to the present queue                             #
	//                     																                         #
	//############################################################################################################

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &vk.swap_chain_updated_semaphores[vk.current_frame];
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
