#include <std/Win95/stdSysTagsWin32.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Main/JonesMainSys.h>
#include <windows.h> // Fixed: types.h no longer includes windows.h; include it after Strict is defined
#include <windowsx.h>
#include <Jones3D/Display/JonesDisplay.h>
#include <Jones3D/Display/jonesConfig.h>
#include <Jones3D/Main/JonesFile.h>
#include <rdroid/Primitives/rdModel3.h>
#include <sith/Main/sithMain.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithModel.h>
#include <std/General/std.h>
#include <std/General/stdConfig.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdUtil.h>
#include <std/General/stdFnames.h>
#include <std/Win95/std3D.h>


// Lives further down in this file.
INT_PTR CALLBACK JonesMain_DevDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// These live just below in this file now; they read the startup display data through
// JonesMain_GetStartupDisplayEnv/JonesMain_GetCurVideoMode and keep their RTI hooks from
// JonesMain.c. JonesMain_CurDisplaySupportsBPP still lives in Jones3D/Main/JonesMain.c.
int J3DAPI JonesMain_InitDevDialog(HWND hDlg, WPARAM wParam, JonesState* pConfig);
void J3DAPI JonesMain_DevDialogHandleCommand(HWND hWnd, int controlId, LPARAM lParam, int notifyCode);
void J3DAPI JonesMain_DevDialogInitDisplayDevices(HWND hDlg, JonesState* pConfig);
bool J3DAPI JonesMain_CurDisplaySupportsBPP(const JonesDisplaySettings* pSettings, size_t bpp);

int J3DAPI JonesMain_ShowDevDialog(HWND hWnd, JonesState* pConfig)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    return DialogBoxParam(hInstance, (LPCSTR)101, hWnd, JonesMain_DevDialogProc, (LPARAM)pConfig);
}


int J3DAPI JonesMain_ShowDisplayOptions(HWND hWnd, JonesState* pState)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    return DialogBoxParam(hInstance, (LPCSTR)106, hWnd, JonesMain_DevDialogProc, (LPARAM)pState);
}


INT_PTR CALLBACK JonesMain_DevDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            return JonesMain_InitDevDialog(hDlg, wParam, (JonesState*)lParam);

        case WM_COMMAND:
            JonesMain_DevDialogHandleCommand(hDlg, (uint16_t)wParam, lParam, HIWORD(wParam));
            return 1;
    };

    return 0;
}


void J3DAPI JonesMain_DevDialogUpdateRadioButtons(HWND hDlg, const JonesState* pState)
{
    switch ( pState->outputMode )
    {
        case JONES_OUTPUTMODE_NONE:
            CheckRadioButton(hDlg, 1013, 1015, 1013); // Normal
            break;
        case JONES_OUTPUTMODE_CONSOLE:
            CheckRadioButton(hDlg, 1013, 1015, 1014); // Console
            break;
        case JONES_OUTPUTMODE_LOGFILE:
            CheckRadioButton(hDlg, 1013, 1015, 1015); // Log file
            break;
    };

    switch ( pState->logLevel )
    {
        case JONES_LOGLEVEL_ERROR:
            CheckRadioButton(hDlg, 1016, 1018, 1018); // Quite
            break;
        case JONES_LOGLEVEL_NORMAL:
            CheckRadioButton(hDlg, 1016, 1018, 1016); // Normal
            break;
        case JONES_LOGLEVEL_VERBOSE:
            CheckRadioButton(hDlg, 1016, 1018, 1017); // Verbose
            break;
    };

    switch ( pState->performanceLevel )
    {
        case 0:
            CheckRadioButton(hDlg, 1176, 1180, 1176);
            break;

        case 1:
            CheckRadioButton(hDlg, 1176, 1180, 1177);
            break;

        case 2:
            CheckRadioButton(hDlg, 1176, 1180, 1178);
            break;

        case 3:
            CheckRadioButton(hDlg, 1176, 1180, 1179);
            break;

        case 4:
            CheckRadioButton(hDlg, 1176, 1180, 1180);
            break;

        default:
            return;
    }
}


