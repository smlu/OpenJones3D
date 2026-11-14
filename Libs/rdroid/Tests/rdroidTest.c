#include "rdroidTestHost.h"

// Storage of the globals redirected in rdroidTestHost.h
tHostServices* rdroidTest_pHS;

const rdMatrix34 rdroidTest_identMatrix34 = {
    { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }
};
