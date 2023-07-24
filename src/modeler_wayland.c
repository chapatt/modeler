#include "modeler_wayland.h"
#include "instance.h"
#include "surface.h"
#include "surface_wayland.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"

#include "renderloop.h"

bool initVulkanWayland(struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, char **error)
{
        VkExtent2D windowExtent = {
                .width = 600,
                .height = 400
        };

	const char *instanceExtensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
	};
	VkInstance instance;
	if (!createInstance(instanceExtensions, 3, &instance, error)) {
		return false;
	}

	VkSurfaceKHR surface;
	if (!createSurfaceWayland(instance, waylandDisplay, waylandSurface, &surface, error)) {
		return false;
	}

        VkPhysicalDevice physicalDevice;
        PhysicalDeviceCharacteristics characteristics;
        PhysicalDeviceSurfaceCharacteristics surfaceCharacteristics;
        if (!choosePhysicalDevice(instance, surface, &physicalDevice, &characteristics, &surfaceCharacteristics, error)) {
                return false;
        }

        VkDevice device;
        QueueInfo queueInfo = {};
        if (!createDevice(physicalDevice, surface, characteristics, surfaceCharacteristics, &device, &queueInfo, error)) {
                return false;
        }

        VkSwapchainKHR swapchain;
        if (!createSwapchain(device, surface, surfaceCharacteristics, queueInfo.graphicsQueueFamilyIndex, queueInfo.presentationQueueFamilyIndex, windowExtent, &swapchain, error)) {
                return false;
        }

        draw(device, swapchain, windowExtent, queueInfo.graphicsQueue, queueInfo.presentationQueue, queueInfo.graphicsQueueFamilyIndex, ".");

        return true;
}
