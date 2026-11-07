#include <unity_fixture.h>
#include <Jones3D/Display/jonesConfig.h>
#include <stdio.h>
#include <string.h>

#include <std/General/std.h>
#include <std/Win95/stdDisplay.h>
#include <commctrl.h>

// The layout tests supply desktop, DPI, font and GDI observations without
// changing the host's display mode or distributing any retail UI resources.
static BOOL WINAPI jonesConfigTest_GetClientRect(HWND hWnd, LPRECT pRect);
static BOOL WINAPI jonesConfigTest_GetWindowRect(HWND hWnd, LPRECT pRect);
static BOOL WINAPI jonesConfigTest_ClientToScreen(HWND hWnd, LPPOINT pPoint);
static LONG WINAPI jonesConfigTest_SetWindowLongA(HWND hWnd, int index, LONG value);
static BOOL WINAPI jonesConfigTest_MoveWindow(HWND hWnd, int x, int y, int width, int height, BOOL repaint);
static HWND WINAPI jonesConfigTest_GetDesktopWindow(void);
static UINT WINAPI jonesConfigTest_GetDpiForWindow(HWND hWnd);
static HMONITOR WINAPI jonesConfigTest_MonitorFromWindow(HWND hWnd, DWORD flags);
static BOOL WINAPI jonesConfigTest_GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO pInfo);
static BOOL WINAPI jonesConfigTest_EnumDisplaySettingsExA(LPCSTR pDevice, DWORD mode, DEVMODEA* pMode, DWORD flags);
static LRESULT WINAPI jonesConfigTest_SendMessageA(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static int WINAPI jonesConfigTest_GetObjectA(HGDIOBJ hObject, int size, LPVOID pObject);
static HFONT WINAPI jonesConfigTest_CreateFontIndirectA(const LOGFONTA* pFont);
static LONG WINAPI jonesConfigTest_GetWindowLongA(HWND hWnd, int index);
static HIMAGELIST WINAPI jonesConfigTest_ImageListCreate(int width, int height, UINT flags, int initial, int growth);
static HICON WINAPI jonesConfigTest_LoadIconA(HINSTANCE hInstance, LPCSTR pName);
static int WINAPI jonesConfigTest_ImageListReplaceIcon(HIMAGELIST hList, int index, HICON hIcon);
static HWND WINAPI jonesConfigTest_GetDlgItem(HWND hWnd, int id);
static HGDIOBJ WINAPI jonesConfigTest_SelectObject(HDC hDC, HGDIOBJ hObject);
static BOOL WINAPI jonesConfigTest_StretchBlt(HDC hDest, int x, int y, int width, int height,
    HDC hSource, int sourceX, int sourceY, int sourceWidth, int sourceHeight, DWORD operation);
static void J3DAPI jonesConfigTest_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight);

#define GetClientRect jonesConfigTest_GetClientRect
#define GetWindowRect jonesConfigTest_GetWindowRect
#define ClientToScreen jonesConfigTest_ClientToScreen
#define SetWindowLongA jonesConfigTest_SetWindowLongA
#define MoveWindow jonesConfigTest_MoveWindow
#define GetDesktopWindow jonesConfigTest_GetDesktopWindow
#define GetDpiForWindow jonesConfigTest_GetDpiForWindow
#define MonitorFromWindow jonesConfigTest_MonitorFromWindow
#define GetMonitorInfoA jonesConfigTest_GetMonitorInfoA
#define EnumDisplaySettingsExA jonesConfigTest_EnumDisplaySettingsExA
#define SendMessageA jonesConfigTest_SendMessageA
#define GetObjectA jonesConfigTest_GetObjectA
#define CreateFontIndirectA jonesConfigTest_CreateFontIndirectA
#define GetWindowLongA jonesConfigTest_GetWindowLongA
#define ImageList_Create jonesConfigTest_ImageListCreate
#define LoadIconA jonesConfigTest_LoadIconA
#define ImageList_ReplaceIcon jonesConfigTest_ImageListReplaceIcon
#define GetDlgItem jonesConfigTest_GetDlgItem
#define SelectObject jonesConfigTest_SelectObject
#define StretchBlt jonesConfigTest_StretchBlt
#define stdDisplay_GetBackBufferSize jonesConfigTest_GetBackBufferSize
#undef STDLOG_STATUS
#undef STDLOG_ERROR
#define STDLOG_STATUS(...) ((void)0)
#define STDLOG_ERROR(...) ((void)0)

// Compile the actual module so its private action/key tables drive synthetic
// input. Only file, localization and window dependencies are replaced.
static StdConffileEntry jonesConfigTest_entry;
static int J3DAPI jonesConfigTest_Open(const char* pFilename);
static void jonesConfigTest_Close(void);
static int jonesConfigTest_ReadArgs(void);
static int WINAPI jonesConfigTest_GetKeyNameTextA(LONG lParam, LPSTR pString, int size);

#undef stdConffile_g_entry
#define stdConffile_g_entry jonesConfigTest_entry
#define stdConffile_Open jonesConfigTest_Open
#define stdConffile_Close jonesConfigTest_Close
#define stdConffile_ReadArgs jonesConfigTest_ReadArgs
#define GetKeyNameTextA jonesConfigTest_GetKeyNameTextA

#include "../jonesConfig.c"

static unsigned jonesConfigTest_numOpens;
static unsigned jonesConfigTest_numCloses;
static unsigned jonesConfigTest_readIndex;
static int jonesConfigTest_failOpen;
static int jonesConfigTest_failRead;
static unsigned jonesConfigTest_actionIndex;
static unsigned jonesConfigTest_numBindings;
static unsigned jonesConfigTest_numActionLines;
static int jonesConfigTest_keyNameFails;
static unsigned jonesConfigTest_numKeyNames;
static LONG jonesConfigTest_keyNameParam;
static JonesControlsScheme* jonesConfigTest_pScheme;
static JonesControlsScheme jonesConfigTest_overflowSnapshot;
static int jonesConfigTest_sawOverflow;


#define JONESCONFIGTEST_DIALOG ((HWND)(uintptr_t)0x1000)
#define JONESCONFIGTEST_CONTROL ((HWND)(uintptr_t)0x1001)
#define JONESCONFIGTEST_DESKTOP ((HWND)(uintptr_t)0x1002)
#define JONESCONFIGTEST_LIST ((HWND)(uintptr_t)0x1003)
#define JONESCONFIGTEST_OLD_FONT ((HFONT)(uintptr_t)0x2000)
#define JONESCONFIGTEST_NEW_FONT ((HFONT)(uintptr_t)0x2001)
#define JONESCONFIGTEST_IMAGE ((HBITMAP)(uintptr_t)0x3000)
#define JONESCONFIGTEST_MASK ((HBITMAP)(uintptr_t)0x3001)
#define JONESCONFIGTEST_INSTANCE ((HINSTANCE)(uintptr_t)0x4000)
#define JONESCONFIGTEST_IMAGE_LIST ((HIMAGELIST)(uintptr_t)0x5000)
#define JONESCONFIGTEST_MONITOR ((HMONITOR)(uintptr_t)0x6000)
#define JONESCONFIGTEST_SOURCE_DC ((HDC)(uintptr_t)0x7000)
#define JONESCONFIGTEST_DEST_DC ((HDC)(uintptr_t)0x7001)

typedef struct sJonesConfigTestStretch
{
    HDC hDest;
    int x, y, width, height;
    HDC hSource;
    int sourceX, sourceY, sourceWidth, sourceHeight;
    DWORD operation;
    HGDIOBJ hSelected;
} JonesConfigTestStretch;

