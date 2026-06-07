#include "sithCogExec.h"

#include <j3dcore/j3dhook.h>

#include <sith/Cog/sithCogParse.h>
#include <sith/Devices/sithConsole.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Math/rdVector.h>

#include <sound/Sound.h>

#include <std/General/stdUtil.h>

#include <math.h>

void sithCogExec_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCogExec_Execute);
    J3D_HOOKFUNC(sithCogExec_ExecuteMessage);
    J3D_HOOKFUNC(sithCogExec_PopSymbol);
    J3D_HOOKFUNC(sithCogExec_PopFlex);
    J3D_HOOKFUNC(sithCogExec_PopInt);
    J3D_HOOKFUNC(sithCogExec_PopArray);
    J3D_HOOKFUNC(sithCogExec_PopVector);
    J3D_HOOKFUNC(sithCogExec_PopCog);
    J3D_HOOKFUNC(sithCogExec_PopThing);
    J3D_HOOKFUNC(sithCogExec_TryPopThing);
    J3D_HOOKFUNC(sithCogExec_PopTemplate);
    J3D_HOOKFUNC(sithCogExec_PopSound);
    J3D_HOOKFUNC(sithCogExec_PopSector);
    J3D_HOOKFUNC(sithCogExec_PopSurface);
    J3D_HOOKFUNC(sithCogExec_PopMaterial);
    J3D_HOOKFUNC(sithCogExec_PopModel3);
    J3D_HOOKFUNC(sithCogExec_PopKeyframe);
    J3D_HOOKFUNC(sithCogExec_PopAIClass);
    J3D_HOOKFUNC(sithCogExec_PopString);
    J3D_HOOKFUNC(sithCogExec_PushStack);
    J3D_HOOKFUNC(sithCogExec_PushInt);
    J3D_HOOKFUNC(sithCogExec_PushFlex);
    J3D_HOOKFUNC(sithCogExec_PushVector);
    J3D_HOOKFUNC(sithCogExec_GetOpCode);
    J3D_HOOKFUNC(sithCogExec_ResetStack);
    J3D_HOOKFUNC(sithCogExec_PushCallstack);
    J3D_HOOKFUNC(sithCogExec_PopCallstack);
    J3D_HOOKFUNC(sithCogExec_PopStack);
    J3D_HOOKFUNC(sithCogExec_FunctionCallOp);
    J3D_HOOKFUNC(sithCogExec_RetOp);
    J3D_HOOKFUNC(sithCogExec_PushNumberOp);
    J3D_HOOKFUNC(sithCogExec_PushVectorOp);
    J3D_HOOKFUNC(sithCogExec_CallOp);
    J3D_HOOKFUNC(sithCogExec_JmpnzOp);
    J3D_HOOKFUNC(sithCogExec_JmpzOp);
    J3D_HOOKFUNC(sithCogExec_JmpOp);
    J3D_HOOKFUNC(sithCogExec_ArrayOp);
    J3D_HOOKFUNC(sithCogExec_NegFalseOps);
    J3D_HOOKFUNC(sithCogExec_AssignOp);
    J3D_HOOKFUNC(sithCogExec_IntererOps);
    J3D_HOOKFUNC(sithCogExec_FloatOps);
    J3D_HOOKFUNC(sithCogExec_GetSymbolValue);
}

void sithCogExec_ResetGlobals(void)
{}

