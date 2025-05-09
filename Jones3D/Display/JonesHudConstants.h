#ifndef JONESHUDCONSTANTS_H
#define JONESHUDCONSTANTS_H
#include <Jones3D/types.h>
#include <rdroid/types.h>

// HUD constants
static const size_t JonesHud_msecGameOverWait = 5000; // 5 sec

// Color constants
static const rdVector4 JonesHud_colorGreen   = { 0.14f, 0.69f, 0.0f, 0.0f };
static const rdVector4 JonesHud_colorYellow  = { 1.0f , 0.76999998f, 0.0f, 0.0f };
static const rdVector4 JonesHud_colorRed     = { 0.82999998f , 0.055f , 0.0f , 0.0f };
static const rdVector4 JonesHud_colorBlack   = { 0.0099999998f , 0.0099999998f , 0.0099999998f , 0.0f };
static const rdVector4 JonesHud_colorWhite   = { 1.0f , 1.0f , 1.0f , 0.0f };
static const rdVector4 JonesHud_skyBlue      = { 0.40000001f ,  0.69999999f , 0.89999998f ,  0.0f };
static const rdVector4 JonesHud_colorPink    = { 1.0f , 0.0f , 1.0f, 0.0f };

// HUD camera constants
static const rdVector3 JonesHud_camViewPos = { 0.0f , -2.0f , 0.64999998f };
static const rdVector3 JonesHud_camViewPyr = { -17.700001f , 0.0f ,  0.0f };

// Store constants

// TODO: Sync this constant with the one in jonesConfig
static const tStoreItem JonesHud_aStoreItems[14] = {
// icon ID |    item store name       |    item ammunition name    |            menu ID              | price | units per bought item 
    { 204, "JONES_STR_STORE_MAUSER"   , "JONES_STR_CLIP_MAUSER"    , JONESHUD_MENU_WEAP_MAUSER       ,   50,  5 },
    { 198, "JONES_STR_STORE_PPSH41"   , "JONES_STR_CLIP_PPSH41"    , JONESHUD_MENU_WEAP_SUBMACHINE   ,  100, 20 },
    { 203, "JONES_STR_STORE_SIMINOV"  , "JONES_STR_CLIP_SIMINOV"   , JONESHUD_MENU_WEAP_SIMONOV      ,   50,  5 },
    { 199, "JONES_STR_STORE_TOKAREV"  , "JONES_STR_CLIP_TOKAREV"   , JONESHUD_MENU_WEAP_TOKAREV      ,   50, 10 },
    { 200, "JONES_STR_STORE_TOZ34"    , "JONES_STR_STORE_TOZ34"    , JONESHUD_MENU_WEAP_SHOTGUN      ,   50,  5 },
    { 202, "JONES_STR_STORE_SATCHEL"  , "JONES_STR_STORE_SATCHEL"  , JONESHUD_MENU_WEAP_SATCHEL      ,   40,  3 },
    { 197, "JONES_STR_STORE_GRENADE"  , "JONES_STR_STORE_GRENADE"  , JONESHUD_MENU_WEAP_GRENADE      ,   20,  3 },
    { 193, "JONES_STR_STORE_BAZOOKA"  , "JONES_STR_STORE_BAZOOKA"  , JONESHUD_MENU_WEAP_BAZOOKA      ,   30,  3 },
    { 194, "JONES_STR_STORE_MEDICINE" , "JONES_STR_STORE_MEDICINE" , JONESHUD_MENU_HEALTH_SMALL      ,  100,  1 },
    { 196, "JONES_STR_STORE_1STAID"   , "JONES_STR_STORE_1STAID"   , JONESHUD_MENU_HEALTH_BIG        ,  300,  1 },
    { 192, "JONES_STR_STORE_VENOM"    , "JONES_STR_STORE_VENOM"    , JONESHUD_MENU_HEALTH_POISONKIT  ,   50,  1 },
    { 201, "JONES_STR_STORE_MAP"      , "JONES_STR_STORE_MAP"      , JONESHUD_MENU_INVITEM_BONUSMAP  , 2500,  0 },
    { 217, "JONES_STR_STORE_BIGHERB"  , "JONES_STR_STORE_BIGHERB"  , JONESHUD_MENU_HEALTH_HERBS_BIG  ,  300,  1 },
    { 227, "JONES_STR_STORE_SMALLHERB", "JONES_STR_STORE_SMALLHERB", JONESHUD_MENU_HEALTH_HERBS_SMALL,  100,  1 }
};


// Sound fx indices to jones3Dstatic sound list
// TODO: Replace indices in the list with some constants
// Note, from index 7 to the end are indy voice lines for the item that can't be activated (e.g. whoops)
static const int JonesHud_aSoundFxIdxs[13] = { -1, 52, 54, 53, -1, 57, 56, 59, 60, 61, 62, 136, 137 };


// Inventory menu item constants
#define JONESHUD_MAX_MENU_ITEMS 134

static const rdVector3 JonesHud_vecSelectedMenuItemScale = { 1.25f , 1.25f , 1.25f };

