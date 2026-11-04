#ifndef SITH_SITHSPRITE_H
#define SITH_SITHSPRITE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHSPRITE_CFG_WORLDSPRITES_EXTRACAPACITY "engine.world.static.sprites.extraCapacity"

int sithSprite_Startup(void);
void sithSprite_Shutdown(void);

void J3DAPI sithSprite_Update(SithThing* pThing, uint32_t msecDeltaTime);
int J3DAPI sithSprite_Draw(SithThing* pThing);

void J3DAPI sithSprite_Create(SithThing* pThing);
void J3DAPI sithSprite_Initialize(SithThing* pThing);
rdSprite3* J3DAPI sithSprite_Load(SithWorld* pWorld, const char* pName);

int J3DAPI sithSprite_AllocWorldSprites(SithWorld* pWorld, size_t size);
void J3DAPI sithSprite_FreeWorldSprites(SithWorld* pWorld);

int J3DAPI sithSprite_WriteStaticSpritesListText(const SithWorld* pWorld); // Added: From debug version
int J3DAPI sithSprite_ReadStaticSpritesListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithSprite_WriteStaticSpritesListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithSprite_ReadStaticSpritesListBinary(tFileHandle fh, SithWorld* pWorld);

void sithSprite_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHSPRITE_H
