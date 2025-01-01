#include <stdio.h>
#include <jni.h>

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "../../../../../src/modeler_android.h"

typedef struct modeler_user_data_t {
    Queue *inputQueue;
    pthread_t thread;
} ModelerUserData;

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd(struct android_app *pApp, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // A new window is created, associate a renderer with it. You may replace this with a
            // "game" class if that suits your needs. Remember to change all instances of userData
            // if you change the class here as a reinterpret_cast is dangerous this in the
            // android_main function and the APP_CMD_TERM_WINDOW handler case.
            char *error;
            ModelerUserData *userData = (ModelerUserData *) (pApp->userData);
            if (!(userData->thread = initVulkanAndroid(nativeWindow, &inputQueue, &error))) {
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
bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    int32_t sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
    // Can be removed, useful to ensure your code is running
    printf("Welcome to android_main");

    Queue inputQueue;
    initializeQueue(&inputQueue);
    ModelerUserData userData = {
            .inputQueue = inputQueue,
            .thread = NULL
    };
    (ModelerUserData *) pApp->userData = &userData;

    // Register an event handler for Android events
    pApp->onAppCmd = handle_cmd;

    // Set input event filters (set it to NULL if the app wants to process all inputs).
    // Note that for key inputs, this example uses the default default_key_filter()
    // implemented in android_native_app_glue.c.
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    // This sets up a typical game/event loop. It will run until the app is destroyed.
    do {
        // Process all pending events before running game logic.
        bool done = false;
        while (!done) {
            // 0 is non-blocking.
            int timeout = 0;
            int events;
            struct android_poll_source *pSource;
            int result = ALooper_pollOnce(timeout, NULL, &events,
                    (void **) (&pSource));
            switch (result) {
                case ALOOPER_POLL_TIMEOUT: case ALOOPER_POLL_WAKE:
                    // No events occurred before the timeout or explicit wake. Stop checking for events.
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

        // Check if any user data is associated. This is assigned in handle_cmd
        if ((ModelerUserData *) (pApp->userData)->thread) {
        }
    } while (!pApp->destroyRequested);
}