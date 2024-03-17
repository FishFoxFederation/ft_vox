# Render Loop {#render_loop}

This page will explain how the render loop code works.

- [Main rendering](#main_rendering)
	- [Wait for previous frame](#wait_for_previous_frame)
	- [Start recording command buffer](#start_recording_command_buffer)
	- [Begin rendering](#begin_main_rendering)
	- [Draw the chunks](#draw_the_chunks)
	- [Draw the skybox](#draw_the_skybox)
	- [End command buffer](#end_command_buffer)
- [Swapchain](#swapchain)
	- [Acquire image](#acquire_image)
	- [Transition images layout](#transition_images_layout)
	- [Start recording command buffer](#start_recording_copy_command_buffer)
	- [Copy color attachment to swapchain image](#copy_color_attachment_to_swapchain_image)
	- [End command buffer](#end_copy_command_buffer)
	- [Transition images layout back](#transition_images_layout_back)
- [ImGui](#imgui)
	- [Start recording command buffer](#start_recording_imgui_command_buffer)
	- [Begin rendering](#begin_imgui_rendering)
	- [Render ImGui](#render_imgui)
	- [End command buffer](#end_imgui_command_buffer)
- [Present](#present)
	- [Transition images layout](#transition_images_layout_present)
	- [Present image](#present_image)
- [Conclusion](#conclusion)
	- [Possible improvements](#possible_improvements)

## Main rendering {#main_rendering}

#### Wait for previous frame {#wait_for_previous_frame}

First, we need to wait for the previous frame to finish rendering. This is done by calling `vkWaitForFences` with the fence of the current frame. This will block the CPU until the GPU has finished rendering the previous frame. After that, we reset the fence so that it can be used again.

```cpp
vkWaitForFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());
vkResetFences(vk.device, 1, &vk.in_flight_fences[vk.current_frame]);
```

Since we can have multiple frames in flight, we access the current frame's fence by using `vk.current_frame`. This will be the case for all the other resources that are specific to a specific frame.

#### Start recording command buffer {#start_recording_command_buffer}

We can start recording the command buffer for the current frame. We do this by calling `vkResetCommandBuffer` to reset the command buffer, and then `vkBeginCommandBuffer` to start recording.

```cpp
vkResetCommandBuffer(vk.render_command_buffers[vk.current_frame], 0);

VkCommandBufferBeginInfo begin_info = {};
begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

vkBeginCommandBuffer(vk.render_command_buffers[vk.current_frame], &begin_info);
```

#### Begin rendering {#begin_main_rendering}

We can now start rendering by calling `vkCmdBeginRendering`. This function takes a `VkRenderingInfo` struct that contains information about the rendering. Here we specify a color attachment and a depth attachment which are the images that we will render to.

```cpp
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
```

#### Draw the chunks {#draw_the_chunks}

First, we need to update the camera uniform buffer. This buffer contains the camera matrices that are used in the vertex shader for the chunks and the skybox as well.

```cpp
memcpy(vk.camera_uniform_buffers_mapped_memory[vk.current_frame], &camera_matrices, sizeof(camera_matrices));
```

Then we bind the pipeline and the descriptor sets.

```cpp
vkCmdBindPipeline(vk.render_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.graphics_pipeline);

vkCmdBindDescriptorSets(
	vk.render_command_buffers[vk.current_frame],
	VK_PIPELINE_BIND_POINT_GRAPHICS,
	vk.pipeline_layout,
	0,
	1,
	&vk.camera_descriptor_set,
	0,
	nullptr
);

vkCmdBindDescriptorSets(
	vk.render_command_buffers[vk.current_frame],
	VK_PIPELINE_BIND_POINT_GRAPHICS,
	vk.pipeline_layout,
	1,
	1,
	&vk.texture_array_descriptor_set,
	0,
	nullptr
);
```

Then we loop through all the chunk meshes and draw them if they are inside the view frustum. Each time updating the model matrix push constant which translates the chunk to its wolrd position.

```cpp
for (auto & chunk_mesh : chunk_meshes)
{
	glm::dvec3 pos = chunk_mesh.transform.position();
	if (!camera.view_frustum.sphereInFrustum(pos + glm::dvec3(CHUNK_SIZE / 2), CHUNK_SIZE / 2 * std::sqrt(3)))
	{
		continue;
	}

	VkBuffer vertex_buffers[] = { vk.meshes[chunk_mesh.id].buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(
		vk.render_command_buffers[vk.current_frame],
		0, 1,
		vertex_buffers,
		offsets
	);

	vkCmdBindIndexBuffer(
		vk.render_command_buffers[vk.current_frame],
		vk.meshes[chunk_mesh.id].buffer,
		vk.meshes[chunk_mesh.id].index_offset,
		VK_INDEX_TYPE_UINT32
	);

	ModelMatrice model_matrice = {};
	model_matrice.model = chunk_mesh.transform.model();
	vkCmdPushConstants(
		vk.render_command_buffers[vk.current_frame],
		vk.pipeline_layout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(ModelMatrice),
		&model_matrice
	);

	vkCmdDrawIndexed(
		vk.render_command_buffers[vk.current_frame],
		static_cast<uint32_t>(vk.meshes[chunk_mesh.id].index_count),
		1, 0, 0, 0
	);
}
```

#### Draw the skybox {#draw_the_skybox}

For the skybox we use a cube map texture that is rendered using a separate pipeline.
The skybox is drawn last so that we can use the depth buffer that was created when drawing the chunks to only draw the skybox where necessary.
As the skybox is always drawn around the camera, the model matrix is set to the camera position.
Finally, the skybox is a simple cube so the vertices are hardcoded in the vertex shader.

```cpp
vkCmdBindPipeline(vk.render_command_buffers[vk.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.skybox_graphics_pipeline);

vkCmdBindDescriptorSets(
	vk.render_command_buffers[vk.current_frame],
	VK_PIPELINE_BIND_POINT_GRAPHICS,
	vk.skybox_pipeline_layout,
	0,
	1,
	&vk.camera_descriptor_set,
	0,
	nullptr
);

vkCmdBindDescriptorSets(
	vk.render_command_buffers[vk.current_frame],
	VK_PIPELINE_BIND_POINT_GRAPHICS,
	vk.skybox_pipeline_layout,
	1,
	1,
	&vk.cube_map_descriptor_set,
	0,
	nullptr
);

ModelMatrice camera_model_matrice = {};
camera_model_matrice.model = glm::translate(glm::dmat4(1.0f), camera.position);
vkCmdPushConstants(
	vk.render_command_buffers[vk.current_frame],
	vk.skybox_pipeline_layout,
	VK_SHADER_STAGE_VERTEX_BIT,
	0,
	sizeof(ModelMatrice),
	&camera_model_matrice
);

vkCmdDraw(
	vk.render_command_buffers[vk.current_frame],
	36,
	1,
	0,
	0
);
```

#### End command buffer {#end_command_buffer}

Finally, we end the rendering and the command buffer and submit it to the graphics queue.

```cpp
vkCmdEndRendering(vk.render_command_buffers[vk.current_frame]);

vkEndCommandBuffer(vk.render_command_buffers[vk.current_frame]);

VkSubmitInfo render_submit_info = {};
render_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
render_submit_info.commandBufferCount = 1;
render_submit_info.pCommandBuffers = &vk.render_command_buffers[vk.current_frame];
render_submit_info.signalSemaphoreCount = 1;
render_submit_info.pSignalSemaphores = &vk.render_finished_semaphores[vk.current_frame];

vkQueueSubmit(vk.graphics_queue, 1, &render_submit_info, vk.in_flight_fences[vk.current_frame]);
```


## Swapchain {#swapchain}

#### Acquire image {#acquire_image}

After the rendering is submitted, we can acquire the next image from the swapchain. This is done by calling `vkAcquireNextImageKHR` which will return the index of the image that we can render to.
Depending on the result, we might need to recreate the swapchain if it is out of date.

```cpp
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
```

#### Transition images layout {#transition_images_layout}

We transition the swapchain image and the color attachment to the correct layout for the blit, e.i. `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` and `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` respectively.

```cpp
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
```

#### Start recording command buffer {#start_recording_copy_command_buffer}

We use a separate command buffer for the blit operation.

```cpp
vkResetCommandBuffer(vk.copy_command_buffers[vk.current_frame], 0);

VkCommandBufferBeginInfo copy_begin_info = {};
copy_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

vkBeginCommandBuffer(vk.copy_command_buffers[vk.current_frame], &copy_begin_info);
```

#### Copy color attachment to swapchain image {#copy_color_attachment_to_swapchain_image}

We use blit because the color attachment may have a different size than the swapchain image which will allow us to do anti-aliasing for example.

```cpp
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
```

#### End command buffer {#end_copy_command_buffer}

Finally, we end the command buffer and submit it to the graphics queue.

```cpp
vkEndCommandBuffer(vk.copy_command_buffers[vk.current_frame]);

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

vkQueueSubmit(vk.graphics_queue, 1, &copy_submit_info, VK_NULL_HANDLE);

vkQueueWaitIdle(vk.graphics_queue);
```

#### Transition images layout back {#transition_images_layout_back}

After the blit operation, we need to transition the images layout back to `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` for the color attachment and `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` for the swapchain image because we will render to it the ImGui textures.

```cpp
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
```

## ImGui {#imgui}

#### Start recording command buffer {#start_recording_imgui_command_buffer}

We create a new command buffer for the ImGui rendering.

```cpp
VkCommandBuffer imgui_command_buffer;
VkCommandBufferAllocateInfo imgui_command_buffer_info = {};
imgui_command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
imgui_command_buffer_info.commandPool = vk.command_pool;
imgui_command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
imgui_command_buffer_info.commandBufferCount = 1;

vkAllocateCommandBuffers(vk.device, &imgui_command_buffer_info, &imgui_command_buffer);

VkCommandBufferBeginInfo imgui_begin_info = {};
imgui_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

vkBeginCommandBuffer(imgui_command_buffer, &imgui_begin_info);
```

#### Begin rendering {#begin_imgui_rendering}

We start rendering with the swapchain image as the color attachment.

```cpp
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
```

#### Render ImGui {#render_imgui}

We render the ImGui UI. The `m_debug_gui.updateImGui()` function will call the ImGui functions to render the UI.

```cpp
ImGui_ImplVulkan_NewFrame();
ImGui_ImplGlfw_NewFrame();
ImGui::NewFrame();

m_debug_gui.updateImGui();

ImGui::Render();

ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imgui_command_buffer);
```

#### End command buffer {#end_imgui_command_buffer}

Finally, we end the command buffer and submit it to the graphics queue.

```cpp
vkCmdEndRendering(imgui_command_buffer);

vkEndCommandBuffer(imgui_command_buffer);

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

vkQueueSubmit(vk.graphics_queue, 1, &imgui_submit_info, VK_NULL_HANDLE);

vkQueueWaitIdle(vk.graphics_queue);
```

## Present {#present}

#### Transition images layout {#transition_images_layout_present}

We need to transition the swapchain image layout to `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` so that it can be presented.

```cpp
vk.transitionImageLayout(
	vk.swap_chain_images[image_index],
	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	VK_IMAGE_ASPECT_COLOR_BIT,
	1,
	0,
	0,
	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
);
```

#### Present image {#present_image}

Finally, we present the image to the swapchain. Optionally, we can recreate the swapchain if it is out of date.

```cpp
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
```

Finally, we increment the current frame index and we are ready to start rendering the next frame.

```cpp
vk.current_frame = (vk.current_frame + 1) % vk.max_frames_in_flight;
```

## Conclusion {#conclusion}

#### Possible improvements {#possible_improvements}

- The render loop could be improved by using multiple threads to record the command buffers and submit them to the graphics queue.
- If the render loop is done within one thread, we could use one single command buffer for the whole rendering process.
- The occlusion culling could be done on the GPU.
