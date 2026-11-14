# World thing and template I/O, voice lip-sync and COG suites. Included by CMakeLists.txt.
set(SITH_WORLD_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

set(SITH_WORLD_TEST_SOURCES
    "${SITH_WORLD_DIR}/Cog/Tests/sithCogTest.c"
    "${SITH_WORLD_DIR}/World/Tests/sithTemplateTest.c"
    "${SITH_WORLD_DIR}/World/Tests/sithThingTest.c"
    "${SITH_WORLD_DIR}/World/Tests/sithVoiceTest.c"
)

set(SITH_WORLD_TESTED_SOURCES
    "${SITH_WORLD_DIR}/Cog/sithCog.c"
    "${SITH_WORLD_DIR}/World/sithTemplate.c"
    "${SITH_WORLD_DIR}/World/sithThing.c"
    "${SITH_WORLD_DIR}/World/sithVoice.c"
    "${SITH_WORLD_DIR}/../std/General/stdConffile.c"
)

set_property(SOURCE "${SITH_WORLD_DIR}/Cog/sithCog.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
    sithConsole_PrintString=sithCogTest_PrintString
)

set_property(SOURCE "${SITH_WORLD_DIR}/Cog/Tests/sithCogTest.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
)

set_property(SOURCE "${SITH_WORLD_DIR}/World/sithTemplate.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
    J3D_LOCAL_RDROID_RTI
    rdThing_FreeEntry=sithThingTest_FreeRdThingEntry
    stdMemory_Malloc=sithThingTest_MemoryMalloc
    stdMemory_Free=sithThingTest_MemoryFree
    stdHashtbl_New=sithThingTest_HashtblNew
    stdHashtbl_Free=sithThingTest_HashtblFree
    stdHashtbl_Add=sithThingTest_HashtblAdd
    stdHashtbl_Find=sithThingTest_HashtblFind
    stdHashtbl_Remove=sithThingTest_HashtblRemove
)

set_property(SOURCE "${SITH_WORLD_DIR}/World/sithThing.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
    J3D_LOCAL_RDROID_RTI
    sithTemplate_GetTemplate=sithThingTest_GetTemplate
    sithModel_Load=sithThingTest_LoadModel
    sithSprite_Load=sithThingTest_LoadSprite
    sithParticle_Load=sithThingTest_LoadParticle
    sithPuppet_LoadPuppetClass=sithThingTest_LoadPuppetClass
    sithSoundClass_Load=sithThingTest_LoadSoundClass
    sithCog_Load=sithThingTest_LoadCog
    sithMaterial_Load=sithThingTest_LoadMaterial
    sithAIClass_Load=sithThingTest_LoadAIClass
    rdThing_FreeEntry=sithThingTest_FreeRdThingEntry
    rdThing_SetModel3=sithThingTest_SetModel3
    rdThing_SetSprite3=sithThingTest_SetSprite3
    rdThing_SetParticleCloud=sithThingTest_SetParticleCloud
    rdPuppet_New=sithThingTest_NewRdPuppet
    stdMemory_Malloc=sithThingTest_MemoryMalloc
    stdMemory_Free=sithThingTest_MemoryFree
    sithAI_AllocAIFrames=sithThingTest_AllocAIFrames
    stdHashtbl_New=sithThingTest_HashtblNew
    stdHashtbl_Free=sithThingTest_HashtblFree
    stdHashtbl_Add=sithThingTest_HashtblAdd
    stdHashtbl_Find=sithThingTest_HashtblFind
    stdConfig_GetInt=sithThingTest_ConfigGetInt
    stdConfig_Contains=sithThingTest_ConfigContains
    stdConfig_SetInt=sithThingTest_ConfigSetInt
    sithGetPerformanceLevel=sithThingTest_GetPerformanceLevel
    sithIntersect_IsSphereInSector=sithThingTest_IsSphereInSector
    sithSoundMixer_StopAllSoundsThing=sithThingTest_StopAllSoundsThing
    stdComm_IsGameActive=sithThingTest_IsGameActive
    stdComm_IsGameHost=sithThingTest_IsGameHost
)

set_property(SOURCE "${SITH_WORLD_DIR}/../std/General/stdConffile.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
    stdMemory_Malloc=sithThingTest_MemoryMalloc
    stdMemory_Free=sithThingTest_MemoryFree
)

set_property(SOURCE "${SITH_WORLD_DIR}/World/Tests/sithThingTest.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
    J3D_LOCAL_RDROID_RTI
)

set_property(SOURCE "${SITH_WORLD_DIR}/World/Tests/sithTemplateTest.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
    J3D_LOCAL_RDROID_RTI
)

set(SITH_WORLD_SUITES
    sithThingBinary
    sithThingParseArg
    sithTemplate
    sithVoice
    sithCog
)

sith_add_test_suite(sithWorldSuite
    SUITES      ${SITH_WORLD_SUITES}
    TESTS       ${SITH_WORLD_TEST_SOURCES}
    TESTED      ${SITH_WORLD_TESTED_SOURCES}
    SCAN        "${SITH_WORLD_DIR}/World/Tests/sithThingTestSupport.h"
    # Globals of the retail executable that only these tests define locally; the files built without the local
    # storage switch see the headers' macros for them
    EXCLUDE
        rdroid_g_identMatrix34
        std_g_pHS
        std_g_genBuffer
        stdConffile_g_aLine
        stdConffile_g_entry
    DEFINITIONS
        J3D_TEST
        J3D_LOCAL_SITH_RTI
        SITH_TEST_TV_DIR="${SITH_WORLD_DIR}/World/Tests/tv"
)
