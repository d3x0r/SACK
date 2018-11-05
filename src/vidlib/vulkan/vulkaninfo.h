#ifndef __VULKAN_INFO_DEFINTIIONS_INCLUDED__
#define __VULKAN_INFO_DEFINTIIONS_INCLUDED__

#ifdef _WIN32 
#  define VK_USE_PLATFORM_WIN32_KHR
#else
//#  define VK_USE_PLATFORM_XLIB_KHR
#  define VK_USE_PLATFORM_XCB_KHR
#endif

#ifndef VULKAN_H_
#  include <vulkan/vulkan.h>
#endif

struct SwapChainBuffer {
	VkImage image;
	VkImageView view;
};

struct SwapChain {
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
	VkFormat colorFormat;
	VkColorSpaceKHR colorSpace;
	VkSwapchainKHR swapChain;
	VkImage *images; // VkImage list
	int nImages;
	struct SwapChainBuffer *buffers;//SwapChainBuffer List
	size_t nodeIndex;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
	PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
	PFN_vkQueuePresentKHR fpQueuePresentKHR;
};

struct vulkan_local {
	VkApplicationInfo applicationInfo;
	VkInstanceCreateInfo instanceInfo;
	VkInstance instance;
	VkPhysicalDevice *physicalDevices;

#if defined(_WIN32)
	int a;
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
#elif defined(__ANDROID__)
	VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo;
#else
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo;
#endif

	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;

};

#endif