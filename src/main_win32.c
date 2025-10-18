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
#include "utils.h"
#include "window.h"

#include "modeler_win32.h"

#define IDM_MENU 3

const LPCTSTR CLASS_NAME = L"Modeler Window Class";
void handleFatalError(HWND hWnd, char *message);
static wchar_t *utf8ToUtf16(const char *utf8);
static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT calcSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT hitTest(HWND hWnd, int x, int y);
static bool isMaximized(HWND hWnd);
static void enqueueResizeEvent(Queue *queue, WindowDimensions windowDimensions, HINSTANCE hInstance, HWND hWnd);
static void setFullscreen(HWND hWnd);
static void exitFullscreen(HWND hWnd);

static char *error = NULL;
static pthread_t thread = 0;
static DWORD initialDwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
static WINDOWPLACEMENT lastWindowPlacement = {sizeof(lastWindowPlacement)};
static bool isFullscreen = false;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT value);
	PFN_SetProcessDpiAwarenessContext _SetProcessDpiAwarenessContext = (PFN_SetProcessDpiAwarenessContext) GetProcAddress(GetModuleHandle(L"User32.dll"), "SetProcessDpiAwarenessContext");

	#ifdef DEBUG
	if (AllocConsole()) {
		FILE* fi = 0;
		freopen_s(&fi, "CONOUT$", "w", stdout);
	}
	#endif /* DEBUG */

	if (!_SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
		handleFatalError(NULL, "Failed to advertise DPI awareness");
	}

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
		initialDwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	initialDwStyle = GetWindowLong(hWnd, GWL_STYLE);

	if (!hWnd) {
		handleFatalError(NULL, "Can't create window.");
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
	case CLOSE_NOTIFICATION_MESSAGE:
		exit(0);
		return 0;
	case MAXIMIZE_NOTIFICATION_MESSAGE:
		if (isMaximized(hWnd)) {
			ShowWindow(hWnd, SW_RESTORE);
		} else {
			ShowWindow(hWnd, SW_MAXIMIZE);
		}
		return 0;
	case MINIMIZE_NOTIFICATION_MESSAGE:
		ShowWindow(hWnd, SW_MINIMIZE);
		return 0;
	case FULLSCREEN_NOTIFICATION_MESSAGE:
		setFullscreen(hWnd);
		return 0;
	case EXIT_FULLSCREEN_NOTIFICATION_MESSAGE:
		exitFullscreen(hWnd);
		return 0;
	case WM_CLOSE:
		terminateVulkan(inputQueue, thread);
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		return 0;
	case WM_COMMAND:
		SendMessage(hWnd, WM_SYSCOMMAND, wParam, lParam);
 		return 0;
	case WM_NCRBUTTONUP:
		if (wParam == HTCAPTION) {
			HMENU hMenu = GetSystemMenu(hWnd, false);
			TrackPopupMenu(
				hMenu,
				TPM_RIGHTBUTTON,
				GET_X_LPARAM(lParam),
				GET_Y_LPARAM(lParam),
				0,
				hWnd,
				NULL
			);
		}
		return 0;
	case WM_SIZE:
		if (inputQueue) {
			HINSTANCE hInstance = GetModuleHandle(NULL);
			Win32Window window = {
				.hInstance = hInstance,
				.hWnd = hWnd
			};
			float scale = getWindowScale(&window);
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
				.cornerRadius = 0,
				.scale = scale,
				.titlebarHeight = CHROME_HEIGHT * scale,
				.fullscreen = isFullscreen,
				.orientation = ROTATE_0
			};
			enqueueResizeEvent(inputQueue, windowDimensions, hInstance, hWnd);
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
	case WM_DPICHANGED:
		RECT *newWindowRect = (RECT *) lParam;

		SetWindowPos(
			hWnd,
			NULL,
			newWindowRect->left,
			newWindowRect->top,
			newWindowRect->right - newWindowRect->left,
			newWindowRect->bottom - newWindowRect->top,
			0
		);

		if (inputQueue) {
			HINSTANCE hInstance = GetModuleHandle(NULL);
			Win32Window window = {
				.hInstance = hInstance,
				.hWnd = hWnd
			};
			float scale = getWindowScale(&window);
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
				.cornerRadius = 0,
				.scale = scale,
				.titlebarHeight = CHROME_HEIGHT * scale,
				.fullscreen = isFullscreen,
				.orientation = ROTATE_0
			};
			enqueueResizeEvent(inputQueue, windowDimensions, hInstance, hWnd);
		}

		return 0;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

static bool isMaximized(HWND hWnd)
{
	WINDOWPLACEMENT windowPlacement = {.length = sizeof(windowPlacement)};
	if (!GetWindowPlacement(hWnd, &windowPlacement)) {
		return -1;
	}
	return windowPlacement.showCmd == SW_MAXIMIZE;
}

