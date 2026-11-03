#ifndef JONES3D_JONESHUD_H
#define JONES3D_JONESHUD_H
#include <Jones3D/types.h>
#include <Jones3D/RTI/addresses.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

// Config key macros
#define JONESHUD_CFG_MENU_ITEM_SCALE                    "ui.menu.item.scale"
#define JONESHUD_CFG_MENU_ITEM_TEXT_POS_Y               "ui.menu.item.textPosY"
#define JONESHUD_CFG_MENU_ITEM_TEXT_SIZE                "ui.menu.item.textSize"
#define JONESHUD_CFG_MENU_ITEM_FONT_FILENAME            "ui.menu.item.font"
#define JONESHUD_CFG_MENU_ITEM_MOVEDURATION             "ui.menu.item.moveDuration"
#define JONESHUD_CFG_MENU_COLUMN_HEIGHT                 "ui.menu.column.height"
#define JONESHUD_CFG_MENU_COLUMN_SPACING                "ui.menu.column.spacing"
#define JONESHUD_CFG_MENU_COLUMN_HORIZONTAL_SPACING     "ui.menu.column.horizontalSpacing"
#define JONESHUD_CFG_MENU_COLUMN_EXPAND_DURATION        "ui.menu.column.expandDuration"
#define JONESHUD_CFG_MENU_COLUMN_COLLAPSE_DURATION      "ui.menu.column.collapseDuration"
#define JONESHUD_CFG_MENU_OPEN_START_SLIDE_DURATION     "ui.menu.openStartSlideDuration"
#define JONESHUD_CFG_MENU_OPEN_SLIDE_DURATION           "ui.menu.openSlideDuration"
#define JONESHUD_CFG_MENU_CLOSE_START_SLIDE_DURATION    "ui.menu.closeStartSlideDuration"
#define JONESHUD_CFG_MENU_CLOSE_SLIDE_DURATION          "ui.menu.closeSlideDuration"
#define JONESHUD_CFG_MENU_MUSICENABLED                  "ui.menu.music.enabled"
#define JONESHUD_CFG_MENU_MUSICFILENAME                 "ui.menu.music.music"
#define JONESHUD_CFG_MENU_MUSICEVOLUME                  "ui.menu.music.file"
#define JONESHUD_CFG_MENU_FADEDURATION                  "ui.menu.music.fade"

#define JONESHUD_CFG_ITEM_CHANGED_SHOW_DURATION         "ui.hud.itemChanged.showDuration"
#define JONESHUD_CFG_ITEM_CHANGED_SLIDE_EASE_IN_RATE    "ui.hud.itemChanged.slideEaseInRate"
#define JONESHUD_CFG_ITEM_CHANGED_SLIDE_SPEED_RATE      "ui.hud.itemChanged.slideSpeedRate"

#define JONESHUD_CFG_HEALTH_IND_SIZE                    "ui.hud.indicators.health.size"
#define JONESHUD_CFG_HEALTH_IND_POS_X                   "ui.hud.indicators.health.posX"
#define JONESHUD_CFG_HEALTH_IND_POS_Y                   "ui.hud.indicators.health.posY"
#define JONESHUD_CFG_HEALTH_IND_FADE                    "ui.hud.indicators.health.fade"
#define JONESHUD_CFG_HEALTH_IND_FADE_TIME               "ui.hud.indicators.health.fadeTime"
#define JONESHUD_CFG_HEALTH_IND_BASE_MAT                "ui.hud.indicators.health.textures.base"
#define JONESHUD_CFG_HEALTH_IND_OVERLAY_MAT             "ui.hud.indicators.health.textures.overlay"
#define JONESHUD_CFG_HEALTH_IND_HIT_OVERLAY_MAT         "ui.hud.indicators.health.textures.overlayHit"
#define JONESHUD_CFG_HEALTH_IND_POISONED_OVERLAY_MAT    "ui.hud.indicators.health.textures.overlayPoisoned"
#define JONESHUD_CFG_HEALTH_IND_COLOR_FULL              "ui.hud.indicators.health.colors.full"
#define JONESHUD_CFG_HEALTH_IND_COLOR_MED               "ui.hud.indicators.health.colors.medium"
#define JONESHUD_CFG_HEALTH_IND_COLOR_LOW               "ui.hud.indicators.health.colors.low"
#define JONESHUD_CFG_HEALTH_IND_COLOR_CRITICAL          "ui.hud.indicators.health.colors.critical"

#define JONESHUD_CFG_ENDURANCE_IND_SIZE                 "ui.hud.indicators.endurance.size"
#define JONESHUD_CFG_ENDURANCE_IND_POS_X                "ui.hud.indicators.endurance.posX"
#define JONESHUD_CFG_ENDURANCE_IND_POS_Y                "ui.hud.indicators.endurance.posY"
#define JONESHUD_CFG_ENDURANCE_IND_FADE                 "ui.hud.indicators.endurance.fade"
#define JONESHUD_CFG_ENDURANCE_IND_FADE_TIME            "ui.hud.indicators.endurance.fadeTime"
#define JONESHUD_CFG_ENDURANCE_IND_OVERLAY_MAT          "ui.hud.indicators.endurance.textures.overlay"
#define JONESHUD_CFG_ENDURANCE_IND_COLOR_OXYGEN         "ui.hud.indicators.endurance.colors.oxygen"
#define JONESHUD_CFG_ENDURANCE_IND_COLOR_RAFT           "ui.hud.indicators.endurance.colors.raft"
#define JONESHUD_CFG_ENDURANCE_IND_COLOR_IMP_ENERGY     "ui.hud.indicators.endurance.colors.impEnergy"


int JonesHud_Startup(void);
void JonesHud_Shutdown(void);

int JonesHud_Open(void);
void JonesHud_Close(void);
// TODO: Add function that will discard current state i.e. current item change animation, when loading game

void JonesHud_ToggleMenu(void);
void J3DAPI JonesHud_EnableMenu(int bEnable);
int JonesHud_IsMenuEnabled(void);

void JonesHud_Process(void);

void J3DAPI JonesHud_BindActivateControlKeys(const size_t* aKeyIds, size_t numKeys);
void J3DAPI JonesHud_InventoryItemChanged(int typeId);

// Returns menu item by index or NULL if index >= JONESHUD_MAX_MENU_ITEMS
const JonesHudMenuItem* J3DAPI JonesHud_GetMenuItem(size_t index); // new

// Returns 1 when bonus map was bought
int JonesHud_ShowLevelCompleted(void);

void J3DAPI JonesHud_ShowGameOverDialog(int bPlayDiedMusic);

void J3DAPI JonesHud_SetFadeHealthHUD(int bShow, int bFade);
float JonesHud_GetHealthBarAlpha(void);
void J3DAPI JonesHud_SetHealthBarAlpha(float a1);

void J3DAPI JonesHud_CutsceneStart(int a1);
int J3DAPI JonesHud_EnableInterface(int bEnable);

int J3DAPI JonesHud_DrawCredits(int bEndCredits, tSoundChannelHandle hSndChannel);

void JonesHud_RestoreGameStatistics(void);
void JonesHud_RestoreTreasuresStatistics(void);

// Helper hooking functions
void JonesHud_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESHUD_H
