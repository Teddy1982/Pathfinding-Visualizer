#pragma once

#include <vulkan/vulkan.h>

// Headerdatei zum Debuggen von Vulkanfunktionen

#define ENABLE_VULKAN_DEBUG_CALLBACK

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

// Logfunktion zum Ausgeben auf dem Bildschirm oder ggfs. in eine Logdatei
inline
void dprintf(const char* fmt, ...) {
	va_list parms;
	static char buf[2048] = { 0 };

	va_start(parms, fmt);
	vsprintf_s(buf, fmt, parms);
	va_end(parms);

	// Schreiben von Informationen in eine Logdatei
#if 0
	FILE* fp = fopen("output.txt", "a");
	fprintf(fp, "%s", buf);
	fclose(fp);
#endif

	// Ausgeben der Informationen auf dem Bildschirm
	printf(buf);

}

// Debug-Definitionen (gewöhnliche Asserts)
#if defined(_WIN32)
#define DBG_ASSERT(f) { if(!(f)){ __debugbreak(); }; }
#else
#define DBG_ASSERT(f) { #error(platform assert todo) }
#endif

//-------------------------------------------------------//
//-------------------------------------------------------//

#ifdef ENABLE_VULKAN_DEBUG_CALLBACK //Debug callback

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
}
#endif