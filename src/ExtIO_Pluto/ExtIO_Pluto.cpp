#define _CRT_SECURE_NO_WARNINGS


#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "resource.h"

#include "iio.h"
#include "ExtIO_Pluto.h"

#include <commctrl.h>

#define CFG_IDENTIFIER "PlutoSDR"
#define EXT_BLOCKLEN   512 * 4


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

#pragma comment(lib, "legacy_stdio_definitions.lib")

#ifdef _DEBUG
  #define CONSOLE_DEBUG // Activate a debug console
#else

#ifdef _WIN64
//#pragma comment(lib, "../lib/msvcrt_64.lib")
#else
#pragma comment(lib, "../lib/msvcrt_32.lib")
#endif

#endif
#pragma comment(lib, "comctl32.lib")

#pragma warning(disable : 4996)

#define snprintf _snprintf

static char gSDR_uri[64] = "usb:\0";

pfnExtIOCallback gCallback = 0;
volatile bool    gExitThread = false;
volatile bool    gThreadRunning = false;

HMODULE gDllInstance = NULL;
HWND gHWND_Dlg = NULL;
HWND gHWND_Host = NULL;


#ifdef  CONSOLE_DEBUG
#define ErrPrintf(msg) printf(msg "\n");
#else
#define ErrPrintf(msg) MessageBoxA(gHWND_Host, msg, NULL, MB_OK | MB_ICONERROR) 
#endif


struct
{
   iio_context* m_ctx;
   iio_device*  m_rx;
   iio_channel* m_rx0_i;
   iio_channel* m_rx0_q;
   iio_buffer * m_rxbuf;

   iio_channel* m_conf_rx;
   iio_channel* m_conf_LO; // Local Oscillator

   bool m_init_value;
   int m_gain_min;
   int m_gain_max;
   int m_gain_step;

   int64_t m_lo_min;
   int64_t m_lo_max;
   int64_t m_lo_val;

   bool m_settings_valid;
   uint32_t m_sample_rate_id;
   uint32_t m_sample_rate;
   bool     m_fix_dc_offset;
   bool     m_fix_quadrature;
   char     m_gain[64];
   int16_t  m_gain_manual;
   char     m_gain_list[256];

   void Init()
   {
       m_ctx = NULL;
       m_rxbuf = NULL;

       m_init_value = false;
       m_gain_min = -1;
       m_gain_max = 71;
       m_gain_step = 1;
       m_lo_min = 70000000;
       m_lo_max = 6000000000;
       m_lo_val = m_lo_min;

       m_settings_valid = false;

       m_sample_rate_id = 0;
       m_sample_rate = 2500000;
       m_gain_manual = 25;
       strcpy(m_gain, "manual");
       strcpy(m_gain_list, "manual");

       m_fix_dc_offset = true;
       m_fix_quadrature = false;
   }