int J3DAPI JonesMain_InitDevDialog(HWND hDlg, WPARAM wParam, JonesState* pConfig)
{
    J3D_UNUSED(wParam);

    // Added: Adjust window title to aid new user support.
    SetWindowText(hDlg, "Welcome to OpenJones3D");

    // Populate display driver combo box list
    HWND hDlgItem = GetDlgItem(hDlg, 1030); // Display driver
    for ( size_t i = 0; i < JonesMain_GetStartupDisplayEnv()->numInfos; ++i )
    {
        if ( JonesMain_GetStartupDisplayEnv()->aDisplayInfos[i].displayDevice.bHAL == 1 )
        {
            STD_FORMAT(std_g_genBuffer, "%s", JonesMain_GetStartupDisplayEnv()->aDisplayInfos[i].displayDevice.aDriverName);
            int itemIdx = ComboBox_AddString(hDlgItem, std_g_genBuffer);
            ComboBox_SetItemData(hDlgItem, itemIdx, i);

            // Select driver if matches the one stored in settings
            if ( i == pConfig->displaySettings.displayDeviceNum )
            {
                ComboBox_SetCurSel(hDlgItem, itemIdx);
            }
        }
    }

    // Init CB GeometryMOde
    hDlgItem = GetDlgItem(hDlg, 1008);
    int itemIdx = ComboBox_AddString(hDlgItem, "Vertex Only");
    ComboBox_SetItemData(hDlgItem, itemIdx, 1);

    int selectedItemIdx = itemIdx;
    itemIdx = ComboBox_AddString(hDlgItem, "Wire Frame");
    ComboBox_SetItemData(hDlgItem, itemIdx, 2);
    if ( pConfig->displaySettings.geoMode == RD_GEOMETRY_WIREFRAME )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Solid");
    ComboBox_SetItemData(hDlgItem, itemIdx, 3);
    if ( pConfig->displaySettings.geoMode == RD_GEOMETRY_SOLID )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Texture");
    ComboBox_SetItemData(hDlgItem, itemIdx, 4);
    if ( pConfig->displaySettings.geoMode == RD_GEOMETRY_FULL )
    {
        selectedItemIdx = itemIdx;
    }

    ComboBox_SetCurSel(hDlgItem, selectedItemIdx);

    // Init CB Lighting Mode
    hDlgItem = GetDlgItem(hDlg, 1009);
    itemIdx = ComboBox_AddString(hDlgItem, "None");
    ComboBox_SetItemData(hDlgItem, itemIdx, 1);

    selectedItemIdx = itemIdx;
    itemIdx = ComboBox_AddString(hDlgItem, "Lit");
    ComboBox_SetItemData(hDlgItem, itemIdx, 0);
    if ( pConfig->displaySettings.lightMode == RD_LIGHTING_NONE )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Diffuse");
    ComboBox_SetItemData(hDlgItem, itemIdx, 2);
    if ( pConfig->displaySettings.lightMode == RD_LIGHTING_DIFFUSE )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Gouraud");
    ComboBox_SetItemData(hDlgItem, itemIdx, 3);
    if ( pConfig->displaySettings.lightMode == RD_LIGHTING_GOURAUD )
    {
        selectedItemIdx = itemIdx;
    }

    ComboBox_SetCurSel(hDlgItem, selectedItemIdx);

    // Init CB MimpMap Filter
    hDlgItem = GetDlgItem(hDlg, 1012);
    itemIdx = ComboBox_AddString(hDlgItem, "None");
    ComboBox_SetItemData(hDlgItem, itemIdx, 0);
    if ( pConfig->displaySettings.filter == STD3D_MIPMAPFILTER_NONE )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Bilinear");
    ComboBox_SetItemData(hDlgItem, itemIdx, STD3D_MIPMAPFILTER_BILINEAR);
    if ( pConfig->displaySettings.filter == STD3D_MIPMAPFILTER_BILINEAR )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Trilinear");
    ComboBox_SetItemData(hDlgItem, itemIdx, STD3D_MIPMAPFILTER_TRILINEAR);
    if ( pConfig->displaySettings.filter == STD3D_MIPMAPFILTER_TRILINEAR )
    {
        selectedItemIdx = itemIdx;
    }

    // Altered: Select filter mode from config. Original bilinear mode was always selected
    ComboBox_SetCurSel(hDlgItem, selectedItemIdx);

   // Added
   // Enable and init  HiPoly check button
    hDlgItem = GetDlgItem(hDlg, 1051);
    EnableWindow(hDlgItem, 1);
    ShowWindow(hDlgItem, 1);
    CheckDlgButton(hDlg, 1051, sithModel_IsHiPolyEnabled());

    // Setup Display related controls
    JonesMain_DevDialogInitDisplayDevices(hDlg, pConfig);

    // Populate level List box
    hDlgItem = GetDlgItem(hDlg, 1001);
    if ( hDlgItem )
    {
        char aNdyDir[128];
        STD_MAKEPATH(aNdyDir, JonesFile_GetWorkingDirPath(), "ndy");

        // Added
        if ( !stdUtil_DirExists(aNdyDir) )
        {
            STD_MAKEPATH(aNdyDir, JonesFile_GetResourcePath(), "ndy"); // Use absolute resource path to search for level files
        }

        tFoundFileInfo fileInfo;
        FindFileData* pFileData = stdFileUtil_NewFind(aNdyDir, 3, "ndy");
        while ( stdFileUtil_FindNext(pFileData, &fileInfo) )
        {
            itemIdx = ListBox_AddString(hDlgItem, fileInfo.aName);
        }
        stdFileUtil_DisposeFind(pFileData);

        pFileData = stdFileUtil_NewFind(aNdyDir, 3, "cnd");
        while ( stdFileUtil_FindNext(pFileData, &fileInfo) )
        {
            itemIdx = ListBox_AddString(hDlgItem, fileInfo.aName);
        }

        stdFileUtil_DisposeFind(pFileData);

        // Select cur level from config
        int numLevels = ListBox_GetCount(hDlgItem);
        for ( itemIdx = 0; itemIdx < numLevels; ++itemIdx )
        {
            char aLevelName[128] = { 0 }; // Fixed: Increased string len to 128 from 64
            ListBox_GetText(hDlgItem, itemIdx, aLevelName);
            if ( streq(aLevelName, pConfig->aCurLevelFilename) )
            {
                ListBox_SetCurSel(hDlgItem, itemIdx);
                break;
            }
        }

        if ( itemIdx == numLevels )
        {
            // None selected, select level in the middle of the list
            ListBox_SetCurSel(hDlgItem, numLevels / 2);
        }
    }

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pConfig); // Set config to dialog handle

    CheckDlgButton(hDlg, 1007, pConfig->bDevMode);// Dev mode

    JonesMain_DevDialogUpdateRadioButtons(hDlg, pConfig);
    return 1;
}

