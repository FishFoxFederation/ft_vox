#include "DebugGui.hpp"

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

	ImGui::Begin("Debug");

	ImGui::Text("Fps: %d", fps.load());

	ImGui::Text("Triangle count: %ld", triangle_count.load());

	ImGui::Text("Gpu memory: %ld MB", vk.vma.allocatedMemorySize() / 1024 / 1024);

	ImGui::End();
}