static int jonesConfigTest_layoutEnabled;
static int jonesConfigTest_hasStarted;
static int jonesConfigTest_missingSettings;
static JonesDisplaySettings jonesConfigTest_settings;
static uint32_t jonesConfigTest_renderWidth;
static uint32_t jonesConfigTest_renderHeight;
static DWORD jonesConfigTest_desktopWidth;
static DWORD jonesConfigTest_desktopHeight;
static UINT jonesConfigTest_dpi;
static UINT jonesConfigTest_desktopDpi;
static UINT jonesConfigTest_listDpi;
static LONG jonesConfigTest_dialogHeight;
static LONG jonesConfigTest_listWidth;
static int jonesConfigTest_listClientFails;
static RECT jonesConfigTest_controlRect;
static int jonesConfigTest_desktopFailure;
static unsigned jonesConfigTest_monitorQueries;
static unsigned jonesConfigTest_modeQueries;
static int jonesConfigTest_missingFont;
static LOGFONTA jonesConfigTest_createdFont;
static unsigned jonesConfigTest_numFonts;
static int jonesConfigTest_imageListFails;
static int jonesConfigTest_iconWidth;
static int jonesConfigTest_iconHeight;
static unsigned jonesConfigTest_numIconLoads;
static unsigned jonesConfigTest_numIconCopies;
static int jonesConfigTest_charWidth;
static int jonesConfigTest_missingHeader;
static const char* jonesConfigTest_costHeader;
static LVCOLUMNA jonesConfigTest_columns[2];
static int jonesConfigTest_columnPresent[2];
static unsigned jonesConfigTest_numColumnInserts;
static unsigned jonesConfigTest_numColumnUpdates;
static unsigned jonesConfigTest_numPriceMeasurements;
static unsigned jonesConfigTest_numCountMeasurements;
static HGDIOBJ jonesConfigTest_selectedObject;
static JonesConfigTestStretch jonesConfigTest_stretches[2];
static unsigned jonesConfigTest_numStretches;
static int jonesConfigTest_stretchResult;
static LONG jonesConfigTest_imageStyle;
static int jonesConfigTest_imageX, jonesConfigTest_imageY;
static int jonesConfigTest_imageWidth, jonesConfigTest_imageHeight;
static unsigned jonesConfigTest_numImageMoves;

static void jonesConfigTest_ResetLayout(void)
{
    jonesConfigTest_layoutEnabled = 1;
    jonesConfigTest_hasStarted = 1;
    jonesConfigTest_missingSettings = 0;
    memset(&jonesConfigTest_settings, 0, sizeof(jonesConfigTest_settings));
    jonesConfigTest_renderWidth = jonesConfigTest_desktopWidth = 3840;
    jonesConfigTest_renderHeight = jonesConfigTest_desktopHeight = 2160;
    jonesConfigTest_dpi = jonesConfigTest_desktopDpi = jonesConfigTest_listDpi = 144;
    jonesConfigTest_dialogHeight = 154;
    jonesConfigTest_listWidth = 500;
    jonesConfigTest_listClientFails = 0;
    jonesConfigTest_controlRect = (RECT){ 0, 0, 48, 48 };
    jonesConfigTest_desktopFailure = 0;
    jonesConfigTest_monitorQueries = jonesConfigTest_modeQueries = 0;
    jonesConfigTest_missingFont = 0;
    memset(&jonesConfigTest_createdFont, 0, sizeof(jonesConfigTest_createdFont));
    jonesConfigTest_numFonts = 0;
    jonesConfigTest_imageListFails = 0;
    jonesConfigTest_iconWidth = jonesConfigTest_iconHeight = 0;
    jonesConfigTest_numIconLoads = jonesConfigTest_numIconCopies = 0;
    jonesConfigTest_charWidth = 8;
    jonesConfigTest_missingHeader = 0;
    jonesConfigTest_costHeader = NULL;
    memset(jonesConfigTest_columns, 0, sizeof(jonesConfigTest_columns));
    memset(jonesConfigTest_columnPresent, 0, sizeof(jonesConfigTest_columnPresent));
    jonesConfigTest_numColumnInserts = jonesConfigTest_numColumnUpdates = 0;
    jonesConfigTest_numPriceMeasurements = jonesConfigTest_numCountMeasurements = 0;
    jonesConfigTest_selectedObject = NULL;
    memset(jonesConfigTest_stretches, 0, sizeof(jonesConfigTest_stretches));
    jonesConfigTest_numStretches = 0;
    jonesConfigTest_stretchResult = 1;
    jonesConfigTest_imageStyle = SS_NOTIFY | SS_ICON;
    jonesConfigTest_imageX = jonesConfigTest_imageY = 0;
    jonesConfigTest_imageWidth = jonesConfigTest_imageHeight = 0;
    jonesConfigTest_numImageMoves = 0;
}

static BOOL WINAPI jonesConfigTest_GetClientRect(HWND hWnd, LPRECT pRect)
{
    if ( hWnd == JONESCONFIGTEST_DIALOG )
    {
        *pRect = (RECT){ 0, 0, 399, jonesConfigTest_dialogHeight };
    }
    else if ( hWnd == JONESCONFIGTEST_LIST )
    {
        if ( jonesConfigTest_listClientFails ) { return FALSE; }
        // This is already the available client viewport: the scrollbar is not
        // included. Repeated column layout must not deduct it a second time.
        *pRect = (RECT){ 0, 0, jonesConfigTest_listWidth, 200 };
    }
    else
    {
        TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_CONTROL, hWnd);
        *pRect = jonesConfigTest_controlRect;
    }
    return TRUE;
}

static BOOL WINAPI jonesConfigTest_GetWindowRect(HWND hWnd, LPRECT pRect)
{
    if ( hWnd == JONESCONFIGTEST_CONTROL )
    {
        *pRect = (RECT){ 130, 240, 194, 304 };
    }
    else
    {
        TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_DESKTOP, hWnd);
        *pRect = (RECT){ 0, 0, (LONG)jonesConfigTest_desktopWidth, (LONG)jonesConfigTest_desktopHeight };
    }
    return TRUE;
}

static BOOL WINAPI jonesConfigTest_ClientToScreen(HWND hWnd, LPPOINT pPoint)
{
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_DIALOG, hWnd);
    pPoint->x += 100;
    pPoint->y += 200;
    return TRUE;
}

static LONG WINAPI jonesConfigTest_SetWindowLongA(HWND hWnd, int index, LONG value)
{
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_CONTROL, hWnd);
    TEST_ASSERT_EQUAL_INT(GWL_STYLE, index);
    LONG previous = jonesConfigTest_imageStyle;
    jonesConfigTest_imageStyle = value;
    return previous;
}

static BOOL WINAPI jonesConfigTest_MoveWindow(HWND hWnd, int x, int y, int width, int height, BOOL repaint)
{
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_CONTROL, hWnd);
    TEST_ASSERT_TRUE(repaint);
    jonesConfigTest_imageX = x;
    jonesConfigTest_imageY = y;
    jonesConfigTest_imageWidth = width;
    jonesConfigTest_imageHeight = height;
    ++jonesConfigTest_numImageMoves;
    return TRUE;
}

static HWND WINAPI jonesConfigTest_GetDesktopWindow(void)
{
    return JONESCONFIGTEST_DESKTOP;
}

static UINT WINAPI jonesConfigTest_GetDpiForWindow(HWND hWnd)
{
    if ( hWnd == JONESCONFIGTEST_LIST ) { return jonesConfigTest_listDpi; }
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_DIALOG, hWnd);
    return jonesConfigTest_dpi;
}

static HMONITOR WINAPI jonesConfigTest_MonitorFromWindow(HWND hWnd, DWORD flags)
{
    J3D_UNUSED(hWnd);
    TEST_ASSERT_EQUAL_UINT32(MONITOR_DEFAULTTONEAREST, flags);
    ++jonesConfigTest_monitorQueries;
    return jonesConfigTest_desktopFailure == 1 ? NULL : JONESCONFIGTEST_MONITOR;
}

static BOOL WINAPI jonesConfigTest_GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO pInfo)
{
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_MONITOR, hMonitor);
    TEST_ASSERT_EQUAL_UINT(sizeof(MONITORINFOEXA), pInfo->cbSize);
    if ( jonesConfigTest_desktopFailure == 2 )
    {
        return FALSE;
    }
    MONITORINFOEXA* pExtended = (MONITORINFOEXA*)pInfo;
    pExtended->rcMonitor = (RECT){ -3840, 0, 0, 2160 };
    STD_STRCPY(pExtended->szDevice, "\\\\.\\DISPLAY2");
    return TRUE;
}

static BOOL WINAPI jonesConfigTest_EnumDisplaySettingsExA(LPCSTR pDevice, DWORD mode, DEVMODEA* pMode, DWORD flags)
{
    TEST_ASSERT_EQUAL_STRING("\\\\.\\DISPLAY2", pDevice);
    TEST_ASSERT_EQUAL_UINT32(ENUM_REGISTRY_SETTINGS, mode);
    TEST_ASSERT_EQUAL_UINT(sizeof(DEVMODEA), pMode->dmSize);
    TEST_ASSERT_EQUAL_UINT32(0, flags);
    ++jonesConfigTest_modeQueries;
    if ( jonesConfigTest_desktopFailure == 3 )
    {
        return FALSE;
    }
    pMode->dmPelsWidth = jonesConfigTest_desktopWidth;
    pMode->dmPelsHeight = jonesConfigTest_desktopHeight;
    return TRUE;
}