   void Close()
   {
       if(m_rxbuf)
       {
           iio_buffer_destroy(m_rxbuf);
           m_rxbuf = NULL;
       }
       if(m_ctx)
       {
           if(m_rx0_i)
               iio_channel_disable(m_rx0_i);
           if(m_rx0_q)
               iio_channel_disable(m_rx0_q);
           iio_context_destroy(m_ctx);
           m_rx0_i = NULL;
           m_rx0_q = NULL;
           m_ctx = NULL;
       }
   }
   bool isOpen()
   {
       return m_ctx != NULL;
   }
   bool Open(const char* uri, bool test)
   {
       if(m_ctx)
           this->Close();
       m_ctx = iio_create_context_from_uri(uri);
       if(test)
           return m_ctx != NULL;
       if(m_ctx == NULL)
           return false;
       m_rx = iio_context_find_device(m_ctx, "cf-ad9361-lpc");
       if(m_rx == NULL)
       {
           ErrPrintf("rx not created");
           this->Close();
           return false;
       };
       iio_device* ad9361_phy = iio_context_find_device(m_ctx, "ad9361-phy");

#ifdef CONSOLE_DEBUG
       {
           uint32_t count = iio_context_get_devices_count(m_ctx);
           printf("Devices count %u \n", count);
           for(uint32_t i = 0; i < count; i++)
           {
               iio_device* dev = iio_context_get_device(m_ctx, i);
               const char* name = iio_device_get_name(dev);
               printf("   %u) %s \n", i, name);
           }
           if(ad9361_phy)
           {
               count = iio_device_get_channels_count(ad9361_phy);
               printf(" Devices found \"ad9361-phy\" channel: %u \n", count);
               for(uint32_t i = 0; i < count; i++)
               {
                   iio_channel* ch = iio_device_get_channel(ad9361_phy, i);
                   const char* name = iio_channel_get_name(ch);
                  // if(!name)
                   name = iio_channel_get_id(ch);
                   char* t = iio_channel_is_output(ch)?"OUT":" IN";
                   printf("   %u) %s %s\n", i, t, name);
               }
           }
           if(m_rx)
           {
               count = iio_device_get_channels_count(m_rx);
               printf(" RX device found \"cf-ad9361-lpc\" channel: %u \n", count);
               for(uint32_t i = 0; i < count; i++)
               {
                   iio_channel* ch = iio_device_get_channel(m_rx, i);
                   const char* name = iio_channel_get_name(ch);
                  // if(!name)
                   name = iio_channel_get_id(ch);
                   char* t = iio_channel_is_output(ch)?"OUT":" IN";
                   printf("   %u) %s %s\n", i, t, name);
               }
           };
       };
#endif
       
       m_conf_rx = iio_device_find_channel(ad9361_phy, "voltage0", false);
       if(m_conf_rx == NULL)
       {
           ErrPrintf("conf_rx not created (ad9361-phy : voltage0)");
           this->Close();
           return false;
       };
#ifdef CONSOLE_DEBUG
       printf("conf_rx device found    'ad9361-phy : voltage0'\n");
#endif

       m_conf_LO = iio_device_find_channel(ad9361_phy, "altvoltage0", true);
       if(m_conf_LO == NULL)
       {
           ErrPrintf("conf_LO not created");
           this->Close();
           return false;
       };
#ifdef CONSOLE_DEBUG
       printf("chnLO device found  'ad9361-phy : altvoltage0'\n");
#endif

    // Initializing AD9361 IIO streaming channels
       m_rx0_i = iio_device_find_channel(m_rx, "voltage0", false);
       if(m_rx0_i == NULL)
       {
           ErrPrintf("rx0_i not created");
           this->Close();
           return false;
       };
#ifdef CONSOLE_DEBUG
       printf("rx0_i device found  'cf-ad9361-lpc : voltage0'\n");
#endif
       m_rx0_q = iio_device_find_channel(m_rx, "voltage1", false);
       if(m_rx0_q == NULL)
       {
           ErrPrintf("rx0_q not created");
           this->Close();
           return false;
       };
#ifdef CONSOLE_DEBUG
       printf("rx0_q device found  'cf-ad9361-lpc : voltage1'\n");
#endif
       if(!m_init_value)
       {
           char attr_buf[256];

           if(iio_channel_attr_read(m_conf_rx, "hardwaregain_available", attr_buf, sizeof(attr_buf)) > 0)
           {
               float f_min, f_step = 0, f_max;
               if(sscanf(attr_buf, "[%f %f %f]", &f_min, &f_step, &f_max) == 3)
               {

                   m_gain_min  = (int)f_min;
                   m_gain_max  = (int)f_max;
                   m_gain_step = (int)f_step;
               }
           }
           if(iio_channel_attr_read(m_conf_LO, "frequency_available", attr_buf, sizeof(attr_buf)) > 0)
           {
               double min_f, step_f, max_f;
               if(sscanf(attr_buf, "[%lf %lf %lf]", &min_f, &step_f, &max_f) == 3)
               {
                   m_lo_min = (int64_t)min_f;
                   m_lo_max = (int64_t)max_f;
               }
           }
           if(iio_channel_attr_read(gIIO.m_conf_rx, "gain_control_mode_available", attr_buf, sizeof(attr_buf)) > 0)
           {
               strcpy(gIIO.m_gain_list, attr_buf);
           }

           if(!m_settings_valid)
           {
               double val;
               char txt[64];
               iio_channel_attr_read(m_conf_rx, "gain_control_mode", gIIO.m_gain, sizeof(gIIO.m_gain));

               iio_channel_attr_read(m_conf_rx, "hardwaregain", txt, sizeof(txt));
               if(sscanf(txt, "%lf", &val) == 1)
                   m_gain_manual = (int16_t)val;
               bool b;
               if(iio_channel_attr_read_bool(m_conf_rx, "bb_dc_offset_tracking_en", &b) == 0)
                   m_fix_dc_offset = b;
               if(iio_channel_attr_read_bool(m_conf_rx, "quadrature_tracking_en", &b) == 0)
                   m_fix_quadrature = b;
           }
           m_init_value = true;
       }
       return true;
   }


}gIIO;



