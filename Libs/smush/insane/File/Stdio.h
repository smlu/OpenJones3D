#ifndef SMUSH_INSANE_FILE_STDIO_H
#define SMUSH_INSANE_FILE_STDIO_H
#include <j3dcore/j3d.h>
#include <smush/insane/lec.h>
#include <smush/insane/filio.h>

/*
 * Stdio.c: the `FILIO=insane_stdio` module (C stdio). The original's Startup/Shutdown and read/write/tell/seek bodies are
 * folded by the linker onto other functions (see RTI/addresses.h), so only Register, Unregister, open, close and flush have
 * addresses of their own.
 */

J3D_EXTERN_C_START

int J3DAPI stdio_Register(LecShare* pShare, void* pParam);
void stdio_Unregister(void);
int stdio_Startup(void); // Folded onto 0x004E74A0; no address of its own, never hook
void stdio_Shutdown(void); // Folded onto 0x004E74A0; no address of its own, never hook
int J3DAPI stdio_open(const char* pPath, const char* pMode);
void J3DAPI stdio_close(int fh);
int J3DAPI stdio_read(int fh, void* pBuf, int size); // Folded onto stdFileRead 0x004C05D0; no address of its own, never hook
int J3DAPI stdio_write(int fh, const void* pBuf, int size); // Folded onto stdFileWrite 0x004C05F0; no address of its own, never hook
int J3DAPI stdio_tell(int fh); // Folded onto stdFileTell 0x004C0660; no address of its own, never hook
int J3DAPI stdio_seek(int fh, int offset, int whence); // Folded onto stdFileSeek 0x004EA0C0; no address of its own, never hook
void J3DAPI stdio_flush(int fh);

void Stdio_InstallHooks(void);
void Stdio_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_FILE_STDIO_H