void J3DAPI JonesMain_DevDialogHandleCommand(HWND hWnd, int controlId, LPARAM lParam, int notifyCode)
{
    J3D_UNUSED(lParam);

    JonesState* pState = (JonesState*)GetWindowLongPtr(hWnd, DWL_USER);
    if ( controlId > 1001 )
    {
        switch ( controlId )
        {
            case 1013: // error output normal
                pState->outputMode = JONES_OUTPUTMODE_NONE;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1014: // error output console
                pState->outputMode = JONES_OUTPUTMODE_CONSOLE;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1015: // error output file
                pState->outputMode = JONES_OUTPUTMODE_LOGFILE;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1016: // debug output normal
                pState->logLevel = JONES_LOGLEVEL_NORMAL;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1017: // debug output verbose
                pState->logLevel = JONES_LOGLEVEL_VERBOSE;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1018: // debug output quiet
                pState->logLevel = JONES_LOGLEVEL_ERROR;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1029: // display mode
                if ( notifyCode == CBN_SELCHANGE )
                {
                    HWND hCBDisplayMode = GetDlgItem(hWnd, 1029);
                    int curSelIdx = ComboBox_GetCurSel(hCBDisplayMode);
                    pState->displaySettings.videoModeNum = ComboBox_GetItemData(hCBDisplayMode, curSelIdx);
                }
                break;

            case 1030: // Display settings
                if ( notifyCode == CBN_SELCHANGE )
                {
                    HWND CBDisplayDriver = GetDlgItem(hWnd, 1030);
                    int curSelIdx = ComboBox_GetCurSel(CBDisplayDriver);
                    pState->displaySettings.displayDeviceNum = ComboBox_GetItemData(CBDisplayDriver, curSelIdx);
                    JonesMain_DevDialogInitDisplayDevices(hWnd, pState);
                }
                break;

            case 1031: // 3D Driver
                if ( notifyCode == CBN_SELCHANGE )
                {
                    HWND hCB3DDriver = GetDlgItem(hWnd, 1031);
                    int curSelIdx = ComboBox_GetCurSel(hCB3DDriver);
                    pState->displaySettings.device3DNum = ComboBox_GetItemData(hCB3DDriver, curSelIdx);
                    JonesMain_DevDialogInitDisplayDevices(hWnd, pState);
                }
                break;

            case 1176: // performance level 0
                pState->performanceLevel = 0;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1177: // performance level 1
                pState->performanceLevel = 1;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1178: // performance level 2
                pState->performanceLevel = 2;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1179: // performance level 3
                pState->performanceLevel = 3;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1180: // performance level 4
                pState->performanceLevel = 4;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            default:
                return;
        }
    }
    else
    {
        switch ( controlId )
        {
            case 1001: // select level list
                if ( notifyCode != LBN_DBLCLK )
                {
                    return;
                }

                controlId = 1; // start game
                break;

            case 1: // start the game
                break;

            case 2: // exit
                EndDialog(hWnd, controlId);
                return;

            default:
                return;
        }

        // Start game logic

        // Get 3D driver
        HWND hCB3DDriver = GetDlgItem(hWnd, 1031);
        int curSelIdx = ComboBox_GetCurSel(hCB3DDriver);
        pState->displaySettings.device3DNum = ComboBox_GetItemData(hCB3DDriver, curSelIdx);

        // Get Display mode
        HWND hCBDisplayMode = GetDlgItem(hWnd, 1029);
        curSelIdx = ComboBox_GetCurSel(hCBDisplayMode);
        pState->displaySettings.videoModeNum = ComboBox_GetItemData(hCBDisplayMode, curSelIdx);

        // Geometry mode
        HWND hCBGeometryMode = GetDlgItem(hWnd, 1008);
        curSelIdx = ComboBox_GetCurSel(hCBGeometryMode);
        pState->displaySettings.geoMode = ComboBox_GetItemData(hCBGeometryMode, curSelIdx);

        if ( pState->displaySettings.geoMode == RD_GEOMETRY_WIREFRAME || pState->displaySettings.geoMode == RD_GEOMETRY_VERTEX )
        {
            pState->displaySettings.bClearBackBuffer = 1;
        }

        // Light mode
        HWND hCBLightMode = GetDlgItem(hWnd, 1009);
        curSelIdx = ComboBox_GetCurSel(hCBLightMode);
        pState->displaySettings.lightMode = ComboBox_GetItemData(hCBLightMode, curSelIdx);

        // Filter mode
        HWND hCBFilterMode = GetDlgItem(hWnd, 1012);
        curSelIdx = ComboBox_GetCurSel(hCBFilterMode);
        pState->displaySettings.filter = ComboBox_GetItemData(hCBFilterMode, curSelIdx);

        // Added: Enable/Disable HiPoly
        sithModel_EnableHiPoly(IsDlgButtonChecked(hWnd, 1051) == 1);

        pState->displaySettings.bWindowMode = IsDlgButtonChecked(hWnd, 1002) == 1; // window mode
        pState->bDevMode = IsDlgButtonChecked(hWnd, 1007) == 1;// devmode

        pState->displaySettings.width  = JonesMain_GetStartupDisplayEnv()->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].rasterInfo.width;
        pState->displaySettings.height = JonesMain_GetStartupDisplayEnv()->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].rasterInfo.height;

        // Get selected level & Save settings
        HWND hCBLevelList = GetDlgItem(hWnd, 1001);
        curSelIdx = ListBox_GetCurSel(hCBLevelList);
        if ( ListBox_GetText(hCBLevelList, curSelIdx, pState->aCurLevelFilename) != -1 )
        {
            stdConfig_SetString(JONESDISPLAY_CFG_GRAPHICS_DISPLAY, JonesMain_GetStartupDisplayEnv()->aDisplayInfos[pState->displaySettings.displayDeviceNum].displayDevice.aDriverName);
            stdConfig_SetString(JONESDISPLAY_CFG_GRAPHICS_DEVICE, JonesMain_GetStartupDisplayEnv()->aDisplayInfos[pState->displaySettings.displayDeviceNum].aDevices[pState->displaySettings.device3DNum].deviceDescription);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_WIDTH, pState->displaySettings.width);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_HEIGHT, pState->displaySettings.height);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_BPP, JonesMain_GetStartupDisplayEnv()->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].rasterInfo.colorInfo.bpp);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_REFRESHRATE, JonesMain_GetStartupDisplayEnv()->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].refreshRate);
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_MIPMAPFILTER, pState->displaySettings.filter);
            stdConfig_SetBool(JONESDISPLAY_CFG_ENGINE_RENDER_HIPOLY, sithModel_IsHiPolyEnabled()); // Added

            stdConfig_SetString(JONESCONFIG_CFG_GAMEPLAY_STARTLEVEL, pState->aCurLevelFilename);

            stdConfig_SetBool(JONESDISPLAY_CFG_GRAPHICS_WINDOW, pState->displaySettings.bWindowMode);
            stdConfig_SetBool(JONESDISPLAY_CFG_GRAPHICS_DUALMONITOR, pState->displaySettings.bDualMonitor);

            stdConfig_SetBool(JONESCONFIG_CFG_DEVMODE, pState->bDevMode);

            stdConfig_SetBool(JONESCONFIG_CFG_SOUND_HW, pState->soundSettings.b3DHWSupport);

            stdConfig_SetInt(JONESCONFIG_CFG_LOG_MODE, pState->outputMode);
            stdConfig_SetInt(JONESCONFIG_CFG_LOG_LEVEL, pState->logLevel);

            stdConfig_SetString(SITH_CFG_INSTALLPATH, pState->aInstallPath); // Changed: Changed setting key from 'User Path' to 'Install Path'
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_PERFORMANCELEVEL, pState->performanceLevel);

            stdConfig_SetInt(JONESDISPLAY_CFG_ENGINE_RENDER_GEOMETRYMODE, pState->displaySettings.geoMode);
            stdConfig_SetInt(JONESDISPLAY_CFG_ENGINE_RENDER_LIGHTINGMODE, pState->displaySettings.lightMode);

            EndDialog(hWnd, controlId); // Close dialog
        }
    }
}

