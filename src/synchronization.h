#ifndef MODELER_SYNCHRONIZATION_H
#define MODELER_SYNCHRONIZATION_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "device.h"
#include "swapchain.h"

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct synchronization_info_t {
	VkSemaphore *imageAvailableSemaphores;
	VkSemaphore *renderFinishedSemaphores;
	VkFence *frameInFlightFences;
} SynchronizationInfo;

bool createSynchronization(VkDevice device, SynchronizationInfo *synchronizationInfo, char **error);

void destroySynchronization(VkDevice device, SynchronizationInfo synchronizationInfo);

#endif /* MODELER_SYNCHRONIZATION_H */
