// Case tables of the sithAIMove vectors (included by sithAIMoveVec.h). Every case is named <Function>_<state> and
// edge cases <Function>_edge_<state>. The AI actor stands at the origin looking along +Y with +Z up unless a case says
// otherwise (so +X is to its right); times are multiples of 1/16 or 1/32 seconds unless a threshold is probed.
#ifndef SITH_AI_TESTS_SITHAIMOVEVECCASES_H
#define SITH_AI_TESTS_SITHAIMOVEVECCASES_H

#define CALLS(name, ...) static const SithAIMoveVecCall sithAIMoveVec_c##name[] = { __VA_ARGS__ };
#define PARAMS(name, calls, ...)                                                                               \
    static const SithAIMoveVecParams sithAIMoveVec_p##name = { __VA_ARGS__,                                    \
        .numCalls = (uint8_t)SITHVEC_ARRAYLEN(sithAIMoveVec_c##calls), .aCalls = sithAIMoveVec_c##calls };

#define MODE(x)    SITHAI_MODE_##x
#define SUB(x)     SITHAI_SUBMODE_##x
#define AFLAG(x)   SITH_AF_##x
#define TFLAG(x)   SITH_TF_##x
#define PFLAG(x)   SITH_PF_##x
#define MSTAT(x)   SITHPLAYERMOVE_##x
#define PSUB(x)    SITHPUPPETSUBMODE_##x

// Frame scripts
CALLS(Dt1, { .f = 0.0625f })
CALLS(Dt3, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.0625f })
CALLS(Dt6, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.0625f })
CALLS(Dt0, { .f = 0.0f })
CALLS(DtQuarter4, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f })
CALLS(DtTurn1, { .f = 0.25f })
CALLS(DtBig, { .f = 1.0f })
CALLS(DtTiny, { .f = 0.015625f }, { .f = 0.015625f })

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_Update
// ---------------------------------------------------------------------------------------------------------------
PARAMS(UpdIdle, Dt1, 0)
PARAMS(UpdItem, Dt1, .type = SITH_THING_ITEM)
PARAMS(UpdDead, Dt1, .health = -1.0f, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdAliveTiny, Dt1, .health = 0.25f, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdDeadZero, Dt1, .zeroHealth = 1, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdSleeping, Dt1, .mode = MODE(SLEEPING) | MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdSleepingBlock, Dt1, .mode = MODE(SLEEPING) | MODE(BLOCK) | MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdSleepingNoSlope, Dt1, .mode = MODE(SLEEPING) | MODE(TURNING), .actorFlags = AFLAG(NOSLOPEMOVE), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdFlagDisabled, Dt1, .mode = MODE(TURNING), .thingFlags = TFLAG(DISABLED), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdFlagDying, Dt1, .mode = MODE(TURNING), .thingFlags = TFLAG(DYING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdFlagDestroyed, Dt1, .mode = MODE(TURNING), .thingFlags = TFLAG(DESTROYED), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdFlagSubmerged, Dt1, .mode = MODE(TURNING), .thingFlags = TFLAG(SUBMERGED), .goalLVec = { 1.0f, 0.0f, 0.0f })

// Turning toward +X in 22.5 degree steps (maxRot 90), through the final snap
PARAMS(UpdTurnRight, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdTurnLeftBehind, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 0.0f, -1.0f, 0.0f })
PARAMS(UpdTurnBoss, DtQuarter4, .mode = MODE(TURNING), .actorFlags = AFLAG(BOSS), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdTurnBossUp, Dt1, .mode = MODE(TURNING), .actorFlags = AFLAG(BOSS), .goalLVec = { 0.0f, 0.0f, 1.0f })
PARAMS(UpdTurnMineCar, DtQuarter4, .mode = MODE(TURNING), .physFlags = PFLAG(MINECAR), .zeroTweak = 1, .meshIdx = 3, .lookPos = { 4.0f, 0.0f, 0.0f }, .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdTurnMineCarNotPhysics, Dt1, .notPhysics = 1, .mode = MODE(TURNING), .physFlags = PFLAG(MINECAR), .zeroTweak = 1, .meshIdx = 3, .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdBodyTrackTarget, DtQuarter4, .submode = SUB(BODYTRACKINGMOTION), .target = 1, .targetPos = { 4.0f, 0.0f, 0.0f })
PARAMS(UpdBodyTrackTurning, DtQuarter4, .mode = MODE(TURNING), .submode = SUB(BODYTRACKINGMOTION), .target = 1, .targetPos = { 4.0f, 4.0f, 0.0f })
PARAMS(UpdBodyTrackTurningHeadEnabled, Dt1, .mode = MODE(TURNING), .submode = SUB(BODYTRACKINGMOTION) | SUB(HEADTRACKINGENABLED), .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetPos = { 4.0f, 4.0f, 0.0f }, .headPYR = { 0.0f, 5.0f, 0.0f })
PARAMS(UpdBodyTrackNoTarget, Dt3, .submode = SUB(BODYTRACKINGMOTION), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UpdBodyTrackHeadEnabledFar, Dt1, .submode = SUB(BODYTRACKINGMOTION) | SUB(HEADTRACKINGENABLED), .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetPos = { 4.0f, 0.0f, 0.0f }, .headPYR = { 0.0f, 25.0f, 0.0f })
PARAMS(UpdBodyTrackHeadEnabledNear, Dt1, .submode = SUB(BODYTRACKINGMOTION) | SUB(HEADTRACKINGENABLED), .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetPos = { 4.0f, 0.0f, 0.0f }, .headPYR = { 0.0f, 15.0f, 0.0f })
PARAMS(UpdBodyTrackHeadEnabledNegFar, Dt1, .submode = SUB(BODYTRACKINGMOTION) | SUB(HEADTRACKINGENABLED), .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetPos = { 4.0f, 0.0f, 0.0f }, .headPYR = { 0.0f, -21.0f, 0.0f })
PARAMS(UpdBodyTrackHeadEnabledEdge, Dt1, .submode = SUB(BODYTRACKINGMOTION) | SUB(HEADTRACKINGENABLED), .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetPos = { 4.0f, 0.0f, 0.0f }, .headPYR = { 0.0f, 20.0f, 0.0f })

// Moving: the look position follows the move position, alignment, then the move itself
PARAMS(UpdMoveForward, Dt3, .mode = MODE(MOVING), .submode = SUB(MOVEFORWARD), .movePos = { 0.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING)
PARAMS(UpdMoveForwardSide, Dt6, .mode = MODE(MOVING), .submode = SUB(MOVEFORWARD), .movePos = { 4.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING, .maxRot = 360.0f)
PARAMS(UpdMoveForwardNotMoving, Dt3, .submode = SUB(MOVEFORWARD), .movePos = { 4.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING)
PARAMS(UpdMoveForwardDisabled, Dt3, .mode = MODE(MOVING) | MODE(DISABLED), .submode = SUB(MOVEFORWARD), .movePos = { 4.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING)
PARAMS(UpdMoveForwardDisabledEditor, Dt1, .mode = MODE(MOVING) | MODE(DISABLED), .submode = SUB(MOVEFORWARD), .movePos = { 4.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING, .debugModeFlags = SITHDEBUG_INEDITOR)
PARAMS(UpdMoveAlignBackwards, Dt3, .mode = MODE(MOVING), .submode = SUB(ALIGNING) | SUB(NOMOVEBACKWARDS), .goalLVec = { 0.0f, -1.0f, 0.0f }, .movePos = { 0.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING)
PARAMS(UpdMoveAlignForwards, Dt3, .mode = MODE(MOVING), .submode = SUB(ALIGNING) | SUB(NOMOVEBACKWARDS), .goalLVec = { 0.0f, 1.0f, 0.0f }, .movePos = { 0.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING)
PARAMS(UpdMoveAlignPlain, Dt3, .mode = MODE(MOVING), .submode = SUB(ALIGNING), .goalLVec = { 0.0f, -1.0f, 0.0f }, .movePos = { 0.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING)
PARAMS(UpdMoveAlignWpnts, Dt3, .mode = MODE(MOVING) | MODE(TRAVERSEWPNTS), .submode = SUB(ALIGNING), .goalLVec = { 0.0f, 1.0f, 0.0f }, .movePos = { 0.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING)
PARAMS(UpdMoveOnly, Dt3, .mode = MODE(MOVING), .movePos = { 0.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING)
PARAMS(UpdMovePlusTurn, Dt6, .mode = MODE(MOVING) | MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f }, .movePos = { 8.0f, 0.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING)

// Weapons and head tracking
PARAMS(UpdWeaponSelected, Dt1, .hasWeapon = 1)
PARAMS(UpdWeaponAiming, Dt1, .hasWeapon = 1, .isAiming = 1, .mode = MODE(LOOKING), .actorFlags = AFLAG(CANROTATEHEAD))
PARAMS(UpdAimingNoWeapon, Dt1, .isAiming = 1, .mode = MODE(LOOKING), .actorFlags = AFLAG(CANROTATEHEAD))
PARAMS(UpdLookTarget, Dt3, .mode = MODE(LOOKING), .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(UpdLookBlock, Dt3, .mode = MODE(LOOKING) | MODE(BLOCK), .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(UpdLookBlockHeadEnabled, Dt3, .mode = MODE(LOOKING) | MODE(BLOCK), .submode = SUB(HEADTRACKINGENABLED), .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(UpdLookNoSlopeHeadEnabled, Dt3, .mode = MODE(LOOKING), .submode = SUB(HEADTRACKINGENABLED), .actorFlags = AFLAG(CANROTATEHEAD) | AFLAG(NOSLOPEMOVE), .target = 1, .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(UpdLookNoSlope, Dt3, .mode = MODE(LOOKING), .actorFlags = AFLAG(CANROTATEHEAD) | AFLAG(NOSLOPEMOVE), .target = 1, .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(UpdHeadEnabledOnly, Dt3, .submode = SUB(HEADTRACKINGENABLED), .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetPos = { -4.0f, 4.0f, 0.0f }, .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(UpdTurningHeadBlock, Dt3, .mode = MODE(TURNING) | MODE(BLOCK), .actorFlags = AFLAG(CANROTATEHEAD), .goalLVec = { 1.0f, 0.0f, 0.0f }, .lookPos = { 4.0f, 0.0f, 0.0f })

// Tail
PARAMS(UpdQuetzTail, Dt3, .tail = 1, .quetz = 1, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM3, .strikeSnap = 1, .strikeNum = 5, .strikeTime = 0.0f, .snapStep = 0.0625f)
PARAMS(UpdTailNoBlock, Dt1, .tail = 1)
PARAMS(UpdAttachedNoTailFlag, Dt1, .tail = 2, .pName = "marduk", .jointIdx = 5)
PARAMS(UpdAttachedNoTailFlagOther, Dt1, .tail = 2)
PARAMS(UpdMarduk, Dt3, .pName = "marduk", .jointIdx = 5)
PARAMS(UpdMarduk2, Dt1, .pName = "marduk2", .jointIdx = 5)
PARAMS(UpdMardukPrefix, Dt1, .pName = "marduk_boss", .jointIdx = 5)
PARAMS(UpdMardukNoJoint, Dt1, .pName = "marduk", .jointIdx = -1)
PARAMS(UpdTailAndMarduk, Dt1, .tail = 1, .pName = "marduk", .jointIdx = 5)

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_UpdateMineCar
// ---------------------------------------------------------------------------------------------------------------
PARAMS(McImmobile, Dt1, .actorFlags = AFLAG(IMMOBILE), .zeroTweak = 1, .meshIdx = 3, .lookPos = { 4.0f, 0.0f, 0.0f })
PARAMS(McTurnRight, DtQuarter4, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 4.0f, 0.0f, 0.0f }, .moveStatus = MSTAT(WALKING))
PARAMS(McTurnLeft, DtQuarter4, .zeroTweak = 1, .meshIdx = 3, .lookPos = { -4.0f, 0.0f, 0.0f })
PARAMS(McTurnBehind, DtQuarter4, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.0f, -4.0f, 0.0f })
PARAMS(McAligned, Dt1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.0f, 4.0f, 0.0f })
PARAMS(McNearGoal, Dt1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.0625f, 4.0f, 0.0f })
PARAMS(McSlowZone, Dt3, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.25f, 4.0f, 0.0f })
PARAMS(McSmall, Dt1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.0625f, 4.0f, 0.0f }, .maxRot = 720.0f)
PARAMS(McTurn45, Dt1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(McLookUp, Dt1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.0f, 0.0f, 4.0f })
PARAMS(McLookDownFly, Dt1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.0f, 0.0f, -4.0f }, .physFlags = PFLAG(FLY))
PARAMS(McMeshIndex0, Dt1, .zeroTweak = 1, .meshIdx = 0, .lookPos = { 4.0f, 0.0f, 0.0f })
PARAMS(McMoveStatusSet, Dt1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 4.0f, 0.0f, 0.0f }, .moveStatus = MSTAT(ROLLING_LEFT))
CALLS(DtBig1, { .f = 1.0f })
PARAMS(McRandomTweakRight, DtQuarter4, .meshIdx = 3, .lookPos = { 4.0f, 0.0f, 0.0f })
PARAMS(McRandomTweakLeft, DtQuarter4, .meshIdx = 5, .lookPos = { -4.0f, 4.0f, 0.0f })
PARAMS(McLimitEqual, DtBig1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 4.0f, 0.0f, 0.0f }, .maxRot = 85.7142868f)
PARAMS(McLimitBelow, DtBig1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 4.0f, 0.0f, 0.0f }, .maxRot = 85.7142792f)
PARAMS(McLimitAbove, DtBig1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 4.0f, 0.0f, 0.0f }, .maxRot = 85.7142944f)
PARAMS(McLimitEqualBehind, DtBig1, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.0f, -4.0f, 0.0f }, .maxRot = 171.428574f)
PARAMS(McZeroDt, Dt0, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 4.0f, 0.0f, 0.0f })
PARAMS(McZeroDtBehind, Dt0, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.0f, -4.0f, 0.0f })
PARAMS(McNegativeRate, DtQuarter4, .zeroTweak = 1, .meshIdx = 3, .lookPos = { 0.0f, 4.0f, 0.0f }, .maxRot = -90.0f)
PARAMS(McTilted, DtQuarter4, .zeroTweak = 1, .meshIdx = 3, .hx = 0, .hy = 3, .hz = 1, .ux = 0, .uy = -1, .uz = 3, .lookPos = { 4.0f, 2.0f, 1.0f })

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_UpdateTurn
// ---------------------------------------------------------------------------------------------------------------
PARAMS(UtRight, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UtLeft, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { -1.0f, 0.0f, 0.0f })
PARAMS(UtRight30, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 0.5f, 0.8660254f, 0.0f })
PARAMS(UtLeft30, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { -0.5f, 0.8660254f, 0.0f })
PARAMS(UtRight50, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 0.766f, 0.6428f, 0.0f })
PARAMS(UtLeft50, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { -0.766f, 0.6428f, 0.0f })
PARAMS(UtBehind, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 0.0f, -1.0f, 0.0f })
PARAMS(UtDiag, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 0.5f, 0.5f, 0.0f })
PARAMS(UtDiagLeft, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { -0.5f, 1.0f, 0.0f })
PARAMS(UtSlowZone, Dt6, .mode = MODE(TURNING), .goalLVec = { 0.0625f, 1.0f, 0.0f })
PARAMS(UtSlowZoneNeg, Dt6, .mode = MODE(TURNING), .goalLVec = { -0.09375f, 1.0f, 0.0f })
PARAMS(UtBelowSlowAngle, Dt3, .mode = MODE(TURNING), .goalLVec = { 0.015625f, 1.0f, 0.0f })
PARAMS(UtAligned, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 1.0f, 0.0f })
PARAMS(UtImmobile, Dt1, .mode = MODE(TURNING), .actorFlags = AFLAG(IMMOBILE), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UtStationary, Dt1, .mode = MODE(TURNING), .moveStatus = MSTAT(MOUNTING_WALL), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UtStationaryTurn90, Dt1, .mode = MODE(TURNING), .moveStatus = MSTAT(TURNING_LEFT_90_DEGREES), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UtAnimated, Dt3, .mode = MODE(TURNING), .moveStatus = MSTAT(ROLLING_LEFT), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(UtAnimatedSnap, Dt1, .mode = MODE(TURNING), .moveStatus = MSTAT(STAND2WALK), .goalLVec = { 0.0625f, 1.0f, 0.0f })
PARAMS(UtStatusKept, Dt1, .mode = MODE(TURNING), .moveStatus = MSTAT(WALKING), .goalLVec = { 0.0f, 1.0f, 0.0f })
PARAMS(UtUpDotEqual, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.04f, 0.99900001f })
PARAMS(UtUpDotAbove, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.04f, 0.99900007f })
PARAMS(UtLimitEqual, DtBig1, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f }, .maxRot = 85.7142868f)
PARAMS(UtLimitBelow, DtBig1, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f }, .maxRot = 85.7142792f)
PARAMS(UtLimitAbove, DtBig1, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f }, .maxRot = 85.7142944f)
PARAMS(UtLimitEqualLeft, DtBig1, .mode = MODE(TURNING), .goalLVec = { -1.0f, 0.0f, 0.0f }, .maxRot = 85.7142868f)
PARAMS(UtLimitEqualBehind, DtBig1, .mode = MODE(TURNING), .goalLVec = { 0.0f, -1.0f, 0.0f }, .maxRot = 171.428574f)
PARAMS(UtNegativeRate, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 0.0f, 1.0f, 0.0f }, .maxRot = -90.0f)
PARAMS(UtAngle45Right, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 1.00000262f, 1.0f, 0.0f })
PARAMS(UtAngle45Left, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { -1.00000262f, 1.0f, 0.0f })
PARAMS(UtLookUp, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.0f, 1.0f })
PARAMS(UtLookDown, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.0f, -1.0f })
PARAMS(UtLookUpFly, Dt1, .mode = MODE(TURNING), .physFlags = PFLAG(FLY), .goalLVec = { 0.0f, 0.0f, 1.0f })
PARAMS(UtSteep, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.03125f, 1.0f })
PARAMS(UtSteepBelow, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.0625f, 1.0f })
PARAMS(UtTilted, DtQuarter4, .mode = MODE(TURNING), .hx = 0, .hy = 3, .hz = 1, .ux = 0, .uy = -1, .uz = 3, .goalLVec = { 1.0f, 2.0f, 1.0f })
PARAMS(UtFastTurn, Dt1, .mode = MODE(TURNING), .maxRot = 720.0f, .goalLVec = { 1.0f, 0.0f, 0.0f })

// Special turns: the angle selects the turn animation; the keyframes decide whether it is played
PARAMS(UtSpecialTurn30, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { 0.5f, 1.0f, 0.0f }, .numKf = 2, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN) })
PARAMS(UtSpecialTurn20, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { 0.25f, 1.0f, 0.0f }, .numKf = 2, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN) })
PARAMS(UtSpecialTurn60, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { 1.0f, 0.5f, 0.0f }, .numKf = 2, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN) })
PARAMS(UtSpecialTurn90, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { 1.0f, 0.0f, 0.0f }, .numKf = 4, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT) })
PARAMS(UtSpecialTurn90Left, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { -1.0f, 0.0f, 0.0f }, .numKf = 4, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT) })
PARAMS(UtSpecialTurn90NoKf, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { 1.0f, 0.0f, 0.0f }, .numKf = 2, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN) })
PARAMS(UtSpecialTurn135, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { 1.0f, -1.0f, 0.0f }, .numKf = 6, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT), PSUB(WHIPCLIMBDISMOUNT), PSUB(WHIPSWINGMOUNT) })
PARAMS(UtSpecialTurn135NoKf, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { 1.0f, -1.0f, 0.0f }, .numKf = 3, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBMOUNT) })
PARAMS(UtSpecialTurn180, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { 0.0f, -1.0f, 0.0f }, .numKf = 3, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBMOUNT) })
PARAMS(UtSpecialTurn180NoKf, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .goalLVec = { 0.0f, -1.0f, 0.0f }, .numKf = 2, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN) })
PARAMS(UtSpecialTurnAnimated, Dt3, .mode = MODE(TURNING), .submode = SUB(SPECIALTURNS), .moveStatus = MSTAT(ROLLING_LEFT), .goalLVec = { 1.0f, 0.0f, 0.0f }, .numKf = 4, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT) })

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_UpdateMove
// ---------------------------------------------------------------------------------------------------------------
#define UM_BASE .mode = MODE(MOVING), .movePos = { 0.0f, 4.0f, 0.0f }, .moveSpeed = 1.0f, .attachFlags = SITH_ATTACH_THING

PARAMS(UmWalk, Dt6, UM_BASE)
PARAMS(UmWalkSide, Dt3, UM_BASE, .movePos = { 4.0f, 0.0f, 0.0f })
PARAMS(UmWalkSlow, Dt3, UM_BASE, .moveSpeed = 0.25f)
PARAMS(UmWalkNear, Dt3, UM_BASE, .movePos = { 0.0f, 0.125f, 0.0f })
PARAMS(UmWalkVeryNear, Dt3, UM_BASE, .movePos = { 0.0f, 0.03125f, 0.0f })
PARAMS(UmWalkOnSpot, Dt1, UM_BASE, .movePos = { 0.0f, 0.0f, 0.0f })
PARAMS(UmWalkVelocity, Dt3, UM_BASE, .velocity = { 0.0f, 0.125f, 0.0f })
PARAMS(UmWalkVelocitySmall, Dt3, UM_BASE, .velocity = { 0.0f, 0.00001f, 0.0f }, .movePos = { 0.0f, 0.0f, 0.0f })
PARAMS(UmDisabledFlag, Dt1, UM_BASE, .thingFlags = TFLAG(DISABLED))
PARAMS(UmDyingFlag, Dt1, UM_BASE, .thingFlags = TFLAG(DYING))
PARAMS(UmDestroyedFlag, Dt1, UM_BASE, .thingFlags = TFLAG(DESTROYED))
PARAMS(UmImmobile, Dt1, UM_BASE, .actorFlags = AFLAG(IMMOBILE))
PARAMS(UmPaused, Dt1, UM_BASE, .msecGameTime = 1000, .msecPauseMoveUntil = 2000)
PARAMS(UmPauseOver, Dt1, UM_BASE, .msecGameTime = 2000, .msecPauseMoveUntil = 2000)
PARAMS(UmNewMoveBits, Dt1, UM_BASE, .submode = SUB(NEWMOVE) | SUB(UNKNOWN_4) | SUB(ALIGNING))
PARAMS(UmMoveForward, Dt6, UM_BASE, .submode = SUB(MOVEFORWARD))
PARAMS(UmMoveForwardDiag, Dt3, UM_BASE, .submode = SUB(MOVEFORWARD), .movePos = { 4.0f, 4.0f, 0.0f })
PARAMS(UmMoveForwardTurned, Dt3, UM_BASE, .submode = SUB(MOVEFORWARD), .movePos = { 4.0f, 0.0f, 0.0f })
PARAMS(UmMoveForwardBehind, Dt3, UM_BASE, .submode = SUB(MOVEFORWARD), .movePos = { 0.0f, -4.0f, 0.0f })
PARAMS(UmMoveForwardSideGoal, Dt3, UM_BASE, .submode = SUB(MOVEFORWARD), .movePos = { 0.03125f, 4.0f, 0.0f })
PARAMS(UmMoveForwardSideGoalLeft, Dt3, UM_BASE, .submode = SUB(MOVEFORWARD), .movePos = { -0.03125f, 4.0f, 0.0f })
PARAMS(UmMoveForwardSideGoalFar, Dt3, UM_BASE, .submode = SUB(MOVEFORWARD), .movePos = { 0.25f, 4.0f, 0.0f })
PARAMS(UmMatchVelocity, Dt3, UM_BASE, .mode = MODE(MOVING) | MODE(MATCHVELOCITY), .goal = 1, .goalVelocity = { 0.0f, 0.5f, 0.0f }, .velocity = { 0.25f, 0.25f, 0.0f })
PARAMS(UmMatchVelocityNoGoal, Dt3, UM_BASE, .mode = MODE(MOVING) | MODE(MATCHVELOCITY))
PARAMS(UmMatchVelocityZeroOwn, Dt3, UM_BASE, .mode = MODE(MOVING) | MODE(MATCHVELOCITY), .goal = 1, .goalVelocity = { 0.5f, 0.0f, 0.0f })
PARAMS(UmStationary, Dt3, UM_BASE, .moveStatus = MSTAT(MOUNTING_WALL), .velocity = { 0.0f, 0.125f, 0.0f })
PARAMS(UmAnimatedStatus, Dt3, UM_BASE, .moveStatus = MSTAT(ROLLING_RIGHT))
PARAMS(UmNoCheckCliff, Dt3, UM_BASE, .mode = MODE(MOVING) | MODE(NOCHECKFORCLIFF), .aCheckPos = { SITHAIUTIL_FLOOR_NONE }, .numCheckPos = 1)
PARAMS(UmCliffNone, Dt3, UM_BASE, .aCheckPos = { SITHAIUTIL_FLOOR_NONE }, .numCheckPos = 1)
PARAMS(UmCliffBlocked, Dt3, UM_BASE, .aCheckPos = { SITHAIUTIL_FLOOR_BLOCKED }, .numCheckPos = 1)
PARAMS(UmWall, Dt3, UM_BASE, .aCheckPos = { SITHAIUTIL_FLOOR_WALL }, .numCheckPos = 1)
PARAMS(UmWallCrawlInstinct, Dt3, UM_BASE, .aCheckPos = { SITHAIUTIL_FLOOR_WALL }, .numCheckPos = 1, .hasInstinct = 1)
PARAMS(UmThingBlock, Dt3, UM_BASE, .aCheckPos = { SITHAIUTIL_FLOOR_THING }, .numCheckPos = 1)
PARAMS(UmThingStep, Dt3, UM_BASE, .aCheckPos = { SITHAIUTIL_FLOOR_THING }, .numCheckPos = 1, .submode = SUB(ALLOWSTEPTHING))
PARAMS(UmFloorUnknown, Dt3, UM_BASE, .aCheckPos = { 3 }, .numCheckPos = 1)
PARAMS(UmFloorSequence, Dt6, UM_BASE, .aCheckPos = { SITHAIUTIL_FLOOR_OK, SITHAIUTIL_FLOOR_NONE, SITHAIUTIL_FLOOR_WALL, SITHAIUTIL_FLOOR_THING }, .numCheckPos = 4)
PARAMS(UmUnreachableBlock, Dt3, UM_BASE, .mode = MODE(MOVING) | MODE(BLOCK), .aCheckPos = { 3 }, .numCheckPos = 1)
PARAMS(UmNotAttached, Dt3, UM_BASE, .attachFlags = 0)
PARAMS(UmFly, Dt3, UM_BASE, .attachFlags = 0, .physFlags = PFLAG(FLY))
PARAMS(UmFlyNoSector, Dt3, UM_BASE, .attachFlags = 0, .physFlags = PFLAG(FLY), .sec0Flags = SITH_SECTOR_NOACTORENTER)
PARAMS(UmFlyNoSectorBlock, Dt3, UM_BASE, .attachFlags = 0, .physFlags = PFLAG(FLY), .sec0Flags = SITH_SECTOR_NOACTORENTER, .mode = MODE(MOVING) | MODE(BLOCK))
PARAMS(UmFlyUnderwater, Dt3, UM_BASE, .attachFlags = 0, .physFlags = PFLAG(FLY), .findSectorMode = 1, .sec1Flags = SITH_SECTOR_UNDERWATER)
PARAMS(UmFlySubmerged, Dt3, UM_BASE, .attachFlags = 0, .physFlags = PFLAG(FLY), .thingFlags = TFLAG(SUBMERGED))
PARAMS(UmFlyBreathe, Dt3, UM_BASE, .attachFlags = 0, .physFlags = PFLAG(FLY), .actorFlags = AFLAG(BREATHEUNDERWATER))
PARAMS(UmFlyBreatheSubmerged, Dt3, UM_BASE, .attachFlags = 0, .physFlags = PFLAG(FLY), .actorFlags = AFLAG(BREATHEUNDERWATER), .thingFlags = TFLAG(SUBMERGED), .sec0Flags = SITH_SECTOR_UNDERWATER)
PARAMS(UmSwimAirDestroyed, Dt3, UM_BASE, .attachFlags = 0, .thingFlags = TFLAG(AIRDESTROYED), .sec0Flags = SITH_SECTOR_UNDERWATER, .velocity = { 0.0f, 0.125f, 0.0f })
PARAMS(UmSwimSubmerged, Dt3, UM_BASE, .attachFlags = 0, .thingFlags = TFLAG(SUBMERGED), .sec0Flags = SITH_SECTOR_UNDERWATER, .movePos = { 0.0f, 4.0f, 0.0f })
PARAMS(UmSwimDry, Dt3, UM_BASE, .attachFlags = 0, .thingFlags = TFLAG(SUBMERGED), .movePos = { 0.0f, 4.0f, 0.0f })
PARAMS(UmSwimBreathe, Dt3, UM_BASE, .attachFlags = 0, .thingFlags = TFLAG(SUBMERGED), .actorFlags = AFLAG(BREATHEUNDERWATER), .sec0Flags = SITH_SECTOR_UNDERWATER)
PARAMS(UmNoMoveAtAll, Dt3, UM_BASE, .attachFlags = 0, .movePos = { 0.0f, 4.0f, 0.0f })
PARAMS(UmGoalReached, Dt3, UM_BASE, .movePos = { 0.0f, 0.015625f, 0.0f }, .aCheckPos = { SITHAIUTIL_FLOOR_OK }, .numCheckPos = 1)
PARAMS(UmGoalReachedBlock, Dt1, UM_BASE, .mode = MODE(MOVING) | MODE(BLOCK), .movePos = { 0.0f, 0.015625f, 0.0f })
PARAMS(UmGoalReachedContinuous, Dt1, UM_BASE, .submode = SUB(CONTINUOUSMOTION), .movePos = { 0.0f, 0.015625f, 0.0f })
PARAMS(UmGoalReachedNewMove, Dt1, UM_BASE, .submode = SUB(NEWMOVE), .movePos = { 0.0f, 0.015625f, 0.0f })
PARAMS(UmGoalReachedAnimated, Dt1, UM_BASE, .movePos = { 0.0f, 0.015625f, 0.0f }, .moveStatus = MSTAT(ROLLING_LEFT))
PARAMS(UmGoalReachedStationary, Dt1, UM_BASE, .movePos = { 0.0f, 0.015625f, 0.0f }, .moveStatus = MSTAT(MOUNTING_WALL))
PARAMS(UmGoalReachedWpnts, Dt1, UM_BASE, .mode = MODE(MOVING) | MODE(TRAVERSEWPNTS), .movePos = { 0.0f, 0.015625f, 0.0f }, .advanceRet = 1)
PARAMS(UmGoalReachedWpntsEnd, Dt1, UM_BASE, .mode = MODE(MOVING) | MODE(TRAVERSEWPNTS), .movePos = { 0.0f, 0.015625f, 0.0f }, .advanceRet = 0)
PARAMS(UmGoalReachedSaved, Dt1, UM_BASE, .submode = SUB(UNKNOWN_2), .movePos = { 0.0f, 0.015625f, 0.0f }, .savedMovePos = { 4.0f, 4.0f, 0.0f }, .moveSpeed = 0.5f)
PARAMS(UmGoalReachedSweep, Dt1, UM_BASE, .submode = SUB(UNKNOWN_8), .movePos = { 0.0f, 0.015625f, 0.0f }, .savedMovePos = { 4.0f, 4.0f, 0.0f }, .sweepRet = 1)
PARAMS(UmGoalReachedSweepActive, Dt1, UM_BASE, .mode = MODE(MOVING) | MODE(ACTIVE), .submode = SUB(UNKNOWN_8), .movePos = { 0.0f, 0.015625f, 0.0f }, .savedMovePos = { 4.0f, 4.0f, 0.0f }, .sweepRet = 1)
PARAMS(UmGoalReachedSweepFails, Dt1, UM_BASE, .mode = MODE(MOVING) | MODE(ACTIVE), .submode = SUB(UNKNOWN_8), .movePos = { 0.0f, 0.015625f, 0.0f }, .savedMovePos = { 4.0f, 4.0f, 0.0f }, .sweepRet = 0)
PARAMS(UmGoalReachedSweepWpnts, Dt1, UM_BASE, .mode = MODE(MOVING) | MODE(TRAVERSEWPNTS), .submode = SUB(UNKNOWN_8), .movePos = { 0.0f, 0.015625f, 0.0f }, .sweepRet = 0, .advanceRet = 1)
PARAMS(UmGoalSetEvent, Dt1, UM_BASE, .movePos = { 0.0f, 0.375f, 0.0f }, .velocity = { 0.0f, 0.125f, 0.0f })
PARAMS(UmGoalSetEventEdge, Dt1, UM_BASE, .movePos = { 0.0f, 0.375f, 0.0f }, .movesize = 0.125f)
PARAMS(UmGoalSetEventJustFar, Dt1, UM_BASE, .movePos = { 0.0f, 0.37500003f, 0.0f }, .movesize = 0.125f)
PARAMS(UmGoalReachedEdge, Dt1, UM_BASE, .movePos = { 0.0f, 0.03f, 0.0f })
PARAMS(UmGoalReachedJustFar, Dt1, UM_BASE, .movePos = { 0.0f, 0.030001f, 0.0f })
PARAMS(UmUnderwaterTolerance, Dt3, UM_BASE, .sec0Flags = SITH_SECTOR_UNDERWATER, .movePos = { 0.0f, 0.0625f, 0.0f })
PARAMS(UmWallCrawl, Dt3, UM_BASE, .mode = MODE(MOVING) | MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.0f }, .movePos = { 0.0f, 4.0f, 0.0f })
PARAMS(UmWallCrawlClose, Dt3, UM_BASE, .mode = MODE(MOVING) | MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.0f }, .movePos = { 0.0f, 0.015625f, 0.0f })

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_CheckMoveAlign
// ---------------------------------------------------------------------------------------------------------------
PARAMS(CmaPlain, Dt1, .submode = SUB(ALIGNING), .goalLVec = { 0.0f, -1.0f, 0.0f })
PARAMS(CmaNoBackFacing, Dt1, .submode = SUB(ALIGNING) | SUB(NOMOVEBACKWARDS), .goalLVec = { 0.0f, 1.0f, 0.0f })
PARAMS(CmaNoBackBehind, Dt1, .submode = SUB(ALIGNING) | SUB(NOMOVEBACKWARDS), .goalLVec = { 0.0f, -1.0f, 0.0f })
PARAMS(CmaNoBackSide, Dt1, .submode = SUB(ALIGNING) | SUB(NOMOVEBACKWARDS), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(CmaNoBackSlightlyBehind, Dt1, .submode = SUB(ALIGNING) | SUB(NOMOVEBACKWARDS), .goalLVec = { 1.0f, -0.0625f, 0.0f })
PARAMS(CmaNoAligning, Dt1, .submode = SUB(NOMOVEBACKWARDS), .goalLVec = { 0.0f, 1.0f, 0.0f })
PARAMS(CmaOtherBits, Dt1, .submode = SUB(ALIGNING) | SUB(MOVEFORWARD) | SUB(HEADTRACKINGENABLED), .goalLVec = { 0.0f, 1.0f, 0.0f })

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_GetGoalTolerance: pos = v, movePos = w
// ---------------------------------------------------------------------------------------------------------------
CALLS(GtA, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 1.5f, 3.0f, 4.0f } })
CALLS(GtSame, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 1.0f, 2.0f, 3.0f } })
CALLS(GtNear, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 1.015625f, 2.0f, 3.0f } })
CALLS(GtOnX, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 1.0f, 3.0f, 5.0f } })
CALLS(GtWall, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 1.0f, 2.0f, 6.0f } })
CALLS(GtWall2, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 1.0f, 2.0f, 3.5f } })
CALLS(GtWallFar, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 5.0f, 7.0f, 6.0f } })
CALLS(GtWallAxis, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 1.0f, 2.0f, 3.015625f } })
CALLS(GtSide, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 0.03125f, 4.0f, 1.0f } })
CALLS(GtSideLeft, { .v = { 0.0f, 0.0f, 0.0f }, .w = { -0.03125f, 4.0f, 1.0f } })
CALLS(GtSideFar, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 0.25f, 4.0f, 1.0f } })
CALLS(GtSideEdge, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 0.1f, 0.0f, 1.0f } })
CALLS(GtSideEdge2, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 0.0f, 0.25f, 1.0f } })