void J3DAPI sithCogExec_Execute(SithCog* pCog)
{
    SITH_ASSERTREL(pCog != NULL);
    SITH_ASSERTREL(pCog->execPos < pCog->pScript->codeSize);

    pCog->status = SITHCOG_STATUS_EXECUTING;
    while ( pCog->status == SITHCOG_STATUS_EXECUTING )
    {
        int opcode = sithCogExec_GetOpCode(pCog);
        switch ( opcode )
        {
            case SITHCOGEXEC_OPCODE_NOP:
                SITH_ASSERTREL(0);
                break;

            case SITHCOGEXEC_OPCODE_PUSHINT:
            case SITHCOGEXEC_OPCODE_PUSHFLOAT:
            case SITHCOGEXEC_OPCODE_PUSHSYMBOL:
                sithCogExec_PushNumberOp(pCog, opcode);
                break;

            case SITHCOGEXEC_OPCODE_ARRAYIDX:
                sithCogExec_ArrayOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_CALLFUNC:
                sithCogExec_FunctionCallOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_ASSIGN:
                sithCogExec_AssignOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_PUSHVECTOR:
                sithCogExec_PushVectorOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_CMPFALSE:
            case SITHCOGEXEC_OPCODE_NEG:
                sithCogExec_NegFalseOps(pCog, opcode);
                break;

            case SITHCOGEXEC_OPCODE_CMPAND:
            case SITHCOGEXEC_OPCODE_CMPOR:
            case SITHCOGEXEC_OPCODE_CMPNE:
            case SITHCOGEXEC_OPCODE_AND:
            case SITHCOGEXEC_OPCODE_OR:
            case SITHCOGEXEC_OPCODE_XOR:
                sithCogExec_IntererOps(pCog, opcode);
                break;

            case SITHCOGEXEC_OPCODE_JZ:
                sithCogExec_JmpzOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_JNZ:
                sithCogExec_JmpnzOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_JMP:
                sithCogExec_JmpOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_RET:
                sithCogExec_RetOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_CALL:
                sithCogExec_CallOp(pCog);
                break;

            default:
                sithCogExec_FloatOps(pCog, opcode);
                break;
        }
    }
}

void J3DAPI sithCogExec_ExecuteMessage(SithCog* pCog, size_t handlerNum)
{
    SITH_ASSERTREL(pCog != NULL);
    SITH_ASSERT(pCog->pScript != NULL); // Added
    SITH_ASSERT(handlerNum < pCog->pScript->numHandlers); // Added

    SithCogScript* pScript = pCog->pScript;
    if ( pCog->pScript->aHandlers[handlerNum].codeOffset < 0 )
    {
        SITHLOG_ERROR("Cog %s: Message type %d not defined in the code.\n",
            pCog->aName, pCog->pScript->aHandlers[handlerNum].type  // Fixed: User correct handler message type in log format. OG: was using handlerNum
        );
        return;
    }

    if ( pCog->status == SITHCOG_STATUS_STOPPED )
    {
        sithCogExec_ResetStack(pCog);
    }
    else
    {
        if ( pCog->status == SITHCOG_STATUS_EXECUTING )
        {
            pCog->status = SITHCOG_STATUS_EXECUTION_SUSPENDED;
        }

        sithCogExec_PushCallstack(pCog);
    }

    pCog->execPos     = pScript->aHandlers[handlerNum].codeOffset;
    pCog->execMsgType = pScript->aHandlers[handlerNum].type;

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        SITHCONSOLE_PRINTF("Cog %s: execution started.\n", pCog->aName);
    }

    sithCogExec_Execute(pCog);
    if ( pCog->status == SITHCOG_STATUS_EXECUTION_SUSPENDED )
    {
        pCog->status = SITHCOG_STATUS_EXECUTING;
    }
}

int J3DAPI sithCogExec_PopSymbol(SithCog* pCog, SithCogSymbolValue* pVal)
{
    SITH_ASSERTREL(pCog != NULL);
    SITH_ASSERTREL(pVal != NULL);

    if ( !sithCogExec_PopStack(pCog, pVal) )
    {
        return 0;
    }

    SithCogSymbolValue dest;
    *pVal = *sithCogExec_GetSymbolValue(&dest, pCog, pVal);
    return 1;
}

