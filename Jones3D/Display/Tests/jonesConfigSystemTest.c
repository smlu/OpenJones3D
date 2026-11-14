#include <unity_fixture.h>
#include <Jones3D/Display/jonesConfig.h>
#include <std/General/std.h>
#include <std/General/stdConfig.h>
#include <std/Win95/stdDisplay.h>
#include <commctrl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void J3DAPI jonesConfigSystem_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight);
static HICON WINAPI jonesConfigSystem_LoadIconA(HINSTANCE hInstance, LPCSTR pName);
static bool J3DAPI jonesConfigSystem_GetConfigBool(const char* pKey, bool defaultValue);

// The application supplies its render dimensions and localization. All window,
// font, image-list and raster operations below use the real Win32 APIs. The icon
// resource input is authored in memory, so no retail executable is required.
#define stdDisplay_GetBackBufferSize jonesConfigSystem_GetBackBufferSize
#define LoadIconA jonesConfigSystem_LoadIconA
#define stdConfig_GetBool jonesConfigSystem_GetConfigBool
#undef STDLOG_STATUS
#undef STDLOG_ERROR
#undef STDLOG_DEBUG
#define STDLOG_STATUS(...) ((void)0)
#define STDLOG_ERROR(...) ((void)0)
#define STDLOG_DEBUG(...) ((void)0)
// The tests run without std host services, so engine allocations use the C heap.
#include <stdlib.h>
#include <std/General/stdMemory.h>
#undef STDMALLOC
#undef STDFREE
#define STDMALLOC(size) malloc(size)
#define STDFREE(pData) free(pData)
// The game functions jonesConfig.c calls are fixtures of this test (defined below). They are renamed seams,
// so the module test executable can also link the real functions.
#define JonesDisplay_GetDesktopDpi           jonesConfigSystem_JonesDisplay_GetDesktopDpi
#define JonesMain_HasStarted                 jonesConfigSystem_JonesMain_HasStarted
#define JonesMain_GetDisplaySettings         jonesConfigSystem_JonesMain_GetDisplaySettings
#define jonesString_GetString                jonesConfigSystem_jonesString_GetString
#define JonesDialog_ShowDialog               jonesConfigSystem_JonesDialog_ShowDialog
#define JonesHud_BindActivateControlKeys     jonesConfigSystem_JonesHud_BindActivateControlKeys
#define JonesMain_BindToggleMenuControlKeys  jonesConfigSystem_JonesMain_BindToggleMenuControlKeys
#define JonesControl_EnableJoystickAxes      jonesConfigSystem_JonesControl_EnableJoystickAxes
#define JonesReticle_Enable                  jonesConfigSystem_JonesReticle_Enable
#define JonesReticle_IsEnabled               jonesConfigSystem_JonesReticle_IsEnabled
#define JonesDialog_ShowFileSelectDialog     jonesConfigSystem_JonesDialog_ShowFileSelectDialog
#define JonesFile_GetCurrentCDNum            jonesConfigSystem_JonesFile_GetCurrentCDNum
#define JonesMain_GetCurrentLevelNum         jonesConfigSystem_JonesMain_GetCurrentLevelNum
#define jonesInventory_GetMaxDifficultyLevel jonesConfigSystem_jonesInventory_GetMaxDifficultyLevel
#define jonesInventory_GetTotalIQPoints      jonesConfigSystem_jonesInventory_GetTotalIQPoints

UINT JonesDisplay_GetDesktopDpi(void);
int JonesMain_HasStarted(void);
JonesDisplaySettings* JonesMain_GetDisplaySettings(void);
const char* J3DAPI jonesString_GetString(const char* pKey);
int J3DAPI JonesDialog_ShowDialog(LPCSTR pTemplate, HWND hWnd, DLGPROC pfnDialog, LPARAM data);
void J3DAPI JonesHud_BindActivateControlKeys(const size_t* pKeys, size_t numKeys);
void J3DAPI JonesMain_BindToggleMenuControlKeys(const size_t* pKeys, size_t numKeys);
void JonesControl_EnableJoystickAxes(void);
void J3DAPI JonesReticle_Enable(bool bEnabled);
bool JonesReticle_IsEnabled(void);
BOOL J3DAPI JonesDialog_ShowFileSelectDialog(LPOPENFILENAMEA pData, int bOpen);
int JonesFile_GetCurrentCDNum(void);
size_t JonesMain_GetCurrentLevelNum(void);
size_t jonesInventory_GetMaxDifficultyLevel(void);
int jonesInventory_GetTotalIQPoints(void);

#include "../jonesConfig.c"
#undef stdDisplay_GetBackBufferSize
#undef LoadIconA
#undef stdConfig_GetBool

static HWND jonesConfigSystem_hDialog;
static HWND jonesConfigSystem_hList;
static DPI_AWARENESS_CONTEXT jonesConfigSystem_previousDpi;
static JonesDisplaySettings jonesConfigSystem_settings;
static uint32_t jonesConfigSystem_desktopWidth;
static uint32_t jonesConfigSystem_desktopHeight;
static uint32_t jonesConfigSystem_renderWidth;
static uint32_t jonesConfigSystem_renderHeight;
static HFONT jonesConfigSystem_aFonts[2];
static HICON jonesConfigSystem_hIcon;
static HIMAGELIST jonesConfigSystem_hImages;
static HDC jonesConfigSystem_hSourceDC;
static HDC jonesConfigSystem_hDestDC;
static HGDIOBJ jonesConfigSystem_hOldSource;
static HGDIOBJ jonesConfigSystem_hOldDest;
static HBITMAP jonesConfigSystem_hImage;
static HBITMAP jonesConfigSystem_hMask;
static HBITMAP jonesConfigSystem_hDestination;
static int jonesConfigSystem_upscalingDisabled;

static bool J3DAPI jonesConfigSystem_GetConfigBool(const char* pKey, bool defaultValue)
{
    // The config file isn't loaded; only the dialog image upscaling option is controlled
    if ( strcmp(pKey, JONESCONFIG_CFG_GRAPHICS_UPSCALEDIALOGIMAGES) == 0 )
    {
        return !jonesConfigSystem_upscalingDisabled;
    }
    return defaultValue;
}

static void J3DAPI jonesConfigSystem_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight)
{
    *pWidth = jonesConfigSystem_renderWidth;
    *pHeight = jonesConfigSystem_renderHeight;
}

static HICON WINAPI jonesConfigSystem_LoadIconA(HINSTANCE hInstance, LPCSTR pName)
{
    J3D_UNUSED(hInstance);
    J3D_UNUSED(pName);
    return jonesConfigSystem_hIcon;
}

UINT JonesDisplay_GetDesktopDpi(void)
{
    // The hidden native fixture has not switched the monitor's display mode.
    return GetDpiForWindow(jonesConfigSystem_hDialog);
}

