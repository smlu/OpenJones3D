
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

// Added: A standalone build has no retail executable, so reaching a fixed address is a compile error naming the symbol.
#define J3D_TRAMPOLINE_CALL(func_name, ...) J3D_STANDALONE_FAR_CALL_##func_name

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
// Added: A standalone build has no retail executable, so hooks cannot be installed and the
// J3DHookFunction helper is not compiled; the hook macro expands to nothing.
#define J3D_HOOKFUNC(func)

// Added: A standalone build has no retail executable, so reaching a fixed address is a compile error naming the symbol.
#define J3D_DECL_FAR_VAR(var_name, var_type) J3D_STANDALONE_FAR_GLOBAL_##var_name
#define J3D_DECL_FAR_ARRAYVAR(var_name, var_type) J3D_STANDALONE_FAR_GLOBAL_##var_name

J3D_EXTERN_C_START

// Added: The rest of this header hooks bytes in the retail executable, so everything below uses
// VirtualProtect/FlushInstructionCache and the Win32 types. A standalone build has no retail
// executable and does not compile or use it.

J3D_EXTERN_C_END
#endif //JONES3D_HOOK_H
