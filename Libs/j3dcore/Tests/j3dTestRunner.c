#include "j3dTestRunner.h"

#include <unity_fixture.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The run function of UnityMain takes no arguments, so the selection lives here for the run
static const J3DTestSuite* J3DTest_aRunSuites;
static size_t J3DTest_numRunSuites;
static bool* J3DTest_aRunSelected;

static const char* J3DTest_GetKindName(J3DTestSuiteKind kind)
{
    switch ( kind )
    {
        case J3DTEST_SUITE_SYSTEM:
            return "system";
        case J3DTEST_SUITE_DISRUPTIVE:
            return "disruptive";
        default:
            return "unit";
    }
}

static int J3DTest_FindSuite(const J3DTestSuite* aSuites, size_t numSuites, const char* pName, size_t nameLen)
{
    for ( size_t i = 0; i < numSuites; ++i )
    {
        if ( strlen(aSuites[i].pName) == nameLen && strncmp(aSuites[i].pName, pName, nameLen) == 0 )
        {
            return (int)i;
        }
    }

    return -1;
}

static void J3DTest_PrintSuites(FILE* pFile, const J3DTestSuite* aSuites, size_t numSuites)
{
    for ( size_t i = 0; i < numSuites; ++i )
    {
        fprintf(pFile, "%s %s\n", aSuites[i].pName, J3DTest_GetKindName(aSuites[i].kind));
    }
}

// Marks the suites of a comma separated list in aMarked. Returns false and reports the first unknown name.
static bool J3DTest_MarkSuites(const char* pOption, const char* pList, const J3DTestSuite* aSuites, size_t numSuites, bool* aMarked)
{
    const char* pCur = pList;
    for ( ;; )
    {
        const char* pEnd = strchr(pCur, ',');
        const size_t len = pEnd ? (size_t)(pEnd - pCur) : strlen(pCur);
        if ( len > 0 )
        {
            const int idx = J3DTest_FindSuite(aSuites, numSuites, pCur, len);
            if ( idx < 0 )
            {
                fprintf(stderr, "%s: unknown suite '%.*s'. Suites in this executable:\n", pOption, (int)len, pCur);
                J3DTest_PrintSuites(stderr, aSuites, numSuites);
                return false;
            }

            aMarked[idx] = true;
        }

        if ( !pEnd )
        {
            return true;
        }

        pCur = pEnd + 1;
    }
}

// Parses a comma separated list of suite kinds. Returns 0 for an unknown kind.
static int J3DTest_ParseKinds(const char* pList)
{
    int kinds = 0;
    const char* pCur = pList;
    for ( ;; )
    {
        const char* pEnd = strchr(pCur, ',');
        const size_t len = pEnd ? (size_t)(pEnd - pCur) : strlen(pCur);
        if ( len == 4 && strncmp(pCur, "unit", 4) == 0 )
        {
            kinds |= J3DTEST_SUITE_UNIT;
        }
        else if ( len == 6 && strncmp(pCur, "system", 6) == 0 )
        {
            kinds |= J3DTEST_SUITE_SYSTEM;
        }
        else if ( len == 10 && strncmp(pCur, "disruptive", 10) == 0 )
        {
            kinds |= J3DTEST_SUITE_DISRUPTIVE;
        }
        else if ( len > 0 )
        {
            fprintf(stderr, "--kind: unknown kind '%.*s', expected unit, system or disruptive\n", (int)len, pCur);
            return 0;
        }

        if ( !pEnd )
        {
            return kinds;
        }

        pCur = pEnd + 1;
    }
}

// Returns the value of an option given as "--name value" or "--name=value", or NULL if argv[*pIdx] is not the option.
// Sets *pbMissing when the value is missing.
static const char* J3DTest_GetOptionValue(int argc, const char* argv[], int* pIdx, const char* pName, bool* pbMissing)
{
    const char* pArg = argv[*pIdx];
    const size_t nameLen = strlen(pName);
    if ( strncmp(pArg, pName, nameLen) != 0 )
    {
        return NULL;
    }

    if ( pArg[nameLen] == '=' )
    {
        return pArg + nameLen + 1;
    }

    if ( pArg[nameLen] != '\0' )
    {
        return NULL;
    }

    if ( *pIdx + 1 >= argc )
    {
        *pbMissing = true;
        return NULL;
    }

    *pIdx += 1;
    return argv[*pIdx];
}

