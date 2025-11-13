#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "imgui_impl_modeler.h"

CIMGUI_IMPL_API bool ImGui_ImplModeler_Init(WindowDimensions *windowDimensions)
{
	ImGuiIO *io = ImGui_GetIO();
	ImGui_ImplModeler_Data* bd = (ImGui_ImplModeler_Data *) ImGui_MemAlloc(sizeof(ImGui_ImplModeler_Data));
	bd->time = 0;
	bd->windowDimensions = windowDimensions;
	io->BackendPlatformUserData = bd;

	return true;
}

CIMGUI_IMPL_API void ImGui_ImplModeler_NewFrame(void)
{
	struct timespec spec;
	ImGuiIO *io = ImGui_GetIO();
	ImGui_ImplModeler_Data *bd = ImGui_GetCurrentContext() ? (ImGui_ImplModeler_Data *) io->BackendPlatformUserData : NULL;
	const float scaleX = 1;
	const float scaleY = 1;
	ImVec2 size = {
		bd->windowDimensions->surfaceArea.width,
		bd->windowDimensions->surfaceArea.height
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

CIMGUI_IMPL_API void ImGui_ImplModeler_HandleInput(InputEvent *inputEvent)
{
	ImGuiIO *io = ImGui_GetIO();
	ImGui_ImplModeler_Data *bd = ImGui_GetCurrentContext() ? (ImGui_ImplModeler_Data *) io->BackendPlatformUserData : NULL;
	PointerPosition *data = inputEvent->data;

	switch (inputEvent->type) {
	case POINTER_MOVE:
		ImGuiIO_AddMouseSourceEvent(io, ImGuiMouseSource_Mouse);
                switch (bd->windowDimensions->orientation) {
                case ROTATE_0:
                    ImGuiIO_AddMousePosEvent(io, data->x, data->y);
                    break;
                case ROTATE_90:
                     ImGuiIO_AddMousePosEvent(io, io->DisplaySize.y - data->y, data->x);
                    break;
                case ROTATE_270:
                     ImGuiIO_AddMousePosEvent(io, data->y, io->DisplaySize.x - data->x);
                    break;
                case ROTATE_180:
                    ImGuiIO_AddMousePosEvent(io, io->DisplaySize.x - data->x, io->DisplaySize.y - data->y);
                    break;
                }
		break;
	case BUTTON_DOWN:
		ImGuiIO_AddMouseSourceEvent(io, ImGuiMouseSource_Mouse);
		ImGuiIO_AddMouseButtonEvent(io, 0, true);
		break;
	case BUTTON_UP: case POINTER_LEAVE:
		ImGuiIO_AddMouseSourceEvent(io, ImGuiMouseSource_Mouse);
		ImGuiIO_AddMouseButtonEvent(io, 0, false);
		break;
	}
}