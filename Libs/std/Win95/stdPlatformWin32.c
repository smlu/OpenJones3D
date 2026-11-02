
#define STRICT
#include <windows.h> // timeGetTime, include first to avoid undefined warning due to WIN32_LEAN_AND_MEAN
#include <dbghelp.h>
#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>

#include <std/General/std.h>
#include <std/General/stdUtil.h>

#include <std/General/stdPlatformSys.h>
#include <std/General/stdPlatform.h>

// Added: high-resolution timer support
static bool stdPlatform_bHighResTimerInitialized = false;
LARGE_INTEGER stdPlatform_perfCounterFreq        = { 0 };
LARGE_INTEGER stdPlatform_perfCounterStart       = { 0 };

// Breaks into the platform debugger
void stdPlatformSys_DebugBreak(void)
{
    DebugBreak();
}

// Shows the given text in the platform assert message box, with the original title and flags
void stdPlatformSys_ShowAssertMessage(const char* pText)
{
    MessageBox(NULL, pText, "Assert Handler", MB_TASKMODAL);
}

// Writes the given text to the platform debug output
void stdPlatformSys_DebugOutput(const char* pText)
{
    OutputDebugString(pText);
}

// Starts COM on the calling thread, returns the underlying CoInitialize result value as int
int stdPlatformSys_CoInitialize(void)
{
    return CoInitialize(NULL);
}

// Releases COM on the calling thread, must balance a previous stdPlatformSys_CoInitialize
void stdPlatformSys_CoUninitialize(void)
{
    CoUninitialize();
}

void stdPlatform_InitHighResTimer(void)
{
    QueryPerformanceFrequency(&stdPlatform_perfCounterFreq);
    QueryPerformanceCounter(&stdPlatform_perfCounterStart);
    stdPlatform_bHighResTimerInitialized = true;
}

tStdTime stdPlatform_GetTimeMsec(void)
{
#ifdef J3D_QOL_IMPROVEMENTS
    if ( !stdPlatform_bHighResTimerInitialized )
    {
        stdPlatform_InitHighResTimer();
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    LONGLONG elapsed = now.QuadPart - stdPlatform_perfCounterStart.QuadPart;

    // Calculate milliseconds, result will wrap around at ~49.7 days (same as timeGetTime)
    LONGLONG msec = (elapsed * 1000) / stdPlatform_perfCounterFreq.QuadPart;

    return (tStdTime)msec; // truncate to tStdTime
#else
    return timeGetTime();
#endif
}

void J3DAPI stdPlatform_Sleep(uint32_t msec)
{
    Sleep(msec);
}

void J3DAPI stdPlatform_GetLocalTime(StdLocalTime* pTime)
{
    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);

    pTime->year   = sysTime.wYear;
    pTime->month  = sysTime.wMonth;
    pTime->day    = sysTime.wDay;
    pTime->hour   = sysTime.wHour;
    pTime->minute = sysTime.wMinute;
    pTime->second = sysTime.wSecond;
}