static void J3DTest_PrintHelp(void)
{
    printf("Test suite options:\n");
    printf("  --suite A,B           Only run these suites (exact names, can be repeated)\n");
    printf("  --exclude-suite C,D   Run all but these suites (can be repeated)\n");
    printf("  --kind KINDS          Only run suites of these kinds: unit, system, disruptive\n");
    printf("  --exclude-system      Skip system and disruptive suites\n");
    printf("  --list                List the suites and their kind\n");
    printf("All suites run by default. Other options are Unity options:\n\n");
}

static void J3DTest_RunSelectedSuites(void)
{
    for ( size_t i = 0; i < J3DTest_numRunSuites; ++i )
    {
        if ( J3DTest_aRunSelected[i] )
        {
            J3DTest_aRunSuites[i].pfRun();
        }
    }
}

int J3DTest_RunSuites(int argc, const char* argv[], const J3DTestSuite* aSuites, size_t numSuites)
{
    // Room for the Unity arguments and a "-h" when help is requested
    const char** aUnityArgs = (const char**)calloc((size_t)argc + 2, sizeof(const char*));
    bool* aIncluded         = (bool*)calloc(numSuites + 1, sizeof(bool));
    bool* aExcluded         = (bool*)calloc(numSuites + 1, sizeof(bool));
    bool* aSelected         = (bool*)calloc(numSuites + 1, sizeof(bool));
    if ( !aUnityArgs || !aIncluded || !aExcluded || !aSelected )
    {
        fprintf(stderr, "Out of memory\n");
        free(aUnityArgs);
        free(aIncluded);
        free(aExcluded);
        free(aSelected);
        return 2;
    }

    int result         = 0;
    int numUnityArgs   = 0;
    int kinds          = 0; // Kinds given with --kind, 0 runs every kind
    int excludedKinds  = 0;
    bool bHasInclude   = false;
    bool bList         = false;
    bool bHelp         = false;

    aUnityArgs[numUnityArgs++] = argc > 0 ? argv[0] : "";
    for ( int i = 1; i < argc && result == 0; ++i )
    {
        bool bMissing = false;
        const char* pValue;
        if ( (pValue = J3DTest_GetOptionValue(argc, argv, &i, "--suite", &bMissing)) != NULL )
        {
            bHasInclude = true;
            result = J3DTest_MarkSuites("--suite", pValue, aSuites, numSuites, aIncluded) ? 0 : 2;
        }
        else if ( !bMissing && (pValue = J3DTest_GetOptionValue(argc, argv, &i, "--exclude-suite", &bMissing)) != NULL )
        {
            result = J3DTest_MarkSuites("--exclude-suite", pValue, aSuites, numSuites, aExcluded) ? 0 : 2;
        }
        else if ( !bMissing && (pValue = J3DTest_GetOptionValue(argc, argv, &i, "--kind", &bMissing)) != NULL )
        {
            const int parsed = J3DTest_ParseKinds(pValue);
            kinds |= parsed;
            result = parsed ? 0 : 2;
        }
        else if ( bMissing )
        {
            fprintf(stderr, "%s: missing value\n", argv[i]);
            result = 2;
        }
        else if ( strcmp(argv[i], "--exclude-system") == 0 )
        {
            excludedKinds |= J3DTEST_SUITE_SYSTEM | J3DTEST_SUITE_DISRUPTIVE;
        }
        else if ( strcmp(argv[i], "--list") == 0 )
        {
            bList = true;
        }
        else
        {
            bHelp = bHelp || strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0;
            aUnityArgs[numUnityArgs++] = argv[i];
        }
    }

    if ( result == 0 && bList )
    {
        J3DTest_PrintSuites(stdout, aSuites, numSuites);
    }
    else if ( result == 0 )
    {
        if ( bHelp )
        {
            J3DTest_PrintHelp();
        }

        for ( size_t i = 0; i < numSuites; ++i )
        {
            aSelected[i] = (!bHasInclude || aIncluded[i]) && !aExcluded[i]
                && (kinds == 0 || (aSuites[i].kind & kinds) != 0) && (aSuites[i].kind & excludedKinds) == 0;
        }

        J3DTest_aRunSuites    = aSuites;
        J3DTest_numRunSuites  = numSuites;
        J3DTest_aRunSelected  = aSelected;
        result = UnityMain(numUnityArgs, aUnityArgs, J3DTest_RunSelectedSuites);
        J3DTest_aRunSuites    = NULL;
        J3DTest_numRunSuites  = 0;
        J3DTest_aRunSelected  = NULL;
    }

    free(aUnityArgs);
    free(aIncluded);
    free(aExcluded);
    free(aSelected);
    return result;
}
