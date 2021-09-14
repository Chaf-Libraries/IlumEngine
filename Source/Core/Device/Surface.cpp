#include "Surface.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>

namespace Ilum
{
Surface::Surface(const Instance &instance, const PhysicalDevice &physical_device, SDL_Window *window_handle) :
    m_instance(instance),
    m_physical_device(physical_device)
{
	// Create surface handle
	SDL_Vulkan_CreateSurface(window_handle, instance, &m_handle);

	// Get surface capabilities
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, m_handle, &m_capabilities));

	// Get surface format
	uint32_t surface_format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_handle, &surface_format_count, nullptr);
	std::vector<VkSurfaceFormatKHR> surface_formats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_handle, &surface_format_count, surface_formats.data());

	if (surface_format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
	{
		m_format.format = VK_FORMAT_R8G8B8A8_UNORM;
		m_format.colorSpace = surface_formats[0].colorSpace;
	}
	else
	{

	}
}

Surface::~Surface()
{
	if (m_handle)
	{
		vkDestroySurfaceKHR(m_instance, m_handle, nullptr);
	}
}

Surface::operator const VkSurfaceKHR &() const
{
	return m_handle;
}

const VkSurfaceKHR &Surface::getSurface() const
{
	return m_handle;
}
}        // namespace Ilum