int JonesMain_HasStarted(void)
{
    return 1;
}

JonesDisplaySettings* JonesMain_GetDisplaySettings(void)
{
    return &jonesConfigSystem_settings;
}

const char* J3DAPI jonesString_GetString(const char* pKey)
{
    if ( strcmp(pKey, "JONES_STR_FONT") == 0 ) return "Segoe UI";
    if ( strcmp(pKey, "JONES_STR_ITEMNAME") == 0 ) return "Equipment";
    if ( strcmp(pKey, "JONES_STR_COST") == 0 ) return "Cost";
    if ( strcmp(pKey, "JONES_STR_COUNT") == 0 ) return "Quantity";
    return NULL;
}

// The full module is included for its actual GUI code. Unrelated application
// operations are link dependencies, and fail immediately if a fixture
// accidentally enters gameplay, a modal dialog or an input setup path.
int J3DAPI JonesDialog_ShowDialog(LPCSTR pTemplate, HWND hWnd, DLGPROC pfnDialog, LPARAM data)
{
    J3D_UNUSED(pTemplate); J3D_UNUSED(hWnd); J3D_UNUSED(pfnDialog); J3D_UNUSED(data);
    TEST_FAIL_MESSAGE("Unexpected modal dialog");
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


static HBITMAP jonesConfigSystem_CreateDIB(int width, int height, uint32_t** ppPixels)
{
    BITMAPINFO info = { 0 };
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(NULL, &info, DIB_RGB_COLORS, (void**)ppPixels, NULL, 0);
}

static void jonesConfigSystem_SizeDialog(int width, int height)
{
    UINT dpi = GetDpiForWindow(jonesConfigSystem_hDialog);
    TEST_ASSERT_TRUE(MoveWindow(jonesConfigSystem_hDialog, 0, 0,
        MulDiv(width, dpi, USER_DEFAULT_SCREEN_DPI),
        MulDiv(height, dpi, USER_DEFAULT_SCREEN_DPI), FALSE));
}

static HFONT jonesConfigSystem_CreateFont(int slot, int dialogID, float* pScale)
{
    jonesConfigSystem_aFonts[slot] = jonesConfig_CreateDialogFont(
        jonesConfigSystem_hDialog, 0, dialogID, pScale);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_aFonts[slot]);
    return jonesConfigSystem_aFonts[slot];
}

static void jonesConfigSystem_AssertColumnsFit(const char* pValueHeader)
{
    RECT rect;
    TEST_ASSERT_TRUE(GetClientRect(jonesConfigSystem_hList, &rect));
    int scrollOrigin = GetScrollPos(jonesConfigSystem_hList, SB_HORZ);
    TEST_ASSERT_EQUAL_INT(0, scrollOrigin);
    int nameWidth = ListView_GetColumnWidth(jonesConfigSystem_hList, 0);
    int valueWidth = ListView_GetColumnWidth(jonesConfigSystem_hList, 1);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, nameWidth);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, valueWidth);
    TEST_ASSERT_EQUAL_INT(rect.right - rect.left, nameWidth + valueWidth);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(
        ListView_GetStringWidth(jonesConfigSystem_hList, pValueHeader), valueWidth);
    HWND hHeader = ListView_GetHeader(jonesConfigSystem_hList);
    TEST_ASSERT_EQUAL_INT(2, Header_GetItemCount(hHeader));
    RECT headerRect;
    TEST_ASSERT_TRUE(Header_GetItemRect(hHeader, 1, &headerRect));
    MapWindowPoints(hHeader, jonesConfigSystem_hList, (POINT*)&headerRect, 2);
    RECT itemRect = { 0 };
    if ( ListView_GetItemCount(jonesConfigSystem_hList) )
    {
        TEST_ASSERT_TRUE(ListView_GetSubItemRect(jonesConfigSystem_hList, 0, 1, LVIR_BOUNDS, &itemRect));
    }
    HFONT hHeaderFont = (HFONT)SendMessageA(hHeader, WM_GETFONT, 0, 0);
    HDC hHeaderDC = GetDC(hHeader);
    TEST_ASSERT_NOT_NULL(hHeaderDC);
    HGDIOBJ hOldFont = SelectObject(hHeaderDC, hHeaderFont);
    SIZE textSize;
    BOOL measured = GetTextExtentPoint32A(hHeaderDC, pValueHeader, (int)strlen(pValueHeader), &textSize);
    SelectObject(hHeaderDC, hOldFont);
    ReleaseDC(hHeader, hHeaderDC);
    TEST_ASSERT_TRUE(measured);
    LOGFONTA headerFont = { 0 }, listFont = { 0 };
    GetObjectA(hHeaderFont, sizeof(headerFont), &headerFont);
    GetObjectA((HFONT)SendMessageA(jonesConfigSystem_hList, WM_GETFONT, 0, 0), sizeof(listFont), &listFont);
    printf("native store: client=%ld columns=%d+%d header=%ld..%ld item=%ld..%ld hscroll=%d headerFont=%ld listFont=%ld headerText=%ld\n",
        rect.right - rect.left, nameWidth, valueWidth, headerRect.left, headerRect.right,
        itemRect.left, itemRect.right, GetScrollPos(jonesConfigSystem_hList, SB_HORZ),
        headerFont.lfHeight, listFont.lfHeight, textSize.cx);
    // A temporary trailing column prevents AUTOSIZE_USEHEADER from applying
    // its special "last column fills the remaining viewport" behavior. Native
    // autosizing includes control/theme text insets that glyph width omits.
    LVCOLUMNA trailing = { 0 };
    trailing.mask = LVCF_WIDTH;
    trailing.cx = 0;
    TEST_ASSERT_EQUAL_INT(2, ListView_InsertColumn(jonesConfigSystem_hList, 2, &trailing));
    TEST_ASSERT_TRUE(ListView_SetColumnWidth(jonesConfigSystem_hList, 1, LVSCW_AUTOSIZE_USEHEADER));
    int headerAutoWidth = ListView_GetColumnWidth(jonesConfigSystem_hList, 1);
    int itemAutoWidth = 0;
    if ( ListView_GetItemCount(jonesConfigSystem_hList) )
    {
        TEST_ASSERT_TRUE(ListView_SetColumnWidth(jonesConfigSystem_hList, 1, LVSCW_AUTOSIZE));
        itemAutoWidth = ListView_GetColumnWidth(jonesConfigSystem_hList, 1);
    }
    TEST_ASSERT_TRUE(ListView_DeleteColumn(jonesConfigSystem_hList, 2));
    TEST_ASSERT_TRUE(ListView_SetColumnWidth(jonesConfigSystem_hList, 1, valueWidth));
    TEST_ASSERT_EQUAL_INT(scrollOrigin, GetScrollPos(jonesConfigSystem_hList, SB_HORZ));
    printf("native value text insets: header autosize=%d item autosize=%d configured=%d DPI=%u\n",
        headerAutoWidth, itemAutoWidth, valueWidth, GetDpiForWindow(jonesConfigSystem_hList));
    TEST_ASSERT_GREATER_OR_EQUAL_INT(headerAutoWidth, valueWidth);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(itemAutoWidth, valueWidth);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(textSize.cx, valueWidth);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(rect.left, headerRect.left);
    TEST_ASSERT_LESS_OR_EQUAL_INT(rect.right, headerRect.right);
    if ( ListView_GetItemCount(jonesConfigSystem_hList) )
    {
        TEST_ASSERT_GREATER_OR_EQUAL_INT(rect.left, itemRect.left);
        TEST_ASSERT_LESS_OR_EQUAL_INT(rect.right, itemRect.right);
    }
}

