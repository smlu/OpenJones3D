#include <unity_fixture.h>

#include <stddef.h>

#include <std/General/stdUtil.h>
#include <std/Win95/stdShader.h>

#include "stdGeneralTest.h"

#ifdef J3D_DIRECTX9
#   include <std/Win95/DX9/stdShaderDX9.h>
#endif

TEST_GROUP(stdShader);

TEST_SETUP(stdShader)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdShader)
{
#ifdef J3D_DIRECTX9
    stdShader_Shutdown();
#endif
    StdGeneralTest_Shutdown();
}

TEST(stdShader, TestPublicEnumValuesAndInvalidHandle)
{
    TEST_ASSERT_EQUAL_size_t(0u, STDSHADER_INVALIDHANDLE);

    TEST_ASSERT_EQUAL_INT(1, STDSHADER_TYPE_VERTEX);
    TEST_ASSERT_EQUAL_INT(2, STDSHADER_TYPE_PIXEL);
    TEST_ASSERT_EQUAL_INT(3, STDSHADER_MAX_TYPES);

    TEST_ASSERT_EQUAL_INT(1, STDSHADER_PARAM_FLOAT);
    TEST_ASSERT_EQUAL_INT(2, STDSHADER_PARAM_VECTOR2);
    TEST_ASSERT_EQUAL_INT(3, STDSHADER_PARAM_VECTOR3);
    TEST_ASSERT_EQUAL_INT(4, STDSHADER_PARAM_VECTOR4);
    TEST_ASSERT_EQUAL_INT(5, STDSHADER_PARAM_MATRIX);
    TEST_ASSERT_EQUAL_INT(6, STDSHADER_PARAM_TEXTURE);
}

