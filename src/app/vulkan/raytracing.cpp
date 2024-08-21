#include "VulkanAPI.hpp"
#include "logger.hpp"
#include "Block.hpp"
#include "Model.hpp"
#include "ObjLoader.hpp"

#include "Tracy.hpp"

#include <stb_image.h>

#include <cstring>
#include <set>
#include <algorithm>
#include <map>

void VulkanAPI::setupRayTracing()
{
	getRayTracingProperties();
	createRayTracingUBO();
	createRayTracingImages();
	createRayTracingDescriptor();
	updateRTImagesDescriptor();
	createRayTracingPipeline();
	createRayTracingShaderBindingTable();
	createComputePipeline();

	LOG_INFO("Ray tracing setup complete");
}

void VulkanAPI::destroyRayTracing()
{

	for (uint32_t i = 0; i < blas_list.size(); i++)
	{
		destroyBottomLevelAS(blas_list[i]);
	}

	for (uint32_t i = 0; i < instance_list.size(); i++)
	{
		destroyInstance(instance_list[i]);
	}

	if (tlas != VK_NULL_HANDLE)
	{
		vkDestroyAccelerationStructureKHR(device, tlas, nullptr);
		vkDestroyBuffer(device, tlas_buffer, nullptr);
		vma.freeMemory(device, tlas_buffer_memory, nullptr);
	}

	if (rt_mesh_data_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, rt_mesh_data_buffer, nullptr);
		vma.freeMemory(device, rt_mesh_data_buffer_memory, nullptr);
	}

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		vkUnmapMemory(device, rt_camera_ubo.memory[i]);
		vma.freeMemory(device, rt_camera_ubo.memory[i], nullptr);
		vkDestroyBuffer(device, rt_camera_ubo.buffers[i], nullptr);
	}

	rt_lighting_shadow_image_descriptor.clear();
	rt_output_image_descriptor.clear();
	rt_objects_descriptor.clear();
	rt_camera_descriptor.clear();

	vkDestroyImage(device, rt_lighting_image, nullptr);
	vma.freeMemory(device, rt_lighting_image_memory, nullptr);
	vkDestroyImageView(device, rt_lighting_image_view, nullptr);

	vkDestroyImage(device, rt_shadow_image, nullptr);
	vma.freeMemory(device, rt_shadow_image_memory, nullptr);
	vkDestroyImageView(device, rt_shadow_image_view, nullptr);

	vkDestroyImage(device, rt_output_image, nullptr);
	vma.freeMemory(device, rt_output_image_memory, nullptr);
	vkDestroyImageView(device, rt_output_image_view, nullptr);

	vkDestroyPipeline(device, rt_pipeline, nullptr);
	vkDestroyPipelineLayout(device, rt_pipeline_layout, nullptr);

	vkDestroyPipeline(device, compute_pipeline, nullptr);
	vkDestroyPipelineLayout(device, compute_pipeline_layout, nullptr);

	vkDestroyBuffer(device, rt_sbt_buffer, nullptr);
	vma.freeMemory(device, rt_sbt_buffer_memory, nullptr);
}

void VulkanAPI::handleResizeRT()
{
	vkDestroyImage(device, rt_lighting_image, nullptr);
	vma.freeMemory(device, rt_lighting_image_memory, nullptr);
	vkDestroyImageView(device, rt_lighting_image_view, nullptr);

	vkDestroyImage(device, rt_shadow_image, nullptr);
	vma.freeMemory(device, rt_shadow_image_memory, nullptr);
	vkDestroyImageView(device, rt_shadow_image_view, nullptr);

	vkDestroyImage(device, rt_output_image, nullptr);
	vma.freeMemory(device, rt_output_image_memory, nullptr);
	vkDestroyImageView(device, rt_output_image_view, nullptr);

	createRayTracingImages();
	updateRTImagesDescriptor();
}

void VulkanAPI::getRayTracingProperties()
{
	VkPhysicalDeviceProperties2 properties = {};
	properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

	// ray_tracing_features = {};
	// ray_tracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	// properties.pNext = &ray_tracing_features;
	// vkGetPhysicalDeviceProperties2(physical_device, &properties);

	ray_tracing_properties = {};
	ray_tracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	properties.pNext = &ray_tracing_properties;
	vkGetPhysicalDeviceProperties2(physical_device, &properties);

	// acceleration_structure_features = {};
	// acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	// properties.pNext = &acceleration_structure_features;
	// vkGetPhysicalDeviceProperties2(physical_device, &properties);

	acceleration_structure_properties = {};
	acceleration_structure_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
	properties.pNext = &acceleration_structure_properties;
	vkGetPhysicalDeviceProperties2(physical_device, &properties);
}

