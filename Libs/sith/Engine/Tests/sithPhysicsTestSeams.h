#ifndef SITH_PHYSICS_TEST_SEAMS_H
#define SITH_PHYSICS_TEST_SEAMS_H

#include "sithEngineTestAddrs.h"
#include <rdroid/Math/rdVector.h>

// Include the real inline helper before renaming its call sites. Observing the
// integration step bounds a regression to the old unsigned negative-step loop.
void J3DAPI sithPhysicsTest_MultAcc3(rdVector3* pAcc, const rdVector3* pVector, float scale);
#define rdVector_MultAcc3 sithPhysicsTest_MultAcc3

#endif
