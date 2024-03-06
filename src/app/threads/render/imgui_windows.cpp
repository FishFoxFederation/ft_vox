#include "RenderThread.hpp"
#include "logger.hpp"

void RenderThread::updateImGui()
{
	// ImGui::ShowDemoWindow();

	// ImGui::Begin("Debug");

	// ImGui::Text("Frame time: %f ms", m_delta_time.count() / 1e6);

	// ImGui::Text("CPU rendering time: %ld ms", Debug<std::chrono::milliseconds>::get("cpu_rendering_time").count());

	// ImGui::Text("FPS: %d", Debug<int>::get("fps"));

	// ImGui::Text("Triangle count: %d", Debug<int>::get("triangle_count"));

	// ImGui::Text("Gpu memory for mesh: %ld MB", Debug<uint64_t>::get("mesh_memory_size") / 1024 / 1024);


	// auto frame_times = Debug<FrameTimeArray>::get("frame_times");
	// float max_frame_time = *std::max_element(frame_times.begin(), frame_times.end());
	// max_frame_time = std::max(max_frame_time, 50.0f);

	// ImGui::PlotHistogram("", frame_times.data(), frame_times.size(), 0, "frame time", 0, max_frame_time, ImVec2(0, 100));


	// // write to texture
	// vk.imgui_texture.clear();
	// for (uint32_t x = 0; x < vk.imgui_texture.width(); x++)
	// {
	// 	for (uint32_t y = 0; y < vk.imgui_texture.height(); y++)
	// 	{
	// 		vk.imgui_texture.putPixel(x, y, 255, 0, 0, 255);
	// 	}
	// }

	// // display image
	// ImGui::Image(
	// 	(void *)vk.imgui_texture.descriptor_set,
	// 	ImVec2(vk.imgui_texture.extent.width, vk.imgui_texture.extent.height)
	// );

	// ImGui::End();
}
