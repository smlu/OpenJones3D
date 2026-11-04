// Added: the stdFile* host file functions on the C runtime, split out of std.c so a platform column can
// replace them as a whole (the Babylon column does).

#include <stdio.h>

#include "std.h"

// TODO: Functions below might all needed to be move to stdPlatform

tFileHandle J3DAPI stdFileOpen(const char* pFilename, const char* mode)
{
    FILE* fh;
    if ( fopen_s(&fh, pFilename, mode) != 0 )
    {
        fh = NULL; // Make sure it's null
    }
    return (tFileHandle)fh;
}

int J3DAPI stdFileClose(tFileHandle fh)
{
    fclose((FILE*)fh);
    return 0;
}

size_t J3DAPI stdFileRead(tFileHandle fh, void* pOutData, size_t size)
{
    return fread(pOutData, 1u, size, (FILE*)fh);
}

size_t J3DAPI stdFileWrite(tFileHandle fh, const void* pData, size_t size)
{
    return fwrite(pData, 1u, size, (FILE*)fh);
}

int J3DAPI stdFileFlush(tFileHandle fh)
{
    return fflush((FILE*)fh);
}

char* J3DAPI stdFileGets(tFileHandle fh, char* pStr, size_t size)
{
    return fgets(pStr, size, (FILE*)fh);
}

wchar_t* J3DAPI stdFileGetws(tFileHandle fh, wchar_t* pOutStr, size_t size)
{
    return fgetws(pOutStr, size, (FILE*)fh);
}

int J3DAPI stdFileEof(tFileHandle fh)
{
    return feof((FILE*)fh);
}

int J3DAPI stdFileTell(tFileHandle fh)
{
    return ftell((FILE*)fh);
}

size_t J3DAPI stdFileSize(const char* pFilePath)
{
    tFileHandle fh = stdFileOpen(pFilePath, "rb");
    if ( !fh )
    {
        return 0;
    }

    stdFileSeek(fh, 0, 2);
    int size = stdFileTell(fh);
    stdFileClose(fh);
    return size;
}

int J3DAPI stdFileSeek(tFileHandle fh, int offset, int origin)
{
    return fseek((FILE*)fh, offset, origin);
}
