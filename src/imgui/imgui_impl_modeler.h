#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "../window.h"
#include "../input_event.h"

#include "dcimgui.h"

typedef struct ImGui_ImplModeler_Data_t {
	long time;
	WindowDimensions *windowDimensions;
} ImGui_ImplModeler_Data;

CIMGUI_IMPL_API bool ImGui_ImplModeler_Init(WindowDimensions *windowDimensions);
CIMGUI_IMPL_API void ImGui_ImplModeler_NewFrame(void);
CIMGUI_IMPL_API void ImGui_ImplModeler_HandleInput(InputEvent *inputEvent);