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
						auto lock = chunk_load_queue_size_history.lock();
						ImGui::PlotHistogram("Load queue size", chunk_load_queue_size_history.data(), chunk_load_queue_size_history.size(), 0, "Chunks", 0.0f, 100.0f, ImVec2(200, 80));
					}
					{
						auto lock = chunk_unload_queue_size_history.lock();
						ImGui::PlotHistogram("Unload queue size", chunk_unload_queue_size_history.data(), chunk_unload_queue_size_history.size(), 0, "Chunks", 0.0f, 100.0f, ImVec2(200, 80));
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