int VulkanAPI::createBottomLevelAS(uint64_t mesh_id)
{
	int blas_index = -1;
	for (uint32_t i = 0; i < blas_list.size(); i++)
	{
		if (blas_list[i].is_used == false)
		{
			blas_index = i;
			break;
		}
	}
	if (blas_index == -1)
	{
		blas_index = blas_list.size();
		blas_list.push_back({});
	}

	Mesh & mesh = mesh_map[mesh_id];

	BottomLevelAS & blas = blas_list[blas_index];
	blas.is_used = true;
	blas.instance_custom_index = blas_index;
	blas.mesh_id = mesh_id;

	VkDeviceAddress mesh_address = getBufferDeviceAddress(mesh.buffer);

	VkTransformMatrixKHR transform_matrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};

	createBuffer(
		sizeof(VkTransformMatrixKHR),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		blas.transform_buffer,
		blas.transform_buffer_memory
	);

	vkMapMemory(device, blas.transform_buffer_memory, 0, sizeof(VkTransformMatrixKHR), 0, &blas.transform_mapped_memory);
	memcpy(blas.transform_mapped_memory, &transform_matrix, sizeof(VkTransformMatrixKHR));

	blas.transform_address = getBufferDeviceAddress(blas.transform_buffer);

	VkAccelerationStructureGeometryKHR as_geometry = {};
	as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	as_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	as_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	as_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	as_geometry.geometry.triangles.vertexData.deviceAddress = mesh_address;
	as_geometry.geometry.triangles.vertexStride = mesh.vertex_size;
	as_geometry.geometry.triangles.maxVertex = mesh.vertex_count - 1;
	as_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	as_geometry.geometry.triangles.indexData.deviceAddress = mesh_address + mesh.index_offset;
	as_geometry.geometry.triangles.transformData.hostAddress = nullptr;
	as_geometry.geometry.triangles.transformData.deviceAddress = blas.transform_address;

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.geometryCount = 1;
	build_info.pGeometries = &as_geometry;


	uint32_t primitive_count = mesh.index_count / 3;

	VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = {};
	build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&build_info,
		&primitive_count,
		&build_sizes_info
	);

	VkBuffer scratch_buffer;
	VkDeviceMemory scratch_buffer_memory;
	createBuffer(
		build_sizes_info.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		scratch_buffer,
		scratch_buffer_memory
	);

	createBuffer(
		build_sizes_info.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		blas.buffer,
		blas.memory
	);

	VkAccelerationStructureCreateInfoKHR acceleration_structure_info = {};
	acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	acceleration_structure_info.buffer = blas.buffer;
	acceleration_structure_info.size = build_sizes_info.accelerationStructureSize;

	VK_CHECK(
		vkCreateAccelerationStructureKHR(device, &acceleration_structure_info, nullptr, &blas.handle),
		"Failed to create bottom level acceleration structure"
	);

	build_info.scratchData.deviceAddress = getBufferDeviceAddress(scratch_buffer);
	build_info.dstAccelerationStructure = blas.handle;

	VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};
	build_range_info.primitiveCount = primitive_count;
	build_range_info.primitiveOffset = 0;
	build_range_info.firstVertex = 0;
	build_range_info.transformOffset = 0;

	std::vector<VkAccelerationStructureBuildRangeInfoKHR *> build_range_infos = { &build_range_info };

	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	vkCmdBuildAccelerationStructuresKHR(
		command_buffer,
		1,
		&build_info,
		build_range_infos.data()
	);

	endSingleTimeCommands(command_buffer);

	vkDestroyBuffer(device, scratch_buffer, nullptr);
	vma.freeMemory(device, scratch_buffer_memory, nullptr);

	VkAccelerationStructureDeviceAddressInfoKHR device_address_info = {};
	device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	device_address_info.accelerationStructure = blas.handle;

	blas.address = vkGetAccelerationStructureDeviceAddressKHR(device, &device_address_info);

	return blas_index;
}