float J3DAPI sithCogExec_PopFlex(SithCog* pCog)
{
    SITH_ASSERTREL(pCog != NULL);

    SithCogSymbolValue value;
    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        return 0.0f;
    }

    SithCogSymbolValue retVal;
    value = *sithCogExec_GetSymbolValue(&retVal, pCog, &value);
    if ( value.type == SITHCOG_VALUE_INT )
    {
        return (float)value.val.intValue;
    }

    if ( value.type == SITHCOG_VALUE_FLOAT )
    {
        return value.val.floatValue;
    }

    SITHLOG_ERROR("Could not convert value type %d to flex.\n", value.type);
    return 0.0f;
}

int J3DAPI sithCogExec_PopInt(SithCog* pCog)
{
    SITH_ASSERTREL(pCog != NULL);

    SithCogSymbolValue value;
    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        return -1;
    }

    SithCogSymbolValue retVal;
    value = *sithCogExec_GetSymbolValue(&retVal, pCog, &value);
    if ( value.type == SITHCOG_VALUE_FLOAT )
    {
        return (int)value.val.floatValue;
    }

    if ( value.type == SITHCOG_VALUE_INT )
    {
        return value.val.intValue;
    }

    SITHLOG_ERROR("Could not convert value type %d to INT.\n", value.type);
    return -1;
}

int J3DAPI sithCogExec_PopArray(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithCogSymbolValue value;
    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        return 0;
    }

    if ( value.type == SITHCOG_VALUE_SYMBOLID )
    {
        return value.val.intValue;
    }

    return 0;
}

int J3DAPI sithCogExec_PopVector(SithCog* pCog, rdVector3* vec)
{
    SITH_ASSERTREL(pCog != NULL);
    SITH_ASSERTREL(vec != NULL);

    SithCogSymbolValue value;
    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        rdVector_Zero3(vec);
        return 0;
    }

    SithCogSymbolValue retValue;
    value = *sithCogExec_GetSymbolValue(&retValue, pCog, &value);
    if ( value.type != SITHCOG_VALUE_VECTOR )
    {
        SITHLOG_ERROR("Cog %s: Expected vector, got int/flex constant.\n", pCog->aName);
        rdVector_Zero3(vec);
        return 0;
    }

    *vec = value.val.vecValue;
    return 1;
}

SithCog* J3DAPI sithCogExec_PopCog(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    int index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
        SITH_ASSERTREL(sithWorld_g_pStaticWorld); // Added: Added null check like in other cases
        pWorld = sithWorld_g_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( pWorld && index >= 0 && index < pWorld->numCogs )
    {
        return &pWorld->aCogs[index];
    }

    SITHLOG_ERROR("Cog %s: Bad cog index %d on stack.\n", pCog->aName, index);
    return NULL;
}

SithThing* J3DAPI sithCogExec_PopThing(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    int thidx = sithCogExec_PopInt(pCog);
    if ( pWorld && thidx >= 0 && thidx <= pWorld->lastThingIdx && pWorld->aThings[thidx].type )
    {
        return &pWorld->aThings[thidx];
    }

    SITHLOG_ERROR("Cog %s: Bad thing index %d on stack.\n", pCog->aName, thidx);
    return NULL;
}

SithThing* J3DAPI sithCogExec_TryPopThing(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    int index = sithCogExec_PopInt(pCog);
    if ( index < 0 )
    {
        return NULL;
    }

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( pWorld && index <= pWorld->lastThingIdx && pWorld->aThings[index].type )
    {
        return &pWorld->aThings[index];
    }

    SITHLOG_ERROR("Cog %s: Bad thing index %d on stack.\n", pCog->aName, index);
    return NULL;
}

SithThing* J3DAPI sithCogExec_PopTemplate(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    int index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
        SITH_ASSERTREL(sithWorld_g_pStaticWorld);
        pWorld = sithWorld_g_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( pWorld && index >= 0 && index < pWorld->numThingTemplates )
    {
        return &pWorld->aThingTemplates[index];
    }

    SITHLOG_ERROR("Cog %s: Bad template index %d on stack.\n", pCog->aName, index);
    return NULL;
}

