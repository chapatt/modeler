#include <android/log.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <jni.h>

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "modeler_android.h"
#include "utils.h"

typedef struct modeler_user_data_t {
	Queue *inputQueue;
	pthread_t thread;
	int *threadPipe;
	char **error;
} ModelerUserData;

static int handleCustomLooperEvent(int fd, int events, void *data);
static void handleFatalError(char *message);
static void handle_cmd(struct android_app *pApp, int32_t cmd);

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
static void handle_cmd(struct android_app *pApp, int32_t cmd)
{
	ModelerUserData *userData = (ModelerUserData *)(pApp->userData);
	struct ANativeWindow *window = NULL;

	switch (cmd) {
	case APP_CMD_INIT_WINDOW:
		window = (struct ANativeWindow *)(pApp->window);
		if (!(userData->thread = initVulkanAndroid(window, userData->inputQueue, userData->threadPipe[1], userData->error))) {
			break;
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// The window is being destroyed. Use this to clean up your userData to avoid leaking
		// resources.
		//
		// We have to check if userData is assigned just in case this comes in really quickly
		if (pApp->userData) {
			pApp->userData = NULL;
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
	return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
		sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp)
{
	char *error;
	Queue inputQueue;
	initializeQueue(&inputQueue);
	int threadPipe[2];
	if (pipe(threadPipe)) {
		handleFatalError("Failed to create pipe");
	}
	ModelerUserData userData = {
		.inputQueue = &inputQueue,
		.threadPipe = threadPipe,
		.error = &error};
	pApp->userData = &userData;

	pApp->onAppCmd = handle_cmd;

	// Set input event filters (set it to NULL if the app wants to process all inputs).
	// Note that for key inputs, this example uses the default default_key_filter()
	// implemented in android_native_app_glue.c.
	android_app_set_motion_event_filter(pApp, motion_event_filter_func);

	ALooper_addFd(
		ALooper_forThread(),
		threadPipe[0],
		ALOOPER_POLL_CALLBACK,
		ALOOPER_EVENT_INPUT,
		&handleCustomLooperEvent,
		&userData
	);

	// This sets up a typical game/event loop. It will run until the app is destroyed.
	do {
		// Process all pending events before running game logic.
		bool done = false;
		while (!done) {
			// 0 is non-blocking.
			int timeout = 0;
			int events;
			struct android_poll_source *pSource;
			int result = ALooper_pollOnce(timeout, NULL, &events, (void **) (&pSource));
			switch (result) {
			case ALOOPER_POLL_TIMEOUT:
			case ALOOPER_POLL_WAKE:
				// No events occurred before the timeout or explicit wake. Stop checking for events.
				done = true;
				break;
			case ALOOPER_EVENT_ERROR:
				printf("ALooper_pollOnce returned an error");
				break;
			case ALOOPER_POLL_CALLBACK:
				break;
			default:
				if (pSource)
				{
					pSource->process(pApp, pSource);
				}
			}
		}
	} while (!pApp->destroyRequested);
}

static int handleCustomLooperEvent(int fd, int events, void *data)
{
	ModelerUserData *userData = (ModelerUserData *) data;
	char c;
	read(fd, &c, 1);
	close(fd);
	handleFatalError(*userData->error);

	return 1;
}

static void handleFatalError(char *message)
{
	fprintf(stderr, "%s\n", message);
	__android_log_print(ANDROID_LOG_DEBUG, "MODELER_ERROR", "%s\n", message);
}