// Added: Sizes of the real DirectX 9 renderer types, measured from the compiler. The shared
// std/types.h uses them to give the engine opaque stand-in types with the same layouts.
// Numbers only, no includes: a backend includes this after defining its real types and
// asserts that the sizes match.
#ifndef STD_STDBACKENDTYPES_H
#define STD_STDBACKENDTYPES_H

#define STD_SYS_SURFACEDESC_SIZE  32
#define STD_SYS_DISPLAYCAPS_SIZE  304
#define STD_SYS_DEVICE3DDESC_SIZE 304
#define STD_SYS_PIXELFORMAT_SIZE  4
#define STD_SYS_GUID_SIZE         16
#define STD_SYS_ALIGN             4

#endif // STD_STDBACKENDTYPES_H
