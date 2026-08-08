#pragma once

#include <vulkan/vulkan.h>

// Do we want to enable the added Vulkan debug?
#define ENABLE_VULKAN_DEBUG_CALLBACK

// For variable argument functions e.g., dprintf(..),
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>	// vsprintf_s

// Saving debug information to a log file/screen/..
inline
void dprintf(const char* fmt, ...) {
	va_list parms;
	static char buf[2048] = { 0 };

	// Try to print in the allocated space.
	va_start(parms, fmt);
	vsprintf_s(buf, fmt, parms);
	va_end(parms);

	// Write the information out to a txt file
#if 0
	FILE* fp = fopen("output.txt", "a");
	fprintf(fp, "%s", buf);
	fclose(fp);
#endif

	// Output to the visual studio window
	// OutputDebugStringA( buf );
	printf(buf);

} // End dprintf(..)

// Debug defines (custom asserts)
#if defined(_WIN32)
#define DBG_ASSERT(f) { if(!(f)){ __debugbreak(); }; }
#else
#define DBG_ASSERT(f) { #error(platform assert todo) }
#endif

//-------------------------------------------------------//
//-------------------------------------------------------//

#ifdef ENABLE_VULKAN_DEBUG_CALLBACK //Debug callback
// Set this function as a debug callback when we initialize
// Vulkan to let us know if something went wrong
VKAPI_ATTR VkBool32 VKAPI_CALL
MyDebugReportCallback(VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData)
{
	printf(pLayerPrefix);
	printf(" ");
	printf(pMessage);
	printf("\n");
	DBG_ASSERT(false);

	return VK_FALSE;
}// End MyDebugReportCallback(..)
#endif