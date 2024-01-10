#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "../swapchain.h"
#include "../input_event.h"

#include "cimgui.h"

typedef struct ImGui_ImplModeler_Data_t {
	long time;
	SwapchainInfo *swapchainInfo;
} ImGui_ImplModeler_Data;

CIMGUI_IMPL_API bool ImGui_ImplModeler_Init(SwapchainInfo *swapchainInfo);
CIMGUI_IMPL_API void ImGui_ImplModeler_NewFrame(void);
CIMGUI_IMPL_API void ImGui_ImplModeler_HandleInput(InputEvent *inputEvent);