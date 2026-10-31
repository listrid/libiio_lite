#include <windows.h>
#include <stdio.h>


const char* TARGET_DEVICES[] =
{
    "VID_0456&PID_B673&MI_05", // PlutoSDR IIO USBD
    "VID_0456&PID_B672&MI_05", // M2k IIO USBD
    "VID_0456&PID_B671&MI_00", // Generic IIO USBD
    "VID_0456&PID_B676&MI_05", // PlutoSDR2 IIO USBD
    "VID_0456&PID_B678&MI_06"  // ADI Generic IIO USBD
};

#define DEVICE_COUNT (sizeof(TARGET_DEVICES) / sizeof(TARGET_DEVICES[0]))


void FixDeviceParameters(HKEY hBaseKey, const char* subKeyName) {
    char path[512];
    HKEY hKey;

    sprintf(path, "%s\\Device Parameters", subKeyName);

    LONG result = RegOpenKeyExA(hBaseKey, path, 0, KEY_READ | KEY_WRITE, &hKey);
    if(result == ERROR_FILE_NOT_FOUND)
        result = RegCreateKeyExA(hBaseKey, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);

    if(result == ERROR_SUCCESS)
    {
        result = RegQueryValueExA(hKey, "DeviceInterfaceGUIDs", NULL, NULL, NULL, NULL);

        if(result == ERROR_SUCCESS)
        {
            printf("  [SKIP]    Already exists for: %s\n", subKeyName);
        } else {
            char guidData[] = "{6E6E6375-582A-4A59-A967-E5C35082518B}\0";
            DWORD dataSize = sizeof(guidData);
            result = RegSetValueExA(hKey, "DeviceInterfaceGUIDs", 0, REG_MULTI_SZ, (BYTE*)guidData, dataSize);
            if(result == ERROR_SUCCESS)
            {
                printf("  [PATCHED] Written GUID for: %s\n", subKeyName);
            }else{
                printf("  [ERROR] Failed to write value. Code: %ld\n", result);
            }
        }
        RegCloseKey(hKey);
    }else{
        printf("  [ERROR] Failed to open/create Device Parameters. Code: %ld\n", result);
    }
}

int main()
{
    printf("Checking registry for Analog Devices IIO hardware configurations (WinUSB context)...\n");
    printf("Target GUID: {6E6E6375-582A-4A59-A967-E5C35082518B}\n\n");
    int patchedCount = 0;

    for(size_t i = 0; i < DEVICE_COUNT; i++)
    {
        char basePath[256];
        HKEY hDeviceKey;

        sprintf(basePath, "SYSTEM\\CurrentControlSet\\Enum\\USB\\%s", TARGET_DEVICES[i]);

        LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, basePath, 0, KEY_ENUMERATE_SUB_KEYS, &hDeviceKey);

        if(result == ERROR_SUCCESS)
        {
            printf("[*] Found Active Tree for ID: %s\n", TARGET_DEVICES[i]);

            DWORD index = 0;
            char subKeyName[256];
            DWORD subKeySize = 256;
            while(RegEnumKeyExA(hDeviceKey, index, subKeyName, &subKeySize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                FixDeviceParameters(hDeviceKey, subKeyName);
                subKeySize = 256;
                index++;
                patchedCount++;
            }
            RegCloseKey(hDeviceKey);
        } else if(result == ERROR_ACCESS_DENIED)
        {
            printf("[CRITICAL ERROR] Access Denied! RUN AS ADMINISTRATOR.\n");
            break;
        }
    }

    printf("\nExecution complete. Devices processed.\n");
    printf("If any new devices were [PATCHED], please UNPLUG and RE-PLUG them now.\n\n");

    printf("Press Enter to exit...");
    getchar();
    return 0;
}