void VulkanAPI::destroyBottomLevelAS(BottomLevelAS & blas)
{
	if (blas.is_used == false)
	{
		return;
	}

	for (uint32_t i = 0; i < instance_list.size(); i++)
	{
		if (instance_list[i].blas_address == blas.address)
		{
			// LOG_WARNING("Destroying Bottom Level AS triggers instance destruction");
			destroyInstance(instance_list[i]);
		}
	}

	vkDestroyAccelerationStructureKHR(device, blas.handle, nullptr);
	vkUnmapMemory(device, blas.transform_buffer_memory);
	vma.freeMemory(device, blas.memory, nullptr);
	vma.freeMemory(device, blas.transform_buffer_memory, nullptr);
	vkDestroyBuffer(device, blas.buffer, nullptr);
	vkDestroyBuffer(device, blas.transform_buffer, nullptr);

	blas = {};
}

int VulkanAPI::createInstance(const BottomLevelAS & blas, const glm::mat4 & transform)
{
	int instance_index = -1;
	for (uint32_t i = 0; i < instance_list.size(); i++)
	{
		if (instance_list[i].is_used == false)
		{
			instance_index = i;
			break;
		}
	}
	if (instance_index == -1)
	{
		instance_index = instance_list.size();
		instance_list.push_back({});
	}

	VkAccelerationStructureInstanceKHR instance = {};
	instance.transform = glmToVkTransformMatrix(transform);
	instance.instanceCustomIndex = blas.instance_custom_index;
	instance.mask = 0xFF;
	instance.instanceShaderBindingTableRecordOffset = 0;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	instance.accelerationStructureReference = blas.address;

	VkBuffer instance_buffer;
	VkDeviceMemory instance_buffer_memory;
	createBuffer(
		sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		instance_buffer,
		instance_buffer_memory
	);

	void * data;
	vkMapMemory(device, instance_buffer_memory, 0, sizeof(VkAccelerationStructureInstanceKHR), 0, &data);
	memcpy(data, &instance, sizeof(VkAccelerationStructureInstanceKHR));

	VkDeviceAddress instance_address = getBufferDeviceAddress(instance_buffer);

	instance_list[instance_index] = InstanceData({
		.is_used = true,
		.transform = transform,
		.blas_address = blas.address,
		.custom_index = blas.instance_custom_index,
		.buffer = instance_buffer,
		.memory = instance_buffer_memory,
		.address = instance_address,
		.mapped_memory = data
	});

	return instance_index;
}

void VulkanAPI::destroyInstance(InstanceData & instance)
{
	if (instance.is_used == false)
	{
		return;
	}

	vkUnmapMemory(device, instance.memory);
	vma.freeMemory(device, instance.memory, nullptr);
	vkDestroyBuffer(device, instance.buffer, nullptr);

	instance = {};
}

void VulkanAPI::createMeshDataBuffer()
{
	std::vector<RTMeshData> mesh_data;
	for (uint32_t i = 0; i < blas_list.size(); i++)
	{
		if (blas_list[i].is_used == false)
		{
			mesh_data.push_back({});
			continue;
		}

		Mesh & mesh = mesh_map.at(blas_list[i].mesh_id);

		RTMeshData data = {};
		data.vertex_buffer_address = getBufferDeviceAddress(mesh.buffer);
		data.index_buffer_address = data.vertex_buffer_address + mesh.index_offset;

		mesh_data.push_back(data);
	}

	rt_mesh_data_buffer_count = mesh_data.size();
	rt_mesh_data_buffer_size = sizeof(RTMeshData) * mesh_data.size();

	createBuffer(
		rt_mesh_data_buffer_size,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		rt_mesh_data_buffer,
		rt_mesh_data_buffer_memory
	);

	void * data;
	vkMapMemory(device, rt_mesh_data_buffer_memory, 0, rt_mesh_data_buffer_size, 0, &data);
	memcpy(data, mesh_data.data(), rt_mesh_data_buffer_size);
	vkUnmapMemory(device, rt_mesh_data_buffer_memory);
}

