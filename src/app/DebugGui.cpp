#include "DebugGui.hpp"

#include <algorithm>

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
			if (ImGui::BeginTabItem("Performance"))
			{
				if (ImGui::CollapsingHeader("Render Thread"))
				{
					ImGui::Text("Fps: %d", fps.load());

					{
						std::lock_guard<std::mutex> lock(frame_time_history_mutex);
						ImGui::PlotHistogram("", frame_time_history.data(), frame_time_history.size(), 0, "Frame time", 0.0f, 100.0f, ImVec2(200, 80));
					}

					ImGui::Text("Cpu time: %.2f ms", cpu_time.load());
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
				ImGui::Text("Triangle count: %ld", triangle_count.load());

				ImGui::Text("Gpu memory: %ld MB", vk.vma.allocatedMemorySize() / 1024 / 1024);

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	
	}
	ImGui::End();
}

void DebugGui::pushFrameTime(float frame_time)
{
	std::lock_guard<std::mutex> lock(frame_time_history_mutex);

	std::shift_left(frame_time_history.begin(), frame_time_history.end(), 1);
	frame_time_history.back() = frame_time;
}