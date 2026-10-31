// Small command-line replacement for the shader compiler of the MSVC build (fxc).
// Compiles one preprocessed DX9 shader through D3DCompile and writes its byte code
// out as a C header file, for example:
//
//     hlslc default_vs.pp.hlsl vs_3_0 std_default_vs.h std_default_vs
//
// Used by the MinGW build, where the executables run under Wine. See Libs/std/CMakeLists.txt.

#define COBJMACROS

#include <d3dcompiler.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool hlslc_ReadFile(const char* pName, char** ppBuffer, size_t* pSize)
{
    FILE* pFile;
    long size;

    pFile = fopen(pName, "rb");
    if ( !pFile )
    {
        fprintf(stderr, "hlslc: cannot open '%s'.\n", pName);
        return false;
    }

    if ( fseek(pFile, 0, SEEK_END) != 0 || (size = ftell(pFile)) < 0 )
    {
        fprintf(stderr, "hlslc: cannot determine the size of '%s'.\n", pName);
        fclose(pFile);
        return false;
    }

    char* pBuffer = malloc((size_t)size);
    if ( !pBuffer )
    {
        fprintf(stderr, "hlslc: out of memory reading '%s'.\n", pName);
        fclose(pFile);
        return false;
    }

    if ( fseek(pFile, 0, SEEK_SET) != 0 || fread(pBuffer, 1, (size_t)size, pFile) != (size_t)size )
    {
        fprintf(stderr, "hlslc: cannot read '%s'.\n", pName);
        free(pBuffer);
        fclose(pFile);
        return false;
    }

    fclose(pFile);
    *ppBuffer = pBuffer;
    *pSize = (size_t)size;
    return true;
}

static bool hlslc_WriteHeader(const char* pName, const char* pVariableName, const unsigned char* pCode, size_t size)
{
    FILE* pFile = fopen(pName, "w");
    if ( !pFile )
    {
        fprintf(stderr, "hlslc: cannot open '%s' for writing.\n", pName);
        return false;
    }

    if ( fprintf(pFile, "const unsigned char %s[] = {\n", pVariableName) < 0 )
    {
        fprintf(stderr, "hlslc: cannot write '%s'.\n", pName);
        fclose(pFile);
        return false;
    }

    for ( size_t value = 0; value < size; value++ )
    {
        if ( fprintf(pFile, "%u", (unsigned int)pCode[value]) < 0 )
        {
            fprintf(stderr, "hlslc: cannot write '%s'.\n", pName);
            fclose(pFile);
            return false;
        }

        if ( value + 1 < size )
        {
            fputc(',', pFile);
        }

        if ( (value + 1) % 16 == 0 )
        {
            fputc('\n', pFile);
        }
    }

    if ( size % 16 != 0 )
    {
        fputc('\n', pFile);
    }

    if ( fprintf(pFile, "};\n") < 0 || fclose(pFile) != 0 )
    {
        fprintf(stderr, "hlslc: cannot write '%s'.\n", pName);
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    if ( argc != 5 )
    {
        fprintf(stderr, "Usage: hlslc <input.hlsl> <target> <output.h> <variableName>\n");
        return 1;
    }

    const char* pInputName    = argv[1];
    const char* pTarget       = argv[2];
    const char* pOutputName   = argv[3];
    const char* pVariableName = argv[4];

    char*  pBuffer;
    size_t size;
    if ( !hlslc_ReadFile(pInputName, &pBuffer, &size) )
    {
        return 1;
    }

    ID3DBlob* pCode   = NULL;
    ID3DBlob* pErrors = NULL;
    HRESULT   hr      = D3DCompile(pBuffer, size, pInputName, NULL, NULL, "main", pTarget, 0, 0, &pCode, &pErrors);
    free(pBuffer);

    if ( pErrors )
    {
        fwrite(ID3D10Blob_GetBufferPointer(pErrors), 1, ID3D10Blob_GetBufferSize(pErrors), stderr);
        fputc('\n', stderr);
        ID3D10Blob_Release(pErrors);
    }

    if ( FAILED(hr) )
    {
        fprintf(stderr, "hlslc: D3DCompile of '%s' for target '%s' failed with HRESULT 0x%08lX.\n",
            pInputName, pTarget, (unsigned long)hr);
        if ( pCode )
        {
            ID3D10Blob_Release(pCode);
        }
        return 1;
    }

    if ( !hlslc_WriteHeader(pOutputName, pVariableName, (const unsigned char*)ID3D10Blob_GetBufferPointer(pCode),
        ID3D10Blob_GetBufferSize(pCode)) )
    {
        ID3D10Blob_Release(pCode);
        return 1;
    }

    ID3D10Blob_Release(pCode);
    return 0;
}