TEST_GROUP(jonesConfigSystem);

TEST_SETUP(jonesConfigSystem)
{
    memset(&jonesConfigSystem_settings, 0, sizeof(jonesConfigSystem_settings));
    memset(jonesConfigSystem_aFonts, 0, sizeof(jonesConfigSystem_aFonts));
    jonesConfigSystem_hDialog = jonesConfigSystem_hList = NULL;
    jonesConfigSystem_hIcon = NULL;
    jonesConfigSystem_hImages = NULL;
    jonesConfigSystem_hSourceDC = jonesConfigSystem_hDestDC = NULL;
    jonesConfigSystem_hOldSource = jonesConfigSystem_hOldDest = NULL;
    jonesConfigSystem_hImage = jonesConfigSystem_hMask = jonesConfigSystem_hDestination = NULL;
    jonesConfigSystem_upscalingDisabled = 0;
    jonesConfig_FreeScaledMaskedImages();
    jonesConfigSystem_previousDpi = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if ( !jonesConfigSystem_previousDpi ) TEST_IGNORE_MESSAGE("Per-monitor V2 thread DPI context is unavailable.");

    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_LISTVIEW_CLASSES };
    if ( !InitCommonControlsEx(&controls) ) TEST_IGNORE_MESSAGE("Native list-view controls are unavailable.");
    jonesConfigSystem_hDialog = CreateWindowExA(0, "STATIC", "OpenJones3D authored dialog test", WS_POPUP,
        0, 0, 600, 400, NULL, NULL, GetModuleHandleA(NULL), NULL);
    if ( !jonesConfigSystem_hDialog ) TEST_IGNORE_MESSAGE("A hidden Win32 window is unavailable.");
    SendMessageA(jonesConfigSystem_hDialog, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);
    TEST_ASSERT_FALSE(IsWindowVisible(jonesConfigSystem_hDialog));

    MONITORINFOEXA monitor = { .cbSize = sizeof(monitor) };
    DEVMODEA desktop = { .dmSize = sizeof(desktop) };
    HMONITOR hMonitor = MonitorFromWindow(jonesConfigSystem_hDialog, MONITOR_DEFAULTTONEAREST);
    if ( !hMonitor || !GetMonitorInfoA(hMonitor, (MONITORINFO*)&monitor)
        || !EnumDisplaySettingsExA(monitor.szDevice, ENUM_REGISTRY_SETTINGS, &desktop, 0)
        || !desktop.dmPelsWidth || !desktop.dmPelsHeight )
    {
        TEST_IGNORE_MESSAGE("The monitor's saved desktop mode is unavailable.");
    }
    jonesConfigSystem_desktopWidth = jonesConfigSystem_renderWidth = desktop.dmPelsWidth;
    jonesConfigSystem_desktopHeight = jonesConfigSystem_renderHeight = desktop.dmPelsHeight;
    printf("jonesConfigSystem: DPI=%u, saved desktop=%lux%lu\n",
        GetDpiForWindow(jonesConfigSystem_hDialog), (unsigned long)desktop.dmPelsWidth,
        (unsigned long)desktop.dmPelsHeight);
}

TEST_TEAR_DOWN(jonesConfigSystem)
{
    if ( jonesConfigSystem_hDialog ) DestroyWindow(jonesConfigSystem_hDialog);
    if ( jonesConfigSystem_hImages ) ImageList_Destroy(jonesConfigSystem_hImages);
    if ( jonesConfigSystem_hIcon ) DestroyIcon(jonesConfigSystem_hIcon);
    for ( size_t i = 0; i < STD_ARRAYLEN(jonesConfigSystem_aFonts); ++i )
    {
        if ( jonesConfigSystem_aFonts[i] ) DeleteObject(jonesConfigSystem_aFonts[i]);
    }
    if ( jonesConfigSystem_hSourceDC )
    {
        if ( jonesConfigSystem_hOldSource ) SelectObject(jonesConfigSystem_hSourceDC, jonesConfigSystem_hOldSource);
        DeleteDC(jonesConfigSystem_hSourceDC);
    }
    if ( jonesConfigSystem_hDestDC )
    {
        if ( jonesConfigSystem_hOldDest ) SelectObject(jonesConfigSystem_hDestDC, jonesConfigSystem_hOldDest);
        DeleteDC(jonesConfigSystem_hDestDC);
    }
    if ( jonesConfigSystem_hImage ) DeleteObject(jonesConfigSystem_hImage);
    if ( jonesConfigSystem_hMask ) DeleteObject(jonesConfigSystem_hMask);
    if ( jonesConfigSystem_hDestination ) DeleteObject(jonesConfigSystem_hDestination);
    if ( jonesConfigSystem_previousDpi ) SetThreadDpiAwarenessContext(jonesConfigSystem_previousDpi);
}

TEST(jonesConfigSystem, NativeFontsFollowTheFullscreenPresentationScale)
{
    // Real font creation and host DPI must agree with the resolution policy;
    // exact glyph pixels remain font/host dependent and are not golden images.
    jonesConfigSystem_SizeDialog(266, 103);
    float nativeScale = 0.0f;
    HFONT hNative = jonesConfigSystem_CreateFont(0, 211, &nativeScale);
    LOGFONTA nativeFont;
    TEST_ASSERT_EQUAL_INT(sizeof(nativeFont), GetObjectA(hNative, sizeof(nativeFont), &nativeFont));
    TEST_ASSERT_LESS_THAN_INT(0, nativeFont.lfHeight);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, nativeScale);

    jonesConfigSystem_renderWidth = jonesConfigSystem_desktopWidth / 2;
    jonesConfigSystem_renderHeight = jonesConfigSystem_desktopHeight / 2;
    TEST_ASSERT_GREATER_THAN_UINT32(0, jonesConfigSystem_renderWidth);
    TEST_ASSERT_GREATER_THAN_UINT32(0, jonesConfigSystem_renderHeight);
    float halfScale = 0.0f;
    HFONT hHalf = jonesConfigSystem_CreateFont(1, 211, &halfScale);
    LOGFONTA halfFont;
    TEST_ASSERT_EQUAL_INT(sizeof(halfFont), GetObjectA(hHalf, sizeof(halfFont), &halfFont));
    TEST_ASSERT_LESS_THAN_INT(0, halfFont.lfHeight);
    TEST_ASSERT_GREATER_THAN_INT(nativeFont.lfHeight, halfFont.lfHeight);
    TEST_ASSERT_FLOAT_WITHIN(0.002f, nativeScale * 0.5f, halfScale);
    TEST_ASSERT_INT_WITHIN(1, (-nativeFont.lfHeight + 1) / 2, -halfFont.lfHeight);
}

