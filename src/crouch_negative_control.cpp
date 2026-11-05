#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include <ViGEm/Client.h>

#include "fc3_log.h"
#include "vr_pad.h"
#include "aim_probe.h"
#include <intrin.h>

using fc3::Log;

namespace
{
// XInput 1.3 layout, declared here so no SDK header or import library is needed.
struct PadState
{
    WORD buttons;
    BYTE leftTrigger, rightTrigger;
    SHORT thumbLX, thumbLY, thumbRX, thumbRY;
};
struct PadReading
{
    DWORD packet;
    PadState pad;
};

constexpr WORD kStart = 0x0010, kBack = 0x0020, kLeftThumb = 0x0040,
               kRightThumb = 0x0080, kLeftShoulder = 0x0100, kRightShoulder = 0x0200,
               kA = 0x1000, kB = 0x2000, kX = 0x4000, kY = 0x8000;

using GetStateFn = DWORD(WINAPI*)(DWORD, PadReading*);
GetStateFn g_oGetState = nullptr;

SRWLOCK g_lock = SRWLOCK_INIT;
fc3pad::Controllers g_state = {};
PadState g_last = {};
DWORD g_changes = 0;
LONG g_calls = 0, g_injected = 0;
WORD g_seenButtons = 0;

SHORT Axis(float v)
{
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    return static_cast<SHORT>(v * 32767.0f);
}

BYTE Trigger(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    return static_cast<BYTE>(v * 255.0f + 0.5f);
}

volatile bool g_leftGripSuppressed = false;
volatile bool g_fireBlocked = false;
volatile bool g_bowHandling=false;volatile float g_bowTrigger=0;
volatile bool g_fullFirePull = false;
volatile bool g_shotgunHandling = false;
volatile bool g_oneShellX=false,g_oneXSpent=false,g_oneXBusy=false;
volatile bool g_manualReloadInput=false,g_reloadAlignmentInput=false;
volatile bool g_magazineHandling=false,g_magazineBusy=false,g_pistolCalibration=false;
volatile bool g_leverHandling=false,g_leverXOwned=false,g_xOwned=false; // 1887: right grip = lever/shell, A with right grip = shell pickup, left grip = hold
volatile bool g_menuExitWait=false;
volatile bool g_wheelLatched=false;
volatile bool g_triggerWheel=false,g_triggerWheelWait=false;
volatile bool g_swapBX=false;
fc3pad::AmmoButtonAction g_ammoPress=fc3pad::AmmoButtonAction::None;
volatile bool g_stanceCrouched=false; // retained for old diagnostic fixtures; never determines gameplay stance
volatile LONG g_nativeStance=-1,g_nativeStanceAt=0;
volatile LONG g_gripOwner=0; // 0 released, 1 weapon for entire squeeze, 2 wheel
volatile LONG g_gripContext=0; // 0 unavailable, 1 near weapon/reload, 2 free
volatile LONG g_gripContextAt=0;
volatile bool g_gameplayControls=false,g_dedicatedInteractions=true;
volatile bool g_wheelYSpent=false,g_interactReleaseWait=false,g_crouchDown=false,g_crouchReleaseWait=false;
volatile LONG g_reloadPulseUntil=0;
volatile bool g_nativePaused=false;
volatile LONG g_nativeMousePointer=0;
volatile LONG g_nativeMenuPendingUntil=0;
bool NativeMenuBlocked(){const LONG until=g_nativeMenuPendingUntil;return g_nativePaused||(until&&static_cast<LONG>(GetTickCount()-until)<0);}
void ObserveNativePause(bool paused)
{
 if(paused){InterlockedExchange(&g_nativeMenuPendingUntil,0);}
 if(g_nativePaused==paused)return;
 g_nativePaused=paused;g_wheelLatched=false;g_gripOwner=0;
 InterlockedExchange(&g_reloadPulseUntil,0);
 if(!paused)g_menuExitWait=true;
 Log("[MENU25] native pause %s; physical input %s",paused?"on":"off",paused?"blocked":"awaits neutral release");
}
// Read the exact field used by the engine's registered IsGamePaused Lua getter.
// No native calls, hooks, writes or retained heap pointer. Reject other builds.
bool ReadNativePause(bool& paused)
{
 __try {
  const auto module=GetModuleHandleW(L"FC3_d3d11.dll");if(!module)return false;
  const auto base=reinterpret_cast<BYTE*>(module);
  const auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
  if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>4096)return false;
  const auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
  if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800)return false;
  static const BYTE prefix[]={0x55,0x8b,0xec,0x8b,0x45,0x08,0x8b,0x48,0x08,0x33,0xd2,0x56,0x8b,0x35};
  static const BYTE suffix[]={0x38,0x56,0x58,0xc7,0x41,0x04,0x01,0,0,0,0x0f,0x95,0xc2};
  const auto getter=base+0x6684d0;
  if(memcmp(getter,prefix,sizeof(prefix))||memcmp(getter+18,suffix,sizeof(suffix))||
     *reinterpret_cast<const UINT*>(getter+14)!=reinterpret_cast<UINT>(base+0x2829bc0)||
     strcmp(reinterpret_cast<const char*>(base+0x208c4f8),"IsGamePaused"))return false;
  const auto owner=*reinterpret_cast<const BYTE* const*>(base+0x2829bc0);
  if(!owner)return false;
  paused=owner[0x58]!=0;return true;
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// GetPawnStance is the registered native getter, not a remembered toggle.
// Read only at the game input seam, using a fresh local-player handle each time.
void ObserveNativeStance(int stance)
{
 InterlockedExchange(&g_nativeStance,stance==0||stance==1?stance:-1);
 InterlockedExchange(&g_nativeStanceAt,static_cast<LONG>(GetTickCount()));
}
bool ActualCrouch(bool& crouched)
{
 const LONG stance=InterlockedCompareExchange(&g_nativeStance,0,0);
 if(stance<0||static_cast<DWORD>(GetTickCount()-g_nativeStanceAt)>250)return false;
 crouched=stance==1;return true;
}
void ReadNativeStanceAtInput(const void* caller)
{
 __try {
  BYTE* base=reinterpret_cast<BYTE*>(GetModuleHandleW(L"FC3_d3d11.dll"));if(!base)return;
  const UINT site=reinterpret_cast<UINT>(caller),image=reinterpret_cast<UINT>(base);
  if(site<image||site-image>=48332800)return;
  auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
  if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>4096)return;
  auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
  if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||
     nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800)return;
  static const BYTE local[]={0x55,0x8b,0xec,0x51,0xa1};
  static const BYTE getter[]={0x55,0x8b,0xec,0x83,0xec,0x08,0x8b,0x08,0x8b,0x40,0x04};
  static const BYTE field[]={0x8b,0x50,0x24,0x8b,0xba,0x0c,0x06,0,0};
  if(memcmp(base+0xc80190,local,sizeof(local))||memcmp(base+0x1986310,getter,sizeof(getter))||
     memcmp(base+0x19863a2,field,sizeof(field))||
     *reinterpret_cast<UINT*>(base+0xc80195)!=image+0x2829474||
     strcmp(reinterpret_cast<const char*>(base+0x208d970),"GetPawnStance"))return;
  // Serialized native stance enum explicitly registers Stand=0 and Crouch=1.
  if(*reinterpret_cast<UINT*>(base+0x1b066a9)!=image+0x2036168||
     base[0x1b066b2]!=0x33||base[0x1b066b3]!=0xf6||
     *reinterpret_cast<UINT*>(base+0x1b066e7)!=image+0x1fbe150||
     memcmp(base+0x1b066f0,"\xbe\x01\x00\x00\x00",5))return;
  const UINT manager=*reinterpret_cast<UINT*>(base+0x2829474);
  if(!manager||!*reinterpret_cast<UINT*>(manager+0xc)||*reinterpret_cast<UINT*>(manager+0xc)>4){ObserveNativeStance(-1);return;}
  UINT handle[2]={~0u,~0u};
  using LocalHandle=UINT* (__cdecl*)(UINT*);
  auto localHandle=reinterpret_cast<LocalHandle>(base+0xc80190);
  if(localHandle(handle)!=handle||handle[0]==~0u||handle[1]==~0u){ObserveNativeStance(-1);return;}
  void* getterTarget=base+0x1986310;int stance=-1;const UINT* handlePointer=handle;
  __asm {
   mov eax,handlePointer
   call getterTarget
   mov stance,eax
  }
  ObserveNativeStance(stance);
 } __except(EXCEPTION_EXECUTE_HANDLER){ObserveNativeStance(-1);}
}

