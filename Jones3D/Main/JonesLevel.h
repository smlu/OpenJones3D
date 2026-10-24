#ifndef JONESLEVEL_H
#define JONESLEVEL_H
#include <j3dcore/j3d.h>
#include <Jones3D/types.h>

#include <stdbool.h>

J3D_EXTERN_C_START

#define JONESLEVEL_NUMLEVELS       18
#define JONESLEVEL_GENERICLEVELNUM  0
#define JONESLEVEL_FIRSTLEVELNUM    1
#define JONESLEVEL_LASTLEVELNUM    16
#define JONESLEVEL_BONUSLEVELNUM   17

static const JonesLevelInfo JonesMain_aCndLevelLoadInfos[JONESLEVEL_NUMLEVELS] =
{
    {
        NULL,
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 366.0f, 221.0f },
        0,
        NULL
    },
    {
        "00_CYN.cnd",
        "mat\\cyn_amap",
        { -5.0f, 187.0f },
        { 345.0f, 344.0f },
        1,
        "CANYONLANDS"
    },
    {
        "01_BAB.cnd",
        "mat\\bab_amap",
        { -5.0f, 163.0f },
        { 419.0f, 251.0f },
        1,
        "BABYLON"
    },
    {
        "02_RIV.cnd",
        "mat\\riv_amap",
        { -5.0f, 344.0f },
        { 423.0f, 130.0f },
        0,
        "TIAN SHAN RIVER"
    },
    {
        "03_SHS.cnd",
        "mat\\shs_amap",
        { 150.0f, 512.0f },
        { 397.0f, 274.0f },
        0,
        "SHAMBALA SANCTUARY"
    },
    {
        "05_LAG.cnd",
        "mat\\lag_amap",
        { -5.0f, 147.0f },
        { 435.0f, 380.0f },
        1,
        "PALAWAN LAGOON"
    },
    {
        "06_VOL.cnd",
        "mat\\vol_amap",
        { -5.0f, 184.0f },
        { 330.0f, 269.0f },
        0,
        "PALAWAN VOLCANO"
    },
    {
        "07_TEM.cnd",
        "mat\\tem_amap",
        { -5.0f, 127.0f },
        { 349.0f, 312.0f },
        0,
        "PALAWAN TEMPLE"
    },
    {
        "16_JEP.cnd",
        "mat\\jep_amap",
        { -5.0f, 223.0f },
        { 370.0f, 309.0f },
        1,
        "JEEP"
    },
    {
        "08_TEO.cnd",
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 365.0f, 221.0f },
        1,
        "TEOTIAACAN"
    },
    {
        "09_OLV.cnd",
        "mat\\olv_amap",
        { -5.0f, 277.0f },
        { 413.0f, 257.0f },
        0,
        "OLMEC VALLEY"
    },
    {
        "10_SEA.cnd",
        "mat\\sea_amap",
        { -5.0f, 252.0f },
        { 380.0f, 199.0f },
        0,
        "PUDOVKIN SHIP"
    },
    {
        "11_PYR.cnd",
        "mat\\pyr_amap",
        { -5.0f, 243.0f },
        { 421.0f, 391.0f },
        1,
        "MEROE PYRAMIDS"
    },
    {
        "12_SOL.cnd",
        "mat\\sol_amap",
        { -5.0f, 208.0f },
        { 348.0f, 266.0f },
        1,
        "SOLOMON'S MINES"
    },
    {
        "13_NUB.cnd",
        "mat\\nub_amap",
        { -5.0f, 237.0f },
        { 383.0f, 245.0f },
        0,
        "NUB'S TOMB"
    },
    {
        "14_INF.cnd",
        "mat\\inf_amap",
        { 85.0f, 485.0f },
        { 395.0f, 224.0f },
        0,
        "INFERNAL MACHINE"
    },
    {
        "15_AET.cnd",
        "mat\\aet_amap",
        { 329.0f, -5.0f },
        { 329.0f, 316.0f },
        1,
        "AETHERIUM"
    },
    {
        "17_PRU.cnd",
        "mat\\pru_amap",
        { 565.0f, -5.0f },
        { 176.0f, 341.0f },
        1,
        "RETURN TO PERU"
    }
};