TEST(jonesConfigSystem, NativeStoreColumnsFitAfterScrollbarChanges)
{
    jonesConfigSystem_SizeDialog(538, 297);
    jonesConfigSystem_renderWidth = jonesConfigSystem_desktopWidth / 2;
    jonesConfigSystem_renderHeight = jonesConfigSystem_desktopHeight / 2;
    float scale;
    HFONT hFont = jonesConfigSystem_CreateFont(0, 190, &scale);
    JonesDialogFontInfo layout = { 0 };
    layout.fontScaleX = layout.fontScaleY = scale; // fullscreen layout
    layout.dialogID = 190;
    jonesConfig_SaveDialogImageScale(jonesConfigSystem_hDialog, &layout);
    // Reproduce the observed 1080p store's actual 12-pixel font independent of
    // the host's template rounding; the separate font case covers that path.
    LOGFONTA storeFont;
    TEST_ASSERT_EQUAL_INT(sizeof(storeFont), GetObjectA(hFont, sizeof(storeFont), &storeFont));
    storeFont.lfHeight = -12;
    DeleteObject(hFont);
    hFont = jonesConfigSystem_aFonts[0] = CreateFontIndirectA(&storeFont);
    TEST_ASSERT_NOT_NULL(hFont);
    jonesConfigSystem_hList = CreateWindowExA(0, WC_LISTVIEWA, "", WS_CHILD | LVS_REPORT | LVS_SHAREIMAGELISTS,
        0, 0, 320, 120, jonesConfigSystem_hDialog, (HMENU)(INT_PTR)1005, GetModuleHandleA(NULL), NULL);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hList);
    SendMessageA(jonesConfigSystem_hList, WM_SETFONT, (WPARAM)hFont, FALSE);

    // An authored two-color icon exercises the real image-list copies while
    // replacing only the retail-resource lookup used by the application.
    const unsigned char aAndBits[8] = { 0 };
    const unsigned char aXorBits[8] = { 0xAA, 0, 0x55, 0, 0xAA, 0, 0x55, 0 };
    jonesConfigSystem_hIcon = CreateIcon(GetModuleHandleA(NULL), 4, 4, 1, 1, aAndBits, aXorBits);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hIcon);
    tStoreCartState cart = { 0 };
    int iconsReady = jonesConfig_StoreInitItemIcons(jonesConfigSystem_hDialog, &cart);
    jonesConfigSystem_hImages = cart.hList;
    TEST_ASSERT_TRUE(iconsReady);
    int iconWidth, iconHeight;
    TEST_ASSERT_TRUE(ImageList_GetIconSize(jonesConfigSystem_hImages, &iconWidth, &iconHeight));
    // 48px at JONESCONFIG_IMAGE_REFDPI, scaled by the dialog DPI and the layout scale
    int expectedIconSize = (int)floor(48.0 * GetDpiForWindow(jonesConfigSystem_hDialog) / JONESCONFIG_IMAGE_REFDPI * scale + 0.5);
    TEST_ASSERT_INT_WITHIN(1, expectedIconSize, iconWidth);
    TEST_ASSERT_EQUAL_INT(iconWidth, iconHeight);
    ListView_SetImageList(jonesConfigSystem_hList, jonesConfigSystem_hImages, LVSIL_SMALL);

    jonesConfig_StoreSetListColumns(jonesConfigSystem_hList, "JONES_STR_COST");
    for ( int i = 0; i < 64; ++i )
    {
        LVITEMA item = { 0 };
        item.mask = LVIF_TEXT | LVIF_IMAGE;
        item.iItem = i;
        item.pszText = "Authored store item";
        item.iImage = i % (int)STD_ARRAYLEN(JonesHud_aStoreItems);
        TEST_ASSERT_EQUAL_INT(i, ListView_InsertItem(jonesConfigSystem_hList, &item));
        char aPrice[32];
        snprintf(aPrice, sizeof(aPrice), "%d", JonesHud_aStoreItems[item.iImage].price);
        ListView_SetItemText(jonesConfigSystem_hList, i, 1, aPrice);
    }
    RECT rowRect;
    TEST_ASSERT_TRUE(ListView_GetItemRect(jonesConfigSystem_hList, 0, &rowRect, LVIR_BOUNDS));
    TEST_ASSERT_GREATER_OR_EQUAL_INT(iconHeight, rowRect.bottom - rowRect.top);
    HDC hTextDC = GetDC(jonesConfigSystem_hList);
    TEST_ASSERT_NOT_NULL(hTextDC);
    HGDIOBJ hOldFont = SelectObject(hTextDC, hFont);
    TEXTMETRICA metrics;
    BOOL measured = GetTextMetricsA(hTextDC, &metrics);
    SelectObject(hTextDC, hOldFont);
    ReleaseDC(jonesConfigSystem_hList, hTextDC);
    TEST_ASSERT_TRUE(measured);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(metrics.tmHeight, rowRect.bottom - rowRect.top);
    TEST_ASSERT_GREATER_THAN_INT(ListView_GetCountPerPage(jonesConfigSystem_hList), ListView_GetItemCount(jonesConfigSystem_hList));
    // A fully hidden control defers automatic scrollbar visibility updates.
    // Give it a native vertical scrollbar while its parent remains hidden.
    TEST_ASSERT_TRUE(ShowScrollBar(jonesConfigSystem_hList, SB_VERT, TRUE));
    TEST_ASSERT_TRUE(GetWindowLongPtrA(jonesConfigSystem_hList, GWL_STYLE) & WS_VSCROLL);
    ListView_SetItemState(jonesConfigSystem_hList, 3, LVIS_SELECTED, LVIS_SELECTED);
    jonesConfig_StoreSetListColumns(jonesConfigSystem_hList, "JONES_STR_COST");
    jonesConfigSystem_AssertColumnsFit("Cost");
    TEST_ASSERT_EQUAL_INT(3, ListView_GetNextItem(jonesConfigSystem_hList, -1, LVNI_SELECTED));
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aStoreItems); ++i )
    {
        char aPrice[32];
        snprintf(aPrice, sizeof(aPrice), "%d", JonesHud_aStoreItems[i].price);
        TEST_ASSERT_GREATER_OR_EQUAL_INT(ListView_GetStringWidth(jonesConfigSystem_hList, aPrice),
            ListView_GetColumnWidth(jonesConfigSystem_hList, 1));
    }

    // Match the narrow viewports seen in the fullscreen trading post. Inspect
    // actual header/subitem bounds and the scroll origin, not only width sums.
    const int aClientWidths[] = { 134, 160, 170 };
    for ( size_t i = 0; i < STD_ARRAYLEN(aClientWidths); ++i )
    {
        RECT client, window;
        TEST_ASSERT_TRUE(GetClientRect(jonesConfigSystem_hList, &client));
        TEST_ASSERT_TRUE(GetWindowRect(jonesConfigSystem_hList, &window));
        int nonClientWidth = window.right - window.left - (client.right - client.left);
        TEST_ASSERT_TRUE(MoveWindow(jonesConfigSystem_hList, 0, 0,
            aClientWidths[i] + nonClientWidth, 120, FALSE));
        TEST_ASSERT_TRUE(ShowScrollBar(jonesConfigSystem_hList, SB_VERT, TRUE));
        jonesConfig_StoreSetListColumns(jonesConfigSystem_hList, "JONES_STR_COST");
        jonesConfigSystem_AssertColumnsFit("Cost");
    }

    jonesConfig_StoreSetListColumns(jonesConfigSystem_hList, "JONES_STR_COUNT");
    jonesConfigSystem_AssertColumnsFit("Quantity");
    TEST_ASSERT_GREATER_OR_EQUAL_INT(ListView_GetStringWidth(jonesConfigSystem_hList, "4095"),
        ListView_GetColumnWidth(jonesConfigSystem_hList, 1));
    TEST_ASSERT_TRUE(ListView_DeleteAllItems(jonesConfigSystem_hList));
    TEST_ASSERT_TRUE(ShowScrollBar(jonesConfigSystem_hList, SB_VERT, FALSE));
    jonesConfig_StoreSetListColumns(jonesConfigSystem_hList, "JONES_STR_COST");
    jonesConfigSystem_AssertColumnsFit("Cost");
    TEST_ASSERT_FALSE(IsWindowVisible(jonesConfigSystem_hDialog));
}