static void UpdateDialog()
{
    if(!gHWND_Dlg)
        return;

    SetDlgItemTextA(gHWND_Dlg, IDC_COMBO_SDR, gSDR_uri);

    BOOL bEnbl = gIIO.isOpen()?FALSE:TRUE;


    EnableWindow(GetDlgItem(gHWND_Dlg, IDC_COMBO_SDR), bEnbl);
    EnableWindow(GetDlgItem(gHWND_Dlg, IDC_BUTTON_TEST), bEnbl);

    if(!gIIO.isOpen())
    {
        EnableWindow(GetDlgItem(gHWND_Dlg, IDC_COMBO_GAIN), FALSE);
        EnableWindow(GetDlgItem(gHWND_Dlg, IDC_SLIDER_GAIN), FALSE);
        EnableWindow(GetDlgItem(gHWND_Dlg, IDC_CHECK_IQ), FALSE);
        EnableWindow(GetDlgItem(gHWND_Dlg, IDC_CHECK_DC), FALSE);
        return;
    }

    static bool init = false;
    if(!init && gIIO.m_init_value)
    {
        init = true;
        {
            char *attr_buf = gIIO.m_gain_list;
            size_t len = strlen(attr_buf);
            size_t start = 0;
            for(size_t i = 0; i < len; i++)
            {
                if(attr_buf[i] == ' ')
                {
                    attr_buf[i] = 0;
                    SendDlgItemMessageA(gHWND_Dlg, IDC_COMBO_GAIN, CB_ADDSTRING, 0, (LPARAM)&attr_buf[start]);
                    attr_buf[i] = ' ';
                    start = i+1;
                }
            }
            if(start < len)
                SendDlgItemMessageA(gHWND_Dlg, IDC_COMBO_GAIN, CB_ADDSTRING, 0, (LPARAM)&attr_buf[start]);
            SendDlgItemMessageA(gHWND_Dlg, IDC_COMBO_GAIN, CB_SETMINVISIBLE, (WPARAM)4, 0);
        }

        SendDlgItemMessageA(gHWND_Dlg, IDC_COMBO_GAIN, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)gIIO.m_gain);
        SendDlgItemMessageA(gHWND_Dlg, IDC_SLIDER_GAIN, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG((int16_t)gIIO.m_gain_min, (int16_t)gIIO.m_gain_max));
        SendDlgItemMessageA(gHWND_Dlg, IDC_SLIDER_GAIN, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)gIIO.m_gain_manual);
        SendDlgItemMessageA(gHWND_Dlg, IDC_SLIDER_GAIN, TBM_SETTICFREQ, (WPARAM)1, 0);
        SendDlgItemMessageA(gHWND_Dlg, IDC_SLIDER_GAIN, TBM_SETPAGESIZE, 0, (LPARAM)10);
        SendDlgItemMessageA(gHWND_Dlg, IDC_CHECK_IQ, BM_SETCHECK, gIIO.m_fix_quadrature?BST_CHECKED:BST_UNCHECKED, 0);
        SendDlgItemMessageA(gHWND_Dlg, IDC_CHECK_DC, BM_SETCHECK, gIIO.m_fix_dc_offset?BST_CHECKED:BST_UNCHECKED, 0);

        char txt[64];
        sprintf(txt, "%i dB", gIIO.m_gain_manual);
        SetDlgItemText(gHWND_Dlg, IDC_STATIC_GAIN_DB, txt);
    }

    EnableWindow(GetDlgItem(gHWND_Dlg, IDC_COMBO_GAIN), TRUE);
    EnableWindow(GetDlgItem(gHWND_Dlg, IDC_SLIDER_GAIN), strcmp("manual", gIIO.m_gain)?FALSE:TRUE);

    EnableWindow(GetDlgItem(gHWND_Dlg, IDC_CHECK_IQ), TRUE);
    EnableWindow(GetDlgItem(gHWND_Dlg, IDC_CHECK_DC), TRUE);
}

