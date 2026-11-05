#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cwchar>

#include "fc3_log.h"
#include "fov_boost.h"

using fc3::Log;

namespace
{
constexpr int kMaxCandidates = 256;

float g_needTanX = 0, g_needTanY = 0;

enum class State { Idle, Scanning, Testing, Restoring, Retesting, Holding, Failed, Verifying };
State g_state = State::Idle;
volatile LONG g_scanDone = 0;
float* g_candidates[kMaxCandidates] = {};
volatile LONG g_candidateCount = 0;
float g_profileFactor = 0;

int g_index = 0;                // candidate being tested
int g_wait = 0;                 // frames until its effect is judged
float g_baselineXScale = 0;
float g_baselineYScale=0,g_testOriginal=0,g_confirmX=0,g_confirmY=0;
float g_liveOriginal=0,g_liveWritten=0,g_scanOtherFactor=0;
float g_written = 0;
float* g_live = nullptr;        // the one that works
int g_refinements = 0;
int g_retryDelay = 0;
int g_attempts = 0;
LONG g_rewrites = 0;
bool g_covered = false;
int g_uncoveredFrames = 0;
// Vehicle view (the car and the jetski showed black cut-outs when looking around). Run 50192: in a
// vehicle the game re-derives the camera factor every tick, so a test write widened the view for one tick and then
// decayed back over about seven frames (world scale 0.12 -> 0.31 -> 0.45 -> 0.59 -> 0.73 -> 0.92 after each write,
// native 1.29). The classic widen/restore/widen check can never confirm such a value, and the retry loop re-wrote it
// every few seconds: a 165-degree flash decaying to 75 degrees, seen in the headset as flickering culling, with the
// view never held wide (fovBound=0 in every vehicle frame). Verifying tells a stable widening from a transient one.
// Candidate 99 tried holding such a value by rewriting it every frame (run 67572): the vehicle view stayed native in
// 146 of 165 frames, and on foot the same value over-widened to 166 degrees. So a transient candidate is restored and
// blacklisted for the session instead, and the vehicle view is widened by the engine's own camera override (below).
bool g_hiddenWeapon = false;    // presenter hint: no visible weapon this frame (vehicle-like)
float* g_blacklist[kMaxCandidates] = {};
int g_blacklistCount = 0;
bool Blacklisted(const float* candidate)
{
    for(int i=0;i<g_blacklistCount;++i)if(g_blacklist[i]==candidate)return true;
    return false;
}
// gfx_FOVOverride ("Field of View Override (Degrees)"): the engine's own console variable. Its reader (engine RVA
// 0x166200f) sits at the end of the generic camera update, after the vehicle speed FOV, the scale factor and every
// blend: a nonzero value, in degrees, replaces the main camera FOV of whatever camera is active, with no lerp (the
// near/weapon FOV has its own gfx_FOVNearOverride, left alone). Registered at RVA 0x1b4a397 (name store), default 0
// at 0x1b4a3f2, value at RVA 0x2845578. Used for hidden-weapon frames whose view the scale factor cannot cover (a
// vehicle, a cinematic), tuned from the measured view, cleared when a weapon is visible or tracking stops.
float* g_overrideTest = nullptr;    // tests: a fake variable
float* g_overrideVar = nullptr;
bool g_overrideSearched = false, g_overrideActive = false, g_overrideExternal = false;
float g_overrideDeg = 0;
int g_overrideWait = 0, g_overrideSteps = 0;
LONG g_overrideFrames = 0;
// Residue (the FOV was wrong right after loading the game): the game saves the live factor back into
// GamerProfile.xml when it exits, so the widened value this module wrote (4.61 after a weapon-camera-fed compounding)
// came back as the game's OWN setting next launch: 166 degrees before the mod did anything. The heal below keeps a record
// of the real profile value, puts it back into GamerProfile.xml for the next launch, and corrects the running game by
// writing the real value over the residue in memory (verified by the view narrowing, then widened again normally).
bool g_healEnabled = true;        // tests: off (no file access)
float g_residue = 0;              // the saved widened value found in the profile at launch (0 = none)
float g_residueRestore = 0;       // the real profile value to put back
constexpr float kDefaultProfileFactor = 1.8564f; // a measured profile value (75.1 degree view), used when no record exists
wchar_t g_profilePath[MAX_PATH] = {};

bool ReadProfileText(char* text, size_t capacity, DWORD& length)
{
    if (!g_profilePath[0])
    {
        wchar_t path[MAX_PATH] = {};
        if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, path))) return false;
        if (lstrlenW(path) + 48 >= MAX_PATH) return false;
        lstrcatW(path, L"\\My Games\\Far Cry 3\\GamerProfile.xml");
        lstrcpyW(g_profilePath, path);
    }
    HANDLE file = CreateFileW(g_profilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    std::memset(text, 0, capacity);
    length = 0;
    ReadFile(file, text, static_cast<DWORD>(capacity - 1), &length, nullptr);
    CloseHandle(file);
    return length > 0;
}