PARAMS(GtPlain, GtA, 0)
PARAMS(GtPlainSame, GtSame, 0)
PARAMS(GtPlainBigMove, GtA, .movesize = 0.5f)
PARAMS(GtPlainSmallMove, GtA, .movesize = 0.015625f)
PARAMS(GtPlainZPos, GtA, .pos = { 0.0f, 0.0f, 9.0f })
PARAMS(GtUnderwater, GtA, .sec0Flags = SITH_SECTOR_UNDERWATER)
PARAMS(GtUnderwaterBig, GtA, .sec0Flags = SITH_SECTOR_UNDERWATER, .movesize = 0.5f)
PARAMS(GtUnderwaterSmall, GtA, .sec0Flags = SITH_SECTOR_UNDERWATER, .movesize = 0.0625f)
PARAMS(GtUnderwaterTiny, GtA, .sec0Flags = SITH_SECTOR_UNDERWATER, .movesize = 0.03125f)
PARAMS(GtFlyTiny, GtA, .physFlags = PFLAG(FLY), .movesize = 0.03125f)
PARAMS(GtFly, GtA, .physFlags = PFLAG(FLY))
PARAMS(GtFlyBig, GtA, .physFlags = PFLAG(FLY), .movesize = 0.5f)
PARAMS(GtSectorOnly, GtA, .sector = 1, .sec0Flags = SITH_SECTOR_UNDERWATER)
PARAMS(GtSectorUnderwaterOwner, GtA, .sector = 1, .sec1Flags = SITH_SECTOR_UNDERWATER)
PARAMS(GtMoveForward, GtSide, .submode = SUB(MOVEFORWARD))
PARAMS(GtMoveForwardLeft, GtSideLeft, .submode = SUB(MOVEFORWARD))
PARAMS(GtMoveForwardFar, GtSideFar, .submode = SUB(MOVEFORWARD))
PARAMS(GtMoveForwardEdge, GtSideEdge, .submode = SUB(MOVEFORWARD))
PARAMS(GtMoveForwardEdge2, GtSideEdge2, .submode = SUB(MOVEFORWARD))
PARAMS(GtMoveForwardTurned, GtSide, .submode = SUB(MOVEFORWARD), .hx = 1, .hy = 1, .hz = 0)
PARAMS(GtWallMounting, GtWall, .mode = MODE(WALLCRAWLING), .moveStatus = MSTAT(MOUNTING_WALL))
PARAMS(GtWallNoSurface, GtWall, .mode = MODE(WALLCRAWLING))
PARAMS(GtWallAligned2, GtWall2, .mode = MODE(WALLCRAWLING))
PARAMS(GtWallAligned1, GtOnX, .mode = MODE(WALLCRAWLING))
PARAMS(GtWallFarUnreachable, GtWallFar, .mode = MODE(WALLCRAWLING))
PARAMS(GtWallFarUnreachableBlock, GtWallFar, .mode = MODE(WALLCRAWLING) | MODE(BLOCK))
PARAMS(GtWallAxisClose, GtWallAxis, .mode = MODE(WALLCRAWLING))
PARAMS(GtWallAxisFarSurface, GtWallAxis, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.0f })
CALLS(GtDist5b, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 0.0f, 3.0f, 4.0f } })
CALLS(GtWallDiag, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 1.0f, 5.0f, 7.0f } })
PARAMS(GtWallSurfParallel, GtWallDiag, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.6f, 0.8f })
PARAMS(GtWallSurfParallelBlock, GtWallDiag, .mode = MODE(WALLCRAWLING) | MODE(BLOCK), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.6f, 0.8f })
PARAMS(GtWallSurfDot9965, GtWallDiag, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0835f, 0.59791f, 0.79721f })
PARAMS(GtWallSurfDot9955, GtWallDiag, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0941f, 0.59734f, 0.79646f })
PARAMS(GtWallSurfDotNeg, GtWallDiag, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, -0.6f, -0.8f })
PARAMS(GtWallSurfNear, GtWallDiag, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.6f, 0.8f }, .movesize = 3.0f)
CALLS(GtWallDiagY, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 4.0f, 2.0f, 7.0f } })
CALLS(GtWallDiagZ, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 4.0f, 6.0f, 3.0f } })
CALLS(GtWallDiagNone, { .v = { 1.0f, 2.0f, 3.0f }, .w = { 4.0f, 6.0f, 15.0f } })
PARAMS(GtWallSurfParallelY, GtWallDiagY, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.6f, 0.0f, 0.8f })
PARAMS(GtWallSurfParallelZ, GtWallDiagZ, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.6f, 0.8f, 0.0f })
PARAMS(GtWallSurfParallelNone, GtWallDiagNone, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.230769f, 0.307692f, 0.923077f })
PARAMS(GtWallNoSurfY, GtWallDiagY, .mode = MODE(WALLCRAWLING))
PARAMS(GtWallNoSurfZ, GtWallDiagZ, .mode = MODE(WALLCRAWLING))
PARAMS(GtWallNoSurfNone, GtWallDiagNone, .mode = MODE(WALLCRAWLING))
CALLS(GtEqXY, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 0.03f, 0.03f, 4.0f } })
CALLS(GtEqXZ, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 0.03f, 4.0f, 0.03f } })
CALLS(GtEqYZ, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 4.0f, 0.03f, 0.03f } })
CALLS(GtEqX, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 0.03f, 3.0f, 4.0f } })
CALLS(GtEqRight, { .v = { 0.0f, 0.0f, 0.0f }, .w = { -0.1f, 0.0f, 1.0f } })
CALLS(GtDist5, { .v = { 0.0f, 0.0f, 0.0f }, .w = { 0.0f, 3.0f, 4.0f } })
PARAMS(GtWallEqXY, GtEqXY, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.0f })
PARAMS(GtWallEqXZ, GtEqXZ, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 1.0f, 0.0f })
PARAMS(GtWallEqYZ, GtEqYZ, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 1.0f, 0.0f, 0.0f })
PARAMS(GtWallEqX, GtEqX, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.6f, 0.8f })
PARAMS(GtWallDist5Equal, GtDist5, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.6f, 0.8f }, .movesize = 2.5f)
PARAMS(GtWallDist5Below, GtDist5, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.6f, 0.8f }, .movesize = 2.4999998f)
PARAMS(GtMoveForwardEdgeLeft, GtEqRight, .submode = SUB(MOVEFORWARD))
PARAMS(GtWallDotEqual, GtDist5b, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.245f })
PARAMS(GtWallDotBelow, GtDist5b, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.2449999f })
PARAMS(GtWallSurfaceAlong, GtWall, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.0f })
PARAMS(GtWallSurfacePerp, GtWall, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 1.0f, 0.0f, 0.0f })
PARAMS(GtWallSurfaceNear996, GtWall, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0625f, 0.0f, 0.99804f })
PARAMS(GtWallSurfaceFar996, GtWall, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.125f, 0.0f, 0.99216f })
PARAMS(GtWallSurfaceFlat, GtA, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 1.0f, 0.0f })
PARAMS(GtWallSurfaceSame, GtSame, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 1.0f, 0.0f })
PARAMS(GtWallSurfaceNegNormal, GtWall, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, -1.0f })
PARAMS(GtWallSurfaceBigMove, GtWallFar, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.0f }, .movesize = 4.0f)
PARAMS(GtWallBigMoveNoSurface, GtWallFar, .mode = MODE(WALLCRAWLING), .movesize = 4.0f)
PARAMS(GtWallSmallMove, GtWall2, .mode = MODE(WALLCRAWLING), .movesize = 0.015625f, .attachSurface = 1, .surfaceNormal = { 0.0f, 1.0f, 0.0f })

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_CheckWalkMove / CheckFlyMove / CheckSwimMove
// ---------------------------------------------------------------------------------------------------------------
CALLS(Cw1, { .f = 0.0625f })
CALLS(Cw2, { .f = 0.0625f }, { .f = 0.0625f })
CALLS(Cw4, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.0625f })

PARAMS(CwOkEven, Cw4, .velocity = { 0.0f, 0.5f, 0.0f }, .frameNumber = 1)
PARAMS(CwOkOdd, Cw4, .velocity = { 0.0f, 0.5f, 0.0f }, .frameNumber = 0)
PARAMS(CwWallCrawlOdd, Cw4, .velocity = { 0.0f, 0.5f, 0.0f }, .frameNumber = 0, .mode = MODE(WALLCRAWLING), .movesize = 0.25f)
PARAMS(CwWallCrawlSmallMove, Cw4, .velocity = { 0.0f, 0.5f, 0.0f }, .frameNumber = 0, .mode = MODE(WALLCRAWLING), .movesize = 0.015625f)
PARAMS(CwWallCrawlMove003, Cw4, .velocity = { 0.0f, 0.5f, 0.0f }, .frameNumber = 0, .mode = MODE(WALLCRAWLING), .movesize = 0.03f)
PARAMS(CwWallCrawlMove003b, Cw4, .velocity = { 0.0f, 0.5f, 0.0f }, .frameNumber = 0, .mode = MODE(WALLCRAWLING), .movesize = 0.030001f)
PARAMS(CwWallCrawlEven, Cw4, .velocity = { 0.0f, 0.5f, 0.0f }, .frameNumber = 1, .mode = MODE(WALLCRAWLING))
PARAMS(CwOddThingIndex, Cw2, .velocity = { 0.0f, 0.5f, 0.0f }, .frameNumber = 0)
PARAMS(CwBlocked, Cw2, .aCheckPos = { SITHAIUTIL_FLOOR_BLOCKED }, .numCheckPos = 1)
PARAMS(CwNone, Cw2, .aCheckPos = { SITHAIUTIL_FLOOR_NONE }, .numCheckPos = 1)
PARAMS(CwWall, Cw2, .aCheckPos = { SITHAIUTIL_FLOOR_WALL }, .numCheckPos = 1)
PARAMS(CwWallCrawler, Cw2, .aCheckPos = { SITHAIUTIL_FLOOR_WALL }, .numCheckPos = 1, .hasInstinct = 1)
PARAMS(CwThing, Cw2, .aCheckPos = { SITHAIUTIL_FLOOR_THING }, .numCheckPos = 1)
PARAMS(CwThingStep, Cw2, .aCheckPos = { SITHAIUTIL_FLOOR_THING }, .numCheckPos = 1, .submode = SUB(ALLOWSTEPTHING))
PARAMS(CwUnknown3, Cw2, .aCheckPos = { 3 }, .numCheckPos = 1)
PARAMS(CwUnknown16, Cw2, .aCheckPos = { 16 }, .numCheckPos = 1)
PARAMS(CwUnknownBlock, Cw2, .aCheckPos = { 3 }, .numCheckPos = 1, .mode = MODE(BLOCK))
PARAMS(CwSequence, Cw4, .aCheckPos = { SITHAIUTIL_FLOOR_OK, SITHAIUTIL_FLOOR_NONE, SITHAIUTIL_FLOOR_WALL, SITHAIUTIL_FLOOR_THING }, .numCheckPos = 4)

PARAMS(CfSubmerged, Cw2, .thingFlags = TFLAG(SUBMERGED), .physFlags = PFLAG(FLY), .velocity = { 0.0f, 0.25f, 0.125f }, .mode = MODE(MOVING))
PARAMS(CfSubmergedContinuous, Cw1, .thingFlags = TFLAG(SUBMERGED), .physFlags = PFLAG(FLY), .submode = SUB(CONTINUOUSMOTION), .mode = MODE(MOVING))
PARAMS(CfClear, Cw2, .physFlags = PFLAG(FLY), .velocity = { 0.0f, 0.25f, 0.0f })
PARAMS(CfNoSector, Cw2, .physFlags = PFLAG(FLY), .velocity = { 0.0f, 0.25f, 0.0f }, .sec0Flags = 0, .noSector = 1)
PARAMS(CfNoActorEnter, Cw2, .physFlags = PFLAG(FLY), .findSectorMode = 1, .sec1Flags = SITH_SECTOR_NOACTORENTER)
PARAMS(CfNoActorEnterBlock, Cw2, .physFlags = PFLAG(FLY), .findSectorMode = 1, .sec1Flags = SITH_SECTOR_NOACTORENTER, .mode = MODE(BLOCK))
PARAMS(CfUnderwater, Cw2, .physFlags = PFLAG(FLY), .findSectorMode = 1, .sec1Flags = SITH_SECTOR_UNDERWATER, .velocity = { 0.0f, 0.25f, 0.0625f }, .mode = MODE(MOVING))
PARAMS(CfUnderwaterBoth, Cw1, .physFlags = PFLAG(FLY), .findSectorMode = 1, .sec1Flags = SITH_SECTOR_UNDERWATER | SITH_SECTOR_NOACTORENTER)
PARAMS(CfSameSectorUnderwater, Cw1, .physFlags = PFLAG(FLY), .sec0Flags = SITH_SECTOR_UNDERWATER)

PARAMS(CsClear, Cw2, .sec0Flags = SITH_SECTOR_UNDERWATER, .velocity = { 0.0f, 0.25f, 0.0f })
PARAMS(CsDry, Cw2, .velocity = { 0.0f, 0.25f, 0.0f })
PARAMS(CsNoSector, Cw1, .noSector = 1, .velocity = { 0.0f, 0.25f, 0.0f })
PARAMS(CsNoActorEnter, Cw1, .sec0Flags = SITH_SECTOR_UNDERWATER | SITH_SECTOR_NOACTORENTER)
PARAMS(CsNoActorEnterBlock, Cw1, .sec0Flags = SITH_SECTOR_UNDERWATER | SITH_SECTOR_NOACTORENTER, .mode = MODE(BLOCK))
PARAMS(CsRisingSurfaceWet, Cw2, .sec0Flags = SITH_SECTOR_UNDERWATER, .velocity = { 0.0f, 0.25f, 0.125f }, .movePos = { 0.0f, 4.0f, 1.0f })
PARAMS(CsRisingSurfaceDry, Cw2, .findSectorMode = 1, .sec0Flags = SITH_SECTOR_UNDERWATER, .sec1Flags = 0, .velocity = { 0.0f, 0.25f, 0.125f }, .movePos = { 0.0f, 4.0f, 1.0f }, .pos = { 0.0f, 0.0f, 0.5f })
PARAMS(CsRisingSurfaceDryLowTarget, Cw2, .findSectorMode = 1, .sec0Flags = SITH_SECTOR_UNDERWATER, .sec1Flags = 0, .velocity = { 0.0f, 0.25f, 0.125f }, .movePos = { 0.0f, 4.0f, -1.0f }, .pos = { 0.0f, 0.0f, 0.5f })
PARAMS(CsRisingSurfaceDryNear, Cw2, .findSectorMode = 1, .sec0Flags = SITH_SECTOR_UNDERWATER, .sec1Flags = 0, .submode = SUB(SWIMNEARSURFACE), .velocity = { 0.0f, 0.25f, 0.125f }, .movePos = { 0.0f, 4.0f, 1.0f }, .pos = { 0.0f, 0.0f, 0.5f })
PARAMS(CsRisingSurfaceWetNext, Cw2, .findSectorMode = 1, .sec0Flags = SITH_SECTOR_UNDERWATER, .sec1Flags = SITH_SECTOR_UNDERWATER, .velocity = { 0.0f, 0.25f, 0.125f }, .movePos = { 0.0f, 4.0f, 1.0f })
PARAMS(CsRisingSameHeight, Cw2, .findSectorMode = 1, .sec0Flags = SITH_SECTOR_UNDERWATER, .sec1Flags = 0, .velocity = { 0.0f, 0.25f, 0.125f }, .movePos = { 0.0f, 4.0f, 0.5f }, .pos = { 0.0f, 0.0f, 0.5f })
PARAMS(CsSinking, Cw2, .findSectorMode = 1, .sec0Flags = SITH_SECTOR_UNDERWATER, .sec1Flags = 0, .velocity = { 0.0f, 0.25f, -0.125f }, .movePos = { 0.0f, 4.0f, 1.0f })
PARAMS(CsNoUpdraft, Cw2, .findSectorMode = 1, .sec0Flags = SITH_SECTOR_UNDERWATER, .sec1Flags = SITH_SECTOR_UNDERWATER, .velocity = { 0.0f, 0.25f, 0.0f }, .movePos = { 0.0f, 4.0f, 1.0f })
PARAMS(CsNoSectorRising, Cw1, .noSector = 1, .velocity = { 0.0f, 0.25f, 0.125f })

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_AIGetMoveState (every move status), sithAIMove_AISpecialTurn
// ---------------------------------------------------------------------------------------------------------------
CALLS(MsAll, { .i = 0, .j = 127 })
CALLS(MsOutOfRange, { .i = 128, .j = 140 }, { .i = -3, .j = 0 })
PARAMS(MsPlain, MsAll, 0)
PARAMS(MsFlags, MsAll, .mode = MODE(TURNING) | MODE(MOVING), .actorFlags = AFLAG(IMMOBILE), .physFlags = PFLAG(FLY))
PARAMS(MsOutOfRange, MsOutOfRange, 0)

CALLS(StAngles, { .f = 0.0f }, { .f = 22.4f }, { .f = 22.5f }, { .f = -22.5f }, { .f = 44.0f }, { .f = -44.0f }, { .f = 67.4f }, { .f = 67.5f }, { .f = -67.5f }, { .f = 90.0f }, { .f = -90.0f }, { .f = 112.4f }, { .f = 112.5f }, { .f = -112.5f }, { .f = 135.0f }, { .f = -135.0f }, { .f = 157.5f }, { .f = 157.6f }, { .f = -157.6f }, { .f = 180.0f }, { .f = -180.0f }, { .f = 181.0f })
PARAMS(StNoSpecial, StAngles, .numKf = 7, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT), PSUB(WHIPCLIMBDISMOUNT), PSUB(WHIPSWINGMOUNT), PSUB(WHIPCLIMBMOUNT) })
PARAMS(StAllKf, StAngles, .submode = SUB(SPECIALTURNS), .numKf = 7, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT), PSUB(WHIPCLIMBDISMOUNT), PSUB(WHIPSWINGMOUNT), PSUB(WHIPCLIMBMOUNT) })
PARAMS(StNoKf, StAngles, .submode = SUB(SPECIALTURNS))
PARAMS(St180Only, StAngles, .submode = SUB(SPECIALTURNS), .numKf = 1, .aKf = { PSUB(WHIPCLIMBMOUNT) })
PARAMS(St135Only, StAngles, .submode = SUB(SPECIALTURNS), .numKf = 2, .aKf = { PSUB(WHIPCLIMBDISMOUNT), PSUB(WHIPSWINGMOUNT) })
PARAMS(St135And180, StAngles, .submode = SUB(SPECIALTURNS), .numKf = 3, .aKf = { PSUB(WHIPCLIMBDISMOUNT), PSUB(WHIPSWINGMOUNT), PSUB(WHIPCLIMBMOUNT) })
PARAMS(St90Only, StAngles, .submode = SUB(SPECIALTURNS), .numKf = 2, .aKf = { PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT) })
PARAMS(St45Only, StAngles, .submode = SUB(SPECIALTURNS), .numKf = 2, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN) })
PARAMS(St45OneSide, StAngles, .submode = SUB(SPECIALTURNS), .numKf = 1, .aKf = { PSUB(WHIPCLIMBUP) })
PARAMS(StBusy, StAngles, .submode = SUB(SPECIALTURNS), .moveStatus = MSTAT(STRAFING_LEFT), .numKf = 7, .aKf = { PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT), PSUB(WHIPCLIMBDISMOUNT), PSUB(WHIPSWINGMOUNT), PSUB(WHIPCLIMBMOUNT) })
PARAMS(StBusyStationary, StAngles, .submode = SUB(SPECIALTURNS), .moveStatus = MSTAT(TURNING_180_DEGREES))

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_UpdateAIMove: the movement mode follows the velocity in the owner's frame
// ---------------------------------------------------------------------------------------------------------------
CALLS(UaOnce, { .f = 0.0f })
CALLS(UaTwice, { .f = 0.0f }, { .f = 0.0f })
CALLS(UaChange, { .f = 0.0f }, { .newPuppetSubmode = PSUB(WALK) }, { .newPuppetSubmode = PSUB(STAND) })