void J3DAPI JonesMain_DevDialogInitDisplayDevices(HWND hDlg, JonesState* pConfig)
{
    HWND hBTNStart      = GetDlgItem(hDlg, 1);
    HWND hCBWindowMode  = GetDlgItem(hDlg, 1002);
    HWND hCB3DDriver    = GetDlgItem(hDlg, 1031);

    HWND hCBDisplayMode = GetDlgItem(hDlg, 1029);
    ComboBox_ResetContent(hCBDisplayMode);

    StdDisplayInfo* pDisplay = &JonesMain_GetStartupDisplayEnv()->aDisplayInfos[pConfig->displaySettings.displayDeviceNum];
    if ( !pDisplay->displayDevice.bHAL )
    {
        EnableWindow(hBTNStart, 0);
        EnableWindow(hCBWindowMode, 0);
        EnableWindow(hCBDisplayMode, 0);
        EnableWindow(hCB3DDriver, 0);
        return;
    }

    EnableWindow(hBTNStart, 1);
    EnableWindow(hCBWindowMode, pDisplay->displayDevice.bWindowRenderNotSupported == 0);

    if ( pDisplay->displayDevice.bWindowRenderNotSupported )
    {
        pConfig->displaySettings.bWindowMode = 0;
    }

    EnableWindow(hCBDisplayMode, 1);
    EnableWindow(hCB3DDriver, 1);
    CheckDlgButton(hDlg, 1002, pConfig->displaySettings.bWindowMode);// CB window mode

    // Init 3D driver combo box list
    bool bDriverSet = false;

    CHAR aDriverName[128] = { 0 };
    GetWindowText(hCB3DDriver, aDriverName, STD_ARRAYLEN(aDriverName));

    ComboBox_ResetContent(hCB3DDriver);
    for ( size_t deviceNum = 0; deviceNum < pDisplay->numDevices; ++deviceNum )
    {
        STD_FORMAT(std_g_genBuffer, "%s", pDisplay->aDevices[deviceNum].deviceDescription);

        int itemIdx = ComboBox_AddString(hCB3DDriver, std_g_genBuffer);
        ComboBox_SetItemData(hCB3DDriver, itemIdx, deviceNum);

        // Select item in cb list
        if ( streq(aDriverName, std_g_genBuffer) || pConfig->displaySettings.device3DNum == deviceNum )
        {
            ComboBox_SetCurSel(hCB3DDriver, itemIdx);
            pConfig->displaySettings.device3DNum = deviceNum;
            bDriverSet = true;
        }
    }

    if ( !bDriverSet )
    {
        // Select first element in list
        pConfig->displaySettings.device3DNum = 0;
        ComboBox_SetCurSel(hCB3DDriver, 0);
    }

    // Populate display mode combo box list (resolutions) and select 

    // (*JonesMain_GetCurVideoMode()).rasterInfo.colorInfo.bpp = 16; // Removed: This prevents getting stored 32 BPP resolution from config

    bDriverSet = false;
    for ( size_t modeNum = 0; modeNum < JonesMain_GetStartupDisplayEnv()->aDisplayInfos[pConfig->displaySettings.displayDeviceNum].numModes; ++modeNum )
    {
        if ( pDisplay->aModes[modeNum].aspectRatio == 1.0f
            && pDisplay->aModes[modeNum].rasterInfo.width >= 512
            && pDisplay->aModes[modeNum].rasterInfo.height >= 384 )
        {
            if ( JonesMain_CurDisplaySupportsBPP(&pConfig->displaySettings, pDisplay->aModes[modeNum].rasterInfo.colorInfo.bpp) )
            {
                 // Changed: Moved in this scope
                if ( pDisplay->aModes[modeNum].refreshRate )
                {
                    STD_FORMAT(std_g_genBuffer, "%dx%d %dbpp (%d Hz)", pDisplay->aModes[modeNum].rasterInfo.width, pDisplay->aModes[modeNum].rasterInfo.height, pDisplay->aModes[modeNum].rasterInfo.colorInfo.bpp, pDisplay->aModes[modeNum].refreshRate);  // Added: refresh rate fromat
                }
                else
                {
                    STD_FORMAT(std_g_genBuffer, "%dx%d %dbpp", pDisplay->aModes[modeNum].rasterInfo.width, pDisplay->aModes[modeNum].rasterInfo.height, pDisplay->aModes[modeNum].rasterInfo.colorInfo.bpp);
                }

                int itemIdx = ComboBox_AddString(hCBDisplayMode, std_g_genBuffer);
                ComboBox_SetItemData(hCBDisplayMode, itemIdx, modeNum);
                // Select mode
                if ( pDisplay->aModes[modeNum].rasterInfo.width == (*JonesMain_GetCurVideoMode()).rasterInfo.width
                    && pDisplay->aModes[modeNum].rasterInfo.height == (*JonesMain_GetCurVideoMode()).rasterInfo.height
                    && pDisplay->aModes[modeNum].rasterInfo.colorInfo.bpp == (*JonesMain_GetCurVideoMode()).rasterInfo.colorInfo.bpp
                    && (pDisplay->aModes[modeNum].refreshRate == 0 || pDisplay->aModes[modeNum].refreshRate == (*JonesMain_GetCurVideoMode()).refreshRate) )
                {
                    ComboBox_SetCurSel(hCBDisplayMode, itemIdx);
                    bDriverSet = true;
                }
            }
        }
    }

    if ( !bDriverSet )
    {
        ComboBox_SetCurSel(hCBDisplayMode, 0);
    }
}