// The raw FOVScaleFactor in GamerProfile.xml, whatever its value.
bool ReadProfileRaw(float& factor)
{
    char text[16384] = {};
    DWORD length = 0;
    if (!ReadProfileText(text, sizeof(text), length)) return false;
    const char* key = std::strstr(text, "FOVScaleFactor=\"");
    if (!key) return false;
    factor = static_cast<float>(std::atof(key + 16));
    return std::isfinite(factor) && factor > 0;
}

bool ReadProfileFactor(float& factor)
{
    if (g_residue > 0) { factor = g_residueRestore; return true; } // the real value, not the saved residue
    return ReadProfileRaw(factor) && factor > 0.2f && factor < 4.0f;
}

// Rewrites only the FOVScaleFactor attribute text of GamerProfile.xml (the game's file that it saved the mod's value into).
bool WriteProfileFactor(float value)
{
    char text[16384] = {};
    DWORD length = 0;
    if (!ReadProfileText(text, sizeof(text), length) || length > sizeof(text) - 64) return false;
    char* key = std::strstr(text, "FOVScaleFactor=\"");
    if (!key) return false;
    char* start = key + 16;
    const char* end = std::strchr(start, '"');
    if (!end) return false;
    char number[32] = {};
    _snprintf_s(number, sizeof(number), _TRUNCATE, "%.4f", value);
    char out[16384] = {};
    const size_t head = static_cast<size_t>(start - text), tailLength = length - static_cast<size_t>(end - text);
    if (head + std::strlen(number) + tailLength >= sizeof(out)) return false;
    std::memcpy(out, text, head);
    std::memcpy(out + head, number, std::strlen(number));
    std::memcpy(out + head + std::strlen(number), end, tailLength);
    const DWORD total = static_cast<DWORD>(head + std::strlen(number) + tailLength);
    HANDLE file = CreateFileW(g_profilePath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const bool ok = WriteFile(file, out, total, &written, nullptr) && written == total;
    CloseHandle(file);
    return ok;
}

void RecordPath(wchar_t* path)
{
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%sfc3_fov_profile_original.txt", fc3::DataFolder());
}
bool ReadOriginalRecord(float& value)
{
    wchar_t path[MAX_PATH] = {};
    RecordPath(path);
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    char text[64] = {};
    DWORD read = 0;
    ReadFile(file, text, sizeof(text) - 1, &read, nullptr);
    CloseHandle(file);
    value = static_cast<float>(std::atof(text));
    return std::isfinite(value) && value > 0.2f && value < 4.0f;
}
void WriteOriginalRecord(float value)
{
    wchar_t path[MAX_PATH] = {};
    RecordPath(path);
    HANDLE file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    char text[64] = {};
    const int length = _snprintf_s(text, sizeof(text), _TRUNCATE, "%.6f", value);
    DWORD written = 0;
    WriteFile(file, text, static_cast<DWORD>(length), &written, nullptr);
    CloseHandle(file);
}

// Once per session, before the first scan.
void HealProfile()
{
    static bool done = false;
    if (done || !g_healEnabled) return;
    done = true;
    float raw = 0;
    if (!ReadProfileRaw(raw)) return;
    float original = 0;
    const bool haveOriginal = ReadOriginalRecord(original);
    // residue = clearly above the recorded real value, or (no record yet) above anything the in-game slider gives
    const bool residue = haveOriginal ? raw > original * 1.25f : raw > 2.5f;
    if (!residue)
    {
        if (raw > 0.2f && raw < 4.0f && (!haveOriginal || std::fabs(original - raw) > 1e-4f)) WriteOriginalRecord(raw);
        return;
    }
    g_residue = raw;
    g_residueRestore = haveOriginal ? original : kDefaultProfileFactor;
    const bool healed = WriteProfileFactor(g_residueRestore);
    if (!haveOriginal) WriteOriginalRecord(g_residueRestore);
    Log("[FOV] GamerProfile.xml FOVScaleFactor=%.4f is this mod's widened value, saved by the game when it last exited; %s %.4f for the next launch; this session is corrected in memory first",
        raw, healed ? "set back to" : "COULD NOT write it back to", g_residueRestore);
}

bool SafeRead(const float* address, float& value)
{
    __try { value = *address; return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

bool SafeWrite(float* address, float value)
{
    MEMORY_BASIC_INFORMATION info = {};
    if (!VirtualQuery(address, &info, sizeof(info)) || info.State != MEM_COMMIT ||
        !(info.Protect & (PAGE_READWRITE | PAGE_WRITECOPY)) || (info.Protect & PAGE_GUARD))
        return false;
    __try { *address = value; return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

// Scans one committed, writable region for the exact bit pattern of the factor.
void ScanRegion(const BYTE* base, SIZE_T size, DWORD bits)
{
    __try
    {
        const DWORD* cursor = reinterpret_cast<const DWORD*>(base);
        const DWORD* end = cursor + size / 4;
        for (; cursor < end; ++cursor)
        {
            DWORD other=0;std::memcpy(&other,&g_scanOtherFactor,4);
            if (*cursor != bits && (!other || *cursor!=other)) continue;
            const LONG index = g_candidateCount;
            if (index >= kMaxCandidates) return;
            g_candidates[index] = const_cast<float*>(reinterpret_cast<const float*>(cursor));
            InterlockedIncrement(&g_candidateCount);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

DWORD WINAPI ScanThread(LPVOID)
{
    HMODULE ownModule=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&ScanThread),&ownModule);
    DWORD bits = 0;
    std::memcpy(&bits, &g_profileFactor, 4);
    SYSTEM_INFO system = {};
    GetSystemInfo(&system);
    const BYTE* address = static_cast<const BYTE*>(system.lpMinimumApplicationAddress);
    const BYTE* limit = static_cast<const BYTE*>(system.lpMaximumApplicationAddress);
    while (address < limit && g_candidateCount < kMaxCandidates)
    {
        MEMORY_BASIC_INFORMATION info = {};
        if (!VirtualQuery(address, &info, sizeof(info))) break;
        const bool writable = info.AllocationBase!=ownModule&&info.State == MEM_COMMIT && (info.Protect & PAGE_READWRITE) &&
                              !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
                              (info.Type == MEM_PRIVATE || info.Type == MEM_IMAGE);
        if (writable && info.RegionSize <= 512u * 1024u * 1024u)
            ScanRegion(static_cast<const BYTE*>(info.BaseAddress), info.RegionSize, bits);
        address = static_cast<const BYTE*>(info.BaseAddress) + info.RegionSize;
    }
    InterlockedExchange(&g_scanDone, 1);
    return 0;
}

// The horizontal view (degrees) that covers the headset, given the camera's aspect.
float WantedDegrees(float xScale, float yScale)
{
    const float needXs = 1.0f / (g_needTanX * 1.06f);
    const float needYs = 1.0f / (g_needTanY * 1.06f);
    const float aspect = yScale / xScale;                       // ys = xs * aspect
    const float targetXs = needXs < needYs / aspect ? needXs : needYs / aspect;
    float wantDegrees = 2.0f * std::atan(1.0f / targetXs) * 57.29578f;
    if (wantDegrees > 150.0f) wantDegrees = 150.0f;
    return wantDegrees;
}
float ViewDegrees(float scale) { return 2.0f * std::atan(1.0f / scale) * 57.29578f; }
// Factor that would give the needed view, assuming degrees scale with it.
float WantedFactor(float currentFactor, float xScale, float yScale)
{
    return currentFactor * WantedDegrees(xScale, yScale) / ViewDegrees(xScale);
}

bool FullyCovered(float xScale, float yScale)
{
    return 1.0f / xScale >= g_needTanX * 1.02f && 1.0f / yScale >= g_needTanY * 1.02f;
}

// Aiming down sights narrows the game's view a little; that must not count as
// "no longer covered".
bool MostlyCovered(float xScale, float yScale)
{
    return 1.0f / xScale >= g_needTanX * 0.6f && 1.0f / yScale >= g_needTanY * 0.6f;
}
}

namespace fc3fov
{
bool Widened(float x,float y)
{
    return std::isfinite(x)&&std::isfinite(y)&&x<g_baselineXScale*.97f&&y<g_baselineYScale*.97f;
}
bool Narrowed(float x,float y)
{
    return std::isfinite(x)&&std::isfinite(y)&&x>g_baselineXScale*1.03f&&y>g_baselineYScale*1.03f;
}
// The effect a test write must have: a residue test narrows (real value over the saved widened one), a normal test widens.
bool Effect(float x,float y)
{
    return g_residue>0&&g_testOriginal==g_residue?Narrowed(x,y):Widened(x,y);
}
bool NearProjection(float x,float y,float expectedX,float expectedY)
{
    return std::fabs(x-expectedX)<expectedX*.02f&&std::fabs(y-expectedY)<expectedY*.02f;
}
void RestoreCandidate()
{
    if(g_index<0||g_index>=g_candidateCount)return;
    float value=0;
    if(SafeRead(g_candidates[g_index],value)&&value==g_written)
        SafeWrite(g_candidates[g_index],g_testOriginal);
}
void HoldKnownValue()
{
    if(!g_live)return;float value=0;
    if(!SafeRead(g_live,value)){g_live=nullptr;return;}
    if(value==g_liveOriginal)SafeWrite(g_live,g_liveWritten);
    else if(value!=g_liveWritten)g_live=nullptr; // never overwrite repurposed storage
}
void SetHiddenWeapon(bool hidden)
{
    g_hiddenWeapon=hidden;
}
bool OverrideActive()
{
    return g_overrideActive;
}
void ForceOverrideForTest(bool active)
{
    g_overrideActive=active;
}
// The engine variable, verified by its registration, its default store and its camera reader.
float* OverrideVariable()
{
    if(g_overrideTest)return g_overrideTest;
    if(g_overrideSearched)return g_overrideVar;
    g_overrideSearched=true;
    const UINT base=reinterpret_cast<UINT>(GetModuleHandleW(L"FC3_d3d11.dll"));if(!base)return nullptr;
    const BYTE* reg=reinterpret_cast<const BYTE*>(base+0x1b4a397);   // mov dword ptr [name slot], "gfx_FOVOverride"
    const BYTE* init=reinterpret_cast<const BYTE*>(base+0x1b4a3f2);  // movss [value], xmm0 (= 0)
    const BYTE* read=reinterpret_cast<const BYTE*>(base+0x166200f);  // movss xmm0, [value] in the camera update
    const UINT nameSlot=base+0x2845568,value=base+0x2845578;
    bool ok=!IsBadReadPtr(reg,10)&&!IsBadReadPtr(init,8)&&!IsBadReadPtr(read,8);
    ok=ok&&reg[0]==0xc7&&reg[1]==0x05&&*reinterpret_cast<const UINT*>(reg+2)==nameSlot;
    const char* name=ok?*reinterpret_cast<const char* const*>(reg+6):nullptr;
    ok=ok&&name&&!IsBadReadPtr(name,16)&&!memcmp(name,"gfx_FOVOverride",16);
    ok=ok&&init[0]==0xf3&&init[1]==0x0f&&init[2]==0x11&&init[3]==0x05&&*reinterpret_cast<const UINT*>(init+4)==value;
    ok=ok&&read[0]==0xf3&&read[1]==0x0f&&read[2]==0x10&&read[3]==0x05&&*reinterpret_cast<const UINT*>(read+4)==value;
    g_overrideVar=ok?reinterpret_cast<float*>(value):nullptr;
    Log(ok?"[FOV] gfx_FOVOverride found (engine camera override, degrees); hidden-weapon views are widened through it"
          :"[FOV] gfx_FOVOverride not found (engine code differs); hidden-weapon views keep the scale-factor path only");
    return g_overrideVar;
}
void OverrideOff(const char* why)
{
    if(!g_overrideActive)return;
    float* variable=OverrideVariable();
    if(variable){float current=0;if(SafeRead(variable,current)&&current==g_overrideDeg)SafeWrite(variable,0.f);}
    Log("[FOV] gfx_FOVOverride cleared (%s) after %ld frames at %.1f degrees",why,g_overrideFrames,g_overrideDeg);
    g_overrideActive=false;g_overrideDeg=0;g_overrideFrames=0;g_overrideWait=0;g_overrideSteps=0;
}
// Once per live frame. True while the override owns the view (then no scan or test write runs).
bool OverrideTick(float xScale,float yScale)
{
    float* variable=OverrideVariable();if(!variable)return false;
    float current=0;
    if(!SafeRead(variable,current)||!std::isfinite(current)){OverrideOff("variable unreadable");return false;}
    if(g_overrideActive&&current!=g_overrideDeg)
    { // someone else (console) uses it: leave it alone until it is 0 again
        Log("[FOV] gfx_FOVOverride changed outside the mod to %.1f; leaving it",current);
        g_overrideActive=false;g_overrideExternal=true;g_overrideDeg=0;g_overrideFrames=0;return false;
    }
    if(g_overrideExternal){if(current!=0)return false;g_overrideExternal=false;}
    const bool want=g_hiddenWeapon&&(g_overrideActive||!FullyCovered(xScale,yScale));
    if(!want){OverrideOff(g_hiddenWeapon?"view covered without it":"weapon visible");return false;}
    if(!g_overrideActive)
    {
        if(current!=0){g_overrideExternal=true;return false;}
        g_overrideDeg=(std::max)(40.f,(std::min)(170.f,WantedDegrees(xScale,yScale)));
        if(!SafeWrite(variable,g_overrideDeg)){g_overrideDeg=0;return false;}
        g_overrideActive=true;g_overrideWait=0;g_overrideSteps=0;g_overrideFrames=0;
        Log("[FOV] hidden-weapon view %.1f x %.1f degrees (want %.1f): gfx_FOVOverride = %.1f, the engine's own camera override (vehicle / cinematic camera included)",
            ViewDegrees(xScale),ViewDegrees(yScale),WantedDegrees(xScale,yScale),g_overrideDeg);
        return true;
    }
    ++g_overrideFrames;
    if(++g_overrideWait<3)return true; // the override applies on the next camera update
    g_overrideWait=0;
    const float ratio=WantedDegrees(xScale,yScale)/ViewDegrees(xScale);
    if(ratio>1.02f||ratio<.95f)
    { // whichever axis the engine's degrees mean, the measured view says how far off the view is
        float next=g_overrideDeg*ratio;
        next=(std::min)(next,g_overrideDeg*1.5f);next=(std::max)(next,g_overrideDeg*.67f);
        next=(std::max)(40.f,(std::min)(170.f,next));
        if(next!=g_overrideDeg&&SafeWrite(variable,next))
        {
            if(++g_overrideSteps<=12)Log("[FOV] override step %d: view %.1f x %.1f degrees, want %.1f: %.1f -> %.1f",g_overrideSteps,ViewDegrees(xScale),ViewDegrees(yScale),WantedDegrees(xScale,yScale),g_overrideDeg,next);
            g_overrideDeg=next;
        }
    }
    return true;
}
void SetNeed(float tanHorizontal, float tanVertical)
{
    g_needTanX = tanHorizontal;
    g_needTanY = tanVertical;
}

void Update(bool wanted, bool live, float xScale, float yScale)
{
    if(!wanted)
    {
        if(g_state==State::Testing||g_state==State::Restoring||g_state==State::Retesting||g_state==State::Verifying)RestoreCandidate();
        OverrideOff("tracking stopped");
        if(g_live)
        {float value=0;if(SafeRead(g_live,value)&&value==g_liveWritten)SafeWrite(g_live,g_liveOriginal);}
        g_live=nullptr;g_covered=false;
        // A scan worker owns its arrays until it completes. Never start another.
        if(g_state!=State::Scanning||g_scanDone)g_state=State::Idle;
        return;
    }
    HealProfile();
    HoldKnownValue();
    if(!live||!std::isfinite(xScale)||!std::isfinite(yScale)||xScale<=.01f||yScale<=.01f)
    {
        if(g_state==State::Testing||g_state==State::Restoring||g_state==State::Retesting||g_state==State::Verifying)
        {RestoreCandidate();g_state=State::Idle;g_retryDelay=30;}
        return; // an active override is kept through a menu
    }
    if(g_needTanX<=0||g_needTanY<=0)return;
    g_covered=g_covered?MostlyCovered(xScale,yScale):FullyCovered(xScale,yScale);
    if(OverrideTick(xScale,yScale))
    { // the engine override owns this view: no scan or test write meanwhile
        if(g_state==State::Testing||g_state==State::Restoring||g_state==State::Retesting||g_state==State::Verifying)
        {RestoreCandidate();g_state=g_live?State::Holding:State::Idle;g_retryDelay=5;}
        return;
    }
    switch(g_state)
    {
    case State::Idle:
        if(g_residue<=0&&FullyCovered(xScale,yScale))return; // a residue is corrected even though 166 degrees "covers"
        if(g_retryDelay>0){--g_retryDelay;return;}
        if(!ReadProfileFactor(g_profileFactor)){g_state=State::Failed;return;}
        ++g_attempts;g_candidateCount=0;g_scanDone=0;g_scanOtherFactor=g_residue>0?g_residue:g_liveWritten;
        if(HANDLE thread=CreateThread(nullptr,0,ScanThread,nullptr,0,nullptr))
        {CloseHandle(thread);g_state=State::Scanning;Log("[FOV] searching for a causally verified widening value; known binding retained during transition");}
        return;
    case State::Scanning:
        if(!g_scanDone)return;
        g_index=-1;g_wait=0;g_state=State::Testing;return;
    case State::Testing:
        if(g_wait>0)
        {
            if(--g_wait>0)return;
            if(Effect(xScale,yScale))
            {
                // Leave the value in place a little longer: a stable camera keeps the
                // new view, a re-derived (vehicle) camera decays back without any restore.
                g_confirmX=xScale;g_confirmY=yScale;
                g_state=State::Verifying;g_wait=6;return;
            }
            RestoreCandidate(); // narrower or unrelated change is NOT evidence
        }
        for(++g_index;g_index<g_candidateCount;++g_index)
        {
            float value=0;float* candidate=g_candidates[g_index];
            if(candidate==g_live||Blacklisted(candidate)||!SafeRead(candidate,value)||
               (value!=g_profileFactor&&value!=g_scanOtherFactor))continue;
            g_baselineXScale=xScale;g_baselineYScale=yScale;g_testOriginal=value;
            const bool residue=g_residue>0&&value==g_residue;
            g_written=residue?g_residueRestore:WantedFactor(value,xScale,yScale);
            const bool sane=residue?g_written<value:(g_written>value&&g_written<5.f&&g_written<=g_profileFactor*2.6f);
            if(std::isfinite(g_written)&&sane&&SafeWrite(candidate,g_written))
            {g_wait=5;return;}
        }
        g_retryDelay=90;g_state=g_live?State::Holding:State::Idle;return;
    case State::Verifying:
        if(--g_wait>0)return;
        {
            float value=0;const bool readable=SafeRead(g_candidates[g_index],value);
            const float full=std::fabs(g_confirmX-g_baselineXScale),back=std::fabs(xScale-g_baselineXScale);
            const bool residue=g_residue>0&&g_testOriginal==g_residue;
            const bool decayed=readable&&std::isfinite(value)&&(value!=g_written||(full>0&&back<full*.5f));
            if(decayed&&!residue)
            {
                // Causal but transient: the game re-derives it each tick (a vehicle camera).
                // Rewriting it every frame did not hold the vehicle view (candidate 99), so
                // it is put back and never tested again this session; no more flashes.
                RestoreCandidate();
                if(g_blacklistCount<kMaxCandidates)g_blacklist[g_blacklistCount++]=g_candidates[g_index];
                Log("[FOV] candidate %d widened the view only transiently (%.1f degrees at +5 frames, %.1f at +11, value now %.4f): restored and excluded for this session",
                    g_index,ViewDegrees(g_confirmX),ViewDegrees(xScale),value);
                g_state=State::Testing;g_wait=0;return;
            }
        }
        RestoreCandidate();g_state=State::Restoring;g_wait=4;return;
    case State::Restoring:
        if(--g_wait>0)return;
        if(NearProjection(xScale,yScale,g_baselineXScale,g_baselineYScale))
        {
            float value=0;
            if(SafeRead(g_candidates[g_index],value)&&value==g_testOriginal&&SafeWrite(g_candidates[g_index],g_written))
            {g_state=State::Retesting;g_wait=4;return;}
        }
        RestoreCandidate();g_state=State::Testing;g_wait=0;return;
    case State::Retesting:
        if(--g_wait>0)return;
        if(Effect(xScale,yScale)&&NearProjection(xScale,yScale,g_confirmX,g_confirmY))
        {
            if(g_live)
            {float value=0;if(SafeRead(g_live,value)&&value==g_liveWritten)SafeWrite(g_live,g_liveOriginal);}
            const bool residue=g_residue>0&&g_testOriginal==g_residue;
            g_live=g_candidates[g_index];g_liveOriginal=residue?g_residueRestore:g_testOriginal;g_liveWritten=g_written;
            if(residue)
            { // corrected: now widen from the real value at once (the confirm view is the real, narrow one)
                g_residue=0;const float next=WantedFactor(g_residueRestore,g_confirmX,g_confirmY);
                if(std::isfinite(next)&&next>g_liveWritten&&next<=g_profileFactor*2.6f&&next<5.f&&SafeWrite(g_live,next))g_liveWritten=next;
                Log("[FOV] residue %.4f replaced in memory by the real profile value %.4f, then widened to %.4f",g_testOriginal,g_residueRestore,g_liveWritten);
            }
            g_state=State::Holding;g_refinements=0;g_wait=0;g_uncoveredFrames=0;
            Log("[FOV] verified widen/restore/widen; view %.1f x %.1f degrees",2*std::atan(1/xScale)*57.29578f,2*std::atan(1/yScale)*57.29578f);
            return;
        }
        RestoreCandidate();g_state=State::Testing;g_wait=0;return;
    case State::Holding:
        if(!g_live){g_state=State::Idle;g_retryDelay=30;return;}
        if(!MostlyCovered(xScale,yScale))
        {
            if(++g_uncoveredFrames>90)
            {
                // Door/cinematic FOV does not prove the verified address died.
                // Keep it held while searching, so it can recover immediately.
                g_uncoveredFrames=0;g_retryDelay=30;g_state=State::Idle;
                Log("[FOV] narrow camera transition; retaining verified factor while checking replacement camera");
            }
            return;
        }
        g_uncoveredFrames=0;
        if(!FullyCovered(xScale,yScale)&&g_refinements<4&&++g_wait>30)
        {
            g_wait=0;const float next=WantedFactor(g_liveWritten,xScale,yScale);
            if(next>g_liveWritten*1.01f&&next<g_profileFactor*2.6f&&next<5.f)
            {++g_refinements;g_liveWritten=next;SafeWrite(g_live,next);}
        }
        return;
    case State::Failed:return;
    }
}
bool Covered()
{
    return g_covered;
}
void ProbeState(int& state,int& candidates,int& attempts,bool& bound)
{
    state=g_overrideActive?8:static_cast<int>(g_state);candidates=g_candidateCount;
    attempts=g_attempts;bound=g_live!=nullptr||g_overrideActive;
}
}

