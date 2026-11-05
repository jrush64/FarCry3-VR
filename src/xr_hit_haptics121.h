#pragma once
#include <windows.h>
#include <openxr/openxr.h>
namespace fc3hit121 {
struct Queue {
    volatile LONG stamp=0;
    void Request(DWORD now){InterlockedExchange(&stamp,static_cast<LONG>(now?now:1));}
};
// Called only by the XR thread. Coalesce requests, discard stale/unfocused
// hits, and never route this feedback through the two-hand diagnostic buzz.
inline bool Consume(Queue& queue,DWORD now,bool ready,bool focused,XrSession session,XrAction right,PFN_xrApplyHapticFeedback apply)
{
    const DWORD stamp=static_cast<DWORD>(InterlockedExchange(&queue.stamp,0));
    if(!stamp||!ready||!focused||!session||!right||!apply||now-stamp>150)return false;
    XrHapticActionInfo info={XR_TYPE_HAPTIC_ACTION_INFO};info.action=right;
    XrHapticVibration pulse={XR_TYPE_HAPTIC_VIBRATION};pulse.duration=45000000;
    pulse.frequency=XR_FREQUENCY_UNSPECIFIED;pulse.amplitude=.55f;
    return XR_SUCCEEDED(apply(session,&info,reinterpret_cast<const XrHapticBaseHeader*>(&pulse)));
}
}
