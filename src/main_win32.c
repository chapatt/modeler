#ifndef UNICODE
#define UNICODE
#endif 

#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "modeler.h"
#include "queue.h"
#include "input_event.h"

#include "modeler_win32.h"

#define CHROME_HEIGHT 50
#define IDM_MENU 3

const LPCTSTR CLASS_NAME = L"Modeler Window Class";
void handleFatalError(HWND hWnd, char *message);
static wchar_t *utf8ToUtf16(const char *utf8);
static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT calcSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT hitTest(HWND hWnd, int x, int y);

static char *error = NULL;
static pthread_t thread = 0;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	#ifdef DEBUG
	if (AllocConsole()) {
		FILE* fi = 0;
		freopen_s(&fi, "CONOUT$", "w", stdout);
	}
	#endif /* DEBUG */

	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = windowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = CLASS_NAME;
	wc.hIconSm = NULL;

	if (!RegisterClassEx(&wc)) {
		handleFatalError(NULL, "Can't register Windows window class");
	}

	HWND hWnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		CLASS_NAME,
		L"Modeler",
		WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
		500, 500, 500, 300,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd) {
		handleFatalError(NULL, "Can't create window.");
	}

	if (ShowWindow(hWnd, nCmdShow) != 0) {
		handleFatalError(NULL, "Can't show window.");
	}

	Queue inputQueue;
	initializeQueue(&inputQueue);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) &inputQueue);
	if (!(thread = initVulkanWin32(hInstance, hWnd, &inputQueue, &error))) {
		handleFatalError(hWnd, error);
	}

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Queue *inputQueue = (Queue *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch (uMsg) {
	case THREAD_FAILURE_NOTIFICATION_MESSAGE:
		handleFatalError(hWnd, error);
	case WM_CLOSE:
		terminateVulkan(inputQueue, thread);
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		if (inputQueue) {
			uint32_t width = LOWORD(lParam);
			uint32_t height = HIWORD(lParam);
			WindowDimensions windowDimensions = {
				.surfaceArea = {
					.width = width,
					.height = height
				},
				.activeArea = {
					.extent = {
						.width = width,
						.height = height
					},
					.offset = {0, 0}
				},
				.cornerRadius = 0
			};
			enqueueInputEventWithWindowDimensions(inputQueue, RESIZE, windowDimensions);
		}
		return 0;
	case WM_LBUTTONDOWN:
		enqueueInputEvent(inputQueue, BUTTON_DOWN, NULL);
		return 0;
	case WM_LBUTTONUP:
		enqueueInputEvent(inputQueue, BUTTON_UP, NULL);
		return 0;
	case WM_MOUSEMOVE:
		TRACKMOUSEEVENT trackMouseEvent = {
			.cbSize = sizeof(TRACKMOUSEEVENT),
			.dwFlags = TME_LEAVE,
			.hwndTrack = hWnd
		};
		TrackMouseEvent(&trackMouseEvent);

		enqueueInputEventWithPosition(inputQueue, POINTER_MOVE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSELEAVE:
		enqueueInputEvent(inputQueue, POINTER_LEAVE, NULL);
		return 0;
	case WM_NCCALCSIZE:
		return calcSize(hWnd, uMsg, wParam, lParam);
	case WM_NCHITTEST:
		return hitTest(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

static LRESULT calcSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!wParam) return DefWindowProc(hWnd, uMsg, wParam, lParam);

	int padding = GetSystemMetrics(SM_CXPADDEDBORDER);
	int expandX = GetSystemMetrics(SM_CXFRAME) - GetSystemMetrics(SM_CXBORDER) + padding;
	int expandY = GetSystemMetrics(SM_CYFRAME) - GetSystemMetrics(SM_CYBORDER) + padding;

	NCCALCSIZE_PARAMS *params = (NCCALCSIZE_PARAMS *) lParam;
	RECT *clientRect = params->rgrc;

	clientRect->right -= expandX;
	clientRect->left += expandX;
	clientRect->bottom -= expandY;

	bool isMaximized = false;
	if (isMaximized) {
		clientRect->top += padding;
	}

	return 0;
}

static LRESULT hitTest(HWND hWnd, int x, int y)
{
	enum regionMask
	{
		CLIENT = 0b00000,
		LEFT = 0b00001,
		RIGHT = 0b00010,
		TOP = 0b00100,
		BOTTOM = 0b01000,
		CHROME = 0b10000
	};

	int padding = GetSystemMetrics(SM_CXPADDEDBORDER);
	int borderX = GetSystemMetrics(SM_CXFRAME) + padding;
	int borderY = GetSystemMetrics(SM_CYFRAME) + padding;

	RECT windowRect;
	if (!GetWindowRect(hWnd, &windowRect)) {
		return HTNOWHERE;
	}

	enum regionMask result = CLIENT;

	if (x < (windowRect.left + borderX)) {
		result |= LEFT;
	}

	if (x >= (windowRect.right - borderX)) {
		result |= RIGHT;
	}

	if (y < (windowRect.top + borderY)) {
		result |= TOP;
	}

	if (y >= (windowRect.bottom - borderY)) {
		result |= BOTTOM;
	}

	if (!(result & (LEFT | RIGHT | TOP | BOTTOM)) && y < (windowRect.top + CHROME_HEIGHT)) {
		result |= CHROME;
	}

	if (result & TOP)
	{
		if (result & LEFT) return HTTOPLEFT;
		if (result & RIGHT) return HTTOPRIGHT;
		return HTTOP;
	} else if (result & BOTTOM) {
		if (result & LEFT) return HTBOTTOMLEFT;
		if (result & RIGHT) return HTBOTTOMRIGHT;
		return HTBOTTOM;
	} else if (result & LEFT) {
		return HTLEFT;
	} else if (result & RIGHT) {
		return HTRIGHT;
	} else if (result & CHROME) {
		return HTCAPTION;
	} else {
		return HTCLIENT;
	}
}

void handleFatalError(HWND hWnd, char *message)
{
	wchar_t *messageW = utf8ToUtf16(message);

	MessageBox(
		hWnd,
		messageW,
		L"Modeler Error",
		MB_OK | MB_DEFBUTTON1 | MB_ICONERROR | MB_SYSTEMMODAL
	);

	free(messageW);

	fprintf(stderr, "%s\n", message);
	exit(1);
}

static wchar_t *utf8ToUtf16(const char *utf8)
{
	int outputSize = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t *output = malloc(outputSize * sizeof(wchar_t));

	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, output, outputSize);
 
	return output;
}