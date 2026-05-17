#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <windows.h>

#include "sha256.h"

namespace fs = std::filesystem;
constexpr std::string_view Indy3D_v1_0_sha256 = "3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53";

bool iequals(const std::string_view& lhs, const std::string_view& rhs)
{
    namespace rvs = std::ranges::views;
    auto to_lower = rvs::transform([](unsigned char c) { return std::tolower(c); });
    return std::ranges::equal(lhs | to_lower, rhs | to_lower);
}

static std::optional<std::string> CalcFileSha256(const fs::path& filepath)
{
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if ( !file.is_open() ) {
        return std::nullopt;
    }

    // Read the file into memory
    std::string data{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    file.close();

    SHA256 hasher;
    hasher.update(data.data(), data.size());
    return hasher.hash();
}

static bool IsValidFile(const fs::path& filepath, const std::string_view fileSha256)
{
    auto hash = CalcFileSha256(filepath);
    if ( !hash ) {
        return false;
    }
    return iequals(hash.value(), fileSha256);
}

static fs::path GetAbsolutePath(const fs::path& path)
{
    std::error_code ec;
    fs::path absolutePath = fs::absolute(path, ec);
    return ec ? path : absolutePath;
}

static std::wstring QuoteCommandLineArg(const std::wstring& arg)
{
    if ( arg.empty() )
    {
        return L"\"\"";
    }

    if ( arg.find_first_of(L" \t\n\v\"") == std::wstring::npos )
    {
        return arg;
    }

    std::wstring quoted;
    quoted.push_back(L'"');

    size_t backslashes = 0;
    for ( wchar_t ch : arg )
    {
        if ( ch == L'\\' )
        {
            ++backslashes;
            continue;
        }

        if ( ch == L'"' )
        {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(ch);
            backslashes = 0;
            continue;
        }

        quoted.append(backslashes, L'\\');
        backslashes = 0;
        quoted.push_back(ch);
    }

    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

int wmain(int argc, wchar_t* argv[])
{
    // Execute Indy3D.exe in suspended mode.
    fs::path exePath = "Indy3D.exe";
    if ( argc > 1 )
    {
        exePath = argv[1]; // should be path to Indy3D.exe
    }
    else if ( !fs::exists(exePath) )
    {
        // Try locating exe in resource folder
        exePath = "Resource/Indy3D.exe";
    }

    if ( !fs::exists(exePath) )
    {
        std::fprintf(stderr, "Can't find '%ls' to inject DLL!\n", exePath.c_str());
        MessageBox(NULL, (L"File '" + exePath.wstring() + L"' doesn't exist!").c_str(), L"Jones3D Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    exePath = GetAbsolutePath(exePath);
    if ( !IsValidFile(exePath, Indy3D_v1_0_sha256) )
    {
        std::fprintf(stderr, "Invalid Indy3D exe to inject DLL: %ls.\n", exePath.c_str());
        MessageBox(NULL, L"Only Indy3D.exe version 1.0 is supported!", L"Jones3D Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    fs::path dllPath = "Jones3D.dll";
    if ( !fs::exists(dllPath) )
    {
        std::fprintf(stderr, "Can't find %ls.\n", dllPath.c_str());
        MessageBox(NULL, L"Can't find Jones3D.dll!", L"Jones3D Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    dllPath = GetAbsolutePath(dllPath);
    const std::wstring dllPathWide = dllPath.native();
    const size_t dllPathSize = (dllPathWide.size() + 1) * sizeof(wchar_t);

    STARTUPINFO si = { 0 };
    si.cb = sizeof(STARTUPINFO);
    PROCESS_INFORMATION pi = { 0 };
    std::wstring cmd = QuoteCommandLineArg(exePath.native());
    for ( int i = 2; i < argc; ++i )
    {
        cmd.push_back(L' ');
        cmd.append(QuoteCommandLineArg(argv[i]));
    }

    if ( !CreateProcessW(exePath.c_str(), cmd.data(), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi) )
    {
        std::fprintf(stderr, "CreateProcess(\"%ls\") failed; error code = 0x%08X\n", exePath.c_str(), GetLastError());
        const wchar_t* pError = L"Failed to start Indy3D.exe!";
        if ( GetLastError() == ERROR_ELEVATION_REQUIRED ) {
            pError = L"Failed to start Indy3D.exe!\n\nIt looks like administrator permissions are required to run this program?! 🤔";
        }
        MessageBox(NULL, pError, L"Jones3D Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    LPWSTR remoteDllPath = NULL;
    HANDLE hThread = NULL;
    auto closeProcessHandles = [&pi]()
    {
        if ( pi.hThread )
        {
            CloseHandle(pi.hThread);
            pi.hThread = NULL;
        }

        if ( pi.hProcess )
        {
            CloseHandle(pi.hProcess);
            pi.hProcess = NULL;
        }
    };

    auto failInjection = [&](const wchar_t* pError)
    {
        if ( hThread )
        {
            CloseHandle(hThread);
            hThread = NULL;
        }

        if ( remoteDllPath )
        {
            VirtualFreeEx(pi.hProcess, remoteDllPath, 0, MEM_RELEASE);
            remoteDllPath = NULL;
        }

        TerminateProcess(pi.hProcess, 1);
        closeProcessHandles();
        MessageBox(NULL, pError, L"Jones3D Error", MB_ICONERROR | MB_OK);
        return 1;
    };

    // Allocate a page in the process's address space for the DLL path
    remoteDllPath = (LPWSTR)VirtualAllocEx(pi.hProcess, NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if ( remoteDllPath == NULL )
    {
        std::fprintf(stderr, "VirtualAllocEx failed; error code = 0x%08X\n", GetLastError());
        return failInjection(L"Failed to inject Jones3D.dll!");
    }

    // Write the Jones3D.dll path to the allocated memory
    if ( WriteProcessMemory(pi.hProcess, remoteDllPath, dllPathWide.c_str(), dllPathSize, NULL) == 0 )
    {
        std::fprintf(stderr, "WriteProcessMemory failed; error code = 0x%08X\n", GetLastError());
        return failInjection(L"Failed to inject Jones3D.dll!");
    }

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPTHREAD_START_ROUTINE pLoadLibraryW = hKernel32
        ? (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW")
        : NULL;
    if ( !pLoadLibraryW )
    {
        std::fprintf(stderr, "GetProcAddress(\"LoadLibraryW\") failed; error code = 0x%08X\n", GetLastError());
        return failInjection(L"Failed to inject Jones3D.dll!");
    }

    // Inject Jones3D.dll into the address space of the process
    hThread = CreateRemoteThread(pi.hProcess, NULL, 0, pLoadLibraryW, remoteDllPath, 0, NULL);
    if ( hThread == NULL )
    {
        std::fprintf(stderr, "CreateRemoteThread failed; error code = 0x%08X\n", GetLastError());
        return failInjection(L"Failed to inject Jones3D.dll!");
    }

    // Wait for DllMain to return
    if ( WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED )
    {
        std::fprintf(stderr, "WaitForSingleObject failed; error code = 0x%08X\n", GetLastError());
        return failInjection(L"Failed to inject Jones3D.dll!");
    }

    DWORD remoteModule = 0;
    if ( !GetExitCodeThread(hThread, &remoteModule) || !remoteModule )
    {
        std::fprintf(stderr, "LoadLibraryW(\"%ls\") failed in remote process.\n", dllPath.c_str());
        return failInjection(L"Failed to inject Jones3D.dll!");
    }

    CloseHandle(hThread);
    hThread = NULL;

    // Resume Indy3D.exe
    if ( ResumeThread(pi.hThread) == -1 )
    {
        std::fprintf(stderr, "ResumeThread failed; error code = 0x%08X\n", GetLastError());
        return failInjection(L"Failed to inject Jones3D.dll!");
    }

    VirtualFreeEx(pi.hProcess, remoteDllPath, 0, MEM_RELEASE);
    remoteDllPath = NULL;
    closeProcessHandles();
    std::printf("%ls successfully injected!\n", dllPath.c_str());
    return 0;
}
