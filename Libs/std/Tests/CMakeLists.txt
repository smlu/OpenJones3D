set(STD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(STD_GENERAL_SOURCE_DIR "${STD_SOURCE_DIR}/General")
set(STD_GENERAL_TEST_DIR "${STD_GENERAL_SOURCE_DIR}/Tests")
set(STD_WIN95_SOURCE_DIR "${STD_SOURCE_DIR}/Win95")
set(STD_WIN95_TEST_DIR "${STD_WIN95_SOURCE_DIR}/Tests")

set(STD_TEST_SUPPORT_SOURCES
    stdTestRunner.c
    stdGeneralTest.c
)

set(STD_SYSTEM_TEST_SUPPORT_SOURCES
    stdSystemTestRunner.c
    stdGeneralTest.c
    "${STD_WIN95_TEST_DIR}/std3DSystemTestSupport.c"
    "${STD_WIN95_TEST_DIR}/stdWin95SystemTestSupport.c"
)

set(STD_DISRUPTIVE_SYSTEM_TEST_SUPPORT_SOURCES
    "${STD_WIN95_TEST_DIR}/stdDisruptiveSystemTestRunner.c"
    stdGeneralTest.c
    "${STD_WIN95_TEST_DIR}/std3DSystemTestSupport.c"
    "${STD_WIN95_TEST_DIR}/stdWin95SystemTestSupport.c"
)

set(STD_GENERAL_TEST_SOURCES
    "${STD_GENERAL_TEST_DIR}/stdTest.c"
    "${STD_GENERAL_TEST_DIR}/stdMemoryTest.c"
    "${STD_GENERAL_TEST_DIR}/stdMathTest.c"
    "${STD_GENERAL_TEST_DIR}/stdUtilTest.c"
    "${STD_GENERAL_TEST_DIR}/stdHashtblTest.c"
    "${STD_GENERAL_TEST_DIR}/stdFnamesTest.c"
    "${STD_GENERAL_TEST_DIR}/stdLinkListTest.c"
    "${STD_GENERAL_TEST_DIR}/stdEffectTest.c"
    "${STD_GENERAL_TEST_DIR}/stdColorTest.c"
    "${STD_GENERAL_TEST_DIR}/stdCircBufTest.c"
    "${STD_GENERAL_TEST_DIR}/stdConffileTest.c"
    "${STD_GENERAL_TEST_DIR}/stdConfigTest.c"
    "${STD_GENERAL_TEST_DIR}/stdFileUtilTest.c"
    "${STD_GENERAL_TEST_DIR}/stdJSONTest.c"
    "${STD_GENERAL_TEST_DIR}/stdBmpTest.c"
    "${STD_GENERAL_TEST_DIR}/stdPlatformTest.c"
    "${STD_GENERAL_TEST_DIR}/stdStrTableTest.c"
)

set(STD_WIN95_TEST_SOURCES
    "${STD_WIN95_TEST_DIR}/stdWin95TestSuite.c"
    "${STD_WIN95_TEST_DIR}/stdGobTest.c"
    "${STD_WIN95_TEST_DIR}/std3DTest.c"
    "${STD_WIN95_TEST_DIR}/std3DTestVectorTest.c"
    "${STD_WIN95_TEST_DIR}/stdConsoleTest.c"
    "${STD_WIN95_TEST_DIR}/stdControlTest.c"
    "${STD_WIN95_TEST_DIR}/stdDisplayTest.c"
    "${STD_WIN95_TEST_DIR}/stdShaderTest.c"
    "${STD_WIN95_TEST_DIR}/stdWin95Test.c"
)

if (J3D_DIRECTX9)
    list(APPEND STD_WIN95_TEST_SOURCES
        "${STD_WIN95_TEST_DIR}/stdCommTest.c"
        "${STD_WIN95_TEST_DIR}/stdWin95DirectX9Test.c"
    )
elseif(J3D_DIRECTX6)
    list(APPEND STD_WIN95_TEST_SOURCES
        "${STD_WIN95_TEST_DIR}/stdCommDX6Test.c"
        "${STD_WIN95_TEST_DIR}/stdWin95DirectX6Test.c"
    )
endif()

set(STD_WIN95_SYSTEM_TEST_SOURCES
    "${STD_WIN95_TEST_DIR}/stdWin95SystemTest.c"
)

set(STD_DIRECTX_TEST_LABEL "")
if (J3D_DIRECTX9)
    set(STD_DIRECTX_TEST_LABEL "DirectX9")
    list(APPEND STD_WIN95_SYSTEM_TEST_SOURCES
        "${STD_WIN95_TEST_DIR}/stdControlDX9SystemTest.c"
        "${STD_WIN95_TEST_DIR}/stdDisplayDX9SystemTest.c"
        "${STD_WIN95_TEST_DIR}/std3DX9SystemTest.c"
        "${STD_WIN95_TEST_DIR}/stdShaderDX9SystemTest.c"
    )
elseif(J3D_DIRECTX6)
    set(STD_DIRECTX_TEST_LABEL "DirectX6")
    list(APPEND STD_WIN95_SYSTEM_TEST_SOURCES
        "${STD_WIN95_TEST_DIR}/stdControlDX6SystemTest.c"
        "${STD_WIN95_TEST_DIR}/stdDisplayDX6SystemTest.c"
        "${STD_WIN95_TEST_DIR}/std3DX6SystemTest.c"
    )
endif()

set(STD_WIN95_DISRUPTIVE_SYSTEM_TEST_SOURCES "")
if (J3D_DIRECTX9)
    list(APPEND STD_WIN95_DISRUPTIVE_SYSTEM_TEST_SOURCES
        "${STD_WIN95_TEST_DIR}/stdDisplayDX9DisruptiveSystemTest.c"
    )
elseif(J3D_DIRECTX6)
    list(APPEND STD_WIN95_DISRUPTIVE_SYSTEM_TEST_SOURCES
        "${STD_WIN95_TEST_DIR}/stdDisplayDX6DisruptiveSystemTest.c"
    )
endif()

set(STD_TESTED_SOURCES
    "${STD_GENERAL_SOURCE_DIR}/std.c"
    "${STD_GENERAL_SOURCE_DIR}/stdBmp.c"
    "${STD_GENERAL_SOURCE_DIR}/stdConffile.c"
    "${STD_GENERAL_SOURCE_DIR}/stdConfig.c"
    "${STD_GENERAL_SOURCE_DIR}/stdFileUtil.c"
    "${STD_GENERAL_SOURCE_DIR}/stdJSON.c"
    "${STD_GENERAL_SOURCE_DIR}/stdMath.c"
    "${STD_GENERAL_SOURCE_DIR}/stdMemory.c"
    "${STD_GENERAL_SOURCE_DIR}/stdPlatform.c"
    "${STD_GENERAL_SOURCE_DIR}/stdStrTable.c"
    "${STD_GENERAL_SOURCE_DIR}/stdUtil.c"
    "${STD_GENERAL_SOURCE_DIR}/stdHashtbl.c"
    "${STD_GENERAL_SOURCE_DIR}/stdFnames.c"
    "${STD_GENERAL_SOURCE_DIR}/stdLinkList.c"
    "${STD_GENERAL_SOURCE_DIR}/stdEffect.c"
    "${STD_GENERAL_SOURCE_DIR}/stdColor.c"
    "${STD_GENERAL_SOURCE_DIR}/stdCircBuf.c"
    "${STD_WIN95_SOURCE_DIR}/stdConsole.c"
    "${STD_WIN95_SOURCE_DIR}/stdGob.c"
    "${STD_WIN95_SOURCE_DIR}/stdWin95.c"
)

if (J3D_DIRECTX9)
    list(APPEND STD_TESTED_SOURCES
        "${STD_WIN95_SOURCE_DIR}/stdComm.c"
        "${STD_WIN95_SOURCE_DIR}/DX9/stdControlDX9.c"
        "${STD_WIN95_SOURCE_DIR}/DX9/std3DX9.c"
        "${STD_WIN95_SOURCE_DIR}/DX9/stdDisplayDX9.c"
        "${STD_WIN95_SOURCE_DIR}/DX9/stdShaderDX9.c"
    )
elseif(J3D_DIRECTX6)
    list(APPEND STD_TESTED_SOURCES
        "${STD_WIN95_SOURCE_DIR}/DX6/stdCommDX6.c"
        "${STD_WIN95_SOURCE_DIR}/DX6/stdControlDX6.c"
        "${STD_WIN95_SOURCE_DIR}/DX6/std3DX6.c"
        "${STD_WIN95_SOURCE_DIR}/DX6/stdDisplayDX6.c"
    )
endif()

set_source_files_properties("${STD_WIN95_SOURCE_DIR}/stdConsole.c" PROPERTIES
    COMPILE_DEFINITIONS "stdConsole_WriteConsole=stdConsole_WriteConsoleReal"
)

add_executable(stdTests
    ${STD_TEST_SUPPORT_SOURCES}
    ${STD_GENERAL_TEST_SOURCES}
    ${STD_WIN95_TEST_SOURCES}
    ${STD_TESTED_SOURCES}
)

target_compile_definitions(stdTests PRIVATE
    J3D_TEST
    J3D_LOCAL_RTI
    STD_GENERAL_TEST_TV_DIR="${STD_GENERAL_TEST_DIR}/tv"
    STD_WIN95_TEST_TV_DIR="${STD_WIN95_TEST_DIR}/tv"
)

target_include_directories(stdTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(stdTests PRIVATE
    j3dcore
    j3dTestSupport
    jansson::jansson
    Dbghelp
    Gdi32
    Ole32
    Winmm
)

if (J3D_DIRECTX6)
    target_link_libraries(stdTests PRIVATE
        DirectX61c
    )
elseif(J3D_DIRECTX9)
    target_link_libraries(stdTests PRIVATE
        d3d9
        d3dcompiler
        dinput8
        dxguid
        Xinput
        wbemuuid
        OleAut32
    )
endif()

set_target_properties(stdTests PROPERTIES
    FOLDER "Tests/Libs/std"
)

source_group("Test Support" FILES ${STD_TEST_SUPPORT_SOURCES})
source_group("Tests" FILES ${STD_GENERAL_TEST_SOURCES})
source_group("Tests\\Win95" FILES ${STD_WIN95_TEST_SOURCES})
source_group("Tested Sources" FILES ${STD_TESTED_SOURCES})

add_test(NAME std COMMAND stdTests)
set_tests_properties(std PROPERTIES
    LABELS "unit;std;${STD_DIRECTX_TEST_LABEL}"
)

if (JONES3D_BUILD_SYSTEM_TESTS)
    add_executable(stdSystemTests
        ${STD_SYSTEM_TEST_SUPPORT_SOURCES}
        ${STD_WIN95_SYSTEM_TEST_SOURCES}
        ${STD_TESTED_SOURCES}
    )

    target_compile_definitions(stdSystemTests PRIVATE
        J3D_TEST
        J3D_LOCAL_RTI
        STD_GENERAL_TEST_TV_DIR="${STD_GENERAL_TEST_DIR}/tv"
        STD_WIN95_TEST_TV_DIR="${STD_WIN95_TEST_DIR}/tv"
    )

    target_include_directories(stdSystemTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_link_libraries(stdSystemTests PRIVATE
        j3dcore
        j3dTestSupport
        jansson::jansson
        Dbghelp
        Gdi32
        Ole32
        Winmm
    )

    if (J3D_DIRECTX6)
        target_link_libraries(stdSystemTests PRIVATE
            DirectX61c
        )
    elseif(J3D_DIRECTX9)
        target_link_libraries(stdSystemTests PRIVATE
            d3d9
            d3dcompiler
            dinput8
            dxguid
            Xinput
            wbemuuid
            OleAut32
        )
    endif()

    set_target_properties(stdSystemTests PROPERTIES
        FOLDER "Tests/Libs/std"
    )

    source_group("System Test Support" FILES ${STD_SYSTEM_TEST_SUPPORT_SOURCES})
    source_group("System Tests\\Win95" FILES ${STD_WIN95_SYSTEM_TEST_SOURCES})

    add_test(NAME std.System COMMAND stdSystemTests)
    set_tests_properties(std.System PROPERTIES
        LABELS "system;std;Win95;device;${STD_DIRECTX_TEST_LABEL}"
    )
endif()

if (JONES3D_BUILD_DISRUPTIVE_SYSTEM_TESTS)
    add_executable(stdDisruptiveSystemTests
        ${STD_DISRUPTIVE_SYSTEM_TEST_SUPPORT_SOURCES}
        ${STD_WIN95_DISRUPTIVE_SYSTEM_TEST_SOURCES}
        ${STD_TESTED_SOURCES}
    )

    target_compile_definitions(stdDisruptiveSystemTests PRIVATE
        J3D_TEST
        J3D_LOCAL_RTI
        STD_GENERAL_TEST_TV_DIR="${STD_GENERAL_TEST_DIR}/tv"
        STD_WIN95_TEST_TV_DIR="${STD_WIN95_TEST_DIR}/tv"
    )

    target_include_directories(stdDisruptiveSystemTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_link_libraries(stdDisruptiveSystemTests PRIVATE
        j3dcore
        j3dTestSupport
        jansson::jansson
        Dbghelp
        Gdi32
        Ole32
        Winmm
    )

    if (J3D_DIRECTX6)
        target_link_libraries(stdDisruptiveSystemTests PRIVATE
            DirectX61c
        )
    elseif(J3D_DIRECTX9)
        target_link_libraries(stdDisruptiveSystemTests PRIVATE
            d3d9
            d3dcompiler
            dinput8
            dxguid
            Xinput
            wbemuuid
            OleAut32
        )
    endif()

    set_target_properties(stdDisruptiveSystemTests PROPERTIES
        FOLDER "Tests/Libs/std"
    )

    source_group("Disruptive System Test Support" FILES ${STD_DISRUPTIVE_SYSTEM_TEST_SUPPORT_SOURCES})
    source_group("Disruptive System Tests\\Win95" FILES ${STD_WIN95_DISRUPTIVE_SYSTEM_TEST_SOURCES})

    add_test(NAME std.DisruptiveSystem COMMAND stdDisruptiveSystemTests)
    set_tests_properties(std.DisruptiveSystem PROPERTIES
        LABELS "system;disruptive;std;Win95;device;${STD_DIRECTX_TEST_LABEL}"
    )
endif()