TEST(jonesConfigSystem, NativeBitmapDrawingPreservesMaskAndControlBounds)
{
    // The original drawing, as used with the upscaling option turned off
    jonesConfigSystem_upscalingDisabled = 1;
    HWND hImageControl = CreateWindowExA(0, "STATIC", "", WS_CHILD, 0, 0, 4, 4,
        jonesConfigSystem_hDialog, (HMENU)(INT_PTR)1182, GetModuleHandleA(NULL), NULL);
    TEST_ASSERT_NOT_NULL(hImageControl);
    uint32_t *pImage, *pMask, *pDestination;
    jonesConfigSystem_hImage = jonesConfigSystem_CreateDIB(2, 2, &pImage);
    jonesConfigSystem_hMask = jonesConfigSystem_CreateDIB(2, 2, &pMask);
    jonesConfigSystem_hDestination = jonesConfigSystem_CreateDIB(8, 6, &pDestination);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hImage);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hMask);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hDestination);
    const uint32_t aImage[4] = { 0x00FF0000, 0, 0x000000FF, 0 };
    memcpy(pImage, aImage, sizeof(aImage));
    pMask[0] = pMask[2] = 0;
    pMask[1] = pMask[3] = 0x00FFFFFF;
    jonesConfigSystem_hSourceDC = CreateCompatibleDC(NULL);
    jonesConfigSystem_hDestDC = CreateCompatibleDC(NULL);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hSourceDC);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hDestDC);
    jonesConfigSystem_hOldSource = SelectObject(jonesConfigSystem_hSourceDC, jonesConfigSystem_hImage);
    jonesConfigSystem_hOldDest = SelectObject(jonesConfigSystem_hDestDC, jonesConfigSystem_hDestination);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hOldSource);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hOldDest);
    TEST_ASSERT_NOT_EQUAL_INT(0, SetStretchBltMode(jonesConfigSystem_hDestDC, COLORONCOLOR));

    // The transparent source pixels are black; SRCAND then SRCPAINT must
    // retain the destination there. Both paths must leave every pixel outside
    // the 4x4 control unchanged. BI_RGB's unused high byte is not an alpha value.
    const uint32_t background = 0x00123456;
    for ( int masked = 0; masked <= 1; ++masked )
    {
        for ( int i = 0; i < 8 * 6; ++i ) pDestination[i] = background;
        TEST_ASSERT_TRUE(jonesConfig_DrawImageOnDialogItem(jonesConfigSystem_hDialog,
            jonesConfigSystem_hSourceDC, jonesConfigSystem_hDestDC, 1182,
            jonesConfigSystem_hImage, masked ? jonesConfigSystem_hMask : NULL));
        TEST_ASSERT_TRUE(GdiFlush());
        for ( int y = 0; y < 6; ++y )
        {
            for ( int x = 0; x < 8; ++x )
            {
                uint32_t expected = background;
                if ( x < 4 && y < 4 )
                {
                    int source = (y / 2) * 2 + x / 2;
                    if ( !masked || aImage[source] ) expected = aImage[source];
                }
                TEST_ASSERT_EQUAL_HEX32(expected, pDestination[y * 8 + x] & 0x00FFFFFF);
            }
        }
    }
}

static void jonesConfigSystem_PrepareDrawing(int imageSize, int destWidth, int destHeight, uint32_t** ppImage, uint32_t** ppMask, uint32_t** ppDestination)
{
    jonesConfigSystem_hImage = jonesConfigSystem_CreateDIB(imageSize, imageSize, ppImage);
    jonesConfigSystem_hMask = jonesConfigSystem_CreateDIB(imageSize, imageSize, ppMask);
    jonesConfigSystem_hDestination = jonesConfigSystem_CreateDIB(destWidth, destHeight, ppDestination);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hImage);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hMask);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hDestination);
    jonesConfigSystem_hSourceDC = CreateCompatibleDC(NULL);
    jonesConfigSystem_hDestDC = CreateCompatibleDC(NULL);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hSourceDC);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hDestDC);
    jonesConfigSystem_hOldSource = SelectObject(jonesConfigSystem_hSourceDC, jonesConfigSystem_hImage);
    jonesConfigSystem_hOldDest = SelectObject(jonesConfigSystem_hDestDC, jonesConfigSystem_hDestination);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hOldSource);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hOldDest);
    TEST_ASSERT_NOT_EQUAL_INT(0, SetStretchBltMode(jonesConfigSystem_hDestDC, COLORONCOLOR));
}