void RefreshNativePause(){
 bool paused=false;
 if(ReadNativePause(paused)){
  static bool reported=false;if(!reported){reported=true;Log("[MENU25] verified native IsGamePaused getter; read-only UI isolation active");}
  ObserveNativePause(paused);
 }
 const LONG until=g_nativeMenuPendingUntil;
 if(until&&!g_nativePaused&&static_cast<LONG>(GetTickCount()-until)>=0){
  InterlockedExchange(&g_nativeMenuPendingUntil,0);g_menuExitWait=true;
  Log("[MENU25] opening request expired; await neutral before gameplay resumes");
 }
}
bool DedicatedLayout(){return g_gameplayControls&&g_dedicatedInteractions&&!NativeMenuBlocked();}
bool WheelChord(const fc3pad::Controllers& c){return c.active&&!c.menu&&(g_triggerWheel||g_wheelLatched||(c.leftGrip>.5f&&c.y&&(!DedicatedLayout()||!g_wheelYSpent)));}


volatile bool g_macheteContext=false,g_macheteOwned=false,g_macheteReleaseWait=false;
volatile bool g_macheteHoldMode=false,g_macheteClickReleaseWait=false;
volatile LONG g_macheteSourceUntil=0,g_macheteAttackUntil=0;

// The controllers as one pad state.
// Right hand: A, B, trigger = fire (RT), grip = RB, stick = look, click = R3.
// Left hand:  X, Y, trigger = aim (LT), grip = LB, stick = move, click = L3.
// Menu = Start; menu while holding the left grip = Back (map / journal).
PadState FromControllers(const fc3pad::Controllers& c)
{
    PadState p = {};
    if(!c.active||g_pistolCalibration||g_menuExitWait)return p;
    if(NativeMenuBlocked()){
        // Native UI owns these controls regardless of world draws or weapon leases.
        if(c.a)p.buttons|=kA;if(c.b)p.buttons|=kB;if(c.x)p.buttons|=kX;if(c.y)p.buttons|=kY;
        if(c.menu)p.buttons|=c.leftGrip>.5f?kBack:kStart;
        p.leftTrigger=Trigger(c.leftTrigger);p.rightTrigger=Trigger(c.rightTrigger);
        p.thumbLX=Axis(c.leftX);p.thumbLY=Axis(c.leftY);p.thumbRX=Axis(c.rightX);p.thumbRY=Axis(c.rightY);
        if(InterlockedCompareExchange(&g_nativeMousePointer,0,0)){p.buttons&=~kA;p.thumbRX=p.thumbRY=0;}
        return p;
    }
    const auto ammo=fc3pad::AmmoAction(c);
    const bool ammoChord=c.x&&ammo!=fc3pad::AmmoButtonAction::Interaction;
    const bool manual=g_manualReloadInput&&g_shotgunHandling;
    const bool backCombo = c.menu && c.leftGrip > 0.5f;
    // An explicit chord opens the wheel even when physical support/pump grip
    // owns LB. Y alone keeps the native previous-weapon/healing behavior.
    const bool wheelChord=WheelChord(c);
    const bool dedicated=DedicatedLayout();
    const bool alignment=manual&&g_reloadAlignmentInput&&!wheelChord;
    const bool macheteChord=g_macheteContext&&c.active&&c.rightGrip>.7f&&c.rightStickClick&&!c.leftStickClick;
    if (c.a&&!alignment&&!(g_leverHandling&&c.rightGrip>.5f)) p.buttons |= kA;
    if (!dedicated&&c.b&&!alignment&&!(manual&&c.leftGrip>.7f)) p.buttons |= kB;
    if(dedicated&&g_crouchDown&&!wheelChord&&!alignment)p.buttons|=kB;
    if((c.x&&ammo==fc3pad::AmmoButtonAction::Interaction&&!g_interactReleaseWait&&!alignment)||static_cast<LONG>(GetTickCount()-InterlockedCompareExchange(&g_reloadPulseUntil,0,0))<0)p.buttons|=kX;
    if (c.y&&!wheelChord&&(!dedicated||!g_wheelYSpent)) p.buttons |= kY;
    if (c.menu) p.buttons |= backCombo ? kBack : kStart;
    // Both stick clicks together = arm calibration, not sprint + melee.
    const bool calibrating = c.leftStickClick && c.rightStickClick;
    if (c.leftStickClick && !calibrating && !(g_leverHandling&&c.leftGrip>.5f)) p.buttons |= kLeftThumb; // 1887 hand switch
    if (c.rightStickClick && !calibrating&&!macheteChord&&!g_macheteOwned&&!g_macheteClickReleaseWait) p.buttons |= kRightThumb;
    if(!calibrating&&c.active&&(static_cast<LONG>(GetTickCount()-InterlockedCompareExchange(&g_macheteSourceUntil,0,0))<0||
        (g_macheteOwned&&g_macheteContext&&(!c.rightStickClick||(g_macheteHoldMode&&c.rightGrip>.7f))&&!c.leftStickClick&&static_cast<LONG>(GetTickCount()-InterlockedCompareExchange(&g_macheteAttackUntil,0,0))<0))) p.buttons|=kRightThumb;
    // Support grip never emits inventory LB, including transient non-gameplay frames.
    if (c.rightGrip > 0.5f && !g_gameplayControls && !g_xOwned && !g_bowHandling && !g_shotgunHandling&&!g_magazineHandling&&!g_leverHandling&&!g_leverXOwned) p.buttons |= kRightShoulder;
    if(wheelChord)p.buttons|=kLeftShoulder;
    p.leftTrigger = (g_gameplayControls||g_shotgunHandling||g_bowHandling||g_magazineHandling||g_leverHandling) ? 0 : Trigger(c.leftTrigger); // no native ADS camera in motion gameplay; raw LT retained for mechanics
    p.rightTrigger = g_bowHandling ? Trigger(g_bowTrigger) : alignment||g_fireBlocked||g_magazineBusy||(g_fullFirePull&&c.rightTrigger<=.7f) ? 0 : Trigger(c.rightTrigger);
    p.thumbLX = alignment?0:Axis(c.leftX);
    p.thumbLY = alignment?0:Axis(c.leftY);
    p.thumbRX = alignment?0:Axis(c.rightX);
    p.thumbRY = alignment||(dedicated&&!wheelChord)?0:Axis(c.rightY);
    if(g_macheteOwned||macheteChord)
    {
        p.buttons&=~(kX|kY|kLeftShoulder|kRightShoulder);
        p.leftTrigger=p.rightTrigger=0;
    }
    if(ammoChord)p.buttons&=~kRightShoulder; // right grip modifier must not throw a grenade
    if(c.x&&ammo==fc3pad::AmmoButtonAction::Interaction&&!g_interactReleaseWait&&!alignment)p.buttons|=kX;
    // Ordinary X interaction and explicit weapon selection outrank weapon-local
    // X/grip ownership, including a held visual machete. Raw X stays unchanged.
    if(dedicated){
        // B is no longer a duplicate interaction; X alone owns native X.
        if(wheelChord){p.buttons|=kLeftShoulder;p.buttons&=~(kX|kY|kB|kRightShoulder);p.leftTrigger=p.rightTrigger=0;}
        else if(c.y&&!g_wheelYSpent)p.buttons|=kY;
    }
    if(g_macheteReleaseWait)p.rightTrigger=0;
    return p;
}

