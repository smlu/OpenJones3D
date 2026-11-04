// Added: M6.0 render capture control for the renderer forwarding layer.
#ifndef STD_STDDISPLAYCAPTURE_H
#define STD_STDDISPLAYCAPTURE_H

#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

/**
 * @brief Open a render capture file and arm the std3D_ and stdDisplay_ forwarders to record into it.
 *
 * Calls stdRenderCapture_Open(pPath, pColumn), registers &stdDisplay_g_backBuffer as VBuffer
 * handle 1 and resets the per-capture state of the forwarding layer (the warn-once flags and the
 * remembered back-buffer lock). Returns false, leaving the forwarding layer state untouched, when
 * stdRenderCapture_Open fails (a capture that is already open stays open); when the back buffer
 * does not become handle 1 it logs an error, closes the capture, resets the state and returns
 * false. The caller keeps ownership of both strings. May be called any time; the forwarders record
 * nothing until this returns true.
 */
bool stdDisplayCapture_Open(const char* pPath, const char* pColumn);

/**
 * @brief Close the capture file and reset the per-capture state of the forwarding layer.
 *
 * Calls stdRenderCapture_Close (a safe no-op when nothing is open) and then resets the same state
 * stdDisplayCapture_Open resets. May be called any time, also when no capture was ever opened.
 * Logs nothing.
 */
void stdDisplayCapture_Close(void);

J3D_EXTERN_C_END
#endif // STD_STDDISPLAYCAPTURE_H
