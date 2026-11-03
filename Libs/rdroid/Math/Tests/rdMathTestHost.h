#ifndef RDROID_TESTS_RDMATHTESTHOST_H
#define RDROID_TESTS_RDMATHTESTHOST_H

#include <rdroid/Main/rdroid.h>

extern tHostServices* rdMathTest_pHS;

// Redirect only the test build's host slot to avoid collisions with ASLR.
// Production math retains its normal RTI binding.
#undef rdroid_g_pHS_ADDR
// Added: A standalone build has no retail image and no fixed addresses: rdroid_g_pHS is a real
// variable, so the tests bind their host services to that slot. Windows behavior is unchanged.
#define rdroid_g_pHS_ADDR ((uintptr_t)&rdroid_g_pHS)

#endif // RDROID_TESTS_RDMATHTESTHOST_H
