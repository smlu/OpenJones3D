set(SITH_TEST_SOURCES
    sithTestRunner.c
    ../World/Tests/sithTemplateTest.c
    ../World/Tests/sithThingTest.c
    ../World/Tests/sithVoiceTest.c
)

set(SITH_TESTED_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../World/sithTemplate.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../World/sithThing.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../World/sithVoice.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../std/General/stdConffile.c"
)

set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../World/sithTemplate.c" PROPERTY COMPILE_DEFINITIONS
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

set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../World/sithThing.c" PROPERTY COMPILE_DEFINITIONS
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

set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../std/General/stdConffile.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
    stdMemory_Malloc=sithThingTest_MemoryMalloc
    stdMemory_Free=sithThingTest_MemoryFree
)

set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../World/Tests/sithThingTest.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
    J3D_LOCAL_RDROID_RTI
)

set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../World/Tests/sithTemplateTest.c" PROPERTY COMPILE_DEFINITIONS
    J3D_LOCAL_RTI
    J3D_LOCAL_RDROID_RTI
)

add_executable(sithTests
    ${SITH_TEST_SOURCES}
    ${SITH_TESTED_SOURCES}
)

target_link_libraries(sithTests PRIVATE
    sith
    sound
    unity
    w32util
)

target_compile_definitions(sithTests PRIVATE
    J3D_TEST
    J3D_LOCAL_SITH_RTI
    SITH_TEST_TV_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../World/Tests/tv"
)

set_target_properties(sithTests PROPERTIES
    FOLDER "Tests/Libs/sith"
)

source_group("Tests" FILES ${SITH_TEST_SOURCES})
source_group("Tested Sources" FILES ${SITH_TESTED_SOURCES})

set(SITH_TEST_BACKEND_LABEL "")
if (J3D_DIRECTX6)
    set(SITH_TEST_BACKEND_LABEL "DirectX6")
elseif (J3D_DIRECTX9)
    set(SITH_TEST_BACKEND_LABEL "DirectX9")
endif()

add_test(NAME sith COMMAND sithTests)
# The local fixtures perform bounded in-memory I/O; allow slower hosts and file reads.
set_tests_properties(sith PROPERTIES
    LABELS "unit;sith;${SITH_TEST_BACKEND_LABEL}"
    TIMEOUT 60
)
