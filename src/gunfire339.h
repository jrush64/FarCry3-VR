#pragma once
#include "weapon_hands303.h"

namespace fc3gunfire339 {
struct Pulse {int hand=-1;float amplitude=0;LONGLONG duration=0;};
inline Pulse Shape(int profile) {
    Pulse p;
    if(fc3hands303::Shotgun318(profile)){p.amplitude=.7f;p.duration=35000000;}
    else if(fc3hands303::Smg332(profile)){p.amplitude=.3f;p.duration=18000000;}
    else if(fc3hands303::Sidearm315(profile)){p.amplitude=.5f;p.duration=25000000;}
    else if(fc3hands303::Bolt319(profile)||profile==35){p.amplitude=.65f;p.duration=32000000;}
    else if(fc3hands303::MagazineRifle317(profile)||profile==43){p.amplitude=.45f;p.duration=22000000;}
    return p; // Continuous tools, explosives and unknown models stay silent.
}
inline bool Owner(const fc3hands303::Snapshot& s,unsigned entity,ULONGLONG now) {
    return s.enabled&&s.active&&!s.empty&&(s.engaged||s.ownerContext326)&&
        entity&&s.entity==entity&&s.hand>=0&&s.hand<2&&now>=s.at&&now-s.at<=100;
}
// One pending native-shot receipt, never a backlog of recoil pulses. The native
// bullet-vector boundary can run once per pellet: coalesce a volley within32ms.
// Capture the physical owner at the event; discard it if that owner changes.
class Queue {
    SRWLOCK lock_=SRWLOCK_INIT;
    bool live_=false,pending_=false,last_=false;
    ULONGLONG frame_=0,shot_=0,lastAt_=0;
    unsigned entity_=0,generation_=0,lastEntity_=0,lastGeneration_=0;
    int hand_=-1,profile_=-1;
public:
    void SetLive(ULONGLONG now,bool live) {
        AcquireSRWLockExclusive(&lock_);
        if(!live||!live_||now<frame_||now-frame_>100){pending_=last_=false;}
        live_=live;frame_=now;ReleaseSRWLockExclusive(&lock_);
    }
    bool Request(unsigned entity,int profile,const fc3hands303::Snapshot& s,ULONGLONG now) {
        if(!Owner(s,entity,now)||Shape(profile).duration==0)return false;
        AcquireSRWLockExclusive(&lock_);
        bool accept=live_&&now>=frame_&&now-frame_<=100;
        if(last_&&entity==lastEntity_&&s.generation==lastGeneration_&&now>=lastAt_&&now-lastAt_<32)accept=false;
        if(accept){entity_=entity;generation_=s.generation;hand_=s.hand;profile_=profile;shot_=now;pending_=true;
            last_=true;lastEntity_=entity;lastGeneration_=s.generation;lastAt_=now;}
        ReleaseSRWLockExclusive(&lock_);return accept;
    }
    Pulse Take(const fc3hands303::Snapshot& s,ULONGLONG now) {
        Pulse p;AcquireSRWLockExclusive(&lock_);
        if(pending_&&live_&&now>=frame_&&now-frame_<=100&&now>=shot_&&now-shot_<=80&&
            Owner(s,entity_,now)&&s.generation==generation_&&s.hand==hand_){p=Shape(profile_);p.hand=hand_;}
        pending_=false;ReleaseSRWLockExclusive(&lock_);return p;
    }
};
}
