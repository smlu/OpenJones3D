#include <unity_fixture.h>
#include <Jones3D/Display/jonesConfig.h>
#include <stdio.h>
#include <string.h>

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
    return 0;
}

JonesDisplaySettings* JonesMain_GetDisplaySettings(void)
{
    static JonesDisplaySettings settings;
    return &settings;
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

TEST_GROUP_RUNNER(jonesConfig)
{
    RUN_TEST_CASE(jonesConfig, EightBindingsFitTheActionRow);
    RUN_TEST_CASE(jonesConfig, NinthBindingDoesNotOverwriteNextRowOrName);
    RUN_TEST_CASE(jonesConfig, FailedLoadClearsActionsAccordingToProfile);
    RUN_TEST_CASE(jonesConfig, OpenFailurePreservesScheme);
    RUN_TEST_CASE(jonesConfig, KeyNameFailureReturnsAnEmptyTerminatedString);
    RUN_TEST_CASE(jonesConfig, KeyNameSuccessUsesTheReturnedName);
}

static void JonesConfigTest_RunAllTests(void)
{
    RUN_TEST_GROUP(jonesConfig);
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, JonesConfigTest_RunAllTests);
}