SHORT Larger(SHORT a, SHORT b)
{
    return std::abs(static_cast<int>(a)) >= std::abs(static_cast<int>(b)) ? a : b;
}

void LogNewButtons(WORD buttons)
{
    static const struct { WORD bit; const char* name; } names[] = {
        { kA, "A" }, { kB, "B" }, { kX, "X" }, { kY, "Y" }, { kStart, "Start" }, { kBack, "Back" },
        { kLeftThumb, "L3" }, { kRightThumb, "R3" }, { kLeftShoulder, "LB" }, { kRightShoulder, "RB" } };
    const WORD fresh = buttons & ~g_seenButtons;
    if (!fresh) return;
    g_seenButtons |= fresh;
    for (const auto& n : names)
        if (fresh & n.bit) Log("[PAD] first press from the controllers: %s", n.name);
}

// What the game reads. Normally only watched: the controllers reach the game as a
// real (virtual) Xbox 360 pad on the ViGEm bus, which every input path of the
// game sees. (Faking the answer here was ignored by the game when NO pad was
// connected at all.)
// ViGEm Bus Driver is optional (another VR mod's installer can remove it, and it should not be required all the time):
// when the ViGEm pad is not plugged in, slot 0 is answered here with the Touch controllers - as a connected pad, the game's
// own reads (xinput1_3, imported by ordinal) get them directly. No driver needed.
extern volatile LONG g_padReady;
}
namespace fc3pad { void ApplyLookOverride(PadState& p); }
namespace
{
DWORD WINAPI HookGetState(DWORD user, PadReading* reading)
{
    RefreshNativePause();
    if(!NativeMenuBlocked()&&!g_pistolCalibration&&!g_menuExitWait)ReadNativeStanceAtInput(_ReturnAddress());
    DWORD result = g_oGetState(user, reading);
    if (!g_padReady && user == 0 && reading)
    {
        AcquireSRWLockShared(&g_lock);
        const fc3pad::Controllers c = g_state;
        ReleaseSRWLockShared(&g_lock);
        static bool everActive = false;
        if (c.active) everActive = true;
        if (everActive)
        {
            PadState p = c.active ? FromControllers(c) : PadState{};
            if(c.active)fc3pad::ApplyLookOverride(p);
            static DWORD packet = 1;
            static PadState last = {};
            if (std::memcmp(&p, &last, sizeof(p)) != 0) { ++packet; last = p; }
            reading->packet = packet;
            reading->pad = p;
            if (result != ERROR_SUCCESS)
            {
                static LONG logged = 0;
                if (InterlockedExchange(&logged, 1) == 0) Log("[PAD] no pad in slot 0: the Touch controllers answer slot 0 directly");
            }
            result = ERROR_SUCCESS;
            InterlockedIncrement(&g_injected);
            LogNewButtons(p.buttons);
        }
    }
    if(result==ERROR_SUCCESS&&reading&&user<4){
        static LONG reloadX[4]={};
        const LONG down=(reading->pad.buttons&kX)!=0;
        const LONG prior=InterlockedExchange(&reloadX[user],down);
        if(prior!=down&&g_magazineHandling)Log("[MAG INPUT] native XInput slot %lu X %s, RT %u LT %u busy %d request %d",user,down?"down":"up",reading->pad.rightTrigger,reading->pad.leftTrigger,g_magazineBusy,static_cast<LONG>(GetTickCount()-InterlockedCompareExchange(&g_reloadPulseUntil,0,0))<0);
    }
    if(g_pistolCalibration||g_menuExitWait){if(reading)reading->pad={};fc3aim::Suspend();}
    else if(NativeMenuBlocked())fc3aim::Suspend();
    else fc3aim::InputTick(_ReturnAddress());
    static LONG seen[4] = {};
    if (user < 4 && result == ERROR_SUCCESS && InterlockedExchange(&seen[user], 1) == 0)
        Log("[PAD] the game sees a pad in slot %lu", user);
    InterlockedIncrement(&g_calls);
    return result;
}

// ---- the virtual Xbox 360 pad ------------------------------------------------
PVIGEM_CLIENT g_client = nullptr;
PVIGEM_TARGET g_pad = nullptr;
volatile LONG g_padReady = 0;
// Plug-in result for the main-thread log (the plug-in thread's own lines never reached the log): 0 pending, 1 ok, 2.. failure stage.
volatile LONG g_padStage = 0, g_padError = 0;

DWORD WINAPI PlugInPad(LPVOID)
{
    g_client = vigem_alloc();
    if (!g_client)
    {
        Log("[PAD] virtual pad: out of memory");InterlockedExchange(&g_padStage,2);
        return 0;
    }
    const VIGEM_ERROR connected = vigem_connect(g_client);
    if (!VIGEM_SUCCESS(connected))
    {
        Log("[PAD] virtual pad: the ViGEm bus driver did not answer (0x%08X); Touch buttons cannot reach the game",
            static_cast<unsigned>(connected));
        InterlockedExchange(&g_padError,static_cast<LONG>(connected));InterlockedExchange(&g_padStage,3);
        return 0;
    }
    g_pad = vigem_target_x360_alloc();
    const VIGEM_ERROR added = g_pad ? vigem_target_add(g_client, g_pad) : VIGEM_ERROR_NO_FREE_SLOT;
    if (!VIGEM_SUCCESS(added))
    {
        Log("[PAD] virtual pad: could not plug in an Xbox 360 pad (0x%08X)", static_cast<unsigned>(added));
        InterlockedExchange(&g_padError,static_cast<LONG>(added));InterlockedExchange(&g_padStage,4);
        return 0;
    }
    InterlockedExchange(&g_padReady, 1);InterlockedExchange(&g_padStage,1);
    Log("[PAD] virtual Xbox 360 pad plugged in (user index %lu); the Touch controllers drive it",
        vigem_target_get_index(g_pad));
    return 0;
}
}