static const JonesLevelInfo JonesMain_aNdyLevelLoadInfos[JONESLEVEL_NUMLEVELS] =
{
    {
        NULL,
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 366.0f, 221.0f },
        1,
        NULL
    },
    {
        "00_CYN.ndy",
        "mat\\cyn_amap",
        { -5.0f, 187.0f },
        { 345.0f, 344.0f },
        1,
        "CANYONLANDS"
    },
    {
        "01_BAB.ndy",
        "mat\\bab_amap",
        { -5.0f, 163.0f },
        { 419.0f, 251.0f },
        1,
        "BABYLON"
    },
    {
        "02_RIV.ndy",
        "mat\\riv_amap",
        { -5.0f, 344.0f },
        { 423.0f, 130.0f },
        1,
        "TIAN SHAN RIVER"
    },
    {
        "03_SHS.ndy",
        "mat\\shs_amap",
        { 150.0f, 512.0f },
        { 397.0f, 274.0f },
        1,
        "SHAMBALA SANCTUARY"
    },
    {
        "05_LAG.ndy",
        "mat\\lag_amap",
        { -5.0f, 147.0f },
        { 435.0f, 380.0f },
        1,
        "PALAWAN LAGOON"
    },
    {
        "06_VOL.ndy",
        "mat\\vol_amap",
        { -5.0f, 184.0f },
        { 330.0f, 267.0f },
        1,
        "PALAWAN VOLCANO"
    },
    {
        "07_TEM.ndy",
        "mat\\tem_amap",
        { -5.0f, 127.0f },
        { 349.0f, 312.0f },
        1,
        "PALAWAN TEMPLE"
    },
    {
        "16_JEP.ndy",
        "mat\\jep_amap",
        { -5.0f, 223.0f },
        { 370.0f, 308.0f },
        1,
        "JEEP"
    },
    {
        "08_TEO.ndy",
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 365.0f, 221.0f },
        1,
        "TEOTIAACAN"
    },
    {
        "09_OLV.ndy",
        "mat\\olv_amap",
        { -5.0f, 277.0f },
        { 413.0f, 257.0f },
        1,
        "OLMEC VALLEY"
    },
    {
        "10_SEA.ndy",
        "mat\\sea_amap",
        { -5.0f, 252.0f },
        { 380.0f, 199.0f },
        1,
        "PUDOVKIN SHIP"
    },
    {
        "11_PYR.ndy",
        "mat\\pyr_amap",
        { -5.0f, 243.0f },
        { 421.0f, 391.0f },
        1,
        "MEROE PYRAMIDS"
    },
    {
        "12_SOL.ndy",
        "mat\\sol_amap",
        { -5.0f, 208.0f },
        { 348.0f, 266.0f },
        1,
        "SOLOMON'S MINES"
    },
    {
        "13_NUB.ndy",
        "mat\\nub_amap",
        { -5.0f, 237.0f },
        { 383.0f, 245.0f },
        1,
        "NUB'S TOMB"
    },
    {
        "14_INF.ndy",
        "mat\\inf_amap",
        { 85.0f, 485.0f },
        { 395.0f, 224.0f },
        1,
        "INFERNAL MACHINE"
    },
    {
        "15_AET.ndy",
        "mat\\aet_amap",
        { 327.0f, -5.0f },
        { 327.0f, 316.0f },
        1,
        "AETHERIUM"
    },
    {
        "17_PRU.ndy",
        "mat\\pru_amap",
        { 565.0f, -5.0f },
        { 176.0f, 340.0f },
        1,
        "RETURN TO PERU"
    }
};

inline bool J3DAPI JonesLevel_IsValidLevelNum(size_t levelNum)
{
    if ( levelNum != JONESLEVEL_GENERICLEVELNUM && levelNum < JONESLEVEL_NUMLEVELS ) {
        return true;
    }
    return false;
}

J3D_EXTERN_C_END
#endif // JONESLEVEL_H
