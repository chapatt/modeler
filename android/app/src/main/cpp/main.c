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

static int handleCustomLooperEvent(int fd, int events, void *data);
static void handleFatalError(char *message);
static void handle_cmd(struct android_app *pApp, int32_t cmd);

static void handle_cmd(struct android_app *pApp, int32_t cmd)
{
	switch (cmd) {
	case APP_CMD_INIT_WINDOW:
		__android_log_print(ANDROID_LOG_DEBUG, "MODELER_LIFECYCLE", "APP_CMD_INIT_WINDOW\n");
		{
			struct ANativeWindow *window = (struct ANativeWindow *) (pApp->window);
			ModelerUserData *userData = malloc(sizeof(*userData));

			initializeQueue(&userData->inputQueue);

			if (pipe(userData->threadPipe)) {
				handleFatalError("Failed to create pipe");
			}

			ALooper_addFd(
				ALooper_forThread(),
				userData->threadPipe[0],
				ALOOPER_POLL_CALLBACK,
				ALOOPER_EVENT_INPUT,
				&handleCustomLooperEvent,
				userData
			);

			if (!(userData->thread = initVulkanAndroid(window, &userData->inputQueue, userData->threadPipe[1], &userData->error))) {
				break;
			}

			pApp->userData = userData;
		}
		break;
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

/*!
 * Enable the motion events you want to handle; not handled events are
 * passed back to OS for further processing. For this example case,
 * only pointer and joystick devices are enabled.
 *
 * @param motionEvent the newly arrived GameActivityMotionEvent.
 * @return true if the event is from a pointer or joystick device,
 *         false for all other input devices.
 */
bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent)
{
	int32_t sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;

	return (sourceClass == AINPUT_SOURCE_CLASS_POINTER);
}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp)
{
	pApp->onAppCmd = handle_cmd;

	android_app_set_motion_event_filter(pApp, motion_event_filter_func);

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
				GameActivityMotionEvent* motionEvent = &inputBuffer->motionEvents[i];

				if (motionEvent->pointerCount > 0) {
					switch (motionEvent->action & AMOTION_EVENT_ACTION_MASK) {
					case AMOTION_EVENT_ACTION_DOWN:
						if (userData) {
							enqueueInputEventWithPosition(&userData->inputQueue, POINTER_MOVE, floor(motionEvent->pointers[0].rawX), floor(motionEvent->pointers[0].rawY));
							enqueueInputEvent(&userData->inputQueue, BUTTON_DOWN, NULL);
						}
						break;
					case AMOTION_EVENT_ACTION_UP:
						if (userData) {
							enqueueInputEventWithPosition(&userData->inputQueue, POINTER_MOVE, floor(motionEvent->pointers[0].rawX), floor(motionEvent->pointers[0].rawY));
							enqueueInputEvent(&userData->inputQueue, BUTTON_UP, NULL);
						}
						break;
					case AMOTION_EVENT_ACTION_MOVE: {
						if (userData) {
							enqueueInputEventWithPosition(&userData->inputQueue, POINTER_MOVE, floor(motionEvent->pointers[0].rawX), floor(motionEvent->pointers[0].rawY));
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