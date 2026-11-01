#ifndef SITH_SITHGAMESAVESYS_H
#define SITH_SITHGAMESAVESYS_H

#include <sith/Dss/sithGamesave.h>
#include <std/General/stdColor.h>

J3D_EXTERN_C_START

#define SITHSAVEGAME_THUMBCOLORFORMAT stdColor_cfRGB888 // Note, changed encoding format due to little-endian fix in stdColor_ColorConvertOneRow
#define SITHSAVEGAME_THUMBSIZE       (SITHSAVEGAME_THUMB_WIDTH * SITHSAVEGAME_THUMB_HEIGHT * SITHSAVEGAME_THUMBCOLORFORMAT.bpp) / 8
//static_assert(SITHSAVEGAME_THUMBSIZE == 0x9000, "SITHSAVEGAME_THUMBSIZE must be  0x9000 bytes");

#define SITHSAVEGAME_FILEVERSION  13

// The save game thumbnail state, shared between sithGamesave.c and the Win32 thumbnail code in
// sithGamesaveWin32.c.
extern tVBuffer* sithGamesave_pThumbnailImage;
extern int sithGamesave_bThumbnail;
extern tSysBitmap sithGamesave_hBmpThumbnail;

// Releases the Win32 thumbnail bitmap exactly as sithGamesave_Close does; a safe no-op when not set.
void sithGamesaveSys_ReleaseThumbnail(void); // Implemented in Libs/sith/Dss/sithGamesaveWin32.c

J3D_EXTERN_C_END
#endif // SITH_SITHGAMESAVESYS_H
