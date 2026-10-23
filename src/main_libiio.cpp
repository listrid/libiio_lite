#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

#pragma comment(lib, "legacy_stdio_definitions.lib")

#ifdef _WIN64
#  pragma comment(lib, "../lib/vcruntime_64.lib")
#else
#  pragma comment(lib, "../lib/vcruntime_32.lib")
#endif


BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
	    case DLL_PROCESS_ATTACH:
	    case DLL_THREAD_ATTACH:
	    case DLL_THREAD_DETACH:
	    case DLL_PROCESS_DETACH:
		    break;
	}
	return TRUE;
}