static const tMenuItemOrientInfo JonesHud_aDfltMenuItemOrients[JONESHUD_MAX_MENU_ITEMS] =
{
    { 1.1f, { { 0.0f }, { 270.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 1.0f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.85000002f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.80000001f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.89999998f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 1.0f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 1.0f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 1.05f, { { 60.0f }, { 90.0f }, { 90.0f } } },
    { 0.60000002f, { { 135.0f }, { 90.0f }, { 135.0f } } },
    { 0.89999998f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 1.0f, { { 60.0f }, { 90.0f }, { 90.0f } } },
    { 0.80000001f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 1.0f, { { 90.0f }, { 180.0f }, { 0.0f } } },
    { 1.0f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.80000001f, { { 45.0f }, { 315.0f }, { 90.0f } } },
    { 0.80000001f, { { 225.0f }, { 270.0f }, { 0.0f } } },
    { 1.25f, { { 225.0f }, { 270.0f }, { 0.0f } } },
    { 1.25f, { { 225.0f }, { 270.0f }, { 0.0f } } },
    { 0.69999999f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.89999998f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85000002f, { { 30.0f }, { 0.0f }, { 315.0f } } },
    { 1.1f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.64999998f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.94999999f, { { 270.0f }, { 180.0f }, { 0.0f } } },
    { 1.1f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.05f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.05f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.69999999f, { { 0.0f }, { 180.0f }, { 0.0f } } },
    { 1.25f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.25f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.60000002f, { { 30.0f }, { 270.0f }, { 0.0f } } },
    { 0.60000002f, { { 30.0f }, { 270.0f }, { 0.0f } } },
    { 0.60000002f, { { 30.0f }, { 270.0f }, { 0.0f } } },
    { 0.60000002f, { { 30.0f }, { 270.0f }, { 0.0f } } },
    { 1.25f, { { 0.0f }, { 150.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 1.1f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 0.69999999f, { { 45.0f }, { 90.0f }, { 0.0f } } },
    { 0.80000001f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.80000001f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.85000002f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.85000002f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.80000001f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 1.0f, { { 45.0f }, { 200.0f }, { 120.0f } } },
    { 0.85000002f, { { 45.0f }, { 135.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 1.0f, { { 315.0f }, { 0.0f }, { 0.0f } } },
    { 0.94999999f, { { 315.0f }, { 0.0f }, { 0.0f } } },
    { 0.64999998f, { { 315.0f }, { 0.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 0.64999998f, { { 45.0f }, { 315.0f }, { 0.0f } } },
    { 0.85000002f, { { 270.0f }, { 0.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 300.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 300.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 300.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 300.0f }, { 0.0f } } },
    { 0.69999999f, { { 45.0f }, { 315.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 225.0f }, { 0.0f } } },
    { 0.55000001f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.80000001f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85000002f, { { 45.0f }, { 270.0f }, { 0.0f } } },
    { 0.85000002f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.85000002f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.80000001f, { { 240.0f }, { 315.0f }, { 0.0f } } },
    { 0.80000001f, { { 240.0f }, { 315.0f }, { 0.0f } } },
    { 0.80000001f, { { 240.0f }, { 315.0f }, { 0.0f } } },
    { 0.85000002f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.89999998f, { { 45.0f }, { 90.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 225.0f }, { 0.0f } } },
    { 0.85000002f, { { 45.0f }, { 225.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 135.0f }, { 0.0f } } },
    { 0.85000002f, { { 225.0f }, { 225.0f }, { 0.0f } } },
    { 0.85000002f, { { 45.0f }, { 270.0f }, { 90.0f } } },
    { 0.85000002f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.80000001f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.75f, { { 0.0f }, { 45.0f }, { 45.0f } } },
    { 0.85000002f, { { 0.0f }, { 25.0f }, { 45.0f } } },
    { 1.0f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 0.69999999f, { { 45.0f }, { 270.0f }, { 90.0f } } },
    { 0.80000001f, { { 45.0f }, { 270.0f }, { 0.0f } } },
    { 1.0f, { { 315.0f }, { 270.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 80.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 240.0f }, { 0.0f } } },
    { 0.89999998f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 225.0f }, { 45.0f } } },
    { 0.85000002f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.85000002f, { { 45.0f }, { 135.0f }, { 0.0f } } },
    { 0.69999999f, { { 225.0f }, { 225.0f }, { 0.0f } } },
    { 0.75f, { { 0.0f }, { 180.0f }, { 45.0f } } },
    { 0.85000002f, { { 90.0f }, { 0.0f }, { 0.0f } } },
    { 0.85000002f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 45.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 45.0f }, { 0.0f } } },
    { 0.85000002f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.85000002f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.64999998f, { { 45.0f }, { 90.0f }, { 0.0f } } },
    { 0.69999999f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.85000002f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 180.0f }, { 0.0f } } },
    { 0.85000002f, { { 315.0f }, { 270.0f }, { 90.0f } } },
    { 0.85000002f, { { 135.0f }, { 270.0f }, { 90.0f } } },
    { 0.80000001f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.75f, { { 0.0f }, { 225.0f }, { 135.0f } } },
    { 0.75f, { { 225.0f }, { 225.0f }, { 0.0f } } },
    { 0.75f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 200.0f }, { 0.0f } } },
    { 0.85000002f, { { 225.0f }, { 225.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 225.0f }, { 45.0f } } },
    { 0.85000002f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.89999998f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.80000001f, { { 45.0f }, { 270.0f }, { 0.0f } } },
    { 0.85000002f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.85000002f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.1f, { { 0.0f }, { 150.0f }, { 0.0f } } },
    { 0.85000002f, { { 45.0f }, { 315.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.80000001f, { { 90.0f }, { 150.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 0.94999999f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 0.94999999f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 330.0f }, { 0.0f } } }
};

static char* JonesHud_apMenuIconModelNames[JONESHUD_MAX_MENU_ITEMS] = {

    // Weapon menu item 3D models
    "weap_gloves_inv.3do",
    "weap_whip_inv.3do",
    "weap_revolver_inv.3do",
    "weap_tokarev_inv.3do",
    "weap_mauser_inv.3do",
    "weap_simonov_inv.3do",
    "weap_machete_inv.3do",
    "weap_ppsh41_inv.3do",
    "weap_toz34_inv.3do",
    "weap_grenade_inv.3do",
    "weap_satchel_inv.3do",
    "weap_bazooka_inv.3do",

    "gen_ammo_mauser.3do",
    "gen_ammo_ppsh41.3do",
    "gen_ammo_simonov.3do",
    "gen_ammo_tokarev.3do",
    "gen_ammo_toz34.3do",
    "gen_rckt_bzooka.3do",
    "weap_rocket_pack.3do",
    "weap_grenade_pack.3do",
    "weap_satchel_pack.3do",

    // Medicine menu item 3D models
    "gen_icon_hlthsml.3do",
    "gen_icon_hlthbig.3do",
    "gen_herb_small.3do",
    "gen_herb_big.3do",
    "gen_icon_bite.3do",
    NULL,

    // Treasury menu item 3D models
    "gen_coins_gold.3do",
    "gen_coins_slvr.3do",
    "gen_idol_gold.3do",
    "gen_idol_slvr.3do",
    "pru_idol.3do",
    "gen_ingots_gold.3do",
    "gen_ingots_slvr.3do",
    "gen_gem_red.3do",
    "gen_gem_green.3do",
    "gen_gem_blue.3do",
    "aet_gem04.3do",
    "riv_box_cash.3do",
    "gen_icon_chest.3do",

    // Inventory item menu 3D models
    "gen_icon_zippo.3do",
    "gen_icon_chalk.3do",
    "imp1_vibrat_inv.3do",
    "imp2_invis_inv.3do",
    "imp3_levit_inv.3do",
    "imp4_energ_inv.3do",
    "imp5_dial_inv.3do",
    "cyn_potshard.3do",
    "bab_gear_elev.3do",
    "bab_tu_key.3do",
    "bab_tblt_a.3do",
    "bab_tblt_b.3do",
    "bab_tblt_c.3do",
    "bab_idol_mrdk.3do",
    "riv_raft_inflt1_inv.3do",
    "riv_kit_ptch.3do",
    "riv_cndl_awind.3do",
    "riv_cndl_bwind.3do",
    "riv_cndl_cwind.3do",
    "riv_cndl_dwind.3do",
    "riv_coin.3do",
    "shs_key_sealdoor.3do",
    "shs_bulb_pot.3do",
    "shs_bulb_leafstem.3do",
    "shs_bulb_bud.3do",
    "gen_bronz_key.3do",
    "shs_key_lady.3do",
    "shw_jar_oil.3do",
    "shw_divkeyblue.3do",
    "shw_divkeygold.3do",
    "shw_divkeyred.3do",
    "lag_key_pdlck.3do",
    "lag_shovel_fold.3do",
    "lag_plane_prop.3do",
    "lag_hammer_small.3do",
    "lag_dvice_armng.3do",
    "lag_crnk_rsty.3do",
    "vol_key_sharkgate.3do",
    NULL,
    NULL,
    "vol_tram_wheel.3do",
    "vol_key_cuffs.3do",
    "tem_key_statue.3do",
    "tem_key_statue_b.3do",
    "tem_key_mnky.3do",
    "tem_sharkey.3do",
    "teo_key_water.3do",
    "teo_idol_bird.3do",
    "teo_idol_fish.3do",
    "teo_idol_jag.3do",
    "teo_mirror.3do",
    NULL,
    "gen_icon_crnk.3do",
    "pyr_minecar_whl.3do",
    "pyr_gem_eye.3do",
    "pyr_watch.3do",
    NULL,
    "pyr_ngin_chain.3do",
    "bab_jcan_gas.3do",
    "pyr_kndlng_cone.3do",
    "gen_bucket_wood.3do",
    "sol_oilcan_oz.3do",
    NULL,
    NULL,
    "sol_fuse_rplc.3do",
    NULL,
    "sol_gem_a.3do",
    "sol_gem_b.3do",
    "sol_gem_c.3do",
    NULL,
    "sol_pymd_key.3do",
    "sol_key_oil.3do",
    "gen_lever_stick.3do",
    "nub_anubis_arm.3do",
    "nub_gear_turtle.3do",
    "nub_key_elvtr.3do",
    "nub_rb_eye.3do",
    "inf_robo_head.3do",
    "aet_marduk_key.3do",
    "aet_mirror.3do",
    "shs_plank_2m.3do",
    "tem_pikes_floor.3do",
    "pru_key_sqr.3do",
    "gen_icon_bonusmap.3do",

    // System menu 3D models
     NULL,
     "iq_icon.3do",
     "gen_icon_help.3do",
     "gen_icon_camera.3do",
     "gen_icon_film.3do",
     "gen_icon_telgrf.3do",
     "gen_icon_type.3do",
     "gen_icon_tv.3do",
     "gen_icon_radio.3do",
     "gen_icon_exit.3do"
};

static char* JonesHud_apItemNames[JONESHUD_MAX_MENU_ITEMS] =
{
    // Weapons menu items
    "JONES_STR_INV_FISTS",
    "JONES_STR_INV_WHIP",
    "JONES_STR_INV_REVOLVER",
    "JONES_STR_INV_TOKEREV",
    "JONES_STR_INV_MAUSER",
    "JONES_STR_INV_SIMONOV",
    "JONES_STR_INV_MACHETE",
    "JONES_STR_INV_PPSH41",
    "JONES_STR_INV_TOZ34",
    "JONES_STR_INV_GRENADE",
    "JONES_STR_INV_SATCHEL",
    "JONES_STR_INV_BAZOOKA",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // Medicine menu
    "JONES_STR_INV_MEDICINE",
    "JONES_STR_INV_1STAID",
    "JONES_STR_INV_HERBSML",
    "JONES_STR_INV_HERBLARGE",
    "JONES_STR_INV_VENOM",
    "JONES_STR_INV_TICKET",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // Item inventory menu
    "JONES_STR_INV_TREASURE",
    "JONES_STR_INV_LIGHTER",
    "JONES_STR_INV_CHALK",
    "JONES_STR_INV_SEISMIC",
    "JONES_STR_INV_INVISO",
    "JONES_STR_INV_ANTIGRAV",
    "JONES_STR_INV_POWER",
    "JONES_STR_INV_DILATOR",
    "JONES_STR_INV_SHARD",
    "JONES_STR_INV_EGEAR",
    "JONES_STR_INV_TRNKEY",
    "JONES_STR_INV_TBLA",
    "JONES_STR_INV_TBLB",
    "JONES_STR_INV_TBLC",
    "JONES_STR_INV_MARDUK",
    "JONES_STR_INV_DFLRFT",
    "JONES_STR_INV_RFTKIT",
    "JONES_STR_INV_CDLA",
    "JONES_STR_INV_CDLB",
    "JONES_STR_INV_CDLC",
    "JONES_STR_INV_CDLD",
    "JONES_STR_INV_COIN",
    "JONES_STR_INV_SEALKEY",
    "JONES_STR_INV_BULBPOT",
    "JONES_STR_INV_LEAF",
    "JONES_STR_INV_FLOWER",
    "JONES_STR_INV_BRONZEKEY",
    "JONES_STR_INV_LADYKEY",
    "JONES_STR_INV_OILJAR",
    "JONES_STR_INV_BLUDVKEY",
    "JONES_STR_INV_GLDDVKEY",
    "JONES_STR_INV_GRNTDVKEY",
    "JONES_STR_INV_PDLCKKY",
    "JONES_STR_INV_ENTOOL",
    "JONES_STR_INV_ZPROP",
    "JONES_STR_INV_RSTYHMMR",
    "JONES_STR_INV_ARMDVCE",
    "JONES_STR_INV_CRANK",
    "JONES_STR_INV_SHKGTKEY",
    "JONES_STR_INV_BLCKTKL",
    "JONES_STR_INV_SHKLKEY",
    "JONES_STR_INV_TRAMWHEEL",
    "JONES_STR_INV_CUFFKEY",
    "JONES_STR_INV_STUKEYA",
    "JONES_STR_INV_STUKEYB",
    "JONES_STR_INV_MKYKEY",
    "JONES_STR_INV_SHKKEY",
    "JONES_STR_INV_WTRKEY",
    "JONES_STR_INV_BRDIDL",
    "JONES_STR_INV_FSHIDL",
    "JONES_STR_INV_JGRIDL",
    "JONES_STR_INV_MIRROR",
    "JONES_STR_INV_DCKKEY",
    "JONES_STR_INV_CRNK",
    "JONES_STR_INV_MCRWHL",
    "JONES_STR_INV_GEMEYE",
    "JONES_STR_INV_WTCH",
    "JONES_STR_INV_RAIL",
    "JONES_STR_INV_DRVCHAIN",
    "JONES_STR_INV_GASCAN",
    "JONES_STR_INV_KNDLNG",
    "JONES_STR_INV_BUCKET",
    "JONES_STR_INV_OILCAN",
    "JONES_STR_INV_TRACK",
    "JONES_STR_INV_TRCKTIE",
    "JONES_STR_INV_FUSE",
    "JONES_STR_INV_MCRWHL",
    "JONES_STR_INV_GEMA",
    "JONES_STR_INV_GEMB",
    "JONES_STR_INV_GEMC",
    "JONES_STR_INV_HORUSEYE",
    "JONES_STR_INV_PYRKEY",
    "JONES_STR_INV_OILKEY",
    "JONES_STR_INV_STICK",
    "JONES_STR_INV_ARM",
    "JONES_STR_INV_BRZEGEAR",
    "JONES_STR_INV_BRZEKEY",
    "JONES_STR_INV_BOSSEYE",
    "JONES_STR_INV_ROBOT",
    "JONES_STR_INV_AETKEY",
    "JONES_STR_INV_MIRROR",
    "JONES_STR_INV_PLANK",
    "JONES_STR_INV_SPIKE",
    "JONES_STR_INV_SQRKEY",

    // System menus
    "JONES_STR_INV_MAP",
    "JONES_STR_INV_STATS",
    "JONES_STR_INV_STATS",
    "JONES_STR_INV_HELP",
    "JONES_STR_INV_SAVE",
    "JONES_STR_INV_LOAD",
    "JONES_STR_INV_DFCTLY",
    "JONES_STR_INV_CONFIG",
    "JONES_STR_INV_DSPLY",
    "JONES_STR_INV_SND",
    "JONES_STR_INV_EXIT"
};


// End credits constants

static const rdVector4 JonesHud_aCreditFontColors[12] =
{
    { { 1.0f }, { 1.0f }, { 1.0f }, { 1.0f } },
    { { 0.95599997f }, { 0.043000001f }, { 0.88999999f }, { 1.0f } },
    { { 0.59200001f }, { 0.0070000002f }, { 0.98400003f }, { 1.0f } },
    { { 0.011f }, { 0.88999999f }, { 0.98799998f }, { 1.0f } },
    { { 0.0f }, { 0.588f }, { 0.0f }, { 1.0f } },
    { { 0.88999999f }, { 0.97600001f }, { 0.011f }, { 1.0f } },
    { { 1.0f }, { 0.0f }, { 0.50099999f }, { 1.0f } },
    { { 1.0f }, { 0.0f }, { 0.0f }, { 1.0f } },
    { { 0.0f }, { 1.0f }, { 0.0f }, { 1.0f } },
    { { 0.0f }, { 0.0f }, { 1.0f }, { 1.0f } },
    { { 1.0f }, { 0.329f }, { 0.0070000002f }, { 1.0f } },
    { { 0.94499999f }, { 0.70899999f }, { 0.086000003f }, { 1.0f } }
};

// TODO: Update flags to be represented as hex numbers
static tJonesCreditEntry JonesHud_aCredits[325] =
{
    { "gen_4_pyramid.mat", 4, 1 },
    { "JONES_STR_HGAME", 2, 1 },
    { "(in alphabetical order)", 16, 0 },
    { "     ", 1, 0 },
    { "JONES_STR_TPROJECT", 0x10, 1 },
    { "HAL BARWOOD", 1, 0 },
    { "gen_4_selfportrait.mat", 4, 2 },
    { "JONES_STR_HENGINE", 2, 2 },
    { "JONES_STR_TLEADPGR", 16, 2 },
    { "PAUL D. LeFEVRE", 1, 0 },
    { "JONES_STR_TPGRS", 16, 2 },
    { "GARY KEITH BRUBAKER", 1, 0 },
    { "PATRICK McCARTHY", 1, 0 },
    { "IFE OLOWE", 1, 0 },
    { "MATTHEW D. RUSSELL", 1, 0 },
    { "STEVE SCHOLL", 1, 0 },
    { "RANDY TUDOR", 1, 0 },
    { "gen_4_sunrise.mat", 4, 3 },
    { "JONES_STR_HWORLD", 2, 3 },
    { "JONES_STR_TLEADLD", 16, 3 },
    { "STEVEN CHEN", 1, 0 },
    { "JONES_STR_TLD", 16, 3 },
    { "JOSEPH CHIANG", 1, 0 },
    { "REED K. DERLETH", 1, 0 },
    { "GEOFF JONES", 1, 0 },
    { "TIM LONGO", 1, 0 },
    { "CHRISTOPHER W. McGEE", 1, 0 },
    { "TIMOTHY R. MILLER", 1, 0 },
    { "DONALD SIELKE", 1, 0 },
    { "PAUL \"Skitoman\" ZABIEREK", 1, 0 },
    { "JONES_STR_TPLCMNT", 16, 3 },
    { "JESSE MOORE", 1, 0 },
    { "gen_4_eye.mat", 4, 4 },
    { "JONES_STR_HART", 2, 4 },
    { "JONES_STR_TART", 16, 4 },
    { "WILLIAM TILLER", 1, 0 },
    { "JONES_STR_TTEXTURE", 16, 4 },
    { "KIM BALESTRERI", 1, 0 },
    { "KATHY HSIEH", 1, 0 },
    { "GREGORY A. KNIGHT", 1, 0 },
    { "KAREN PURDY", 1, 0 },
    { "NATHAN STAPLEY", 1, 0 },
    { "TIM Y. TAO", 1, 0 },
    { "gen_4x_indy2.mat", 4, 5 },
    { "JONES_STR_HMOTION", 2, 5 },
    { "JONES_STR_TLEADANIM", 16, 5 },
    { "DEREK SAKAI", 1, 0 },
    { "JONES_STR_TANIM", 16, 5 },
    { "YOKO (YANG-JA) BALLARD", 1, 0 },
    { "ANSON JEW", 1, 0 },
    { "gen_4_hawk.mat", 4, 6 },
    { "JONES_STR_HSHAPES", 2, 6 },
    { "JONES_STR_TCHAR", 16, 6 },
    { "LEA MAI NGUYEN", 1, 0 },
    { "JONES_STR_TMODEL", 16, 6 },
    { "MIKE DACKO", 1, 0 },
    { "gen_4x_x.mat", 4, 7 },
    { "JONES_STR_HFLASH", 2, 7 },
    { "JONES_STR_TVISUAL", 16, 7 },
    { "JOSEF RICHARDSON", 1, 0 },
    { "gen_4x_check.mat", 4, 8 },
    { "JONES_STR_HARTPATH", 2, 8 },
    { "JONES_STR_TLDARTTECH", 16, 8 },
    { "HARLEY BALDWIN", 1, 0 },
    { "JONES_STR_TARTTECH", 16, 8 },
    { "LISSA KLANOR", 1, 0 },
    { "gen_4x_happy.mat", 4, 9 },
    { "JONES_STR_HDIGGRS", 2, 9 },
    { "JONES_STR_TSNDPGR", 16, 9 },
    { "MIKE McMAHON", 1, 0 },
    { "JONES_STR_TVIDEO", 16, 9 },
    { "VINCE LEE", 1, 0 },
    { "JONES_STR_TVIDEOADPT", 16, 9 },
    { "ANDRE BREMER", 1, 0 },
    { "JONES_STR_TINSTALL", 16, 9 },
    { "DARREN JOHNSON", 1, 0 },
    { "JOE LIGMAN", 1, 0 },
    { "JONES_STR_TTOOL", 16, 9 },
    { "AMIT SHALEV", 1, 0 },
    { "JONES_STR_TSPPGR", 16, 9 },
    { "INGAR SHU", 1, 0 },
    { "HWEI-LI TSAO", 1, 0 },
    { "JONES_STR_TANIM", 16, 9 },
    { "JOHN McLAUGHLIN", 1, 0 },
    { "VAMBERTO MADURO", 1, 0 },
    { "JONES_STR_TEFFECTS", 16, 9 },
    { "C. ANDREW NELSON", 1, 0 },
    { "JONES_STR_TTEXTURE", 16, 9 },
    { "CATHERINE \"Cat\" TILLER", 1, 0 },
    { "gen_4_star.mat", 4, 10 },
    { "JONES_STR_HVOICE", 2, 10 },
    { "JONES_STR_TINDY", 16, 10 },
    { "DOUG LEE", 1, 0 },
    { "JONES_STR_TSOPHIA", 16, 10 },
    { "TASIA VALENZA", 1, 0 },
    { "JONES_STR_TVOL", 16, 10 },
    { "VICTOR RAIDER-WEXLER", 1, 0 },
    { "JONES_STR_TTURNER", 16, 10 },
    { "BRUCE McGILL", 1, 0 },
    { "JONES_STR_TPRIEST", 16, 10 },
    { "CAROLYN SEYMOUR", 1, 0 },
    { "JONES_STR_TBOY", 16, 10 },
    { "GREY DELISLE", 1, 0 },
    { "JONES_STR_TMILITARY", 16, 10 },
    { "ADAM GREGOR", 1, 0 },
    { "ANDREW KOCHERGIN", 1, 0 },
    { "PHIL PROCTOR", 1, 0 },
    { "VLADIMIR ROMANOV", 1, 0 },
    { "JONES_STR_TMARDUK", 16, 10 },
    { "ADAM GREGOR", 1, 0 },
    { "DOUG LEE", 1, 0 },
    { "gen_4_pound.mat", 4, 1 },
    { "JONES_STR_HVPROD", 2, 1 },
    { "JONES_STR_TVPROD", 16, 1 },
    { "DARRAGH O'FARRELL", 1, 0 },
    { "JONES_STR_TVEDIT", 16, 1 },
    { "COYA ELLIOTT", 1, 0 },
    { "CINDY WONG", 1, 0 },
    { "JONES_STR_TVMGR", 16, 1 },
    { "TAMLYNN NIGLIO", 1, 0 },
    { "JONES_STR_TVCOORD", 16, 1 },
    { "PEGGY BARTLETT", 1, 0 },
    { "JONES_STR_TVRECORD", 16, 1 },
    { "SCREENMUSIC STUDIOS", 1, 0 },
    { "Studio City, California", 1, 0 },
    { "LUCASARTS ENTERTAINMENT COMPANY, LLC", 1, 0 },
    { "San Rafael, California", 1, 0 },
    { "gen_4x_circle.mat", 4, 2 },
    { "JONES_STR_HSOUND", 2, 2 },
    { "JONES_STR_TMUSIC", 16, 2 },
    { "CLINT BAJAKIAN", 1, 0 },
    { "JONES_STR_TSNDDSN", 16, 2 },
    { "DAVID LEVISON", 1, 0 },
    { "JONES_STR_TCOMPOSE", 16, 2 },
    { "JOHN WILLIAMS", 1, 0 },
    { "JONES_STR_TSNDASST", 16, 2 },
    { "MICHAEL FRAYNE", 1, 0 },
    { "JORY PRUM", 1, 0 },
    { "JONES_STR_TSNDCOORD", 16, 2 },
    { "KRISTEN BECHT", 1, 0 },
    { "gen_4_five.mat", 4, 3 },
    { "JONES_STR_HPROD", 2, 3 },
    { "JONES_STR_TPRODMGR", 16, 3 },
    { "WAYNE CLINE", 1, 0 },
    { "JONES_STR_TPRODCOORD", 16, 3 },
    { "JENNIFER O'NEAL", 1, 0 },
    { "KELLIE TAUZIN", 1, 0 },
    { "gen_4_skull.mat", 4, 4 },
    { "JONES_STR_HQA", 2, 4 },
    { "JONES_STR_TLEADTEST", 16, 4 },
    { "JOHN \"Buzz\" BUZOLICH", 1, 0 },
    { "JOHN \"Grandpa\" HANNON", 1, 0 },
    { "JONES_STR_TLEADTESTASST", 16, 4 },
    { "BRENT JAY ANDAYA JALIPA II", 1, 0 },
    { "JONES_STR_TTEST", 16, 4 },
    { "PHILLIP \"Philthy\" BERRY", 1, 0 },
    { "GABRIELLE BROWN", 1, 0 },
    { "SHANG-JU CHEN", 1, 0 },
    { "G.W. CHILDS", 1, 0 },
    { "JEFFREY DAY", 1, 0 },
    { "JOHN DRAKE", 1, 0 },
    { "ERIK ELLICOCK", 1, 0 },
    { "BRYAN ERCK", 1, 0 },
    { "DEREK FLIPPO", 1, 0 },
    { "CATHERINE HAIGLER", 1, 0 },
    { "CHANE HOLLANDER", 1, 0 },
    { "HANS LARSON", 1, 0 },
    { "RICARDO LIU", 1, 0 },
    { "TROY MASHBURN", 1, 0 },
    { "CHUCK McFADDEN", 1, 0 },
    { "MATTHEW \"Cyrus\" McMANUS", 1, 0 },
    { "ORION NEMETH", 1, 0 },
    { "TODD STRITTER", 1, 0 },
    { "TORII SWADER", 1, 0 },
    { "JOSEPH TALAVERA II", 1, 0 },
    { "JULIO TORRES", 1, 0 },
    { "JONES_STR_TINTLTEST", 16, 4 },
    { "JEFF SANDERS", 1, 0 },
    { "JONES_STR_TCOMP", 16, 4 },
    { "CHIP HINNENBERG", 1, 0 },
    { "JONES_STR_TCOMPTEST", 16, 4 },
    { "KARSTEN AGLER", 1, 0 },
    { "DARREN BROWN", 1, 0 },
    { "JIM DAVISON", 1, 0 },
    { "MARCUS P. GAINES", 1, 0 },
    { "DOYLE GILSTRAP", 1, 0 },
    { "JASON LAUBOROUGH", 1, 0 },
    { "DAN MIHOERCK", 1, 0 },
    { "CHARLES W. SMITH", 1, 0 },
    { "SCOTT TINGLEY", 1, 0 },
    { "AARON YOUNG", 1, 0 },
    { "JONES_STR_TEBOOK", 16, 4 },
    { "LYNN TAYLOR", 1, 0 },
    { "JONES_STR_TPRODBURN", 16, 4 },
    { "WENDY \"Cupcake\" KAPLAN", 1, 0 },
    { "KELLIE \"Twinky\" WALKER", 1, 0 },
    { "gen_4_questionmark.mat", 4, 5 },
    { "JONES_STR_HSUP", 2, 5 },
    { "JONES_STR_TSUPMGR", 16, 5 },
    { "PAUL PURDY", 1, 0 },
    { "JONES_STR_THINT", 16, 5 },
    { "TABITHA TOSTI", 1, 0 },
    { "gen_4x_indy.mat", 4, 6 },
    { "JONES_STR_HSHOP", 2, 6 },
    { "JONES_STR_TSHOPMGR", 16, 6 },
    { "TOM BYRON", 1, 0 },
    { "JONES_STR_TPRMGR", 16, 6 },
    { "TOM SARRIS", 1, 0 },
    { "JONES_STR_TPACKDSN", 16, 6 },
    { "SOO HOO DESIGN", 1, 0 },
    { "JONES_STR_TPACKPICT", 16, 6 },
    { "DREW STRUZAN", 1, 0 },
    { "BRUCE WOLF", 1, 0 },
    { "JONES_STR_TBOOK", 16, 6 },
    { "JO \"Captain Tripps\" ASHBURN", 1, 0 },
    { "MOLLIE BOERO", 1, 0 },
    { "JONES_STR_TBOOKDSN", 16, 6 },
    { "PATTY HILL", 1, 0 },
    { "JONES_STR_TPRSP", 16, 6 },
    { "HEATHER TWIST", 1, 0 },
    { "JOSH MOORE", 1, 0 },
    { "JONES_STR_TSHOPCOORD", 16, 6 },
    { "ANNE BARSON", 1, 0 },
    { "gen_4_hammer&sickle.mat", 4, 7 },
    { "JONES_STR_HINTL", 2, 7 },
    { "JONES_STR_TLCLINTL", 16, 7 },
    { "ADAM PASZTORY", 1, 0 },
    { "JONES_STR_TINTLDEVSP", 16, 7 },
    { "THERESA O'CONNOR", 1, 0 },
    { "JONES_STR_TINTLPROD", 16, 7 },
    { "BENJAMIN BECKER", 1, 0 },
    { "JONES_STR_TINTLDEV", 16, 7 },
    { "ARIC WILMUNDER", 1, 0 },
    { "JONES_STR_INTLCOORD", 16, 7 },
    { "LEE SUSEN", 1, 0 },
    { "JONES_STR_TINTLPR", 16, 7 },
    { "KATHY APOSTOLI", 1, 0 },
    { "gen_4_alien.mat", 4, 8 },
    { "JONES_STR_HSTAFF", 2, 8 },
    { "JONES_STR_TPRES", 16, 8 },
    { "JACK SORENSEN", 1, 0 },
    { "JONES_STR_TDIRDEV", 16, 8 },
    { "STEVE DAUTERMAN", 1, 0 },
    { "SIMON JEFFERY", 1, 0 },
    { "JONES_STR_TSHOPDIR", 16, 8 },
    { "MARY BIHR", 1, 0 },
    { "JONES_STR_TTECHDIR", 16, 8 },
    { "MALCOLM JOHNSON", 1, 0 },
    { "JONES_STR_TARTDIR", 16, 8 },
    { "KAREN CHELINI", 1, 0 },
    { "JONES_STR_TPRODDIR", 16, 8 },
    { "CAMELA MARTIN", 1, 0 },
    { "JONES_STR_TPRODBIS", 16, 8 },
    { "JEREMY SALESIN", 1, 0 },
    { "JONES_STR_TCTRLR", 16, 8 },
    { "TOM McCARTHY", 1, 0 },
    { "JONES_STR_TQADIR", 16, 8 },
    { "MARK CARTWRIGHT", 1, 0 },
    { "JONES_STR_TQAMGR", 16, 8 },
    { "DAN PETTIT", 1, 0 },
    { "JONES_STR_TSNDMGR", 16, 8 },
    { "MICHAEL LAND", 1, 0 },
    { "JONES_STR_TSALEMGR", 16, 8 },
    { "MEREDITH CAHILL", 1, 0 },
    { "JONES_STR_TINTLMGR", 16, 8 },
    { "LISA STAR", 1, 0 },
    { "JONES_STR_TCHMGR", 16, 8 },
    { "TIM MOORE", 1, 0 },
    { "JONES_STR_TBIS", 16, 8 },
    { "JOCELYN LAMM", 1, 0 },
    { "JANNETT SHIRLEY-PAUL", 1, 0 },
    { "JONES_STR_TADMIN", 16, 8 },
    { "CATHERINE DURAND", 1, 0 },
    { "DENISE GOLLAHER", 1, 0 },
    { "JOHN KNOWLES", 1, 0 },
    { "SANDI MANESIS", 1, 0 },
    { "GABRIEL McDONALD", 1, 0 },
    { "LISA SWART", 1, 0 },
    { "gen_4_ankh.mat", 4, 9 },
    { "JONES_STR_HTHANKS", 2, 9 },
    { "BARBARA BARWOOD", 1, 0 },
    { "MICHELLE MORRIS BRUBAKER", 1, 0 },
    { "THE CHIANG GANG", 1, 0 },
    { "BETH CONNELL & the 008 posse", 1, 0 },
    { "Thanks to JENNIFER for the salsa", 1, 0 },
    { "AMY EWING", 1, 0 },
    { "DR. & MRS. JONES (Bill & Lida)", 1, 0 },
    { "JENNIFER LONGO", 1, 0 },
    { "LIVIA MACKIN", 1, 0 },
    { "RACHEL SAIDMAN", 1, 0 },
    { "SUZIE, HEATHER & KIMMY SIELKE", 1, 0 },
    { "AMY TILLER", 1, 0 },
    { "GINA VENTURI", 1, 0 },
    { "VENUS & MATTHEW", 1, 0 },
    { "ELISA, NICK, JUSTICE & LIBERTY TUDOR", 1, 0 },
    { "TAC", 1, 0 },
    { "BEAN", 1, 0 },
    { "EDISON & OTHELLO", 1, 0 },
    { "MILO & CECIL", 1, 0 },
    { "PIXEL", 1, 0 },
    { "gen_4x_guy.mat", 4, 10 },
    { "JONES_STR_HTIP", 2, 10 },
    { "JONES_STR_TNGINEDEV", 16, 10 },
    { "RAY GRESKO", 1, 0 },
    { "ROB HUEBNER", 1, 0 },
    { "CHE-YUAN WANG", 1, 0 },
    { "WINSTON WOLFF", 1, 0 },
    { "JONES_STR_TCONSOLE", 16, 10 },
    { "PATRICK COSTELLO", 1, 0 },
    { "JONATHAN MENZIES", 1, 0 },
    { "NICK PAVIS", 1, 0 },
    { "JONES_STR_TPGRSUP", 16, 10 },
    { "BOB I. JUDELSON", 1, 0 },
    { "CHRIS KASTENSMIDT", 1, 0 },
    { "THE INTEL CORPORATION", 1, 0 },
    { "JONES_STR_TBOW", 16, 10 },
    { "HARRISON FORD", 1, 0 },
    { "STEVEN SPIELBERG", 1, 0 },
    { "gen_4_r2d2.mat", 4, 1 },
    { "JONES_STR_HSPTHXS", 2, 1 },
    { "GEORGE LUCAS", 1, 0 },
    { "mat\\gg_a.mat", 0x0C, 11 },
    { "mat\\gg_b.mat", 0x0C, 11 },
    { "\xA9 LUCASARTS ENTERTAINMENT COMPANY LLC", 2, 0 },
    { "JONES_STR_HCOPY", 2, 0 }
};

#endif // JONESHUDCONSTANTS_H