#ifndef STD_STDRENDERCAPTURE_H
#define STD_STDRENDERCAPTURE_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

/**
 * @brief Open a render capture file (format RCAP v1) and write its header.
 *
 * Fails and returns false, logging an error, when a capture is already open (the open capture stays
 * open and untouched), when pPath or pColumn is NULL, when pColumn is empty or longer than 64
 * characters, or when the file cannot be opened or the header cannot be written (then nothing stays
 * open). On success the header is written; when frames capture is currently disabled a FRAMES record
 * with value 0 is written right after it. May be called any time. The caller keeps ownership of both
 * strings; their bytes are copied to the file. Returns true when the capture is open.
 */
bool stdRenderCapture_Open(const char* pPath, const char* pColumn);

/**
 * @brief Close the capture file, free all per-capture state and mark the module closed.
 *
 * A safe no-op when nothing is open. Does not change the frames-enabled flag. May be called any
 * time. Logs nothing.
 */
void stdRenderCapture_Close(void);

/**
 * @brief Return whether a capture file is currently open. May be called any time.
 */
bool stdRenderCapture_IsOpen(void);

/**
 * @brief Enable or disable frame capture. The flag defaults to true and survives Open and Close.
 *
 * When the value changes and a capture is open, a FRAMES record with the new value is written; a
 * failed write closes the capture. Setting the same value writes nothing. May be called any time.
 * Logs an error when writing the record fails.
 */
void stdRenderCapture_SetFramesEnabled(bool bEnabled);

/**
 * @brief Return the frames-enabled flag. May be called any time.
 */
bool stdRenderCapture_GetFramesEnabled(void);

/**
 * @brief Close any open capture and restore all module state to its defaults (frames enabled).
 *
 * May be called any time, including after a failed write already closed the capture. Logs nothing.
 */
void stdRenderCapture_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDRENDERCAPTURE_H