static LRESULT WINAPI jonesConfigTest_SendMessageA(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if ( message == WM_GETFONT )
    {
        return (LRESULT)JONESCONFIGTEST_OLD_FONT;
    }
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_LIST, hWnd);
    if ( message == LVM_GETSTRINGWIDTHA )
    {
        const char* pText = (const char*)lParam;
        TEST_ASSERT_NOT_NULL(pText);
        if ( strcmp(pText, "2500") == 0 ) { ++jonesConfigTest_numPriceMeasurements; }
        if ( strcmp(pText, "4095") == 0 ) { ++jonesConfigTest_numCountMeasurements; }
        return (LRESULT)(strlen(pText) * jonesConfigTest_charWidth);
    }
    TEST_ASSERT_LESS_THAN_UINT(2, (unsigned)wParam);
    if ( message == LVM_SETCOLUMNA )
    {
        ++jonesConfigTest_numColumnUpdates;
        if ( !jonesConfigTest_columnPresent[wParam] )
        {
            return FALSE;
        }
        jonesConfigTest_columns[wParam] = *(const LVCOLUMNA*)lParam;
        return TRUE;
    }
    TEST_ASSERT_EQUAL_UINT32(LVM_INSERTCOLUMNA, message);
    TEST_ASSERT_FALSE(jonesConfigTest_columnPresent[wParam]);
    jonesConfigTest_columns[wParam] = *(const LVCOLUMNA*)lParam;
    jonesConfigTest_columnPresent[wParam] = 1;
    ++jonesConfigTest_numColumnInserts;
    return (LRESULT)wParam;
}

static int WINAPI jonesConfigTest_GetObjectA(HGDIOBJ hObject, int size, LPVOID pObject)
{
    if ( hObject == (HGDIOBJ)JONESCONFIGTEST_OLD_FONT )
    {
        TEST_ASSERT_EQUAL_INT(sizeof(LOGFONTA), size);
        memset(pObject, 0, size);
        ((LOGFONTA*)pObject)->lfHeight = -16;
        return size;
    }
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_IMAGE, hObject);
    TEST_ASSERT_EQUAL_INT(sizeof(BITMAP), size);
    memset(pObject, 0, size);
    ((BITMAP*)pObject)->bmWidth = 64;
    ((BITMAP*)pObject)->bmHeight = 64;
    return size;
}

static HFONT WINAPI jonesConfigTest_CreateFontIndirectA(const LOGFONTA* pFont)
{
    jonesConfigTest_createdFont = *pFont;
    ++jonesConfigTest_numFonts;
    return JONESCONFIGTEST_NEW_FONT;
}

static LONG WINAPI jonesConfigTest_GetWindowLongA(HWND hWnd, int index)
{
    if ( hWnd == JONESCONFIGTEST_CONTROL )
    {
        TEST_ASSERT_EQUAL_INT(GWL_STYLE, index);
        return jonesConfigTest_imageStyle;
    }
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_DIALOG, hWnd);
    TEST_ASSERT_EQUAL_INT(GWL_HINSTANCE, index);
    return (LONG)(uintptr_t)JONESCONFIGTEST_INSTANCE;
}

static HIMAGELIST WINAPI jonesConfigTest_ImageListCreate(int width, int height, UINT flags, int initial, int growth)
{
    jonesConfigTest_iconWidth = width;
    jonesConfigTest_iconHeight = height;
    TEST_ASSERT_EQUAL_UINT32(ILC_MASK | ILC_COLOR8, flags);
    TEST_ASSERT_EQUAL_INT(14, initial);
    TEST_ASSERT_EQUAL_INT(0, growth);
    return jonesConfigTest_imageListFails ? NULL : JONESCONFIGTEST_IMAGE_LIST;
}

static HICON WINAPI jonesConfigTest_LoadIconA(HINSTANCE hInstance, LPCSTR pName)
{
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_INSTANCE, hInstance);
    TEST_ASSERT_TRUE(IS_INTRESOURCE(pName));
    ++jonesConfigTest_numIconLoads;
    return (HICON)(uintptr_t)(0x8000 + (uintptr_t)pName);
}

static int WINAPI jonesConfigTest_ImageListReplaceIcon(HIMAGELIST hList, int index, HICON hIcon)
{
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_IMAGE_LIST, hList);
    TEST_ASSERT_EQUAL_INT(-1, index);
    TEST_ASSERT_NOT_NULL(hIcon);
    return (int)jonesConfigTest_numIconCopies++;
}

static HWND WINAPI jonesConfigTest_GetDlgItem(HWND hWnd, int id)
{
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_DIALOG, hWnd);
    TEST_ASSERT_EQUAL_INT(1182, id);
    return JONESCONFIGTEST_CONTROL;
}

static HGDIOBJ WINAPI jonesConfigTest_SelectObject(HDC hDC, HGDIOBJ hObject)
{
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_SOURCE_DC, hDC);
    jonesConfigTest_selectedObject = hObject;
    return NULL;
}

static BOOL WINAPI jonesConfigTest_StretchBlt(HDC hDest, int x, int y, int width, int height,
    HDC hSource, int sourceX, int sourceY, int sourceWidth, int sourceHeight, DWORD operation)
{
    TEST_ASSERT_LESS_THAN_UINT(2, jonesConfigTest_numStretches);
    JonesConfigTestStretch* pCall = &jonesConfigTest_stretches[jonesConfigTest_numStretches++];
    *pCall = (JonesConfigTestStretch){ hDest, x, y, width, height, hSource,
        sourceX, sourceY, sourceWidth, sourceHeight, operation, jonesConfigTest_selectedObject };
    return jonesConfigTest_stretchResult;
}

static void J3DAPI jonesConfigTest_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight)
{
    *pWidth = jonesConfigTest_renderWidth;
    *pHeight = jonesConfigTest_renderHeight;
}

static const char* const jonesConfigTest_aKeys[9] = { "A", "S", "D", "F", "G", "H", "J", "K", "L" };
static const size_t jonesConfigTest_aKeyIds[8] = { DIK_A, DIK_S, DIK_D, DIK_F, DIK_G, DIK_H, DIK_J, DIK_K };

static int J3DAPI jonesConfigTest_Open(const char* pFilename)
{
    TEST_ASSERT_EQUAL_STRING("keys.kfg", pFilename);
    ++jonesConfigTest_numOpens;
    return !jonesConfigTest_failOpen;
}

static void jonesConfigTest_Close(void)
{
    ++jonesConfigTest_numCloses;
}

static int jonesConfigTest_ReadArgs(void)
{
    unsigned index = jonesConfigTest_readIndex++;
    if ( (int)index == jonesConfigTest_failRead )
    {
        return 0;
    }
    memset(&jonesConfigTest_entry, 0, sizeof(jonesConfigTest_entry));
    if ( index == 1 )
    {
        jonesConfigTest_entry.numArgs = 1;
        jonesConfigTest_entry.aArgs[0].argName = "1";
    }
    else if ( index >= 3 )
    {
        unsigned row = index - 3;
        if ( row >= jonesConfigTest_numActionLines )
        {
            return 0;
        }
        // Put the selected action first, then every other action exactly once.
        unsigned action = row == 0 ? jonesConfigTest_actionIndex : row - 1;
        if ( row != 0 && action >= jonesConfigTest_actionIndex )
        {
            ++action;
        }
        jonesConfigTest_entry.numArgs = 1 + (row == 0 ? jonesConfigTest_numBindings : 0);
        jonesConfigTest_entry.aArgs[0].argName = jonesConfig_aJonesControlActionNames[action];
        for ( unsigned i = 0; i + 1 < jonesConfigTest_entry.numArgs; ++i )
        {
            jonesConfigTest_entry.aArgs[i + 1].argName = jonesConfigTest_aKeys[i];
        }
    }
    return 1;
}