#define UA_KF .numKf = 8, .aKf = { PSUB(WALK), PSUB(RUN), PSUB(STAND), PSUB(STAND2WALK), PSUB(WALK2STAND), PSUB(TURNLEFT), PSUB(TURNRIGHT), PSUB(LAND) }
PARAMS(UaStill, UaOnce, UA_KF, .puppetSubmode = PSUB(WALK))
PARAMS(UaStillStanding, UaOnce, UA_KF, .puppetSubmode = PSUB(STAND))
PARAMS(UaWalkFwd, UaOnce, UA_KF, .velocity = { 0.0f, 0.125f, 0.0f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaWalkFwdAlready, UaOnce, UA_KF, .velocity = { 0.0f, 0.125f, 0.0f }, .puppetSubmode = PSUB(WALK))
PARAMS(UaRunFwd, UaOnce, UA_KF, .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK))
PARAMS(UaWalkBack, UaOnce, UA_KF, .velocity = { 0.0f, -0.125f, 0.0f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaStrafe, UaOnce, UA_KF, .velocity = { 0.5f, 0.0f, 0.0f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaStrafeSlow, UaOnce, UA_KF, .velocity = { 0.0625f, 0.0f, 0.0f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaFall, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, -0.5f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaFallAttached, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, -0.5f }, .puppetSubmode = PSUB(STAND), .attachFlags = SITH_ATTACH_THING)
PARAMS(UaLand, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, 0.0f }, .puppetSubmode = PSUB(FALL), .playModeRet = 0)
PARAMS(UaLandNoKf, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, 0.0f }, .puppetSubmode = PSUB(FALL), .playModeRet = -1)
PARAMS(UaFlyUp, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, 0.5f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(STAND))
PARAMS(UaFlyDown, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, -0.5f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(STAND))
PARAMS(UaFlySlowUp, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, 0.00999f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(WALK))
PARAMS(UaFlyThreshold, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, 0.0099999998f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(WALK))
PARAMS(UaFlyAbove, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, 0.0101f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(WALK))
PARAMS(UaFlyForward, UaOnce, UA_KF, .velocity = { 0.0f, 0.5f, 0.125f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(STAND))
PARAMS(UaFlyNegThreshold, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, -0.0101f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(WALK))
PARAMS(UaTurning, UaOnce, UA_KF, .mode = MODE(TURNING), .moveStatus = MSTAT(TURNING_LEFT), .puppetSubmode = PSUB(STAND))
PARAMS(UaTurningMoving, UaOnce, UA_KF, .mode = MODE(TURNING), .velocity = { 0.0f, 0.003f, 0.0f }, .moveStatus = MSTAT(TURNING_RIGHT), .puppetSubmode = PSUB(STAND))
PARAMS(UaImmobile, UaOnce, UA_KF, .actorFlags = AFLAG(IMMOBILE), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK))
PARAMS(UaNoSector, UaOnce, UA_KF, .noSector = 1, .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK))
PARAMS(UaStateAnimated, UaOnce, UA_KF, .velocity = { 0.0f, 0.5f, 0.0f }, .moveStatus = MSTAT(ROLLING_LEFT), .puppetSubmode = PSUB(WALK))
PARAMS(UaStateStationary, UaOnce, UA_KF, .velocity = { 0.0f, 0.5f, 0.0f }, .moveStatus = MSTAT(TURNING_180_DEGREES), .puppetSubmode = PSUB(WALK))
PARAMS(UaControlsDisabled, UaOnce, UA_KF, .controlsDisabled = 1, .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK))
PARAMS(UaRotatedOwner, UaOnce, UA_KF, .hx = 1, .hy = 0, .hz = 0, .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaRotatedOwnerForward, UaOnce, UA_KF, .hx = 1, .hy = 0, .hz = 0, .velocity = { 0.5f, 0.0f, 0.0f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaDiagonal, UaOnce, UA_KF, .velocity = { 0.25f, 0.25f, 0.0f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaDiagonalZ, UaOnce, UA_KF, .velocity = { 0.25f, 0.25f, 0.25f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaForceMovePlay, UaOnce, UA_KF, .forceMovePlay = 1, .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK))
PARAMS(UaTieZY, UaOnce, UA_KF, .velocity = { 0.0f, 0.25f, 0.25f }, .puppetSubmode = PSUB(STAND), .attachFlags = SITH_ATTACH_THING)
PARAMS(UaTieZX, UaOnce, UA_KF, .velocity = { 0.25f, 0.0f, 0.25f }, .puppetSubmode = PSUB(STAND), .attachFlags = SITH_ATTACH_THING)
PARAMS(UaTieZYFly, UaOnce, UA_KF, .velocity = { 0.0f, 0.25f, 0.25f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(STAND))
PARAMS(UaTieZXFly, UaOnce, UA_KF, .velocity = { 0.25f, 0.0f, 0.25f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(STAND))
PARAMS(UaTieAllFly, UaOnce, UA_KF, .velocity = { 0.25f, 0.25f, 0.25f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(STAND))
PARAMS(UaFlyDownExact, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, -0.0099999998f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(WALK))
PARAMS(UaFlyDownBelow, UaOnce, UA_KF, .velocity = { 0.0f, 0.0f, -0.0100000007f }, .physFlags = PFLAG(FLY), .puppetSubmode = PSUB(WALK))
PARAMS(UaTieYXNeg, UaOnce, UA_KF, .velocity = { -0.25f, -0.25f, 0.0f }, .puppetSubmode = PSUB(WALK), .attachFlags = SITH_ATTACH_THING)
PARAMS(UaTieZYNeg, UaOnce, UA_KF, .velocity = { 0.0f, -0.25f, -0.25f }, .puppetSubmode = PSUB(WALK))
PARAMS(UaTieZXNeg, UaOnce, UA_KF, .velocity = { -0.25f, 0.0f, -0.25f }, .puppetSubmode = PSUB(WALK))
PARAMS(UaTieZXNegAttached, UaOnce, UA_KF, .velocity = { -0.25f, 0.0f, -0.25f }, .puppetSubmode = PSUB(WALK), .attachFlags = SITH_ATTACH_THING)
PARAMS(UaNoPuppetKf, UaOnce, .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(STAND))
PARAMS(UaNoPuppetKfWalking, UaOnce, .velocity = { 0.0f, 0.125f, 0.0f }, .puppetSubmode = PSUB(WALK))
PARAMS(UaChanging, UaChange, UA_KF, .velocity = { 0.0f, 0.125f, 0.0f }, .puppetSubmode = PSUB(STAND))

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_GetAIMoveModes: one sweep over speeds and axes per call
// ---------------------------------------------------------------------------------------------------------------
CALLS(GmSweep, { .f = 0.0f })
CALLS(GmSweepStatus, { .newMoveStatus = MSTAT(TURNING_LEFT) }, { .newMoveStatus = MSTAT(TURNING_RIGHT) }, { .newMoveStatus = MSTAT(UNKNOWN_105) }, { .newMoveStatus = MSTAT(UNKNOWN_106) }, { .newMoveStatus = MSTAT(WALKING) })
PARAMS(GmPlain, GmSweep, 0)
PARAMS(GmAttached, GmSweep, .attachFlags = SITH_ATTACH_THING)
PARAMS(GmFast10, GmSweep, .actorFlags = AFLAG(FASTMOVE10))
PARAMS(GmFast15, GmSweep, .actorFlags = AFLAG(FASTMOVE15))
PARAMS(GmFast15And10, GmSweep, .actorFlags = AFLAG(FASTMOVE15) | AFLAG(FASTMOVE10))
PARAMS(GmFast15Attached, GmSweep, .actorFlags = AFLAG(FASTMOVE15), .attachFlags = SITH_ATTACH_SURFACE)
PARAMS(GmTurning, GmSweepStatus, .mode = MODE(TURNING))
PARAMS(GmTurningAttached, GmSweepStatus, .mode = MODE(TURNING), .attachFlags = SITH_ATTACH_THING)
PARAMS(GmTurningFast10, GmSweepStatus, .mode = MODE(TURNING), .actorFlags = AFLAG(FASTMOVE10))
PARAMS(GmTurningFast15, GmSweepStatus, .mode = MODE(TURNING), .actorFlags = AFLAG(FASTMOVE15))
PARAMS(GmTurningFlyer, GmSweepStatus, .mode = MODE(TURNING), .actorFlags = AFLAG(FLYERMOVE))
PARAMS(GmTurningFlyerFast, GmSweepStatus, .mode = MODE(TURNING), .actorFlags = AFLAG(FLYERMOVE) | AFLAG(FASTMOVE15))
PARAMS(GmFlyerNotTurning, GmSweepStatus, .actorFlags = AFLAG(FLYERMOVE))
PARAMS(GmNoLocalTurning, GmSweep, .mode = MODE(TURNING), .actorFlags = AFLAG(FLYERMOVE))

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_SetSubMode: every new submode for a starting submode, with and without keyframes/tracks
// ---------------------------------------------------------------------------------------------------------------
#define SS_KF_ALL .numKf = 11, .aKf = { PSUB(STAND), PSUB(WALK), PSUB(RUN), PSUB(WALKBACK), PSUB(TURNLEFT), PSUB(TURNRIGHT), PSUB(STAND2WALK), PSUB(WALK2STAND), PSUB(LAND), PSUB(FALL), PSUB(STRAFERIGHT) }
#define SS_KF_NOTRANS .numKf = 8, .aKf = { PSUB(STAND), PSUB(WALK), PSUB(RUN), PSUB(WALKBACK), PSUB(TURNLEFT), PSUB(TURNRIGHT), PSUB(LAND), PSUB(FALL) }

CALLS(SsStand, { .i = MSTAT(STILL), .newPuppetSubmode = PSUB(STAND) })
CALLS(SsWalk, { .i = MSTAT(WALKING), .newPuppetSubmode = PSUB(WALK) })
CALLS(SsRun, { .i = MSTAT(RUNNING), .newPuppetSubmode = PSUB(RUN) })
CALLS(SsTurnL, { .i = MSTAT(TURNING_LEFT), .newPuppetSubmode = PSUB(TURNLEFT) })
CALLS(SsTurnR, { .i = MSTAT(TURNING_RIGHT), .newPuppetSubmode = PSUB(TURNRIGHT) })
CALLS(SsFall, { .i = MSTAT(FALLING), .newPuppetSubmode = PSUB(FALL) })
CALLS(SsOther, { .i = MSTAT(STILL), .newPuppetSubmode = PSUB(HOPLEFT) })
CALLS(SsWalkBack, { .i = MSTAT(WALKING), .newPuppetSubmode = PSUB(WALKBACK) })
CALLS(SsStrafeR, { .i = MSTAT(STILL), .newPuppetSubmode = PSUB(STRAFERIGHT) })
CALLS(SsStrafeL, { .i = MSTAT(STILL), .newPuppetSubmode = PSUB(STRAFELEFT) })
CALLS(SsWalkTwice, { .i = MSTAT(WALKING), .newPuppetSubmode = PSUB(STAND) }, { .i = MSTAT(WALKING), .newPuppetSubmode = PSUB(WALK) })

PARAMS(SsFromStand, SsStand, SS_KF_ALL)
PARAMS(SsFromStandNoTrans, SsStand, SS_KF_NOTRANS)
PARAMS(SsFromStandNoKf, SsStand, 0)
PARAMS(SsFromStandTrack, SsStand, SS_KF_ALL, .findTrack = 1, .trackSubmode = PSUB(WALK))
PARAMS(SsFromWalk, SsWalk, SS_KF_ALL)
PARAMS(SsFromWalkNoTrans, SsWalk, SS_KF_NOTRANS)
PARAMS(SsFromWalkTrack, SsWalk, SS_KF_ALL, .findTrack = 1, .trackSubmode = PSUB(RUN))
PARAMS(SsFromRun, SsRun, SS_KF_ALL)
PARAMS(SsFromRunNoTrans, SsRun, SS_KF_NOTRANS)
PARAMS(SsFromTurnL, SsTurnL, SS_KF_ALL)
PARAMS(SsFromTurnR, SsTurnR, SS_KF_ALL, .findTrack = 1, .trackSubmode = PSUB(TURNLEFT))
PARAMS(SsFromFall, SsFall, SS_KF_ALL, .playModeRet = 0)
PARAMS(SsFromFallNoLand, SsFall, SS_KF_ALL, .playModeRet = -1)
PARAMS(SsFromFallPlay1, SsFall, SS_KF_ALL, .playModeRet = 1)
PARAMS(SsFromFallTrack, SsFall, SS_KF_ALL, .playModeRet = 0, .findTrack = 1, .trackSubmode = PSUB(STAND))
PARAMS(SsFromOther, SsOther, SS_KF_ALL)
PARAMS(SsFromWalkBack, SsWalkBack, SS_KF_ALL)
PARAMS(SsFromStrafeR, SsStrafeR, SS_KF_ALL)
PARAMS(SsFromStrafeL, SsStrafeL, SS_KF_ALL)
PARAMS(SsMajorModeOther, SsStand, SS_KF_ALL, .majorMode = 5)
PARAMS(SsMajorModeLast, SsWalk, SS_KF_ALL, .majorMode = 23)
PARAMS(SsNoPuppetClass, SsStand, SS_KF_ALL, .noPuppetClass = 1)
PARAMS(SsNoPuppetState, SsStand, SS_KF_ALL, .noPuppetState = 1)
PARAMS(SsFromWalkTwice, SsWalkTwice, SS_KF_ALL)

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_PuppetCallback
// ---------------------------------------------------------------------------------------------------------------
#define PC_FINISH(status) { .newMoveStatus = (status), .i = 3, .j = 0 }
CALLS(PcFinish,
    PC_FINISH(MSTAT(LAND)), PC_FINISH(MSTAT(MOUNTING_WALL)), PC_FINISH(MSTAT(ROLLING_LEFT)), PC_FINISH(MSTAT(ROLLING_RIGHT)),
    PC_FINISH(MSTAT(STRAFING_LEFT)), PC_FINISH(MSTAT(STRAFING_RIGHT)), PC_FINISH(MSTAT(WALK2STAND)), PC_FINISH(MSTAT(UNKNOWN_78)),
    PC_FINISH(MSTAT(STAND2WALK)), PC_FINISH(MSTAT(STAND2RUN)), PC_FINISH(MSTAT(KNOCKEDOUT)))
CALLS(PcFinishTurns,
    PC_FINISH(MSTAT(TURNING_LEFT_45_DEGREES)), PC_FINISH(MSTAT(TURNING_RIGHT_45_DEGREES)), PC_FINISH(MSTAT(TURNING_LEFT_90_DEGREES)),
    PC_FINISH(MSTAT(TURNING_RIGHT_90_DEGREES)), PC_FINISH(MSTAT(TURNING_LEFT_135_DEGREES)), PC_FINISH(MSTAT(TURNING_RIGHT_135_DEGREES)),
    PC_FINISH(MSTAT(TURNING_180_DEGREES)))
CALLS(PcFinishOther, PC_FINISH(MSTAT(STILL)), PC_FINISH(MSTAT(WALKING)), PC_FINISH(MSTAT(RUNNING)), PC_FINISH(MSTAT(FALLING)), PC_FINISH(MSTAT(TURNING_LEFT)), PC_FINISH(MSTAT(PUSHING)))
CALLS(PcFinishTrack, { .newMoveStatus = MSTAT(LAND), .i = 5, .j = 0 }, { .newMoveStatus = MSTAT(WALK2STAND), .i = 0, .j = 0 }, { .newMoveStatus = MSTAT(LAND), .i = -1, .j = 0 })
CALLS(PcFoot,
    { .newMoveStatus = MSTAT(STRAFING_RIGHT), .i = 1, .j = RDKEYMARKER_LEFTFOOT }, { .newMoveStatus = MSTAT(STRAFING_LEFT), .i = 1, .j = RDKEYMARKER_LEFTFOOT },
    { .newMoveStatus = MSTAT(WALKING), .i = 1, .j = RDKEYMARKER_LEFTFOOT }, { .newMoveStatus = MSTAT(STRAFING_LEFT), .i = 2, .j = RDKEYMARKER_RIGHTFOOT },
    { .newMoveStatus = MSTAT(STRAFING_RIGHT), .i = 2, .j = RDKEYMARKER_RIGHTFOOT }, { .newMoveStatus = MSTAT(RUNNING), .i = 2, .j = RDKEYMARKER_RIGHTFOOT })
CALLS(PcDied,
    { .newMoveStatus = MSTAT(ROLLING_LEFT), .i = 1, .j = RDKEYMARKER_DIED }, { .newMoveStatus = MSTAT(ROLLING_RIGHT), .i = 1, .j = RDKEYMARKER_DIED },
    { .newMoveStatus = MSTAT(KNOCKEDOUT), .i = 1, .j = RDKEYMARKER_DIED }, { .newMoveStatus = MSTAT(RUNOVER), .i = 1, .j = RDKEYMARKER_DIED },
    { .newMoveStatus = MSTAT(WALKING), .i = 1, .j = RDKEYMARKER_DIED })
CALLS(PcOtherMarkers,
    { .i = 1, .j = RDKEYMARKER_ATTACK }, { .i = 2, .j = RDKEYMARKER_SWING }, { .i = 3, .j = RDKEYMARKER_JUMP }, { .i = 0, .j = RDKEYMARKER_RUNLEFTFOOT },
    { .i = 7, .j = 35 }, { .i = 1, .j = 36 })
CALLS(PcTrackNums, { .newMoveStatus = MSTAT(LAND), .i = 0, .j = 0 }, { .newMoveStatus = MSTAT(LAND), .i = 9, .j = 0 })

PARAMS(PcFinishes, PcFinish, UA_KF, .velocity = { 0.0f, 0.0f, 0.0f })
PARAMS(PcFinishesVelocity, PcFinish, UA_KF, .velocity = { 0.0f, 0.125f, 0.0f })
PARAMS(PcFinishesFastVelocity, PcFinish, UA_KF, .velocity = { 0.0f, 0.5f, 0.0f })
PARAMS(PcFinishTurnsZero, PcFinishTurns, UA_KF)
PARAMS(PcFinishTurnsVel, PcFinishTurns, UA_KF, .velocity = { 0.0f, 0.03125f, 0.0f })
PARAMS(PcFinishOthers, PcFinishOther, UA_KF)
PARAMS(PcFinishDying, PcFinish, UA_KF, .thingFlags = TFLAG(DYING))
PARAMS(PcFinishDestroyed, PcFinish, UA_KF, .thingFlags = TFLAG(DESTROYED))
PARAMS(PcFinishNoLocal, PcFinish, UA_KF, .noLocal = 1)
PARAMS(PcFinishKnockedOutFlags, PcFinishTrack, UA_KF, .actorFlags = AFLAG(IMMOBILE) | AFLAG(INVULNERABLE) | AFLAG(BOSS), .mode = MODE(DISABLED) | MODE(MOVING))
CALLS(PcRun, PC_FINISH(MSTAT(STAND2RUN)))
CALLS(PcWalk, PC_FINISH(MSTAT(STAND2WALK)))
PARAMS(PcRunZero, PcRun, UA_KF)
PARAMS(PcRunSmall, PcRun, UA_KF, .velocity = { 0.0f, 0.125f, 0.0f })
PARAMS(PcRunExact, PcRun, UA_KF, .velocity = { 0.0f, 0.27000001f, 0.0f })
PARAMS(PcRunBig, PcRun, UA_KF, .velocity = { 0.0f, 0.5f, 0.0f })
PARAMS(PcWalkZero, PcWalk, UA_KF)
PARAMS(PcWalkSmall, PcWalk, UA_KF, .velocity = { 0.0f, 0.03125f, 0.0f })
PARAMS(PcWalkExact, PcWalk, UA_KF, .velocity = { 0.0f, 0.059999999f, 0.0f })
PARAMS(PcWalkBig, PcWalk, UA_KF, .velocity = { 0.0f, 0.25f, 0.0f })
PARAMS(PcFeet, PcFoot, UA_KF)
PARAMS(PcDiedMarkers, PcDied, UA_KF)
PARAMS(PcMarkers, PcOtherMarkers, UA_KF)
PARAMS(PcMarkersNoLocal, PcOtherMarkers, UA_KF, .noLocal = 1)
PARAMS(PcFeetNoLocal, PcFoot, UA_KF, .noLocal = 1)
PARAMS(PcTracks, PcTrackNums, UA_KF)

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_AISetLookThing / AISetLookPos / AISetLookPosEyeLevel
// ---------------------------------------------------------------------------------------------------------------
CALLS(LtTarget, { .i = SITHAIMOVEVEC_TARGET })
CALLS(LtOther, { .i = SITHAIMOVEVEC_OTHER })
CALLS(LtTargetTwice, { .i = SITHAIMOVEVEC_TARGET }, { .i = SITHAIMOVEVEC_TARGET, .xorMode = MODE(TURNING) })
PARAMS(LtActor, LtTarget, .targetPos = { 4.0f, 4.0f, 0.0f }, .targetEye = { 0.0f, 0.0f, 0.5f })
PARAMS(LtActorRotated, LtTarget, .targetPos = { 4.0f, 4.0f, 0.0f }, .targetEye = { 0.25f, 0.5f, 0.5f }, .tx = 1, .ty = 0, .tz = 0)
PARAMS(LtPlayer, LtTarget, .targetType = SITH_THING_PLAYER, .targetPos = { -4.0f, 4.0f, 0.0f }, .targetEye = { 0.0f, 0.0f, 0.5f })
PARAMS(LtItem, LtOther, .targetPos = { 4.0f, 4.0f, 0.0f }, .targetEye = { 0.0f, 0.0f, 0.5f })
PARAMS(LtItemTarget, LtTarget, .targetType = SITH_THING_ITEM, .targetPos = { 4.0f, 4.0f, 0.0f }, .targetEye = { 0.0f, 0.0f, 0.5f })
PARAMS(LtLocked, LtTarget, .submode = SUB(MOVETARGETLOCKED), .targetPos = { 4.0f, 4.0f, 0.0f })
PARAMS(LtDisabled, LtTarget, .mode = MODE(DISABLED), .targetPos = { 4.0f, 4.0f, 0.0f })
PARAMS(LtDisabledEditor, LtTarget, .mode = MODE(DISABLED), .targetPos = { 4.0f, 4.0f, 0.0f }, .debugModeFlags = SITHDEBUG_INEDITOR)
PARAMS(LtWantEvents, LtTarget, .mode = MODE(WANTALLEVENTS), .targetPos = { 4.0f, 4.0f, 0.0f })
PARAMS(LtSamePosition, LtTarget, .pos = { 4.0f, 4.0f, -0.0625f }, .targetPos = { 4.0f, 4.0f, 0.0f })
PARAMS(LtTiny, LtTarget, .targetPos = { 0.0f, 0.0005f, 0.0625f })
PARAMS(LtTinyEdge, LtTarget, .targetPos = { 0.0f, 0.001f, 0.0625f })
PARAMS(LtJustAbove, LtTarget, .targetPos = { 0.0f, 0.0011f, 0.0625f })
PARAMS(LtTinyWantEvents, LtTarget, .mode = MODE(WANTALLEVENTS), .targetPos = { 0.0f, 0.0f, 0.0625f })
PARAMS(LtTwice, LtTargetTwice, .targetPos = { 4.0f, 4.0f, 0.0f })
PARAMS(LtOwnerNotActor, LtTarget, .type = SITH_THING_ITEM, .targetPos = { 4.0f, 4.0f, 0.0f })
PARAMS(LtOwnerPlayer, LtTarget, .type = SITH_THING_PLAYER, .targetPos = { 4.0f, 4.0f, 0.0f })

CALLS(LpFront, { .v = { 0.0f, 4.0f, 0.0625f } })
CALLS(LpFarSide, { .v = { 4.0f, 4.0f, 2.0f } })
CALLS(LpUp, { .v = { 0.0f, 0.0f, 4.0f } })
CALLS(LpAtEye, { .v = { 0.0f, 0.0f, 0.0625f } })
CALLS(LpNaN, { .v = { NAN, 0.0f, 0.0f } })
CALLS(LpInf, { .v = { 1.0e30f, 1.0e30f, 0.0f } })
PARAMS(LpPlain, LpFront, 0)
PARAMS(LpFar, LpFarSide, .pos = { 1.0f, 2.0f, 3.0f })
PARAMS(LpVertical, LpUp, 0)
PARAMS(LpAtEyePos, LpAtEye, 0)
PARAMS(LpAtEyePosWantEvents, LpAtEye, .mode = MODE(WANTALLEVENTS))
PARAMS(LpLooking, LpFront, .mode = MODE(LOOKING) | MODE(DISABLED))
PARAMS(LpLockedSub, LpFront, .submode = SUB(MOVETARGETLOCKED))
PARAMS(LpWantEvents, LpFront, .mode = MODE(WANTALLEVENTS))
PARAMS(LpOwnerNoEye, LpFront, .type = SITH_THING_ITEM)
PARAMS(LpNaN, LpNaN, 0)
PARAMS(LpHuge, LpInf, 0)
PARAMS(LpEyeZ, LpFront, .eyeZ = 1.5f)

CALLS(LeFront, { .v = { 0.0f, 4.0f, 2.0f } })
CALLS(LeBelow, { .v = { 0.0f, 4.0f, -2.0f } })
CALLS(LeEye, { .v = { 0.0f, 4.0f, 0.0625f } })
PARAMS(LePlain, LeFront, 0)
PARAMS(LeBelow, LeBelow, .pos = { 1.0f, 1.0f, 1.0f })
PARAMS(LeEyeLevel, LeEye, 0)
PARAMS(LeTilted, LeFront, .hx = 0, .hy = 3, .hz = 1, .ux = 0, .uy = -1, .uz = 3)
PARAMS(LeLocked, LeFront, .submode = SUB(MOVETARGETLOCKED))
PARAMS(LeWantEvents, LeFront, .mode = MODE(WANTALLEVENTS))
PARAMS(LeBigEye, LeFront, .eyeZ = 4.0f)

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_AISetMovePos / AISetMoveTargetPos
// ---------------------------------------------------------------------------------------------------------------
CALLS(MpFar, { .v = { 0.0f, 4.0f, 0.0f }, .f = 1.0f })
CALLS(MpFarSpeed2, { .v = { 4.0f, 4.0f, 0.5f }, .f = 2.0f })
CALLS(MpNear, { .v = { 0.0f, 0.03125f, 0.0f }, .f = 1.0f })
CALLS(MpOnSpot, { .v = { 0.0f, 0.0f, 0.0f }, .f = 1.0f })
CALLS(MpTwice, { .v = { 0.0f, 4.0f, 0.0f }, .f = 1.0f }, { .v = { 4.0f, 0.0f, 0.0f }, .f = 0.5f })
CALLS(MpHome, { .v = { 8.0f, 0.0f, 0.0f }, .f = 1.0f })
CALLS(MpEdgeEq, { .v = { 0.0f, 0.03f, 0.0f }, .f = 1.0f })
CALLS(MpEdgeAbove, { .v = { 0.0f, 0.030001f, 0.0f }, .f = 1.0f })
CALLS(MpHomeNear, { .v = { 1.0f, 0.0f, 0.0f }, .f = 1.0f })

PARAMS(MpPlain, MpFar, .attachFlags = SITH_ATTACH_THING)
PARAMS(MpPlainSpeed2, MpFarSpeed2, 0)
PARAMS(MpPlainNear, MpNear, 0)
PARAMS(MpPlainNearBig, MpNear, .movesize = 0.5f)
PARAMS(MpEdgeEqPos, MpEdgeEq, 0)
PARAMS(MpEdgeAbovePos, MpEdgeAbove, 0)
PARAMS(MpOnSpotPos, MpOnSpot, 0)
PARAMS(MpLocked, MpFar, .submode = SUB(MOVETARGETLOCKED))
PARAMS(MpDisabled, MpFar, .mode = MODE(DISABLED))
PARAMS(MpDisabledEditor, MpFar, .mode = MODE(DISABLED), .debugModeFlags = SITHDEBUG_INEDITOR)
PARAMS(MpWantEvents, MpFar, .mode = MODE(WANTALLEVENTS))
PARAMS(MpMoving, MpTwice, .mode = MODE(MOVING))
PARAMS(MpHomeInside, MpHomeNear, .maxHomeDist = 4.0f, .homePos = { 0.0f, 0.0f, 0.0f })
PARAMS(MpHomeOutside, MpHome, .maxHomeDist = 4.0f, .homePos = { 0.0f, 0.0f, 0.0f })
PARAMS(MpHomeOutsideNearOwner, MpHome, .maxHomeDist = 4.0f, .homePos = { 7.9375f, 0.0f, 0.0f }, .pos = { 8.0f, 0.0f, 0.0f })
PARAMS(MpHomeEdge, MpHomeNear, .maxHomeDist = 1.0f, .homePos = { 0.0f, 0.0f, 0.0f })
PARAMS(MpHomeNegative, MpHome, .maxHomeDist = -1.0f, .homePos = { 0.0f, 0.0f, 0.0f })
PARAMS(MpHomeSamePos, MpHomeNear, .maxHomeDist = 4.0f, .homePos = { 1.0f, 0.0f, 0.0f })
PARAMS(MpUnderwater, MpNear, .sec0Flags = SITH_SECTOR_UNDERWATER, .movesize = 0.5f)
PARAMS(MpFlying, MpNear, .physFlags = PFLAG(FLY), .movesize = 0.5f)
PARAMS(MpWallCrawl, MpFarSpeed2, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.0f })
PARAMS(MpWallCrawlSpot, MpOnSpot, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.0f, 0.0f, 1.0f })
PARAMS(MpWallCrawlMounting, MpFar, .mode = MODE(WALLCRAWLING), .moveStatus = MSTAT(MOUNTING_WALL))
PARAMS(MpWallCrawlUnreach, MpFarSpeed2, .mode = MODE(WALLCRAWLING), .attachSurface = 1, .surfaceNormal = { 0.7071f, 0.7071f, 0.0625f })
PARAMS(MpWallCrawlUnreachBlock, MpFarSpeed2, .mode = MODE(WALLCRAWLING) | MODE(BLOCK), .attachSurface = 1, .surfaceNormal = { 0.7071f, 0.7071f, 0.0625f })
PARAMS(MpMoveForward, MpFar, .submode = SUB(MOVEFORWARD))
PARAMS(MpMoveForwardSide, MpNear, .submode = SUB(MOVEFORWARD), .pos = { 0.0f, 0.0f, 0.0f })

CALLS(MtFar, { .v = { 0.0f, 4.0f, 0.0f }, .f = 1.0f })
CALLS(MtTwice, { .v = { 0.0f, 4.0f, 0.0f }, .f = 1.0f }, { .v = { 4.0f, 0.0f, 0.0f }, .f = 2.0f })
CALLS(MtNear, { .v = { 0.0f, 0.03125f, 0.0f }, .f = 1.0f })
CALLS(MtOnSpot, { .v = { 0.0f, 0.015625f, 0.0f }, .f = 1.0f })
PARAMS(MtPlain, MtFar, 0)
PARAMS(MtLockedBefore, MtFar, .submode = SUB(MOVETARGETLOCKED))
PARAMS(MtTwice, MtTwice, 0)
PARAMS(MtNear, MtNear, .submode = SUB(MOVETARGETLOCKED))
PARAMS(MtOnSpot, MtOnSpot, .submode = SUB(MOVETARGETLOCKED))
PARAMS(MtDisabled, MtFar, .mode = MODE(DISABLED))
PARAMS(MtDisabledEditor, MtFar, .mode = MODE(DISABLED), .debugModeFlags = SITHDEBUG_INEDITOR)
PARAMS(MtWantEvents, MtFar, .mode = MODE(WANTALLEVENTS))

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_AISpecialMove
// ---------------------------------------------------------------------------------------------------------------
#define SM_KF_ALL .numKf = 11, .aKf = { PSUB(HOPLEFT), PSUB(HOPRIGHT), PSUB(STRAFELEFT), PSUB(STRAFERIGHT), PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT), PSUB(WHIPCLIMBDISMOUNT), PSUB(WHIPSWINGMOUNT), PSUB(WHIPCLIMBMOUNT) }, .toTarget = { 0.0f, 1.0f, 0.0f }
#define SM_KF_WALL .numKf = 12, .aKf = { PSUB(HOPLEFT), PSUB(HOPRIGHT), PSUB(STRAFELEFT), PSUB(STRAFERIGHT), PSUB(WHIPCLIMBUP), PSUB(WHIPCLIMBDOWN), PSUB(WHIPCLIMBLEFT), PSUB(WHIPCLIMBRIGHT), PSUB(WHIPCLIMBDISMOUNT), PSUB(WHIPSWINGMOUNT), PSUB(WHIPCLIMBMOUNT), PSUB(MOUNTWALL) }, .toTarget = { 0.0f, 1.0f, 0.0f }

#define SM(flags) { .i = (flags) }
CALLS(SmRoll, SM(SITHACTORSPECIALMOVE_ROLL | SITHACTORSPECIALMOVE_DIR_RIGHT))
CALLS(SmRollLeft, SM(SITHACTORSPECIALMOVE_ROLL | SITHACTORSPECIALMOVE_DIR_LEFT))
CALLS(SmRollNone, SM(SITHACTORSPECIALMOVE_ROLL))
CALLS(SmRollRandom, SM(SITHACTORSPECIALMOVE_ROLL | SITHACTORSPECIALMOVE_DIR_RANDOM), SM(SITHACTORSPECIALMOVE_ROLL | SITHACTORSPECIALMOVE_DIR_RANDOM), SM(SITHACTORSPECIALMOVE_ROLL | SITHACTORSPECIALMOVE_DIR_RANDOM))
CALLS(SmStrafe, SM(SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_DIR_RIGHT))
CALLS(SmStrafeLeft, SM(SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_DIR_LEFT))
CALLS(SmStrafeNone, SM(SITHACTORSPECIALMOVE_STRAFE))
CALLS(SmStrafeRandom, SM(SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_DIR_RANDOM), SM(SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_DIR_RANDOM), SM(SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_DIR_RANDOM))
CALLS(SmTurn45, SM(SITHACTORSPECIALMOVE_TURN45 | SITHACTORSPECIALMOVE_DIR_RIGHT), SM(SITHACTORSPECIALMOVE_TURN45 | SITHACTORSPECIALMOVE_DIR_LEFT), SM(SITHACTORSPECIALMOVE_TURN45))
CALLS(SmTurn90, SM(SITHACTORSPECIALMOVE_TURN90 | SITHACTORSPECIALMOVE_DIR_RIGHT), SM(SITHACTORSPECIALMOVE_TURN90 | SITHACTORSPECIALMOVE_DIR_LEFT), SM(SITHACTORSPECIALMOVE_TURN90))
CALLS(SmTurn135, SM(SITHACTORSPECIALMOVE_TURN135 | SITHACTORSPECIALMOVE_DIR_RIGHT), SM(SITHACTORSPECIALMOVE_TURN135 | SITHACTORSPECIALMOVE_DIR_LEFT), SM(SITHACTORSPECIALMOVE_TURN135))
CALLS(SmTurn180, SM(SITHACTORSPECIALMOVE_TURN180), SM(SITHACTORSPECIALMOVE_TURN180 | SITHACTORSPECIALMOVE_DIR_LEFT))
CALLS(SmNothing, SM(0), SM(SITHACTORSPECIALMOVE_DIR_LEFT), SM(SITHACTORSPECIALMOVE_DIR_RIGHT | SITHACTORSPECIALMOVE_DIR_RANDOM))
CALLS(SmMount, SM(SITHACTORSPECIALMOVE_MOUNTWALL))

PARAMS(SmRollOk, SmRoll, SM_KF_ALL, .moveScale = 1.0f)
PARAMS(SmRollOkLeft, SmRollLeft, SM_KF_ALL)
PARAMS(SmRollOkDefault, SmRollNone, SM_KF_ALL)
PARAMS(SmRollRand, SmRollRandom, SM_KF_ALL, .randSeed = 12345)
PARAMS(SmRollRand2, SmRollRandom, SM_KF_ALL, .randSeed = 777)
PARAMS(SmRollBehind, SmRoll, SM_KF_ALL, .toTarget = { 0.0f, -1.0f, 0.0f })
PARAMS(SmRollSide, SmRoll, SM_KF_ALL, .toTarget = { 1.0f, 0.0f, 0.0f })
PARAMS(SmRollJust, SmRoll, SM_KF_ALL, .toTarget = { 0.0f, 0.4f, 0.9165f })
PARAMS(SmRollJust2, SmRoll, SM_KF_ALL, .toTarget = { 0.0f, 0.41f, 0.9121f })
PARAMS(SmRollNoLeftKf, SmRoll, .numKf = 1, .aKf = { PSUB(HOPRIGHT) }, .toTarget = { 0.0f, 1.0f, 0.0f })
PARAMS(SmRollNoRightKf, SmRoll, .numKf = 1, .aKf = { PSUB(HOPLEFT) }, .toTarget = { 0.0f, 1.0f, 0.0f })
PARAMS(SmRollNoKf, SmRoll, .toTarget = { 0.0f, 1.0f, 0.0f })
PARAMS(SmRollPathHit, SmRoll, SM_KF_ALL, .aCheckPath = { 1, 0, 0, 0 })
PARAMS(SmRollPathHitTwice, SmRoll, SM_KF_ALL, .aCheckPath = { 1, 1, 0, 0 })
PARAMS(SmRollPathHitAlways, SmRoll, SM_KF_ALL, .aCheckPath = { 1, 1, 1, 1 })
PARAMS(SmRollHalfBlocked, SmRoll, SM_KF_ALL, .aCheckPos = { SITHAIUTIL_FLOOR_NONE, SITHAIUTIL_FLOOR_OK }, .numCheckPos = 2)
PARAMS(SmRollEndBlocked, SmRoll, SM_KF_ALL, .aCheckPos = { SITHAIUTIL_FLOOR_OK, SITHAIUTIL_FLOOR_WALL }, .numCheckPos = 2)
PARAMS(SmRollMoving, SmRoll, SM_KF_ALL, .mode = MODE(MOVING) | MODE(LOOKING) | MODE(TURNING), .movePos = { 0.0f, 4.0f, 0.0f }, .velocity = { 0.0f, 0.25f, 0.0f }, .actorFlags = AFLAG(CANROTATEHEAD), .headPYR = { 5.0f, 10.0f, 0.0f })
PARAMS(SmRollRotated, SmRoll, SM_KF_ALL, .hx = 1, .hy = 1, .hz = 0, .pos = { 2.0f, 2.0f, 0.5f })
PARAMS(SmRollLocked, SmRoll, SM_KF_ALL, .submode = SUB(MOVETARGETLOCKED))
PARAMS(SmRollBigMove, SmRoll, SM_KF_ALL, .movesize = 0.5f)

PARAMS(SmStrafeOk, SmStrafe, SM_KF_ALL)
PARAMS(SmStrafeOkLeft, SmStrafeLeft, SM_KF_ALL)
PARAMS(SmStrafeOkDefault, SmStrafeNone, SM_KF_ALL)
PARAMS(SmStrafeRand, SmStrafeRandom, SM_KF_ALL, .randSeed = 12345)
PARAMS(SmStrafeTarget, SmStrafe, SM_KF_ALL, .target = 1, .targetPos = { 4.0f, 4.0f, 0.0f })
PARAMS(SmStrafeJust, SmStrafe, SM_KF_ALL, .toTarget = { 0.0f, 0.4f, 0.9165f })
PARAMS(SmStrafeJust2, SmStrafe, SM_KF_ALL, .toTarget = { 0.0f, 0.41f, 0.9121f })
PARAMS(SmStrafeBehind, SmStrafe, SM_KF_ALL, .toTarget = { 0.0f, -1.0f, 0.0f })
PARAMS(SmStrafeNoKf, SmStrafe, .toTarget = { 0.0f, 1.0f, 0.0f })
PARAMS(SmStrafePathHit, SmStrafe, SM_KF_ALL, .aCheckPath = { 1, 0, 0, 0 })
PARAMS(SmStrafePathHitAlways, SmStrafe, SM_KF_ALL, .aCheckPath = { 1, 1, 1, 1 })
PARAMS(SmStrafeEndBlocked, SmStrafe, SM_KF_ALL, .aCheckPos = { SITHAIUTIL_FLOOR_BLOCKED }, .numCheckPos = 1)
PARAMS(SmStrafeMoving, SmStrafe, SM_KF_ALL, .mode = MODE(MOVING), .movePos = { 0.0f, 4.0f, 0.0f }, .velocity = { 0.0f, 0.25f, 0.0f }, .submode = SUB(CONTINUOUSMOTION))

PARAMS(SmTurn45Ok, SmTurn45, SM_KF_ALL)
PARAMS(SmTurn45NoRight, SmTurn45, .numKf = 1, .aKf = { PSUB(WHIPCLIMBUP) })
PARAMS(SmTurn45NoLeft, SmTurn45, .numKf = 1, .aKf = { PSUB(WHIPCLIMBDOWN) })
PARAMS(SmTurn45NoKf, SmTurn45, 0)
PARAMS(SmTurn90Ok, SmTurn90, SM_KF_ALL)
PARAMS(SmTurn90NoKf, SmTurn90, 0)
PARAMS(SmTurn90NoRight, SmTurn90, .numKf = 1, .aKf = { PSUB(WHIPCLIMBLEFT) })
PARAMS(SmTurn135Ok, SmTurn135, SM_KF_ALL)
PARAMS(SmTurn135NoKf, SmTurn135, 0)
PARAMS(SmTurn135NoRight, SmTurn135, .numKf = 1, .aKf = { PSUB(WHIPCLIMBDISMOUNT) })
PARAMS(SmTurn180Ok, SmTurn180, SM_KF_ALL)
PARAMS(SmTurn180NoKf, SmTurn180, 0)
PARAMS(SmNothingToDo, SmNothing, SM_KF_ALL)
PARAMS(SmMajorModeOther, SmTurn45, SM_KF_ALL, .majorMode = 9)

PARAMS(SmMountPerp, SmMount, SM_KF_WALL, .savedMovePos = { 1.0f, 0.0f, 0.0f })
PARAMS(SmMountPerpNeg, SmMount, SM_KF_WALL, .savedMovePos = { -1.0f, 0.0f, 0.0f })
PARAMS(SmMount80, SmMount, SM_KF_WALL, .savedMovePos = { 0.9848f, 0.0f, 0.17365f })
PARAMS(SmMount100, SmMount, SM_KF_WALL, .savedMovePos = { 0.9848f, 0.0f, -0.17365f })
PARAMS(SmMount79, SmMount, SM_KF_WALL, .savedMovePos = { 0.98163f, 0.0f, 0.19081f })
PARAMS(SmMount70, SmMount, SM_KF_WALL, .savedMovePos = { 0.9397f, 0.0f, 0.3420f })
PARAMS(SmMount60, SmMount, SM_KF_WALL, .savedMovePos = { 0.866f, 0.0f, 0.5f })
PARAMS(SmMount60Back, SmMount, SM_KF_WALL, .savedMovePos = { -0.866f, 0.0f, 0.5f })
PARAMS(SmMount795, SmMount, SM_KF_WALL, .savedMovePos = { 0.9833f, 0.0f, 0.1822f })
PARAMS(SmMount50, SmMount, SM_KF_WALL, .savedMovePos = { 0.766f, 0.0f, 0.6428f })
PARAMS(SmMount45, SmMount, SM_KF_WALL, .savedMovePos = { 0.7071f, 0.0f, 0.7071f })
PARAMS(SmMount41, SmMount, SM_KF_WALL, .savedMovePos = { 0.6561f, 0.0f, 0.7547f })
PARAMS(SmMount40, SmMount, SM_KF_WALL, .savedMovePos = { 0.6428f, 0.0f, 0.766f })
PARAMS(SmMount30, SmMount, SM_KF_WALL, .savedMovePos = { 0.5f, 0.0f, 0.866f })
PARAMS(SmMount80Eq, SmMount, SM_KF_WALL, .savedMovePos = { 0.984808f, 0.0f, 0.173648193f })
PARAMS(SmMount80Up, SmMount, SM_KF_WALL, .savedMovePos = { 0.984808f, 0.0f, 0.173648208f })
PARAMS(SmMount80Down, SmMount, SM_KF_WALL, .savedMovePos = { 0.984808f, 0.0f, 0.173648178f })
PARAMS(SmMount100Eq, SmMount, SM_KF_WALL, .savedMovePos = { 0.984808f, 0.0f, -0.173648193f })
PARAMS(SmMount100Up, SmMount, SM_KF_WALL, .savedMovePos = { 0.984808f, 0.0f, -0.173648208f })
PARAMS(SmMount100Down, SmMount, SM_KF_WALL, .savedMovePos = { 0.984808f, 0.0f, -0.173648178f })
PARAMS(SmMount79Eq, SmMount, SM_KF_WALL, .savedMovePos = { 0.981627f, 0.0f, 0.190808997f })
PARAMS(SmMount79Up, SmMount, SM_KF_WALL, .savedMovePos = { 0.981627f, 0.0f, 0.190809012f })
PARAMS(SmMount79Down, SmMount, SM_KF_WALL, .savedMovePos = { 0.981627f, 0.0f, 0.190808982f })
PARAMS(SmMountSame, SmMount, SM_KF_WALL, .savedMovePos = { 0.0f, 0.0f, 1.0f })
PARAMS(SmMountOpposite, SmMount, SM_KF_WALL, .savedMovePos = { 0.0f, 0.0f, -1.0f })
PARAMS(SmMountBehind, SmMount, SM_KF_WALL, .savedMovePos = { 0.0f, -1.0f, 0.0f })
PARAMS(SmMountNoKf, SmMount, .savedMovePos = { 1.0f, 0.0f, 0.0f })
PARAMS(SmMount110, SmMount, SM_KF_WALL, .savedMovePos = { 0.9397f, 0.0f, -0.342f })
PARAMS(SmMountAttached, SmMount, SM_KF_WALL, .savedMovePos = { 0.7071f, 0.0f, 0.7071f }, .attachFlags = SITH_ATTACH_SURFACE)

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_AIFinalizeSpecialMove
// ---------------------------------------------------------------------------------------------------------------
CALLS(FsStrafe, { .i = SITHACTORSPECIALMOVE_STRAFE })
CALLS(FsTurn45, { .i = SITHACTORSPECIALMOVE_TURN45 })
CALLS(FsTurn90, { .i = SITHACTORSPECIALMOVE_TURN90 })
CALLS(FsTurn135, { .i = SITHACTORSPECIALMOVE_TURN135 })
CALLS(FsTurn180, { .i = SITHACTORSPECIALMOVE_TURN180 })
CALLS(FsMount, { .i = SITHACTORSPECIALMOVE_MOUNTWALL })
CALLS(FsOthers, { .i = SITHACTORSPECIALMOVE_ROLL }, { .i = 0 }, { .i = SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_ROLL }, { .i = SITHACTORSPECIALMOVE_DIR_LEFT }, { .i = SITHACTORSPECIALMOVE_TURN45 | SITHACTORSPECIALMOVE_DIR_LEFT })

PARAMS(FsStrafeNoMove, FsStrafe, .moveDistance = 0.5f, .velocity = { 0.0f, 0.5f, 0.0f })
PARAMS(FsStrafeDirection, FsStrafe, .mode = MODE(MOVING), .submode = SUB(CONTINUOUSMOTION), .velocity = { 0.25f, 0.0f, 0.0f })
PARAMS(FsTurn45Left, FsTurn45, .moveStatus = MSTAT(TURNING_LEFT_45_DEGREES))
PARAMS(FsTurn45Right, FsTurn45, .moveStatus = MSTAT(TURNING_RIGHT_45_DEGREES))
PARAMS(FsTurn90Left, FsTurn90, .moveStatus = MSTAT(TURNING_LEFT_90_DEGREES))
PARAMS(FsTurn90Right, FsTurn90, .moveStatus = MSTAT(TURNING_RIGHT_90_DEGREES))
PARAMS(FsTurn135Left, FsTurn135, .moveStatus = MSTAT(TURNING_LEFT_135_DEGREES))
PARAMS(FsTurn135Right, FsTurn135, .moveStatus = MSTAT(TURNING_RIGHT_135_DEGREES))
PARAMS(FsTurn180, FsTurn180, .moveStatus = MSTAT(TURNING_180_DEGREES))
PARAMS(FsTurnTilted, FsTurn90, .moveStatus = MSTAT(TURNING_RIGHT_90_DEGREES), .hx = 0, .hy = 3, .hz = 1, .ux = 0, .uy = -1, .uz = 3)
PARAMS(FsTurn45Rotated, FsTurn45, .moveStatus = MSTAT(TURNING_LEFT_45_DEGREES), .hx = 1, .hy = 2, .hz = 0)
PARAMS(FsMountMounting, FsMount, .moveStatus = MSTAT(MOUNTING_WALL), .savedMovePos = { 1.0f, 0.0f, 0.0f }, .attachFlags = SITH_ATTACH_SURFACE)
PARAMS(FsMountMountingBigMove, FsMount, .moveStatus = MSTAT(MOUNTING_WALL), .savedMovePos = { 0.0f, 0.0f, 1.0f }, .movesize = 0.5f, .moveScale = 0.5f)
PARAMS(FsMountOtherStatus, FsMount, .moveStatus = MSTAT(WALKING), .savedMovePos = { 0.0f, 1.0f, 0.0f })
PARAMS(FsMountDiag, FsMount, .moveStatus = MSTAT(MOUNTING_WALL), .savedMovePos = { 0.7071f, 0.0f, 0.7071f })
PARAMS(FsMountDegenerate, FsMount, .moveStatus = MSTAT(MOUNTING_WALL), .savedMovePos = { 0.0f, 1.0f, 0.0f })
PARAMS(FsNothing, FsOthers, 0)

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_AIJump / AIStop / SetGoalReached / Unreachable / StopAIMovement / ResetAILook
// ---------------------------------------------------------------------------------------------------------------
CALLS(JpJump, { .v = { 1.0f, 4.0f, 2.0f }, .f = 1.0f })
CALLS(JpJump2, { .v = { 1.0f, 4.0f, 2.0f }, .f = -1.0f }, { .v = { -1.0f, 2.0f, 0.5f }, .f = 0.5f, .addMsec = 100 })
PARAMS(JpPlayMode, JpJump, .msecGameTime = 1000, .playModeRet = 0)
PARAMS(JpPlayModeFail, JpJump, .msecGameTime = 1000, .playModeRet = -1)
PARAMS(JpTwice, JpJump2, .msecGameTime = 5000, .playModeRet = -1, .mode = MODE(LOOKING))
PARAMS(JpTimeWrap, JpJump, .msecGameTime = 0xFFFFF000u, .playModeRet = 0)

CALLS(Once, { .f = 0.0f })
CALLS(Twice, { .f = 0.0f }, { .f = 0.0f })
PARAMS(StopPlain, Once, .mode = MODE(MOVING) | MODE(TURNING) | MODE(TRAVERSEWPNTS), .velocity = { 0.0f, 0.5f, 0.0f }, .headPYR = { 10.0f, 20.0f, 0.0f }, .goalLVec = { 1.0f, 0.0f, 0.0f }, .movePos = { 4.0f, 4.0f, 0.0f }, .pos = { 1.0f, 2.0f, 3.0f }, UA_KF)
PARAMS(StopContinuous, Once, .mode = MODE(MOVING) | MODE(TURNING), .submode = SUB(CONTINUOUSMOTION), .velocity = { 0.0f, 0.5f, 0.0f }, .goalLVec = { 1.0f, 0.0f, 0.0f }, .pos = { 1.0f, 2.0f, 3.0f }, UA_KF)
PARAMS(StopAnimated, Once, .mode = MODE(MOVING), .moveStatus = MSTAT(ROLLING_LEFT), .velocity = { 0.0f, 0.5f, 0.0f }, UA_KF)
PARAMS(StopStationary, Once, .mode = MODE(MOVING), .moveStatus = MSTAT(TURNING_180_DEGREES), .velocity = { 0.0f, 0.5f, 0.0f }, UA_KF)
PARAMS(StopWalking, Once, .mode = MODE(MOVING), .moveStatus = MSTAT(WALKING), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(RUN), UA_KF)

PARAMS(GrPlain, Once, .mode = MODE(MOVING), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)
PARAMS(GrBlock, Once, .mode = MODE(MOVING) | MODE(BLOCK), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)
PARAMS(GrNewMove, Once, .mode = MODE(MOVING), .submode = SUB(NEWMOVE), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)
PARAMS(GrWpnts, Once, .mode = MODE(MOVING) | MODE(TRAVERSEWPNTS), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)
PARAMS(GrAnimated, Once, .mode = MODE(MOVING), .moveStatus = MSTAT(STRAFING_LEFT), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)
PARAMS(GrStationary, Once, .mode = MODE(MOVING), .moveStatus = MSTAT(MOUNTING_WALL), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)
PARAMS(GrContinuous, Once, .mode = MODE(MOVING), .submode = SUB(CONTINUOUSMOTION), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)
PARAMS(GrStillStanding, Once, .mode = MODE(MOVING), .puppetSubmode = PSUB(WALK), UA_KF)

PARAMS(UnPlain, Once, .mode = MODE(MOVING), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)
PARAMS(UnBlock, Once, .mode = MODE(MOVING) | MODE(BLOCK), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)
PARAMS(UnAnimated, Twice, .mode = MODE(MOVING), .moveStatus = MSTAT(ROLLING_LEFT), .velocity = { 0.0f, 0.5f, 0.0f }, .puppetSubmode = PSUB(WALK), UA_KF)

PARAMS(SaMoving, Once, .mode = MODE(MOVING) | MODE(TRAVERSEWPNTS), .velocity = { 0.0f, 0.5f, 0.0f })
PARAMS(SaContinuous, Once, .mode = MODE(MOVING) | MODE(TRAVERSEWPNTS), .submode = SUB(CONTINUOUSMOTION), .velocity = { 0.0f, 0.5f, 0.0f })
PARAMS(SaIdle, Twice, .velocity = { 0.0f, 0.5f, 0.0f })

PARAMS(RlTurning, Once, .mode = MODE(TURNING), .headPYR = { 10.0f, 20.0f, 0.0f }, .goalLVec = { 1.0f, 0.0f, 0.0f }, .hx = 1, .hy = 2, .hz = 0)
PARAMS(RlIdle, Once, .headPYR = { 0.0f, 0.0f, 0.0f }, .goalLVec = { 1.0f, 0.0f, 0.0f })

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_UpdateBoss
// ---------------------------------------------------------------------------------------------------------------
PARAMS(BsRight, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(BsLeft, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { -1.0f, 0.0f, 0.0f })
PARAMS(BsBehind, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 0.0f, -1.0f, 0.0f })
PARAMS(BsNear, Dt3, .mode = MODE(TURNING), .goalLVec = { 0.0625f, 1.0f, 0.0f })
PARAMS(BsAligned, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 1.0f, 0.0f })
PARAMS(BsUpDotEqual, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.1f, 0.99000001f })
PARAMS(BsUpDotAbove, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.1f, 0.99000006f })
CALLS(DtBig1b, { .f = 1.0f })
PARAMS(BsLimitEqual, DtBig1b, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f }, .maxRot = 85.7142868f)
PARAMS(BsLimitBelow, DtBig1b, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f }, .maxRot = 85.7142792f)
PARAMS(BsLimitEqualBehind, DtBig1b, .mode = MODE(TURNING), .goalLVec = { 0.0f, -1.0f, 0.0f }, .maxRot = 171.428574f)
PARAMS(BsNegativeRate, DtQuarter4, .mode = MODE(TURNING), .goalLVec = { 0.0f, 1.0f, 0.0f }, .maxRot = -90.0f)
PARAMS(BsLookUp, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.0f, 1.0f })
PARAMS(BsLookDown, Dt1, .mode = MODE(TURNING) | MODE(MOVING), .goalLVec = { 0.0f, 0.0f, -1.0f })
PARAMS(BsSteep, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.1f, 1.0f })
PARAMS(BsSteepBelow, Dt1, .mode = MODE(TURNING), .goalLVec = { 0.0f, 0.2f, 1.0f })
PARAMS(BsTilted, DtQuarter4, .mode = MODE(TURNING), .hx = 0, .hy = 3, .hz = 1, .ux = 0, .uy = -1, .uz = 3, .goalLVec = { 1.0f, 2.0f, 1.0f })
PARAMS(BsFast, Dt1, .mode = MODE(TURNING), .maxRot = 720.0f, .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(BsKeepsStatus, DtQuarter4, .mode = MODE(TURNING), .moveStatus = MSTAT(WALKING), .goalLVec = { 1.0f, 0.0f, 0.0f })
PARAMS(BsZeroDt, Dt0, .mode = MODE(TURNING), .goalLVec = { 1.0f, 0.0f, 0.0f })

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_UpdateHeadTrackTarget / UpdateHeadTrackLookPos
// ---------------------------------------------------------------------------------------------------------------
#define HT_BASE .actorFlags = AFLAG(CANROTATEHEAD), .target = 1, .targetEye = { 0.0f, 0.0f, 0.5f }

PARAMS(HtNoRotate, Dt3, .target = 1, .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(HtImmobile, Dt3, HT_BASE, .actorFlags = AFLAG(CANROTATEHEAD) | AFLAG(IMMOBILE), .targetPos = { 4.0f, 4.0f, 0.0f })
PARAMS(HtTargetRight, Dt6, HT_BASE, .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtTargetLeft, Dt6, HT_BASE, .targetPos = { -4.0f, 4.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtTargetAhead, Dt3, HT_BASE, .targetPos = { 0.0f, 4.0f, 2.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtTargetBelow, Dt6, HT_BASE, .targetPos = { 0.0f, 4.0f, -2.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtTargetBehind, Dt6, HT_BASE, .targetPos = { 0.0f, -4.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtTargetBehindTracking, Dt6, HT_BASE, .targetPos = { 0.0f, -4.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f }, .submode = SUB(HEADTRACKINGTARGET), .headPYR = { 0.0f, 60.0f, 0.0f })
PARAMS(HtTargetSlightlyBehind, Dt6, HT_BASE, .targetPos = { 4.0f, -0.5f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f }, .submode = SUB(HEADTRACKINGTARGET))
PARAMS(HtTargetSlightlyBehindNot, Dt6, HT_BASE, .targetPos = { 4.0f, -0.5f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtTargetBehindDeep, Dt6, HT_BASE, .targetPos = { 1.0f, -4.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f }, .submode = SUB(HEADTRACKINGTARGET))
PARAMS(HtTargetSide, Dt6, HT_BASE, .targetPos = { 4.0f, 0.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtYawExactRight, Dt1, HT_BASE, .submode = SUB(HEADTRACKINGMOTION) | SUB(HEADTRACKINGTARGET), .targetPos = { 4.0f, 0.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f }, .maxHeadYaw = 90.0f, .maxHeadVel = 3600.0f, .targetEye = { 0.0f, 0.0f, 0.0625f })
PARAMS(HtYawExactLeft, Dt1, HT_BASE, .submode = SUB(HEADTRACKINGMOTION) | SUB(HEADTRACKINGTARGET), .targetPos = { -4.0f, 0.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f }, .maxHeadYaw = 90.0f, .maxHeadVel = 3600.0f, .targetEye = { 0.0f, 0.0f, 0.0625f })
PARAMS(HtBehindBlockExact, Dt1, HT_BASE, .mode = MODE(BLOCK), .targetPos = { 0.0f, -4.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f }, .maxHeadVel = 3600.0f, .targetEye = { 0.0f, 0.0f, 0.0625f }, .headPYR = { 0.0f, 20.0f, 0.0f })
PARAMS(HtTargetSideTracking, Dt6, HT_BASE, .targetPos = { 4.0f, 0.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f }, .submode = SUB(HEADTRACKINGTARGET))
PARAMS(HtNoTarget, Dt3, .actorFlags = AFLAG(CANROTATEHEAD), .lookPos = { 4.0f, 4.0f, 2.0f }, .submode = SUB(HEADTRACKINGTARGET))
PARAMS(HtNoTargetLook, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .lookPos = { -4.0f, 4.0f, -2.0f })
PARAMS(HtInvisibleTarget, Dt3, HT_BASE, .targetPos = { 4.0f, 4.0f, 0.0f }, .targetActorFlags = AFLAG(INVISIBLE), .lookPos = { -4.0f, 4.0f, 0.0625f })
PARAMS(HtInvisibleSeen, Dt3, HT_BASE, .actorFlags = AFLAG(CANROTATEHEAD) | AFLAG(SEEINVISIBLE), .targetPos = { 4.0f, 4.0f, 0.0f }, .targetActorFlags = AFLAG(INVISIBLE), .lookPos = { -4.0f, 4.0f, 0.0625f })
PARAMS(HtInvisibleItem, Dt3, HT_BASE, .targetType = SITH_THING_ITEM, .targetPos = { 4.0f, 4.0f, 0.0f }, .targetActorFlags = AFLAG(INVISIBLE), .lookPos = { -4.0f, 4.0f, 0.0625f })
PARAMS(HtInvisiblePlayer, Dt3, HT_BASE, .targetType = SITH_THING_PLAYER, .targetPos = { 4.0f, 4.0f, 0.0f }, .targetActorFlags = AFLAG(INVISIBLE), .lookPos = { -4.0f, 4.0f, 0.0625f })
PARAMS(HtBlind, Dt3, HT_BASE, .actorFlags = AFLAG(CANROTATEHEAD) | AFLAG(BLIND), .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { -4.0f, 4.0f, 0.0625f })
PARAMS(HtBlindSeeInvisible, Dt3, HT_BASE, .actorFlags = AFLAG(CANROTATEHEAD) | AFLAG(BLIND) | AFLAG(SEEINVISIBLE), .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { -4.0f, 4.0f, 0.0625f })
PARAMS(HtItemTarget, Dt3, HT_BASE, .targetType = SITH_THING_ITEM, .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { -4.0f, 4.0f, 0.0625f })
PARAMS(HtPlayerTarget, Dt3, HT_BASE, .targetType = SITH_THING_PLAYER, .targetPos = { 4.0f, 4.0f, 0.0f }, .lookPos = { -4.0f, 4.0f, 0.0625f })
PARAMS(HtNoSlope, Dt6, HT_BASE, .actorFlags = AFLAG(CANROTATEHEAD) | AFLAG(NOSLOPEMOVE), .targetPos = { 0.0f, -4.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtBlockMode, Dt6, HT_BASE, .mode = MODE(BLOCK), .targetPos = { 0.0f, -4.0f, 0.0f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtZeroYawLimit, Dt1, HT_BASE, .zeroHeadYaw = 1, .targetPos = { 0.0f, 4.0f, 0.0f }, .targetEye = { 0.0f, 0.0f, 0.0625f }, .maxHeadVel = 3600.0f)
PARAMS(HtLowEqual, Dt1, HT_BASE, .submode = SUB(HEADTRACKINGTARGET), .targetPos = { 4.89897966f, -1.0f, 0.0f }, .targetEye = { 0.0f, 0.0f, 0.0625f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtLowAbove, Dt1, HT_BASE, .submode = SUB(HEADTRACKINGTARGET), .targetPos = { 4.89897966f, -1.00000012f, 0.0f }, .targetEye = { 0.0f, 0.0f, 0.0625f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtLowBelow, Dt1, HT_BASE, .submode = SUB(HEADTRACKINGTARGET), .targetPos = { 4.89897966f, -0.99999988f, 0.0f }, .targetEye = { 0.0f, 0.0f, 0.0625f }, .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HtLimits, Dt6, HT_BASE, .targetPos = { 4.0f, 4.0f, 8.0f }, .maxHeadYaw = 20.0f, .maxPitch = 10.0f, .minPitch = -10.0f)
PARAMS(HtLimitsDown, Dt6, HT_BASE, .targetPos = { -4.0f, 4.0f, -8.0f }, .maxHeadYaw = 20.0f, .maxPitch = 10.0f, .minPitch = -10.0f)
PARAMS(HtLimitedYawExact, Dt6, HT_BASE, .submode = SUB(HEADTRACKINGMOTION) | SUB(HEADTRACKINGTARGET), .targetPos = { 4.0f, 4.0f, 0.0f }, .maxHeadYaw = 45.0f, .maxHeadVel = 360.0f)
PARAMS(HtFastHead, Dt1, HT_BASE, .targetPos = { 4.0f, 4.0f, 2.0f }, .maxHeadVel = 3600.0f)
PARAMS(HtSlowHead, Dt6, HT_BASE, .targetPos = { 4.0f, 4.0f, 2.0f }, .maxHeadVel = 20.0f)
PARAMS(HtStartPitched, Dt6, HT_BASE, .targetPos = { 0.0f, 4.0f, 0.0f }, .headPYR = { 30.0f, -40.0f, 0.0f })
PARAMS(HtTilted, Dt6, HT_BASE, .hx = 0, .hy = 3, .hz = 1, .ux = 0, .uy = -1, .uz = 3, .targetPos = { 4.0f, 4.0f, 2.0f })
PARAMS(HtTargetOverhead, Dt3, HT_BASE, .targetPos = { 0.0f, 0.0f, 4.0f })
PARAMS(HtTargetRotated, Dt6, HT_BASE, .targetPos = { 4.0f, 4.0f, 0.0f }, .tx = 1, .ty = 1, .tz = 0, .targetEye = { 0.5f, 0.5f, 0.5f })
PARAMS(HtTargetAtOwner, Dt3, HT_BASE, .targetPos = { 0.0f, 0.0f, -0.0625f }, .targetEye = { 0.0f, 0.0f, 0.0625f })
PARAMS(HtLookPosEqualsEye, Dt3, .actorFlags = AFLAG(CANROTATEHEAD), .lookPos = { 0.0f, 0.0f, 0.0625f })

PARAMS(HlNoRotate, Dt3, .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(HlImmobile, Dt3, .actorFlags = AFLAG(CANROTATEHEAD) | AFLAG(IMMOBILE), .lookPos = { 4.0f, 4.0f, 0.0f })
PARAMS(HlRight, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 4.0f, 4.0f, 0.0625f })
PARAMS(HlLeft, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { -4.0f, 4.0f, 2.0f })
PARAMS(HlBehind, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, -4.0f, 0.0625f })
PARAMS(HlAhead, Dt3, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HlAheadDone, Dt3, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING) | MODE(TURNING), .lookPos = { 0.0f, 4.0f, 0.0625f })
PARAMS(HlUp, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 0.0f, 4.0f })
PARAMS(HlDown, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 4.0f, -4.0f })
PARAMS(HlSlowReachesGoal, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.5f, 4.0f, 0.0625f }, .maxHeadVel = 40.0f)
PARAMS(HlStartPitched, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 4.0f, 0.0625f }, .headPYR = { 20.0f, -30.0f, 0.0f })
PARAMS(HlPitchOnly, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 4.0f, 3.0f }, .maxHeadVel = 20.0f)
PARAMS(HlYawOnly, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 3.0f, 4.0f, 0.0625f }, .maxHeadVel = 20.0f)
PARAMS(HlYawBelowHalf, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 4.0f, 0.0625f }, .maxHeadVel = 20.0f, .headPYR = { 0.0f, 0.25f, 0.0f })
PARAMS(HlYawAboveHalf, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 4.0f, 0.0625f }, .maxHeadVel = 20.0f, .headPYR = { 0.0f, 0.75f, 0.0f })
PARAMS(HlPitchBelowHalf, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 4.0f, 0.0625f }, .maxHeadVel = 20.0f, .headPYR = { 0.25f, 0.0f, 0.0f })
PARAMS(HlPitchAboveHalf, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 4.0f, 0.0625f }, .maxHeadVel = 20.0f, .headPYR = { 0.75f, 0.0f, 0.0f })
PARAMS(HlYawDoneExact, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 4.0f, 0.0f, 0.0625f }, .maxHeadYaw = 100.0f, .maxHeadVel = 3600.0f, .headPYR = { 0.0f, -89.5f, 0.0f })
PARAMS(HlYawDoneAbove, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 4.0f, 0.0f, 0.0625f }, .maxHeadYaw = 100.0f, .maxHeadVel = 3600.0f, .headPYR = { 0.0f, -89.25f, 0.0f })
PARAMS(HlPitchDoneExact, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 4.0f, 0.0f, 0.0625f }, .maxHeadYaw = 100.0f, .maxHeadVel = 3600.0f, .headPYR = { -0.5f, -90.0f, 0.0f })
PARAMS(HlPitchDoneAbove, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 4.0f, 0.0f, 0.0625f }, .maxHeadYaw = 100.0f, .maxHeadVel = 3600.0f, .headPYR = { -0.75f, -90.0f, 0.0f })
PARAMS(HlLimits, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 4.0f, 4.0f, 8.0f }, .maxHeadYaw = 20.0f, .maxPitch = 10.0f, .minPitch = -10.0f)
PARAMS(HlBehindClamped, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, -4.0f, 4.0f }, .maxHeadYaw = 20.0f)
PARAMS(HlTilted, Dt6, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .hx = 0, .hy = 3, .hz = 1, .ux = 0, .uy = -1, .uz = 3, .lookPos = { 4.0f, 4.0f, 2.0f })
PARAMS(HlFast, Dt1, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 4.0f, 4.0f, 2.0f }, .maxHeadVel = 3600.0f)
PARAMS(HlAtEye, Dt3, .actorFlags = AFLAG(CANROTATEHEAD), .mode = MODE(LOOKING), .lookPos = { 0.0f, 0.0f, 0.0625f })