TEST(stdShader, TestVectorMatrixAndParamStorageLayout)
{
    StdShaderParam param = { 0 };
    StdShader shader    = { 0 };

    TEST_ASSERT_EQUAL_size_t(sizeof(float) * 4u, sizeof(StdShaderVector));
    TEST_ASSERT_EQUAL_size_t(sizeof(StdShaderVector), sizeof(StdShaderViewport));
    TEST_ASSERT_EQUAL_size_t(sizeof(float) * 16u, sizeof(StdShaderMatrix));
    TEST_ASSERT_EQUAL_size_t(64u, sizeof(param.aName));
    TEST_ASSERT_EQUAL_size_t(64u, sizeof(shader.aName));
    TEST_ASSERT_EQUAL_size_t((size_t)STDSHADER_MAX_TYPES, STD_ARRAYLEN(shader.aTypeParams));
    TEST_ASSERT_TRUE(offsetof(StdShaderParam, value) > offsetof(StdShaderParam, registerIndex));

    param.registerIndex          = 7u;
    param.value.type             = STDSHADER_PARAM_VECTOR4;
    param.value.value.vector[0]  = 1.0f;
    param.value.value.vector[1]  = 2.0f;
    param.value.value.vector[2]  = 3.0f;
    param.value.value.vector[3]  = 4.0f;

    TEST_ASSERT_EQUAL_size_t(7u, param.registerIndex);
    TEST_ASSERT_EQUAL_INT(STDSHADER_PARAM_VECTOR4, param.value.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 1.0f, param.value.value.vector[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 2.0f, param.value.value.vector[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 3.0f, param.value.value.vector[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 4.0f, param.value.value.vector[3]);
}

TEST(stdShader, TestParamValueUnionStoresAllPublicTypes)
{
    StdShaderParamValue value = { 0 };
    tSysTexture* pTexture    = (tSysTexture*)(uintptr_t)0x12345678u;

    value.type             = STDSHADER_PARAM_FLOAT;
    value.value.floatValue = 1.25f;
    TEST_ASSERT_EQUAL_INT(STDSHADER_PARAM_FLOAT, value.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 1.25f, value.value.floatValue);

    value.type            = STDSHADER_PARAM_VECTOR2;
    value.value.vector[0] = 2.0f;
    value.value.vector[1] = 3.0f;
    TEST_ASSERT_EQUAL_INT(STDSHADER_PARAM_VECTOR2, value.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 2.0f, value.value.vector[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 3.0f, value.value.vector[1]);

    value.type               = STDSHADER_PARAM_MATRIX;
    value.value.matrix[0][0] = 1.0f;
    value.value.matrix[1][1] = 2.0f;
    value.value.matrix[2][2] = 3.0f;
    value.value.matrix[3][3] = 4.0f;
    TEST_ASSERT_EQUAL_INT(STDSHADER_PARAM_MATRIX, value.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 1.0f, value.value.matrix[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 2.0f, value.value.matrix[1][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 3.0f, value.value.matrix[2][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 4.0f, value.value.matrix[3][3]);

    value.type           = STDSHADER_PARAM_TEXTURE;
    value.value.pTexture = pTexture;
    TEST_ASSERT_EQUAL_INT(STDSHADER_PARAM_TEXTURE, value.type);
    TEST_ASSERT_EQUAL_PTR(pTexture, value.value.pTexture);
}

#ifdef J3D_DIRECTX9
TEST(stdShader, TestDirectX9ShaderConstants)
{
    TEST_ASSERT_EQUAL_UINT32(64u, STDSHADERDX9_MAX_SHADERS);
    TEST_ASSERT_EQUAL_UINT32(0u, STDSHADERDX9_VS_VIEWPORT_REGISTER);
    TEST_ASSERT_EQUAL_UINT32(24u, STDSHADERDX9_VS_CONSTANTS_START_REGISTER);
    TEST_ASSERT_EQUAL_UINT32(0u, STDSHADERDX9_PS_FOGPARAM_REGISTER);
    TEST_ASSERT_EQUAL_UINT32(1u, STDSHADERDX9_PS_FOGCOLOR_REGISTER);
    TEST_ASSERT_EQUAL_UINT32(8u, STDSHADERDX9_PS_CONSTANTS_START_REGISTER);
    TEST_ASSERT_EQUAL_UINT32(216u, STDSHADERDX9_MAX_PS_PARAMS);
    TEST_ASSERT_EQUAL_UINT32(16u, STDSHADERDX9_MAX_PS_SAMPLERS);
    TEST_ASSERT_EQUAL_UINT32(4u, STDSHADERDX9_MAX_VS_SAMPLERS);
    TEST_ASSERT_EQUAL_size_t(offsetof(StdShaderDX9, base), 0u);
}

TEST(stdShader, TestDirectX9ShaderStartupAndNoDeviceOpenFailure)
{
    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_FALSE(stdShader_Startup());
    StdGeneralTest_ClearAllocationFailures();

    StdGeneralTest_FailNextAllocations(1);
    TEST_ASSERT_FALSE(stdShader_Startup());
    StdGeneralTest_ClearAllocationFailures();

    TEST_ASSERT_TRUE(stdShader_Startup());
    TEST_ASSERT_TRUE(stdShader_Startup());
    TEST_ASSERT_FALSE(stdShader_Open());
    TEST_ASSERT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, stdShader_CompileAndCreate("closed", "source", "source"));
    TEST_ASSERT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, stdShader_GetShader("missing"));

    stdShader_Close();
    stdShader_Shutdown();
    TEST_ASSERT_FALSE(stdShader_Open());
}
#endif

TEST_GROUP_RUNNER(stdShader)
{
    RUN_TEST_CASE(stdShader, TestPublicEnumValuesAndInvalidHandle);
    RUN_TEST_CASE(stdShader, TestVectorMatrixAndParamStorageLayout);
    RUN_TEST_CASE(stdShader, TestParamValueUnionStoresAllPublicTypes);
#ifdef J3D_DIRECTX9
    RUN_TEST_CASE(stdShader, TestDirectX9ShaderConstants);
    RUN_TEST_CASE(stdShader, TestDirectX9ShaderStartupAndNoDeviceOpenFailure);
#endif
}
