// Tests the Microsoft secure-CRT behavior from Microsoft's documented contracts:
// the j3dcrt shim on the Null platform, and the real Windows CRT (j3dcore.Macros target).
// Expected values come from Microsoft's documentation, not from the shim implementation.
#include <unity_fixture.h>

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <j3dcore/j3d.h>

#if !defined(_WIN32)
// Added: the path-resolution tests create and remove a real directory tree beside the binary.
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifndef STRUNCATE
#define STRUNCATE 80
#endif

static int j3dCrtTest_CallVsnprintfS(char* pBuf, size_t bufSize, size_t count, const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int result = vsnprintf_s(pBuf, bufSize, count, pFormat, args);
    va_end(args);
    return result;
}

static int j3dCrtTest_CallVsnwprintfS(wchar_t* pBuf, size_t bufSize, size_t count, const wchar_t* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int result = _vsnwprintf_s(pBuf, bufSize, count, pFormat, args);
    va_end(args);
    return result;
}

static int j3dCrtTest_CallVsscanfS(const char* pBuf, const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    int result = vsscanf_s(pBuf, pFormat, args);
    va_end(args);
    return result;
}

TEST_GROUP(j3dCrt);

TEST_SETUP(j3dCrt)
{
}

TEST_TEAR_DOWN(j3dCrt)
{
}

TEST(j3dCrt, TestStrnlenS)
{
    TEST_ASSERT_EQUAL_size_t(3u, strnlen_s("abc", 8));
    TEST_ASSERT_EQUAL_size_t(2u, strnlen_s("abc", 2));
    TEST_ASSERT_EQUAL_size_t(0u, strnlen_s("", 8));
#ifndef _WIN32
    // Skipped on _WIN32: Microsoft defines the result as 0 for a null string, but a null
    // argument invokes the Windows invalid parameter handler.
    TEST_ASSERT_EQUAL_size_t(0u, strnlen_s(NULL, 8));
#endif
}

TEST(j3dCrt, TestStrcpyS)
{
    char buf[8];

    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(0, strcpy_s(buf, sizeof(buf), "abc"));
    TEST_ASSERT_EQUAL_STRING("abc", buf);

    TEST_ASSERT_EQUAL_INT(0, strcpy_s(buf, 4, "abc")); // exact fit
    TEST_ASSERT_EQUAL_STRING("abc", buf);

    // One character over the buffer: ERANGE and the destination is emptied.
    errno = 0;
    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(ERANGE, strcpy_s(buf, 3, "abc"));
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);
    TEST_ASSERT_EQUAL_INT(ERANGE, errno);

#ifndef _WIN32
    // Skipped on _WIN32: null pointers and a zero size invoke the Windows invalid
    // parameter handler. Microsoft returns EINVAL and empties the destination.
    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(EINVAL, strcpy_s(NULL, sizeof(buf), "abc"));
    TEST_ASSERT_EQUAL_INT(EINVAL, strcpy_s(buf, 0, "abc"));
    TEST_ASSERT_EQUAL_INT(EINVAL, strcpy_s(buf, sizeof(buf), NULL));
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);
#endif
}

TEST(j3dCrt, TestStrncpyS)
{
    char buf[8];

    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(0, strncpy_s(buf, sizeof(buf), "abcdefgh", 3));
    TEST_ASSERT_EQUAL_STRING("abc", buf);

    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(0, strncpy_s(buf, sizeof(buf), "ab", 5)); // stops at the terminator
    TEST_ASSERT_EQUAL_STRING("ab", buf);

    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(0, strncpy_s(buf, 4, "abc", _TRUNCATE)); // exact fit
    TEST_ASSERT_EQUAL_STRING("abc", buf);

    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(STRUNCATE, strncpy_s(buf, 3, "abc", _TRUNCATE)); // one character over
    TEST_ASSERT_EQUAL_STRING("ab", buf);

    errno = 0;
    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(ERANGE, strncpy_s(buf, 3, "abcdef", 3)); // requested copy does not fit
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);
#ifndef _WIN32
    TEST_ASSERT_EQUAL_INT(ERANGE, errno); // UCRT reports ERANGE but leaves errno unset here