const char* J3DAPI jonesString_GetString(const char* pKey)
{
    if ( jonesConfigTest_layoutEnabled )
    {
        if ( strcmp(pKey, "JONES_STR_FONT") == 0 )
        {
            return jonesConfigTest_missingFont ? NULL : "Synthetic Test Font";
        }
        if ( strcmp(pKey, "JONES_STR_ITEMNAME") == 0 ) { return "Item"; }
        if ( strcmp(pKey, "JONES_STR_COST") == 0 ) { return jonesConfigTest_missingHeader ? NULL : (jonesConfigTest_costHeader ? jonesConfigTest_costHeader : "Unit price"); }
        if ( strcmp(pKey, "JONES_STR_COUNT") == 0 ) { return jonesConfigTest_missingHeader ? NULL : "Quantity"; }
    }
    // Observe the ninth binding before error cleanup could hide a bad store to
    // the following row or to the scheme name after the final action.
    if ( jonesConfigTest_pScheme
         && strcmp(pKey, jonesConfig_aJonesCapControlActionNames[jonesConfigTest_actionIndex]) == 0 )
    {
        jonesConfigTest_overflowSnapshot = *jonesConfigTest_pScheme;
        jonesConfigTest_sawOverflow = 1;
    }
    return NULL; // the parser accepts the raw table names as fallback tokens
}

static int WINAPI jonesConfigTest_GetKeyNameTextA(LONG lParam, LPSTR pString, int size)
{
    ++jonesConfigTest_numKeyNames;
    jonesConfigTest_keyNameParam = lParam;
    if ( jonesConfigTest_keyNameFails )
    {
        return 0; // failure leaves the caller's buffer untouched
    }
    TEST_ASSERT_GREATER_THAN_INT(1, size);
    pString[0] = 'q';
    pString[1] = 0;
    return 1;
}

// Localization returns NULL on parser errors, so a real modal dialog must never
// be entered. These application dependencies do not create windows or devices.
int J3DAPI JonesDialog_ShowDialog(LPCSTR pTemplate, HWND hWnd, DLGPROC pfnDialog, LPARAM data)
{
    J3D_UNUSED(pTemplate); J3D_UNUSED(hWnd); J3D_UNUSED(pfnDialog); J3D_UNUSED(data);
    TEST_FAIL_MESSAGE("Unexpected modal dialog");
}

int JonesMain_HasStarted(void)
{
    return jonesConfigTest_hasStarted;
}

JonesDisplaySettings* JonesMain_GetDisplaySettings(void)
{
    return jonesConfigTest_missingSettings ? NULL : &jonesConfigTest_settings;
}

UINT JonesDisplay_GetDesktopDpi(void)
{
    return jonesConfigTest_desktopDpi;
}

void J3DAPI JonesHud_BindActivateControlKeys(const size_t* pKeys, size_t numKeys)
{
    J3D_UNUSED(pKeys); J3D_UNUSED(numKeys);
    TEST_FAIL_MESSAGE("Unexpected HUD binding");
}

void J3DAPI JonesMain_BindToggleMenuControlKeys(const size_t* pKeys, size_t numKeys)
{
    J3D_UNUSED(pKeys); J3D_UNUSED(numKeys);
    TEST_FAIL_MESSAGE("Unexpected menu binding");
}

void JonesControl_EnableJoystickAxes(void)
{
    TEST_FAIL_MESSAGE("Unexpected joystick setup");
}

void J3DAPI JonesReticle_Enable(bool bEnabled)
{
    J3D_UNUSED(bEnabled);
    TEST_FAIL_MESSAGE("Unexpected reticle update");
}

bool JonesReticle_IsEnabled(void)
{
    TEST_FAIL_MESSAGE("Unexpected reticle query");
}

BOOL J3DAPI JonesDialog_ShowFileSelectDialog(LPOPENFILENAMEA pData, int bOpen)
{
    J3D_UNUSED(pData); J3D_UNUSED(bOpen);
    TEST_FAIL_MESSAGE("Unexpected file dialog");
}

int JonesFile_GetCurrentCDNum(void)
{
    TEST_FAIL_MESSAGE("Unexpected CD query");
}

size_t JonesMain_GetCurrentLevelNum(void)
{
    TEST_FAIL_MESSAGE("Unexpected level query");
}

size_t jonesInventory_GetMaxDifficultyLevel(void)
{
    TEST_FAIL_MESSAGE("Unexpected difficulty query");
}

int jonesInventory_GetTotalIQPoints(void)
{
    TEST_FAIL_MESSAGE("Unexpected IQ query");
}

TEST_GROUP(jonesConfig);

TEST_SETUP(jonesConfig)
{
    jonesConfigTest_layoutEnabled = jonesConfigTest_hasStarted = jonesConfigTest_missingSettings = 0;
    memset(&jonesConfigTest_settings, 0, sizeof(jonesConfigTest_settings));
    jonesConfigTest_numOpens = jonesConfigTest_numCloses = jonesConfigTest_readIndex = 0;
    jonesConfigTest_failOpen = 0;
    jonesConfigTest_failRead = -1;
    jonesConfigTest_actionIndex = jonesConfigTest_numBindings = 0;
    jonesConfigTest_numActionLines = JONESCONTROL_ACTION_NUMACTIONS;
    jonesConfigTest_keyNameFails = 0;
    jonesConfigTest_numKeyNames = 0;
    jonesConfigTest_keyNameParam = 0;
    jonesConfigTest_pScheme = NULL;
    jonesConfigTest_sawOverflow = 0;
    memset(&jonesConfigTest_entry, 0, sizeof(jonesConfigTest_entry));
    memset(&jonesConfigTest_overflowSnapshot, 0, sizeof(jonesConfigTest_overflowSnapshot));
}

TEST_TEAR_DOWN(jonesConfig)
{
    jonesConfigTest_pScheme = NULL;
    jonesConfigTest_layoutEnabled = jonesConfigTest_hasStarted = jonesConfigTest_missingSettings = 0;
}

TEST(jonesConfig, EightBindingsFitTheActionRow)
{
    JonesControlsScheme scheme;
    memset(&scheme, 0xA5, sizeof(scheme));
    jonesConfigTest_numBindings = 8;
    TEST_ASSERT_EQUAL_INT(1, jonesConfig_LoadControlScheme("keys.kfg", &scheme));
    TEST_ASSERT_EQUAL_STRING("keys", scheme.aName);
    TEST_ASSERT_EQUAL_UINT32(0x0808, scheme.aActions[0][0]);
    TEST_ASSERT_EQUAL_MEMORY(jonesConfigTest_aKeyIds, &scheme.aActions[0][1], sizeof(jonesConfigTest_aKeyIds));
    for ( size_t i = 1; i < STD_ARRAYLEN(scheme.aActions); ++i )
    {
        TEST_ASSERT_EQUAL_UINT32(0, scheme.aActions[i][0]);
    }
    TEST_ASSERT_EQUAL_UINT(1, jonesConfigTest_numOpens);
    TEST_ASSERT_EQUAL_UINT(1, jonesConfigTest_numCloses);
}

TEST(jonesConfig, NinthBindingDoesNotOverwriteNextRowOrName)
{
    static const unsigned aRows[] = { 0, JONESCONTROL_ACTION_NUMACTIONS - 1 };
    for ( size_t i = 0; i < STD_ARRAYLEN(aRows); ++i )
    {
        JonesControlsScheme scheme;
        memset(&scheme, 0xA5, sizeof(scheme));
        jonesConfigTest_actionIndex = aRows[i];
        jonesConfigTest_numBindings = 9;
        jonesConfigTest_pScheme = &scheme;
        jonesConfigTest_readIndex = 0;
        jonesConfigTest_sawOverflow = 0;
        TEST_ASSERT_EQUAL_INT(0, jonesConfig_LoadControlScheme("keys.kfg", &scheme));
        TEST_ASSERT_TRUE(jonesConfigTest_sawOverflow);
        TEST_ASSERT_EQUAL_UINT32(0x0909, jonesConfigTest_overflowSnapshot.aActions[aRows[i]][0]);
        TEST_ASSERT_EQUAL_MEMORY(jonesConfigTest_aKeyIds,
            &jonesConfigTest_overflowSnapshot.aActions[aRows[i]][1], sizeof(jonesConfigTest_aKeyIds));
        if ( aRows[i] + 1 < JONESCONTROL_ACTION_NUMACTIONS )
        {
            TEST_ASSERT_EQUAL_UINT32(0xA5A5A5A5u, jonesConfigTest_overflowSnapshot.aActions[aRows[i] + 1][0]);
        }
        TEST_ASSERT_EQUAL_STRING("keys", jonesConfigTest_overflowSnapshot.aName);
        TEST_ASSERT_EQUAL_STRING("", scheme.aName);
    }
}

