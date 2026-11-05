#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include "fc3_log.h"
#include "throwable_capture_host.h"
#include "aim_probe.h"
namespace fc3throwhost {
namespace {
SRWLOCK lock = SRWLOCK_INIT;
HANDLE capture = INVALID_HANDLE_VALUE;
bool attempted = false, active = false, stopped = false;
ULONGLONG nextReport = 0;
unsigned reports = 0;
void Report(const char* state) {
    const auto s = fc3throw::Status();
    const auto manual=fc3throw::ManualStatus143();
    unsigned sndBlocked=0,sndPlayed=0;fc3throw::PinSoundStatus156(sndBlocked,sndPlayed);fc3::Log("[PIN156 SOUND] draw events deferred=%u physical pin sounds played=%u",sndBlocked,sndPlayed);
    unsigned pinBlocked=0,pinArmed=0,pinStowed=0;fc3throw::SafetyStatus155(pinBlocked,pinArmed,pinStowed);
    fc3::Log("[PIN155] native activation blocked=%u armed-after-extraction=%u safe-native-stows=%u",pinBlocked,pinArmed,pinStowed);
    fc3::Log("[THROW_CAPTURE] %s candidates; flag2048=matched_fresh_native_local_identity; queued=%u lost=%ld phases=%ld,%ld,%ld,%ld,%ld,%ld error=%lu full=%d",
        state, s.queued, s.lost, s.phaseCounts[0], s.phaseCounts[1], s.phaseCounts[2],
        s.phaseCounts[3], s.phaseCounts[4], s.phaseCounts[5], s.writeError, s.full);
    fc3::Log("[THROW145] native trajectory tickets prepared=%u queued=%u applied=%u completed=%u rejected=%u pending=%d; v2 owner is owning pawn through wrapper+28, not weapon through wrapper+4",
        manual.prepared,manual.queued,manual.applied,manual.completed,manual.rejected,manual.pending);
    fc3::Log("[DROP148] native update visits=%u early calls=%u readiness blocked=%u last native flags=%08X gate=%u; rejected expiry=%u context=%u vector=%u",
        manual.updates148,manual.earlyCalls148,manual.readyBlocks148,manual.lastFlags148,manual.lastGate148,manual.expired148,manual.contextRejects148,manual.vectorRejects148);
    fc3::Log("[THROW147] unlimited grenade/Molotov stock %s; preserved native debits=%u",fc3throw::Unlimited147()?"ON":"OFF",fc3throw::PreservedStock147());
}
void Close() {
    if (capture != INVALID_HANDLE_VALUE) { CloseHandle(capture); capture = INVALID_HANDLE_VALUE; }
}
}
void Tick(bool minHookReady) {
    AcquireSRWLockExclusive(&lock);
    if (stopped) { ReleaseSRWLockExclusive(&lock); return; }
    if (!attempted && minHookReady && GetModuleHandleW(L"FC3_d3d11.dll")) {
        attempted = true;
        fc3throw::SetIdentityReader143(fc3aim::LocalPlayerIdentity143);
        wchar_t path[MAX_PATH]; SYSTEMTIME time; GetSystemTime(&time);
        const int n = _snwprintf_s(path, MAX_PATH, _TRUNCATE,
            L"%sthrowable_launch_%04u%02u%02u_%02u%02u%02u_%03u_pid%lu.tlp1",
            fc3::DataFolder(), time.wYear, time.wMonth, time.wDay, time.wHour,
            time.wMinute, time.wSecond, time.wMilliseconds, GetCurrentProcessId());
        if (n >= 0) capture = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        const DWORD openError=capture==INVALID_HANDLE_VALUE?GetLastError():0;
        if (!fc3throw::Install()) {
            fc3::Log("[THROW_CAPTURE] throw/update install rejected; identity/signature/hook check failed; no capture retry");
            Close();
        } else if(capture==INVALID_HANDLE_VALUE){
            fc3throw::StopCapture143();
            fc3::Log("[THROW_CAPTURE] open failed error=%lu; recording disabled, launch bridge retained",openError);
        } else {
            active = true;
            fc3::Log("[THROW_CAPTURE] active path=%ls; physical release146 on native gadget update; trajectory writes require explicit local projectile release ticket", path);
            nextReport = GetTickCount64() + 30000;
        }
    }
    if (active) {
        const bool ok = fc3throw::FlushTo(capture);
        const auto s = fc3throw::Status();
    const auto manual=fc3throw::ManualStatus143();
        if (!ok || s.full) {
            fc3throw::StopCapture143(); active = false;
            Report(ok ? "size cap reached, stopped" : "write failed, stopped"); Close();
        } else if (reports < 32 && GetTickCount64() >= nextReport) {
            Report("active"); ++reports; nextReport = GetTickCount64() + 30000;
        }
    }
    ReleaseSRWLockExclusive(&lock);
}
void PublishNativePose(const fc3throw::PoseSample& supplied, bool ownerIdentityProven) {
    auto pose = supplied;
    if (!ownerIdentityProven) pose.localOwnerId = 0;
    fc3throw::PublishPose(pose);
}
void Stop() {
    AcquireSRWLockExclusive(&lock);
    if (!stopped) {
        stopped = true;
        fc3throw::Disable();
        if (active) { fc3throw::FlushTo(capture); Report("shutdown"); }
        active = false; Close();
    }
    ReleaseSRWLockExclusive(&lock);
}
}
