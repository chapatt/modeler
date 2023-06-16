#ifndef UNICODE
#define UNICODE
#endif 

#include <stdio.h>
#include <windows.h>
#include "modeler_win32.h"

const LPCTSTR CLASS_NAME = L"Modeler Window Class";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
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
		fprintf(stderr, "Can't register Windows window class\n");
		exit(1);
	}

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Modeler",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,  
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        fprintf(stderr, "Can't create Windows window");
		exit(1);
    }

    ShowWindow(hwnd, nCmdShow);

    initVulkanWin32(hInstance, hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
	case WM_CREATE:
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}