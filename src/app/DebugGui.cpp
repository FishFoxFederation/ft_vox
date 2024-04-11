#include "DebugGui.hpp"
#include "logger.hpp"

#include <algorithm>
#include <numeric>

DebugGui::DebugGui()
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

					ImGui::Text("Chunk gen time:     %f µs", chunk_gen_time_history.average());
					ImGui::Text("Chunk unload time:  %f µs", chunk_unload_time_history.average());
					ImGui::Text("Chunk render time:  %f µs", chunk_render_time_history.average());
					ImGui::Text("  Chunk mesh create time:  %f µs", create_mesh_time.load());
					ImGui::Text("  Chunk mesh store time:   %f µs", store_mesh_time.load());
					ImGui::Text("    Mutex wait time:            %f µs", store_mesh_mutex_wait_time.load());
					ImGui::Text("    Staging buffer create time: %f µs", store_mesh_create_staging_buffer_time.load());
					ImGui::Text("    Memcpy time:                %f µs", store_mesh_memcpy_time.load());
					ImGui::Text("    Buffer create time:         %f µs", store_mesh_create_buffer_time.load());
					ImGui::Text("    Copy buffer time:           %f µs", store_mesh_copy_buffer_time.load());
					ImGui::Text("    Destroy buffer time:        %f µs", store_mesh_destroy_buffer_time.load());
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Memory"))
			{
				ImGui::Text("Gpu memory: %ld MB", gpu_allocated_memory / 1024 / 1024);

				ImGui::EndTabItem();
			}
			
			ImGui::EndTabBar();
		}
	
	}
	ImGui::End();
}