static void CenterWindow(HWND hwnd)
{
    RECT rc;
    GetWindowRect(hwnd, &rc);

    int windowWidth  = rc.right - rc.left;
    int windowHeight = rc.bottom - rc.top;

    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}


static INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            SendDlgItemMessageA(hwndDlg, IDC_COMBO_SDR, CB_ADDSTRING, 0, (LPARAM)"usb:");
            SendDlgItemMessageA(hwndDlg, IDC_COMBO_SDR, CB_ADDSTRING, 0, (LPARAM)"ip:192.168.2.1");
            SendDlgItemMessageA(hwndDlg, IDC_COMBO_SDR, CB_ADDSTRING, 0, (LPARAM)"ip:192.168.1.100");
            gHWND_Dlg = hwndDlg;
            UpdateDialog();
            CenterWindow(hwndDlg);
            return TRUE;
        }break;

        case WM_COMMAND:
        {
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_BUTTON_TEST:
                {
                    if(GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        if(gThreadRunning)
                        {
                            MessageBoxA(hwndDlg, "Stop receiver first!", "Error", MB_OK | MB_ICONEXCLAMATION);
                            return TRUE;
                        }
                        GetDlgItemTextA(hwndDlg, IDC_COMBO_SDR, gSDR_uri, sizeof(gSDR_uri) - 1);
                        if(!gIIO.Open(gSDR_uri, true))
                        {
                            MessageBoxA(hwndDlg, "Connection failed!", "Error", MB_OK | MB_ICONERROR);
                        } else{
                            MessageBoxA(hwndDlg, "Connection successful!", "Info", MB_OK | MB_ICONINFORMATION);
                        };
                        gIIO.Close();
                        UpdateDialog();
                        return TRUE;
                    }
                }
                break;
                case IDC_COMBO_GAIN:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        HWND hCombo = (HWND)lParam;
                        int index = (int)SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
                        if(index != CB_ERR)
                        {
                            SendMessageA(hCombo, CB_GETLBTEXT, (WPARAM)index, (LPARAM)gIIO.m_gain);
                            if(gIIO.isOpen())
                            {
                                if(iio_channel_attr_write(gIIO.m_conf_rx, "gain_control_mode", gIIO.m_gain) < 0)
                                    ErrPrintf("gain_control_mode failed");
                            }
                            UpdateDialog();
                        }
                    }
                    break;
                case IDC_CHECK_IQ:
                {
                    LRESULT state = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                    gIIO.m_fix_quadrature = (state == BST_CHECKED);
                    if(gIIO.isOpen())
                        iio_channel_attr_write_bool(gIIO.m_conf_rx, "quadrature_tracking_en", gIIO.m_fix_quadrature);
                }break;
                case IDC_CHECK_DC:
                {
                    gIIO.m_fix_dc_offset = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if(gIIO.isOpen())
                    {
                        iio_channel_attr_write_bool(gIIO.m_conf_rx, "bb_dc_offset_tracking_en", gIIO.m_fix_dc_offset);
                        iio_channel_attr_write_bool(gIIO.m_conf_rx, "rf_dc_offset_tracking_en", gIIO.m_fix_dc_offset);
                    }
                }break;
            }
        }
        break;
        case WM_HSCROLL:
        {
            int controlID = GetDlgCtrlID((HWND)lParam);
            if(controlID == IDC_SLIDER_GAIN)
            {
                int pos = (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                if(pos < gIIO.m_gain_min)
                    pos = gIIO.m_gain_min;
                if(pos > gIIO.m_gain_max)
                    pos = gIIO.m_gain_max;
                gIIO.m_gain_manual = (int16_t)pos;
                if(gIIO.isOpen())
                {
                    if(iio_channel_attr_write_double(gIIO.m_conf_rx, "hardwaregain", gIIO.m_gain_manual) < 0)
                        ErrPrintf("hardwaregain failed");
                }
                char txt[64];
                sprintf(txt, "%i dB", gIIO.m_gain_manual);
                SetDlgItemText(hwndDlg, IDC_STATIC_GAIN_DB, txt);
            }
            break;
        }
        case WM_SHOWWINDOW:
            UpdateDialog();
            return TRUE;

        case WM_CLOSE:
            ShowWindow(hwndDlg, SW_HIDE);
            return TRUE;

        case WM_DESTROY:
            ShowWindow(hwndDlg, SW_HIDE);
            gHWND_Dlg = NULL;
            return TRUE;
    }
    return FALSE;
}


