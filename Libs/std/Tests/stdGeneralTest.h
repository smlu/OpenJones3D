#ifndef STD_GENERAL_TEST_H
#define STD_GENERAL_TEST_H

#include <stddef.h>
#include <stdint.h>

#include <j3dcore/j3d.h>

typedef struct sHostServices tHostServices;

J3D_EXTERN_C_START

void StdGeneralTest_Startup(void);
void StdGeneralTest_Shutdown(void);
void StdGeneralTest_AssertLastAssert(const char* pExpectedText, const char* pExpectedFile);
tHostServices* StdGeneralTest_GetHostServices(void);

void StdGeneralTest_FailNextAllocations(int numSuccessfulAllocationsBeforeFailure);
void StdGeneralTest_ClearAllocationFailures(void);
void StdGeneralTest_FailNextFileOpens(int numSuccessfulOpensBeforeFailure);
void StdGeneralTest_FailNextFileReads(int numSuccessfulReadsBeforeFailure);
void StdGeneralTest_FailNextFileWrites(int numSuccessfulWritesBeforeFailure);
void StdGeneralTest_FailNextFileGets(int numSuccessfulGetsBeforeFailure);
void StdGeneralTest_FailNextFileSeeks(int numSuccessfulSeeksBeforeFailure);
void StdGeneralTest_ClearFileFailures(void);

void StdGeneralTest_ResetDebugOutput(void);
const char* StdGeneralTest_GetDebugOutput(void);
uint32_t StdGeneralTest_GetConsoleAttributes(void);
size_t StdGeneralTest_GetConsoleWriteCount(void);

void StdGeneralTest_SetExistingFileName(const char* pFilename);
void StdGeneralTest_SetRegistryInt(const char* pKey, int value);
void StdGeneralTest_SetRegistryFloat(const char* pKey, float value);
void StdGeneralTest_SetRegistryString(const char* pKey, const char* pValue);
void StdGeneralTest_ClearRegistryValues(void);

const char* StdGeneralTest_GetTestVectorPath(const char* pRelativePath);
void StdGeneralTest_DeleteFile(const char* pPath);
int StdGeneralTest_CopyFile(const char* pSrcPath, const char* pDstPath);
int StdGeneralTest_CopyTestVectorFile(const char* pRelativePath, const char* pDstPath);

J3D_EXTERN_C_END

#endif // STD_GENERAL_TEST_H
