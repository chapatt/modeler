#include <time.h>

#include "imgui_impl_modeler.h"

bool ImGui_ImplModeler_Init(VkExtent2D windowExtent)
{
	ImGuiIO *io = ImGui_GetIO();
	ImGui_ImplModeler_Data* bd = (ImGui_ImplModeler_Data *) ImGui_MemAlloc(sizeof(ImGui_ImplModeler_Data));
	bd->time = 0;
	bd->windowExtent = windowExtent;
	io->BackendPlatformUserData = bd;

	return true;
}

void ImGui_ImplModeler_NewFrame(void)
{
	struct timespec spec;
	ImGuiIO *io = ImGui_GetIO();
	ImGui_ImplModeler_Data *bd = ImGui_GetCurrentContext() ? (ImGui_ImplModeler_Data *) io->BackendPlatformUserData : NULL;
	const float scaleX = 1;
	const float scaleY = 1;
	ImVec2 size = {
		bd->windowExtent.width,
		bd->windowExtent.height
	};
	ImVec2 scale = {
		scaleX,
		scaleY
	};
	io->DisplaySize = size;
	io->DisplayFramebufferScale = scale;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	io->DeltaTime = spec.tv_nsec - bd->time;
	bd->time = spec.tv_nsec;
}