void VulkanAPI::createTopLevelAS(const std::vector<InstanceData> & instance_list)
{
	std::vector<VkDeviceOrHostAddressConstKHR> instance_addresses;
	for (uint32_t i = 0; i < instance_list.size(); i++)
	{
		if (instance_list[i].is_used)
		{
			instance_addresses.push_back({
				.deviceAddress = instance_list[i].address
			});
		}
	}

	VkBuffer as_instances_buffer;
	VkDeviceMemory as_instances_buffer_memory;
	VkDeviceSize buffer_size = sizeof(VkDeviceOrHostAddressConstKHR) * instance_addresses.size();
	createBuffer(
		buffer_size,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		as_instances_buffer,
		as_instances_buffer_memory
	);

	void * data;
	vkMapMemory(device, as_instances_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, instance_addresses.data(), buffer_size);
	vkUnmapMemory(device, as_instances_buffer_memory);

	VkAccelerationStructureGeometryKHR as_geometry = {};
	as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	as_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	as_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	as_geometry.geometry.instances.arrayOfPointers = VK_TRUE;
	as_geometry.geometry.instances.data.deviceAddress = getBufferDeviceAddress(as_instances_buffer);

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.geometryCount = 1;
	build_info.pGeometries = &as_geometry;

	uint32_t primitive_count = instance_addresses.size();

	VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = {};
	build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&build_info,
		&primitive_count,
		&build_sizes_info
	);

	VkBuffer scratch_buffer;
	VkDeviceMemory scratch_buffer_memory;
	createBuffer(
		build_sizes_info.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		scratch_buffer,
		scratch_buffer_memory
	);

	createBuffer(
		build_sizes_info.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		tlas_buffer,
		tlas_buffer_memory
	);

	VkAccelerationStructureCreateInfoKHR acceleration_structure_info = {};
	acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	acceleration_structure_info.buffer = tlas_buffer;
	acceleration_structure_info.size = build_sizes_info.accelerationStructureSize;

	VK_CHECK(
		vkCreateAccelerationStructureKHR(device, &acceleration_structure_info, nullptr, &tlas),
		"Failed to create bottom level acceleration structure"
	);

	build_info.scratchData.deviceAddress = getBufferDeviceAddress(scratch_buffer);
	build_info.dstAccelerationStructure = tlas;

	VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};
	build_range_info.primitiveCount = primitive_count;
	build_range_info.primitiveOffset = 0;
	build_range_info.firstVertex = 0;
	build_range_info.transformOffset = 0;

	std::vector<VkAccelerationStructureBuildRangeInfoKHR *> build_range_infos = { &build_range_info };


	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	vkCmdBuildAccelerationStructuresKHR(
		command_buffer,
		1,
		&build_info,
		build_range_infos.data()
	);

	endSingleTimeCommands(command_buffer);

	vkDestroyBuffer(device, scratch_buffer, nullptr);
	vma.freeMemory(device, scratch_buffer_memory, nullptr);

	vkDestroyBuffer(device, as_instances_buffer, nullptr);
	vma.freeMemory(device, as_instances_buffer_memory, nullptr);
}

void VulkanAPI::createRayTracingUBO()
{
	createUBO(rt_camera_ubo, sizeof(RTCameraMatrices), max_frames_in_flight);
}

void VulkanAPI::createRayTracingImages()
{
	{ // Lighting image
		rt_lighting_image_width = swapchain.extent.width * 2;
		rt_lighting_image_height = swapchain.extent.height * 2;

		createImage(
			rt_lighting_image_width,
			rt_lighting_image_height,
			1,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			rt_lighting_image,
			rt_lighting_image_memory
		);

		createImageView(
			rt_lighting_image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			rt_lighting_image_view
		);

		transitionImageLayout(
			rt_lighting_image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1,
			0,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		);
	}

	{ // Shadow image
		rt_shadow_image_width = swapchain.extent.width * 2;
		rt_shadow_image_height = swapchain.extent.height * 2;

		createImage(
			rt_shadow_image_width,
			rt_shadow_image_height,
			1,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			rt_shadow_image,
			rt_shadow_image_memory
		);

		createImageView(
			rt_shadow_image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			rt_shadow_image_view
		);

		transitionImageLayout(
			rt_shadow_image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1,
			0,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		);
	}

	{ // Output image
		rt_output_image_width = swapchain.extent.width * 2;
		rt_output_image_height = swapchain.extent.height * 2;

		createImage(
			rt_output_image_width,
			rt_output_image_height,
			1,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			rt_output_image,
			rt_output_image_memory
		);

		createImageView(
			rt_output_image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			rt_output_image_view
		);

		transitionImageLayout(
			rt_output_image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1,
			0,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		);
	}
}