TEST(jonesConfig, FailedLoadClearsActionsAccordingToProfile)
{
    JonesControlsScheme scheme;
    memset(&scheme, 0xA5, sizeof(scheme));
    scheme.bModified = 123;
    // Reordered input modifies a late row, then fails on the following read.
    jonesConfigTest_actionIndex = JONESCONTROL_ACTION_NUMACTIONS - 1;
    jonesConfigTest_numBindings = 1;
    jonesConfigTest_failRead = 4;
    TEST_ASSERT_EQUAL_INT(0, jonesConfig_LoadControlScheme("keys.kfg", &scheme));
    TEST_ASSERT_EQUAL_INT(123, scheme.bModified);
    TEST_ASSERT_EQUAL_STRING("", scheme.aName);
#ifdef J3D_QOL_IMPROVEMENTS
    const unsigned char* pActions = (const unsigned char*)scheme.aActions;
    for ( size_t i = 0; i < sizeof(scheme.aActions); ++i )
    {
        TEST_ASSERT_EQUAL_UINT8(0, pActions[i]);
    }
#else
    // Legacy clears nine bytes in each of bindNum (two) rows, even though the
    // parsed action was the final row. Preserve that original behavior.
    for ( size_t row = 0; row < 2; ++row )
    {
        const unsigned char* p = (const unsigned char*)scheme.aActions[row];
        for ( size_t i = 0; i < sizeof(scheme.aActions[row]); ++i )
        {
            TEST_ASSERT_EQUAL_UINT8(i < 9 ? 0 : 0xA5, p[i]);
        }
    }
    TEST_ASSERT_EQUAL_UINT32(0xA5A5A5A5u, scheme.aActions[2][0]);
    TEST_ASSERT_EQUAL_UINT32(0x0101, scheme.aActions[JONESCONTROL_ACTION_NUMACTIONS - 1][0]);
    TEST_ASSERT_EQUAL_UINT32(DIK_A, scheme.aActions[JONESCONTROL_ACTION_NUMACTIONS - 1][1]);
#endif
    TEST_ASSERT_EQUAL_UINT(1, jonesConfigTest_numCloses);
}

TEST(jonesConfig, OpenFailurePreservesScheme)
{
    JonesControlsScheme scheme, saved;
    memset(&scheme, 0xA5, sizeof(scheme));
    saved = scheme;
    jonesConfigTest_failOpen = 1;
    TEST_ASSERT_EQUAL_INT(0, jonesConfig_LoadControlScheme("keys.kfg", &scheme));
    TEST_ASSERT_EQUAL_MEMORY(&saved, &scheme, sizeof(scheme));
    TEST_ASSERT_EQUAL_UINT(0, jonesConfigTest_numCloses);
    TEST_ASSERT_EQUAL_UINT(0, jonesConfigTest_readIndex);
}

TEST(jonesConfig, KeyNameFailureReturnsAnEmptyTerminatedString)
{
    struct { unsigned char before[4]; char name[256]; unsigned char after[4]; } output;
    static const unsigned char aGuard[4] = { 0xA5, 0xA5, 0xA5, 0xA5 };
    memset(&output, 0xA5, sizeof(output));
    jonesConfigTest_keyNameFails = 1;
    jonesConfig_ControlToString(DIK_Q, output.name);
    TEST_ASSERT_EQUAL_STRING("", output.name);
    // Bytes after the terminator are not string output (the debug CRT may fill
    // them), but neither bound of the destination may be written past.
    TEST_ASSERT_EQUAL_MEMORY(aGuard, output.before, sizeof(aGuard));
    TEST_ASSERT_EQUAL_MEMORY(aGuard, output.after, sizeof(aGuard));
    TEST_ASSERT_EQUAL_UINT(1, jonesConfigTest_numKeyNames);
    TEST_ASSERT_EQUAL_UINT32(DIK_Q << 16, jonesConfigTest_keyNameParam);
}

TEST(jonesConfig, KeyNameSuccessUsesTheReturnedName)
{
    char aName[256];
    memset(aName, 0xA5, sizeof(aName));
    jonesConfig_ControlToString(DIK_Q, aName);
    TEST_ASSERT_EQUAL_STRING("Q", aName);
    TEST_ASSERT_EQUAL_UINT(1, jonesConfigTest_numKeyNames);
}


typedef struct sJonesConfigTestResolution
{
    uint32_t width, height;
    float ratio;
    int iconSize;
    int fontHeight[3];
} JonesConfigTestResolution;

// Fixed expected results cover the reported modes and smaller 4:3/16:9 modes.
// These are proportions of the configured 4K desktop, not the output mode that
// an exclusive fullscreen switch temporarily reports to Windows.
static const JonesConfigTestResolution jonesConfigTest_aResolutions[] =
{
    { 3840, 2160, 1.0f,        48, { -10, -23, -40 } },
    { 1920, 1080, 0.5f,        24, {  -5, -12, -20 } },
    { 2048, 1536, 0.53333333f, 26, {  -5, -12, -21 } },
    { 2560, 1440, 0.66666667f, 32, {  -7, -15, -27 } },
    { 2560, 1600, 0.66666667f, 32, {  -7, -15, -27 } },
    { 1280,  720, 0.33333333f, 16, {  -3,  -8, -13 } },
    {  800,  600, 0.20833333f, 10, {  -2,  -5,  -8 } }
};

TEST(jonesConfig, FullscreenFontAndGeometryFollowDesktopProportionsAtEachDpi)
{
    static const struct { UINT dpi; LONG dialogHeight; float nativeScale; } aDpi[] =
    {
        { 96, 103, 1.0f }, { 144, 154, 1.50487013f }, { 192, 206, 2.0f }
    };
    for ( size_t dpiIndex = 0; dpiIndex < STD_ARRAYLEN(aDpi); ++dpiIndex )
    {
        for ( size_t i = 0; i < STD_ARRAYLEN(jonesConfigTest_aResolutions); ++i )
        {
            const JonesConfigTestResolution* pCase = &jonesConfigTest_aResolutions[i];
            jonesConfigTest_ResetLayout();
            jonesConfigTest_dpi = jonesConfigTest_desktopDpi = aDpi[dpiIndex].dpi;
            jonesConfigTest_dialogHeight = aDpi[dpiIndex].dialogHeight;
            jonesConfigTest_renderWidth = pCase->width;
            jonesConfigTest_renderHeight = pCase->height;
            float scale = -1.0f;
            TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_NEW_FONT,
                jonesConfig_CreateDialogFont(JONESCONFIGTEST_DIALOG, 0, 211, &scale));
            TEST_ASSERT_EQUAL_INT(pCase->fontHeight[dpiIndex], jonesConfigTest_createdFont.lfHeight);
            // Geometry remains continuous while GDI font heights require whole
            // pixels; this tolerance is below a thousandth of a layout pixel.
            TEST_ASSERT_FLOAT_WITHIN(0.000001f, aDpi[dpiIndex].nativeScale * pCase->ratio, scale);
            TEST_ASSERT_EQUAL_STRING("Synthetic Test Font", jonesConfigTest_createdFont.lfFaceName);
            TEST_ASSERT_EQUAL_UINT(1, jonesConfigTest_numFonts);
        }
    }
}

TEST(jonesConfig, FullscreenSizingFallsBackSafelyWhenDesktopModeCannotBeRead)
{
    // Failed lookup and invalid saved modes keep the render resolution as the
    // baseline. They must not divide by zero or shrink an otherwise usable UI.
    for ( int failure = 1; failure <= 5; ++failure )
    {
        jonesConfigTest_ResetLayout();
        jonesConfigTest_renderWidth = 1920;
        jonesConfigTest_renderHeight = 1080;
        if ( failure <= 3 ) { jonesConfigTest_desktopFailure = failure; }
        if ( failure == 4 ) { jonesConfigTest_desktopWidth = 0; }
        if ( failure == 5 ) { jonesConfigTest_desktopHeight = 0; }
        float scale = 0.0f;
        TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_NEW_FONT,
            jonesConfig_CreateDialogFont(JONESCONFIGTEST_DIALOG, 0, 211, &scale));
        TEST_ASSERT_EQUAL_INT(-23, jonesConfigTest_createdFont.lfHeight);
        TEST_ASSERT_FLOAT_WITHIN(0.000001f, 1.50487013f, scale);
    }
}