namespace fc3pad
{
void Install()
{
    HMODULE module = GetModuleHandleW(L"xinput1_3.dll");
    if (!module) module = LoadLibraryW(L"xinput1_3.dll");
    void* target = module ? reinterpret_cast<void*>(GetProcAddress(module, "XInputGetState")) : nullptr;
    if (!target)
    {
        Log("[PAD] xinput1_3.dll / XInputGetState not found; controllers cannot reach the game");
        return;
    }
    const MH_STATUS create = MH_CreateHook(target, reinterpret_cast<void*>(&HookGetState),
                                           reinterpret_cast<void**>(&g_oGetState));
    const MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : create;
    Log("[PAD] gamepad hook %s (create=%d enable=%d)", create == MH_OK && enable == MH_OK ? "installed" : "FAILED",
        static_cast<int>(create), static_cast<int>(enable));
}

bool g_firePressed = false;

bool FirePressed()
{
    return g_firePressed;
}

bool g_fireHeld = false;
volatile LONG g_calibrate = 0;

bool TakeCalibration()
{
    return InterlockedExchange(&g_calibrate, 0) != 0;
}

bool FireHeld()
{
    return g_fireHeld;
}

volatile bool g_lookOverride = false;
float g_lookX = 0, g_lookY = 0;

void ApplyLookOverride(PadState& p)
{
    if(g_pistolCalibration||g_menuExitWait){p.thumbRX=p.thumbRY=0;return;}
    if(NativeMenuBlocked())return; // map cursor receives the physical stick, never native aim override
    // Native LB opens inventory immediately; its right stick is selection,
    // not gameplay look. Use the emitted LB, including the explicit chord.
    if(g_lookOverride&&!(p.buttons&kLeftShoulder)&&
       !(g_manualReloadInput&&g_shotgunHandling&&g_reloadAlignmentInput))
    {p.thumbRX=Axis(g_lookX);p.thumbRY=Axis(g_lookY);}
}

AmmoButtonAction AmmoAction(const Controllers& c)
{
 if(!c.x)return AmmoButtonAction::None;
 if(c.ammoAction!=AmmoButtonAction::None)return c.ammoAction;
 const bool left=c.leftGrip>.7f,right=c.rightGrip>.7f;
 return left?(right?AmmoButtonAction::PickupOverride:AmmoButtonAction::Release):(right?AmmoButtonAction::Consumed:AmmoButtonAction::Interaction);
}
bool g_gripOnlyAmmo=false;
void SetGripOnlyAmmo(bool on){g_gripOnlyAmmo=on;}
bool GripOnlyAmmoEnabled(){return g_gripOnlyAmmo;}
bool AmmoPickupOverride(const Controllers& c){return AmmoAction(c)==AmmoButtonAction::PickupOverride;}
Controllers AmmoControls(){auto c=Latest();const auto action=AmmoAction(c);if(action==AmmoButtonAction::Interaction||action==AmmoButtonAction::Consumed)c.x=false;return c;}

Controllers Latest()
{
    AcquireSRWLockShared(&g_lock);
    Controllers c = g_state;
    if(NativeMenuBlocked()||g_menuExitWait)c={}; // UI state stays in g_state; physical mechanics see no actions
    if(FromControllers(g_state).buttons&kLeftShoulder){c.leftGrip=0;c.x=false;c.leftTrigger=c.rightTrigger=0;} // wheel cannot become a physical grab or reload
    ReleaseSRWLockShared(&g_lock);
    return c;
}

void Grips(float& left, float& right)
{
    AcquireSRWLockShared(&g_lock);
    if(NativeMenuBlocked()||g_menuExitWait){left=right=0;ReleaseSRWLockShared(&g_lock);return;}
    left = (FromControllers(g_state).buttons&kLeftShoulder)?0:g_state.leftGrip;
    right = g_state.rightGrip;
    ReleaseSRWLockShared(&g_lock);
}

void SetGripContext(bool valid,bool nearWeapon){
 InterlockedExchange(&g_gripContext,valid?(nearWeapon?1:2):0);
 InterlockedExchange(&g_gripContextAt,static_cast<LONG>(GetTickCount()));
}
bool WheelOwnsGrip(){return g_triggerWheel||g_wheelLatched;}
bool DedicatedInteractionLayout(){return DedicatedLayout();}
void SetGameplayControls(bool on)
{
 if(g_gameplayControls==on)return;
 const auto c=Latest();
 // The native inventory may stop rendering the world behind its overlay.
 // Its held selection chord must survive that temporary draw-count drop.
 if(!on&&g_gameplayControls&&WheelChord(c)&&c.active&&!c.menu&&!GameplayInputBlocked())return;
 g_gameplayControls=on;g_wheelLatched=false;
 g_wheelYSpent=c.y;g_interactReleaseWait=c.x;g_crouchDown=false;g_crouchReleaseWait=c.rightY<-.35f;
 CancelReloadRequest();
 Log("[CONTROLS20] %s",on?"gameplay: X interact; right-grip+X release; left-grip+X pickup; both grips+X pickup override; grip+Y wheel":"native menu controls");
}
void SetDedicatedInteractions(bool on){g_dedicatedInteractions=on;g_wheelLatched=false;g_crouchDown=false;g_crouchReleaseWait=true;g_interactReleaseWait=Latest().x;}
bool DedicatedInteractionsEnabled(){return g_dedicatedInteractions;}
void SetSwapBX(bool on){if(g_swapBX==on)return;g_swapBX=on;g_wheelLatched=false;CancelReloadRequest();GuardMenuExit();}
bool SwapBXEnabled(){return g_swapBX;}
bool WeaponSelectionRequested(){const auto c=Latest();return DedicatedLayout()&&c.active&&!GameplayInputBlocked()&&(WheelChord(c)||(c.y&&!g_wheelYSpent));}
bool InteractionOnly()
{
 const auto c=Latest();return c.active&&c.x&&AmmoAction(c)==AmmoButtonAction::Interaction&&!g_interactReleaseWait&&!GameplayInputBlocked()&&!WheelChord(c)&&
  static_cast<LONG>(GetTickCount()-InterlockedCompareExchange(&g_reloadPulseUntil,0,0))>=0;
}
void SetBowInput(bool on,float trigger){g_bowTrigger=trigger;g_bowHandling=on;}
void BlockFire(bool on)
{
    g_fireBlocked = on;
}

void SetShotgunHandling(bool on)
{
    if(g_shotgunHandling!=on)
        Log("[CONTROLS] shotgun handling %s; X native reload, grips physical, stock aim %s",on?"on":"off",on?"filtered":"restored");
    g_shotgunHandling=on;
}
void SetOneShellX(bool on)
{
    if(g_oneShellX==on)return;
    g_oneShellX=on;
    if(on&&Latest().x)g_oneXSpent=true; // an owned press stays consumed until raw X release
}
void SetPistolCalibration(bool on){g_pistolCalibration=on;if(on){g_wheelLatched=false;CancelReloadRequest();CancelMacheteAttack();fc3aim::Suspend();}}
bool MenuOpen(){return g_pistolCalibration;}
bool NativeMenuOpen(){return NativeMenuBlocked();}
bool GameplayInputBlocked(){return g_pistolCalibration||g_menuExitWait||NativeMenuBlocked();}
bool NativeInputBlocked(){return g_pistolCalibration||(g_menuExitWait&&!NativeMenuBlocked());}
bool NativeMenuConfirmed(){return g_nativePaused&&!g_pistolCalibration;}
Controllers NativeUiControls(){AcquireSRWLockShared(&g_lock);const auto c=g_state;ReleaseSRWLockShared(&g_lock);return c;}
void SetNativeMousePointer(bool on){InterlockedExchange(&g_nativeMousePointer,on?1:0);}
void GuardMenuExit(){g_menuExitWait=true;}

void SetMagazineHandling(bool on,bool busy){g_magazineHandling=on;g_magazineBusy=on&&busy;}
void SetLeverHandling(bool on){g_leverHandling=on;}
void SetLeverXOwned(bool on){g_leverXOwned=on;}
void SetXOwned(bool on){g_xOwned=on;}
void SetManualReload(bool on,bool alignment){g_manualReloadInput=on;g_reloadAlignmentInput=on&&alignment;}
void BeginOneShellX(){g_oneXBusy=true;g_oneXSpent=true;CancelReloadRequest();}
void EndOneShellX(){g_oneXBusy=false;}
bool ManualShotgunInputActive(){return g_manualReloadInput&&g_shotgunHandling;}
bool ReloadOutput(){return (FromControllers(Latest()).buttons&kX)!=0;}
unsigned char AimOutput(){return FromControllers(Latest()).leftTrigger;}
unsigned short ButtonsOutput(){return FromControllers(Latest()).buttons;}
void RequireFullFirePull(bool on){g_fullFirePull=on;}
unsigned char FireOutput(){return FromControllers(Latest()).rightTrigger;}

void SuppressLeftGrip(bool on)
{
    g_leftGripSuppressed = on;
}

void LookStick(float& x, float& y)
{
    if(g_pistolCalibration||g_menuExitWait){x=y=0;return;}
    AcquireSRWLockShared(&g_lock);
    const bool wheel=(FromControllers(g_state).buttons&kLeftShoulder)!=0;
    x = wheel?0:g_state.rightX;
    y = wheel||DedicatedLayout()?0:g_state.rightY;
    ReleaseSRWLockShared(&g_lock);
}

void SetLookOverride(bool on, float x, float y)
{
    g_lookX = x;
    g_lookY = y;
    g_lookOverride = on;
}

void StartVirtualPad()
{
    if (GetEnvironmentVariableW(L"FC3_STEREO_NO_XR", nullptr, 0) > 0)
        return;     // offline tests: no pad
    if (HANDLE thread = CreateThread(nullptr, 0, PlugInPad, nullptr, 0, nullptr))
        CloseHandle(thread);
}

void SetMacheteHoldMode(bool on){g_macheteHoldMode=on;}
void SetMacheteContext(bool on){g_macheteContext=on;if(!on)CancelMacheteAttack();}
void SetMacheteHandling(bool on)
{
    if(g_macheteOwned&&!on)
    {
        const auto c=Latest();g_macheteReleaseWait=c.rightTrigger>=.3f;
        g_macheteClickReleaseWait=c.rightStickClick;
    }
    g_macheteOwned=on;if(!on)CancelMacheteAttack();
}
void RequestMacheteSource(){InterlockedExchange(&g_macheteSourceUntil,static_cast<LONG>(GetTickCount()+200));}
void CancelMacheteSource(){InterlockedExchange(&g_macheteSourceUntil,0);}
bool RequestMacheteAttack()
{
    const auto c=Latest();
    if(!g_macheteOwned||!g_macheteContext||!c.active||c.leftStickClick||(c.rightStickClick&&!(g_macheteHoldMode&&c.rightGrip>.7f)))return false;
    InterlockedExchange(&g_macheteAttackUntil,static_cast<LONG>(GetTickCount()+200));return true;
}
void CancelMacheteAttack(){InterlockedExchange(&g_macheteAttackUntil,0);}
bool MacheteFireBlocked(){return g_macheteOwned||g_macheteReleaseWait;}
void CancelReloadRequest(){InterlockedExchange(&g_reloadPulseUntil,0);}
void RequestReload() { if(GameplayInputBlocked()||WeaponSelectionRequested())return;InterlockedExchange(&g_reloadPulseUntil,static_cast<LONG>(GetTickCount()+200)); }

void Publish(const Controllers& incoming)
{
 RefreshNativePause();
 Controllers state=incoming.active?incoming:Controllers{};
 static bool menuWas=false;
 if(state.active&&state.menu&&!menuWas&&!g_pistolCalibration&&!g_menuExitWait&&!NativeMenuBlocked()){
  // Block immediately, before the engine handles Start/Select on its next input read.
  InterlockedExchange(&g_nativeMenuPendingUntil,static_cast<LONG>(GetTickCount()+750));
  g_wheelLatched=false;g_gripOwner=0;CancelReloadRequest();fc3aim::Suspend();
  Log("[MENU25] Start/Select requested; physical input blocked pending native pause");
 }
 menuWas=state.active&&state.menu;
 if(g_swapBX){const bool b=state.b;state.b=state.x;state.x=b;} // normalize once; all physical reload consumers use Latest
 if(!state.y)g_wheelYSpent=false;
 // Hold LEFT grip + LT for inventory; LT alone is reserved for aiming.
 if(!state.active||state.leftTrigger<.35f){g_triggerWheel=false;g_triggerWheelWait=false;}
 else if(state.menu||g_pistolCalibration||g_menuExitWait||NativeMenuBlocked()){
  g_triggerWheel=false;g_triggerWheelWait=true;
 }else if(g_triggerWheel&&state.leftGrip<.35f){
  g_triggerWheel=false;g_triggerWheelWait=true; // no reopen on regrip while LT stays held
 }else if(DedicatedLayout()&&!g_triggerWheelWait&&state.leftGrip>.7f&&state.leftTrigger>.7f)g_triggerWheel=true;
 if(g_triggerWheel){CancelReloadRequest();g_wheelYSpent=state.y;}

 if(!state.x)g_interactReleaseWait=false;
 if(!state.active||!state.x)g_ammoPress=AmmoButtonAction::None;
 else if(g_ammoPress==AmmoButtonAction::None){
  state.ammoAction=AmmoButtonAction::None;g_ammoPress=AmmoAction(state);
  if(g_pistolCalibration||g_menuExitWait||NativeMenuBlocked()||state.menu||state.y||g_triggerWheel)g_ammoPress=AmmoButtonAction::Consumed;
  if(g_ammoPress==AmmoButtonAction::Pickup||g_ammoPress==AmmoButtonAction::Release||g_ammoPress==AmmoButtonAction::PickupOverride){g_wheelLatched=false;g_gripOwner=1;}
  // Simple one-shell mode keeps its native loading cycle, entered only by an explicit pickup chord.
  if((g_ammoPress==AmmoButtonAction::Pickup||g_ammoPress==AmmoButtonAction::Release||g_ammoPress==AmmoButtonAction::PickupOverride)&&g_oneShellX&&g_shotgunHandling&&!g_manualReloadInput&&!g_oneXBusy)InterlockedExchange(&g_reloadPulseUntil,static_cast<LONG>(GetTickCount()+200));
  Log("[CONTROLS27] normalized X press owner %d (1 interaction, 2 pickup, 3 release, 4 override, 5 consumed)",static_cast<int>(g_ammoPress));
 }else if(g_ammoPress!=AmmoButtonAction::Interaction&&g_ammoPress!=AmmoButtonAction::Consumed){
  const bool needsLeft=true,needsRight=g_ammoPress==AmmoButtonAction::PickupOverride;
  if((needsLeft&&state.leftGrip<.35f)||(needsRight&&state.rightGrip<.35f)||state.y||g_triggerWheel||state.menu||g_menuExitWait||NativeMenuBlocked())g_ammoPress=AmmoButtonAction::Consumed;
 }
 state.ammoAction=g_ammoPress;

 if(!state.active||state.leftGrip<=.25f){g_wheelLatched=false;g_gripOwner=0;}
 else if(state.menu||g_pistolCalibration||g_menuExitWait||NativeMenuBlocked()){g_wheelLatched=false;g_gripOwner=1;}
 else if(state.leftGrip>.5f){
  if(state.y&&(!DedicatedLayout()||!g_wheelYSpent)){g_wheelLatched=true;g_gripOwner=2;}
  else if(!g_gripOwner){
   const bool fresh=static_cast<DWORD>(GetTickCount()-g_gripContextAt)<150;
   const bool wheel=DedicatedLayout()&&fresh&&g_gripContext==2&&!state.x&&!g_gripOnlyAmmo;
   g_gripOwner=wheel?2:1;g_wheelLatched=wheel;
   Log("[GRIP23] squeeze owns %s; context %ld fresh %d",wheel?"wheel":"weapon",g_gripContext,fresh?1:0);
  }
 }
 if(DedicatedLayout()&&g_wheelLatched){g_wheelYSpent=true;CancelReloadRequest();}
 if(g_menuExitWait&&!state.a&&!state.b&&!state.x&&!state.y&&!state.menu&&state.rightTrigger<.2f&&state.leftTrigger<.2f&&
 state.leftGrip<.35f&&state.rightGrip<.35f&&!state.leftStickClick&&!state.rightStickClick&&
 std::fabs(state.leftX)<.25f&&std::fabs(state.leftY)<.25f&&std::fabs(state.rightX)<.25f&&std::fabs(state.rightY)<.25f&&
 !(GetAsyncKeyState(VK_LBUTTON)&0x8000)&&!(GetAsyncKeyState(VK_RBUTTON)&0x8000)&&!(GetAsyncKeyState(VK_INSERT)&0x8000)&&!(GetAsyncKeyState(VK_ESCAPE)&0x8000))g_menuExitWait=false;

    const bool crouchBlocked=!state.active||!DedicatedLayout()||WheelChord(state)||state.menu||g_pistolCalibration||g_menuExitWait||g_reloadAlignmentInput;
    if(crouchBlocked){g_crouchDown=false;g_crouchReleaseWait=true;}
    else {
      if(std::fabs(state.rightY)<.35f){g_crouchDown=false;g_crouchReleaseWait=false;}
      if(!g_crouchReleaseWait&&std::fabs(state.rightY)>.7f&&std::fabs(state.rightY)>std::fabs(state.rightX)){
       const bool wantCrouch=state.rightY<0;
       bool actual=false;
       g_crouchDown=wantCrouch!=g_stanceCrouched;g_stanceCrouched=wantCrouch;
       g_crouchReleaseWait=true; // no repeat pulse, even if landing changes stance while held
      }
    }
    if(state.rightTrigger<.3f)g_macheteReleaseWait=false;
    if(!state.rightStickClick)g_macheteClickReleaseWait=false;
    if(!state.x)g_oneXSpent=false;
    else if((g_manualReloadInput&&g_shotgunHandling&&(g_reloadAlignmentInput||state.leftGrip>.7f))||(g_oneShellX&&g_shotgunHandling&&g_oneXBusy))g_oneXSpent=true;
    {static LONG reported=0;const LONG stage=g_padStage;
     if(stage&&!reported){reported=1;
      if(stage==1)Log("[PAD] virtual Xbox 360 pad plugged in (user index %lu): the Touch controllers drive the game",g_pad?vigem_target_get_index(g_pad):99ul);
      else Log("[PAD] VIRTUAL PAD FAILED (stage %ld: %s, error 0x%08X) - the controllers cannot reach the game. Restart Virtual Desktop / reboot clears a stuck ViGEm bus.",stage,stage==2?"out of memory":stage==3?"ViGEm bus did not answer":"no free pad slot",static_cast<unsigned>(g_padError));}}
    if (g_padReady)
    {
        PadState p = FromControllers(state);
        if(state.active)ApplyLookOverride(p);
        static bool wasWheel=false;
        const bool wheel=(p.buttons&kLeftShoulder)!=0;
        if(wheel!=wasWheel){Log("[WHEEL] %s; right stick %s, body turning %s",wheel?"open requested":"release",wheel?"selects":"look",wheel?"paused":"restored");wasWheel=wheel;}
        static PadState last = {};
        static bool sent = false;
        if (!sent || std::memcmp(&p, &last, sizeof(p)) != 0)
        {
            XUSB_REPORT report = {};
            report.wButtons = p.buttons;
            report.bLeftTrigger = p.leftTrigger;
            report.bRightTrigger = p.rightTrigger;
            report.sThumbLX = p.thumbLX;
            report.sThumbLY = p.thumbLY;
            report.sThumbRX = p.thumbRX;
            report.sThumbRY = p.thumbRY;
            const VIGEM_ERROR updated = vigem_target_x360_update(g_client, g_pad, report);
            static LONG failures = 0;
            if (!VIGEM_SUCCESS(updated) && InterlockedIncrement(&failures) <= 3)
                Log("[PAD] virtual pad update failed (0x%08X)", static_cast<unsigned>(updated));
            last = p;
            sent = true;
        }
        LogNewButtons(p.buttons);
    }
    static bool wasActive = false;
    if (state.active != wasActive)
    {
        wasActive = state.active;
        Log("[PAD] controllers %s", state.active ? "active" : "not reported by the headset");
    }
    static bool priorX=false;
    if(state.x!=priorX){Log("[CONTROLS] controller X %s -> virtual X %d (shotgun handling %d)",state.x?"down":"up",(FromControllers(state).buttons&kX)!=0,g_shotgunHandling);priorX=state.x;}
    if (state.rightTrigger > 0.8f) g_firePressed = true;
    g_fireHeld = !GameplayInputBlocked()&&state.rightTrigger > 0.8f;
    static ULONGLONG heldSince = 0;
    static bool done = false;
    if (!GameplayInputBlocked() && state.leftStickClick && state.rightStickClick)
    {
        const ULONGLONG now = GetTickCount64();
        if (heldSince == 0) heldSince = now;
        if (!done && now - heldSince >= 1000)
        {
            done = true;
            InterlockedExchange(&g_calibrate, 1);
        }
    }
    else
    {
        heldSince = 0;
        done = false;
    }
    // Sticks: say once when each one first moves, from the controller side.
    static bool moved[2] = {};
    const float m[2] = { std::fabs(state.leftX) + std::fabs(state.leftY), std::fabs(state.rightX) + std::fabs(state.rightY) };
    for (int s = 0; s < 2; ++s)
        if (!moved[s] && m[s] > 0.5f)
        {
            moved[s] = true;
            Log("[PAD] %s stick moves on the controller (%.2f, %.2f)", s == 0 ? "left" : "right",
                s == 0 ? state.leftX : state.rightX, s == 0 ? state.leftY : state.rightY);
        }
    AcquireSRWLockExclusive(&g_lock);
    g_state = state;
    ReleaseSRWLockExclusive(&g_lock);
    static LONG reported = 0;
    if (++reported % 3600 == 0)
        Log("[PAD] game polls %ld, polls answered with controller input %ld", g_calls, g_injected);
}
}