#endif

    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(0, strncpy_s(buf, sizeof(buf), "abc", 0)); // count 0 empties the buffer
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);

#ifndef _WIN32
    // Skipped on _WIN32: null pointers and a zero size invoke the Windows invalid
    // parameter handler.
    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(EINVAL, strncpy_s(NULL, sizeof(buf), "abc", 2));
    TEST_ASSERT_EQUAL_INT(EINVAL, strncpy_s(buf, 0, "abc", 2));
    TEST_ASSERT_EQUAL_INT(EINVAL, strncpy_s(buf, sizeof(buf), NULL, 2));
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);
#endif
}

TEST(j3dCrt, TestWcsncpyS)
{
    wchar_t buf[8];

    wmemset(buf, L'q', 8);
    TEST_ASSERT_EQUAL_INT(0, wcsncpy_s(buf, 5, L"abcdefgh", 3));
    TEST_ASSERT_EQUAL_INT(0, wcscmp(buf, L"abc"));

    wmemset(buf, L'q', 8);
    TEST_ASSERT_EQUAL_INT(0, wcsncpy_s(buf, 4, L"abc", _TRUNCATE)); // exact fit
    TEST_ASSERT_EQUAL_INT(0, wcscmp(buf, L"abc"));

    wmemset(buf, L'q', 8);
    TEST_ASSERT_EQUAL_INT(STRUNCATE, wcsncpy_s(buf, 3, L"abc", _TRUNCATE)); // one over
    TEST_ASSERT_EQUAL_INT(0, wcscmp(buf, L"ab"));

    wmemset(buf, L'q', 8);
    TEST_ASSERT_EQUAL_INT(ERANGE, wcsncpy_s(buf, 3, L"abcdef", 3)); // does not fit, emptied
    TEST_ASSERT_EQUAL_INT(0, buf[0]);
}

TEST(j3dCrt, TestStrncatS)
{
    char buf[8];

    strcpy_s(buf, sizeof(buf), "ab");
    TEST_ASSERT_EQUAL_INT(0, strncat_s(buf, sizeof(buf), "cd", 2));
    TEST_ASSERT_EQUAL_STRING("abcd", buf);

    strcpy_s(buf, sizeof(buf), "ab");
    TEST_ASSERT_EQUAL_INT(STRUNCATE, strncat_s(buf, 4, "cde", _TRUNCATE)); // only 'c' fits
    TEST_ASSERT_EQUAL_STRING("abc", buf);

    strcpy_s(buf, sizeof(buf), "ab");
    TEST_ASSERT_EQUAL_INT(STRUNCATE, strncat_s(buf, 3, "cd", _TRUNCATE)); // nothing fits
    TEST_ASSERT_EQUAL_STRING("ab", buf);

    errno = 0;
    strcpy_s(buf, sizeof(buf), "abcd");
    TEST_ASSERT_EQUAL_INT(ERANGE, strncat_s(buf, 5, "efgh", 4)); // does not fit, emptied
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);
    TEST_ASSERT_EQUAL_INT(ERANGE, errno);

#ifndef _WIN32
    // Skipped on _WIN32: null pointers and a zero size invoke the Windows invalid
    // parameter handler, as does a destination without a terminator in its size.
    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(EINVAL, strncat_s(NULL, sizeof(buf), "ab", 2));
    TEST_ASSERT_EQUAL_INT(EINVAL, strncat_s(buf, 0, "ab", 2));
    TEST_ASSERT_EQUAL_INT(EINVAL, strncat_s(buf, sizeof(buf), NULL, 2));
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);
#endif
}