static uint32_t jonesConfigSystem_BrushPixel(HBRUSH hBrush)
{
    LOGBRUSH brush = { 0 };
    TEST_ASSERT_EQUAL_INT(sizeof(brush), GetObjectA(hBrush, sizeof(brush), &brush));
    return ((uint32_t)GetRValue(brush.lbColor) << 16) | ((uint32_t)GetGValue(brush.lbColor) << 8) | GetBValue(brush.lbColor);
}

TEST(jonesConfigSystem, NativeMaskedImagesAreUpscaledOverTheDialogBackground)
{
    HWND hImageControl = CreateWindowExA(0, "STATIC", "", WS_CHILD, 0, 0, 24, 24,
        jonesConfigSystem_hDialog, (HMENU)(INT_PTR)1182, GetModuleHandleA(NULL), NULL);
    TEST_ASSERT_NOT_NULL(hImageControl);
    uint32_t *pImage, *pMask, *pDestination;
    jonesConfigSystem_PrepareDrawing(8, 32, 28, &pImage, &pMask, &pDestination);

    // An opaque red square in the middle of a transparent 8x8 image, enlarged 3x
    for ( int y = 0; y < 8; ++y )
    {
        for ( int x = 0; x < 8; ++x )
        {
            int bInside = x >= 2 && x < 6 && y >= 2 && y < 6;
            pImage[y * 8 + x] = bInside ? 0x00FF0000u : 0u;
            pMask[y * 8 + x] = bInside ? 0u : 0x00FFFFFFu;
        }
    }

    const uint32_t background = 0x00123456;
    for ( int i = 0; i < 32 * 28; ++i ) pDestination[i] = background;
    TEST_ASSERT_TRUE(jonesConfig_DrawImageOnDialogItem(jonesConfigSystem_hDialog,
        jonesConfigSystem_hSourceDC, jonesConfigSystem_hDestDC, 1182, jonesConfigSystem_hImage, jonesConfigSystem_hMask));
    TEST_ASSERT_TRUE(GdiFlush());

    // Transparent areas show the dialog's control background instead of a previous image
    HBRUSH hBrush = (HBRUSH)SendMessageA(jonesConfigSystem_hDialog, WM_CTLCOLORSTATIC, (WPARAM)jonesConfigSystem_hDestDC, (LPARAM)hImageControl);
    uint32_t dialogBackground = jonesConfigSystem_BrushPixel(hBrush ? hBrush : GetSysColorBrush(COLOR_BTNFACE));
    for ( int y = 0; y < 28; ++y )
    {
        for ( int x = 0; x < 32; ++x )
        {
            uint32_t pixel = pDestination[y * 32 + x] & 0x00FFFFFF;
            if ( x >= 24 || y >= 24 )
            {
                TEST_ASSERT_EQUAL_HEX32(background, pixel); // outside the control
            }
            else if ( x >= 8 && x < 16 && y >= 8 && y < 16 )
            {
                TEST_ASSERT_EQUAL_HEX32(0x00FF0000, pixel); // inside the enlarged square
            }
            else if ( (x < 4 || x >= 20) && (y < 4 || y >= 20) )
            {
                TEST_ASSERT_EQUAL_HEX32(dialogBackground, pixel); // far from the square
            }
        }
    }
}

TEST(jonesConfigSystem, NativeColorFilterMasksKeepTheOriginalDrawing)
{
    // The IQ image draws its points into the mask, which the original ANDs with the background
    HWND hImageControl = CreateWindowExA(0, "STATIC", "", WS_CHILD, 0, 0, 4, 4,
        jonesConfigSystem_hDialog, (HMENU)(INT_PTR)1182, GetModuleHandleA(NULL), NULL);
    TEST_ASSERT_NOT_NULL(hImageControl);
    uint32_t *pImage, *pMask, *pDestination;
    jonesConfigSystem_PrepareDrawing(2, 4, 4, &pImage, &pMask, &pDestination);
    const uint32_t aImage[4] = { 0x00FF0000, 0, 0x000000FF, 0 };
    const uint32_t aMask[4] = { 0, 0x00808080, 0, 0x00FFFFFF };
    memcpy(pImage, aImage, sizeof(aImage));
    memcpy(pMask, aMask, sizeof(aMask));

    const uint32_t background = 0x00F0E0D0;
    for ( int i = 0; i < 16; ++i ) pDestination[i] = background;
    TEST_ASSERT_TRUE(jonesConfig_DrawImageOnDialogItem(jonesConfigSystem_hDialog,
        jonesConfigSystem_hSourceDC, jonesConfigSystem_hDestDC, 1182, jonesConfigSystem_hImage, jonesConfigSystem_hMask));
    TEST_ASSERT_TRUE(GdiFlush());
    for ( int y = 0; y < 4; ++y )
    {
        for ( int x = 0; x < 4; ++x )
        {
            int source = (y / 2) * 2 + x / 2;
            uint32_t expected = (background & aMask[source]) | aImage[source]; // SRCAND, then SRCPAINT
            TEST_ASSERT_EQUAL_HEX32(expected, pDestination[y * 4 + x] & 0x00FFFFFF);
        }
    }
}

TEST(jonesConfigSystem, NativeChildDialogGrowsInPlaceInsideItsParent)
{
    // Off the screen origin, so screen and parent client coordinates differ
    TEST_ASSERT_TRUE(MoveWindow(jonesConfigSystem_hDialog, 200, 150, 600, 400, FALSE));

    // A child dialog like the load/save template inside the file dialog
    HWND hChild = CreateWindowExA(0, "STATIC", "", WS_CHILD, 10, 20, 100, 80,
        jonesConfigSystem_hDialog, NULL, GetModuleHandleA(NULL), NULL);
    TEST_ASSERT_NOT_NULL(hChild);
    HWND hCtrl = CreateWindowExA(0, "STATIC", "", WS_CHILD, 5, 5, 40, 30,
        hChild, (HMENU)(INT_PTR)1500, GetModuleHandleA(NULL), NULL);
    TEST_ASSERT_NOT_NULL(hCtrl);

    JonesDialogFontInfo fontInfo = { 0 };
    fontInfo.fontScaleX = fontInfo.fontScaleY = 3.0f; // fullscreen layout
    fontInfo.dialogID = 211;
    jonesConfig_SetWindowFontAndPosition(hCtrl, &fontInfo);

    RECT ctrlRect;
    TEST_ASSERT_TRUE(GetWindowRect(hCtrl, &ctrlRect));
    MapWindowPoints(HWND_DESKTOP, hChild, (LPPOINT)&ctrlRect, 2);
    TEST_ASSERT_EQUAL_INT(15, ctrlRect.left);
    TEST_ASSERT_EQUAL_INT(15, ctrlRect.top);
    TEST_ASSERT_EQUAL_INT(120, ctrlRect.right - ctrlRect.left);
    TEST_ASSERT_EQUAL_INT(90, ctrlRect.bottom - ctrlRect.top);

    // The child grows to fit the control and stays where it was in its parent
    RECT childRect;
    TEST_ASSERT_TRUE(GetWindowRect(hChild, &childRect));
    MapWindowPoints(HWND_DESKTOP, jonesConfigSystem_hDialog, (LPPOINT)&childRect, 2);
    TEST_ASSERT_EQUAL_INT(10, childRect.left);
    TEST_ASSERT_EQUAL_INT(20, childRect.top);
    TEST_ASSERT_EQUAL_INT(135, childRect.right - childRect.left);
    TEST_ASSERT_EQUAL_INT(105, childRect.bottom - childRect.top);
}

