// Added: the simulation settings of the run (ADR-0020), for traces: one implementation per platform column.
#ifndef STD_STDCVARNULL_H
#define STD_STDCVARNULL_H
#include <stddef.h>
#include <stdbool.h>

#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

// The number of simulation cvars.
size_t stdCvar_GetNumSimulation(void);

// Copies the name and the value text of simulation cvar `index` (sorted by name, 0-based) into the
// buffers, NUL-terminated. Returns false, writing nothing, for an index out of range, a null buffer
// or a buffer too small for the text.
bool stdCvar_GetSimulation(size_t index, char* pName, size_t nameSize, char* pValue, size_t valueSize);

J3D_EXTERN_C_END
#endif // STD_STDCVARNULL_H
