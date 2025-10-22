#include <stdlib.h>

#include "utils.h"
#include "vulkan_utils.h"

#include "synchronization.h"

bool createSynchronization(VkDevice device, uint32_t swapchainImageCount, SynchronizationInfo *synchronizationInfo, char **error)
{
	synchronizationInfo->imageAvailableSemaphores = malloc(sizeof(*synchronizationInfo->imageAvailableSemaphores) * MAX_FRAMES_IN_FLIGHT);
	synchronizationInfo->renderFinishedSemaphores = malloc(sizeof(*synchronizationInfo->renderFinishedSemaphores) * swapchainImageCount);
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

	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		VkResult result;

		if ((result = vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, synchronizationInfo->renderFinishedSemaphores + i)) != VK_SUCCESS) {
			asprintf(error, "Failed to create semaphore: %s", string_VkResult(result));
			return false;
		}
	}

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkResult result;

		if ((result = vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, synchronizationInfo->imageAvailableSemaphores + i)) != VK_SUCCESS) {
			asprintf(error, "Failed to create semaphore: %s", string_VkResult(result));
			return false;
		}
		if ((result = vkCreateFence(device, &fenceCreateInfo, NULL, synchronizationInfo->frameInFlightFences + i)) != VK_SUCCESS) {
			asprintf(error, "Failed to create fence: %s", string_VkResult(result));
			return false;
		}
	}

	return true;
}

void destroySynchronization(VkDevice device, uint32_t swapchainImageCount, SynchronizationInfo synchronizationInfo)
{
	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		vkDestroySemaphore(device, synchronizationInfo.renderFinishedSemaphores[i], NULL);
	}

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, synchronizationInfo.imageAvailableSemaphores[i], NULL);
		vkDestroyFence(device, synchronizationInfo.frameInFlightFences[i], NULL);
	}

	free(synchronizationInfo.imageAvailableSemaphores);
	free(synchronizationInfo.renderFinishedSemaphores);
	free(synchronizationInfo.frameInFlightFences);
}