TEST(jonesConfig, WindowedAndStartupDialogsDoNotUseFullscreenPresentationRatio)
{
    for ( int windowed = 0; windowed < 2; ++windowed )
    {
        jonesConfigTest_ResetLayout();
        jonesConfigTest_renderWidth = 800;
        jonesConfigTest_renderHeight = 600;
        jonesConfigTest_hasStarted = windowed;
        jonesConfigTest_settings.bWindowMode = windowed;
        float scale = 0.0f;
        TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_NEW_FONT,
            jonesConfig_CreateDialogFont(JONESCONFIGTEST_DIALOG, windowed, 211, &scale));
        TEST_ASSERT_FLOAT_WITHIN(0.000001f, 1.50487013f, scale);
        TEST_ASSERT_EQUAL_INT(-16, jonesConfigTest_createdFont.lfHeight);
        TEST_ASSERT_EQUAL_UINT(0, jonesConfigTest_monitorQueries);
    }
}

TEST(jonesConfig, MissingDialogOrFontDoesNotCreateAGdiFont)
{
    jonesConfigTest_ResetLayout();
    float scale = 123.0f;
    TEST_ASSERT_NULL(jonesConfig_CreateDialogFont(NULL, 0, 211, &scale));
    TEST_ASSERT_EQUAL_FLOAT(123.0f, scale);
    jonesConfigTest_missingFont = 1;
    TEST_ASSERT_NULL(jonesConfig_CreateDialogFont(JONESCONFIGTEST_DIALOG, 0, 211, &scale));
    TEST_ASSERT_EQUAL_FLOAT(123.0f, scale);
    TEST_ASSERT_EQUAL_UINT(0, jonesConfigTest_numFonts);
    TEST_ASSERT_EQUAL_UINT(0, jonesConfigTest_monitorQueries);
}

TEST(jonesConfig, StoreIconsFollowTheSameFullscreenResolutionProportions)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(jonesConfigTest_aResolutions); ++i )
    {
        jonesConfigTest_ResetLayout();
        const JonesConfigTestResolution* pCase = &jonesConfigTest_aResolutions[i];
        jonesConfigTest_renderWidth = pCase->width;
        jonesConfigTest_renderHeight = pCase->height;
        tStoreCartState cart = { 0 };
        TEST_ASSERT_TRUE(jonesConfig_StoreInitItemIcons(JONESCONFIGTEST_DIALOG, &cart));
        TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_IMAGE_LIST, cart.hList);
        TEST_ASSERT_EQUAL_INT(pCase->iconSize, jonesConfigTest_iconWidth);
        TEST_ASSERT_EQUAL_INT(pCase->iconSize, jonesConfigTest_iconHeight);
        TEST_ASSERT_EQUAL_UINT(14, jonesConfigTest_numIconLoads);
        TEST_ASSERT_EQUAL_UINT(14, jonesConfigTest_numIconCopies);
        for ( size_t icon = 0; icon < STD_ARRAYLEN(cart.aItemIcons); ++icon )
        {
            TEST_ASSERT_NOT_NULL(cart.aItemIcons[icon]);
        }
    }
}

TEST(jonesConfig, TinyFullscreenStillUsesNonzeroFontAndIconPixels)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_renderWidth = jonesConfigTest_renderHeight = 1;
    float scale = 0.0f;
    TEST_ASSERT_NOT_NULL(jonesConfig_CreateDialogFont(JONESCONFIGTEST_DIALOG, 0, 211, &scale));
    TEST_ASSERT_EQUAL_INT(-1, jonesConfigTest_createdFont.lfHeight);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, scale);
    tStoreCartState cart = { 0 };
    TEST_ASSERT_TRUE(jonesConfig_StoreInitItemIcons(JONESCONFIGTEST_DIALOG, &cart));
    TEST_ASSERT_EQUAL_INT(1, jonesConfigTest_iconWidth);
    TEST_ASSERT_EQUAL_INT(1, jonesConfigTest_iconHeight);
}

TEST(jonesConfig, StoreIconAllocationFailureDoesNotLoadOrCopyIcons)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_imageListFails = 1;
    tStoreCartState cart = { 0 };
    TEST_ASSERT_FALSE(jonesConfig_StoreInitItemIcons(JONESCONFIGTEST_DIALOG, &cart));
    TEST_ASSERT_NULL(cart.hList);
    TEST_ASSERT_EQUAL_UINT(0, jonesConfigTest_numIconLoads);
    TEST_ASSERT_EQUAL_UINT(0, jonesConfigTest_numIconCopies);
}

TEST(jonesConfig, StoreColumnsFitMeasuredLocalizedHeadersAndNumbers)
{
    jonesConfigTest_ResetLayout();
    jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COST");
    TEST_ASSERT_EQUAL_STRING("Item", jonesConfigTest_columns[0].pszText);
    TEST_ASSERT_EQUAL_STRING("Unit price", jonesConfigTest_columns[1].pszText);
    TEST_ASSERT_EQUAL_INT(LVCFMT_LEFT, jonesConfigTest_columns[0].fmt);
    TEST_ASSERT_EQUAL_INT(LVCFMT_RIGHT, jonesConfigTest_columns[1].fmt);
    TEST_ASSERT_EQUAL_INT(402, jonesConfigTest_columns[0].cx);
    TEST_ASSERT_EQUAL_INT(98, jonesConfigTest_columns[1].cx);
    TEST_ASSERT_EQUAL_INT(0, jonesConfigTest_columns[0].iSubItem);
    TEST_ASSERT_EQUAL_INT(1, jonesConfigTest_columns[1].iSubItem);
    TEST_ASSERT_GREATER_THAN_UINT(0, jonesConfigTest_numPriceMeasurements);
    TEST_ASSERT_EQUAL_UINT(2, jonesConfigTest_numColumnInserts);

    // The same function updates existing columns after the real client viewport
    // shrinks for a scrollbar. It must not append duplicates or deduct the bar
    // again. A four-character count still needs room when its heading is absent.
    jonesConfigTest_listWidth = 483;
    jonesConfigTest_missingHeader = 1;
    jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COUNT");
    TEST_ASSERT_EQUAL_INT(433, jonesConfigTest_columns[0].cx);
    TEST_ASSERT_EQUAL_INT(50, jonesConfigTest_columns[1].cx);
    TEST_ASSERT_EQUAL_STRING("", jonesConfigTest_columns[1].pszText);
    TEST_ASSERT_GREATER_THAN_UINT(0, jonesConfigTest_numCountMeasurements);
    TEST_ASSERT_EQUAL_UINT(2, jonesConfigTest_numColumnInserts);
    TEST_ASSERT_EQUAL_UINT(4, jonesConfigTest_numColumnUpdates);
}

TEST(jonesConfig, StoreColumnsUseTheCurrentFontAndProportionalPadding)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_renderWidth = 1920;
    jonesConfigTest_renderHeight = 1080;
    jonesConfigTest_charWidth = 4;
    jonesConfigTest_listDpi = 96;
    jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COST");
    TEST_ASSERT_EQUAL_INT(448, jonesConfigTest_columns[0].cx);
    TEST_ASSERT_EQUAL_INT(52, jonesConfigTest_columns[1].cx);
    TEST_ASSERT_EQUAL_INT(500, jonesConfigTest_columns[0].cx + jonesConfigTest_columns[1].cx);
}

TEST(jonesConfig, NarrowAndEmptyStoreListsNeverProduceNegativeColumnWidths)
{
    static const int aWidths[] = { 97, 91, 20, 1, 0 };
    for ( size_t i = 0; i < STD_ARRAYLEN(aWidths); ++i )
    {
        jonesConfigTest_ResetLayout();
        jonesConfigTest_listWidth = aWidths[i];
        jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COST");
        TEST_ASSERT_EQUAL_INT(0, jonesConfigTest_columns[0].cx);
        TEST_ASSERT_EQUAL_INT(aWidths[i], jonesConfigTest_columns[1].cx);
    }
}

