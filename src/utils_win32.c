#include <windows.h>
#include <windowsx.h>

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

float getWindowScale(void *platformWindow)
{
	typedef UINT (WINAPI *PFN_GetDpiForWindow)(HWND hwnd);
	PFN_GetDpiForWindow _GetDpiForWindow = (PFN_GetDpiForWindow) GetProcAddress(GetModuleHandle(L"User32.dll"), "GetDpiForWindow");
	HWND hWnd = ((Win32Window *) platformWindow)->hWnd;
	unsigned int dpi = _GetDpiForWindow(hWnd);
	return dpi / 96.0f;
}