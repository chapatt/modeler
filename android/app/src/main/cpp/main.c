#include <android/log.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <jni.h>

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "modeler.h"
#include "modeler_android.h"
#include "utils.h"

typedef struct modeler_user_data_t {
	Queue inputQueue;
	pthread_t thread;
	int threadPipe[2];
	char *error;
} ModelerUserData;

static void handleAppCmd(struct android_app *pApp, int32_t cmd);
static bool filterMotionEvents(const GameActivityMotionEvent *motionEvent);
static int handleCustomLooperEvent(int fd, int events, void *data);
static void handleFatalError(char *message);
static void enqueueResizeEvent(Queue *queue, WindowDimensions windowDimensions, struct ANativeWindow *nativeWindow);
static void enqueueInsetChangeEvent(Queue *queue, Insets insets);

void android_main(struct android_app *pApp)
{
	pApp->onAppCmd = handleAppCmd;

	android_app_set_motion_event_filter(pApp, filterMotionEvents);

	do {
		ModelerUserData *userData = (ModelerUserData *) pApp->userData;

		bool done = false;
		while (!done) {
			int timeout = 0;
			int events;
			struct android_poll_source *pSource;
			int result = ALooper_pollOnce(timeout, NULL, &events, (void **) (&pSource));
			switch (result) {
			case ALOOPER_POLL_TIMEOUT:
			case ALOOPER_POLL_WAKE:
				done = true;
				break;
			case ALOOPER_EVENT_ERROR:
				printf("ALooper_pollOnce returned an error");
				break;
			case ALOOPER_POLL_CALLBACK:
				break;
			default:
				if (pSource) {
					pSource->process(pApp, pSource);
				}
			}
		}

		struct android_input_buffer *inputBuffer = android_app_swap_input_buffers(pApp);
		if (inputBuffer && inputBuffer->motionEventsCount) {
			for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i) {
				GameActivityMotionEvent *motionEvent = &inputBuffer->motionEvents[i];

				if (motionEvent->pointerCount > 0) {
					int x = floor(GameActivityPointerAxes_getAxisValue(motionEvent->pointers, 0));
					int y = floor(GameActivityPointerAxes_getAxisValue(motionEvent->pointers, 1));
					switch (motionEvent->action & AMOTION_EVENT_ACTION_MASK) {
					case AMOTION_EVENT_ACTION_DOWN:
						if (userData) {
							enqueueInputEventWithPosition(&userData->inputQueue, POINTER_MOVE, x, y);
							enqueueInputEvent(&userData->inputQueue, BUTTON_DOWN, NULL);
						}
						break;
					case AMOTION_EVENT_ACTION_UP:
						if (userData) {
							enqueueInputEventWithPosition(&userData->inputQueue, POINTER_MOVE, x, y);
							enqueueInputEvent(&userData->inputQueue, BUTTON_UP, NULL);
						}
						break;
					case AMOTION_EVENT_ACTION_MOVE: {
						if (userData) {
							enqueueInputEventWithPosition(&userData->inputQueue, POINTER_MOVE, x, y);
						}
						break;
					}
					default:
						break;
					}

				}
			}

			android_app_clear_motion_events(inputBuffer);
		}
	} while (!pApp->destroyRequested);
}

