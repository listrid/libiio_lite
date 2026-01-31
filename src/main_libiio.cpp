#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

#pragma comment(lib, "legacy_stdio_definitions.lib");

#pragma comment(lib, "../lib/msvcrt_64.lib")


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


#include <vcruntime_string.h>
#include <vcruntime_typeinfo.h>

struct __type_info_node
{
    _SLIST_HEADER _Header;
};
#define _free_crt     free

extern "C" void __cdecl __std_type_info_destroy_list(
    __type_info_node* const root_node
)
{
    PSLIST_ENTRY current_node = InterlockedFlushSList(&root_node->_Header);
    while(current_node)
    {
        PSLIST_ENTRY const next_node = current_node->Next;
        _free_crt(current_node);
        current_node = next_node;
    }
}