static RECT jonesConfigSystem_GetChildRect(HWND hCtrl)
{
    RECT rect;
    TEST_ASSERT_TRUE(GetWindowRect(hCtrl, &rect));
    MapWindowPoints(HWND_DESKTOP, GetParent(hCtrl), (LPPOINT)&rect, 2);
    return rect;
}

TEST(jonesConfigSystem, NativeFilmIconMovesDownWithTheThumbnailOnlyWhenBelowIt)
{
    TEST_ASSERT_TRUE(MoveWindow(jonesConfigSystem_hDialog, 200, 150, 600, 400, FALSE));
    HWND hBelow = CreateWindowExA(0, "STATIC", "", WS_CHILD, 10, 100, 32, 32,
        jonesConfigSystem_hDialog, (HMENU)(INT_PTR)1117, GetModuleHandleA(NULL), NULL);
    HWND hAbove = CreateWindowExA(0, "STATIC", "", WS_CHILD, 10, 20, 32, 32,
        jonesConfigSystem_hDialog, (HMENU)(INT_PTR)1115, GetModuleHandleA(NULL), NULL);
    TEST_ASSERT_NOT_NULL(hBelow);
    TEST_ASSERT_NOT_NULL(hAbove);

    // Thumbnail bottom at client y 60 (screen 210), enlarged by 25 pixels
    LONG thumbBottom = 150 + 60;
    jonesConfig_MoveBelowThumbnail(jonesConfigSystem_hDialog, hBelow, thumbBottom, 25);
    jonesConfig_MoveBelowThumbnail(jonesConfigSystem_hDialog, hAbove, thumbBottom, 25);

    RECT below = jonesConfigSystem_GetChildRect(hBelow);
    TEST_ASSERT_EQUAL_INT(10, below.left);
    TEST_ASSERT_EQUAL_INT(125, below.top);
    TEST_ASSERT_EQUAL_INT(32, below.bottom - below.top);
    RECT above = jonesConfigSystem_GetChildRect(hAbove);
    TEST_ASSERT_EQUAL_INT(20, above.top);

    // No growth, no move
    jonesConfig_MoveBelowThumbnail(jonesConfigSystem_hDialog, hBelow, thumbBottom, 0);
    TEST_ASSERT_EQUAL_INT(125, jonesConfigSystem_GetChildRect(hBelow).top);
}

TEST(jonesConfigSystem, NativeSliderThumbScalesOnceWithTheDialogDpi)
{
    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_BAR_CLASSES };
    TEST_ASSERT_TRUE(InitCommonControlsEx(&controls));
    UINT dpi = GetDpiForWindow(jonesConfigSystem_hDialog);
    HWND hSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "", WS_CHILD | TBS_HORZ | TBS_AUTOTICKS,
        0, 0, 200, MulDiv(60, dpi, USER_DEFAULT_SCREEN_DPI), jonesConfigSystem_hDialog, (HMENU)(INT_PTR)1216, GetModuleHandleA(NULL), NULL);
    TEST_ASSERT_NOT_NULL(hSlider);
    UINT baseLength = (UINT)SendMessageA(hSlider, TBM_GETTHUMBLENGTH, 0, 0);
    TEST_ASSERT_GREATER_THAN_UINT(0, baseLength);

    RECT client;
    TEST_ASSERT_TRUE(GetClientRect(hSlider, &client));
    LONG expected = (LONG)floor((double)baseLength * dpi / USER_DEFAULT_SCREEN_DPI + 0.5);
    if ( expected > client.bottom ) expected = client.bottom;

    // Windowed layout at 1.0 doesn't resize the slider, so only the dialog DPI applies
    JonesDialogFontInfo fontInfo = { 0 };
    fontInfo.bWindowMode = 1;
    fontInfo.fontScaleX = fontInfo.fontScaleY = 1.0f;
    fontInfo.dialogID = 148;
    for ( int pass = 0; pass < 2; ++pass ) // a second layout pass must not scale it again
    {
        jonesConfig_ScaleTrackbarThumb(hSlider, &fontInfo);
        TEST_ASSERT_EQUAL_INT(expected, (LONG)SendMessageA(hSlider, TBM_GETTHUMBLENGTH, 0, 0));
    }
    TEST_ASSERT_TRUE(GetWindowLongPtrA(hSlider, GWL_STYLE) & TBS_FIXEDLENGTH);

    DWORD_PTR data = 0;
    TEST_ASSERT_TRUE(GetWindowSubclass(hSlider, jonesConfig_DataSubclassProc, JONESCONFIG_SUBCLASSID, &data));
    TEST_ASSERT_EQUAL_UINT(baseLength, (UINT)data); // the native length is kept, not the scaled one
    TEST_ASSERT_TRUE(DestroyWindow(hSlider));
}