TEST(j3dCrt, TestVsnprintfS)
{
    char buf[8];

    TEST_ASSERT_EQUAL_INT(2, j3dCrtTest_CallVsnprintfS(buf, sizeof(buf), 3, "%s", "ab"));
    TEST_ASSERT_EQUAL_STRING("ab", buf);

    // count < size: at most count characters plus a terminator, -1 when truncated.
    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(-1, j3dCrtTest_CallVsnprintfS(buf, sizeof(buf), 3, "%s", "abcdef"));
    TEST_ASSERT_EQUAL_STRING("abc", buf);
    TEST_ASSERT_EQUAL_INT('\0', buf[3]);
    TEST_ASSERT_EQUAL_INT(7, buf[4]);

    // _TRUNCATE: keeps the part that fits with a terminator and returns -1.
    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(-1, j3dCrtTest_CallVsnprintfS(buf, 5, _TRUNCATE, "%s", "abcdefgh"));
    TEST_ASSERT_EQUAL_STRING("abcd", buf);

    TEST_ASSERT_EQUAL_INT(5, j3dCrtTest_CallVsnprintfS(buf, 6, _TRUNCATE, "%s", "abcde"));

    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(-1, j3dCrtTest_CallVsnprintfS(buf, sizeof(buf), 0, "%s", "ab"));
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);

#ifndef _WIN32
    // Skipped on _WIN32: count >= size with a too-small output invokes the Windows invalid
    // parameter handler. Microsoft sets buffer[0] to the terminator, -1 and errno ERANGE.
    errno = 0;
    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(-1, j3dCrtTest_CallVsnprintfS(buf, 4, 4, "%s", "abcdef"));
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);
    TEST_ASSERT_EQUAL_INT(ERANGE, errno);
#endif
}

TEST(j3dCrt, TestSnprintfS)
{
    char buf[8];

    TEST_ASSERT_EQUAL_INT(2, _snprintf_s(buf, sizeof(buf), 3, "%s", "ab"));
    TEST_ASSERT_EQUAL_STRING("ab", buf);

    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(-1, _snprintf_s(buf, sizeof(buf), 3, "%s", "abcdef"));
    TEST_ASSERT_EQUAL_STRING("abc", buf);
    TEST_ASSERT_EQUAL_INT('\0', buf[3]);

    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(-1, _snprintf_s(buf, 5, _TRUNCATE, "%s", "abcdefgh"));
    TEST_ASSERT_EQUAL_STRING("abcd", buf);

    // count >= size still succeeds while the output fits into the buffer.
    TEST_ASSERT_EQUAL_INT(2, _snprintf_s(buf, 4, 100, "%s", "ab"));
}

TEST(j3dCrt, TestSprintfS)
{
    char buf[8];

    TEST_ASSERT_EQUAL_INT(5, sprintf_s(buf, sizeof(buf), "%d", 12345));
    TEST_ASSERT_EQUAL_STRING("12345", buf);

    TEST_ASSERT_EQUAL_INT(2, sprintf_s(buf, 3, "%s", "ab")); // exact fit

#ifndef _WIN32
    // Skipped on _WIN32: a too-small buffer invokes the Windows invalid parameter handler.
    // Microsoft returns -1, sets buffer[0] to the terminator and errno to ERANGE.
    errno = 0;
    memset(buf, 7, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(-1, sprintf_s(buf, 4, "%s", "abcdef"));
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);
    TEST_ASSERT_EQUAL_INT(ERANGE, errno);
#endif
}

