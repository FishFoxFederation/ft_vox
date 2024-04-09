#include "DebugGui.hpp"
#include "logger.hpp"

#include <algorithm>
#include <numeric>

DebugGui::DebugGui(VulkanAPI & vulkanAPI):
	vk(vulkanAPI)
{
}

DebugGui::~DebugGui()
{
}

void DebugGui::updateImGui()
{
	// ImGui::ShowDemoWindow();

	if (ImGui::Begin("Debug"))
	{
		if (ImGui::BeginTabBar("HeadTabBar"))
		{
			if (ImGui::BeginTabItem("Game"))
			{
				ImGui::Text("Fps: %d", fps.load());

				ImGui::Separator();

				ImGui::Text("Rendered triangles: %ld", rendered_triangles.load());
				ImGui::Text("Chunk meshes count: %d", chunk_mesh_count.load());

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Performance"))
			{
				if (ImGui::CollapsingHeader("Render Thread"))
				{
					ImGui::Text("Fps: %d", fps.load());

					{
						std::string average = "Average: " + std::to_string(frame_time_history.average()) + " ms";
						auto lock = frame_time_history.lock();
						ImGui::PlotHistogram("Frame time", frame_time_history.data(), frame_time_history.size(), 0, average.c_str(), 0.0f, 3000.0f, ImVec2(200, 80));
					}

					{
						std::string average = "Average: " + std::to_string(cpu_time_history.average()) + " ms";
						auto lock = cpu_time_history.lock();
						ImGui::PlotHistogram("Cpu time", cpu_time_history.data(), cpu_time_history.size(), 0, average.c_str(), 0.0f, 3000.0f, ImVec2(200, 80));
					}
				}
				if (ImGui::CollapsingHeader("Update Thread"))
				{
					ImGui::Text("Fps: %d", fps.load());
				}
				if (ImGui::CollapsingHeader("Block Update Thread"))
				{
					ImGui::Text("Fps: %d", fps.load());

					{
						ImGui::Text("Chunk gen time:     %f ms", chunk_gen_time_history.average());
					}
					{
						ImGui::Text("Chunk unload time:  %f ms", chunk_unload_time_history.average());
					}
					{
						ImGui::Text("Chunk render time:  %f ms", chunk_render_time_history.average());
					}

				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Memory"))
			{
				ImGui::Text("Gpu memory: %ld MB", vk.vma.allocatedMemorySize() / 1024 / 1024);

				ImGui::EndTabItem();
			}
			
			ImGui::EndTabBar();
		}
	
	}
	ImGui::End();
}
