
#ifndef JONES3D_HOOK_H
#define JONES3D_HOOK_H
#include <stdint.h>
#include <stdio.h>
#include <j3dcore/j3d.h>

/**
* @brief Macro calls a function at a far address.
*
* @param func_addr Address of the function.
* @param func_type Type of the function.
* @param ... Function parameters (can be empty for functions with no parameters).
*/
#define J3D_CALLFUNCFAR(func_addr, func_type, ...) \
    _J3D_CALLFUNCFAR_HELPER(func_addr, func_type, ##__VA_ARGS__)

#define _J3D_CALLFUNCFAR_HELPER(func_addr, func_type, ...) \
    ((func_type)func_addr)(__VA_ARGS__)

/**
* @brief Simplified macro wrapper for CALLFUNCFAR.
*
* @param func_name Name of the function.
*                  The global function address variable (func_name_ADDR) and type variable (func_name_TYPE) are derived from name.
* @param ... Function parameters (can be empty for functions with no parameters).
*/
#define J3D_TRAMPOLINE_CALL(func_name, ...) \
    J3D_CALLFUNCFAR(func_name##_ADDR, func_name##_TYPE, ##__VA_ARGS__)

/**
* @brief Redirects the original function to a specified hook function.
*
* This macro is used to hook a function by redirecting the original function
* to a new function. It derives the address of the original function by
* appending `_ADDR` to the function name and then replaces it with the new
* function provided as the argument.
*
* @param func The function to which the original function is redirected.
*             The macro derives the address variable of the original function
*             as `func_ADDR`, which is expected to be globally defined or
*             included in the source file where this macro is used.
*/
#define J3D_HOOKFUNC(func) \
    J3DHookFunction(func##_ADDR, (void*)func)

/**
* @brief Declares a variable at a specific memory address.
*
* This macro is used to declare a variable of a given type at a predefined
* memory address. It's typically used for accessing variables in memory-mapped
* regions or in external processes.
*
* @param var_name The name of the variable (without the _ADDR suffix).
* @param var_type The type of the variable.
*
* @return A reference to the variable at the specified address.
*
* @note The address of the variable should be defined elsewhere as var_name_ADDR.
*/
#define J3D_DECL_FAR_VAR(var_name, var_type) \
    (*(var_type *)(var_name##_ADDR))

/**
* @brief Declares an array variable at a specific memory address.
*
* This macro is used to declare an array variable of a given type at a predefined
* memory address. It's typically used for accessing array variables in memory-mapped
* regions or in external processes.
*
* @param var_name The name of the array variable (without the _ADDR suffix).
* @param var_type The type of the array, including its dimensions.
*
* @return A reference to the array at the specified address.
*
* @note The address of the array should be defined elsewhere as var_name_ADDR.
*/
#define J3D_DECL_FAR_ARRAYVAR(var_name, var_type) \
    (*(var_type)(var_name##_ADDR))

J3D_EXTERN_C_START

/**
 * @brief Structure to hold the context for hooking
 */
    typedef struct {
    DWORD oldProtect;
    uintptr_t startAddress;
    size_t size;
} J3DHookContext;

/**
 * @brief Begins the hooking context by changing memory protection to read-write
 *
 * @param pCtx The pointer to a J3DHookContext structure
 * @param startAddress The start address of the region to modify
 * @param endAddress The end address of the region to modify
 * @return true if successful, false otherwise
 */
inline bool J3DStartHookContext(J3DHookContext* pCtx, uintptr_t startAddress, uintptr_t endAddress)
{
    if ( endAddress <= startAddress ) {
        return false;
    }
    pCtx->startAddress = startAddress;
    pCtx->size = endAddress - startAddress;
    return VirtualProtect((LPVOID)pCtx->startAddress, pCtx->size, PAGE_EXECUTE_READWRITE, &pCtx->oldProtect);
}

/**
 * @brief Ends the hooking context by restoring memory protection
 *
 * @param pCtx The pointer to a J3DHookContext structure
 * @return true if successful, false otherwise
 */
inline bool J3DEndHookContext(J3DHookContext* pCtx)
{
    DWORD temp;
    return VirtualProtect((LPVOID)pCtx->startAddress, pCtx->size, pCtx->oldProtect, &temp);
}

/**
 * @brief Installs hook on function to redirect all calls to the new function
 *
 * @param pFuncAddr Address of the original function
 * @param pHookFunc The new function to redirect to
 * @return true if hooking was successful, false otherwise
 */
static bool J3DHookFunction(intptr_t pFuncAddr, void* pHookFunc)
{
    if ( pFuncAddr == (intptr_t)pHookFunc )
    {
        printf("WARNING J3DHookFunction: Attempted to hook function at address %x to itself!\n", pFuncAddr);
        return false;
    }

    // Install hook in-place
    *(uint8_t*)(pFuncAddr) = 0xE9; // JMP opcode
    *(uint32_t*)(pFuncAddr + 1) = ((uintptr_t)pHookFunc - pFuncAddr - 5); // relative address to h

    // Flush instruction cache to ensure the CPU picks up the change
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)pFuncAddr, 5);

    return true;
}

J3D_EXTERN_C_END
#endif //JONES3D_HOOK_H
