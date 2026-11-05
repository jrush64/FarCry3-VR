#pragma once
#include <windows.h>
#include <algorithm>
#include <cmath>

namespace fc3haptic302 {
// Both the render-side notification producers and XR consumer use this small
// bounded queue. A focus/session boundary always discards the previous queue.
class Queue {
    SRWLOCK lock_ = SRWLOCK_INIT;
    int pending_ = 0;
    DWORD requested_ = 0, frame_ = 0, pulse_ = 0;
    bool live_ = false, pulsed_ = false;
public:
    bool SetLive(DWORD now, bool live) {
        AcquireSRWLockExclusive(&lock_);
        const bool continuous = live_ && now-frame_ <= 150;
        if (!live || !continuous) { pending_=0; pulsed_=false; }
        live_=live; frame_=now;
        ReleaseSRWLockExclusive(&lock_);
        return live && continuous;
    }
    bool Accepting(DWORD now) {
        AcquireSRWLockShared(&lock_);
        const bool result=live_ && now-frame_<=150;
        ReleaseSRWLockShared(&lock_);
        return result;
    }
    void Request(int count, DWORD now) {
        if (count<=0) return;
        AcquireSRWLockExclusive(&lock_);
        if (live_ && now-frame_<=150) {
            pending_=(std::min)(4,pending_+(std::min)(4,count));
            requested_=now;
        }
        ReleaseSRWLockExclusive(&lock_);
    }
    bool Take(DWORD now) {
        AcquireSRWLockExclusive(&lock_);
        if (!live_ || now-frame_>150 || now-requested_>1000) pending_=0;
        const bool result=pending_>0 && (!pulsed_ || now-pulse_>=200);
        if (result) { --pending_; pulse_=now; pulsed_=true; }
        ReleaseSRWLockExclusive(&lock_);
        return result;
    }
};
inline float Amplitude(float original, int percent) {
    if (!std::isfinite(original) || percent<=0 || percent>100) return 0;
    return (std::max)(0.f,(std::min)(1.f,original))*percent*.01f;
}
}
