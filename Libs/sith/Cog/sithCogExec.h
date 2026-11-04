#ifndef SITH_SITHCOGEXEC_H
#define SITH_SITHCOGEXEC_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

typedef enum eSithCogExecOpcode
{
    SITHCOGEXEC_OPCODE_NOP        = 0,
    SITHCOGEXEC_OPCODE_PUSHINT    = 1,
    SITHCOGEXEC_OPCODE_PUSHFLOAT  = 2,
    SITHCOGEXEC_OPCODE_PUSHSYMBOL = 3,
    SITHCOGEXEC_OPCODE_ARRAYIDX   = 4,
    SITHCOGEXEC_OPCODE_CALLFUNC   = 5,
    SITHCOGEXEC_OPCODE_ASSIGN     = 6,
    SITHCOGEXEC_OPCODE_PUSHVECTOR = 7,
    SITHCOGEXEC_OPCODE_ADD        = 8,
    SITHCOGEXEC_OPCODE_SUB        = 9,
    SITHCOGEXEC_OPCODE_MUL        = 10,
    SITHCOGEXEC_OPCODE_DIV        = 11,
    SITHCOGEXEC_OPCODE_MOD        = 12,
    SITHCOGEXEC_OPCODE_CMPFALSE   = 13,
    SITHCOGEXEC_OPCODE_NEG        = 14,
    SITHCOGEXEC_OPCODE_CMPGT      = 15,
    SITHCOGEXEC_OPCODE_CMPLS      = 16,
    SITHCOGEXEC_OPCODE_CMPEQ      = 17,
    SITHCOGEXEC_OPCODE_CMPLE      = 18,
    SITHCOGEXEC_OPCODE_CMPGE      = 19,
    SITHCOGEXEC_OPCODE_CMPAND     = 20,
    SITHCOGEXEC_OPCODE_CMPOR      = 21,
    SITHCOGEXEC_OPCODE_CMPNE      = 22,
    SITHCOGEXEC_OPCODE_AND        = 23,
    SITHCOGEXEC_OPCODE_OR         = 24,
    SITHCOGEXEC_OPCODE_XOR        = 25,
    SITHCOGEXEC_OPCODE_JZ         = 26,
    SITHCOGEXEC_OPCODE_JNZ        = 27,
    SITHCOGEXEC_OPCODE_JMP        = 28,
    SITHCOGEXEC_OPCODE_RET        = 29,
    SITHCOGEXEC_OPCODE_UNUSED_30  = 30,
    SITHCOGEXEC_OPCODE_CALL       = 31,
} SithCogExecOpcode;

void J3DAPI sithCogExec_Execute(SithCog* pCog);
void J3DAPI sithCogExec_ExecuteMessage(SithCog* pCog, size_t handlerNum);

int J3DAPI sithCogExec_PopSymbol(SithCog* pCog, SithCogSymbolValue* pVal);
float J3DAPI sithCogExec_PopFlex(SithCog* pCog);
int J3DAPI sithCogExec_PopInt(SithCog* pCog);
int J3DAPI sithCogExec_PopArray(SithCog* pCog);
int J3DAPI sithCogExec_PopVector(SithCog* pCog, rdVector3* vec);
SithCog* J3DAPI sithCogExec_PopCog(SithCog* pCog);
SithThing* J3DAPI sithCogExec_PopThing(SithCog* pCog);
SithThing* J3DAPI sithCogExec_TryPopThing(SithCog* pCog);
SithThing* J3DAPI sithCogExec_PopTemplate(SithCog* pCog);
tSoundHandle J3DAPI sithCogExec_PopSound(SithCog* pCog);
SithSector* J3DAPI sithCogExec_PopSector(SithCog* pCog);
SithSurface* J3DAPI sithCogExec_PopSurface(SithCog* pCog);
rdMaterial* J3DAPI sithCogExec_PopMaterial(SithCog* pCog);
rdModel3* J3DAPI sithCogExec_PopModel3(SithCog* pCog);
rdKeyframe* J3DAPI sithCogExec_PopKeyframe(SithCog* pCog);
SithAIClass* J3DAPI sithCogExec_PopAIClass(SithCog* pCog);
void* J3DAPI sithCogExec_PopPointer(SithCog* pCog); // Added
const char* J3DAPI sithCogExec_PopString(SithCog* pCog);

void J3DAPI sithCogExec_PushStack(SithCog* pCog, SithCogSymbolValue* pValue);
void J3DAPI sithCogExec_PushInt(SithCog* pCog, int val);
void J3DAPI sithCogExec_PushFlex(SithCog* pCog, float val);
void J3DAPI sithCogExec_PushVector(SithCog* pCog, const rdVector3* vec);

SithCogExecOpcode J3DAPI sithCogExec_GetOpCode(SithCog* pCog);
void J3DAPI sithCogExec_ResetStack(SithCog* pCog);
void J3DAPI sithCogExec_PushCallstack(SithCog* pCog);
void J3DAPI sithCogExec_PopCallstack(SithCog* pCog);
int J3DAPI sithCogExec_PopStack(SithCog* pCog, SithCogSymbolValue* pValue);

void J3DAPI sithCogExec_FunctionCallOp(SithCog* pCog); // calls function
void J3DAPI sithCogExec_RetOp(SithCog* pCog);
void J3DAPI sithCogExec_PushNumberOp(SithCog* pCog, int opcode);
void J3DAPI sithCogExec_PushVectorOp(SithCog* pCog);
void J3DAPI sithCogExec_CallOp(SithCog* pCog); // Calls label within script aka goto
void J3DAPI sithCogExec_JmpnzOp(SithCog* pCog); // Jump if non zero value is on stack
void J3DAPI sithCogExec_JmpzOp(SithCog* pCog); // Jump if zero is on stack
void J3DAPI sithCogExec_JmpOp(SithCog* pCog);
void J3DAPI sithCogExec_ArrayOp(SithCog* pCog);
void J3DAPI sithCogExec_NegFalseOps(SithCog* pCog, int opcode);
void J3DAPI sithCogExec_AssignOp(SithCog* pCog);
void J3DAPI sithCogExec_IntererOps(SithCog* pCog, int opcode);
void J3DAPI sithCogExec_FloatOps(SithCog* pCog, int opcode);

SithCogSymbolValue* J3DAPI sithCogExec_GetSymbolValue(SithCogSymbolValue* pDest, SithCog* pCog, SithCogSymbolValue* pValue);
SithCogSymbolValue* J3DAPI sithCogExec_GetPointerValue(SithCogSymbolValue* pDest, SithCog* pCog, SithCogSymbolValue* pValue); // Added

// Helper hooking functions
void sithCogExec_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCOGEXEC_H