static DWORD WINAPI GeneratorThreadProc(LPVOID lpParameter)
{
    int16_t iqbuf[EXT_BLOCKLEN * 2];
    ssize_t nbytes_rx;
    char    *p_dat, *p_end;
    ptrdiff_t p_inc;

    int iqcnt = 0; // pointer to sample in iqbuf
    
    while(!gExitThread)
    {
        nbytes_rx = iio_buffer_refill(gIIO.m_rxbuf);
        //if (nbytes_rx < 0) { printf("Error refilling buf %d\n", (int)nbytes_rx); }
        p_inc = iio_buffer_step(gIIO.m_rxbuf);
        p_end = (char *)iio_buffer_end(gIIO.m_rxbuf);
        for (p_dat = (char *)iio_buffer_first(gIIO.m_rxbuf, gIIO.m_rx0_i); p_dat < p_end; p_dat += p_inc)
        {
            iqbuf[iqcnt++] = ((int16_t*)p_dat)[0];
            iqbuf[iqcnt++] = ((int16_t*)p_dat)[1];

            if (iqcnt == EXT_BLOCKLEN * 2)
            { // buffer full
                iqcnt = 0;
                gCallback(EXT_BLOCKLEN, 0, 0.0F, &iqbuf[0]);
            }
        }
    }
    gExitThread = false;
    gThreadRunning = false;
    return 0;
}


static void stopThread()
{
    if(gThreadRunning)
    {
        gExitThread = true;
        while(gThreadRunning)
            Sleep(10);
    }
}


static void startThread()
{
    gExitThread = false;
    gThreadRunning = true;

    HANDLE hThread = CreateThread(NULL, (SIZE_T)(64 * 1024), GeneratorThreadProc, NULL, 0, NULL);
    if(!hThread)
    {
        SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
        CloseHandle(hThread);
    }
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    if(ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        gIIO.Init();
        gDllInstance = hModule;
        InitCommonControls();
    }
    return TRUE;
}


static void UpdateHostWnd()
{
    if(gHWND_Host != NULL)
        return;
    gHWND_Host = GetActiveWindow();
    if(!gHWND_Host)
        gHWND_Host = GetForegroundWindow();
    if(gHWND_Dlg != NULL)
       SetWindowLongPtrA(gHWND_Dlg, GWLP_HWNDPARENT, (LONG_PTR)gHWND_Host);
}


