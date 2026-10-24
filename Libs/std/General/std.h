#ifndef STD_STD_H
#define STD_STD_H
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

#define STDLOG_DEBUG(format, ...) \
    J3DLOG_DEBUG(std_g_pHS, format, ##__VA_ARGS__)

#define STDLOG_STATUS(format, ...) \
    J3DLOG_STATUS(std_g_pHS, format, ##__VA_ARGS__)

#define STDLOG_MESSAGE(format, ...) \
    J3DLOG_MESSAGE(std_g_pHS, format, ##__VA_ARGS__)

#define STDLOG_WARNING(format, ...) \
    J3DLOG_WARNING(std_g_pHS, format, ##__VA_ARGS__)

#define STDLOG_ERROR(format, ...) \
    J3DLOG_ERROR(std_g_pHS, format, ##__VA_ARGS__)

#define STDLOG_FATAL(message) \
    J3DLOG_FATAL(std_g_pHS, message)

#define STD_ASSERT(condition) \
    J3D_ASSERT(condition, std_g_pHS)

#define STD_ASSERTREL(condition) \
    J3D_ASSERTREL(condition, std_g_pHS)

#ifdef J3D_LOCAL_RTI
extern const char std_g_aEmptyString[8];
extern char std_g_genBuffer[2048];
extern tHostServices* std_g_pHS;
#else
#   define std_g_aEmptyString J3D_DECL_FAR_ARRAYVAR(std_g_aEmptyString, const char(*)[8])

#   define std_g_genBuffer J3D_DECL_FAR_ARRAYVAR(std_g_genBuffer, char(*)[2048])

#   define std_g_pHS J3D_DECL_FAR_VAR(std_g_pHS, tHostServices*)
#endif

void J3DAPI stdStartup(tHostServices* pHS);
void stdShutdown(void);
void J3DAPI stdPrintf(tPrintfFunc fpPrint, const char* pFilePath, unsigned int linenum, const char* format, ...);
int stdConsolePrintf(const char* pFormat, ...);
const char* J3DAPI stdFileFromPath(const char* pPath);
int J3DAPI stdCalcBitPos(int a1);
tFileHandle J3DAPI stdFileOpen(const char* pFilename, const char* mode);
int J3DAPI stdFileClose(tFileHandle fh);
size_t J3DAPI stdFileRead(tFileHandle fh, void* pOutData, size_t size);
size_t J3DAPI stdFileWrite(tFileHandle fh, const void* pData, size_t size);
int J3DAPI stdFileFlush(tFileHandle fh);
char* J3DAPI stdFileGets(tFileHandle fh, char* pStr, size_t size);
wchar_t* J3DAPI stdFileGetws(tFileHandle fh, wchar_t* pOutStr, size_t size);
int J3DAPI stdFileEof(tFileHandle fh);
int J3DAPI stdFileTell(tFileHandle fh);
size_t J3DAPI stdFileSize(const char* pFilePath);
int stdFilePrintf(tFileHandle fh, const char* pFormat, ...);
int J3DAPI stdFileSeek(tFileHandle pFile, int offset, int origin);

// Helper hooking functions
void std_InstallHooks(void);
void std_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STD_H
