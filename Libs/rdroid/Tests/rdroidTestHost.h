#ifndef RDROID_TESTS_RDROIDTESTHOST_H
#define RDROID_TESTS_RDROIDTESTHOST_H

#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/addresses.h>
#include <rdroid/types.h>

// Force-included into every rdroid test translation unit, before inline helpers expand RTI addresses.
// Redirects the host services and the identity matrix from their retail addresses, which hold other data in the
// test process, to the storage in rdroidTest.c. Production builds keep their RTI bindings.

extern tHostServices* rdroidTest_pHS;
#undef rdroid_g_pHS_ADDR
#define rdroid_g_pHS_ADDR ((uintptr_t)&rdroidTest_pHS)

// rdMatrix_Identity34 copies the identity matrix from its address
extern const rdMatrix34 rdroidTest_identMatrix34;
#undef rdroid_g_identMatrix34_ADDR
#define rdroid_g_identMatrix34_ADDR ((uintptr_t)&rdroidTest_identMatrix34)

#endif // RDROID_TESTS_RDROIDTESTHOST_H
