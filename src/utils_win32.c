#include <windows.h>

#include "modeler_win32.h"
#include "utils.h"

VkExtent2D getWindowExtent(void *platformWindow)
{
	HWND hWnd = ((Win32Window *) platformWindow)->hWnd;
	RECT rect;
	VkExtent2D windowExtent = {};
	if (GetClientRect(hWnd, &rect)) {
		windowExtent = (VkExtent2D) {
			.width = rect.right - rect.left,
			.height = rect.bottom - rect.top
		};
	}

	return windowExtent;
};