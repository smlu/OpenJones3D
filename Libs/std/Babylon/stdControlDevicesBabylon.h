// Added: the engine-key to Babylon-key mapping of stdControlDevicesBabylon.cpp, adapter-internal.
// The one table lives in that file; both directions of a lookup go through it, and the Babylon
// column's stdControl test walks it to check that it pairs every key both sides have, one engine
// key id per Babylon key and one Babylon key per engine key id. Mouse buttons are not in it: they
// have their own engine ids (STDCONTROL_KID_MOUSE_*) and their own table in that file.
#ifndef STD_STDCONTROLDEVICESBABYLON_H
#define STD_STDCONTROLDEVICESBABYLON_H
#ifndef __cplusplus
#error "stdControlDevicesBabylon.h is C++ only: it names babylon::platform::Key and std::span."
#endif

#include <babylon/platform/Keys.h>

#include <cstddef>
#include <span>

// One row of the mapping table.
struct StdBabylonKeyMapping
{
    int engineKeyId;               // an engine key id from std/Win95/stdKeys.h
    babylon::platform::Key key;    // the Babylon key of the same physical key
};

// The mapping table: every key both sides have, ordered by engine key id, so the read writes the
// same ids in the same order every frame.
std::span<const StdBabylonKeyMapping> stdControlDevicesBabylon_KeyMappings();

// The Babylon key of an engine key id; babylon::platform::Key::Unknown when the id has no Babylon
// key. Mouse-button ids and joystick ids are not keys and answer Unknown too.
babylon::platform::Key stdControlDevicesBabylon_KeyFromEngineId(int engineKeyId);

// The engine key id of a Babylon key; -1 when the key has no engine id. The inverse of
// stdControlDevicesBabylon_KeyFromEngineId() over the same table.
int stdControlDevicesBabylon_EngineIdFromKey(babylon::platform::Key key);

#ifdef J3D_TEST
// Test-only: drops the key and button state stdControlDevicesBabylon.cpp holds between reads, which
// is a file-static a test cannot otherwise reset. Defined only when J3D_TEST is set.
void stdControlDevicesBabylon_ResetForTest();
#endif

#endif // STD_STDCONTROLDEVICESBABYLON_H