static LRESULT calcSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (isMaximized(hWnd) || isFullscreen) {
		return 0;
	}

	typedef UINT (WINAPI *PFN_GetDpiForWindow)(HWND hwnd);
	PFN_GetDpiForWindow _GetDpiForWindow = (PFN_GetDpiForWindow) GetProcAddress(GetModuleHandle(L"User32.dll"), "GetDpiForWindow");

	typedef int (WINAPI *PFN_GetSystemMetricsForDpi)(int nIndex, UINT dpi);
	PFN_GetSystemMetricsForDpi _GetSystemMetricsForDpi = (PFN_GetSystemMetricsForDpi) GetProcAddress(GetModuleHandle(L"User32.dll"), "GetSystemMetricsForDpi");

	int dpi = _GetDpiForWindow(hWnd);
	int padding = _GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
	int expandX = _GetSystemMetricsForDpi(SM_CXFRAME, dpi) - _GetSystemMetricsForDpi(SM_CXBORDER, dpi) + padding;
	int expandY = _GetSystemMetricsForDpi(SM_CYFRAME, dpi) - _GetSystemMetricsForDpi(SM_CYBORDER, dpi) + padding;

	RECT *clientRect;
	if (!wParam) {
		clientRect = (RECT *)lParam;
	} else {
		NCCALCSIZE_PARAMS *params = (NCCALCSIZE_PARAMS *) lParam;
		clientRect = params->rgrc;
	}


	clientRect->right -= expandX;
	clientRect->left += expandX;
	clientRect->bottom -= expandY;

	return 0;
}

static LRESULT hitTest(HWND hWnd, int x, int y)
{
	typedef UINT (WINAPI *PFN_GetDpiForWindow)(HWND hwnd);
	PFN_GetDpiForWindow _GetDpiForWindow = (PFN_GetDpiForWindow) GetProcAddress(GetModuleHandle(L"User32.dll"), "GetDpiForWindow");

	typedef int (WINAPI *PFN_GetSystemMetricsForDpi)(int nIndex, UINT dpi);
	PFN_GetSystemMetricsForDpi _GetSystemMetricsForDpi = (PFN_GetSystemMetricsForDpi) GetProcAddress(GetModuleHandle(L"User32.dll"), "GetSystemMetricsForDpi");

	RECT windowRect;
	if (!GetWindowRect(hWnd, &windowRect)) {
		return HTNOWHERE;
	}

	if (isFullscreen) {
		return HTCLIENT;
	}

	if (isMaximized(hWnd)) {
		if (y < (windowRect.top + CHROME_HEIGHT) && x < (windowRect.right - CHROME_HEIGHT * 3)) {
			return HTCAPTION;
		}

		return HTCLIENT;
	}

	enum regionMask
	{
		CLIENT = 0b00000,
		LEFT = 0b00001,
		RIGHT = 0b00010,
		TOP = 0b00100,
		BOTTOM = 0b01000,
		CHROME = 0b10000
	};

	int dpi = _GetDpiForWindow(hWnd);
	int padding = _GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
	int borderX = _GetSystemMetricsForDpi(SM_CXFRAME, dpi) + padding;
	int borderY = _GetSystemMetricsForDpi(SM_CYFRAME, dpi) + padding;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	Win32Window window = {
		.hInstance = hInstance,
		.hWnd = hWnd
	};
	float scale = getWindowScale(&window);

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

	if (!(result & (LEFT | RIGHT | TOP | BOTTOM)) && y < (windowRect.top + CHROME_HEIGHT * scale)) {
		if (x < (windowRect.right - CHROME_HEIGHT * scale * 3)) {
			result |= CHROME;
		}
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

static char *getLastErrorString(void) {
	//LPWSTR messageBuffer = NULL;
	//FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) &messageBuffer, 0, NULL);
	//LocalFree(messageBuffer);
}

static void enqueueResizeEvent(Queue *queue, WindowDimensions windowDimensions, HINSTANCE hInstance, HWND hWnd)
{
	Win32Window *window = malloc(sizeof(*window));
	*window = (Win32Window) {
		.hInstance = hInstance,
		.hWnd = hWnd
	};
	ResizeInfo *resizeInfo = malloc(sizeof(*resizeInfo));
	*resizeInfo = (ResizeInfo) {
		.windowDimensions = windowDimensions,
		.platformWindow = window
	};
	enqueueInputEvent(queue, RESIZE, resizeInfo);
}

static void setFullscreen(HWND hWnd)
{
	isFullscreen = true;
	MONITORINFO mi = {sizeof(mi)};
	if (GetWindowPlacement(hWnd, &lastWindowPlacement) && GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
		SetWindowLong(hWnd, GWL_STYLE, initialDwStyle & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos(hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

static void exitFullscreen(HWND hWnd)
{
	isFullscreen = false;
	SetWindowLong(hWnd, GWL_STYLE, initialDwStyle);
	SetWindowPlacement(hWnd, &lastWindowPlacement);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}