tSoundHandle J3DAPI sithCogExec_PopSound(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    int index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return 0;
    }

    return Sound_GetSoundHandle(index);
}

SithSector* J3DAPI sithCogExec_PopSector(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    int index = sithCogExec_PopInt(pCog);
    if ( pWorld && index >= 0 && index < pWorld->numSectors )
    {
        return &pWorld->aSectors[index];
    }

    SITHLOG_ERROR("Cog %s: Bad sector index %d on stack.\n", pCog->aName, index);
    return NULL;
}

SithSurface* J3DAPI sithCogExec_PopSurface(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    int index = sithCogExec_PopInt(pCog);
    if ( pWorld && index >= 0 && index < pWorld->numSurfaces )
    {
        return &pWorld->aSurfaces[index];
    }

    SITHLOG_ERROR("Cog %s: Bad surface index %d on stack.\n", pCog->aName, index);
    return NULL;
}

rdMaterial* J3DAPI sithCogExec_PopMaterial(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    int index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
        SITH_ASSERTREL(sithWorld_g_pStaticWorld);
        pWorld = sithWorld_g_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( pWorld && index >= 0 && index < pWorld->numMaterials && pWorld->aMaterials[index].aName[0] )
    {
        return &pWorld->aMaterials[index];
    }

    SITHLOG_ERROR("Cog %s: Bad material index %d on stack.\n", pCog->aName, index);
    return NULL;
}

rdModel3* J3DAPI sithCogExec_PopModel3(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    int index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return 0;
    }

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
        SITH_ASSERTREL(sithWorld_g_pStaticWorld);
        pWorld = sithWorld_g_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( pWorld && index >= 0 && index < pWorld->numModels )
    {
        return &pWorld->aModels[index];
    }

    SITHLOG_ERROR("Cog %s: Bad model index %d on stack.\n", pCog->aName, index);
    return 0;
}

rdKeyframe* J3DAPI sithCogExec_PopKeyframe(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    int index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
        SITH_ASSERTREL(sithWorld_g_pStaticWorld);
        pWorld = sithWorld_g_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( pWorld && index >= 0 && index < pWorld->numKeyframes )
    {
        return &pWorld->aKeyframes[index];
    }

    SITHLOG_ERROR("Cog %s: Bad keyframe index %d on stack.\n", pCog->aName, index); // Fixed: Fixed typo in description, it said '...model index..'
    return NULL;
}

SithAIClass* J3DAPI sithCogExec_PopAIClass(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    int index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    SithWorld* pWorld  = sithWorld_g_pCurrentWorld;
    if ( pWorld && index >= 0 && index < pWorld->numAIClasses )
    {
        return &pWorld->aAIClasses[index];
    }

    SITHLOG_ERROR("Cog %s: Bad AI index %d on stack.\n", pCog->aName, index);
    return NULL;
}

void* J3DAPI sithCogExec_PopPointer(SithCog* pCog)
{
    SITH_ASSERTREL(pCog != NULL);

    SithCogSymbolValue returnVal;
    if ( !sithCogExec_PopStack(pCog, &returnVal) )
    {
        return NULL;
    }

    SithCogSymbolValue retVal;
    returnVal = *sithCogExec_GetPointerValue(&retVal, pCog, &returnVal);
    SITH_ASSERTREL(returnVal.type == SITHCOG_VALUE_POINTER);
    return returnVal.val.pointerValue;
}

const char* J3DAPI sithCogExec_PopString(SithCog* pCog)
{
    SITH_ASSERTREL(pCog != NULL);

    SithCogSymbolValue value;
    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        return NULL;
    }

    if ( value.type != SITHCOG_VALUE_SYMBOLID )
    {
        SITHLOG_ERROR("Cog %s: Expected string, got constant of type %d.\n", pCog->aName, value.type);
        return NULL;
    }

    SithCogSymbol* pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, value.val.intValue);
    SITH_ASSERTREL(pSymbol != NULL);

    if ( pSymbol->value.type == SITHCOG_VALUE_STRING )
    {
        return pSymbol->value.val.pString;
    }

    SITHLOG_ERROR("Cog %s: Expected string, got constant of type %d.\n", pCog->aName, value.type);

