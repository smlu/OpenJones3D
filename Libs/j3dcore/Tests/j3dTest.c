#include "j3dTest.h"

#include <stdio.h>
#include <string.h>

#include <unity.h>

#include <std/types.h>

#define J3DTEST_MAX_HOST_SERVICE_BINDINGS 8

typedef struct J3DTestHostServicesBinding
{
    uintptr_t address;
    tHostServices* pSavedHostServices;
} J3DTestHostServicesBinding;

static tHostServices j3dTest_hostServices;
static J3DTestHostServicesBinding j3dTest_aHostServiceBindings[J3DTEST_MAX_HOST_SERVICE_BINDINGS];
static size_t j3dTest_numHostServiceBindings;
static bool j3dTest_bHostServicesActive;

static int j3dTest_numAsserts;
static char j3dTest_aAssertText[128];
static char j3dTest_aAssertFile[64];
static int j3dTest_assertLine;

static void J3DTest_CopyString(char* pDst, size_t dstSize, const char* pSrc)
{
    if ( dstSize == 0 )
    {
        return;
    }

    snprintf(pDst, dstSize, "%s", pSrc ? pSrc : "");
    pDst[dstSize - 1] = '\0';
}

static void J3DAPI J3DTest_RecordAssert(const char* pText, const char* pFile, int line)
{
    ++j3dTest_numAsserts;

    J3DTest_CopyString(j3dTest_aAssertText, sizeof(j3dTest_aAssertText), pText);
    J3DTest_CopyString(j3dTest_aAssertFile, sizeof(j3dTest_aAssertFile), pFile);
    j3dTest_assertLine = line;
}

void J3DTest_MapRtiMemory(uintptr_t address, size_t size)
{
#if defined(J3D_OS_WINDOWS)
    SYSTEM_INFO systemInfo;
    uintptr_t allocationGranularity;
    uintptr_t pageSize;
    uintptr_t allocationBase;
    uintptr_t endAddress;
    uintptr_t allocationEnd;
    SIZE_T allocationSize;
    MEMORY_BASIC_INFORMATION memoryInfo;
    void* pMappedPage;

    TEST_ASSERT_NOT_EQUAL(0u, address);
    TEST_ASSERT_NOT_EQUAL(0u, size);

    GetSystemInfo(&systemInfo);
    allocationGranularity = (uintptr_t)systemInfo.dwAllocationGranularity;
    pageSize = (uintptr_t)systemInfo.dwPageSize;

    endAddress = address + size;
    TEST_ASSERT_TRUE(endAddress >= address);

    allocationBase = address & ~(allocationGranularity - 1u);
    allocationEnd = (endAddress + pageSize - 1u) & ~(pageSize - 1u);
    allocationSize = (SIZE_T)(allocationEnd - allocationBase);

    if ( VirtualQuery((LPCVOID)address, &memoryInfo, sizeof(memoryInfo)) == sizeof(memoryInfo)
        && memoryInfo.State == MEM_COMMIT
        && (uintptr_t)memoryInfo.BaseAddress <= address
        && (uintptr_t)memoryInfo.BaseAddress + memoryInfo.RegionSize >= endAddress )
    {
        return;
    }

    pMappedPage = VirtualAlloc((LPVOID)allocationBase, allocationSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    TEST_ASSERT_EQUAL_PTR((void*)allocationBase, pMappedPage);
#else
#   error "Add fixed-address RTI memory mapping for this platform before enabling these tests."
#endif
}

void J3DTest_Startup(void)
{
    TEST_ASSERT_FALSE(j3dTest_bHostServicesActive);

    memset(&j3dTest_hostServices, 0, sizeof(j3dTest_hostServices));
    memset(j3dTest_aHostServiceBindings, 0, sizeof(j3dTest_aHostServiceBindings));
    j3dTest_numHostServiceBindings = 0;
    j3dTest_bHostServicesActive    = true;

    j3dTest_hostServices.pAssert = J3DTest_RecordAssert;
    J3DTest_ResetAssertCapture();
}

void J3DTest_BindHostServicesGlobal(uintptr_t hostServicesAddress)
{
    tHostServices** ppHostServices;

    TEST_ASSERT_TRUE(j3dTest_bHostServicesActive);
    TEST_ASSERT_TRUE(j3dTest_numHostServiceBindings < J3DTEST_MAX_HOST_SERVICE_BINDINGS);

    for ( size_t i = 0; i < j3dTest_numHostServiceBindings; ++i )
    {
        TEST_ASSERT_FALSE(j3dTest_aHostServiceBindings[i].address == hostServicesAddress);
    }

    J3DTest_MapRtiMemory(hostServicesAddress, sizeof(tHostServices*));

    ppHostServices = (tHostServices**)hostServicesAddress;
    j3dTest_aHostServiceBindings[j3dTest_numHostServiceBindings].address = hostServicesAddress;
    j3dTest_aHostServiceBindings[j3dTest_numHostServiceBindings].pSavedHostServices = *ppHostServices;
    ++j3dTest_numHostServiceBindings;

    *ppHostServices = &j3dTest_hostServices;
}

void J3DTest_Shutdown(void)
{
    TEST_ASSERT_TRUE(j3dTest_bHostServicesActive);

    while ( j3dTest_numHostServiceBindings > 0 )
    {
        J3DTestHostServicesBinding* pBinding;
        tHostServices** ppHostServices;

        --j3dTest_numHostServiceBindings;
        pBinding = &j3dTest_aHostServiceBindings[j3dTest_numHostServiceBindings];
        ppHostServices = (tHostServices**)pBinding->address;
        if ( ppHostServices )
        {
            *ppHostServices = pBinding->pSavedHostServices;
        }

        pBinding->address = 0;
        pBinding->pSavedHostServices = NULL;
    }

    j3dTest_bHostServicesActive = false;
}

tHostServices* J3DTest_GetHostServices(void)
{
    return &j3dTest_hostServices;
}

void J3DTest_ResetAssertCapture(void)
{
    j3dTest_numAsserts     = 0;
    j3dTest_aAssertText[0] = '\0';
    j3dTest_aAssertFile[0] = '\0';
    j3dTest_assertLine     = 0;
}

int J3DTest_GetAssertCount(void)
{
    return j3dTest_numAsserts;
}

const char* J3DTest_GetLastAssertText(void)
{
    return j3dTest_aAssertText;
}

const char* J3DTest_GetLastAssertFile(void)
{
    return j3dTest_aAssertFile;
}

int J3DTest_GetLastAssertLine(void)
{
    return j3dTest_assertLine;
}

void J3DTest_AssertLastAssert(const char* pExpectedText, const char* pExpectedFileFragment)
{
    TEST_ASSERT_EQUAL_STRING(pExpectedText, J3DTest_GetLastAssertText());
    TEST_ASSERT_NOT_NULL(strstr(J3DTest_GetLastAssertFile(), pExpectedFileFragment));
    TEST_ASSERT_TRUE(J3DTest_GetLastAssertLine() > 0);
}