TEST(j3dCrt, TestVsnwprintfS)
{
    wchar_t buf[8];

    wmemset(buf, L'q', 8);
    TEST_ASSERT_EQUAL_INT(2, j3dCrtTest_CallVsnwprintfS(buf, 8, 3, L"%ls", L"ab"));
    TEST_ASSERT_EQUAL_INT(0, wcscmp(buf, L"ab"));

    wmemset(buf, L'q', 8);
    TEST_ASSERT_EQUAL_INT(-1, j3dCrtTest_CallVsnwprintfS(buf, 8, 3, L"%ls", L"abcdef"));
    TEST_ASSERT_EQUAL_INT(0, wcscmp(buf, L"abc"));
    TEST_ASSERT_EQUAL_INT((int)L'q', (int)buf[4]);

    wmemset(buf, L'q', 8);
    TEST_ASSERT_EQUAL_INT(-1, j3dCrtTest_CallVsnwprintfS(buf, 5, _TRUNCATE, L"%ls", L"abcdefgh"));
    TEST_ASSERT_EQUAL_INT((int)L'a', (int)buf[0]);
    TEST_ASSERT_EQUAL_INT((int)L'd', (int)buf[3]);
    TEST_ASSERT_EQUAL_INT(0, buf[4]);

    TEST_ASSERT_EQUAL_INT(4, j3dCrtTest_CallVsnwprintfS(buf, 5, _TRUNCATE, L"%ls", L"abcd"));
}

TEST(j3dCrt, TestFopenS)
{
    FILE* pFile = NULL;
    FILE* pProbe = fopen("j3dCrtTest.txt", "w");
    TEST_ASSERT_NOT_NULL(pProbe);
    fputs("x", pProbe);
    fclose(pProbe);

    TEST_ASSERT_EQUAL_INT(0, fopen_s(&pFile, "j3dCrtTest.txt", "rb"));
    TEST_ASSERT_NOT_NULL(pFile);
    fclose(pFile);

    pFile = (FILE*)1;
    TEST_ASSERT_NOT_EQUAL(0, fopen_s(&pFile, "j3dCrtMissingFile.txt", "rb"));
    TEST_ASSERT_NULL(pFile);

    remove("j3dCrtTest.txt");

#ifndef _WIN32
    // Skipped on _WIN32: null arguments invoke the Windows invalid parameter handler.
    TEST_ASSERT_EQUAL_INT(EINVAL, fopen_s(NULL, "j3dCrtTest.txt", "rb"));
    pFile = NULL;
    TEST_ASSERT_EQUAL_INT(EINVAL, fopen_s(&pFile, NULL, "rb"));
    TEST_ASSERT_NULL(pFile);
#endif
}

#if !defined(_WIN32)
// Added: the path-resolution tests create a real directory tree beside the test binary. The shim's
// resolver is the non-Windows equivalent of the case-insensitive Windows path lookup.
#define J3DCRTTEST_DIR "j3dCrtTestDir"

static void j3dCrtTest_CleanTree(void)
{
    remove(J3DCRTTEST_DIR "/Sub/File.TXT");
    remove(J3DCRTTEST_DIR "/Sub/new.txt");
    rmdir(J3DCRTTEST_DIR "/Sub");
    rmdir(J3DCRTTEST_DIR "/sub");
    rmdir(J3DCRTTEST_DIR);
}

static void j3dCrtTest_MakeTree(void)
{
    FILE* pFile;

    j3dCrtTest_CleanTree();
    mkdir(J3DCRTTEST_DIR, 0777);
    mkdir(J3DCRTTEST_DIR "/Sub", 0777);

    pFile = j3dcrt_fopen(J3DCRTTEST_DIR "/Sub/File.TXT", "wb");
    TEST_ASSERT_NOT_NULL(pFile);
    TEST_ASSERT_EQUAL_size_t(1, fwrite("x", 1, 1, pFile));
    fclose(pFile);
}

