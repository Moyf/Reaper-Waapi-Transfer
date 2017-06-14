#ifdef _WIN32
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version = '6.0.0.0' processorArchitecture = '*' publicKeyToken = '6595b64144ccf1df' language = '*'\"")
#include <windows.h>
#include <Wincodec.h>
#include <Commctrl.h>
#else
#endif

#include "reaper_plugin.h"

#define REAPERAPI_IMPLEMENT
#include "reaper_plugin_functions.h"

#include "Reaper_WAAPI_Transfer.h"
#include "TransferWindowHandler.h"
#include "RecallWindowHandler.h"
#include "WAAPITransfer.h"
#include "resource.h"
#include "WAAPIHelpers.h"
#include "WwiseSettingsReader.h"

#define GET_FUNC_AND_CHKERROR(x) if (!((*((void **)&(x)) = (void *)rec->GetFunc(#x)))) ++funcerrcnt
#define REGISTER_AND_CHKERROR(variable, name, info) if(!(variable = rec->Register(name, (void*)info))) ++regerrcnt

//define globals
HWND g_parentWindow;
HINSTANCE g_hInst;

int g_Waapi_Port;

//actions
gaccel_register_t actionOpenTransferWindow = { { 0, 0, 0 }, "Open WAAPI transfer window." };
gaccel_register_t actionOpenRecallWindow = { { 0, 0, 0 }, "Open WAAPI recall window." };

//produces an error message during reaper startup
//similar to SWS function ErrMsg in sws_extension.cpp
void StartupError(const std::string &errMsg, 
                  uint32 messageBoxFlags = MB_ICONERROR, 
                  const std::string &caption = "Waapi Transfer Error")
{
    if (!IsREAPER || IsREAPER())
    {
        HWND msgBoxHwnd = Splash_GetWnd ? Splash_GetWnd() : nullptr;
        MessageBox(msgBoxHwnd, errMsg.c_str(), caption.c_str(), MB_OK | messageBoxFlags);
    }
}

extern "C"
{
    REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
    {
        //return if plugin is exiting
        if (!rec)
        {
            return 0;
        }
        //set globals
        g_parentWindow = rec->hwnd_main;
        g_hInst = hInstance;

        //get func pointers that we need
        int funcerrcnt = 0;
        GET_FUNC_AND_CHKERROR(Main_OnCommand);
        GET_FUNC_AND_CHKERROR(EnumProjectMarkers2);
        GET_FUNC_AND_CHKERROR(ShowConsoleMsg);
        GET_FUNC_AND_CHKERROR(GetMainHwnd);
        GET_FUNC_AND_CHKERROR(GetResourcePath);
        GET_FUNC_AND_CHKERROR(GetProjectPath);
        GET_FUNC_AND_CHKERROR(EnumProjects);
        GET_FUNC_AND_CHKERROR(Main_openProject);
        GET_FUNC_AND_CHKERROR(GetProjectName);
        GET_FUNC_AND_CHKERROR(AddExtensionsMainMenu);
        GET_FUNC_AND_CHKERROR(plugin_register);
        GET_FUNC_AND_CHKERROR(IsREAPER);
        GET_FUNC_AND_CHKERROR(Splash_GetWnd);
        GET_FUNC_AND_CHKERROR(EnumProjectMarkers);

        //exit if any func pointer couldn't be found
        if (funcerrcnt)
        {
            StartupError("An error occured whilst initializing WAAPI Transfer.\n"
                         "Try updating to the latest Reaper version.");
            return 0;
        }

        //register commands
        int regerrcnt = 0;
        REGISTER_AND_CHKERROR(actionOpenTransferWindow.accel.cmd, "command_id", "actionOpenTransferWindow");
        REGISTER_AND_CHKERROR(actionOpenRecallWindow.accel.cmd, "command_id", "actionOpenRecallWindow");
        if (regerrcnt)
        {
            StartupError("An error occured whilst initializing the WAAPI Transfer actions.\n"
                         "Try updating to the latest Reaper version.");
            return 0;
        }

        //register actions
        plugin_register("gaccel", &actionOpenRecallWindow.accel);
        plugin_register("gaccel", &actionOpenTransferWindow.accel);

        rec->Register("hookcommand", (void*)HookCommandProc);

        AddExtensionsMainMenu();

        HMENU hMenu = GetSubMenu(GetMenu(GetMainHwnd()), 8);
        {
            MENUITEMINFO mi = { sizeof(MENUITEMINFO), };
            mi.fMask = MIIM_TYPE | MIIM_ID;
            mi.fType = MFT_STRING;
            mi.wID = actionOpenTransferWindow.accel.cmd;
            mi.dwTypeData = "WAAPI Transfer";
            InsertMenuItem(hMenu, 0, true, &mi);
        }
        {
            MENUITEMINFO mi = { sizeof(MENUITEMINFO), };
            mi.fMask = MIIM_TYPE | MIIM_ID;
            mi.fType = MFT_STRING;
            mi.wID = actionOpenRecallWindow.accel.cmd;
            mi.dwTypeData = "WAAPI Recall";
            InsertMenuItem(hMenu, 1, true, &mi);
        }

        //setup images
        WwiseImageList::LoadIcons({
            { "WorkUnit", IDI_WORKUNIT },
            { "ActorMixer", IDI_ACTORMIXER },
            { "BlendContainer", IDI_BLENDCONTAINER },
            { "RandomSequenceContainer", IDI_RANDOMCONTAINER },
            { "SequenceContainer", IDI_SEQUENCECONTAINER },
            { "SwitchContainer", IDI_SWITCHCONTAINER },
            { "Folder", IDI_FOLDER},
            { "MusicSegment", IDI_MUSICSEGMENT}
        });

        //lookup wwise Waapi info
        int waapiEnabled, waapiPort;
        if (GetWaapiSettings(waapiEnabled, waapiPort))
        {
            if (waapiEnabled == -1)
            {
                StartupError("Waapi is disabled in Wwise settings or you are using an unsupported version of Wwise.\n" 
                             "Please enable Waapi or update Wwise to use Waapi Transfer.");
            }
            if (waapiEnabled == 0)
            {
                StartupError("Waapi is disabled in Wwise settings, please enable it to use Waapi Transfer.");
            }
            if (waapiPort != -1)
            {
                g_Waapi_Port = waapiPort;
            }
            else
            {
                g_Waapi_Port = WAAPI_DEFAULT_PORT;
            }
        }
        else
        {
            g_Waapi_Port = WAAPI_DEFAULT_PORT;
        }


        return 1;
    }
}


bool HookCommandProc(int command, int flag)
{
    if (command == actionOpenTransferWindow.accel.cmd)
    {
        OpenTransferWindow();
        return true;
    }
    if (command == actionOpenRecallWindow.accel.cmd)
    {
        OpenRecallWindow();
        return true;
    }
    return false;
}