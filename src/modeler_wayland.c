#include "modeler_wayland.h"
#include "instance.h"
#include "surface_wayland.h"
#include "physical_device.h"
#include "device.h"

bool initVulkanWayland(struct wl_display *waylandDisplay, struct wl_surface *waylandSurface, char **error)
{
        const char *instanceExtensions[] = {
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
        };
        VkInstance instance;
        if (!createInstance(instanceExtensions, 3, &instance, error)) {
                return false;
        }
        VkSurfaceKHR surface = createSurfaceWayland(instance, waylandDisplay, waylandSurface);
        VkPhysicalDevice physicalDevice = choosePhysicalDevice(instance, surface);
        VkDevice device = createDevice(physicalDevice);

        return true;
}
