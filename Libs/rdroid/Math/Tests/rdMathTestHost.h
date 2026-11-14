#ifndef RDROID_TESTS_RDMATHTESTHOST_H
#define RDROID_TESTS_RDMATHTESTHOST_H

#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/addresses.h>
#include <rdroid/types.h>

extern tHostServices* rdMathTest_pHS;

// Redirect only the test build's host slot to avoid collisions with ASLR.
// Production math retains its normal RTI binding.
#undef rdroid_g_pHS_ADDR
#define rdroid_g_pHS_ADDR ((uintptr_t)&rdMathTest_pHS)

// rdMatrix_Identity34 copies the identity matrix from its retail address, which holds other data in the test process.
extern const rdMatrix34 rdMathTest_identMatrix34;
#undef rdroid_g_identMatrix34_ADDR
#define rdroid_g_identMatrix34_ADDR ((uintptr_t)&rdMathTest_identMatrix34)

#endif // RDROID_TESTS_RDMATHTESTHOST_H