TEST(jonesConfigSystem, NativeCheckBoxHookDrawsTheBoxAtTheFontSize)
{
    HWND hCheckBox = CreateWindowExA(0, "BUTTON", "Option", WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 200, 40, jonesConfigSystem_hDialog, (HMENU)(INT_PTR)1053, GetModuleHandleA(NULL), NULL);
    TEST_ASSERT_NOT_NULL(hCheckBox);
    LOGFONTA font = { 0 };
    font.lfHeight = -24;
    STD_STRCPY(font.lfFaceName, "Segoe UI");
    jonesConfigSystem_aFonts[0] = CreateFontIndirectA(&font);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_aFonts[0]);
    SendMessageA(hCheckBox, WM_SETFONT, (WPARAM)jonesConfigSystem_aFonts[0], FALSE);

    jonesConfig_InstallCheckBoxHook(hCheckBox);
    jonesConfig_InstallCheckBoxHook(hCheckBox); // installed once
    DWORD_PTR data = 0;
    TEST_ASSERT_TRUE(GetWindowSubclass(hCheckBox, jonesConfig_CheckBoxSubclassProc, JONESCONFIG_SUBCLASSID, &data));

    // Windows still keeps the check state
    SendMessageA(hCheckBox, BM_SETCHECK, BST_CHECKED, 0);
    TEST_ASSERT_EQUAL_INT(BST_CHECKED, (int)SendMessageA(hCheckBox, BM_GETCHECK, 0, 0));

    uint32_t* pPixels;
    jonesConfigSystem_hDestination = jonesConfigSystem_CreateDIB(200, 40, &pPixels);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hDestination);
    jonesConfigSystem_hDestDC = CreateCompatibleDC(NULL);
    TEST_ASSERT_NOT_NULL(jonesConfigSystem_hDestDC);
    jonesConfigSystem_hOldDest = SelectObject(jonesConfigSystem_hDestDC, jonesConfigSystem_hDestination);
    SendMessageA(hCheckBox, WM_PRINTCLIENT, (WPARAM)jonesConfigSystem_hDestDC, PRF_CLIENT);
    TEST_ASSERT_TRUE(GdiFlush());

    // The box starts at the left edge; its width is the first run of pixels that differ
    // from the background in the middle row
    HGDIOBJ hOldFont = SelectObject(jonesConfigSystem_hDestDC, jonesConfigSystem_aFonts[0]);
    TEXTMETRICA metrics;
    TEST_ASSERT_TRUE(GetTextMetricsA(jonesConfigSystem_hDestDC, &metrics));
    SelectObject(jonesConfigSystem_hDestDC, hOldFont);
    int expectedBox = metrics.tmHeight < 40 ? metrics.tmHeight : 40;

    const uint32_t* pRow = &pPixels[20 * 200];
    uint32_t background = pRow[199] & 0x00FFFFFF; // right of the short text
    int boxWidth = 0;
    while ( boxWidth < 200 && (pRow[boxWidth] & 0x00FFFFFF) != background )
    {
        ++boxWidth;
    }
    TEST_ASSERT_INT_WITHIN(1, expectedBox, boxWidth);
    TEST_ASSERT_TRUE(DestroyWindow(hCheckBox));
}

static HICON jonesConfigSystem_CreateAlphaIcon(int size, bool bTriangle)
{
    uint32_t* pColor;
    HBITMAP hColor = jonesConfigSystem_CreateDIB(size, size, &pColor);
    HBITMAP hMask = CreateBitmap(size, size, 1, 1, NULL);
    TEST_ASSERT_NOT_NULL(hColor);
    TEST_ASSERT_NOT_NULL(hMask);
    for ( int y = 0; y < size; ++y )
    {
        for ( int x = 0; x < size; ++x )
        {
            bool bInside = bTriangle ? x <= y : (x >= size / 4 && x < size * 3 / 4 && y >= size / 4 && y < size * 3 / 4);
            pColor[y * size + x] = bInside ? 0xFFFF0000u : 0u;
        }
    }
    ICONINFO info = { 0 };
    info.fIcon = TRUE;
    info.hbmColor = hColor;
    info.hbmMask = hMask;
    HICON hIcon = CreateIconIndirect(&info);
    DeleteObject(hColor);
    DeleteObject(hMask);
    TEST_ASSERT_NOT_NULL(hIcon);
    return hIcon;
}

TEST(jonesConfigSystem, NativeUpscaledIconsKeepTheirAlphaOrBlendIntoTheListBackground)
{
    // Kept alpha, enlarged 3x
    HICON hSquare = jonesConfigSystem_CreateAlphaIcon(16, false);
    HICON hScaled = jonesConfig_UpscaleIcon(hSquare, 48, 48, CLR_NONE);
    TEST_ASSERT_NOT_NULL(hScaled);
    int width = 0, height = 0;
    uint32_t* pPixels = jonesConfig_GetIconPixels(hScaled, &width, &height);
    TEST_ASSERT_NOT_NULL(pPixels);
    TEST_ASSERT_EQUAL_INT(48, width);
    TEST_ASSERT_EQUAL_INT(48, height);
    TEST_ASSERT_EQUAL_HEX32(0xFFFF0000u, pPixels[24 * 48 + 24]); // center
    TEST_ASSERT_EQUAL_HEX32(0u, pPixels[0] >> 24);               // corner stays transparent
    free(pPixels);
    DestroyIcon(hScaled);
    DestroyIcon(hSquare);

    // Blended into a white list background at the same size: smooth but only fully opaque or transparent
    HICON hTriangle = jonesConfigSystem_CreateAlphaIcon(16, true);
    hScaled = jonesConfig_UpscaleIcon(hTriangle, 16, 16, RGB(255, 255, 255));
    TEST_ASSERT_NOT_NULL(hScaled);
    pPixels = jonesConfig_GetIconPixels(hScaled, &width, &height);
    TEST_ASSERT_NOT_NULL(pPixels);
    TEST_ASSERT_EQUAL_INT(16, width);
    int numBlended = 0;
    for ( int i = 0; i < 16 * 16; ++i )
    {
        uint32_t a = pPixels[i] >> 24;
        TEST_ASSERT_TRUE(a == 0 || a == 0xFF);
        uint32_t rgb = pPixels[i] & 0x00FFFFFFu;
        numBlended += a == 0xFF && rgb != 0x00FF0000u; // red mixed with white along the diagonal
    }
    free(pPixels);
    TEST_ASSERT_GREATER_THAN_INT(0, numBlended);
    DestroyIcon(hScaled);
    DestroyIcon(hTriangle);
}

TEST_GROUP_RUNNER(jonesConfigSystem)
{
    RUN_TEST_CASE(jonesConfigSystem, NativeFontsFollowTheFullscreenPresentationScale);
    RUN_TEST_CASE(jonesConfigSystem, NativeStoreColumnsFitAfterScrollbarChanges);
    RUN_TEST_CASE(jonesConfigSystem, NativeBitmapDrawingPreservesMaskAndControlBounds);
    RUN_TEST_CASE(jonesConfigSystem, NativeMaskedImagesAreUpscaledOverTheDialogBackground);
    RUN_TEST_CASE(jonesConfigSystem, NativeColorFilterMasksKeepTheOriginalDrawing);
    RUN_TEST_CASE(jonesConfigSystem, NativeChildDialogGrowsInPlaceInsideItsParent);
    RUN_TEST_CASE(jonesConfigSystem, NativeFilmIconMovesDownWithTheThumbnailOnlyWhenBelowIt);
    RUN_TEST_CASE(jonesConfigSystem, NativeSliderThumbScalesOnceWithTheDialogDpi);
    RUN_TEST_CASE(jonesConfigSystem, NativeCheckBoxHookDrawsTheBoxAtTheFontSize);
    RUN_TEST_CASE(jonesConfigSystem, NativeUpscaledIconsKeepTheirAlphaOrBlendIntoTheListBackground);
}

static void jonesConfigSystem_RunAll(void)
{
    RUN_TEST_GROUP(jonesConfigSystem);
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, jonesConfigSystem_RunAll);
}
