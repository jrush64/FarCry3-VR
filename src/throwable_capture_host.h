#pragma once
#include "throwable_launch_probe.h"
namespace fc3throwhost {
// Call from the normal serialized host frame tick AFTER successful MinHook init.
// Module arrival is polled; opening/install is attempted once per process.
void Tick(bool minHookReady);
// Default capture has no pose and no actor filter. Only supply a proven native
// world-space pose; owner filtering additionally requires proven actor identity.
void PublishNativePose(const fc3throw::PoseSample&, bool ownerIdentityProven);
// Explicit shutdown on a normal host thread, never DllMain. Retains trampolines;
// DLL unload/MH_Uninitialize still require all game hook callers to quiesce.
void Stop();
}
