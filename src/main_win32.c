#ifndef UNICODE
#define UNICODE
#endif 

#include <stdio.h>
#include <windows.h>
#include "modeler_win32.h"

const LPCTSTR CLASS_NAME = L"Modeler Window Class";
wchar_t *utf8ToUtf16(const char *utf8);
void handleFatalError(HWND hwnd, char *message);

LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = windowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = CLASS_NAME;
	wc.hIconSm = NULL;

	if (!RegisterClassEx(&wc)) {
		handleFatalError(NULL, "Can't register Windows window class");
	}

	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"Modeler",
		WS_POPUP,
		500, 500, 500, 300,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hwnd) {
		handleFatalError(NULL, "Can't create window.");
	}

	if (ShowWindow(hwnd, nCmdShow) != 0) {
		handleFatalError(NULL, "Can't show window.");
	}

	char *error;
	if (!initVulkanWin32(hInstance, hwnd, &error)) {
		handleFatalError(hwnd, error);
	}

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//	switch (uMsg) {
//	case WM_CREATE:
//		return 0;
//	case WM_DESTROY:
//		PostQuitMessage(0);
//		return 0;
//	case WM_PAINT:
//		return 0;
//	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void handleFatalError(HWND hwnd, char *message)
{
	MessageBox(
		hwnd,
		utf8ToUtf16(message),
		L"Modeler Error",
		MB_OK | MB_DEFBUTTON1 | MB_ICONERROR | MB_SYSTEMMODAL
	);

	fprintf(stderr, "%s\n", message);
	exit(1);
}

wchar_t *utf8ToUtf16(const char *utf8)
{
	int outputSize = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t *output = (wchar_t *) malloc(outputSize * sizeof(wchar_t));

	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, output, outputSize);
 
	return output;
}