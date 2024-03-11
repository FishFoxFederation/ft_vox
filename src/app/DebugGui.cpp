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

				ImGui::Text("Last position: %11f / %11f / %11f", camera_last_position.get().x, camera_last_position.get().y, camera_last_position.get().z);
				ImGui::Text("Displacement:  %11f / %11f / %11f", camera_displacement.get().x, camera_displacement.get().y, camera_displacement.get().z);
				ImGui::Text("New position:  %11f / %11f / %11f", camera_new_position.get().x, camera_new_position.get().y, camera_new_position.get().z);
				ImGui::Text("New position - last position: %11f / %11f / %11f", camera_position_sub_last_position.get().x, camera_position_sub_last_position.get().y, camera_position_sub_last_position.get().z);
				// ImGui::Text("Update time: %f ms", camera_update_time.load());

				ImGui::Separator();

				ImGui::Text("Rendered triangles: %ld", rendered_triangles.load());

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
						ImGui::PlotHistogram("Frame time", frame_time_history.data(), frame_time_history.size(), 0, average.c_str(), 0.0f, 100.0f, ImVec2(200, 80));
					}

					{
						std::string average = "Average: " + std::to_string(cpu_time_history.average()) + " ms";
						auto lock = cpu_time_history.lock();
						ImGui::PlotHistogram("Cpu time", cpu_time_history.data(), cpu_time_history.size(), 0, average.c_str(), 0.0f, 100.0f, ImVec2(200, 80));
					}
				}
				if (ImGui::CollapsingHeader("Update Thread"))
				{
					ImGui::Text("Fps: %d", fps.load());
				}
				if (ImGui::CollapsingHeader("Block Update Thread"))
				{
					ImGui::Text("Fps: %d", fps.load());
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
