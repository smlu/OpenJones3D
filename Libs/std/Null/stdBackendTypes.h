// Added: Placeholder backend types for the Null platform (native non-Windows configure).
// No real device: each type is one aligned pointer-size word. Sizes only, no includes: a
// backend includes this after defining its real types and asserts that the sizes match.
#ifndef STD_STDBACKENDTYPES_H
#define STD_STDBACKENDTYPES_H

#define STD_SYS_SURFACEDESC_SIZE  8
#define STD_SYS_DISPLAYCAPS_SIZE  8
#define STD_SYS_DEVICE3DDESC_SIZE 8
#define STD_SYS_PIXELFORMAT_SIZE  8
#define STD_SYS_GUID_SIZE         16
#define STD_SYS_ALIGN             8

#endif // STD_STDBACKENDTYPES_H
