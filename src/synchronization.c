#include <stdlib.h>

#include "utils.h"
#include "vulkan_utils.h"

#include "synchronization.h"

bool createSynchronization(VkDevice device, SwapchainInfo swapchainInfo, SynchronizationInfo *synchronizationInfo, char **error)
{
	synchronizationInfo->imageAvailableSemaphores = malloc(sizeof(*synchronizationInfo->imageAvailableSemaphores) * MAX_FRAMES_IN_FLIGHT);
	synchronizationInfo->renderFinishedSemaphores = malloc(sizeof(*synchronizationInfo->renderFinishedSemaphores) * MAX_FRAMES_IN_FLIGHT);
	synchronizationInfo->frameInFlightFences = malloc(sizeof(*synchronizationInfo->frameInFlightFences) * MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

	VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, synchronizationInfo->imageAvailableSemaphores + i);
		vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, synchronizationInfo->renderFinishedSemaphores + i);
		vkCreateFence(device, &fenceCreateInfo, NULL, synchronizationInfo->frameInFlightFences + i);
	}

	return true;
}

void destroySynchronization(VkDevice device, SynchronizationInfo synchronizationInfo)
{
}