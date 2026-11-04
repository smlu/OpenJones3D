#include "sithPVS.h"

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>

#define SITHPVS_SER_NUM_PERLINE 16
#define SITHPVS_SER_LINESIZE    (sizeof(uint32_t) * SITHPVS_SER_NUM_PERLINE) // 64

void sithPVS_ResetGlobals(void)
{}

int J3DAPI sithPVS_WritePVSText(const SithWorld* pWorld)
{
    if ( !pWorld->aPVS )
    {
        return 0;
    }

    size_t num = pWorld->sizePVS / SITHPVS_SER_LINESIZE; // / 64
    if ( pWorld->sizePVS % SITHPVS_SER_LINESIZE )
    {
        ++num;
    }

    if ( stdConffile_WriteLine("######### PVS information ######\n") || stdConffile_WriteLine("SECTION: pvs\n") || stdConffile_Printf("PVS Size: %d\n", pWorld->sizePVS) )
    {
        return 1;
    }

    size_t curSize = 0;
    for ( size_t i = 0; i < num; ++i )
    {
        uint32_t* aPVS = (uint32_t*)&pWorld->aPVS[SITHPVS_SER_LINESIZE * i];
        for ( size_t j = 0; j < SITHPVS_SER_NUM_PERLINE && curSize < pWorld->sizePVS; ++j )
        {
            if ( stdConffile_Printf("%.8x ", &aPVS[j]) )
            {
                return 1;
            }

            curSize += sizeof(uint32_t);
        }

        if ( stdConffile_Printf("\n") )
        {
            return 1;
        }
    }

    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        if ( stdConffile_Printf("%x \n", pWorld->aSectors[i].pvsIdx) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithPVS_ReadPVSText(SithWorld* pWorld, int bSkip)
{
    J3D_UNUSED(bSkip); // TODO: maybe implement skip logic

    if ( pWorld->aPVS )
    {
        return 1;
    }

    int rlen = stdConffile_ScanLine("pvs size: %" J3D_SCN_SIZE, &pWorld->sizePVS);
    if ( rlen != 1 ) {
        if ( rlen < 0 ) {
            goto eof_error;
        }
        goto syntax_error;
    }

    size_t num = pWorld->sizePVS / SITHPVS_SER_LINESIZE;
    if ( pWorld->sizePVS % SITHPVS_SER_LINESIZE )
    {
        ++num;
    }

    pWorld->aPVS = (uint8_t*)STDMALLOC(num * SITHPVS_SER_LINESIZE);
    if ( !pWorld->aPVS )
    {
        SITHLOG_ERROR("Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
        return 1;
    }

    size_t curSize = 0;
    for ( size_t i = 0; i < num; ++i )
    {
        if ( !stdConffile_ReadArgs() )
        {
            goto eof_error;
        }

        size_t curArg = 0;
        uint32_t* aPvs = (uint32_t*)&pWorld->aPVS[SITHPVS_SER_LINESIZE * i];
        for ( size_t j = 0; j < SITHPVS_SER_NUM_PERLINE && curSize < pWorld->sizePVS; ++j )
        {
            if ( sscanf_s(stdConffile_g_entry.aArgs[curArg++].argValue, "%x", &aPvs[j]) != 1 )
            {
                goto syntax_error;
            }

            curSize += sizeof(uint32_t);
        }
    }

    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        if ( rlen = stdConffile_ScanLine("%x", &pWorld->aSectors[i].pvsIdx), rlen != 1 ) {
            if ( rlen < 0 ) {
                goto eof_error;
            }
            goto syntax_error;
        }
    }

    return 0; // success

eof_error:
    SITHLOG_ERROR("Error: Unexpected EOF in '%s'\n", stdConffile_GetFilename());
    return 1;

syntax_error:
    SITHLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
    return 1;
}

int J3DAPI sithPVS_WritePVSBinary(tFileHandle fh, const SithWorld* pWorld)
{
    uint32_t sizePVS = (uint32_t)pWorld->sizePVS; // Altered: the file holds 4 bytes, sizePVS is size_t
    if ( sith_g_pHS->pFileWrite(fh, &sizePVS, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 1;
    }

    return pWorld->sizePVS && sith_g_pHS->pFileWrite(fh, pWorld->aPVS, pWorld->sizePVS) != pWorld->sizePVS;
}

int J3DAPI sithPVS_ReadPVSBinary(tFileHandle fh, SithWorld* pWorld)
{
    uint32_t sizePVS; // Fixed: a 4-byte read straight into size_t only half fills it on 64-bit
    if ( sith_g_pHS->pFileRead(fh, &sizePVS, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 1;
    }

    pWorld->sizePVS = sizePVS;

    if ( !pWorld->sizePVS )
    {
        return 0;
    }

    pWorld->aPVS = (uint8_t*)STDMALLOC(pWorld->sizePVS);
    if ( !pWorld->aPVS )
    {
        return 1;
    }

    return sith_g_pHS->pFileRead(fh, pWorld->aPVS, pWorld->sizePVS) != pWorld->sizePVS;
}

void J3DAPI sithPVS_SetTable(uint8_t* paAdjoinTable, const uint8_t* aElements, size_t numAdjoints)
{
    size_t i = 0;
    while ( i < numAdjoints )
    {
        size_t el = *aElements++;
        if ( (el & 0x80) != 0 )
        {
            if ( i + (el & ~0xFFFFFF80) + 1 > numAdjoints )
            {
                el = numAdjoints - i - 1;
            }

            for ( size_t j = 0; j < (el & ~0xFFFFFF80) + 1; ++j )
            {
                *paAdjoinTable++ = *aElements++;
                ++i;
            }
        }
        else
        {
            if ( i + el + 3 > numAdjoints )
            {
                el = numAdjoints - i - 3;
            }

            for ( size_t j = 0; j < el + 3; ++j )
            {
                *paAdjoinTable++ = 0;
                ++i;
            }
        }
    }

    SITH_ASSERTREL(i <= numAdjoints);
}