TEST(j3dCrt, TestResolvePath)
{
    char aPath[260];

    j3dCrtTest_MakeTree();

    // Backslashes become slashes, and components that exist in another letter case are rewritten.
    TEST_ASSERT_EQUAL_STRING(J3DCRTTEST_DIR "/Sub/File.TXT",
        j3dcrt_ResolvePath(J3DCRTTEST_DIR "\\sub\\file.txt", aPath, sizeof(aPath)));
    TEST_ASSERT_EQUAL_PTR(aPath, j3dcrt_ResolvePath("j3dcrttestdir\\SUB\\FILE.TXT", aPath, sizeof(aPath)));
    TEST_ASSERT_EQUAL_STRING(J3DCRTTEST_DIR "/Sub/File.TXT", aPath);

    // A component that matches nothing, and everything after it, stay as given, so a file that does
    // not exist yet can still be created in a directory that does.
    TEST_ASSERT_EQUAL_STRING(J3DCRTTEST_DIR "/Sub/new.txt",
        j3dcrt_ResolvePath(J3DCRTTEST_DIR "\\sub\\new.txt", aPath, sizeof(aPath)));
    TEST_ASSERT_EQUAL_STRING("j3dCrtMissingDir/new.txt",
        j3dcrt_ResolvePath("j3dCrtMissingDir\\new.txt", aPath, sizeof(aPath)));
    TEST_ASSERT_EQUAL_STRING("j3dCrtMissingDir/sub/file.txt",
        j3dcrt_ResolvePath("j3dCrtMissingDir\\sub\\file.txt", aPath, sizeof(aPath)));

    // A name that exists with the case it was written in wins over a differently cased sibling.
    mkdir(J3DCRTTEST_DIR "/sub", 0777);
    TEST_ASSERT_EQUAL_STRING(J3DCRTTEST_DIR "/sub",
        j3dcrt_ResolvePath(J3DCRTTEST_DIR "\\sub", aPath, sizeof(aPath)));

    // Of two entries that only differ in letter case, the resolver picks the one that sorts first.
    TEST_ASSERT_EQUAL_STRING(J3DCRTTEST_DIR "/Sub",
        j3dcrt_ResolvePath(J3DCRTTEST_DIR "\\SUB", aPath, sizeof(aPath)));

    j3dCrtTest_CleanTree();
}

TEST(j3dCrt, TestFopenResolvePath)
{
    struct stat st;
    char        aPath[260];
    FILE*       pFile;
    int         ch;

    j3dCrtTest_MakeTree();

    // The path does not exist with the case it is written in, and j3dcrt_fopen opens it anyway.
    TEST_ASSERT_EQUAL_INT(-1, stat(J3DCRTTEST_DIR "/sub/file.txt", &st));
    pFile = j3dcrt_fopen(J3DCRTTEST_DIR "\\sub\\file.txt", "rb");
    TEST_ASSERT_NOT_NULL(pFile);
    TEST_ASSERT_EQUAL_INT('x', fgetc(pFile));
    TEST_ASSERT_EQUAL_INT(EOF, fgetc(pFile));
    fclose(pFile);

    pFile = fopen("j3dCrttestdir\\Sub\\FILE.TXT", "rb"); // Added: the fopen macro routes here.
    TEST_ASSERT_NOT_NULL(pFile);
    fclose(pFile);

    // A new file is created in the directory the resolver found, under the name it was given.
    pFile = j3dcrt_fopen(J3DCRTTEST_DIR "\\sub\\new.txt", "wb");
    TEST_ASSERT_NOT_NULL(pFile);
    TEST_ASSERT_EQUAL_size_t(1, fwrite("y", 1, 1, pFile));
    fclose(pFile);
    TEST_ASSERT_EQUAL_INT(0, stat(J3DCRTTEST_DIR "/Sub/new.txt", &st));
    TEST_ASSERT_EQUAL_INT(-1, stat(J3DCRTTEST_DIR "/Sub/New.txt", &st));

    // A file in a directory that does not exist cannot be opened for reading.
    TEST_ASSERT_NULL(j3dcrt_fopen(J3DCRTTEST_DIR "\\sub\\missing.txt", "rb"));
    TEST_ASSERT_NULL(j3dcrt_fopen("j3dCrtMissingDir\\file.txt", "rb"));
    TEST_ASSERT_NULL(j3dcrt_fopen(NULL, "rb"));

    TEST_ASSERT_EQUAL_STRING(J3DCRTTEST_DIR "/Sub/File.TXT",
        j3dcrt_ResolvePath(J3DCRTTEST_DIR "\\sub\\file.txt", aPath, sizeof(aPath)));

    j3dCrtTest_CleanTree();
}
#endif // !_WIN32