// ---------------------------------------------------------------------------------------------------------------
// Quetzalcoatl: strike angles, the strike and the other animations, the tail update, marduk
// ---------------------------------------------------------------------------------------------------------------
#define QZ_BASE .tail = 1, .quetz = 1, .target = 1, .targetEye = { 0.0f, 0.0f, 0.5f }
CALLS(QaOnce, { .f = 0.0f })
CALLS(QaTwice, { .f = 0.0f }, { .f = 0.0f })

PARAMS(QaStrikeNear, QaOnce, QZ_BASE, .targetPos = { 0.0f, 0.25f, 0.0f })
PARAMS(QaStrikeMin, QaOnce, QZ_BASE, .targetPos = { 0.0f, 0.3f, 0.0f })
PARAMS(QaStrikeMid, QaOnce, QZ_BASE, .targetPos = { 0.0f, 0.4f, 0.0f })
PARAMS(QaStrikeMax, QaOnce, QZ_BASE, .targetPos = { 0.0f, 0.5f, 0.0f })
PARAMS(QaStrikeFar, QaOnce, QZ_BASE, .targetPos = { 0.0f, 3.0f, 0.0f })
PARAMS(QaStrikeSide, QaOnce, QZ_BASE, .targetPos = { 2.0f, 2.0f, 0.0f })
PARAMS(QaStrikeLeftHigh, QaOnce, QZ_BASE, .targetPos = { -2.0f, 1.0f, 3.0f })
PARAMS(QaStrikeBehind, QaOnce, QZ_BASE, .targetPos = { 0.0f, -1.0f, 0.0f })
PARAMS(QaStrikeCrouching, QaOnce, QZ_BASE, .targetPos = { 0.0f, 0.4f, 0.0f }, .targetPhysFlags = PFLAG(CROUCHING))
PARAMS(QaStrikeRotatedTarget, QaOnce, QZ_BASE, .targetPos = { 0.0f, 0.4f, 0.0f }, .tx = 1, .ty = 1, .tz = 0, .targetEye = { 0.25f, 0.5f, 0.5f })
PARAMS(QaStrikeTilted, QaOnce, QZ_BASE, .targetPos = { 1.0f, 1.0f, 1.0f }, .hx = 0, .hy = 3, .hz = 1, .ux = 0, .uy = -1, .uz = 3)
PARAMS(QaStrikeOwnerEye, QaOnce, QZ_BASE, .targetPos = { 0.0f, 0.4f, 0.0f }, .eyeZ = 0.5f, .pos = { 0.25f, 0.0f, 0.5f })