void VulkanAPI::createRayTracingDescriptor()
{
	{ // Lighting and shadow image descriptor
		VkDescriptorSetLayoutBinding lighting_image_binding = {};
		lighting_image_binding.binding = 0;
		lighting_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		lighting_image_binding.descriptorCount = 1;
		lighting_image_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
		lighting_image_binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding shadow_image_binding = {};
		shadow_image_binding.binding = 1;
		shadow_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		shadow_image_binding.descriptorCount = 1;
		shadow_image_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
		shadow_image_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = {
			lighting_image_binding,
			shadow_image_binding
		};
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);
		descriptor_info.set_count = static_cast<uint32_t>(max_frames_in_flight);

		rt_lighting_shadow_image_descriptor = Descriptor(device, descriptor_info);
	}

	{ // Output image descriptor
		VkDescriptorSetLayoutBinding result_image_binding = {};
		result_image_binding.binding = 0;
		result_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		result_image_binding.descriptorCount = 1;
		result_image_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
		result_image_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = {
			result_image_binding
		};
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);
		descriptor_info.set_count = static_cast<uint32_t>(max_frames_in_flight);

		rt_output_image_descriptor = Descriptor(device, descriptor_info);
	}

	{ // Objects Descriptor
		VkDescriptorSetLayoutBinding acceleration_structure_binding = {};
		acceleration_structure_binding.binding = 0;
		acceleration_structure_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		acceleration_structure_binding.descriptorCount = 1;
		acceleration_structure_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		acceleration_structure_binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding mesh_data_binding = {};
		mesh_data_binding.binding = 1;
		mesh_data_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		mesh_data_binding.descriptorCount = 1;
		mesh_data_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		mesh_data_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = {
			acceleration_structure_binding,
			mesh_data_binding
		};
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);
		descriptor_info.set_count = static_cast<uint32_t>(max_frames_in_flight);

		rt_objects_descriptor = Descriptor(device, descriptor_info);
	}

	{ // Camera Descriptor
		VkDescriptorSetLayoutBinding camera_binding = {};
		camera_binding.binding = 0;
		camera_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		camera_binding.descriptorCount = 1;
		camera_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		camera_binding.pImmutableSamplers = nullptr;

		Descriptor::CreateInfo descriptor_info = {};
		descriptor_info.bindings = {
			camera_binding
		};
		descriptor_info.descriptor_count = static_cast<uint32_t>(max_frames_in_flight);
		descriptor_info.set_count = static_cast<uint32_t>(max_frames_in_flight);

		rt_camera_descriptor = Descriptor(device, descriptor_info);

		for (int i = 0; i < max_frames_in_flight; i++)
		{
			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = rt_camera_ubo.buffers[i];
			buffer_info.offset = 0;
			buffer_info.range = sizeof(ViewProjMatrices);

			VkWriteDescriptorSet descriptor_write = {};
			descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_write.dstSet = rt_camera_descriptor.sets[i];
			descriptor_write.dstBinding = 0;
			descriptor_write.dstArrayElement = 0;
			descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_write.descriptorCount = 1;
			descriptor_write.pBufferInfo = &buffer_info;

			vkUpdateDescriptorSets(
				device,
				1,
				&descriptor_write,
				0, nullptr
			);
		}
	}
}

void VulkanAPI::updateRTImagesDescriptor()
{
	VkDescriptorImageInfo lighting_image_info = {};
	lighting_image_info.imageView = rt_lighting_image_view;
	lighting_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorImageInfo shadow_image_info = {};
	shadow_image_info.imageView = rt_shadow_image_view;
	shadow_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorImageInfo output_image_info = {};
	output_image_info.imageView = rt_output_image_view;
	output_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		VkWriteDescriptorSet lighting_image_write = {};
		lighting_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		lighting_image_write.dstSet = rt_lighting_shadow_image_descriptor.sets[i];
		lighting_image_write.dstBinding = 0;
		lighting_image_write.descriptorCount = 1;
		lighting_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		lighting_image_write.pImageInfo = &lighting_image_info;

		VkWriteDescriptorSet shadow_image_write = {};
		shadow_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		shadow_image_write.dstSet = rt_lighting_shadow_image_descriptor.sets[i];
		shadow_image_write.dstBinding = 1;
		shadow_image_write.descriptorCount = 1;
		shadow_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		shadow_image_write.pImageInfo = &shadow_image_info;

		VkWriteDescriptorSet output_image_write = {};
		output_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		output_image_write.dstSet = rt_output_image_descriptor.sets[i];
		output_image_write.dstBinding = 0;
		output_image_write.descriptorCount = 1;
		output_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		output_image_write.pImageInfo = &output_image_info;


		std::vector<VkWriteDescriptorSet> writes = {
			lighting_image_write,
			shadow_image_write,
			output_image_write
		};

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
}

