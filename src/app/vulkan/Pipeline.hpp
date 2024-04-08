#pragma once

#include "vk_define.hpp"
#include "logger.hpp"

#include <string>
#include <vector>
#include <fstream>
#include <optional>

class Pipeline
{

public:

	struct CreateInfo
	{
		VkExtent2D extent;

		std::string vert_path;
		std::string frag_path;

		std::optional<VkVertexInputBindingDescription> binding_description = {};
		std::vector<VkVertexInputAttributeDescription> attribute_descriptions = {};

		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		std::vector<VkFormat> color_formats = {};
		VkFormat depth_format = VK_FORMAT_UNDEFINED;

		VkBool32 depth_bias_enable = VK_FALSE;
		float depth_bias_constant_factor = 0.0f;
		float depth_bias_clamp = 0.0f;
		float depth_bias_slope_factor = 0.0f;

		std::vector<VkDescriptorSetLayout> descriptor_set_layouts = {};
		std::vector<VkPushConstantRange> push_constant_ranges = {};

		VkRenderPass render_pass = VK_NULL_HANDLE;
	};

	Pipeline():
		pipeline(VK_NULL_HANDLE),
		layout(VK_NULL_HANDLE),
		m_device(VK_NULL_HANDLE)
	{
	}

	Pipeline(VkDevice device, const CreateInfo & create_info):
		m_device(device)
	{
		std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

		if (!create_info.vert_path.empty())
		{
			auto vert_code = readFile(create_info.vert_path);
			VkShaderModule vert_module = createShaderModule(vert_code);
			VkPipelineShaderStageCreateInfo vert_stage_info = {};
			vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vert_stage_info.module = vert_module;
			vert_stage_info.pName = "main";

			shader_stages.push_back(vert_stage_info);
		}

		if (!create_info.frag_path.empty())
		{
			auto frag_code = readFile(create_info.frag_path);
			VkShaderModule frag_module = createShaderModule(frag_code);
			VkPipelineShaderStageCreateInfo frag_stage_info = {};
			frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			frag_stage_info.module = frag_module;
			frag_stage_info.pName = "main";

			shader_stages.push_back(frag_stage_info);
		}


		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 0;
		vertex_input_info.pVertexBindingDescriptions = nullptr;
		vertex_input_info.vertexAttributeDescriptionCount = 0;
		vertex_input_info.pVertexAttributeDescriptions = nullptr;

		if (create_info.binding_description.has_value())
		{
			vertex_input_info.vertexBindingDescriptionCount = 1;
			vertex_input_info.pVertexBindingDescriptions = &create_info.binding_description.value();
		}
		if (!create_info.attribute_descriptions.empty())
		{
			vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(create_info.attribute_descriptions.size());
			vertex_input_info.pVertexAttributeDescriptions = create_info.attribute_descriptions.data();
		}


		VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
		input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology = create_info.topology;
		input_assembly.primitiveRestartEnable = VK_FALSE;


		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(create_info.extent.width);
		viewport.height = static_cast<float>(create_info.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = create_info.extent;

		VkPipelineViewportStateCreateInfo viewport_state = {};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &scissor;


		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = create_info.polygon_mode;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = create_info.cull_mode;
		rasterizer.frontFace = create_info.front_face;
		rasterizer.depthBiasEnable = create_info.depth_bias_enable;
		rasterizer.depthBiasConstantFactor = create_info.depth_bias_constant_factor;
		rasterizer.depthBiasClamp = create_info.depth_bias_clamp;
		rasterizer.depthBiasSlopeFactor = create_info.depth_bias_slope_factor;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


		VkPipelineColorBlendAttachmentState color_blend_attachment = {};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;

		std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments(create_info.color_formats.size(), color_blend_attachment);

		VkPipelineColorBlendStateCreateInfo color_blending = {};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.attachmentCount = static_cast<uint32_t>(color_blend_attachments.size());
		color_blending.pAttachments = color_blend_attachments.data();


		VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
		depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil.depthTestEnable = VK_TRUE;
		depth_stencil.depthWriteEnable = VK_TRUE;
		depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depth_stencil.depthBoundsTestEnable = VK_FALSE;
		depth_stencil.stencilTestEnable = VK_FALSE;

		if (create_info.depth_format != VK_FORMAT_UNDEFINED)
		{
			depth_stencil.depthTestEnable = VK_TRUE;
			depth_stencil.depthWriteEnable = VK_TRUE;
		}


		VkPipelineRenderingCreateInfo rendering_info = {};
		rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		rendering_info.colorAttachmentCount = static_cast<uint32_t>(create_info.color_formats.size());
		rendering_info.pColorAttachmentFormats = create_info.color_formats.data();
		rendering_info.depthAttachmentFormat = create_info.depth_format;


		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(create_info.descriptor_set_layouts.size());
		pipeline_layout_info.pSetLayouts = create_info.descriptor_set_layouts.data();
		pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(create_info.push_constant_ranges.size());
		pipeline_layout_info.pPushConstantRanges = create_info.push_constant_ranges.data();

		VK_CHECK(
			vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &layout),
			"Failed to create pipeline layout"
		);

		VkGraphicsPipelineCreateInfo pipeline_info = {};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
		pipeline_info.pStages = shader_stages.data();
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDepthStencilState = &depth_stencil;
		pipeline_info.layout = layout;

		if (create_info.render_pass == VK_NULL_HANDLE)
		{
			pipeline_info.pNext = &rendering_info;
		}
		else
		{
			pipeline_info.renderPass = create_info.render_pass;
		}

		VK_CHECK(
			vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline),
			"Failed to create graphics pipeline"
		);

		for (auto & shader_stage : shader_stages)
		{
			vkDestroyShaderModule(m_device, shader_stage.module, nullptr);
		}
	}


	Pipeline(const Pipeline &) = delete;
	Pipeline & operator=(const Pipeline &) = delete;

	Pipeline(Pipeline && other) noexcept:
		pipeline(other.pipeline),
		layout(other.layout),
		m_device(other.m_device)
	{
		other.pipeline = VK_NULL_HANDLE;
		other.layout = VK_NULL_HANDLE;
		other.m_device = VK_NULL_HANDLE;
	}

	Pipeline & operator=(Pipeline && other) noexcept
	{
		if (this != &other)
		{
			clear();

			pipeline = other.pipeline;
			layout = other.layout;
			m_device = other.m_device;

			other.pipeline = VK_NULL_HANDLE;
			other.layout = VK_NULL_HANDLE;
			other.m_device = VK_NULL_HANDLE;
		}

		return *this;
	}

	~Pipeline()
	{
		clear();
	}

	VkPipeline pipeline;
	VkPipelineLayout layout;

private:

	VkDevice m_device;

	void clear()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			if (pipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(m_device, pipeline, nullptr);
			}

			if (layout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(m_device, layout, nullptr);
			}
		}
	}

	std::vector<char> readFile(const std::string & filename)
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

	VkShaderModule createShaderModule(const std::vector<char> & code)
	{
		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

		VkShaderModule shader_module;
		VK_CHECK(
			vkCreateShaderModule(m_device, &create_info, nullptr, &shader_module),
			"Failed to create shader module"
		);

		return shader_module;
	}

};