PARAMS(QsNoBlock, QaOnce, .tail = 1, .target = 1)
PARAMS(QsNoBlockTwice, QaTwice, .tail = 1, .target = 1)

// Strike snapshots with presets
CALLS(QsFrames1, { .f = 0.015625f }, { .f = 0.015625f }, { .f = 0.015625f }, { .f = 0.015625f })
CALLS(QsFrames2, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f }, { .f = 0.03125f })
CALLS(QsFrames8, { .f = 0.125f }, { .f = 0.125f }, { .f = 0.125f }, { .f = 0.125f }, { .f = 0.125f }, { .f = 0.125f }, { .f = 0.125f }, { .f = 0.125f })
CALLS(QsFrames4s, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f })
CALLS(QsBigStep, { .f = 0.375f }, { .f = 0.375f }, { .f = 0.375f })
CALLS(QsHuge, { .f = 100.0f })
CALLS(QsExact, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.0625f })
CALLS(QsZero, { .f = 0.0f }, { .f = 0.0f })
CALLS(QsStartThenStep, { .f = 0.0f }, { .f = 0.0625f }, { .f = 0.0625f }, { .f = 0.125f }, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.5f })

PARAMS(QsStrikeMode, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 1, .strikeNum = 6, .strikeTime = 0.0f, .snapStep = 0.0625f, .targetPos = { 0.0f, 0.4f, 0.0f })
PARAMS(QsStrikeSnap2, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 2, .strikeNum = 6, .strikeTime = 0.09375f, .snapStep = 0.0625f, .targetPos = { 1.0f, 0.4f, 0.0f })
PARAMS(QsStrikeSnap3, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 3, .strikeNum = 6, .strikeTime = 0.15625f, .snapStep = 0.0625f, .targetPos = { -1.0f, 0.4f, 0.0f })
PARAMS(QsStrikeSnap4, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 4, .strikeNum = 6, .strikeTime = 0.21875f, .snapStep = 0.0625f, .targetPos = { 0.0f, 2.0f, 1.0f })
PARAMS(QsStrikeSnap5Last, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 5, .strikeNum = 6, .strikeTime = 0.28125f, .snapStep = 0.0625f)
PARAMS(QsStrikeSnap5NotLast, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 5, .strikeNum = 9, .strikeTime = 0.28125f, .snapStep = 0.0625f)
PARAMS(QsStrikeSnap6Beyond, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 6, .strikeNum = 8, .strikeTime = 0.34375f, .snapStep = 0.0625f)
PARAMS(QsStrikeNoBlockFrames, QsFrames8, .tail = 1, .target = 1)
PARAMS(QsStrikeWrongMode, QsFrames1, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM2, .strikeSnap = 1, .strikeNum = 6, .strikeTime = 0.0f)
PARAMS(QsStrikeNoneMode, QsFrames1, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_NONE, .strikeSnap = 1, .strikeNum = 6, .strikeTime = 0.0f)
PARAMS(QsStrikeExact, QsExact, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 1, .strikeNum = 6, .strikeTime = 0.0f, .snapStep = 0.0625f, .targetPos = { 0.0f, 0.4f, 0.0f })
PARAMS(QsStrikeBig, QsBigStep, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 1, .strikeNum = 6, .strikeTime = 0.0f, .snapStep = 0.0625f, .targetPos = { 0.0f, 0.4f, 0.0f })
PARAMS(QsStrikeHuge, QsHuge, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 1, .strikeNum = 6, .strikeTime = 0.0f, .snapStep = 0.0625f, .targetPos = { 0.0f, 0.4f, 0.0f })
PARAMS(QsStrikeZeroStep, QsZero, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 1, .strikeNum = 6, .strikeTime = 0.0625f, .snapStep = 0.0625f, .targetPos = { 0.0f, 0.4f, 0.0f })

PARAMS(QsAnim2Mode, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM2, .strikeSnap = 1, .strikeNum = 4, .strikeTime = 0.0f, .snapStep = 0.0625f, .targetPos = { 1.0f, 2.0f, 0.5f })
PARAMS(QsAnim2Snap2, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM2, .strikeSnap = 2, .strikeNum = 4, .strikeTime = 0.09375f, .snapStep = 0.0625f, .targetPos = { -1.0f, 2.0f, 0.5f })
PARAMS(QsAnim2Last, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM2, .strikeSnap = 3, .strikeNum = 4, .strikeTime = 0.15625f, .snapStep = 0.0625f)
PARAMS(QsAnim2WrongMode, QsFrames1, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 1, .strikeNum = 4, .strikeTime = 0.0f)
PARAMS(QsAnim2NoBlock, QsFrames1, .tail = 1, .target = 1)
PARAMS(QsAnim2Big, QsBigStep, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM2, .strikeSnap = 1, .strikeNum = 4, .strikeTime = 0.0f, .snapStep = 0.0625f, .targetPos = { 1.0f, 2.0f, 0.5f })

CALLS(QsExact3, { .f = 0.375f }, { .f = 0.375f }, { .f = 0.375f })
CALLS(QsExact5, { .f = 0.25f }, { .f = 0.25f }, { .f = 0.25f })
CALLS(QsExact4, { .f = 1.0f }, { .f = 1.0f }, { .f = 1.0f })
PARAMS(QsAnim2Exact, QsExact, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM2, .strikeSnap = 1, .strikeNum = 4, .strikeTime = 0.0f, .snapStep = 0.0625f, .targetPos = { 1.0f, 2.0f, 0.5f })
PARAMS(QsAnim3Exact, QsExact3, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM3, .strikeSnap = 1, .strikeNum = 15, .strikeTime = 0.0f, .snapStep = 0.375f)
PARAMS(QsAnim5Exact, QsExact5, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM5, .strikeSnap = 1, .strikeNum = 10, .strikeTime = 0.0f, .snapStep = 0.25f)
PARAMS(QsAnim4Exact, QsExact4, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM4, .strikeSnap = 1, .strikeNum = 3, .strikeTime = 0.0f, .snapStep = 1.0f)
PARAMS(QsAnim3Mode, QsFrames4s, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM3, .strikeSnap = 1, .strikeNum = 15, .strikeTime = 0.0f, .snapStep = 0.375f)
PARAMS(QsAnim3Last, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM3, .strikeSnap = 14, .strikeNum = 15, .strikeTime = 5.0f, .snapStep = 0.375f)
PARAMS(QsAnim3Beyond, QsFrames1, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM3, .strikeSnap = 13, .strikeNum = 14, .strikeTime = 4.4f, .snapStep = 0.375f)
PARAMS(QsAnim3WrongMode, QsFrames1, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM5, .strikeSnap = 1, .strikeNum = 15, .strikeTime = 0.0f)
PARAMS(QsAnim3Huge, QsHuge, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM3, .strikeSnap = 1, .strikeNum = 15, .strikeTime = 0.0f, .snapStep = 0.375f)
PARAMS(QsAnim5Mode, QsFrames4s, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM5, .strikeSnap = 1, .strikeNum = 10, .strikeTime = 0.0f, .snapStep = 0.25f)
PARAMS(QsAnim5Last, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM5, .strikeSnap = 9, .strikeNum = 10, .strikeTime = 2.125f, .snapStep = 0.25f)
PARAMS(QsAnim5WrongMode, QsFrames1, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM3, .strikeSnap = 1, .strikeNum = 10, .strikeTime = 0.0f)
PARAMS(QsAnim5Huge, QsHuge, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM5, .strikeSnap = 1, .strikeNum = 10, .strikeTime = 0.0f, .snapStep = 0.25f)
PARAMS(QsAnim4Mode, QsFrames4s, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM4, .strikeSnap = 1, .strikeNum = 3, .strikeTime = 0.0f, .snapStep = 1.0f)
PARAMS(QsAnim4Last, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM4, .strikeSnap = 2, .strikeNum = 3, .strikeTime = 1.5f, .snapStep = 1.0f)
PARAMS(QsAnim4WrongMode, QsFrames1, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM2, .strikeSnap = 1, .strikeNum = 3, .strikeTime = 0.0f)
PARAMS(QsAnim4Huge, QsHuge, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM4, .strikeSnap = 1, .strikeNum = 3, .strikeTime = 0.0f, .snapStep = 1.0f)

// Dispatch of the tail update
PARAMS(QtNone, QsFrames1, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_NONE, .strikeSnap = 1, .strikeNum = 3)
PARAMS(QtStrike, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_STRIKE, .strikeSnap = 1, .strikeNum = 6, .snapStep = 0.0625f, .targetPos = { 0.0f, 0.4f, 0.0f })
PARAMS(QtAnim2, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM2, .strikeSnap = 1, .strikeNum = 4, .snapStep = 0.0625f, .targetPos = { 1.0f, 2.0f, 0.5f })
PARAMS(QtAnim3, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM3, .strikeSnap = 1, .strikeNum = 15, .snapStep = 0.375f)
PARAMS(QtAnim4, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM4, .strikeSnap = 1, .strikeNum = 3, .snapStep = 1.0f)
PARAMS(QtAnim5, QsFrames8, QZ_BASE, .quetzPreset = 1, .strikeMode = SITHQUETZSTRIKE_MODE_ANIM5, .strikeSnap = 1, .strikeNum = 10, .snapStep = 0.25f)
PARAMS(QtMode6, QsFrames1, QZ_BASE, .quetzPreset = 1, .strikeMode = 6, .strikeSnap = 1, .strikeNum = 3)
PARAMS(QtNoBlock, QsFrames1, .tail = 1, .target = 1)

// Start* then Update*: complete animations
PARAMS(QxStrike, QsStartThenStep, QZ_BASE, .targetPos = { 0.0f, 0.4f, 0.0f })
PARAMS(QxStrikeFar, QsFrames2, QZ_BASE, .targetPos = { 3.0f, 3.0f, 1.0f })
PARAMS(QxStrikeBigSteps, QsBigStep, QZ_BASE, .targetPos = { -1.0f, 0.4f, 0.5f })
PARAMS(QxStrikeNoBlock, QsStartThenStep, .tail = 1, .target = 1, .targetPos = { 0.0f, 0.4f, 0.0f })
PARAMS(QxAnim2, QsStartThenStep, QZ_BASE, .targetPos = { 1.0f, 2.0f, 0.5f })
PARAMS(QxAnim2Frames, QsFrames2, QZ_BASE, .targetPos = { -1.0f, 2.0f, 0.5f })
PARAMS(QxAnim2NoBlock, QsStartThenStep, .tail = 1, .target = 1)
PARAMS(QxAnim3, QsFrames4s, QZ_BASE)
PARAMS(QxAnim3Short, QsStartThenStep, QZ_BASE)
PARAMS(QxAnim3NoBlock, QsStartThenStep, .tail = 1)
PARAMS(QxAnim5, QsFrames4s, QZ_BASE)
PARAMS(QxAnim5Short, QsStartThenStep, QZ_BASE)
PARAMS(QxAnim5NoBlock, QsStartThenStep, .tail = 1)
PARAMS(QxAnim4, QsFrames4s, QZ_BASE)
PARAMS(QxAnim4Short, QsStartThenStep, QZ_BASE)
PARAMS(QxAnim4NoBlock, QsStartThenStep, .tail = 1)

// ---------------------------------------------------------------------------------------------------------------
// sithAIMove_UpdateMardukTail
// ---------------------------------------------------------------------------------------------------------------
CALLS(MkSteps, { .f = 0.0625f }, { .f = 0.125f }, { .f = 0.5f }, { .f = 1.0f }, { .f = 0.0f })
PARAMS(MkMarduk, MkSteps, .pName = "marduk", .jointIdx = 5)
PARAMS(MkMarduk2, MkSteps, .pName = "marduk2", .jointIdx = 6)
PARAMS(MkOther, MkSteps, .pName = "marduk3", .jointIdx = 5)
PARAMS(MkPrefix, MkSteps, .pName = "Marduk", .jointIdx = 5)
PARAMS(MkNoJoint, MkSteps, .pName = "marduk", .jointIdx = -1)
PARAMS(MkJointZero, MkSteps, .pName = "marduk2", .jointIdx = 0)
PARAMS(MkJointLast, MkSteps, .pName = "marduk", .jointIdx = 31)

// ---------------------------------------------------------------------------------------------------------------
// Case table
// ---------------------------------------------------------------------------------------------------------------
#define C(name, func, params) { name, sithAIMoveVec_Run, func, &sithAIMoveVec_p##params }