bool J3DAPI stdPlatform_DirExists(const char* pPath)
{
    DWORD dwAttrib = GetFileAttributes(pPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static LONG WINAPI WinMiniDumpExceptionFilter(LPEXCEPTION_POINTERS ExceptionInfo)
{
    HANDLE hFile = CreateFile("core.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
        exceptionInfo.ThreadId          = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = ExceptionInfo;
        exceptionInfo.ClientPointers    = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &exceptionInfo, NULL, NULL);
        CloseHandle(hFile);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void J3DAPI stdPlatform_InstallSignalHandler(void)
{
    SetUnhandledExceptionFilter(WinMiniDumpExceptionFilter);
}

bool CheckStackFrameStringArg(uintptr_t ptr, bool* bWide, char* outAnsiBuf, size_t ansiBufSize, wchar_t* outWideBuf, size_t wideBufSize, size_t* readLen)
{
    *bWide  = false;
    *readLen = 0;

    if ( !ptr )
    {
        return false;
    }

    // Validate memory first with VirtualQuery
    MEMORY_BASIC_INFORMATION mbi;
    if ( !VirtualQuery((LPCVOID)ptr, &mbi, sizeof(mbi))
        || mbi.State != MEM_COMMIT
        || (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY)) == 0 )
    {
        return false;
    }

    // First, try wide string
    if ( ReadProcessMemory(GetCurrentProcess(), (LPCVOID)ptr, outWideBuf, (wideBufSize - 1) * sizeof(wchar_t), (PSIZE_T)readLen)
        && *readLen >= sizeof(wchar_t) )
    {
        *readLen /= sizeof(wchar_t);
        outWideBuf[wideBufSize - 1] = 0;
        if ( wmemchr(outWideBuf, L'\0', *readLen) )
        {
            for ( size_t i = 0; i < *readLen; ++i )
            {
                if ( outWideBuf[i] == 0 ) break;
                if ( !iswprint(outWideBuf[i]) )
                {
                    goto try_narrow;
                }
            }
            *bWide = true;
            return true;
        }
    }

try_narrow:
    if ( ReadProcessMemory(GetCurrentProcess(), (LPCVOID)ptr, outAnsiBuf, ansiBufSize - 1, (PSIZE_T)readLen) )
    {
        outAnsiBuf[ansiBufSize - 1] = 0;
        if ( memchr(outAnsiBuf, '\0', *readLen) )
        {
            for ( size_t i = 0; i < *readLen; ++i )
            {
                if ( outAnsiBuf[i] == '\0' ) break;
                if ( !isprint((unsigned char)outAnsiBuf[i]) )
                {
                    return false;
                }
            }
            return true;
        }
    }

    return false;
}

static void PrintStackFrameArgs(tStackTracePrintFunc pfPrintFunc, size_t frame, CONTEXT* ctx, STACKFRAME64* sf)
{
    // Note: Function assumes that function arguments are passed in registers or stack but 
    // if stack unwind information not available read arguments will be wrong. 
    // Mostly likely will happen in release builds.

    uintptr_t args[4] = { 0 };

#ifdef _M_X64
    if ( frame != 0 )
    {
        return;
    }
    args = (uintptr_t[4]){ (uintptr_t)ctx->Rcx, (uintptr_t)ctx->Rdx, (uintptr_t)ctx->R8, (uintptr_t)ctx->R9 };
#else
    J3D_UNUSED(ctx);
    J3D_UNUSED(frame);
    ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(sf->AddrFrame.Offset + 8), args, sizeof(args), NULL);
#endif

    for ( size_t i = 0; i < STD_ARRAYLEN(args); ++i )
    {
        char ansiBuf[256]    = { 0 };
        wchar_t wideBuf[256] = { 0 };
        bool bWide           = false;
        size_t readLen       = 0;

        if ( CheckStackFrameStringArg(args[i], &bWide, ansiBuf, STD_ARRAYLEN(ansiBuf), wideBuf, STD_ARRAYLEN(wideBuf), &readLen) )
        {
            if ( bWide )
            {
                stdUtil_ToAStringEx(ansiBuf, wideBuf, readLen);
                ansiBuf[readLen] = 0;
                STD_FORMAT(std_g_genBuffer, "\n  [%u]=\"%s\"", i, ansiBuf);
            }
            else
            {
                STD_FORMAT(std_g_genBuffer, "\n  [%u]=\"%s\"", i, ansiBuf);
            }
        }
        else
        {
            STD_FORMAT(std_g_genBuffer, "\n  [%u]=0x%X", i, args[i]);
        }

        pfPrintFunc(std_g_genBuffer);
    }
}

void stdPlatform_PrintStackTrace(tStackTracePrintFunc pfPrintFunc, size_t numFrames)
{
    HANDLE hProc = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();
    SymInitialize(hProc, NULL, TRUE);

    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&ctx);

    STACKFRAME64 sf = { 0 };
    DWORD machine;
#ifdef _M_X64
    sf.AddrPC.Offset    = ctx.Rip;
    sf.AddrFrame.Offset = ctx.Rbp;
    sf.AddrStack.Offset = ctx.Rsp;
    machine = IMAGE_FILE_MACHINE_AMD64;
#else
    sf.AddrPC.Offset    = ctx.Eip;
    sf.AddrFrame.Offset = ctx.Ebp;
    sf.AddrStack.Offset = ctx.Esp;
    machine = IMAGE_FILE_MACHINE_I386;
#endif
    sf.AddrPC.Mode = sf.AddrFrame.Mode = sf.AddrStack.Mode = AddrModeFlat;

    STD_FORMAT(std_g_genBuffer, "\n==================== STACK TRACE ====================\n");
    pfPrintFunc(std_g_genBuffer);
    for ( size_t i = 0; i < numFrames; ++i )
    {
        if ( !StackWalk64(machine, hProc, hThread, &sf, &ctx, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)
            || sf.AddrPC.Offset == 0 )
        {
            break;
        }

         // Get Module name
        char aModuleName[MAX_PATH]   = { 0 };
        IMAGEHLP_MODULE64 moduleInfo = { 0 };
        moduleInfo.SizeOfStruct = sizeof(moduleInfo);
        if ( SymGetModuleInfo64(hProc, sf.AddrPC.Offset, &moduleInfo) )
        {
            const char* moduleName = strrchr(moduleInfo.ImageName, '\\');
            moduleName = moduleName ? moduleName + 1 : moduleInfo.ModuleName;
            STD_STRCPY(aModuleName, moduleName);
        }

        // Get symbol info
        char symbuf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = { 0 };
        SYMBOL_INFO* sym  = (SYMBOL_INFO*)symbuf;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen   = MAX_SYM_NAME;

        DWORD64 disp = 0;
        char aSymName[MAX_SYM_NAME] = { 0 };
        if ( SymFromAddr(hProc, sf.AddrPC.Offset, &disp, sym) )
        {
            IMAGEHLP_LINE64 line;
            DWORD linedisp = 0;
            if ( SymGetLineFromAddr64(hProc, sf.AddrPC.Offset, &linedisp, &line) )
            {
                const char* filename = strrchr(line.FileName, '\\');
                if ( filename )
                {
                    filename++; // skip leading backslash
                    STD_FORMAT(aSymName, "%s [%s:%d]", sym->Name, filename, line.LineNumber);
                }
                else
                {
                    STD_FORMAT(aSymName, "%s [%d]", sym->Name, line.LineNumber);
                }
            }
            else
            {
                // Could not get line info, just use function name and offset
                STD_FORMAT(aSymName, "%s+0x%llX", sym->Name, disp);
            }
        }
        else
        {
            // Symbol couldn't be resolved, use PC address
            STD_FORMAT(aSymName, "0x%llX", sf.AddrPC.Offset);
        }

        STD_FORMAT(std_g_genBuffer, "#%02u %s:%s", i, aModuleName, aSymName);
        pfPrintFunc(std_g_genBuffer);

        PrintStackFrameArgs(pfPrintFunc, i, &ctx, &sf);
        pfPrintFunc("\n\n");
    }

    STD_FORMAT(std_g_genBuffer, "=========================================================\n");
    pfPrintFunc(std_g_genBuffer);
    SymCleanup(hProc);
}