extern "C" bool EXTIO_API InitHW(char *name, char *model, int& type)
{
#ifdef CONSOLE_DEBUG
    if (AllocConsole())
    {
        FILE* f;
        freopen_s(&f, "CONOUT$", "wt", stdout);
        SetConsoleTitle(TEXT("Debug Console ExtIO_PlutoSDR"));
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
    }
#endif

    type = exthwUSBdata16;
    strcpy(name, "PlutoSDR");
    strcpy(model, "Adalm-Pluto");

    if(!gIIO.isOpen())
    {
        if(!gIIO.Open(gSDR_uri, false))
        {
       //     char txt[128];
      //      sprintf(txt, "Connection failed!\r\nInitHW\r\nPluto URI: %s", gSDR_uri);
         //   MessageBoxA(NULL, txt, "Error", MB_OK | MB_ICONERROR);
            // return false;
        };
    }
    return true;
}


extern "C" bool EXTIO_API OpenHW(void)
{
    return true;
}


extern "C" int EXTIO_API StartHW(long LOfreq)
{
    return (int)StartHW64((int64_t)LOfreq);
}


extern "C" int64_t EXTIO_API StartHW64(int64_t LOfreq)
{
    stopThread();
    UpdateHostWnd();
#ifdef CONSOLE_DEBUG
    printf("StartHW uri: %s\n", gSDR_uri);
#endif
    if (!gIIO.isOpen())
    {
        if(!gIIO.Open(gSDR_uri, false))
        {
            char txt[128];
            sprintf(txt, "Connection failed!\r\nPluto URI: %s", gSDR_uri);
            MessageBoxA(gHWND_Host, txt, "Error", MB_OK | MB_ICONERROR);
            return -1;
        }
    }

    // setting reciever
    if(iio_channel_attr_write(gIIO.m_conf_rx, "rf_port_select", "A_BALANCED") < 0)
        ErrPrintf("rf_port_select failed");
    if(iio_channel_attr_write_longlong(gIIO.m_conf_rx, "rf_bandwidth", (long)(gIIO.m_sample_rate * 0.8)) < 0)
        ErrPrintf("rf_bandwidth failed");
    if(iio_channel_attr_write_longlong(gIIO.m_conf_rx, "sampling_frequency", gIIO.m_sample_rate) < 0)
        ErrPrintf("sampling_frequency failed");
    if(iio_channel_attr_write(gIIO.m_conf_rx, "gain_control_mode", gIIO.m_gain) < 0)
        ErrPrintf("gain_control_mode failed");
    if(!strcmp(gIIO.m_gain, "manual"))
    {
        if(iio_channel_attr_write_double(gIIO.m_conf_rx, "hardwaregain", gIIO.m_gain_manual) < 0)
            ErrPrintf("hardwaregain failed");
    }
    iio_channel_attr_write_bool(gIIO.m_conf_rx, "bb_dc_offset_tracking_en", gIIO.m_fix_dc_offset);
    iio_channel_attr_write_bool(gIIO.m_conf_rx, "rf_dc_offset_tracking_en", gIIO.m_fix_dc_offset);
    iio_channel_attr_write_bool(gIIO.m_conf_rx, "quadrature_tracking_en", gIIO.m_fix_quadrature);

    // setting LO
    if(iio_channel_attr_write_longlong(gIIO.m_conf_LO, "frequency", LOfreq) < 0)
        ErrPrintf("frequency set failed");
    gIIO.m_lo_val = LOfreq;

    // Enabling IIO streaming channels
    iio_channel_enable(gIIO.m_rx0_i);
    iio_channel_enable(gIIO.m_rx0_q);

    gIIO.m_rxbuf = iio_device_create_buffer(gIIO.m_rx, 1024 * 16, false);
    if (!gIIO.m_rxbuf)
    {
        ErrPrintf("Could not create RX buffer");
        if (gIIO.m_rx0_i)
            iio_channel_disable(gIIO.m_rx0_i);
        if (gIIO.m_rx0_q)
            iio_channel_disable(gIIO.m_rx0_q);
        return -1;
    }
#ifdef CONSOLE_DEBUG
    printf("rxbuf created\n");
#endif

    startThread();
    UpdateDialog();
    return EXT_BLOCKLEN;
}