#ifdef J3D_DEBUG
    // TODO: Verify this is correct, found in debug version
    value.val.intValue = SITHCOG_VALUE_POINTER;
    return value.val.pointerValue;
#else
    return NULL;
#endif
}

void J3DAPI sithCogExec_PushStack(SithCog* pCog, SithCogSymbolValue* pValue)
{
    SITH_ASSERTREL(pCog != NULL);
    SITH_ASSERTREL(pValue != NULL);

    if ( pCog->stackSize >= STD_ARRAYLEN(pCog->stack) ) // Fixed: Added GE check (was EQ) to make absolutely sure the stack doesn't exceed
    {
        SITHLOG_ERROR("Stack overflow in script %s.  Bottom element discarded.\n", pCog->pScript->aName);
        STD_COPYMEM(pCog->stack, &pCog->stack[1], (STD_ARRAYLEN(pCog->stack) - 1) * sizeof(SithCogSymbolValue));
        static_assert((STD_ARRAYLEN(pCog->stack) - 1) * sizeof(SithCogSymbolValue) == 4080, "STD_ARRAYLEN(pCog->stack) - 1 * sizeof(SithCogSymbolValue) == 4080");
        --pCog->stackSize; /// TODO: make sure the call stack size is STD_ARRAYLEN(pCog->stack) - 1
    }

    SITH_ASSERTREL(pCog->stackSize < STD_ARRAYLEN(pCog->stack));

    pCog->stack[pCog->stackSize] = *pValue;
    ++pCog->stackSize;
}