TEST(j3dCrt, TestStrcmpi)
{
    TEST_ASSERT_EQUAL_INT(0, _strcmpi("Jones", "jONES"));
    TEST_ASSERT_TRUE(_strcmpi("abc", "abd") < 0);
    TEST_ASSERT_TRUE(_strcmpi("abd", "abc") > 0);
}

TEST(j3dCrt, TestIsnan)
{
    double value = 1.0;
    TEST_ASSERT_EQUAL_INT(0, _isnan(value));

    volatile double zero = 0.0;
    value = zero / zero;
    TEST_ASSERT_TRUE(_isnan(value) != 0);
}

TEST(j3dCrt, TestSscanfS)
{
    char buf[8];

    TEST_ASSERT_EQUAL_INT(1, sscanf_s("hello sim", "%s", buf, (rsize_t)sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("hello", buf);

    int iv = 0;
    float fv = 0.f;
    char sb[8];
    TEST_ASSERT_EQUAL_INT(3, sscanf_s("12 1.25 abc", "%d %f %s", &iv, &fv, sb, (rsize_t)sizeof(sb)));
    TEST_ASSERT_EQUAL_INT(12, iv);
    TEST_ASSERT_EQUAL_FLOAT(1.25f, fv);
    TEST_ASSERT_EQUAL_STRING("abc", sb);

    // A given width stays and limits the scan; the size argument is still consumed.
    memset(sb, 9, sizeof(sb));
    TEST_ASSERT_EQUAL_INT(1, sscanf_s("hello", "%3s", sb, (rsize_t)sizeof(sb)));
    TEST_ASSERT_EQUAL_STRING("hel", sb);

    // The string fills the whole buffer with no terminator: Microsoft fails the conversion
    // and returns the number of conversions completed before it.
    char t3[4];
    memset(t3, 7, sizeof(t3));
    TEST_ASSERT_EQUAL_INT(0, sscanf_s("abcdef", "%s", t3, (rsize_t)sizeof(t3)));
    TEST_ASSERT_EQUAL_INT('\0', t3[0]);

    char c = 0;
    TEST_ASSERT_EQUAL_INT(1, sscanf_s("ax", "%c", &c, (rsize_t)1));
    TEST_ASSERT_EQUAL_INT('a', c);

    // %2c reads two characters and appends no terminator.
    memset(sb, 9, sizeof(sb));
    TEST_ASSERT_EQUAL_INT(1, sscanf_s("xyz", "%2c", sb, (rsize_t)sizeof(sb)));
    TEST_ASSERT_EQUAL_INT('x', sb[0]);
    TEST_ASSERT_EQUAL_INT('y', sb[1]);
    TEST_ASSERT_EQUAL_INT(9, sb[2]);

    memset(sb, 9, sizeof(sb));
    TEST_ASSERT_EQUAL_INT(1, sscanf_s("abxyz", "ab%[x-z]", sb, (rsize_t)sizeof(sb)));
    TEST_ASSERT_EQUAL_STRING("xyz", sb);

    memset(sb, 9, sizeof(sb));
    TEST_ASSERT_EQUAL_INT(0, sscanf_s("abwxyz", "ab%[w-z]", sb, (rsize_t)3)); // fills the buffer
    TEST_ASSERT_EQUAL_INT('\0', sb[0]);

    // A silent assignment suppression takes neither a pointer nor a size.
    memset(sb, 9, sizeof(sb));
    TEST_ASSERT_EQUAL_INT(1, sscanf_s("skip end", "%*s %s", sb, (rsize_t)sizeof(sb)));
    TEST_ASSERT_EQUAL_STRING("end", sb);

    // %n writes a position and counts neither as a size nor as a returned conversion.
    int pos = -1;
    iv = 0;
    TEST_ASSERT_EQUAL_INT(1, sscanf_s("42 x", "%d %n", &iv, &pos));
    TEST_ASSERT_EQUAL_INT(42, iv);
    TEST_ASSERT_EQUAL_INT(3, pos);

    TEST_ASSERT_EQUAL_INT(EOF, sscanf_s("", "%s", buf, (rsize_t)sizeof(buf)));
}

TEST(j3dCrt, TestSscanfSNoOverrun)
{
    char a[8];
    int n = 0;

    // A %s scan of size or more characters must not write the terminator past the caller's
    // buffer; the conversion fails, empties the destination, and the bytes beyond it are intact.
    memset(a, 0x5A, sizeof(a));
    TEST_ASSERT_EQUAL_INT(0, sscanf_s("abcdef", "%s", a, (rsize_t)4));
    TEST_ASSERT_EQUAL_INT('\0', a[0]);
    TEST_ASSERT_EQUAL_INT(0x5A, a[4]);
    TEST_ASSERT_EQUAL_INT(0x5A, a[5]);
    TEST_ASSERT_EQUAL_INT(0x5A, a[6]);
    TEST_ASSERT_EQUAL_INT(0x5A, a[7]);

    // A token of exactly size-1 characters fits with its terminator.
    memset(a, 0x5A, sizeof(a));
    TEST_ASSERT_EQUAL_INT(1, sscanf_s("abc", "%s", a, (rsize_t)4));
    TEST_ASSERT_EQUAL_STRING("abc", a);
    TEST_ASSERT_EQUAL_INT(0x5A, a[4]);

    // A failing %s counts as no completed conversion, even with a following conversion.
    memset(a, 0x5A, sizeof(a));
    TEST_ASSERT_EQUAL_INT(0, sscanf_s("toolong 5", "%s %d", a, (rsize_t)4, &n));
    TEST_ASSERT_EQUAL_INT('\0', a[0]);
}

TEST(j3dCrt, TestVsscanfS)
{
    char buf[8];
    int iv = 0;

    TEST_ASSERT_EQUAL_INT(2, j3dCrtTest_CallVsscanfS("7 brown", "%d %s", &iv, buf, (rsize_t)sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(7, iv);
    TEST_ASSERT_EQUAL_STRING("brown", buf);

    TEST_ASSERT_EQUAL_INT(EOF, j3dCrtTest_CallVsscanfS("", "%s", buf, (rsize_t)sizeof(buf)));
}

TEST_GROUP_RUNNER(j3dCrt)
{
    RUN_TEST_CASE(j3dCrt, TestStrnlenS);
    RUN_TEST_CASE(j3dCrt, TestStrcpyS);
    RUN_TEST_CASE(j3dCrt, TestStrncpyS);
    RUN_TEST_CASE(j3dCrt, TestWcsncpyS);
    RUN_TEST_CASE(j3dCrt, TestStrncatS);
    RUN_TEST_CASE(j3dCrt, TestVsnprintfS);
    RUN_TEST_CASE(j3dCrt, TestSnprintfS);
    RUN_TEST_CASE(j3dCrt, TestSprintfS);
    RUN_TEST_CASE(j3dCrt, TestVsnwprintfS);
    RUN_TEST_CASE(j3dCrt, TestFopenS);
#if !defined(_WIN32)
    RUN_TEST_CASE(j3dCrt, TestResolvePath);
    RUN_TEST_CASE(j3dCrt, TestFopenResolvePath);
#endif
    RUN_TEST_CASE(j3dCrt, TestStrcmpi);
    RUN_TEST_CASE(j3dCrt, TestIsnan);
    RUN_TEST_CASE(j3dCrt, TestSscanfS);
    RUN_TEST_CASE(j3dCrt, TestSscanfSNoOverrun);
    RUN_TEST_CASE(j3dCrt, TestVsscanfS);
}

void TEST_j3dCrt_GROUP_RUNNER(void);

#ifdef J3D_PLATFORM_NULL
// Added: the Null-platform test target builds a standalone native executable from this file.
static void j3dCrtTest_RunAllTests(void)
{
    TEST_j3dCrt_GROUP_RUNNER();
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, j3dCrtTest_RunAllTests);
}
#endif