static const SithVecCase sithAIMoveVec_aCases[] = {
    // Update: the early outs, the turn variants (plain, boss, mine car, body tracking), alignment, moving, weapon and
    // head tracking branches and the tail updates
    C("Update_idle", SITHAIMOVEVEC_UPDATE, UpdIdle),
    C("Update_notactor", SITHAIMOVEVEC_UPDATE, UpdItem),
    C("Update_dead", SITHAIMOVEVEC_UPDATE, UpdDead),
    C("Update_edge_health_small", SITHAIMOVEVEC_UPDATE, UpdAliveTiny),
    C("Update_edge_health0", SITHAIMOVEVEC_UPDATE, UpdDeadZero),
    C("Update_sleeping", SITHAIMOVEVEC_UPDATE, UpdSleeping),
    C("Update_sleeping_block", SITHAIMOVEVEC_UPDATE, UpdSleepingBlock),
    C("Update_sleeping_noslopemove", SITHAIMOVEVEC_UPDATE, UpdSleepingNoSlope),
    C("Update_flag_disabled", SITHAIMOVEVEC_UPDATE, UpdFlagDisabled),
    C("Update_flag_dying", SITHAIMOVEVEC_UPDATE, UpdFlagDying),
    C("Update_flag_destroyed", SITHAIMOVEVEC_UPDATE, UpdFlagDestroyed),
    C("Update_flag_submerged", SITHAIMOVEVEC_UPDATE, UpdFlagSubmerged),
    C("Update_turn_right", SITHAIMOVEVEC_UPDATE, UpdTurnRight),
    C("Update_turn_behind", SITHAIMOVEVEC_UPDATE, UpdTurnLeftBehind),
    C("Update_turn_boss", SITHAIMOVEVEC_UPDATE, UpdTurnBoss),
    C("Update_turn_boss_lookup", SITHAIMOVEVEC_UPDATE, UpdTurnBossUp),
    C("Update_turn_minecar", SITHAIMOVEVEC_UPDATE, UpdTurnMineCar),
    C("Update_turn_minecar_nonphysics", SITHAIMOVEVEC_UPDATE, UpdTurnMineCarNotPhysics),
    C("Update_bodytrack_target", SITHAIMOVEVEC_UPDATE, UpdBodyTrackTarget),
    C("Update_bodytrack_turning", SITHAIMOVEVEC_UPDATE, UpdBodyTrackTurning),
    C("Update_bodytrack_turning_headtrack", SITHAIMOVEVEC_UPDATE, UpdBodyTrackTurningHeadEnabled),
    C("Update_bodytrack_notarget", SITHAIMOVEVEC_UPDATE, UpdBodyTrackNoTarget),
    C("Update_bodytrack_headtrack_wide", SITHAIMOVEVEC_UPDATE, UpdBodyTrackHeadEnabledFar),
    C("Update_bodytrack_headtrack_narrow", SITHAIMOVEVEC_UPDATE, UpdBodyTrackHeadEnabledNear),
    C("Update_bodytrack_headtrack_wide_negative", SITHAIMOVEVEC_UPDATE, UpdBodyTrackHeadEnabledNegFar),
    C("Update_edge_bodytrack_headtrack_20", SITHAIMOVEVEC_UPDATE, UpdBodyTrackHeadEnabledEdge),
    C("Update_moveforward_look", SITHAIMOVEVEC_UPDATE, UpdMoveForward),
    C("Update_moveforward_side", SITHAIMOVEVEC_UPDATE, UpdMoveForwardSide),
    C("Update_moveforward_notmoving", SITHAIMOVEVEC_UPDATE, UpdMoveForwardNotMoving),
    C("Update_moveforward_disabled", SITHAIMOVEVEC_UPDATE, UpdMoveForwardDisabled),
    C("Update_moveforward_disabled_editor", SITHAIMOVEVEC_UPDATE, UpdMoveForwardDisabledEditor),
    C("Update_align_backwards", SITHAIMOVEVEC_UPDATE, UpdMoveAlignBackwards),
    C("Update_align_forwards", SITHAIMOVEVEC_UPDATE, UpdMoveAlignForwards),
    C("Update_align_plain", SITHAIMOVEVEC_UPDATE, UpdMoveAlignPlain),
    C("Update_align_waypoints", SITHAIMOVEVEC_UPDATE, UpdMoveAlignWpnts),
    C("Update_move", SITHAIMOVEVEC_UPDATE, UpdMoveOnly),
    C("Update_move_and_turn", SITHAIMOVEVEC_UPDATE, UpdMovePlusTurn),
    C("Update_weapon_selected", SITHAIMOVEVEC_UPDATE, UpdWeaponSelected),
    C("Update_weapon_aiming", SITHAIMOVEVEC_UPDATE, UpdWeaponAiming),
    C("Update_aiming_noweapon", SITHAIMOVEVEC_UPDATE, UpdAimingNoWeapon),
    C("Update_look_target", SITHAIMOVEVEC_UPDATE, UpdLookTarget),
    C("Update_look_block", SITHAIMOVEVEC_UPDATE, UpdLookBlock),
    C("Update_look_block_headenabled", SITHAIMOVEVEC_UPDATE, UpdLookBlockHeadEnabled),
    C("Update_look_noslopemove_headenabled", SITHAIMOVEVEC_UPDATE, UpdLookNoSlopeHeadEnabled),
    C("Update_look_noslopemove", SITHAIMOVEVEC_UPDATE, UpdLookNoSlope),
    C("Update_headenabled_only", SITHAIMOVEVEC_UPDATE, UpdHeadEnabledOnly),
    C("Update_turning_block", SITHAIMOVEVEC_UPDATE, UpdTurningHeadBlock),
    C("Update_tail_quetz", SITHAIMOVEVEC_UPDATE, UpdQuetzTail),
    C("Update_tail_noblock", SITHAIMOVEVEC_UPDATE, UpdTailNoBlock),
    C("Update_attached_notail_marduk", SITHAIMOVEVEC_UPDATE, UpdAttachedNoTailFlag),
    C("Update_attached_notail", SITHAIMOVEVEC_UPDATE, UpdAttachedNoTailFlagOther),
    C("Update_marduk", SITHAIMOVEVEC_UPDATE, UpdMarduk),
    C("Update_marduk2", SITHAIMOVEVEC_UPDATE, UpdMarduk2),
    C("Update_marduk_prefix", SITHAIMOVEVEC_UPDATE, UpdMardukPrefix),
    C("Update_marduk_nojoint", SITHAIMOVEVEC_UPDATE, UpdMardukNoJoint),
    C("Update_tail_and_marduk", SITHAIMOVEVEC_UPDATE, UpdTailAndMarduk),

    // UpdateMineCar
    C("UpdateMineCar_immobile", SITHAIMOVEVEC_UPDATEMINECAR, McImmobile),
    C("UpdateMineCar_turn_right", SITHAIMOVEVEC_UPDATEMINECAR, McTurnRight),
    C("UpdateMineCar_turn_left", SITHAIMOVEVEC_UPDATEMINECAR, McTurnLeft),
    C("UpdateMineCar_turn_behind", SITHAIMOVEVEC_UPDATEMINECAR, McTurnBehind),
    C("UpdateMineCar_aligned", SITHAIMOVEVEC_UPDATEMINECAR, McAligned),
    C("UpdateMineCar_near", SITHAIMOVEVEC_UPDATEMINECAR, McNearGoal),
    C("UpdateMineCar_slowzone", SITHAIMOVEVEC_UPDATEMINECAR, McSlowZone),
    C("UpdateMineCar_small_angle", SITHAIMOVEVEC_UPDATEMINECAR, McSmall),
    C("UpdateMineCar_turn_45", SITHAIMOVEVEC_UPDATEMINECAR, McTurn45),
    C("UpdateMineCar_lookup", SITHAIMOVEVEC_UPDATEMINECAR, McLookUp),
    C("UpdateMineCar_lookdown_fly", SITHAIMOVEVEC_UPDATEMINECAR, McLookDownFly),
    C("UpdateMineCar_mesh0", SITHAIMOVEVEC_UPDATEMINECAR, McMeshIndex0),
    C("UpdateMineCar_status_rolling", SITHAIMOVEVEC_UPDATEMINECAR, McMoveStatusSet),
    C("UpdateMineCar_tilted", SITHAIMOVEVEC_UPDATEMINECAR, McTilted),
    C("UpdateMineCar_random_tweak_right", SITHAIMOVEVEC_UPDATEMINECAR, McRandomTweakRight),
    C("UpdateMineCar_random_tweak_left", SITHAIMOVEVEC_UPDATEMINECAR, McRandomTweakLeft),
    C("UpdateMineCar_edge_limit_equal", SITHAIMOVEVEC_UPDATEMINECAR, McLimitEqual),
    C("UpdateMineCar_edge_limit_below", SITHAIMOVEVEC_UPDATEMINECAR, McLimitBelow),
    C("UpdateMineCar_edge_limit_above", SITHAIMOVEVEC_UPDATEMINECAR, McLimitAbove),
    C("UpdateMineCar_edge_limit_equal_behind", SITHAIMOVEVEC_UPDATEMINECAR, McLimitEqualBehind),
    C("UpdateMineCar_zero_dt", SITHAIMOVEVEC_UPDATEMINECAR, McZeroDt),
    C("UpdateMineCar_zero_dt_behind", SITHAIMOVEVEC_UPDATEMINECAR, McZeroDtBehind),
    C("UpdateMineCar_negative_rate", SITHAIMOVEVEC_UPDATEMINECAR, McNegativeRate),

    // UpdateTurn
    C("UpdateTurn_right", SITHAIMOVEVEC_UPDATETURN, UtRight),
    C("UpdateTurn_left", SITHAIMOVEVEC_UPDATETURN, UtLeft),
    C("UpdateTurn_right_30", SITHAIMOVEVEC_UPDATETURN, UtRight30),
    C("UpdateTurn_left_30", SITHAIMOVEVEC_UPDATETURN, UtLeft30),
    C("UpdateTurn_right_50", SITHAIMOVEVEC_UPDATETURN, UtRight50),
    C("UpdateTurn_left_50", SITHAIMOVEVEC_UPDATETURN, UtLeft50),
    C("UpdateTurn_behind", SITHAIMOVEVEC_UPDATETURN, UtBehind),
    C("UpdateTurn_diagonal", SITHAIMOVEVEC_UPDATETURN, UtDiag),
    C("UpdateTurn_diagonal_left", SITHAIMOVEVEC_UPDATETURN, UtDiagLeft),
    C("UpdateTurn_slowzone", SITHAIMOVEVEC_UPDATETURN, UtSlowZone),
    C("UpdateTurn_slowzone_negative", SITHAIMOVEVEC_UPDATETURN, UtSlowZoneNeg),
    C("UpdateTurn_below_slowangle", SITHAIMOVEVEC_UPDATETURN, UtBelowSlowAngle),
    C("UpdateTurn_aligned", SITHAIMOVEVEC_UPDATETURN, UtAligned),
    C("UpdateTurn_immobile", SITHAIMOVEVEC_UPDATETURN, UtImmobile),
    C("UpdateTurn_stationary_mounting", SITHAIMOVEVEC_UPDATETURN, UtStationary),
    C("UpdateTurn_stationary_turning90", SITHAIMOVEVEC_UPDATETURN, UtStationaryTurn90),
    C("UpdateTurn_animated_rolling", SITHAIMOVEVEC_UPDATETURN, UtAnimated),
    C("UpdateTurn_animated_snap", SITHAIMOVEVEC_UPDATETURN, UtAnimatedSnap),
    C("UpdateTurn_status_kept_when_done", SITHAIMOVEVEC_UPDATETURN, UtStatusKept),
    C("UpdateTurn_edge_updot_equal", SITHAIMOVEVEC_UPDATETURN, UtUpDotEqual),
    C("UpdateTurn_edge_updot_above", SITHAIMOVEVEC_UPDATETURN, UtUpDotAbove),
    C("UpdateTurn_edge_limit_equal", SITHAIMOVEVEC_UPDATETURN, UtLimitEqual),
    C("UpdateTurn_edge_limit_below", SITHAIMOVEVEC_UPDATETURN, UtLimitBelow),
    C("UpdateTurn_edge_limit_above", SITHAIMOVEVEC_UPDATETURN, UtLimitAbove),
    C("UpdateTurn_edge_limit_equal_left", SITHAIMOVEVEC_UPDATETURN, UtLimitEqualLeft),
    C("UpdateTurn_edge_limit_equal_behind", SITHAIMOVEVEC_UPDATETURN, UtLimitEqualBehind),
    C("UpdateTurn_negative_rate", SITHAIMOVEVEC_UPDATETURN, UtNegativeRate),
    C("UpdateTurn_edge_angle45_right", SITHAIMOVEVEC_UPDATETURN, UtAngle45Right),
    C("UpdateTurn_edge_angle45_left", SITHAIMOVEVEC_UPDATETURN, UtAngle45Left),
    C("UpdateTurn_lookup", SITHAIMOVEVEC_UPDATETURN, UtLookUp),
    C("UpdateTurn_lookdown", SITHAIMOVEVEC_UPDATETURN, UtLookDown),
    C("UpdateTurn_lookup_fly", SITHAIMOVEVEC_UPDATETURN, UtLookUpFly),
    C("UpdateTurn_edge_steep", SITHAIMOVEVEC_UPDATETURN, UtSteep),
    C("UpdateTurn_edge_steep_below", SITHAIMOVEVEC_UPDATETURN, UtSteepBelow),
    C("UpdateTurn_tilted", SITHAIMOVEVEC_UPDATETURN, UtTilted),
    C("UpdateTurn_fast", SITHAIMOVEVEC_UPDATETURN, UtFastTurn),
    C("UpdateTurn_special_30", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn30),
    C("UpdateTurn_special_20", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn20),
    C("UpdateTurn_special_60", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn60),
    C("UpdateTurn_special_90", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn90),
    C("UpdateTurn_special_90_left", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn90Left),
    C("UpdateTurn_special_90_nokeyframe", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn90NoKf),
    C("UpdateTurn_special_135", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn135),
    C("UpdateTurn_special_135_nokeyframe", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn135NoKf),
    C("UpdateTurn_special_180", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn180),
    C("UpdateTurn_special_180_nokeyframe", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurn180NoKf),
    C("UpdateTurn_special_animated", SITHAIMOVEVEC_UPDATETURN, UtSpecialTurnAnimated),

    // UpdateMove
    C("UpdateMove_walk", SITHAIMOVEVEC_UPDATEMOVE, UmWalk),
    C("UpdateMove_walk_side", SITHAIMOVEVEC_UPDATEMOVE, UmWalkSide),
    C("UpdateMove_walk_slow", SITHAIMOVEVEC_UPDATEMOVE, UmWalkSlow),
    C("UpdateMove_walk_near", SITHAIMOVEVEC_UPDATEMOVE, UmWalkNear),
    C("UpdateMove_walk_verynear", SITHAIMOVEVEC_UPDATEMOVE, UmWalkVeryNear),
    C("UpdateMove_walk_onspot", SITHAIMOVEVEC_UPDATEMOVE, UmWalkOnSpot),
    C("UpdateMove_walk_velocity", SITHAIMOVEVEC_UPDATEMOVE, UmWalkVelocity),
    C("UpdateMove_walk_tinyvelocity", SITHAIMOVEVEC_UPDATEMOVE, UmWalkVelocitySmall),
    C("UpdateMove_flag_disabled", SITHAIMOVEVEC_UPDATEMOVE, UmDisabledFlag),
    C("UpdateMove_flag_dying", SITHAIMOVEVEC_UPDATEMOVE, UmDyingFlag),
    C("UpdateMove_flag_destroyed", SITHAIMOVEVEC_UPDATEMOVE, UmDestroyedFlag),
    C("UpdateMove_immobile", SITHAIMOVEVEC_UPDATEMOVE, UmImmobile),
    C("UpdateMove_paused", SITHAIMOVEVEC_UPDATEMOVE, UmPaused),
    C("UpdateMove_edge_pause_over", SITHAIMOVEVEC_UPDATEMOVE, UmPauseOver),
    C("UpdateMove_clears_newmove_bits", SITHAIMOVEVEC_UPDATEMOVE, UmNewMoveBits),
    C("UpdateMove_moveforward", SITHAIMOVEVEC_UPDATEMOVE, UmMoveForward),
    C("UpdateMove_moveforward_diagonal", SITHAIMOVEVEC_UPDATEMOVE, UmMoveForwardDiag),
    C("UpdateMove_moveforward_turned", SITHAIMOVEVEC_UPDATEMOVE, UmMoveForwardTurned),
    C("UpdateMove_moveforward_behind", SITHAIMOVEVEC_UPDATEMOVE, UmMoveForwardBehind),
    C("UpdateMove_moveforward_sidegoal", SITHAIMOVEVEC_UPDATEMOVE, UmMoveForwardSideGoal),
    C("UpdateMove_moveforward_sidegoal_left", SITHAIMOVEVEC_UPDATEMOVE, UmMoveForwardSideGoalLeft),
    C("UpdateMove_moveforward_sidegoal_far", SITHAIMOVEVEC_UPDATEMOVE, UmMoveForwardSideGoalFar),
    C("UpdateMove_matchvelocity", SITHAIMOVEVEC_UPDATEMOVE, UmMatchVelocity),
    C("UpdateMove_matchvelocity_nogoal", SITHAIMOVEVEC_UPDATEMOVE, UmMatchVelocityNoGoal),
    C("UpdateMove_matchvelocity_zeroown", SITHAIMOVEVEC_UPDATEMOVE, UmMatchVelocityZeroOwn),
    C("UpdateMove_stationary", SITHAIMOVEVEC_UPDATEMOVE, UmStationary),
    C("UpdateMove_animated", SITHAIMOVEVEC_UPDATEMOVE, UmAnimatedStatus),
    C("UpdateMove_nocheckforcliff", SITHAIMOVEVEC_UPDATEMOVE, UmNoCheckCliff),
    C("UpdateMove_cliff_none", SITHAIMOVEVEC_UPDATEMOVE, UmCliffNone),
    C("UpdateMove_cliff_blocked", SITHAIMOVEVEC_UPDATEMOVE, UmCliffBlocked),
    C("UpdateMove_wall", SITHAIMOVEVEC_UPDATEMOVE, UmWall),
    C("UpdateMove_wall_crawler", SITHAIMOVEVEC_UPDATEMOVE, UmWallCrawlInstinct),
    C("UpdateMove_thing_blocks", SITHAIMOVEVEC_UPDATEMOVE, UmThingBlock),
    C("UpdateMove_thing_step", SITHAIMOVEVEC_UPDATEMOVE, UmThingStep),
    C("UpdateMove_floor_unknown", SITHAIMOVEVEC_UPDATEMOVE, UmFloorUnknown),
    C("UpdateMove_floor_sequence", SITHAIMOVEVEC_UPDATEMOVE, UmFloorSequence),
    C("UpdateMove_floor_unknown_block", SITHAIMOVEVEC_UPDATEMOVE, UmUnreachableBlock),
    C("UpdateMove_notattached", SITHAIMOVEVEC_UPDATEMOVE, UmNotAttached),
    C("UpdateMove_fly", SITHAIMOVEVEC_UPDATEMOVE, UmFly),
    C("UpdateMove_fly_noactorenter", SITHAIMOVEVEC_UPDATEMOVE, UmFlyNoSector),
    C("UpdateMove_fly_noactorenter_block", SITHAIMOVEVEC_UPDATEMOVE, UmFlyNoSectorBlock),
    C("UpdateMove_fly_underwater", SITHAIMOVEVEC_UPDATEMOVE, UmFlyUnderwater),
    C("UpdateMove_fly_submerged", SITHAIMOVEVEC_UPDATEMOVE, UmFlySubmerged),
    C("UpdateMove_fly_breathe", SITHAIMOVEVEC_UPDATEMOVE, UmFlyBreathe),
    C("UpdateMove_fly_breathe_submerged", SITHAIMOVEVEC_UPDATEMOVE, UmFlyBreatheSubmerged),
    C("UpdateMove_swim_airdestroyed", SITHAIMOVEVEC_UPDATEMOVE, UmSwimAirDestroyed),
    C("UpdateMove_swim_submerged", SITHAIMOVEVEC_UPDATEMOVE, UmSwimSubmerged),
    C("UpdateMove_swim_dry", SITHAIMOVEVEC_UPDATEMOVE, UmSwimDry),
    C("UpdateMove_swim_breathe", SITHAIMOVEVEC_UPDATEMOVE, UmSwimBreathe),
    C("UpdateMove_nomovecheck", SITHAIMOVEVEC_UPDATEMOVE, UmNoMoveAtAll),
    C("UpdateMove_goal_reached", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReached),
    C("UpdateMove_goal_reached_block", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedBlock),
    C("UpdateMove_goal_reached_continuous", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedContinuous),
    C("UpdateMove_goal_reached_newmove", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedNewMove),
    C("UpdateMove_goal_reached_animated", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedAnimated),
    C("UpdateMove_goal_reached_stationary", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedStationary),
    C("UpdateMove_goal_reached_waypoints", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedWpnts),
    C("UpdateMove_goal_reached_waypoints_end", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedWpntsEnd),
    C("UpdateMove_goal_reached_saved", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedSaved),
    C("UpdateMove_goal_reached_sweep", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedSweep),
    C("UpdateMove_goal_reached_sweep_active", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedSweepActive),
    C("UpdateMove_goal_reached_sweep_fails", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedSweepFails),
    C("UpdateMove_goal_reached_sweep_waypoints", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedSweepWpnts),
    C("UpdateMove_goalset_event", SITHAIMOVEVEC_UPDATEMOVE, UmGoalSetEvent),
    C("UpdateMove_edge_goalset_event_equal", SITHAIMOVEVEC_UPDATEMOVE, UmGoalSetEventEdge),
    C("UpdateMove_edge_goalset_event_just_far", SITHAIMOVEVEC_UPDATEMOVE, UmGoalSetEventJustFar),
    C("UpdateMove_edge_goal_reached_equal", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedEdge),
    C("UpdateMove_edge_goal_reached_just_far", SITHAIMOVEVEC_UPDATEMOVE, UmGoalReachedJustFar),
    C("UpdateMove_underwater_tolerance", SITHAIMOVEVEC_UPDATEMOVE, UmUnderwaterTolerance),
    C("UpdateMove_wallcrawl", SITHAIMOVEVEC_UPDATEMOVE, UmWallCrawl),
    C("UpdateMove_wallcrawl_close", SITHAIMOVEVEC_UPDATEMOVE, UmWallCrawlClose),

    // CheckMoveAlign
    C("CheckMoveAlign_plain", SITHAIMOVEVEC_CHECKMOVEALIGN, CmaPlain),
    C("CheckMoveAlign_nobackwards_facing", SITHAIMOVEVEC_CHECKMOVEALIGN, CmaNoBackFacing),
    C("CheckMoveAlign_nobackwards_behind", SITHAIMOVEVEC_CHECKMOVEALIGN, CmaNoBackBehind),
    C("CheckMoveAlign_nobackwards_side", SITHAIMOVEVEC_CHECKMOVEALIGN, CmaNoBackSide),
    C("CheckMoveAlign_nobackwards_slightlybehind", SITHAIMOVEVEC_CHECKMOVEALIGN, CmaNoBackSlightlyBehind),
    C("CheckMoveAlign_notaligning", SITHAIMOVEVEC_CHECKMOVEALIGN, CmaNoAligning),
    C("CheckMoveAlign_keeps_other_bits", SITHAIMOVEVEC_CHECKMOVEALIGN, CmaOtherBits),

    // GetGoalTolerance
    C("GetGoalTolerance_plain", SITHAIMOVEVEC_GETGOALTOLERANCE, GtPlain),
    C("GetGoalTolerance_plain_samepos", SITHAIMOVEVEC_GETGOALTOLERANCE, GtPlainSame),
    C("GetGoalTolerance_plain_bigmovesize", SITHAIMOVEVEC_GETGOALTOLERANCE, GtPlainBigMove),
    C("GetGoalTolerance_plain_smallmovesize", SITHAIMOVEVEC_GETGOALTOLERANCE, GtPlainSmallMove),
    C("GetGoalTolerance_plain_ownerz", SITHAIMOVEVEC_GETGOALTOLERANCE, GtPlainZPos),
    C("GetGoalTolerance_underwater", SITHAIMOVEVEC_GETGOALTOLERANCE, GtUnderwater),
    C("GetGoalTolerance_underwater_big", SITHAIMOVEVEC_GETGOALTOLERANCE, GtUnderwaterBig),
    C("GetGoalTolerance_underwater_small", SITHAIMOVEVEC_GETGOALTOLERANCE, GtUnderwaterSmall),
    C("GetGoalTolerance_underwater_tiny", SITHAIMOVEVEC_GETGOALTOLERANCE, GtUnderwaterTiny),
    C("GetGoalTolerance_fly_tiny", SITHAIMOVEVEC_GETGOALTOLERANCE, GtFlyTiny),
    C("GetGoalTolerance_fly", SITHAIMOVEVEC_GETGOALTOLERANCE, GtFly),
    C("GetGoalTolerance_fly_big", SITHAIMOVEVEC_GETGOALTOLERANCE, GtFlyBig),
    C("GetGoalTolerance_othersector_dry", SITHAIMOVEVEC_GETGOALTOLERANCE, GtSectorOnly),
    C("GetGoalTolerance_othersector_wet", SITHAIMOVEVEC_GETGOALTOLERANCE, GtSectorUnderwaterOwner),
    C("GetGoalTolerance_moveforward_side", SITHAIMOVEVEC_GETGOALTOLERANCE, GtMoveForward),
    C("GetGoalTolerance_moveforward_side_left", SITHAIMOVEVEC_GETGOALTOLERANCE, GtMoveForwardLeft),
    C("GetGoalTolerance_moveforward_far", SITHAIMOVEVEC_GETGOALTOLERANCE, GtMoveForwardFar),
    C("GetGoalTolerance_moveforward_edge_right", SITHAIMOVEVEC_GETGOALTOLERANCE, GtMoveForwardEdge),
    C("GetGoalTolerance_moveforward_edge_ahead", SITHAIMOVEVEC_GETGOALTOLERANCE, GtMoveForwardEdge2),
    C("GetGoalTolerance_moveforward_turned", SITHAIMOVEVEC_GETGOALTOLERANCE, GtMoveForwardTurned),
    C("GetGoalTolerance_wallcrawl_mounting", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallMounting),
    C("GetGoalTolerance_wallcrawl_nosurface", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallNoSurface),
    C("GetGoalTolerance_wallcrawl_aligned2", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallAligned2),
    C("GetGoalTolerance_wallcrawl_aligned1", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallAligned1),
    C("GetGoalTolerance_wallcrawl_unreachable", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallFarUnreachable),
    C("GetGoalTolerance_wallcrawl_unreachable_block", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallFarUnreachableBlock),
    C("GetGoalTolerance_wallcrawl_axisclose", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallAxisClose),
    C("GetGoalTolerance_wallcrawl_surface_axis", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallAxisFarSurface),
    C("GetGoalTolerance_wallcrawl_surface_parallel", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfParallel),
    C("GetGoalTolerance_wallcrawl_surface_parallel_block", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfParallelBlock),
    C("GetGoalTolerance_wallcrawl_surface_dot9965", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfDot9965),
    C("GetGoalTolerance_wallcrawl_surface_dot9955", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfDot9955),
    C("GetGoalTolerance_wallcrawl_surface_dot_negative", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfDotNeg),
    C("GetGoalTolerance_wallcrawl_surface_near", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfNear),
    C("GetGoalTolerance_wallcrawl_surface_parallel_y", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfParallelY),
    C("GetGoalTolerance_wallcrawl_surface_parallel_z", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfParallelZ),
    C("GetGoalTolerance_wallcrawl_surface_parallel_none", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfParallelNone),
    C("GetGoalTolerance_wallcrawl_nosurface_y", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallNoSurfY),
    C("GetGoalTolerance_wallcrawl_nosurface_z", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallNoSurfZ),
    C("GetGoalTolerance_wallcrawl_nosurface_none", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallNoSurfNone),
    C("GetGoalTolerance_wallcrawl_edge_xy_equal", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallEqXY),
    C("GetGoalTolerance_wallcrawl_edge_xz_equal", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallEqXZ),
    C("GetGoalTolerance_wallcrawl_edge_yz_equal", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallEqYZ),
    C("GetGoalTolerance_wallcrawl_edge_x_equal", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallEqX),
    C("GetGoalTolerance_wallcrawl_edge_distance_equal", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallDist5Equal),
    C("GetGoalTolerance_wallcrawl_edge_distance_below", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallDist5Below),
    C("GetGoalTolerance_moveforward_edge_left", SITHAIMOVEVEC_GETGOALTOLERANCE, GtMoveForwardEdgeLeft),
    C("GetGoalTolerance_wallcrawl_edge_dot_equal", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallDotEqual),
    C("GetGoalTolerance_wallcrawl_edge_dot_below", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallDotBelow),
    C("GetGoalTolerance_wallcrawl_surface_along", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfaceAlong),
    C("GetGoalTolerance_wallcrawl_surface_perpendicular", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfacePerp),
    C("GetGoalTolerance_wallcrawl_surface_dot_near996", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfaceNear996),
    C("GetGoalTolerance_wallcrawl_surface_dot_far996", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfaceFar996),
    C("GetGoalTolerance_wallcrawl_surface_flat", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfaceFlat),
    C("GetGoalTolerance_wallcrawl_surface_samepos", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfaceSame),
    C("GetGoalTolerance_wallcrawl_surface_negnormal", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfaceNegNormal),
    C("GetGoalTolerance_wallcrawl_surface_bigmovesize", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSurfaceBigMove),
    C("GetGoalTolerance_wallcrawl_nosurface_bigmovesize", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallBigMoveNoSurface),
    C("GetGoalTolerance_wallcrawl_smallmovesize", SITHAIMOVEVEC_GETGOALTOLERANCE, GtWallSmallMove),

    // CheckWalkMove, CheckFlyMove, CheckSwimMove
    C("CheckWalkMove_ok_evenframe", SITHAIMOVEVEC_CHECKWALKMOVE, CwOkEven),
    C("CheckWalkMove_ok_oddframe", SITHAIMOVEVEC_CHECKWALKMOVE, CwOkOdd),
    C("CheckWalkMove_wallcrawl_odd", SITHAIMOVEVEC_CHECKWALKMOVE, CwWallCrawlOdd),
    C("CheckWalkMove_wallcrawl_odd_smallmove", SITHAIMOVEVEC_CHECKWALKMOVE, CwWallCrawlSmallMove),
    C("CheckWalkMove_wallcrawl_odd_movesize003", SITHAIMOVEVEC_CHECKWALKMOVE, CwWallCrawlMove003),
    C("CheckWalkMove_wallcrawl_odd_movesize003_above", SITHAIMOVEVEC_CHECKWALKMOVE, CwWallCrawlMove003b),
    C("CheckWalkMove_wallcrawl_even", SITHAIMOVEVEC_CHECKWALKMOVE, CwWallCrawlEven),
    C("CheckWalkMove_blocked", SITHAIMOVEVEC_CHECKWALKMOVE, CwBlocked),
    C("CheckWalkMove_cliff", SITHAIMOVEVEC_CHECKWALKMOVE, CwNone),
    C("CheckWalkMove_wall", SITHAIMOVEVEC_CHECKWALKMOVE, CwWall),
    C("CheckWalkMove_wall_crawler", SITHAIMOVEVEC_CHECKWALKMOVE, CwWallCrawler),
    C("CheckWalkMove_thing", SITHAIMOVEVEC_CHECKWALKMOVE, CwThing),
    C("CheckWalkMove_thing_allowstep", SITHAIMOVEVEC_CHECKWALKMOVE, CwThingStep),
    C("CheckWalkMove_unknown3", SITHAIMOVEVEC_CHECKWALKMOVE, CwUnknown3),
    C("CheckWalkMove_unknown16", SITHAIMOVEVEC_CHECKWALKMOVE, CwUnknown16),
    C("CheckWalkMove_unknown_block", SITHAIMOVEVEC_CHECKWALKMOVE, CwUnknownBlock),
    C("CheckWalkMove_sequence", SITHAIMOVEVEC_CHECKWALKMOVE, CwSequence),
    C("CheckFlyMove_submerged", SITHAIMOVEVEC_CHECKFLYMOVE, CfSubmerged),
    C("CheckFlyMove_submerged_continuous", SITHAIMOVEVEC_CHECKFLYMOVE, CfSubmergedContinuous),
    C("CheckFlyMove_clear", SITHAIMOVEVEC_CHECKFLYMOVE, CfClear),
    C("CheckFlyMove_nosector", SITHAIMOVEVEC_CHECKFLYMOVE, CfNoSector),
    C("CheckFlyMove_noactorenter", SITHAIMOVEVEC_CHECKFLYMOVE, CfNoActorEnter),
    C("CheckFlyMove_noactorenter_block", SITHAIMOVEVEC_CHECKFLYMOVE, CfNoActorEnterBlock),
    C("CheckFlyMove_underwater", SITHAIMOVEVEC_CHECKFLYMOVE, CfUnderwater),
    C("CheckFlyMove_underwater_noactorenter", SITHAIMOVEVEC_CHECKFLYMOVE, CfUnderwaterBoth),
    C("CheckFlyMove_samesector_underwater", SITHAIMOVEVEC_CHECKFLYMOVE, CfSameSectorUnderwater),
    C("CheckSwimMove_clear", SITHAIMOVEVEC_CHECKSWIMMOVE, CsClear),
    C("CheckSwimMove_dry", SITHAIMOVEVEC_CHECKSWIMMOVE, CsDry),
    C("CheckSwimMove_nosector", SITHAIMOVEVEC_CHECKSWIMMOVE, CsNoSector),
    C("CheckSwimMove_noactorenter", SITHAIMOVEVEC_CHECKSWIMMOVE, CsNoActorEnter),
    C("CheckSwimMove_noactorenter_block", SITHAIMOVEVEC_CHECKSWIMMOVE, CsNoActorEnterBlock),
    C("CheckSwimMove_rising_wet", SITHAIMOVEVEC_CHECKSWIMMOVE, CsRisingSurfaceWet),
    C("CheckSwimMove_rising_surface", SITHAIMOVEVEC_CHECKSWIMMOVE, CsRisingSurfaceDry),
    C("CheckSwimMove_rising_surface_lowtarget", SITHAIMOVEVEC_CHECKSWIMMOVE, CsRisingSurfaceDryLowTarget),
    C("CheckSwimMove_rising_surface_nearsurface", SITHAIMOVEVEC_CHECKSWIMMOVE, CsRisingSurfaceDryNear),
    C("CheckSwimMove_rising_wet_next", SITHAIMOVEVEC_CHECKSWIMMOVE, CsRisingSurfaceWetNext),
    C("CheckSwimMove_rising_surface_sameheight", SITHAIMOVEVEC_CHECKSWIMMOVE, CsRisingSameHeight),
    C("CheckSwimMove_sinking", SITHAIMOVEVEC_CHECKSWIMMOVE, CsSinking),
    C("CheckSwimMove_noupdraft", SITHAIMOVEVEC_CHECKSWIMMOVE, CsNoUpdraft),
    C("CheckSwimMove_nosector_rising", SITHAIMOVEVEC_CHECKSWIMMOVE, CsNoSectorRising),

    // AIGetMoveState, AISpecialTurn
    C("AIGetMoveState_all", SITHAIMOVEVEC_GETMOVESTATE, MsPlain),
    C("AIGetMoveState_all_modeflags", SITHAIMOVEVEC_GETMOVESTATE, MsFlags),
    C("AIGetMoveState_edge_outofrange", SITHAIMOVEVEC_GETMOVESTATE, MsOutOfRange),
    C("AISpecialTurn_off", SITHAIMOVEVEC_SPECIALTURN, StNoSpecial),
    C("AISpecialTurn_all_keyframes", SITHAIMOVEVEC_SPECIALTURN, StAllKf),
    C("AISpecialTurn_no_keyframes", SITHAIMOVEVEC_SPECIALTURN, StNoKf),
    C("AISpecialTurn_180_only", SITHAIMOVEVEC_SPECIALTURN, St180Only),
    C("AISpecialTurn_135_only", SITHAIMOVEVEC_SPECIALTURN, St135Only),
    C("AISpecialTurn_135_180", SITHAIMOVEVEC_SPECIALTURN, St135And180),
    C("AISpecialTurn_90_only", SITHAIMOVEVEC_SPECIALTURN, St90Only),
    C("AISpecialTurn_45_only", SITHAIMOVEVEC_SPECIALTURN, St45Only),
    C("AISpecialTurn_45_oneside", SITHAIMOVEVEC_SPECIALTURN, St45OneSide),
    C("AISpecialTurn_busy_animated", SITHAIMOVEVEC_SPECIALTURN, StBusy),
    C("AISpecialTurn_busy_stationary", SITHAIMOVEVEC_SPECIALTURN, StBusyStationary),

    // UpdateAIMove and GetAIMoveModes
    C("UpdateAIMove_still_walking", SITHAIMOVEVEC_UPDATEAIMOVE, UaStill),
    C("UpdateAIMove_still_standing", SITHAIMOVEVEC_UPDATEAIMOVE, UaStillStanding),
    C("UpdateAIMove_walk_forward", SITHAIMOVEVEC_UPDATEAIMOVE, UaWalkFwd),
    C("UpdateAIMove_walk_forward_already", SITHAIMOVEVEC_UPDATEAIMOVE, UaWalkFwdAlready),
    C("UpdateAIMove_run_forward", SITHAIMOVEVEC_UPDATEAIMOVE, UaRunFwd),
    C("UpdateAIMove_walk_back", SITHAIMOVEVEC_UPDATEAIMOVE, UaWalkBack),
    C("UpdateAIMove_strafe", SITHAIMOVEVEC_UPDATEAIMOVE, UaStrafe),
    C("UpdateAIMove_strafe_slow", SITHAIMOVEVEC_UPDATEAIMOVE, UaStrafeSlow),
    C("UpdateAIMove_fall", SITHAIMOVEVEC_UPDATEAIMOVE, UaFall),
    C("UpdateAIMove_fall_attached", SITHAIMOVEVEC_UPDATEAIMOVE, UaFallAttached),
    C("UpdateAIMove_land", SITHAIMOVEVEC_UPDATEAIMOVE, UaLand),
    C("UpdateAIMove_land_nokeyframe", SITHAIMOVEVEC_UPDATEAIMOVE, UaLandNoKf),
    C("UpdateAIMove_fly_up", SITHAIMOVEVEC_UPDATEAIMOVE, UaFlyUp),
    C("UpdateAIMove_fly_down", SITHAIMOVEVEC_UPDATEAIMOVE, UaFlyDown),
    C("UpdateAIMove_fly_slowup", SITHAIMOVEVEC_UPDATEAIMOVE, UaFlySlowUp),
    C("UpdateAIMove_edge_fly_threshold", SITHAIMOVEVEC_UPDATEAIMOVE, UaFlyThreshold),
    C("UpdateAIMove_fly_above_threshold", SITHAIMOVEVEC_UPDATEAIMOVE, UaFlyAbove),
    C("UpdateAIMove_fly_forward", SITHAIMOVEVEC_UPDATEAIMOVE, UaFlyForward),
    C("UpdateAIMove_fly_down_threshold", SITHAIMOVEVEC_UPDATEAIMOVE, UaFlyNegThreshold),
    C("UpdateAIMove_turning", SITHAIMOVEVEC_UPDATEAIMOVE, UaTurning),
    C("UpdateAIMove_turning_moving", SITHAIMOVEVEC_UPDATEAIMOVE, UaTurningMoving),
    C("UpdateAIMove_immobile", SITHAIMOVEVEC_UPDATEAIMOVE, UaImmobile),
    C("UpdateAIMove_nosector", SITHAIMOVEVEC_UPDATEAIMOVE, UaNoSector),
    C("UpdateAIMove_state_animated", SITHAIMOVEVEC_UPDATEAIMOVE, UaStateAnimated),
    C("UpdateAIMove_state_stationary", SITHAIMOVEVEC_UPDATEAIMOVE, UaStateStationary),
    C("UpdateAIMove_rotated", SITHAIMOVEVEC_UPDATEAIMOVE, UaRotatedOwner),
    C("UpdateAIMove_rotated_forward", SITHAIMOVEVEC_UPDATEAIMOVE, UaRotatedOwnerForward),
    C("UpdateAIMove_diagonal", SITHAIMOVEVEC_UPDATEAIMOVE, UaDiagonal),
    C("UpdateAIMove_diagonal_z", SITHAIMOVEVEC_UPDATEAIMOVE, UaDiagonalZ),
    C("UpdateAIMove_controls_disabled", SITHAIMOVEVEC_UPDATEAIMOVE, UaControlsDisabled),
    C("UpdateAIMove_forcemoveplay", SITHAIMOVEVEC_UPDATEAIMOVE, UaForceMovePlay),
    C("UpdateAIMove_tie_yx_negative", SITHAIMOVEVEC_UPDATEAIMOVE, UaTieYXNeg),
    C("UpdateAIMove_tie_zy_negative", SITHAIMOVEVEC_UPDATEAIMOVE, UaTieZYNeg),
    C("UpdateAIMove_tie_zx_negative", SITHAIMOVEVEC_UPDATEAIMOVE, UaTieZXNeg),
    C("UpdateAIMove_tie_zx_negative_attached", SITHAIMOVEVEC_UPDATEAIMOVE, UaTieZXNegAttached),
    C("UpdateAIMove_tie_zy", SITHAIMOVEVEC_UPDATEAIMOVE, UaTieZY),
    C("UpdateAIMove_tie_zx", SITHAIMOVEVEC_UPDATEAIMOVE, UaTieZX),
    C("UpdateAIMove_tie_zy_fly", SITHAIMOVEVEC_UPDATEAIMOVE, UaTieZYFly),
    C("UpdateAIMove_tie_zx_fly", SITHAIMOVEVEC_UPDATEAIMOVE, UaTieZXFly),
    C("UpdateAIMove_tie_all_fly", SITHAIMOVEVEC_UPDATEAIMOVE, UaTieAllFly),
    C("UpdateAIMove_fly_down_exact", SITHAIMOVEVEC_UPDATEAIMOVE, UaFlyDownExact),
    C("UpdateAIMove_fly_down_below", SITHAIMOVEVEC_UPDATEAIMOVE, UaFlyDownBelow),
    C("UpdateAIMove_nokeyframes", SITHAIMOVEVEC_UPDATEAIMOVE, UaNoPuppetKf),
    C("UpdateAIMove_nokeyframes_walking", SITHAIMOVEVEC_UPDATEAIMOVE, UaNoPuppetKfWalking),
    C("UpdateAIMove_changing_submode", SITHAIMOVEVEC_UPDATEAIMOVE, UaChanging),
    C("GetAIMoveModes_plain", SITHAIMOVEVEC_GETMOVEMODES, GmPlain),
    C("GetAIMoveModes_attached", SITHAIMOVEVEC_GETMOVEMODES, GmAttached),
    C("GetAIMoveModes_fastmove10", SITHAIMOVEVEC_GETMOVEMODES, GmFast10),
    C("GetAIMoveModes_fastmove15", SITHAIMOVEVEC_GETMOVEMODES, GmFast15),
    C("GetAIMoveModes_fastmove15_and_10", SITHAIMOVEVEC_GETMOVEMODES, GmFast15And10),
    C("GetAIMoveModes_fastmove15_attached", SITHAIMOVEVEC_GETMOVEMODES, GmFast15Attached),
    C("GetAIMoveModes_turning", SITHAIMOVEVEC_GETMOVEMODES, GmTurning),
    C("GetAIMoveModes_turning_attached", SITHAIMOVEVEC_GETMOVEMODES, GmTurningAttached),
    C("GetAIMoveModes_turning_fastmove10", SITHAIMOVEVEC_GETMOVEMODES, GmTurningFast10),
    C("GetAIMoveModes_turning_fastmove15", SITHAIMOVEVEC_GETMOVEMODES, GmTurningFast15),
    C("GetAIMoveModes_turning_flyer", SITHAIMOVEVEC_GETMOVEMODES, GmTurningFlyer),
    C("GetAIMoveModes_turning_flyer_fast", SITHAIMOVEVEC_GETMOVEMODES, GmTurningFlyerFast),
    C("GetAIMoveModes_flyer_notturning", SITHAIMOVEVEC_GETMOVEMODES, GmFlyerNotTurning),

    // SetSubMode
    C("SetSubMode_from_stand", SITHAIMOVEVEC_SETSUBMODE, SsFromStand),
    C("SetSubMode_from_stand_notransition", SITHAIMOVEVEC_SETSUBMODE, SsFromStandNoTrans),
    C("SetSubMode_from_stand_nokeyframes", SITHAIMOVEVEC_SETSUBMODE, SsFromStandNoKf),
    C("SetSubMode_from_stand_track", SITHAIMOVEVEC_SETSUBMODE, SsFromStandTrack),
    C("SetSubMode_from_walk", SITHAIMOVEVEC_SETSUBMODE, SsFromWalk),
    C("SetSubMode_from_walk_notransition", SITHAIMOVEVEC_SETSUBMODE, SsFromWalkNoTrans),
    C("SetSubMode_from_walk_track", SITHAIMOVEVEC_SETSUBMODE, SsFromWalkTrack),
    C("SetSubMode_from_run", SITHAIMOVEVEC_SETSUBMODE, SsFromRun),
    C("SetSubMode_from_run_notransition", SITHAIMOVEVEC_SETSUBMODE, SsFromRunNoTrans),
    C("SetSubMode_from_turnleft", SITHAIMOVEVEC_SETSUBMODE, SsFromTurnL),
    C("SetSubMode_from_turnright_track", SITHAIMOVEVEC_SETSUBMODE, SsFromTurnR),
    C("SetSubMode_from_fall_land", SITHAIMOVEVEC_SETSUBMODE, SsFromFall),
    C("SetSubMode_from_fall_noland", SITHAIMOVEVEC_SETSUBMODE, SsFromFallNoLand),
    C("SetSubMode_from_fall_land_positive", SITHAIMOVEVEC_SETSUBMODE, SsFromFallPlay1),
    C("SetSubMode_from_fall_land_track", SITHAIMOVEVEC_SETSUBMODE, SsFromFallTrack),
    C("SetSubMode_from_other", SITHAIMOVEVEC_SETSUBMODE, SsFromOther),
    C("SetSubMode_from_walkback", SITHAIMOVEVEC_SETSUBMODE, SsFromWalkBack),
    C("SetSubMode_from_strafe_right", SITHAIMOVEVEC_SETSUBMODE, SsFromStrafeR),
    C("SetSubMode_from_strafe_left", SITHAIMOVEVEC_SETSUBMODE, SsFromStrafeL),
    C("SetSubMode_majormode5", SITHAIMOVEVEC_SETSUBMODE, SsMajorModeOther),
    C("SetSubMode_majormode23", SITHAIMOVEVEC_SETSUBMODE, SsMajorModeLast),
    C("SetSubMode_nopuppetclass", SITHAIMOVEVEC_SETSUBMODE, SsNoPuppetClass),
    C("SetSubMode_nopuppetstate", SITHAIMOVEVEC_SETSUBMODE, SsNoPuppetState),
    C("SetSubMode_twice", SITHAIMOVEVEC_SETSUBMODE, SsFromWalkTwice),

    // PuppetCallback
    C("PuppetCallback_finish", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishes),
    C("PuppetCallback_finish_velocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishesVelocity),
    C("PuppetCallback_finish_fastvelocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishesFastVelocity),
    C("PuppetCallback_run_zerovelocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcRunZero),
    C("PuppetCallback_run_slowvelocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcRunSmall),
    C("PuppetCallback_run_exactvelocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcRunExact),
    C("PuppetCallback_run_fastvelocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcRunBig),
    C("PuppetCallback_walk_zerovelocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcWalkZero),
    C("PuppetCallback_walk_slowvelocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcWalkSmall),
    C("PuppetCallback_walk_exactvelocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcWalkExact),
    C("PuppetCallback_walk_fastvelocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcWalkBig),
    C("PuppetCallback_finish_turns", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishTurnsZero),
    C("PuppetCallback_finish_turns_velocity", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishTurnsVel),
    C("PuppetCallback_finish_other", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishOthers),
    C("PuppetCallback_finish_dying", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishDying),
    C("PuppetCallback_finish_destroyed", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishDestroyed),
    C("PuppetCallback_finish_nolocal", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishNoLocal),
    C("PuppetCallback_finish_knockedout_flags", SITHAIMOVEVEC_PUPPETCALLBACK, PcFinishKnockedOutFlags),
    C("PuppetCallback_feet", SITHAIMOVEVEC_PUPPETCALLBACK, PcFeet),
    C("PuppetCallback_feet_nolocal", SITHAIMOVEVEC_PUPPETCALLBACK, PcFeetNoLocal),
    C("PuppetCallback_died", SITHAIMOVEVEC_PUPPETCALLBACK, PcDiedMarkers),
    C("PuppetCallback_markers", SITHAIMOVEVEC_PUPPETCALLBACK, PcMarkers),
    C("PuppetCallback_markers_nolocal", SITHAIMOVEVEC_PUPPETCALLBACK, PcMarkersNoLocal),
    C("PuppetCallback_tracknums", SITHAIMOVEVEC_PUPPETCALLBACK, PcTracks),

    // Look and move positions
    C("AISetLookThing_actor", SITHAIMOVEVEC_SETLOOKTHING, LtActor),
    C("AISetLookThing_actor_rotated", SITHAIMOVEVEC_SETLOOKTHING, LtActorRotated),
    C("AISetLookThing_player", SITHAIMOVEVEC_SETLOOKTHING, LtPlayer),
    C("AISetLookThing_item", SITHAIMOVEVEC_SETLOOKTHING, LtItem),
    C("AISetLookThing_item_target", SITHAIMOVEVEC_SETLOOKTHING, LtItemTarget),
    C("AISetLookThing_locked", SITHAIMOVEVEC_SETLOOKTHING, LtLocked),
    C("AISetLookThing_disabled", SITHAIMOVEVEC_SETLOOKTHING, LtDisabled),
    C("AISetLookThing_disabled_editor", SITHAIMOVEVEC_SETLOOKTHING, LtDisabledEditor),
    C("AISetLookThing_wantevents", SITHAIMOVEVEC_SETLOOKTHING, LtWantEvents),
    C("AISetLookThing_samepos", SITHAIMOVEVEC_SETLOOKTHING, LtSamePosition),
    C("AISetLookThing_tiny", SITHAIMOVEVEC_SETLOOKTHING, LtTiny),
    C("AISetLookThing_edge_tiny_001", SITHAIMOVEVEC_SETLOOKTHING, LtTinyEdge),
    C("AISetLookThing_just_above_001", SITHAIMOVEVEC_SETLOOKTHING, LtJustAbove),
    C("AISetLookThing_tiny_wantevents", SITHAIMOVEVEC_SETLOOKTHING, LtTinyWantEvents),
    C("AISetLookThing_twice", SITHAIMOVEVEC_SETLOOKTHING, LtTwice),
    C("AISetLookThing_owner_notactor", SITHAIMOVEVEC_SETLOOKTHING, LtOwnerNotActor),
    C("AISetLookThing_owner_player", SITHAIMOVEVEC_SETLOOKTHING, LtOwnerPlayer),
    C("AISetLookPos_front", SITHAIMOVEVEC_SETLOOKPOS, LpPlain),
    C("AISetLookPos_far", SITHAIMOVEVEC_SETLOOKPOS, LpFar),
    C("AISetLookPos_vertical", SITHAIMOVEVEC_SETLOOKPOS, LpVertical),
    C("AISetLookPos_ateye", SITHAIMOVEVEC_SETLOOKPOS, LpAtEyePos),
    C("AISetLookPos_ateye_wantevents", SITHAIMOVEVEC_SETLOOKPOS, LpAtEyePosWantEvents),
    C("AISetLookPos_disabled_looking", SITHAIMOVEVEC_SETLOOKPOS, LpLooking),
    C("AISetLookPos_locked", SITHAIMOVEVEC_SETLOOKPOS, LpLockedSub),
    C("AISetLookPos_wantevents", SITHAIMOVEVEC_SETLOOKPOS, LpWantEvents),
    C("AISetLookPos_owner_notactor", SITHAIMOVEVEC_SETLOOKPOS, LpOwnerNoEye),
    C("AISetLookPos_edge_nan", SITHAIMOVEVEC_SETLOOKPOS, LpNaN),
    C("AISetLookPos_huge", SITHAIMOVEVEC_SETLOOKPOS, LpHuge),
    C("AISetLookPos_eyeoffset", SITHAIMOVEVEC_SETLOOKPOS, LpEyeZ),
    C("AISetLookPosEyeLevel_front", SITHAIMOVEVEC_SETLOOKPOSEYELEVEL, LePlain),
    C("AISetLookPosEyeLevel_below", SITHAIMOVEVEC_SETLOOKPOSEYELEVEL, LeBelow),
    C("AISetLookPosEyeLevel_ateyelevel", SITHAIMOVEVEC_SETLOOKPOSEYELEVEL, LeEyeLevel),
    C("AISetLookPosEyeLevel_tilted", SITHAIMOVEVEC_SETLOOKPOSEYELEVEL, LeTilted),
    C("AISetLookPosEyeLevel_locked", SITHAIMOVEVEC_SETLOOKPOSEYELEVEL, LeLocked),
    C("AISetLookPosEyeLevel_wantevents", SITHAIMOVEVEC_SETLOOKPOSEYELEVEL, LeWantEvents),
    C("AISetLookPosEyeLevel_bigeyeoffset", SITHAIMOVEVEC_SETLOOKPOSEYELEVEL, LeBigEye),
    C("AISetMovePos_plain", SITHAIMOVEVEC_SETMOVEPOS, MpPlain),
    C("AISetMovePos_speed2", SITHAIMOVEVEC_SETMOVEPOS, MpPlainSpeed2),
    C("AISetMovePos_near", SITHAIMOVEVEC_SETMOVEPOS, MpPlainNear),
    C("AISetMovePos_near_bigmovesize", SITHAIMOVEVEC_SETMOVEPOS, MpPlainNearBig),
    C("AISetMovePos_edge_tolerance", SITHAIMOVEVEC_SETMOVEPOS, MpEdgeEqPos),
    C("AISetMovePos_edge_above_tolerance", SITHAIMOVEVEC_SETMOVEPOS, MpEdgeAbovePos),
    C("AISetMovePos_onspot", SITHAIMOVEVEC_SETMOVEPOS, MpOnSpotPos),
    C("AISetMovePos_locked", SITHAIMOVEVEC_SETMOVEPOS, MpLocked),
    C("AISetMovePos_disabled", SITHAIMOVEVEC_SETMOVEPOS, MpDisabled),
    C("AISetMovePos_disabled_editor", SITHAIMOVEVEC_SETMOVEPOS, MpDisabledEditor),
    C("AISetMovePos_wantevents", SITHAIMOVEVEC_SETMOVEPOS, MpWantEvents),
    C("AISetMovePos_twice", SITHAIMOVEVEC_SETMOVEPOS, MpMoving),
    C("AISetMovePos_home_inside", SITHAIMOVEVEC_SETMOVEPOS, MpHomeInside),
    C("AISetMovePos_home_outside", SITHAIMOVEVEC_SETMOVEPOS, MpHomeOutside),
    C("AISetMovePos_home_outside_nearowner", SITHAIMOVEVEC_SETMOVEPOS, MpHomeOutsideNearOwner),
    C("AISetMovePos_home_edge", SITHAIMOVEVEC_SETMOVEPOS, MpHomeEdge),
    C("AISetMovePos_home_negative", SITHAIMOVEVEC_SETMOVEPOS, MpHomeNegative),
    C("AISetMovePos_home_samepos", SITHAIMOVEVEC_SETMOVEPOS, MpHomeSamePos),
    C("AISetMovePos_underwater", SITHAIMOVEVEC_SETMOVEPOS, MpUnderwater),
    C("AISetMovePos_flying", SITHAIMOVEVEC_SETMOVEPOS, MpFlying),
    C("AISetMovePos_wallcrawl", SITHAIMOVEVEC_SETMOVEPOS, MpWallCrawl),
    C("AISetMovePos_wallcrawl_onspot", SITHAIMOVEVEC_SETMOVEPOS, MpWallCrawlSpot),
    C("AISetMovePos_wallcrawl_mounting", SITHAIMOVEVEC_SETMOVEPOS, MpWallCrawlMounting),
    C("AISetMovePos_wallcrawl_unreachable", SITHAIMOVEVEC_SETMOVEPOS, MpWallCrawlUnreach),
    C("AISetMovePos_wallcrawl_unreachable_block", SITHAIMOVEVEC_SETMOVEPOS, MpWallCrawlUnreachBlock),
    C("AISetMovePos_moveforward", SITHAIMOVEVEC_SETMOVEPOS, MpMoveForward),
    C("AISetMovePos_moveforward_near", SITHAIMOVEVEC_SETMOVEPOS, MpMoveForwardSide),
    C("AISetMoveTargetPos_plain", SITHAIMOVEVEC_SETMOVETARGETPOS, MtPlain),
    C("AISetMoveTargetPos_locked_before", SITHAIMOVEVEC_SETMOVETARGETPOS, MtLockedBefore),
    C("AISetMoveTargetPos_twice", SITHAIMOVEVEC_SETMOVETARGETPOS, MtTwice),
    C("AISetMoveTargetPos_onspot", SITHAIMOVEVEC_SETMOVETARGETPOS, MtOnSpot),
    C("AISetMoveTargetPos_near", SITHAIMOVEVEC_SETMOVETARGETPOS, MtNear),
    C("AISetMoveTargetPos_disabled", SITHAIMOVEVEC_SETMOVETARGETPOS, MtDisabled),
    C("AISetMoveTargetPos_disabled_editor", SITHAIMOVEVEC_SETMOVETARGETPOS, MtDisabledEditor),
    C("AISetMoveTargetPos_wantevents", SITHAIMOVEVEC_SETMOVETARGETPOS, MtWantEvents),

    // Special moves
    C("AISpecialMove_roll", SITHAIMOVEVEC_SPECIALMOVE, SmRollOk),
    C("AISpecialMove_roll_left", SITHAIMOVEVEC_SPECIALMOVE, SmRollOkLeft),
    C("AISpecialMove_roll_default_direction", SITHAIMOVEVEC_SPECIALMOVE, SmRollOkDefault),
    C("AISpecialMove_roll_random", SITHAIMOVEVEC_SPECIALMOVE, SmRollRand),
    C("AISpecialMove_roll_random2", SITHAIMOVEVEC_SPECIALMOVE, SmRollRand2),
    C("AISpecialMove_roll_target_behind", SITHAIMOVEVEC_SPECIALMOVE, SmRollBehind),
    C("AISpecialMove_roll_target_side", SITHAIMOVEVEC_SPECIALMOVE, SmRollSide),
    C("AISpecialMove_roll_dot_040", SITHAIMOVEVEC_SPECIALMOVE, SmRollJust),
    C("AISpecialMove_roll_dot_041", SITHAIMOVEVEC_SPECIALMOVE, SmRollJust2),
    C("AISpecialMove_roll_noleftkeyframe", SITHAIMOVEVEC_SPECIALMOVE, SmRollNoLeftKf),
    C("AISpecialMove_roll_norightkeyframe", SITHAIMOVEVEC_SPECIALMOVE, SmRollNoRightKf),
    C("AISpecialMove_roll_nokeyframes", SITHAIMOVEVEC_SPECIALMOVE, SmRollNoKf),
    C("AISpecialMove_roll_path_blocked_once", SITHAIMOVEVEC_SPECIALMOVE, SmRollPathHit),
    C("AISpecialMove_roll_path_blocked_twice", SITHAIMOVEVEC_SPECIALMOVE, SmRollPathHitTwice),
    C("AISpecialMove_roll_path_blocked_always", SITHAIMOVEVEC_SPECIALMOVE, SmRollPathHitAlways),
    C("AISpecialMove_roll_half_blocked", SITHAIMOVEVEC_SPECIALMOVE, SmRollHalfBlocked),
    C("AISpecialMove_roll_end_blocked", SITHAIMOVEVEC_SPECIALMOVE, SmRollEndBlocked),
    C("AISpecialMove_roll_while_moving", SITHAIMOVEVEC_SPECIALMOVE, SmRollMoving),
    C("AISpecialMove_roll_rotated", SITHAIMOVEVEC_SPECIALMOVE, SmRollRotated),
    C("AISpecialMove_roll_movetarget_locked", SITHAIMOVEVEC_SPECIALMOVE, SmRollLocked),
    C("AISpecialMove_roll_bigmovesize", SITHAIMOVEVEC_SPECIALMOVE, SmRollBigMove),
    C("AISpecialMove_strafe", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeOk),
    C("AISpecialMove_strafe_left", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeOkLeft),
    C("AISpecialMove_strafe_default_direction", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeOkDefault),
    C("AISpecialMove_strafe_random", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeRand),
    C("AISpecialMove_strafe_target", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeTarget),
    C("AISpecialMove_strafe_dot_040", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeJust),
    C("AISpecialMove_strafe_dot_041", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeJust2),
    C("AISpecialMove_strafe_target_behind", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeBehind),
    C("AISpecialMove_strafe_nokeyframes", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeNoKf),
    C("AISpecialMove_strafe_path_blocked_once", SITHAIMOVEVEC_SPECIALMOVE, SmStrafePathHit),
    C("AISpecialMove_strafe_path_blocked_always", SITHAIMOVEVEC_SPECIALMOVE, SmStrafePathHitAlways),
    C("AISpecialMove_strafe_end_blocked", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeEndBlocked),
    C("AISpecialMove_strafe_while_moving", SITHAIMOVEVEC_SPECIALMOVE, SmStrafeMoving),
    C("AISpecialMove_turn45", SITHAIMOVEVEC_SPECIALMOVE, SmTurn45Ok),
    C("AISpecialMove_turn45_norightkeyframe", SITHAIMOVEVEC_SPECIALMOVE, SmTurn45NoRight),
    C("AISpecialMove_turn45_noleftkeyframe", SITHAIMOVEVEC_SPECIALMOVE, SmTurn45NoLeft),
    C("AISpecialMove_turn45_nokeyframes", SITHAIMOVEVEC_SPECIALMOVE, SmTurn45NoKf),
    C("AISpecialMove_turn90", SITHAIMOVEVEC_SPECIALMOVE, SmTurn90Ok),
    C("AISpecialMove_turn90_nokeyframes", SITHAIMOVEVEC_SPECIALMOVE, SmTurn90NoKf),
    C("AISpecialMove_turn90_norightkeyframe", SITHAIMOVEVEC_SPECIALMOVE, SmTurn90NoRight),
    C("AISpecialMove_turn135", SITHAIMOVEVEC_SPECIALMOVE, SmTurn135Ok),
    C("AISpecialMove_turn135_nokeyframes", SITHAIMOVEVEC_SPECIALMOVE, SmTurn135NoKf),
    C("AISpecialMove_turn135_norightkeyframe", SITHAIMOVEVEC_SPECIALMOVE, SmTurn135NoRight),
    C("AISpecialMove_turn180", SITHAIMOVEVEC_SPECIALMOVE, SmTurn180Ok),
    C("AISpecialMove_turn180_nokeyframes", SITHAIMOVEVEC_SPECIALMOVE, SmTurn180NoKf),
    C("AISpecialMove_nothing", SITHAIMOVEVEC_SPECIALMOVE, SmNothingToDo),
    C("AISpecialMove_turn45_majormode9", SITHAIMOVEVEC_SPECIALMOVE, SmMajorModeOther),
    C("AISpecialMove_mountwall_perpendicular", SITHAIMOVEVEC_SPECIALMOVE, SmMountPerp),
    C("AISpecialMove_mountwall_perpendicular_negative", SITHAIMOVEVEC_SPECIALMOVE, SmMountPerpNeg),
    C("AISpecialMove_mountwall_80", SITHAIMOVEVEC_SPECIALMOVE, SmMount80),
    C("AISpecialMove_mountwall_100", SITHAIMOVEVEC_SPECIALMOVE, SmMount100),
    C("AISpecialMove_mountwall_79", SITHAIMOVEVEC_SPECIALMOVE, SmMount79),
    C("AISpecialMove_mountwall_70", SITHAIMOVEVEC_SPECIALMOVE, SmMount70),
    C("AISpecialMove_mountwall_60", SITHAIMOVEVEC_SPECIALMOVE, SmMount60),
    C("AISpecialMove_mountwall_60_other_side", SITHAIMOVEVEC_SPECIALMOVE, SmMount60Back),
    C("AISpecialMove_mountwall_795", SITHAIMOVEVEC_SPECIALMOVE, SmMount795),
    C("AISpecialMove_mountwall_50", SITHAIMOVEVEC_SPECIALMOVE, SmMount50),
    C("AISpecialMove_mountwall_45", SITHAIMOVEVEC_SPECIALMOVE, SmMount45),
    C("AISpecialMove_mountwall_41", SITHAIMOVEVEC_SPECIALMOVE, SmMount41),
    C("AISpecialMove_mountwall_40", SITHAIMOVEVEC_SPECIALMOVE, SmMount40),
    C("AISpecialMove_mountwall_30", SITHAIMOVEVEC_SPECIALMOVE, SmMount30),
    C("AISpecialMove_mountwall_edge_80_equal", SITHAIMOVEVEC_SPECIALMOVE, SmMount80Eq),
    C("AISpecialMove_mountwall_edge_80_above", SITHAIMOVEVEC_SPECIALMOVE, SmMount80Up),
    C("AISpecialMove_mountwall_edge_80_below", SITHAIMOVEVEC_SPECIALMOVE, SmMount80Down),
    C("AISpecialMove_mountwall_edge_100_equal", SITHAIMOVEVEC_SPECIALMOVE, SmMount100Eq),
    C("AISpecialMove_mountwall_edge_100_above", SITHAIMOVEVEC_SPECIALMOVE, SmMount100Up),
    C("AISpecialMove_mountwall_edge_100_below", SITHAIMOVEVEC_SPECIALMOVE, SmMount100Down),
    C("AISpecialMove_mountwall_edge_79_equal", SITHAIMOVEVEC_SPECIALMOVE, SmMount79Eq),
    C("AISpecialMove_mountwall_edge_79_above", SITHAIMOVEVEC_SPECIALMOVE, SmMount79Up),
    C("AISpecialMove_mountwall_edge_79_below", SITHAIMOVEVEC_SPECIALMOVE, SmMount79Down),
    C("AISpecialMove_mountwall_same_normal", SITHAIMOVEVEC_SPECIALMOVE, SmMountSame),
    C("AISpecialMove_mountwall_opposite_normal", SITHAIMOVEVEC_SPECIALMOVE, SmMountOpposite),
    C("AISpecialMove_mountwall_behind", SITHAIMOVEVEC_SPECIALMOVE, SmMountBehind),
    C("AISpecialMove_mountwall_nokeyframe", SITHAIMOVEVEC_SPECIALMOVE, SmMountNoKf),
    C("AISpecialMove_mountwall_110", SITHAIMOVEVEC_SPECIALMOVE, SmMount110),
    C("AISpecialMove_mountwall_attached", SITHAIMOVEVEC_SPECIALMOVE, SmMountAttached),
    C("AIFinalizeSpecialMove_strafe", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsStrafeNoMove),
    C("AIFinalizeSpecialMove_strafe_moving", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsStrafeDirection),
    C("AIFinalizeSpecialMove_turn45_left", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsTurn45Left),
    C("AIFinalizeSpecialMove_turn45_right", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsTurn45Right),
    C("AIFinalizeSpecialMove_turn90_left", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsTurn90Left),
    C("AIFinalizeSpecialMove_turn90_right", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsTurn90Right),
    C("AIFinalizeSpecialMove_turn135_left", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsTurn135Left),
    C("AIFinalizeSpecialMove_turn135_right", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsTurn135Right),
    C("AIFinalizeSpecialMove_turn180", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsTurn180),
    C("AIFinalizeSpecialMove_turn_tilted", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsTurnTilted),
    C("AIFinalizeSpecialMove_turn45_rotated", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsTurn45Rotated),
    C("AIFinalizeSpecialMove_mountwall", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsMountMounting),
    C("AIFinalizeSpecialMove_mountwall_bigmove", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsMountMountingBigMove),
    C("AIFinalizeSpecialMove_mountwall_notmounting", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsMountOtherStatus),
    C("AIFinalizeSpecialMove_mountwall_diagonal", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsMountDiag),
    C("AIFinalizeSpecialMove_mountwall_degenerate", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsMountDegenerate),
    C("AIFinalizeSpecialMove_other_flags", SITHAIMOVEVEC_FINALIZESPECIALMOVE, FsNothing),

    // Jump, stop, goal and unreachable handling, look reset
    C("AIJump_playmode", SITHAIMOVEVEC_JUMP, JpPlayMode),
    C("AIJump_noanimation", SITHAIMOVEVEC_JUMP, JpPlayModeFail),
    C("AIJump_twice", SITHAIMOVEVEC_JUMP, JpTwice),
    C("AIJump_edge_timewrap", SITHAIMOVEVEC_JUMP, JpTimeWrap),
    C("AIStop_plain", SITHAIMOVEVEC_STOP, StopPlain),
    C("AIStop_continuous", SITHAIMOVEVEC_STOP, StopContinuous),
    C("AIStop_animated", SITHAIMOVEVEC_STOP, StopAnimated),
    C("AIStop_stationary", SITHAIMOVEVEC_STOP, StopStationary),
    C("AIStop_walking", SITHAIMOVEVEC_STOP, StopWalking),
    C("SetGoalReached_plain", SITHAIMOVEVEC_SETGOALREACHED, GrPlain),
    C("SetGoalReached_block", SITHAIMOVEVEC_SETGOALREACHED, GrBlock),
    C("SetGoalReached_newmove", SITHAIMOVEVEC_SETGOALREACHED, GrNewMove),
    C("SetGoalReached_waypoints", SITHAIMOVEVEC_SETGOALREACHED, GrWpnts),
    C("SetGoalReached_animated", SITHAIMOVEVEC_SETGOALREACHED, GrAnimated),
    C("SetGoalReached_stationary", SITHAIMOVEVEC_SETGOALREACHED, GrStationary),
    C("SetGoalReached_continuous", SITHAIMOVEVEC_SETGOALREACHED, GrContinuous),
    C("SetGoalReached_standing", SITHAIMOVEVEC_SETGOALREACHED, GrStillStanding),
    C("Unreachable_plain", SITHAIMOVEVEC_UNREACHABLE, UnPlain),
    C("Unreachable_block", SITHAIMOVEVEC_UNREACHABLE, UnBlock),
    C("Unreachable_animated", SITHAIMOVEVEC_UNREACHABLE, UnAnimated),
    C("StopAIMovement_moving", SITHAIMOVEVEC_STOPAIMOVEMENT, SaMoving),
    C("StopAIMovement_continuous", SITHAIMOVEVEC_STOPAIMOVEMENT, SaContinuous),
    C("StopAIMovement_idle", SITHAIMOVEVEC_STOPAIMOVEMENT, SaIdle),
    C("ResetAILook_turning", SITHAIMOVEVEC_RESETAILOOK, RlTurning),
    C("ResetAILook_idle", SITHAIMOVEVEC_RESETAILOOK, RlIdle),

    // UpdateBoss
    C("UpdateBoss_right", SITHAIMOVEVEC_UPDATEBOSS, BsRight),
    C("UpdateBoss_left", SITHAIMOVEVEC_UPDATEBOSS, BsLeft),
    C("UpdateBoss_behind", SITHAIMOVEVEC_UPDATEBOSS, BsBehind),
    C("UpdateBoss_near", SITHAIMOVEVEC_UPDATEBOSS, BsNear),
    C("UpdateBoss_aligned", SITHAIMOVEVEC_UPDATEBOSS, BsAligned),
    C("UpdateBoss_edge_updot_equal", SITHAIMOVEVEC_UPDATEBOSS, BsUpDotEqual),
    C("UpdateBoss_edge_updot_above", SITHAIMOVEVEC_UPDATEBOSS, BsUpDotAbove),
    C("UpdateBoss_edge_limit_equal", SITHAIMOVEVEC_UPDATEBOSS, BsLimitEqual),
    C("UpdateBoss_edge_limit_below", SITHAIMOVEVEC_UPDATEBOSS, BsLimitBelow),
    C("UpdateBoss_edge_limit_equal_behind", SITHAIMOVEVEC_UPDATEBOSS, BsLimitEqualBehind),
    C("UpdateBoss_negative_rate", SITHAIMOVEVEC_UPDATEBOSS, BsNegativeRate),
    C("UpdateBoss_lookup", SITHAIMOVEVEC_UPDATEBOSS, BsLookUp),
    C("UpdateBoss_lookdown", SITHAIMOVEVEC_UPDATEBOSS, BsLookDown),
    C("UpdateBoss_edge_steep", SITHAIMOVEVEC_UPDATEBOSS, BsSteep),
    C("UpdateBoss_edge_steep_below", SITHAIMOVEVEC_UPDATEBOSS, BsSteepBelow),
    C("UpdateBoss_tilted", SITHAIMOVEVEC_UPDATEBOSS, BsTilted),
    C("UpdateBoss_fast", SITHAIMOVEVEC_UPDATEBOSS, BsFast),
    C("UpdateBoss_keeps_movestatus", SITHAIMOVEVEC_UPDATEBOSS, BsKeepsStatus),
    C("UpdateBoss_zero_dt", SITHAIMOVEVEC_UPDATEBOSS, BsZeroDt),

    // Head tracking
    C("UpdateHeadTrackTarget_norotate", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtNoRotate),
    C("UpdateHeadTrackTarget_immobile", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtImmobile),
    C("UpdateHeadTrackTarget_target_right", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetRight),
    C("UpdateHeadTrackTarget_target_left", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetLeft),
    C("UpdateHeadTrackTarget_target_ahead", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetAhead),
    C("UpdateHeadTrackTarget_target_below", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetBelow),
    C("UpdateHeadTrackTarget_target_behind", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetBehind),
    C("UpdateHeadTrackTarget_target_behind_tracking", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetBehindTracking),
    C("UpdateHeadTrackTarget_target_slightlybehind_tracking", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetSlightlyBehind),
    C("UpdateHeadTrackTarget_target_slightlybehind", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetSlightlyBehindNot),
    C("UpdateHeadTrackTarget_target_behind_deep_tracking", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetBehindDeep),
    C("UpdateHeadTrackTarget_target_side", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetSide),
    C("UpdateHeadTrackTarget_yaw_limit_exact_right", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtYawExactRight),
    C("UpdateHeadTrackTarget_yaw_limit_exact_left", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtYawExactLeft),
    C("UpdateHeadTrackTarget_block_behind_exact", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtBehindBlockExact),
    C("UpdateHeadTrackTarget_target_side_tracking", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetSideTracking),
    C("UpdateHeadTrackTarget_notarget_clears_flag", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtNoTarget),
    C("UpdateHeadTrackTarget_notarget_lookpos", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtNoTargetLook),
    C("UpdateHeadTrackTarget_invisible_target", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtInvisibleTarget),
    C("UpdateHeadTrackTarget_invisible_target_seen", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtInvisibleSeen),
    C("UpdateHeadTrackTarget_invisible_item", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtInvisibleItem),
    C("UpdateHeadTrackTarget_invisible_player", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtInvisiblePlayer),
    C("UpdateHeadTrackTarget_blind", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtBlind),
    C("UpdateHeadTrackTarget_blind_seeinvisible", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtBlindSeeInvisible),
    C("UpdateHeadTrackTarget_item_target", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtItemTarget),
    C("UpdateHeadTrackTarget_player_target", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtPlayerTarget),
    C("UpdateHeadTrackTarget_noslopemove", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtNoSlope),
    C("UpdateHeadTrackTarget_block", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtBlockMode),
    C("UpdateHeadTrackTarget_edge_zero_yaw_limit", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtZeroYawLimit),
    C("UpdateHeadTrackTarget_edge_stopdot_equal", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtLowEqual),
    C("UpdateHeadTrackTarget_edge_stopdot_above", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtLowAbove),
    C("UpdateHeadTrackTarget_edge_stopdot_below", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtLowBelow),
    C("UpdateHeadTrackTarget_limits_up", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtLimits),
    C("UpdateHeadTrackTarget_limits_down", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtLimitsDown),
    C("UpdateHeadTrackTarget_yaw_limit", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtLimitedYawExact),
    C("UpdateHeadTrackTarget_fasthead", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtFastHead),
    C("UpdateHeadTrackTarget_slowhead", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtSlowHead),
    C("UpdateHeadTrackTarget_start_pitched", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtStartPitched),
    C("UpdateHeadTrackTarget_tilted", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTilted),
    C("UpdateHeadTrackTarget_target_overhead", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetOverhead),
    C("UpdateHeadTrackTarget_target_rotated", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetRotated),
    C("UpdateHeadTrackTarget_target_at_owner", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtTargetAtOwner),
    C("UpdateHeadTrackTarget_lookpos_at_eye", SITHAIMOVEVEC_UPDATEHEADTRACKTARGET, HtLookPosEqualsEye),
    C("UpdateHeadTrackLookPos_norotate", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlNoRotate),
    C("UpdateHeadTrackLookPos_immobile", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlImmobile),
    C("UpdateHeadTrackLookPos_right", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlRight),
    C("UpdateHeadTrackLookPos_left", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlLeft),
    C("UpdateHeadTrackLookPos_behind", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlBehind),
    C("UpdateHeadTrackLookPos_ahead_done", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlAhead),
    C("UpdateHeadTrackLookPos_ahead_turning_kept", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlAheadDone),
    C("UpdateHeadTrackLookPos_up", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlUp),
    C("UpdateHeadTrackLookPos_down", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlDown),
    C("UpdateHeadTrackLookPos_slow_reaches_goal", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlSlowReachesGoal),
    C("UpdateHeadTrackLookPos_start_pitched", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlStartPitched),
    C("UpdateHeadTrackLookPos_pitch_only", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlPitchOnly),
    C("UpdateHeadTrackLookPos_yaw_only", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlYawOnly),
    C("UpdateHeadTrackLookPos_yaw_below_threshold", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlYawBelowHalf),
    C("UpdateHeadTrackLookPos_yaw_above_threshold", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlYawAboveHalf),
    C("UpdateHeadTrackLookPos_pitch_below_threshold", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlPitchBelowHalf),
    C("UpdateHeadTrackLookPos_pitch_above_threshold", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlPitchAboveHalf),
    C("UpdateHeadTrackLookPos_edge_yaw_done_equal", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlYawDoneExact),
    C("UpdateHeadTrackLookPos_edge_yaw_done_above", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlYawDoneAbove),
    C("UpdateHeadTrackLookPos_edge_pitch_done_equal", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlPitchDoneExact),
    C("UpdateHeadTrackLookPos_edge_pitch_done_above", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlPitchDoneAbove),
    C("UpdateHeadTrackLookPos_limits", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlLimits),
    C("UpdateHeadTrackLookPos_behind_clamped", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlBehindClamped),
    C("UpdateHeadTrackLookPos_tilted", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlTilted),
    C("UpdateHeadTrackLookPos_fast", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlFast),
    C("UpdateHeadTrackLookPos_at_eye", SITHAIMOVEVEC_UPDATEHEADTRACKLOOKPOS, HlAtEye),

    // Quetzalcoatl
    C("GetQuetzStrikeAngles_near", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeNear),
    C("GetQuetzStrikeAngles_mindist", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeMin),
    C("GetQuetzStrikeAngles_middist", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeMid),
    C("GetQuetzStrikeAngles_maxdist", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeMax),
    C("GetQuetzStrikeAngles_far", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeFar),
    C("GetQuetzStrikeAngles_side", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeSide),
    C("GetQuetzStrikeAngles_left_high", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeLeftHigh),
    C("GetQuetzStrikeAngles_behind", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeBehind),
    C("GetQuetzStrikeAngles_crouching", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeCrouching),
    C("GetQuetzStrikeAngles_rotated_target", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeRotatedTarget),
    C("GetQuetzStrikeAngles_tilted", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeTilted),
    C("GetQuetzStrikeAngles_owner_eye", SITHAIMOVEVEC_GETQUETZSTRIKEANGLES, QaStrikeOwnerEye),
    C("GetQuetzAnim2Angles_near", SITHAIMOVEVEC_GETQUETZANIM2ANGLES, QaStrikeNear),
    C("GetQuetzAnim2Angles_far", SITHAIMOVEVEC_GETQUETZANIM2ANGLES, QaStrikeFar),
    C("GetQuetzAnim2Angles_side", SITHAIMOVEVEC_GETQUETZANIM2ANGLES, QaStrikeSide),
    C("GetQuetzAnim2Angles_left_high", SITHAIMOVEVEC_GETQUETZANIM2ANGLES, QaStrikeLeftHigh),
    C("GetQuetzAnim2Angles_behind", SITHAIMOVEVEC_GETQUETZANIM2ANGLES, QaStrikeBehind),
    C("GetQuetzAnim2Angles_rotated_target", SITHAIMOVEVEC_GETQUETZANIM2ANGLES, QaStrikeRotatedTarget),
    C("GetQuetzAnim2Angles_tilted", SITHAIMOVEVEC_GETQUETZANIM2ANGLES, QaStrikeTilted),
    C("UpdateQuetzStrike_snapshot1", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeMode),
    C("UpdateQuetzStrike_snapshot2", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeSnap2),
    C("UpdateQuetzStrike_snapshot3", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeSnap3),
    C("UpdateQuetzStrike_snapshot4", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeSnap4),
    C("UpdateQuetzStrike_snapshot5_last", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeSnap5Last),
    C("UpdateQuetzStrike_snapshot5_notlast", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeSnap5NotLast),
    C("UpdateQuetzStrike_snapshot6", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeSnap6Beyond),
    C("UpdateQuetzStrike_noblock", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeNoBlockFrames),
    C("UpdateQuetzStrike_wrongmode", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeWrongMode),
    C("UpdateQuetzStrike_nomode", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeNoneMode),
    C("UpdateQuetzStrike_exact_steps", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeExact),
    C("UpdateQuetzStrike_big_steps", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeBig),
    C("UpdateQuetzStrike_huge_step", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeHuge),
    C("UpdateQuetzStrike_zero_step", SITHAIMOVEVEC_UPDATEQUETZSTRIKE, QsStrikeZeroStep),
    C("UpdateQuetzAnim2_snapshot1", SITHAIMOVEVEC_UPDATEQUETZANIM2, QsAnim2Mode),
    C("UpdateQuetzAnim2_snapshot2", SITHAIMOVEVEC_UPDATEQUETZANIM2, QsAnim2Snap2),
    C("UpdateQuetzAnim2_exact_steps", SITHAIMOVEVEC_UPDATEQUETZANIM2, QsAnim2Exact),
    C("UpdateQuetzAnim2_last", SITHAIMOVEVEC_UPDATEQUETZANIM2, QsAnim2Last),
    C("UpdateQuetzAnim2_wrongmode", SITHAIMOVEVEC_UPDATEQUETZANIM2, QsAnim2WrongMode),
    C("UpdateQuetzAnim2_noblock", SITHAIMOVEVEC_UPDATEQUETZANIM2, QsAnim2NoBlock),
    C("UpdateQuetzAnim2_big_steps", SITHAIMOVEVEC_UPDATEQUETZANIM2, QsAnim2Big),
    C("UpdateQuetzAnim3_exact_steps", SITHAIMOVEVEC_UPDATEQUETZANIM3, QsAnim3Exact),
    C("UpdateQuetzAnim3_frames", SITHAIMOVEVEC_UPDATEQUETZANIM3, QsAnim3Mode),
    C("UpdateQuetzAnim3_last", SITHAIMOVEVEC_UPDATEQUETZANIM3, QsAnim3Last),
    C("UpdateQuetzAnim3_beyond", SITHAIMOVEVEC_UPDATEQUETZANIM3, QsAnim3Beyond),
    C("UpdateQuetzAnim3_wrongmode", SITHAIMOVEVEC_UPDATEQUETZANIM3, QsAnim3WrongMode),
    C("UpdateQuetzAnim3_huge_step", SITHAIMOVEVEC_UPDATEQUETZANIM3, QsAnim3Huge),
    C("UpdateQuetzAnim5_exact_steps", SITHAIMOVEVEC_UPDATEQUETZANIM5, QsAnim5Exact),
    C("UpdateQuetzAnim5_frames", SITHAIMOVEVEC_UPDATEQUETZANIM5, QsAnim5Mode),
    C("UpdateQuetzAnim5_last", SITHAIMOVEVEC_UPDATEQUETZANIM5, QsAnim5Last),
    C("UpdateQuetzAnim5_wrongmode", SITHAIMOVEVEC_UPDATEQUETZANIM5, QsAnim5WrongMode),
    C("UpdateQuetzAnim5_huge_step", SITHAIMOVEVEC_UPDATEQUETZANIM5, QsAnim5Huge),
    C("UpdateQuetzAnim4_exact_steps", SITHAIMOVEVEC_UPDATEQUETZANIM4, QsAnim4Exact),
    C("UpdateQuetzAnim4_frames", SITHAIMOVEVEC_UPDATEQUETZANIM4, QsAnim4Mode),
    C("UpdateQuetzAnim4_last", SITHAIMOVEVEC_UPDATEQUETZANIM4, QsAnim4Last),
    C("UpdateQuetzAnim4_wrongmode", SITHAIMOVEVEC_UPDATEQUETZANIM4, QsAnim4WrongMode),
    C("UpdateQuetzAnim4_huge_step", SITHAIMOVEVEC_UPDATEQUETZANIM4, QsAnim4Huge),
    C("UpdateQuetzTail_nomode", SITHAIMOVEVEC_UPDATEQUETZTAIL, QtNone),
    C("UpdateQuetzTail_strike", SITHAIMOVEVEC_UPDATEQUETZTAIL, QtStrike),
    C("UpdateQuetzTail_anim2", SITHAIMOVEVEC_UPDATEQUETZTAIL, QtAnim2),
    C("UpdateQuetzTail_anim3", SITHAIMOVEVEC_UPDATEQUETZTAIL, QtAnim3),
    C("UpdateQuetzTail_anim4", SITHAIMOVEVEC_UPDATEQUETZTAIL, QtAnim4),
    C("UpdateQuetzTail_anim5", SITHAIMOVEVEC_UPDATEQUETZTAIL, QtAnim5),
    C("UpdateQuetzTail_mode6", SITHAIMOVEVEC_UPDATEQUETZTAIL, QtMode6),
    C("UpdateQuetzTail_noblock", SITHAIMOVEVEC_UPDATEQUETZTAIL, QtNoBlock),
    C("StartQuetzStrike_run", SITHAIMOVEVEC_STARTQUETZSTRIKE, QxStrike),
    C("StartQuetzStrike_run_far", SITHAIMOVEVEC_STARTQUETZSTRIKE, QxStrikeFar),
    C("StartQuetzStrike_run_bigsteps", SITHAIMOVEVEC_STARTQUETZSTRIKE, QxStrikeBigSteps),
    C("StartQuetzStrike_noblock", SITHAIMOVEVEC_STARTQUETZSTRIKE, QxStrikeNoBlock),
    C("StartQuetzAnim2_run", SITHAIMOVEVEC_STARTQUETZANIM2, QxAnim2),
    C("StartQuetzAnim2_run_frames", SITHAIMOVEVEC_STARTQUETZANIM2, QxAnim2Frames),
    C("StartQuetzAnim2_noblock", SITHAIMOVEVEC_STARTQUETZANIM2, QxAnim2NoBlock),
    C("StartQuetzAnim3_run", SITHAIMOVEVEC_STARTQUETZANIM3, QxAnim3),
    C("StartQuetzAnim3_run_short", SITHAIMOVEVEC_STARTQUETZANIM3, QxAnim3Short),
    C("StartQuetzAnim3_noblock", SITHAIMOVEVEC_STARTQUETZANIM3, QxAnim3NoBlock),
    C("StartQuetzAnim5_run", SITHAIMOVEVEC_STARTQUETZANIM5, QxAnim5),
    C("StartQuetzAnim5_run_short", SITHAIMOVEVEC_STARTQUETZANIM5, QxAnim5Short),
    C("StartQuetzAnim5_noblock", SITHAIMOVEVEC_STARTQUETZANIM5, QxAnim5NoBlock),
    C("StartQuetzAnim4_run", SITHAIMOVEVEC_STARTQUETZANIM4, QxAnim4),
    C("StartQuetzAnim4_run_short", SITHAIMOVEVEC_STARTQUETZANIM4, QxAnim4Short),
    C("StartQuetzAnim4_noblock", SITHAIMOVEVEC_STARTQUETZANIM4, QxAnim4NoBlock),
    C("UpdateMardukTail_marduk", SITHAIMOVEVEC_UPDATEMARDUKTAIL, MkMarduk),
    C("UpdateMardukTail_marduk2", SITHAIMOVEVEC_UPDATEMARDUKTAIL, MkMarduk2),
    C("UpdateMardukTail_other_name", SITHAIMOVEVEC_UPDATEMARDUKTAIL, MkOther),
    C("UpdateMardukTail_capitalized", SITHAIMOVEVEC_UPDATEMARDUKTAIL, MkPrefix),
    C("UpdateMardukTail_nojoint", SITHAIMOVEVEC_UPDATEMARDUKTAIL, MkNoJoint),
    C("UpdateMardukTail_joint0", SITHAIMOVEVEC_UPDATEMARDUKTAIL, MkJointZero),
    C("UpdateMardukTail_joint31", SITHAIMOVEVEC_UPDATEMARDUKTAIL, MkJointLast),
};

#undef C

static const SithVecSuite sithAIMoveVec_suite = {
    "sithAIMove",
    sithAIMoveVec_aCases,
    SITHVEC_ARRAYLEN(sithAIMoveVec_aCases),
    sithAIMoveVec_Build,
    sithAIMoveVec_Watch,
    NULL,
};

#define SITHVEC_SUITE sithAIMoveVec_suite

// Test groups: Unity test name, case name prefix
#define SITHAIMOVEVEC_TEST_LIST(X)                       \
    X(Update, "Update_")                                 \
    X(UpdateMineCar, "UpdateMineCar_")                   \
    X(UpdateTurn, "UpdateTurn_")                         \
    X(UpdateMove, "UpdateMove_")                         \
    X(CheckMoveAlign, "CheckMoveAlign_")                 \
    X(GetGoalTolerance, "GetGoalTolerance_")             \
    X(CheckWalkMove, "CheckWalkMove_")                   \
    X(CheckFlyMove, "CheckFlyMove_")                     \
    X(CheckSwimMove, "CheckSwimMove_")                   \
    X(AIGetMoveState, "AIGetMoveState_")                 \
    X(AISpecialTurn, "AISpecialTurn_")                   \
    X(UpdateAIMove, "UpdateAIMove_")                     \
    X(GetAIMoveModes, "GetAIMoveModes_")                 \
    X(SetSubMode, "SetSubMode_")                         \
    X(PuppetCallback, "PuppetCallback_")                 \
    X(AISetLookThing, "AISetLookThing_")                 \
    X(AISetLookPos, "AISetLookPos_")                     \
    X(AISetLookPosEyeLevel, "AISetLookPosEyeLevel_")     \
    X(AISetMovePos, "AISetMovePos_")                     \
    X(AISetMoveTargetPos, "AISetMoveTargetPos_")         \
    X(AISpecialMove, "AISpecialMove_")                   \
    X(AIFinalizeSpecialMove, "AIFinalizeSpecialMove_")   \
    X(AIJump, "AIJump_")                                 \
    X(AIStop, "AIStop_")                                 \
    X(SetGoalReached, "SetGoalReached_")                 \
    X(Unreachable, "Unreachable_")                       \
    X(StopAIMovement, "StopAIMovement_")                 \
    X(ResetAILook, "ResetAILook_")                       \
    X(UpdateBoss, "UpdateBoss_")                         \
    X(UpdateHeadTrackTarget, "UpdateHeadTrackTarget_")   \
    X(UpdateHeadTrackLookPos, "UpdateHeadTrackLookPos_") \
    X(GetQuetzStrikeAngles, "GetQuetzStrikeAngles_")     \
    X(GetQuetzAnim2Angles, "GetQuetzAnim2Angles_")       \
    X(UpdateQuetzStrike, "UpdateQuetzStrike_")           \
    X(UpdateQuetzAnim2, "UpdateQuetzAnim2_")             \
    X(UpdateQuetzAnim3, "UpdateQuetzAnim3_")             \
    X(UpdateQuetzAnim5, "UpdateQuetzAnim5_")             \
    X(UpdateQuetzAnim4, "UpdateQuetzAnim4_")             \
    X(UpdateQuetzTail, "UpdateQuetzTail_")               \
    X(StartQuetzStrike, "StartQuetzStrike_")             \
    X(StartQuetzAnim2, "StartQuetzAnim2_")               \
    X(StartQuetzAnim3, "StartQuetzAnim3_")               \
    X(StartQuetzAnim5, "StartQuetzAnim5_")               \
    X(StartQuetzAnim4, "StartQuetzAnim4_")               \
    X(UpdateMardukTail, "UpdateMardukTail_")

#undef CALLS
#undef PARAMS

#endif // SITH_AI_TESTS_SITHAIMOVEVECCASES_H