static void handleAppCmd(struct android_app *pApp, int32_t cmd)
{
	switch (cmd) {
	case APP_CMD_INIT_WINDOW:
		ModelerUserData *initialUserData = malloc(sizeof(*initialUserData));

		initializeQueue(&initialUserData->inputQueue);

		if (pipe(initialUserData->threadPipe)) {
			handleFatalError("Failed to create pipe");
		}

		ALooper_addFd(
			ALooper_forThread(),
			initialUserData->threadPipe[0],
			ALOOPER_POLL_CALLBACK,
			ALOOPER_EVENT_INPUT,
			&handleCustomLooperEvent,
			initialUserData
		);

		if (!(initialUserData->thread = initVulkanAndroid(pApp->window, pApp->activity, &initialUserData->inputQueue, initialUserData->threadPipe[1], &initialUserData->error))) {
			break;
		}

		pApp->userData = initialUserData;
		break;
	// case APP_CMD_WINDOW_INSETS_CHANGED:
	// 	ModelerUserData *userData = (ModelerUserData *) pApp->userData;
	// 	if (!userData) {
	// 		break;
	// 	}
	// 	ARect androidInsets;
	// 	GameActivity_getWindowInsets(pApp->activity, GAMECOMMON_INSETS_TYPE_SYSTEM_BARS, &androidInsets);
	// 	Insets insets = {
	// 		.top = androidInsets.top,
	// 		.right = androidInsets.right,
	// 		.bottom = androidInsets.bottom,
	// 		.left = androidInsets.left
	// 	};
	// 	enqueueInsetChangeEvent(&userData->inputQueue, insets);
	// 	break;
	// case APP_CMD_WINDOW_RESIZED:
	// 	ModelerUserData *userData = (ModelerUserData *) pApp->userData;
	// 	ARect insets;
	// 	GameActivity_getWindowInsets(pApp->activity, GAMECOMMON_INSETS_TYPE_SYSTEM_BARS, &insets);
	// 	__android_log_print(ANDROID_LOG_DEBUG, "MODELER_ERROR", "insets: %d, %d, %d, %d\n", insets.top, insets.right, insets.bottom, insets.left);
	// 	int width = ANativeWindow_getWidth(pApp->window);
	// 	int height = ANativeWindow_getHeight(pApp->window);
	// 	WindowDimensions windowDimensions = {
	// 		.surfaceArea = {
	// 			.width = width,
	// 			.height = height
	// 		},
	// 		.activeArea = {
	// 			.extent = {
	// 				.width = width - (insets.left + insets.right),
	// 				.height = height - (insets.top + insets.bottom)
	// 			},
	// 			.offset = {
	// 				.x = insets.left,
	// 				.y = insets.top
	// 			}
	// 		},
	// 		.cornerRadius = 0,
	// 		.scale = 1,
	// 		.titlebarHeight = 100,
	// 		.fullscreen = false,
	// 		.orientation = ROTATE_0
	// 	};
	// 	enqueueResizeEvent(&userData->inputQueue, windowDimensions, pApp->window);
	// 	break;
	case APP_CMD_START:
		break;
	case APP_CMD_RESUME:
		break;
	case APP_CMD_PAUSE:
		break;
	case APP_CMD_STOP:
		break;
	case APP_CMD_DESTROY:
		break;
	case APP_CMD_TERM_WINDOW:
		if (pApp->userData) {
			ModelerUserData *userData = (ModelerUserData *) pApp->userData;
			terminateVulkan(&userData->inputQueue, userData->thread);
			pApp->userData = NULL;
			free(userData);
		}
		break;
	default:
		break;
	}
}

static bool filterMotionEvents(const GameActivityMotionEvent *motionEvent)
{
	int32_t sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;

	return (sourceClass == AINPUT_SOURCE_CLASS_POINTER);
}

static int handleCustomLooperEvent(int fd, int events, void *data)
{
	ModelerUserData *userData = (ModelerUserData *) data;
	char c;
	read(fd, &c, 1);
	close(fd);
	handleFatalError(userData->error);

	return 1;
}

static void handleFatalError(char *message)
{
	fprintf(stderr, "%s\n", message);
	__android_log_print(ANDROID_LOG_DEBUG, "MODELER_ERROR", "%s\n", message);
}

static void enqueueResizeEvent(Queue *queue, WindowDimensions windowDimensions, struct ANativeWindow *nativeWindow)
{
	AndroidWindow *window = malloc(sizeof(*window));
	*window = (AndroidWindow) {
		.nativeWindow = nativeWindow
	};
	ResizeInfo *resizeInfo = malloc(sizeof(*resizeInfo));
	*resizeInfo = (ResizeInfo) {
		.windowDimensions = windowDimensions,
		.platformWindow = window
	};
	enqueueInputEvent(queue, RESIZE, resizeInfo);
}

static void enqueueInsetChangeEvent(Queue *queue, Insets insets)
{
	Insets *insetsPointer = malloc(sizeof(*insetsPointer));
	*insetsPointer = insets;
	enqueueInputEvent(queue, INSET_CHANGE, insetsPointer);
}