TEST(jonesConfig, DialogBitmapMaskAndImageUseTheFinalControlBounds)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_controlRect = (RECT){ 0, 0, 48, 37 };
    TEST_ASSERT_TRUE(jonesConfig_DrawImageOnDialogItem(JONESCONFIGTEST_DIALOG,
        JONESCONFIGTEST_SOURCE_DC, JONESCONFIGTEST_DEST_DC, 1182, JONESCONFIGTEST_IMAGE, JONESCONFIGTEST_MASK));
    TEST_ASSERT_EQUAL_UINT(2, jonesConfigTest_numStretches);
    for ( unsigned i = 0; i < 2; ++i )
    {
        const JonesConfigTestStretch* pCall = &jonesConfigTest_stretches[i];
        TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_DEST_DC, pCall->hDest);
        TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_SOURCE_DC, pCall->hSource);
        TEST_ASSERT_EQUAL_INT(0, pCall->x);
        TEST_ASSERT_EQUAL_INT(0, pCall->y);
        TEST_ASSERT_EQUAL_INT(48, pCall->width);
        TEST_ASSERT_EQUAL_INT(37, pCall->height);
        TEST_ASSERT_EQUAL_INT(0, pCall->sourceX);
        TEST_ASSERT_EQUAL_INT(0, pCall->sourceY);
        TEST_ASSERT_EQUAL_INT(64, pCall->sourceWidth);
        TEST_ASSERT_EQUAL_INT(64, pCall->sourceHeight);
        TEST_ASSERT_EQUAL_UINT32(i ? SRCPAINT : SRCAND, pCall->operation);
        TEST_ASSERT_EQUAL_PTR(i ? JONESCONFIGTEST_IMAGE : JONESCONFIGTEST_MASK, pCall->hSelected);
    }
}

TEST(jonesConfig, UnmaskedDialogBitmapReturnsTheGdiCopyResult)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_controlRect = (RECT){ 0, 0, 24, 24 };
    jonesConfigTest_stretchResult = 0;
    TEST_ASSERT_FALSE(jonesConfig_DrawImageOnDialogItem(JONESCONFIGTEST_DIALOG,
        JONESCONFIGTEST_SOURCE_DC, JONESCONFIGTEST_DEST_DC, 1182, JONESCONFIGTEST_IMAGE, NULL));
    TEST_ASSERT_EQUAL_UINT(1, jonesConfigTest_numStretches);
    TEST_ASSERT_EQUAL_UINT32(SRCCOPY, jonesConfigTest_stretches[0].operation);
    TEST_ASSERT_EQUAL_PTR(JONESCONFIGTEST_IMAGE, jonesConfigTest_stretches[0].hSelected);
    TEST_ASSERT_EQUAL_INT(24, jonesConfigTest_stretches[0].width);
    TEST_ASSERT_EQUAL_INT(24, jonesConfigTest_stretches[0].height);
}


TEST(jonesConfig, StoreIconsKeepTheirNativeSizeOutsideStartedFullscreen)
{
    for ( int profile = 0; profile < 3; ++profile )
    {
        jonesConfigTest_ResetLayout();
        jonesConfigTest_renderWidth = 1920;
        jonesConfigTest_renderHeight = 1080;
        if ( profile == 0 ) { jonesConfigTest_hasStarted = 0; }
        if ( profile == 1 ) { jonesConfigTest_settings.bWindowMode = 1; }
        if ( profile == 2 ) { jonesConfigTest_missingSettings = 1; }
        tStoreCartState cart = { 0 };
        TEST_ASSERT_TRUE(jonesConfig_StoreInitItemIcons(JONESCONFIGTEST_DIALOG, &cart));
        TEST_ASSERT_EQUAL_INT(48, jonesConfigTest_iconWidth);
        TEST_ASSERT_EQUAL_INT(48, jonesConfigTest_iconHeight);
        TEST_ASSERT_EQUAL_UINT(0, jonesConfigTest_monitorQueries);
    }
}

TEST(jonesConfig, StoreColumnsReserveTheWidestPriceWhenItsHeadingIsMissing)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_missingHeader = 1;
    jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COST");
    TEST_ASSERT_EQUAL_INT(450, jonesConfigTest_columns[0].cx);
    TEST_ASSERT_EQUAL_INT(50, jonesConfigTest_columns[1].cx);
    TEST_ASSERT_EQUAL_STRING("", jonesConfigTest_columns[1].pszText);
    TEST_ASSERT_GREATER_THAN_UINT(0, jonesConfigTest_numPriceMeasurements);
    TEST_ASSERT_EQUAL_UINT32(LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH | LVCF_FMT,
        jonesConfigTest_columns[1].mask);
}

TEST(jonesConfig, FailedStoreClientAndTextQueriesKeepColumnWidthsBounded)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_listClientFails = 1;
    jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COST");
    TEST_ASSERT_EQUAL_INT(0, jonesConfigTest_columns[0].cx);
    TEST_ASSERT_EQUAL_INT(0, jonesConfigTest_columns[1].cx);

    // A failed string-width message must not propagate a negative column width.
    // Existing columns are updated against the valid client viewport this time.
    jonesConfigTest_listClientFails = 0;
    jonesConfigTest_charWidth = -1;
    jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COUNT");
    TEST_ASSERT_EQUAL_INT(482, jonesConfigTest_columns[0].cx);
    TEST_ASSERT_EQUAL_INT(18, jonesConfigTest_columns[1].cx);
}


TEST(jonesConfig, FullscreenDialogDpiChangesDoNotApplyTheDesktopScaleTwice)
{
    static const struct { uint32_t width, height; int fontHeight; float geometryScale; int iconSize; } aCases[] =
    {
        {  960,  540,  -6, 0.5625f, 12 },
        { 1920, 1080, -12, 1.1250f, 24 },
        { 3840, 2160, -23, 2.2500f, 48 }
    };
    for ( size_t i = 0; i < STD_ARRAYLEN(aCases); ++i )
    {
        jonesConfigTest_ResetLayout();
        // Exclusive fullscreen can report a 96-DPI modal dialog while the game
        // HWND retains the configured 150% desktop DPI. The resource template
        // is correspondingly smaller, but desired physical UI proportions are
        // still based on that original desktop, not on the dialog's new DPI.
        jonesConfigTest_dpi = 96;
        jonesConfigTest_dialogHeight = 103;
        jonesConfigTest_renderWidth = aCases[i].width;
        jonesConfigTest_renderHeight = aCases[i].height;
        float scale = 0.0f;
        TEST_ASSERT_NOT_NULL(jonesConfig_CreateDialogFont(JONESCONFIGTEST_DIALOG, 0, 211, &scale));
        TEST_ASSERT_EQUAL_INT(aCases[i].fontHeight, jonesConfigTest_createdFont.lfHeight);
        TEST_ASSERT_FLOAT_WITHIN(0.000001f, aCases[i].geometryScale, scale);
        tStoreCartState cart = { 0 };
        TEST_ASSERT_TRUE(jonesConfig_StoreInitItemIcons(JONESCONFIGTEST_DIALOG, &cart));
        TEST_ASSERT_EQUAL_INT(aCases[i].iconSize, jonesConfigTest_iconWidth);
        TEST_ASSERT_EQUAL_INT(aCases[i].iconSize, jonesConfigTest_iconHeight);
    }
}

TEST(jonesConfig, LargerDialogDpiAlsoNormalizesAgainstTheSavedDesktopDpi)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_dpi = 192;
    jonesConfigTest_dialogHeight = 206;
    jonesConfigTest_renderWidth = 1920;
    jonesConfigTest_renderHeight = 1080;
    float scale = 0.0f;
    TEST_ASSERT_NOT_NULL(jonesConfig_CreateDialogFont(JONESCONFIGTEST_DIALOG, 0, 211, &scale));
    TEST_ASSERT_EQUAL_INT(-12, jonesConfigTest_createdFont.lfHeight);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.5625f, scale);
}

TEST(jonesConfig, MissingSavedDesktopDpiFallsBackToTheDialogDpi)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_desktopDpi = 0;
    jonesConfigTest_dpi = 96;
    jonesConfigTest_dialogHeight = 103;
    jonesConfigTest_renderWidth = 1920;
    jonesConfigTest_renderHeight = 1080;
    float scale = 0.0f;
    TEST_ASSERT_NOT_NULL(jonesConfig_CreateDialogFont(JONESCONFIGTEST_DIALOG, 0, 211, &scale));
    TEST_ASSERT_EQUAL_INT(-5, jonesConfigTest_createdFont.lfHeight);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.5f, scale);
}


