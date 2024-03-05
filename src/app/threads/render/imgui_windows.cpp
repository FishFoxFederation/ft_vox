#include "RenderThread.hpp"
#include "logger.hpp"
#include "Debug.hpp"

void RenderThread::updateImGui()
{
	// ImGui::ShowDemoWindow();

	ImGui::Begin("Debug");

	ImGui::Text("Frame time: %f ms", m_delta_time.count() / 1e6);
	ImGui::Text("CPU rendering time: %ld ms",
		std::chrono::duration_cast<std::chrono::milliseconds>(m_end_cpu_rendering_time - m_start_cpu_rendering_time).count()
	);
	ImGui::Text("FPS: %f", m_fps);
	ImGui::Text("Triangle count: %d", m_triangle_count);
	ImGui::Text("Debug triangle count: %d", Debug<int>::get("triangle_count"));


	// write to texture
	// vk.imgui_texture.clear();
	// for (uint32_t x = 0; x < vk.imgui_texture.width(); x++)
	// {
	// 	for (uint32_t y = 0; y < vk.imgui_texture.height(); y++)
	// 	{
	// 		vk.imgui_texture.putPixel(x, y, 255, 0, 0, 255);
	// 	}
	// }

	// display image
	// ImGui::Image(
	// 	(void *)vk.imgui_texture.descriptor_set,
	// 	ImVec2(vk.imgui_texture.extent.width, vk.imgui_texture.extent.height)
	// );

	ImGui::End();
}