extern "C" void EXTIO_API StopHW()
{
    stopThread();
    gIIO.Close();
    UpdateDialog();
}


extern "C" void EXTIO_API CloseHW()
{
    gIIO.Close();
    if(gHWND_Dlg)
        DestroyWindow(gHWND_Dlg);
}


extern "C" int EXTIO_API SetHWLO(long LOfreq)
{
    int64_t ret = SetHWLO64((int64_t)LOfreq);
    return (ret & 0xFFFFFFFF);
}


extern "C" int64_t EXTIO_API SetHWLO64(int64_t LOfreq)
{
    const int64_t wishedLO = LOfreq;
    int64_t ret = 0;

    // check limits
    if(LOfreq < gIIO.m_lo_min)
    {
        LOfreq = gIIO.m_lo_min;
        ret = -gIIO.m_lo_min;
    }else if(LOfreq > gIIO.m_lo_max)
    {
        LOfreq = gIIO.m_lo_max;
        ret = gIIO.m_lo_max;
    }

    if(gIIO.isOpen())
    {
        // setting LO
#ifdef CONSOLE_DEBUG
        printf("chnLO set 'frequency' %llu \n", LOfreq);
#endif
        if(iio_channel_attr_write_longlong(gIIO.m_conf_LO, "frequency", LOfreq) < 0)
            ErrPrintf("LO frequency set failed");
    }
    if (wishedLO != LOfreq && gCallback)
        gCallback(-1, extHw_Changed_LO, 0.0F, 0);
    gIIO.m_lo_val = LOfreq;
    return ret;
}


extern "C" int EXTIO_API GetStatus()
{
    return 0;
}


extern "C" void EXTIO_API SetCallback( pfnExtIOCallback funcptr )
{
    gCallback = funcptr;
    return;
}


extern "C" long EXTIO_API GetHWLO()
{
    return (long)GetHWLO64();
}


extern "C" int64_t EXTIO_API GetHWLO64()
{
    if(gIIO.isOpen())
    {
        long long frq = 0;
        // setting LO
        if(iio_channel_attr_read_longlong(gIIO.m_conf_LO, "frequency", &frq) < 0)
            ErrPrintf("frequency read failed");
        return frq;
    }
    return gIIO.m_lo_val;
}


extern "C" long EXTIO_API GetHWSR()
{

    // This DLL controls just an oscillator, not a digitizer
    return gIIO.m_sample_rate;
}


extern "C" void EXTIO_API VersionInfo(const char* progname, int ver_major, int ver_minor)
{
}


extern "C" int EXTIO_API ExtIoGetSrates(int srate_idx, double* samplerate)
{
    switch(srate_idx)
    {
        case 0: *samplerate = 2500000.0; break;
        case 1: *samplerate = 3000000.0; break;
        case 2: *samplerate = 4000000.0; break;
        case 3: *samplerate = 6000000.0; break;
        case 4: *samplerate = 10000000.0; break;
        case 5: *samplerate = 20000000.0; break;
        default: return 1;    // ERROR
    }
    return 0;
}


extern "C" int EXTIO_API ExtIoGetActualSrateIdx()
{
    return gIIO.m_sample_rate_id;
}