TEST(jonesConfig, ExitImageUsesDesktopDpiSizeAndKeepsItsAxesAndOwnerDrawStyle)
{
    static const struct { UINT desktopDpi, dialogDpi; int started, windowed, expectedSize; } aCases[] =
    {
        { 144,  96, 1, 0,  64 },
        { 144, 144, 1, 0,  96 },
        { 144, 192, 1, 0, 128 },
        {   0,  96, 1, 0,  96 },
        { 144,   0, 1, 0,  96 },
        { 144,  96, 1, 1,  96 },
        { 144,  96, 0, 0,  96 }
    };
    for ( size_t i = 0; i < STD_ARRAYLEN(aCases); ++i )
    {
        jonesConfigTest_ResetLayout();
        jonesConfigTest_desktopDpi = aCases[i].desktopDpi;
        jonesConfigTest_dpi = aCases[i].dialogDpi;
        jonesConfigTest_hasStarted = aCases[i].started;
        jonesConfigTest_settings.bWindowMode = aCases[i].windowed;
        // This is the same initializer called by WM_INITDIALOG, before the
        // shared child-layout pass. Asymmetric offsets catch the old X/Y swap.
        jonesConfig_InitExitGameDialogImage(JONESCONFIGTEST_DIALOG);
        TEST_ASSERT_EQUAL_UINT(1, jonesConfigTest_numImageMoves);
        TEST_ASSERT_EQUAL_INT(30, jonesConfigTest_imageX);
        TEST_ASSERT_EQUAL_INT(40, jonesConfigTest_imageY);
        TEST_ASSERT_EQUAL_INT(aCases[i].expectedSize, jonesConfigTest_imageWidth);
        TEST_ASSERT_EQUAL_INT(aCases[i].expectedSize, jonesConfigTest_imageHeight);
        TEST_ASSERT_EQUAL_UINT32(SS_OWNERDRAW, jonesConfigTest_imageStyle & SS_TYPEMASK);
        TEST_ASSERT_EQUAL_UINT32(SS_NOTIFY, jonesConfigTest_imageStyle & ~SS_TYPEMASK);
    }
}


TEST(jonesConfig, StoreColumnsKeepNativeControlInsetsAtSmallFullscreenResolutions)
{
    static const struct { uint32_t width, height; } aModes[] =
    {
        { 1920, 1080 }, { 960, 540 }, { 800, 600 }
    };
    for ( size_t i = 0; i < STD_ARRAYLEN(aModes); ++i )
    {
        jonesConfigTest_ResetLayout();
        jonesConfigTest_renderWidth = aModes[i].width;
        jonesConfigTest_renderHeight = aModes[i].height;
        jonesConfigTest_listDpi = 96;
        jonesConfigTest_charWidth = 6;
        jonesConfigTest_costHeader = "Cost";
        // The native ListView needs 12 pixels beyond its 24-pixel header and
        // price glyphs. At half resolution, shrinking that inset to 6 gave a
        // 30-pixel column even though the control required 36 pixels to fit.
        jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COST");
        TEST_ASSERT_EQUAL_INT(36, jonesConfigTest_columns[1].cx);
        TEST_ASSERT_EQUAL_INT(464, jonesConfigTest_columns[0].cx);
        TEST_ASSERT_EQUAL_STRING("Cost", jonesConfigTest_columns[1].pszText);
        TEST_ASSERT_GREATER_THAN_UINT(0, jonesConfigTest_numPriceMeasurements);
    }
}

TEST(jonesConfig, StoreColumnInsetsUseControlDpiWithA96DpiFailureFallback)
{
    static const struct { UINT dpi; int valueWidth; } aCases[] =
    {
        { 0, 92 }, { 96, 92 }, { 144, 98 }, { 192, 104 }
    };
    for ( size_t i = 0; i < STD_ARRAYLEN(aCases); ++i )
    {
        jonesConfigTest_ResetLayout();
        jonesConfigTest_renderWidth = 1920;
        jonesConfigTest_renderHeight = 1080;
        jonesConfigTest_listDpi = aCases[i].dpi;
        jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COST");
        TEST_ASSERT_EQUAL_INT(aCases[i].valueWidth, jonesConfigTest_columns[1].cx);
        TEST_ASSERT_EQUAL_INT(500 - aCases[i].valueWidth, jonesConfigTest_columns[0].cx);
    }
}

TEST(jonesConfig, LargerPresentationPaddingStillWinsOverTheControlMinimum)
{
    jonesConfigTest_ResetLayout();
    jonesConfigTest_renderWidth = 7680;
    jonesConfigTest_renderHeight = 4320;
    jonesConfigTest_listDpi = 96;
    jonesConfig_StoreSetListColumns(JONESCONFIGTEST_LIST, "JONES_STR_COST");
    TEST_ASSERT_EQUAL_INT(396, jonesConfigTest_columns[0].cx);
    TEST_ASSERT_EQUAL_INT(104, jonesConfigTest_columns[1].cx);
}

TEST_GROUP_RUNNER(jonesConfig)
{
    RUN_TEST_CASE(jonesConfig, EightBindingsFitTheActionRow);
    RUN_TEST_CASE(jonesConfig, NinthBindingDoesNotOverwriteNextRowOrName);
    RUN_TEST_CASE(jonesConfig, FailedLoadClearsActionsAccordingToProfile);
    RUN_TEST_CASE(jonesConfig, OpenFailurePreservesScheme);
    RUN_TEST_CASE(jonesConfig, KeyNameFailureReturnsAnEmptyTerminatedString);
    RUN_TEST_CASE(jonesConfig, KeyNameSuccessUsesTheReturnedName);
    RUN_TEST_CASE(jonesConfig, FullscreenFontAndGeometryFollowDesktopProportionsAtEachDpi);
    RUN_TEST_CASE(jonesConfig, FullscreenSizingFallsBackSafelyWhenDesktopModeCannotBeRead);
    RUN_TEST_CASE(jonesConfig, WindowedAndStartupDialogsDoNotUseFullscreenPresentationRatio);
    RUN_TEST_CASE(jonesConfig, MissingDialogOrFontDoesNotCreateAGdiFont);
    RUN_TEST_CASE(jonesConfig, StoreIconsFollowTheSameFullscreenResolutionProportions);
    RUN_TEST_CASE(jonesConfig, TinyFullscreenStillUsesNonzeroFontAndIconPixels);
    RUN_TEST_CASE(jonesConfig, StoreIconAllocationFailureDoesNotLoadOrCopyIcons);
    RUN_TEST_CASE(jonesConfig, StoreColumnsFitMeasuredLocalizedHeadersAndNumbers);
    RUN_TEST_CASE(jonesConfig, StoreColumnsUseTheCurrentFontAndProportionalPadding);
    RUN_TEST_CASE(jonesConfig, NarrowAndEmptyStoreListsNeverProduceNegativeColumnWidths);
    RUN_TEST_CASE(jonesConfig, DialogBitmapMaskAndImageUseTheFinalControlBounds);
    RUN_TEST_CASE(jonesConfig, UnmaskedDialogBitmapReturnsTheGdiCopyResult);
    RUN_TEST_CASE(jonesConfig, StoreIconsKeepTheirNativeSizeOutsideStartedFullscreen);
    RUN_TEST_CASE(jonesConfig, StoreColumnsReserveTheWidestPriceWhenItsHeadingIsMissing);
    RUN_TEST_CASE(jonesConfig, FailedStoreClientAndTextQueriesKeepColumnWidthsBounded);
    RUN_TEST_CASE(jonesConfig, FullscreenDialogDpiChangesDoNotApplyTheDesktopScaleTwice);
    RUN_TEST_CASE(jonesConfig, LargerDialogDpiAlsoNormalizesAgainstTheSavedDesktopDpi);
    RUN_TEST_CASE(jonesConfig, MissingSavedDesktopDpiFallsBackToTheDialogDpi);
    RUN_TEST_CASE(jonesConfig, ExitImageUsesDesktopDpiSizeAndKeepsItsAxesAndOwnerDrawStyle);
    RUN_TEST_CASE(jonesConfig, StoreColumnsKeepNativeControlInsetsAtSmallFullscreenResolutions);
    RUN_TEST_CASE(jonesConfig, StoreColumnInsetsUseControlDpiWithA96DpiFailureFallback);
    RUN_TEST_CASE(jonesConfig, LargerPresentationPaddingStillWinsOverTheControlMinimum);
}

static void JonesConfigTest_RunAllTests(void)
{
    RUN_TEST_GROUP(jonesConfig);
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, JonesConfigTest_RunAllTests);
}
