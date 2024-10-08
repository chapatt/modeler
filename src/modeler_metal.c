#include "objc/message.h"
#include "objc/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#include "modeler.h"
#include "modeler_metal.h"
#include "queue.h"

#include "renderloop.h"

static void sendNSNotification(char *message);

pthread_t initVulkanMetal(void *surfaceLayer, int width, int height, float scale, const char *resourcePath, Queue *inputQueue, char **error)
{
	pthread_t thread;
	struct threadArguments *threadArgs = malloc(sizeof(*threadArgs));
	MetalWindow *window = malloc(sizeof(MetalWindow));
	window->surfaceLayer = surfaceLayer;
	threadArgs->platformWindow = window;
	asprintf(&threadArgs->resourcePath, "%s", resourcePath);
	threadArgs->inputQueue = inputQueue;
	char *instanceExtensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_EXT_METAL_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	};
	threadArgs->instanceExtensionCount = 4;
	threadArgs->instanceExtensions = malloc(sizeof(*threadArgs->instanceExtensions) * threadArgs->instanceExtensionCount);
	for (size_t i = 0; i < threadArgs->instanceExtensionCount; ++i) {
	    threadArgs->instanceExtensions[i] = instanceExtensions[i];
	}
	threadArgs->windowDimensions = (WindowDimensions) {
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
		.scale = scale
	};
	threadArgs->error = error;

	if (pthread_create(&thread, NULL, threadProc, (void *) threadArgs) != 0) {
		free(threadArgs->resourcePath);
		free(threadArgs);
		asprintf(error, "Failed to start Vulkan thread");
		return 0;
	}

	return thread;
}

void sendThreadFailureSignal(void *platformWindow)
{
	sendNSNotification(THREAD_FAILURE_NOTIFICATION_NAME);
	pthread_exit(NULL);
}

void sendFullscreenSignal(void *platformWindow)
{
	sendNSNotification(FULLSCREEN_NOTIFICATION_NAME);
}

void sendExitFullscreenSignal(void *platformWindow)
{
	sendNSNotification(EXIT_FULLSCREEN_NOTIFICATION_NAME);
}

void enqueueResizeEvent(Queue *queue, WindowDimensions windowDimensions, void *surfaceLayer)
{
	MetalWindow *window = malloc(sizeof(*window));
	*window = (MetalWindow) {
		.surfaceLayer = surfaceLayer
	};
	ResizeInfo *resizeInfo = malloc(sizeof(*resizeInfo));
	*resizeInfo = (ResizeInfo) {
		.windowDimensions = windowDimensions,
		.platformWindow = window
	};
	enqueueInputEvent(queue, RESIZE, resizeInfo);
}

static void sendNSNotification(char *message)
{
	id (*postNotification)(id, SEL, id) = (id (*)(id, SEL, id)) objc_msgSend;
	id (*notificationWithNameObject)(Class, SEL, id, id) = (id (*)(Class, SEL, id, id)) objc_msgSend;
	id (*stringWithUTF8String)(Class, SEL, char *) = (id (*)(Class, SEL, char *)) objc_msgSend;
	id (*defaultCenter)(Class, SEL) = (id (*)(Class, SEL)) objc_msgSend;

	Class NSStringClass = objc_getClass("NSString");
	SEL stringWithUTF8StringSelector = sel_registerName("stringWithUTF8String:");
	id name = stringWithUTF8String(NSStringClass, stringWithUTF8StringSelector, message);

	Class NSNotificationClass = objc_getClass("NSNotification");
	SEL notifcationWithNameObjectSelector = sel_registerName("notificationWithName:object:");
	id notification = notificationWithNameObject(NSNotificationClass, notifcationWithNameObjectSelector, name, NULL);

	Class NSNotificationCenterClass = objc_getClass("NSNotificationCenter");
	SEL defaultCenterSelector = sel_registerName("defaultCenter");
	id notificationCenter = defaultCenter(NSNotificationCenterClass, defaultCenterSelector);

	SEL postNotificationSelector = sel_registerName("postNotification:");
	postNotification(notificationCenter, postNotificationSelector, notification);
}

void ackResize(ResizeInfo *resizeInfo)
{
	/* Unnecessary on Metal/Cocoa */
}
