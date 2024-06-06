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
				ImGui::Text("XYZ: %.3f %.3f %.3f", player_position.get().x, player_position.get().y, player_position.get().z);
				ImGui::Text("V XYZ: %.3f %.3f %.3f", player_velocity_vec.get().x, player_velocity_vec.get().y, player_velocity_vec.get().z);
				ImGui::Text("Velocity: %.3f", player_velocity.load());
				ImGui::Text("Chunk: %f %f %f", std::floor(player_position.get().x / 16) , std::floor(player_position.get().y / 256), std::floor(player_position.get().z / 16));

				ImGui::Separator();

				ImGui::Text("Rendered triangles: %ld", rendered_triangles.load());
				ImGui::Text("Chunk meshes count: %d", chunk_mesh_count.load());

				ImGui::Separator();

				double acceleration_ = acceleration.load();
				double ground_friction_ = ground_friction.load();
				double air_friction_ = air_friction.load();
				double jump_force_ = jump_force.load();
				double gravity_ = gravity.load();

				ImGui::InputDouble("Acceleration", &acceleration_);
				ImGui::InputDouble("Ground friction", &ground_friction_);
				ImGui::InputDouble("Air friction", &air_friction_);
				ImGui::InputDouble("Jump force", &jump_force_);
				ImGui::InputDouble("Gravity", &gravity_);

				acceleration.store(acceleration_);
				ground_friction.store(ground_friction_);
				air_friction.store(air_friction_);
				jump_force.store(jump_force_);
				gravity.store(gravity_);

				ImGui::Separator();
				
				ImGui::Text("Send buffer size: %ld", send_buffer_size.load());
				ImGui::Text("Recv buffer size: %ld", recv_buffer_size.load());

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
					ImGui::Text("Ups: %d", ups.load());
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
