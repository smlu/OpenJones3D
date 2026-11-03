#ifndef STD_STDCIRCBUF_H
#define STD_STDCIRCBUF_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

int J3DAPI stdCircBuf_New(tCircularBuffer* pCirc, int numElementsDesired, int sizeOfEachElement);
void J3DAPI stdCircBuf_Free(tCircularBuffer* pCirc);
void J3DAPI stdCircBuf_Purge(tCircularBuffer* pCirc);
void* J3DAPI stdCircBuf_GetNextElement(tCircularBuffer* pCirc);

// Helper hooking functions
void stdCircBuf_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDCIRCBUF_H