void VulkanAPI::updateRTObjectsDescriptor()
{
	VkWriteDescriptorSetAccelerationStructureKHR acceleration_structure_info = {};
	acceleration_structure_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	acceleration_structure_info.accelerationStructureCount = 1;
	acceleration_structure_info.pAccelerationStructures = &tlas;

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = rt_mesh_data_buffer;
	buffer_info.offset = 0;
	buffer_info.range = rt_mesh_data_buffer_size;

	for (int i = 0; i < max_frames_in_flight; i++)
	{
		VkWriteDescriptorSet acceleration_structure_write = {};
		acceleration_structure_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		acceleration_structure_write.dstSet = rt_objects_descriptor.sets[i];
		acceleration_structure_write.dstBinding = 0;
		acceleration_structure_write.descriptorCount = 1;
		acceleration_structure_write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		acceleration_structure_write.pNext = &acceleration_structure_info;

		VkWriteDescriptorSet buffer_write = {};
		buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		buffer_write.dstSet = rt_objects_descriptor.sets[i];
		buffer_write.dstBinding = 1;
		buffer_write.descriptorCount = 1;
		buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		buffer_write.pBufferInfo = &buffer_info;


		std::vector<VkWriteDescriptorSet> writes = {
			acceleration_structure_write,
			buffer_write
		};

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
}

void VulkanAPI::createRayTracingPipeline()
{
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

	#define LOAD_SHADER_STAGE(stage_name, path) \
	{ \
		auto code = readFile(path); \
		VkShaderModule module = createShaderModule(code); \
		VkPipelineShaderStageCreateInfo stage_info = {}; \
		stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; \
		stage_info.stage = stage_name; \
		stage_info.module = module; \
		stage_info.pName = "main"; \
		shader_stages.push_back(stage_info); \
	}

	LOAD_SHADER_STAGE(VK_SHADER_STAGE_RAYGEN_BIT_KHR, "shaders/ray_tracing/raytrace.rgen.spv");
	LOAD_SHADER_STAGE(VK_SHADER_STAGE_MISS_BIT_KHR, "shaders/ray_tracing/sky.rmiss.spv");
	LOAD_SHADER_STAGE(VK_SHADER_STAGE_MISS_BIT_KHR, "shaders/ray_tracing/shadow.rmiss.spv");
	LOAD_SHADER_STAGE(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "shaders/ray_tracing/raytrace.rchit.spv");

	#undef LOAD_SHADER_STAGE

	VkRayTracingShaderGroupCreateInfoKHR raygen_group = {};
	raygen_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	raygen_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	raygen_group.generalShader = 0;
	raygen_group.closestHitShader = VK_SHADER_UNUSED_KHR;
	raygen_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	raygen_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	VkRayTracingShaderGroupCreateInfoKHR sky_miss_group = {};
	sky_miss_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	sky_miss_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	sky_miss_group.generalShader = 1;
	sky_miss_group.closestHitShader = VK_SHADER_UNUSED_KHR;
	sky_miss_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	sky_miss_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	VkRayTracingShaderGroupCreateInfoKHR shadow_miss_group = {};
	shadow_miss_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	shadow_miss_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	shadow_miss_group.generalShader = 2;
	shadow_miss_group.closestHitShader = VK_SHADER_UNUSED_KHR;
	shadow_miss_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	shadow_miss_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	VkRayTracingShaderGroupCreateInfoKHR hit_group = {};
	hit_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	hit_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	hit_group.generalShader = VK_SHADER_UNUSED_KHR;
	hit_group.closestHitShader = 3;
	hit_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	hit_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups = {
		raygen_group,
		sky_miss_group,
		shadow_miss_group,
		hit_group
	};


	std::vector<VkDescriptorSetLayout> descriptor_set_layouts = {
		rt_lighting_shadow_image_descriptor.layout,
		rt_objects_descriptor.layout,
		rt_camera_descriptor.layout,
		block_textures_descriptor.layout,
		atmosphere_descriptor.layout
	};

	VkPushConstantRange push_constant_range = {};
	push_constant_range.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(RTPushConstant);

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
	pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	VK_CHECK(
		vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &rt_pipeline_layout),
		"Failed to create ray tracing pipeline layout"
	);

	VkRayTracingPipelineCreateInfoKHR pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.groupCount = static_cast<uint32_t>(groups.size());
	pipeline_info.pGroups = groups.data();
	pipeline_info.maxPipelineRayRecursionDepth = 2;
	pipeline_info.layout = rt_pipeline_layout;

	VK_CHECK(
		vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &rt_pipeline),
		"Failed to create ray tracing pipeline"
	);

	for (auto & stage : shader_stages)
	{
		vkDestroyShaderModule(device, stage.module, nullptr);
	}
}