void J3DAPI sithCogExec_PushInt(SithCog* pCog, int val)
{
    SithCogSymbolValue value;
    value.type         = SITHCOG_VALUE_INT;
    value.val.intValue = val;
    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_PushFlex(SithCog* pCog, float value)
{
    SithCogSymbolValue val;
    val.type           = SITHCOG_VALUE_FLOAT;
    val.val.floatValue = value;
    sithCogExec_PushStack(pCog, &val);
}

void J3DAPI sithCogExec_PushVector(SithCog* pCog, const rdVector3* vec)
{
    SithCogSymbolValue value;
    value.type         = SITHCOG_VALUE_VECTOR;
    value.val.vecValue = *vec;
    sithCogExec_PushStack(pCog, &value);
}

SithCogExecOpcode J3DAPI sithCogExec_GetOpCode(SithCog* pCog)
{
    SITH_ASSERTREL(pCog != NULL);

    SithCogScript* pScript = pCog->pScript;
    SITH_ASSERTREL(pScript != NULL);

    if ( pCog->execPos >= pScript->codeSize - 1 )
    {
        SITHLOG_ERROR("End of code reached, cannot get opcode.\n");
        return SITHCOGEXEC_OPCODE_RET;
    }

    return pScript->pCode[pCog->execPos++];
}

void J3DAPI sithCogExec_ResetStack(SithCog* pCog)
{
    SITH_ASSERTREL(pCog != NULL);
    if ( pCog->stackSize )
    {
        pCog->stackSize = 0;
    }
}

void J3DAPI sithCogExec_PushCallstack(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    if ( pCog->callDepth >= STD_ARRAYLEN(pCog->callstack) ) // Fixed: Add GE check (was EQ) to make sure the callDepth never exceeds callstack size
    {
        SITHLOG_ERROR("Cog %s: tried to exceed max execution depth.\n", pCog->aName);
        return;
    }

    pCog->callstack[pCog->callDepth].execPos       = pCog->execPos;
    pCog->callstack[pCog->callDepth].execStatus    = pCog->status;
    pCog->callstack[pCog->callDepth].statusParam   = pCog->statusParams[0];// Should we copy all params?
    pCog->callstack[pCog->callDepth++].execMsgType = pCog->execMsgType;
}

void J3DAPI sithCogExec_PopCallstack(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    if ( pCog->callDepth > 0 )
    {
        --pCog->callDepth;
        pCog->execPos         = pCog->callstack[pCog->callDepth].execPos;
        pCog->status          = pCog->callstack[pCog->callDepth].execStatus;
        pCog->statusParams[0] = pCog->callstack[pCog->callDepth].statusParam;
        pCog->execMsgType     = pCog->callstack[pCog->callDepth].execMsgType;
    }
    else
    {
        pCog->status = SITHCOG_STATUS_STOPPED;
    }
}

int J3DAPI sithCogExec_PopStack(SithCog* pCog, SithCogSymbolValue* pValue)
{
    SITH_ASSERTREL(pCog != NULL);
    SITH_ASSERTREL(pValue != NULL);
    if ( pCog->stackSize < 1 )
    {
        SITHLOG_ERROR("Cog %s: PopStack failed -- stack size < 1.\n", pCog->aName);
        return 0;
    }

    *pValue = pCog->stack[--pCog->stackSize];
    return 1;
}

void J3DAPI sithCogExec_FunctionCallOp(SithCog* pCog)
{
    SithCogFunctionType pfCogFunction;
    SithCogSymbolValue value;

    SithCogSymbol* pSymbol = NULL;
    if ( sithCogExec_PopStack(pCog, &value)
        && value.type == SITHCOG_VALUE_SYMBOLID
        && (pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, value.val.intValue)) != 0
        && pSymbol->value.type == SITHCOG_VALUE_POINTER
        && (pfCogFunction = (SithCogFunctionType)pSymbol->value.val.pointerValue) != NULL )
    {
        pfCogFunction(pCog);
    }
    else if ( pSymbol )
    {
        if ( pSymbol->pName )
        {
            SITHLOG_ERROR("Cog %s: Unrecognized or malformed function call, addr %d, symbol %s.\n", pCog->aName, pCog->execPos, pSymbol->pName);
        }
        else
        {
            SITHLOG_ERROR("Cog %s: Unrecognized or malformed function call, addr %d\n", pCog->aName, pCog->execPos);
        }
    }
    else
    {
        SITHLOG_ERROR("Cog %s: Unrecognized or malformed function call, addr %d\n", pCog->aName, pCog->execPos);
    }
}

void J3DAPI sithCogExec_RetOp(SithCog* pCog)
{
    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        SITHCONSOLE_PRINTF("Cog %s: Returned from depth %d.\n", pCog->aName, pCog->callDepth);
    }

    sithCogExec_PopCallstack(pCog);
}

