#include <stdio.h>
#include <stdlib.h>
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
	io->DeltaTime = 5;
	// clock_gettime(CLOCK_MONOTONIC, &spec);
	// io->DeltaTime = spec.tv_nsec - bd->time;
	// printf("DeltaTime: %d, time: %d, tv_nsec: %d", io->DeltaTime, bd->time, spec.tv_nsec);
	// bd->time = spec.tv_nsec;
}

void ImGui_ImplModeler_HandleInput(InputEvent *inputEvent)
{
	ImGuiIO *io = ImGui_GetIO();

	switch (inputEvent->type) {
	case POINTER_MOVE:
		MousePosition *data = inputEvent->data;
		ImGuiIO_AddMouseSourceEvent(io, ImGuiMouseSource_Mouse);
		ImGuiIO_AddMousePosEvent(io, data->x, data->y);
		free(data);
		free(inputEvent);
		break;
	case BUTTON_DOWN:
		ImGuiIO_AddMouseSourceEvent(io, ImGuiMouseSource_Mouse);
		ImGuiIO_AddMouseButtonEvent(io, 0, true);
		free(inputEvent);
		break;
	case BUTTON_UP:
		ImGuiIO_AddMouseSourceEvent(io, ImGuiMouseSource_Mouse);
		ImGuiIO_AddMouseButtonEvent(io, 0, false);
		free(inputEvent);
		break;
	}
}