void VulkanAPI::createComputePipeline()
{
	auto code = readFile("shaders/compute/shading.comp.spv");
	VkShaderModule module = createShaderModule(code);

	VkPipelineShaderStageCreateInfo stage_info = {};
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage_info.module = module;
	stage_info.pName = "main";

	std::vector<VkDescriptorSetLayout> descriptor_set_layouts = {
		rt_lighting_shadow_image_descriptor.layout,
		rt_output_image_descriptor.layout
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
	pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	VK_CHECK(
		vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &compute_pipeline_layout),
		"Failed to create compute pipeline layout"
	);

	VkComputePipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeline_info.stage = stage_info;
	pipeline_info.layout = compute_pipeline_layout;

	VK_CHECK(
		vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &compute_pipeline),
		"Failed to create compute pipeline"
	);

	vkDestroyShaderModule(device, module, nullptr);
}

uint32_t alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanAPI::createRayTracingShaderBindingTable()
{
	uint32_t miss_count = 2;
	uint32_t hit_count = 1;
	uint32_t handle_count = 1 + miss_count + hit_count;
	uint32_t handle_size = ray_tracing_properties.shaderGroupHandleSize;
	uint32_t handle_size_aligned = alignedSize(handle_size, ray_tracing_properties.shaderGroupHandleAlignment);

	rt_sbt_rgen_region.stride = alignedSize(handle_size_aligned, ray_tracing_properties.shaderGroupBaseAlignment);
	rt_sbt_rgen_region.size = rt_sbt_rgen_region.stride;
	rt_sbt_miss_region.stride = handle_size_aligned;
	rt_sbt_miss_region.size = alignedSize(handle_size_aligned * miss_count, ray_tracing_properties.shaderGroupBaseAlignment);
	rt_sbt_hit_region.stride = handle_size_aligned;
	rt_sbt_hit_region.size = alignedSize(handle_size_aligned * hit_count, ray_tracing_properties.shaderGroupBaseAlignment);

	uint32_t data_size = handle_count * handle_size;
	std::vector<uint8_t> handles(data_size);
	VK_CHECK(
		vkGetRayTracingShaderGroupHandlesKHR(device, rt_pipeline, 0, handle_count, data_size, handles.data()),
		"Failed to get ray tracing shader group handles"
	);

	uint32_t sbt_size = rt_sbt_rgen_region.size + rt_sbt_miss_region.size + rt_sbt_hit_region.size;
	createBuffer(
		sbt_size,
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		rt_sbt_buffer,
		rt_sbt_buffer_memory
	);

	VkDeviceAddress sbt_address = getBufferDeviceAddress(rt_sbt_buffer);
	rt_sbt_rgen_region.deviceAddress = sbt_address;
	rt_sbt_miss_region.deviceAddress = sbt_address + rt_sbt_rgen_region.size;
	rt_sbt_hit_region.deviceAddress = sbt_address + rt_sbt_rgen_region.size + rt_sbt_miss_region.size;

	auto getHandle = [&] (int i) { return handles.data() + i * handle_size; };

	uint32_t handle_index = 0;
	void * mapped_memory_tmp;
	vkMapMemory(device, rt_sbt_buffer_memory, 0, sbt_size, 0, &mapped_memory_tmp);

	uint8_t * mapped_memory = static_cast<uint8_t *>(mapped_memory_tmp);

	// raygen
	uint8_t * data = mapped_memory;
	memcpy(data, getHandle(handle_index++), handle_size);

	// miss
	data = mapped_memory + rt_sbt_rgen_region.size;
	for (uint32_t i = 0; i < miss_count; i++)
	{
		memcpy(data, getHandle(handle_index++), handle_size);
		data += rt_sbt_rgen_region.stride;
	}

	// hit
	data = mapped_memory + rt_sbt_rgen_region.size + rt_sbt_miss_region.size;
	for (uint32_t i = 0; i < hit_count; i++)
	{
		memcpy(data, getHandle(handle_index++), handle_size);
		data += rt_sbt_rgen_region.stride;
	}

	vkUnmapMemory(device, rt_sbt_buffer_memory);
}

std::vector<char> VulkanAPI::readFile(const std::string & filename)
{
	std::ifstream file
	{
		filename,
		std::ios::ate | std::ios::binary
	};

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}

	size_t file_size = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
}