void J3DAPI sithCogExec_PushNumberOp(SithCog* pCog, int opcode)
{
    SithCogSymbolValue value;
    value.val.intValue = sithCogExec_GetOpCode(pCog);
    switch ( opcode )
    {
        case SITHCOGEXEC_OPCODE_PUSHINT:
            value.type = SITHCOG_VALUE_INT;
            break;

        case SITHCOGEXEC_OPCODE_PUSHFLOAT:
            value.type = SITHCOG_VALUE_FLOAT;
            break;

        case SITHCOGEXEC_OPCODE_PUSHSYMBOL:
            value.type = SITHCOG_VALUE_SYMBOLID;
            break;

        default:
            SITH_ASSERTREL(0);
            break;
    }

    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_PushVectorOp(SithCog* pCog)
{
    SithCogSymbolValue value;

    SITH_ASSERTREL(pCog && pCog->pScript);
    rdVector_Copy3(&value.val.vecValue, (const rdVector3*)&pCog->pScript->pCode[pCog->execPos]);
    pCog->execPos += sizeof(value.val.vecValue) / sizeof(*pCog->pScript->pCode);
    static_assert(sizeof(value.val.vecValue) / sizeof(*pCog->pScript->pCode) == 3, "sizeof(value.val.vecValue) / sizeof(*pCog->pScript->pCode) == 3");

    value.type = SITHCOG_VALUE_VECTOR;
    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_CallOp(SithCog* pCog)
{
    if ( pCog->callDepth >= STD_ARRAYLEN(pCog->callstack) )
    {
        SITHLOG_ERROR("Cog %s: Max call depth exceeded.\n", pCog->aName);
        return;
    }

    size_t lable = sithCogExec_GetOpCode(pCog);
    if ( lable >= pCog->pScript->codeSize )
    {
        SITHLOG_ERROR("Script %s: Illegal CALL opcode - undefined label?\n", pCog->aName);
        return;
    }

    sithCogExec_PushCallstack(pCog);
    pCog->execPos = lable;
}

void J3DAPI sithCogExec_JmpnzOp(SithCog* pCog)
{
    size_t jumpTarget = sithCogExec_GetOpCode(pCog);
    SITH_ASSERTREL(jumpTarget < pCog->pScript->codeSize);
    if ( sithCogExec_PopInt(pCog) != 0 )
    {
        pCog->execPos = jumpTarget;
    }
}

void J3DAPI sithCogExec_JmpzOp(SithCog* pCog)
{
    size_t jumpTarget = sithCogExec_GetOpCode(pCog);
    SITH_ASSERTREL(jumpTarget < pCog->pScript->codeSize);

    if ( sithCogExec_PopInt(pCog) == 0 ) // if 0 is on stack
    {
        pCog->execPos = jumpTarget;
    }
}

void J3DAPI sithCogExec_JmpOp(SithCog* pCog)
{
    size_t jumpTarget = sithCogExec_GetOpCode(pCog);
    SITH_ASSERTREL(jumpTarget < pCog->pScript->codeSize);
    pCog->execPos = jumpTarget;
}

void J3DAPI sithCogExec_ArrayOp(SithCog* pCog)
{
    int startIdx = sithCogExec_PopInt(pCog);
    int offset = sithCogExec_PopArray(pCog);

    SithCogSymbolValue value;
    value.type         = SITHCOG_VALUE_SYMBOLID;
    value.val.intValue = startIdx + offset;
    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_NegFalseOps(SithCog* pCog, int opcode)
{
    SithCogSymbolValue value;
    if ( opcode == SITHCOGEXEC_OPCODE_CMPFALSE )
    {
        value.type         = SITHCOG_VALUE_INT;
        value.val.intValue = sithCogExec_PopInt(pCog) == 0;
    }
    else if ( opcode == SITHCOGEXEC_OPCODE_NEG )
    {
        float fltval = sithCogExec_PopFlex(pCog);
        value.type           = SITHCOG_VALUE_FLOAT;
        value.val.floatValue = -fltval;
    }
    else
    {
        SITH_ASSERTREL(0);
    }

    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_AssignOp(SithCog* pCog)
{
    SithCogSymbolValue variable, value;
    if ( !sithCogExec_PopSymbol(pCog, &value) || !sithCogExec_PopStack(pCog, &variable) )
    {
        SITHLOG_ERROR("Cog %s: Error in assignment statement.\n", pCog->aName);
        return;
    }

    if ( variable.type != SITHCOG_VALUE_SYMBOLID )
    {
        SITHLOG_ERROR("Cog %s: Illegal assignment attempted.\n", pCog->aName);
        return;
    }

    SithCogSymbol* pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, variable.val.intValue);
    pSymbol->value = value;
}

void J3DAPI sithCogExec_IntererOps(SithCog* pCog, int opcode)
{
    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_INT;

    int b = sithCogExec_PopInt(pCog);
    int a = sithCogExec_PopInt(pCog);

    switch ( opcode )
    {
        case SITHCOGEXEC_OPCODE_CMPAND:
            value.val.intValue = a && b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_CMPOR:
            value.val.intValue = a || b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_CMPNE:
            value.val.intValue = a != b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_AND:
            value.val.intValue = b & a;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_OR:
            value.val.intValue = b | a;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_XOR:
            value.val.intValue = b ^ a;
            sithCogExec_PushStack(pCog, &value);
            break;

        default:
            SITHLOG_ERROR("Unrecognized opcode passed to IntegerOps.\n");
            break;
    }
}

void J3DAPI sithCogExec_FloatOps(SithCog* pCog, int opcode)
{
    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_FLOAT;

    float b = sithCogExec_PopFlex(pCog);
    float a = sithCogExec_PopFlex(pCog);

    switch ( opcode )
    {
        case SITHCOGEXEC_OPCODE_ADD:
            value.val.floatValue = a + b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_SUB:
            value.val.floatValue = a - b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_MUL:
            value.val.floatValue = a * b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_DIV:
            if ( b == 0.0f )
            {
                value.val.intValue = 0;
            }
            else
            {
                value.val.floatValue = a / b;
            }

            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_MOD:
            value.val.floatValue = fmodf(a, b);
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_CMPGT:
            value.type         = SITHCOG_VALUE_INT;
            value.val.intValue = a > (double)b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_CMPLS:
            value.type         = SITHCOG_VALUE_INT;
            value.val.intValue = a < (double)b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_CMPEQ:
            value.type         = SITHCOG_VALUE_INT;
            value.val.intValue = a == b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_CMPLE:
            value.type         = SITHCOG_VALUE_INT;
            value.val.intValue = a <= (double)b;
            sithCogExec_PushStack(pCog, &value);
            break;

        case SITHCOGEXEC_OPCODE_CMPGE:
            value.type         = SITHCOG_VALUE_INT;
            value.val.intValue = a >= (double)b;
            sithCogExec_PushStack(pCog, &value);
            break;

        default:
            SITHLOG_ERROR("Unrecognized opcode passed to FloatOps.\n");
            break;
    }
}

SithCogSymbolValue* J3DAPI sithCogExec_GetSymbolValue(SithCogSymbolValue* pDest, SithCog* pCog, SithCogSymbolValue* pValue)
{
    SITH_ASSERTREL(pCog && pValue);

    SithCogSymbol* pSymbol;
    if ( pValue->type == SITHCOG_VALUE_SYMBOLID )
    {
        pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, pValue->val.intValue);
        SITH_ASSERTREL(pSymbol != NULL);
        pValue = &pSymbol->value;
    }

    if ( pValue->type != SITHCOG_VALUE_POINTER )
    {
        *pDest = *pValue;
    }
    else
    {
        SITH_ASSERTREL(pValue->val.pointerValue != NULL);
        // TODO: Verify if pointer value should be assigned instead
        pDest->type         = SITHCOG_VALUE_INT;
        pDest->val.intValue = *(uint32_t*)pValue->val.pointerValue;
    }

    return pDest;
}

SithCogSymbolValue* J3DAPI sithCogExec_GetPointerValue(SithCogSymbolValue* pDest, SithCog* pCog, SithCogSymbolValue* pValue)
{
    pDest->type = SITHCOG_VALUE_POINTER;
    if ( pValue->type == SITHCOG_VALUE_SYMBOLID )
    {
        SithCogSymbol* pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, (unsigned int)pValue->val.pointerValue);
        SITH_ASSERTREL(pSymbol != NULL);
        if ( pSymbol->value.type == SITHCOG_VALUE_POINTER )
        {
            pDest->val.pointerValue = pSymbol->value.val.pointerValue;
        }
        else
        {
            pDest->val.pointerValue = &pSymbol->value.val;
        }
    }
    else if ( pValue->type == SITHCOG_VALUE_POINTER )
    {
        *pDest = *pValue;
    }
    else
    {
        SITH_ASSERTREL(0);
    }

    return pDest;
}