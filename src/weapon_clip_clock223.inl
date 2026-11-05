#pragma once
ULONGLONG W7Now(); // shared native/Insert menu pause time, defined below weapon mechanics
double WeaponClipElapsed223(ULONGLONG start,ULONGLONG trackingPause)
{
 const ULONGLONG now=W7Now(),end=trackingPause?(std::min)(trackingPause,now):now;
 return end>=start?double(end-start)/1000.0:0;
}
// Return the tracking-only interval on recovery. Menu resume shifts both origins,
// so an overlapping menu interval is counted once rather than twice.
ULONGLONG AdvanceWeaponClip223(ULONGLONG& start,ULONGLONG& pause,bool playing,bool active,ULONGLONG now)
{
 if(!playing){pause=0;return 0;}
 if(!active){if(!pause)pause=now;return 0;}
 if(!pause)return 0;
 const ULONGLONG elapsed=now>=pause?now-pause:0;
 start+=elapsed;pause=0;return elapsed;
}