VkShaderModule VulkanAPI::createShaderModule(const std::vector<char> & code)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

	VkShaderModule shader_module;
	VK_CHECK(
		vkCreateShaderModule(device, &create_info, nullptr, &shader_module),
		"Failed to create shader module"
	);

	return shader_module;
}

void VulkanAPI::addMeshToTopLevelAS(
	const uint64_t mesh_id,
	const glm::mat4 & transform
)
{
	try
	{
		int blas_id = createBottomLevelAS(mesh_id);

		VkBuffer rt_mesh_data_buffer_tmp = rt_mesh_data_buffer;
		VkDeviceMemory rt_mesh_data_buffer_memory_tmp = rt_mesh_data_buffer_memory;
		createMeshDataBuffer();

		createInstance(blas_list[blas_id], transform);

		VkAccelerationStructureKHR tlas_tmp = tlas;
		VkBuffer tlas_buffer_tmp = tlas_buffer;
		VkDeviceMemory tlas_buffer_memory_tmp = tlas_buffer_memory;
		createTopLevelAS(instance_list);

		updateRTObjectsDescriptor();


		if (rt_mesh_data_buffer_tmp != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, rt_mesh_data_buffer_tmp, nullptr);
			vma.freeMemory(device, rt_mesh_data_buffer_memory_tmp, nullptr);
		}

		if (tlas_tmp != VK_NULL_HANDLE)
		{
			vkDestroyAccelerationStructureKHR(device, tlas_tmp, nullptr);
			vkDestroyBuffer(device, tlas_buffer_tmp, nullptr);
			vma.freeMemory(device, tlas_buffer_memory_tmp, nullptr);
		}
	}
	catch (const std::exception & e)
	{
		LOG_ERROR("Failed to add mesh to top level AS: " << e.what());
	}
}

void VulkanAPI::removeMeshFromTopLevelAS(const uint64_t mesh_id)
{
	try
	{
		// Find the BLAS with the mesh id and destroy it
		for (uint32_t i = 0; i < blas_list.size(); i++)
		{
			if (blas_list[i].mesh_id == mesh_id)
			{
				destroyBottomLevelAS(blas_list[i]);
				break;
			}
		}

		VkBuffer rt_mesh_data_buffer_tmp = rt_mesh_data_buffer;
		VkDeviceMemory rt_mesh_data_buffer_memory_tmp = rt_mesh_data_buffer_memory;
		createMeshDataBuffer();

		VkAccelerationStructureKHR tlas_tmp = tlas;
		VkBuffer tlas_buffer_tmp = tlas_buffer;
		VkDeviceMemory tlas_buffer_memory_tmp = tlas_buffer_memory;
		createTopLevelAS(instance_list);

		updateRTObjectsDescriptor();

		if (rt_mesh_data_buffer_tmp != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, rt_mesh_data_buffer_tmp, nullptr);
			vma.freeMemory(device, rt_mesh_data_buffer_memory_tmp, nullptr);
		}

		if (tlas_tmp != VK_NULL_HANDLE)
		{
			vkDestroyAccelerationStructureKHR(device, tlas_tmp, nullptr);
			vkDestroyBuffer(device, tlas_buffer_tmp, nullptr);
			vma.freeMemory(device, tlas_buffer_memory_tmp, nullptr);
		}
	}
	catch (const std::exception & e)
	{
		LOG_ERROR("Failed to remove mesh from top level AS: " << e.what());
	}
}

VkTransformMatrixKHR VulkanAPI::glmToVkTransformMatrix(const glm::mat4 & matrix)
{
	return {
		matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
		matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
		matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2]
	};
}

void VulkanAPI::updateInstanceTransform(const uint32_t instance_id, const glm::mat4 & transform)
{
	VkTransformMatrixKHR transform_matrix = glmToVkTransformMatrix(transform);

	memcpy(instance_list[instance_id].mapped_memory, &transform_matrix, sizeof(VkTransformMatrixKHR));
}

void VulkanAPI::addMeshToScene(const uint64_t mesh_id, const glm::mat4 & transform)
{
	std::lock_guard lock(global_mutex);
	addMeshToTopLevelAS(mesh_id, transform);
}

void VulkanAPI::removeMeshFromScene(const uint64_t mesh_id)
{
	std::lock_guard lock(global_mutex);
	removeMeshFromTopLevelAS(mesh_id);
}