extern "C" int EXTIO_API ExtIoSetSrate(int srate_idx)
{
    double newSrate = 0.0;
    if(0 == ExtIoGetSrates(srate_idx, &newSrate))
    {
        gIIO.m_sample_rate_id = srate_idx;
        gIIO.m_sample_rate = (unsigned)(newSrate + 0.5);
        if(gIIO.isOpen())
        {
            if(iio_channel_attr_write_longlong(gIIO.m_conf_rx, "rf_bandwidth", (long)(gIIO.m_sample_rate * 0.8)) < 0)
            {
                ErrPrintf("rf_bandwidth set failed");
                return 1;
            };
            if(iio_channel_attr_write_longlong(gIIO.m_conf_rx, "sampling_frequency", gIIO.m_sample_rate) < 0)
            {
                ErrPrintf("sampling_frequency set failed");
                return 1;
            };
            return 0;
        }        
    }
    return 1; // ERROR
}


extern "C" long EXTIO_API ExtIoGetBandwidth(int srate_idx)
{
    double newSrate = 0.0;
    if(0 == ExtIoGetSrates(srate_idx, &newSrate))
         return (long)(newSrate * 0.8);
    return -1; // ERROR
}


extern "C" int EXTIO_API ExtIoGetSetting(int idx, char * description, char * value)
{
    switch(idx)
    {
        case 0:
            snprintf(description, 128, "%s", "Identifier");
            snprintf(value, 128, "%s", CFG_IDENTIFIER);
            break;
        case 1:
            snprintf(description, 128, "%s", "SampleRateID");
            snprintf(value, 128, "%d", gIIO.m_sample_rate_id);
            break;
        case 2:
            snprintf(description, 128, "%s", "SDR_uri");
            snprintf(value, 128, "%s", gSDR_uri);
            break;
        case 3:
            snprintf(description, 128, "%s", "Gain");
            snprintf(value, 128, "%s %i", gIIO.m_gain, gIIO.m_gain_manual);
            break;
        case 4:
            snprintf(description, 128, "%s", "Fix_IQ_DC");
            snprintf(value, 128, "%i %i", gIIO.m_fix_quadrature, gIIO.m_fix_dc_offset);
            break;
        default:
            return -1; // ERROR
    }
    return 0;
}


extern "C" void EXTIO_API ExtIoSetSetting(int idx, const char * value)
{
    double newSrate;
    float  newAtten = 0.0F;
    int tempInt, tempInt2;

    if(idx != 0 && !gIIO.m_settings_valid)
        return; // ignore settings for some other ExtIO

    switch(idx)
    {
        case 0:
            gIIO.m_settings_valid = (value && !strcmp(value, CFG_IDENTIFIER));
            break;
        case 1://SampleRateID
            tempInt = atoi(value);
            if(0 == ExtIoGetSrates(tempInt, &newSrate))
            {
                gIIO.m_sample_rate_id = tempInt;
                gIIO.m_sample_rate = (unsigned)(newSrate + 0.5);
            }
            break;
        case 2://SDR_uri
            strcpy(gSDR_uri, value);
            break;
        case 3://Gain
            if(sscanf(value, "%s %i", gIIO.m_gain, &tempInt) == 2)
                gIIO.m_gain_manual = (int16_t)tempInt;
            break;
        case 4://Fix_IQ_DC
            if(sscanf(value, "%i %i", &tempInt, &tempInt2) == 2)
            {
                gIIO.m_fix_quadrature = (tempInt != 0);
                gIIO.m_fix_dc_offset = (tempInt2 != 0);
            }
            break;
    }
}


extern "C" void EXTIO_API ShowGUI()
{
    UpdateHostWnd();
    if(!gHWND_Dlg)
        CreateDialogA(gDllInstance, MAKEINTRESOURCE(ExtIO_PlutoDialog), gHWND_Host, (DLGPROC)MainDlgProc);
    ShowWindow(gHWND_Dlg, SW_SHOW);
    SetFocus(GetDlgItem(gHWND_Dlg, gIIO.isOpen()?IDC_COMBO_GAIN:IDC_COMBO_SDR));
}


extern "C" void EXTIO_API HideGUI()
{
    ShowWindow(gHWND_Dlg, SW_HIDE);
}



//fix no runtime VC 2022

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


extern "C" int _except_handler4_common() { return 0; }

//_except_handler4_common