#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "aim_probe.h"
#include "fc3_log.h"
#include "locker_probe329.h"
#include "xr_presenter.h"
#include "muzzle58.h"
#include <MinHook.h>

// Native CPawn look access. No sweep, synthetic stick movement, camera nudge,
// or heap pointer retained. Engine calls run at the existing game input seam.
namespace
{
constexpr double kPi=3.14159265358979323846;
constexpr DWORD kFreshMs=250,kCommandMs=150;
using LocalHandle=UINT* (__cdecl*)(UINT*);
using SetAngles=void (__stdcall*)(const UINT*,float,float,float);
LocalHandle g_localHandle=nullptr;SetAngles g_setAngles=nullptr;
void* g_getAngles=nullptr;void* g_getActual=nullptr;
UINT g_engine=0,g_engineBytes=0;
SRWLOCK g_nativeLock=SRWLOCK_INIT;
struct Sample
{
    UINT handle[2]={~0u,~0u};float angles[3]={},actual[3]={};
    ULONGLONG ms=0;DWORD thread=0;UINT sequence=0;bool valid=false;
    int action128=-1;ULONGLONG idleSince128=0;
    int cameraAction282=-1;UINT actionEpoch282=0; // raw owner, never the arm-release grace period
    UINT actionType283=0;
    int vehicle=-1,seat=-1; // seat is a legacy name: CVehicle type, NOT a seat index (audit169).
    int slide280=-1;
} g_sample;
// Draws can lag the latest input poll. Match the observed camera to recent
// samples of this same player before deciding that an animation owns it.
Sample g_cameraHistory127[32]={};UINT g_cameraHistoryCount127=0;
void RecordCameraSample127(const Sample& sample){if(sample.valid)g_cameraHistory127[g_cameraHistoryCount127++%32]=sample;}

// The engine's own vehicle query (candidate 106; naming corrected by static audit169).
// GetPawnVehicleType (handler RVA 0x677330) resolves a pawn from its 64-bit
// entity handle (cdecl RVA 0x657c40, the handle's two dwords as arguments) and asks the pawn for
// its CVehicle component (thiscall RVA 0x13770a0; null = not in a vehicle; TYPE is at +0x14).
// Both helpers release their internal temporary references before returning borrowed pointers.
// Keep reads in this input-thread call; never retain those pointers. The old `seat` output name
// is preserved for compatibility, but cannot identify seat changes or same-type vehicle changes.
// See tools/trace_vehicle169.py: actual GetPawnVehicleSeat is 0x677270, not 0x677330.
void* g_pawnFromHandle=nullptr;void* g_vehicleSeatOf=nullptr;
int g_vehicleStateForTest=-1;
bool g_vehicleSampleForTest187=false;
fc3aim::VehicleSample170 g_vehicleTest187={};
int NativeVehicle(const UINT* handle,int& seat)
{
    seat=-1;
    if(!g_pawnFromHandle||!g_vehicleSeatOf)return -1;
    void* fromHandle=g_pawnFromHandle;void* seatOf=g_vehicleSeatOf;
    UINT h0=handle[0],h1=handle[1],pawn=0,component=0;
    __try
    {
        __asm {
            push h1
            push h0
            call fromHandle
            add esp,8
            mov pawn,eax
        }
        if(!pawn)return -1;
        __asm {
            mov ecx,pawn
            call seatOf
            mov component,eax
        }
        if(!component)return 0;
        seat=*reinterpret_cast<const int*>(component+0x14);
        return 1;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return -1;}
}
// Call sites inside GetPawnVehicleType must still target the two functions.
bool VehicleSignatures(const BYTE* base)
{
    auto target=[&](UINT site)->UINT{if(base[site]!=0xE8)return 0;int rel=0;memcpy(&rel,base+site+1,4);return site+5+rel;};
    const BYTE seatRead[]={0x8b,0x40,0x14}; // mov eax,[eax+14h]
    const BYTE pawnPrologue[]={0x55,0x8b,0xec,0x83,0xec,0x08,0x8b,0x45,0x0c,0x8b,0x4d,0x08};
    const BYTE seatPrologue[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x08,0x56,0x57,0x85,0xc9};
    return target(0x6773b9)==0x657c40&&target(0x6773cc)==0x13770a0&&!memcmp(base+0x6773d5,seatRead,3)&&
        !memcmp(base+0x657c40,pawnPrologue,sizeof(pawnPrologue))&&!memcmp(base+0x13770a0,seatPrologue,sizeof(seatPrologue))&&
        target(0x66f7c0)==0x657c40&&target(0x66f7d0)==0x13770a0; // actual GetPawnVehicleSeat uses the same pair
}
#include "vehicle_sample170.inl"
#include "action_owner128.inl"
struct Command
{UINT handle[2]={};float angles[3]={};ULONGLONG ms=0;bool active=false,visibility=false;double headYaw=0,headPitch=0,yawSign=1,pitchSign=1,pitchOffset=0;} g_command;
struct Visibility68 {UINT handle[2]={~0u,~0u};double appliedYaw=0,appliedPitch=0;bool owned=false;}g_visibility68;
ULONGLONG g_requestMs=0,g_lastPoll=0,g_installRetry=0;
bool g_requested=false,g_ready=false;
bool g_hiddenCameraBlocked127=false;
UINT g_boundHandle[2]={~0u,~0u},g_lastSequence=0;
int g_votes=0,g_badFrames=0,g_yawForm=-1,g_pitchForm=-1;
struct Form {double sign,offset;};
constexpr Form kYawForms[]={{1,0},{-1,0},{1,kPi/2},{1,-kPi/2},
                          {-1,kPi/2},{-1,-kPi/2},{1,kPi},{-1,kPi}};
constexpr Form kPitchForms[]={{1,0},{-1,0},{-1,kPi/2},{1,kPi/2}};
double Wrap(double a){return std::remainder(a,2*kPi);}
bool SameHandle(const UINT* a,const UINT* b){return a[0]==b[0]&&a[1]==b[1];}
bool FiniteAngles(const float* a)
{return std::isfinite(a[0])&&std::isfinite(a[1])&&std::isfinite(a[2])&&
    std::fabs(a[0])<100&&std::fabs(a[1])<100&&std::fabs(a[2])<100000;}
bool NativeRead(void* target,const UINT* handle,float* output)
{
    // Getter: handle in EAX, output on stack, callee ret 4.
    if(!target)return false;output[0]=output[1]=output[2]=NAN;
    __try
    {
        __asm {
            push output
            mov eax,handle
            call target
        }
        return FiniteAngles(output);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool NativeLocal(UINT* handle)
{
    if(!g_localHandle)return false;
    __try
    {
        const UINT manager=*reinterpret_cast<const UINT*>(g_engine+0x2829474);
        if(!manager)return false;
        const UINT count=*reinterpret_cast<const UINT*>(manager+0xc);
        if(count==0||count>4)return false;
        return g_localHandle(handle)==handle&&handle[0]!=~0u&&handle[1]!=~0u;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool NativeSet(const UINT* handle,const float* a)
{
    // Verified stdcall adapter wraps the custom vector setter; it writes both
    // actual and desired vectors and releases its native entity reference.
    if(!g_setAngles||!FiniteAngles(a))return false;
    __try {g_setAngles(handle,a[0],a[1],a[2]);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool NativeSignatures(const BYTE* base)
{
    const BYTE local[]={0x55,0x8b,0xec,0x51,0xa1};
    const BYTE getter[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x51,0x8b,0x08,0x8b,0x40,0x04};
    const BYTE setter[]={0x55,0x8b,0xec,0xf3,0x0f,0x10,0x45,0x10,0xf3,0x0f,0x10,0x4d,0x14};
    if(memcmp(base+0xc80190,local,sizeof(local))||
       memcmp(base+0x17cd220,getter,sizeof(getter))||
       memcmp(base+0x17cd2d0,getter,sizeof(getter))||
       memcmp(base+0x17d7de0,setter,sizeof(setter)))return false;
    UINT managerOperand=0;memcpy(&managerOperand,base+0xc80195,4);
    return managerOperand==reinterpret_cast<UINT>(base)+0x2829474;
}
#include "vehicle_camera279.inl"
bool InstallNative()
{
    if(g_engine)return true;
    auto* base=reinterpret_cast<BYTE*>(GetModuleHandleW(L"FC3_d3d11.dll"));if(!base)return false;
    __try
    {
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||
           nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800||
           !NativeSignatures(base))return false;
        AcquireSRWLockExclusive(&g_nativeLock);
        g_localHandle=reinterpret_cast<LocalHandle>(base+0xc80190);
        g_getActual=base+0x17cd220;g_getAngles=base+0x17cd2d0;
        g_setAngles=reinterpret_cast<SetAngles>(base+0x17d7de0);
        const bool vehicleOk=VehicleSignatures(base);
        const bool actionOk=ActionSignatures128(base);
        g_extraActionBound283=actionOk&&ExtraActionSignatures283(base);
        g_slideBound280=actionOk&&SlideSignatures280(base);
        if(actionOk)g_beautifierOf128=base+0x657cc0;
        if(vehicleOk){g_pawnFromHandle=base+0x657c40;g_vehicleSeatOf=base+0x13770a0;}
        g_vehicleProbeBound170=vehicleOk&&VehicleProbeSignatures170(base);
        g_vehicleDriverBound173=g_vehicleProbeBound170&&VehicleDriverSignatures173(base);
        g_vehicleSeatLookup170=g_vehicleProbeBound170?reinterpret_cast<VehicleSeatLookup170>(base+0x2d3360):nullptr;
        g_engineBytes=nt->OptionalHeader.SizeOfImage;g_engine=reinterpret_cast<UINT>(base);
        ReleaseSRWLockExclusive(&g_nativeLock);
        fc3::Log("[AIM NATIVE] verified look access installed; no startup camera sweep");
        fc3::Log(actionOk?"[ACTION128] native beautifier ownership verified":"[ACTION128] signatures refused; native action arm ownership unavailable");
        fc3::Log(g_extraActionBound283?"[ACTION283] exact pickup/cinematic/carry/door registrations verified; generic no-control excluded":"[ACTION283] additional action identities refused");
        fc3::Log(g_vehicleProbeBound170?"[VEHICLE170] read-only identity/actual-seat/basis signatures verified; no forward axis assumed":"[VEHICLE170] diagnostic signatures refused; legacy vehicle ownership unchanged");
        fc3::Log(vehicleOk?"[VEHICLE STATE] engine vehicle-seat query verified (GetPawnVehicleSeat path)":"[VEHICLE STATE] engine vehicle-seat query NOT verified; vehicle camera falls back to the head-turn test");
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
int MatchForm(double value,double angle,const Form* forms,int count,bool circular)
{
    double best=100,second=100;int match=-1;
    for(int i=0;i<count;++i)
    {
        const double d=value-(forms[i].sign*angle+forms[i].offset);
        const double error=std::fabs(circular?Wrap(d):d);
        if(error<best){second=best;best=error;match=i;}else if(error<second)second=error;
    }
    return match>=0&&best<.06&&second-best>.025?match:-1;
}
void LoseBinding()
{g_ready=false;g_votes=0;g_badFrames=0;g_yawForm=g_pitchForm=-1;fc3aim::Suspend();}
}
#include "muzzle58.inl"
#include "selector_native290.inl"
#include "god_mode210.inl"
#include "heal_observer299.inl"
namespace fc3aim
{
bool SetVehicleCamera279(bool active,const float* view,const float* right,const float* up,const float* forward,const float* body,bool foot){return vehicle279::Set(active,view,right,up,forward,body,foot);}
bool NativeFootCamera281(){AcquireSRWLockShared(&g_nativeLock);const Sample s=g_sample;ReleaseSRWLockShared(&g_nativeLock);const auto now=GetTickCount64();return s.valid&&s.vehicle==0&&s.slide280!=1&&(s.action128<=0||vehicle279::HandoffOwns284(s,now))&&now>=s.ms&&now-s.ms<=kFreshMs;}
bool NativeHandoff284(){AcquireSRWLockShared(&g_nativeLock);const Sample s=g_sample;ReleaseSRWLockShared(&g_nativeLock);return vehicle279::HandoffOwns284(s,GetTickCount64());}
int NativeActionCamera282(){AcquireSRWLockShared(&g_nativeLock);const Sample s=g_sample;ReleaseSRWLockShared(&g_nativeLock);return vehicle279::Action282(s,GetTickCount64())?s.cameraAction282:0;}
bool SlideCamera280(){AcquireSRWLockShared(&g_nativeLock);const Sample s=g_sample;ReleaseSRWLockShared(&g_nativeLock);const auto now=GetTickCount64();return s.valid&&s.vehicle==0&&s.slide280==1&&s.action128<=0&&now>=s.ms&&now-s.ms<=kFreshMs;}
bool VehicleCameraBasis279(const float* right,const float* up,const float* forward,float* body,UINT& serial){return vehicle279::Find(right,up,forward,body,serial);}
void InputTick(const void* caller)
{
    const auto now=GetTickCount64();
    if(!TryAcquireSRWLockExclusive(&g_nativeLock))return;
    const UINT site=reinterpret_cast<UINT>(caller);
    if(!g_engine||site<g_engine||site-g_engine>=g_engineBytes||
       !g_requested||now-g_requestMs>kFreshMs||now-g_lastPoll<5)
    {ReleaseSRWLockExclusive(&g_nativeLock);return;}
    g_lastPoll=now;
    Sample next;next.ms=now;next.thread=GetCurrentThreadId();next.sequence=g_sample.sequence+1;next.actionEpoch282=g_sample.actionEpoch282;
    next.valid=NativeLocal(next.handle)&&NativeRead(g_getAngles,next.handle,next.angles)&&
        NativeRead(g_getActual,next.handle,next.actual);
    InterlockedExchange64(&g_healLocal299,next.valid?static_cast<LONGLONG>((static_cast<unsigned long long>(next.handle[1])<<32)|next.handle[0]):0);
    if(next.valid){
        next.vehicle=NativeVehicle(next.handle,next.seat);
        static ULONGLONG laserPoll194=0;
        if(now-laserPoll194>=8){laserPoll194=now;SampleEquipped206(next.handle);TickLaser194();} // 206: only the hit DISTANCE lags; the ray is re-anchored to the drawn gun every frame
        SampleVehicle170(next.handle,next.vehicle,next.seat,now);
        next.action128=NativeAction128(next.handle,&next.slide280,&next.actionType283);
        const bool same=SameHandle(next.handle,g_sample.handle);
        next.cameraAction282=next.action128;
        next.actionEpoch282=g_sample.actionEpoch282;
        if(!same||!g_sample.valid||next.cameraAction282!=g_sample.cameraAction282){if(!++next.actionEpoch282)++next.actionEpoch282;}
        if(same&&g_sample.action128>0&&next.action128<=0){
            next.idleSince128=next.action128==0?(g_sample.idleSince128?g_sample.idleSince128:now):0;
            if(!next.idleSince128||now-next.idleSince128<150)next.action128=g_sample.action128;
        }
        if(next.action128>0||next.vehicle==1||next.slide280==1||vehicle279::FootOwns281(next,now)){g_command.active=false;g_visibility68={};}
        RecordCameraSample127(next);
    }
    if(!next.valid){if(g_sample.valid){if(!++next.actionEpoch282)++next.actionEpoch282;}g_vehicleSample170={};g_vehicleProbeAt170=0;g_vehicleProbePawn170=0;next.action128=g_sample.action128;next.idleSince128=0;memcpy(next.handle,g_sample.handle,sizeof(next.handle));}
    InterlockedExchange(&g_nativeActionOwned128,next.action128>0?1:0);
    static int lastAction128=-1;
    static UINT lastType283=~0u;static unsigned typeLogs283=0;
    if(next.valid&&next.actionType283!=lastType283&&typeLogs283<200){lastType283=next.actionType283;++typeLogs283;fc3::Log("[ACTION283] current native typeRVA=%08X mask=%d epoch=%u",next.actionType283,next.cameraAction282,next.actionEpoch282);}
    static UINT lastActionEpoch282=0;static unsigned actionLogs282=0;
    if(next.actionEpoch282!=lastActionEpoch282&&actionLogs282<600){lastActionEpoch282=next.actionEpoch282;++actionLogs282;fc3::Log("[ACTION282] raw native owner mask=%d epoch=%u valid=%d; camera ownership ends before arm-release grace",next.cameraAction282,next.actionEpoch282,next.valid?1:0);}
    static int lastSlide280=-1;
    if(next.valid&&next.slide280!=lastSlide280){lastSlide280=next.slide280;fc3::Log("[SLIDE280] native slide state %d (-1 unknown, 0 other, 1 CPawnBeautifierSlide)",next.slide280);}
    if(next.valid&&next.action128!=lastAction128){lastAction128=next.action128;fc3::Log("[ACTION128] native owner mask %d (0 gameplay; bits climb/zipline/takedown/mp/ladder/vault; generic no-control excluded130); idle confirmed before controller restore",lastAction128);}
    if(!next.valid||!SameHandle(next.handle,g_sample.handle))g_command.active=false;
    // A camera-ownership dropout does not undo the last successful native look
    // write. Keep its offset for this SAME player so resume cannot double it.
    if(next.valid&&!SameHandle(next.handle,g_visibility68.handle))g_visibility68={};
    if(next.valid&&g_command.active&&SameHandle(next.handle,g_command.handle)&&now-g_command.ms<=kCommandMs)
    {
        float command[3];memcpy(command,g_command.angles,sizeof(command));double effectiveHeadPitch=g_command.headPitch;
        if(g_command.visibility){
            // Consume an absolute HMD offset once. A newer render request may
            // replace an older one without dropping motion. Native stick look
            // since the last sample remains in next.angles, not overwritten.
            const bool same=SameHandle(next.handle,g_visibility68.handle);
            const double oldYaw=same?g_visibility68.appliedYaw:0,oldPitch=same?g_visibility68.appliedPitch:0;
            command[0]=float(next.angles[0]+g_command.pitchSign*(g_command.headPitch-oldPitch));
            command[2]=float(next.angles[2]+g_command.yawSign*Wrap(g_command.headYaw-oldYaw));
            const double pitchLimit=80*kPi/180;const Form form={g_command.pitchSign,g_command.pitchOffset};
            const double worldPitch=(command[0]-form.offset)/form.sign;
            command[0]=float(form.sign*(std::max)(-pitchLimit,(std::min)(pitchLimit,worldPitch))+form.offset);
            effectiveHeadPitch=oldPitch+(command[0]-next.angles[0])/form.sign;
            if(fabs(command[0]-next.angles[0])<1e-7&&fabs(Wrap(command[2]-next.angles[2]))<1e-7){g_command.active=false;RecordCameraSample127(next);g_sample=next;ReleaseSRWLockExclusive(&g_nativeLock);return;}
        }
        command[1]=next.actual[1]; // retain the game's roll
        next.valid=NativeSet(next.handle,command)&&NativeRead(g_getAngles,next.handle,next.angles)&&NativeRead(g_getActual,next.handle,next.actual);
        if(next.valid&&(std::fabs(next.angles[0]-command[0])>.002||std::fabs(Wrap(next.angles[2]-command[2]))>.002))next.valid=false;
        if(next.valid){
            if(g_command.visibility){memcpy(g_visibility68.handle,next.handle,sizeof(next.handle));g_visibility68.appliedYaw=g_command.headYaw;g_visibility68.appliedPitch=effectiveHeadPitch;}
            else g_visibility68={};
        }else g_command.active=false;
        if(g_command.visibility)g_command.active=false; // head delta is never applied twice
    }
    if(vehicle279::PromoteExit284(next,now))fc3::Log("[HANDOFF284] native action exit promoted before camera update; owner=%08X:%08X epoch=%u; native hands retain their release grace",next.handle[0],next.handle[1],next.actionEpoch282);
    RecordCameraSample127(next);g_sample=next;ReleaseSRWLockExclusive(&g_nativeLock);
}
void Suspend()
{AcquireSRWLockExclusive(&g_nativeLock);g_command.active=false;g_visibility68.owned=false;ReleaseSRWLockExclusive(&g_nativeLock);ClearMuzzle58();}
void Probe(const float forward[3],bool valid,bool gameplay,bool weaponVisible)
{
    const auto now=GetTickCount64();
    if(!valid||!gameplay||!weaponVisible)ClearMuzzle58();
    if(!g_engine&&now>=g_installRetry){g_installRetry=now+1000;InstallNative();}
    if(g_engine){InstallMuzzle58();InstallGodMode210();InstallHealObserver299();}
    AcquireSRWLockExclusive(&g_nativeLock);
    g_requested=valid&&gameplay;g_requestMs=now;
    if(!g_requested||(!weaponVisible&&!g_command.visibility))g_command.active=false;
    if(!g_requested){g_vehicleSample170={};g_vehicleProbeAt170=0;g_vehicleProbePawn170=0;}
    const Sample sample=g_sample;ReleaseSRWLockExclusive(&g_nativeLock);
    fc3xr::SetTestNote(!g_engine?L"NATIVE AIM ACCESS UNAVAILABLE":
        !weaponVisible?(Ready()?L"HIDDEN WEAPON - HEAD VISIBILITY CAMERA":L"HIDDEN WEAPON - CHECKING CAMERA OWNER"):
        Ready()?L"AIM READY - POINT THE SHOTGUN":L"CHECKING PLAYER AIM - HOLD NORMALLY");
    // Correlate existing owner-bound samples with rendered cameras; no new reads/writes.
    static ULONGLONG next67=0;static unsigned lines67=0;
    if(gameplay&&now>=next67&&lines67<1200){next67=now+500;++lines67;
        fc3::Log("[CAMERA67] tick%llu nativeTick%llu sequence%u player%08X:%08X valid%d ready%d visible%d desired %.7f/%.7f/%.7f actual %.7f/%.7f/%.7f renderedForward %.7f/%.7f/%.7f; sampled player look, not native visibility ownership",
            now,sample.ms,sample.sequence,sample.handle[0],sample.handle[1],sample.valid?1:0,Ready()?1:0,weaponVisible?1:0,sample.angles[0],sample.angles[1],sample.angles[2],sample.actual[0],sample.actual[1],sample.actual[2],forward[0],forward[1],forward[2]);
    }
    if(!sample.valid||now-sample.ms>kFreshMs)
    {
        if(g_ready)fc3::Log("[AIM NATIVE] player unavailable; ordinary look controls retained");
        LoseBinding();return;
    }
    if(!SameHandle(sample.handle,g_boundHandle))
    {
        LoseBinding();memcpy(g_boundHandle,sample.handle,sizeof(g_boundHandle));g_lastSequence=0;
        fc3::Log("[AIM NATIVE] local player %08X:%08X input thread %lu; desired %.5f %.5f %.5f actual %.5f %.5f %.5f",
            sample.handle[0],sample.handle[1],sample.thread,sample.angles[0],sample.angles[1],sample.angles[2],
            sample.actual[0],sample.actual[1],sample.actual[2]);
    }
    if(!valid||!gameplay){Suspend();return;}
    const double norm=double(forward[0])*forward[0]+double(forward[1])*forward[1]+double(forward[2])*forward[2];
    if(!std::isfinite(norm)||std::fabs(norm-1)>.05){Suspend();return;}
    const double yaw=std::atan2(forward[1],forward[0]);
    const double pitch=std::asin((std::max)(-1.,(std::min)(1.,double(forward[2]))));
    if(g_ready)
    {
        const double dy=std::fabs(Wrap(sample.angles[2]-kYawForms[g_yawForm].sign*yaw-kYawForms[g_yawForm].offset));
        const double dp=std::fabs(sample.angles[0]-kPitchForms[g_pitchForm].sign*pitch-kPitchForms[g_pitchForm].offset);
        if(!weaponVisible){
            // The captured climb kept driving CPawn look for12 disagreeing
            // frames while the rendered action camera had another owner.
            // Allow only ordinary input/draw latency, then stop immediately.
            double error=(std::max)(dy,dp);
            AcquireSRWLockShared(&g_nativeLock);
            for(const auto& old:g_cameraHistory127)if(old.valid&&SameHandle(old.handle,sample.handle)&&now>=old.ms&&now-old.ms<=60){
                const double hy=fabs(Wrap(old.angles[2]-kYawForms[g_yawForm].sign*yaw-kYawForms[g_yawForm].offset));
                const double hp=fabs(old.angles[0]-kPitchForms[g_pitchForm].sign*pitch-kPitchForms[g_pitchForm].offset);
                error=(std::min)(error,(std::max)(hy,hp));
            }
            ReleaseSRWLockShared(&g_nativeLock);
            if(error>12*kPi/180){
                fc3::Log("[CAMERA127] hidden native/render camera diverged %.2f degrees across60ms input history; release look and rebase after passive reacquisition",error*180/kPi);
                LoseBinding();g_hiddenCameraBlocked127=true;
                // A confirmed camera-owner transition releases reprojection.
                // Its new raw native view becomes the next head-offset base.
                // Ordinary pause/stale samples still preserve68 bookkeeping.
                AcquireSRWLockExclusive(&g_nativeLock);g_visibility68={};ReleaseSRWLockExclusive(&g_nativeLock);
                return;
            }
            g_badFrames=0;return; // matching recent native sample owns this hidden view
        }
        if(dy>.45||dp>.45)++g_badFrames;else g_badFrames=0;
        if(g_badFrames>=12){fc3::Log("[AIM NATIVE] camera changed ownership; releasing aim");LoseBinding();}
        return;
    }
    if(sample.sequence==g_lastSequence)return;g_lastSequence=sample.sequence;
    // Accepted run 81220 causally established desired yaw = camera yaw - pi/2
    // and positive pitch. Its paired actual/desired candidates differ by 0x210,
    // exactly the native CPawn vectors' separation. Verify that known convention
    // on this owner; do not require the user to move to disambiguate level aim.
    const int yf=3,pf=0;
    const bool agrees=std::fabs(Wrap(sample.angles[2]-yaw+kPi/2))<.06&&
        std::fabs(sample.angles[0]-pitch)<.06;
    if(!agrees){g_votes=0;return;}
    if(yf!=g_yawForm||pf!=g_pitchForm){g_yawForm=yf;g_pitchForm=pf;g_votes=0;}
    if(++g_votes>=12)
    {g_ready=true;g_hiddenCameraBlocked127=false;fc3::Log("[AIM NATIVE] READY local player; yaw form %d pitch form %d; no sweep or test write",yf,pf);}
}
bool HiddenCameraBlocked127(){return g_hiddenCameraBlocked127;}
void ProbeNativeView283(const float* right,const float* up,const float* forward,bool valid,bool gameplay,bool weaponVisible){
 float source[3],body[9];UINT serial=0;
 // Bind plain pawn angles only against the unmodified source of a matched
 // on-foot draw. An HMD-composed output cannot establish that convention.
 const bool matched=valid&&gameplay&&right&&up&&forward&&vehicle279::Find(right,up,forward,body,serial,source);
 Probe(matched?source:forward,valid,gameplay,weaponVisible);
}
bool Ready()
{
    AcquireSRWLockShared(&g_nativeLock);const Sample s=g_sample;ReleaseSRWLockShared(&g_nativeLock);
    return g_ready&&s.valid&&SameHandle(s.handle,g_boundHandle)&&GetTickCount64()-s.ms<=kFreshMs;
}
bool LocalPlayerIdentity143(unsigned long long& entityId){
    // A launch hook can run inside native input work; never wait recursively
    // for this lock. Unknown identity refuses trajectory writes.
    entityId=0;if(!TryAcquireSRWLockShared(&g_nativeLock))return false;
    const Sample s=g_sample;ReleaseSRWLockShared(&g_nativeLock);
    const auto now=GetTickCount64();
    if(!s.valid||now<s.ms||now-s.ms>kFreshMs||s.handle[0]==~0u||s.handle[1]==~0u)return false;
    entityId=static_cast<unsigned long long>(s.handle[0])|(static_cast<unsigned long long>(s.handle[1])<<32);
    return entityId!=0&&entityId!=~0ULL;
}
bool VisibilityBasis68(double& yaw,double& pitch,double& roll){
    if(!Ready())return false;
    AcquireSRWLockShared(&g_nativeLock);const auto sample=g_sample;const auto visibility=g_visibility68;ReleaseSRWLockShared(&g_nativeLock);
    if(!sample.valid||!SameHandle(sample.handle,g_boundHandle)||g_yawForm<0||g_pitchForm<0)return false;
    const bool same=SameHandle(sample.handle,visibility.handle);
    yaw=Wrap((sample.angles[2]-kYawForms[g_yawForm].offset)/kYawForms[g_yawForm].sign-(same?visibility.appliedYaw:0));
    pitch=(sample.angles[0]-kPitchForms[g_pitchForm].offset)/kPitchForms[g_pitchForm].sign-(same?visibility.appliedPitch:0);
    roll=sample.actual[1];return std::isfinite(yaw)&&std::isfinite(pitch)&&std::isfinite(roll);
}
bool NativeLookYaw(double& yaw){
    if(!Ready())return false;
    AcquireSRWLockShared(&g_nativeLock);const Sample s=g_sample;ReleaseSRWLockShared(&g_nativeLock);
    if(!s.valid||!SameHandle(s.handle,g_boundHandle)||g_yawForm<0)return false;
    yaw=Wrap((s.angles[2]-kYawForms[g_yawForm].offset)/kYawForms[g_yawForm].sign);return std::isfinite(yaw);
}
bool NativeActionOwnsArms128(){
    if(g_actionTest128>=0)return (g_actionTest128&0x1f3f)!=0;
    return InterlockedCompareExchange(&g_nativeActionOwned128,0,0)!=0; // Only confirmed idle/new player releases native ownership.
}
void SetNativeActionForTest128(int state){g_actionTest128=state;}
bool VehicleState(bool& inVehicle,int& seat){
    if(g_vehicleStateForTest>=0){inVehicle=g_vehicleStateForTest==1;seat=inVehicle?0:-1;return true;}
    AcquireSRWLockShared(&g_nativeLock);const Sample s=g_sample;ReleaseSRWLockShared(&g_nativeLock);
    if(!s.valid||s.vehicle<0||GetTickCount64()-s.ms>kFreshMs)return false;
    inVehicle=s.vehicle==1;seat=s.seat;return true;
}
bool GetVehicleSample170(VehicleSample170& out){
    if(g_vehicleSampleForTest187){out=g_vehicleTest187;return out.identityValid;}
    out={};if(!TryAcquireSRWLockShared(&g_nativeLock))return false;
    out=g_vehicleSample170;const Sample player=g_sample;
    ReleaseSRWLockShared(&g_nativeLock);
    const auto now=GetTickCount64();
    if(!player.valid||player.vehicle!=1||!out.identityValid||out.type!=player.seat||out.pawnId!=VehicleId170(player.handle)||
       now<out.ms||now-out.ms>kFreshMs||now<player.ms||now-player.ms>kFreshMs){out={};return false;}
    return true;
}
bool GetVehicleDrawPose352(VehicleSample170& out){
    if(!GetVehicleSample170(out)||!out.basisValid)return false;
    if(g_vehicleSampleForTest187)return true;
    UINT entity=0;
    AcquireSRWLockShared(&g_nativeLock);
    if(g_vehicleSample170.vehicleId==out.vehicleId)entity=g_vehicleEntity352;
    ReleaseSRWLockShared(&g_nativeLock);
    if(!entity)return false;
    __try {
        if(VehicleId170(reinterpret_cast<const UINT*>(entity+8))!=out.vehicleId)return false;
        for(int axis=0;axis<3;++axis)memcpy(out.basis[axis],reinterpret_cast<const void*>(entity+0x20+axis*0x10),12);
        memcpy(out.position,reinterpret_cast<const void*>(entity+0x50),12);
        if(VehicleId170(reinterpret_cast<const UINT*>(entity+8))!=out.vehicleId||!VehicleBasisValid170(out))return false;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    return true;
}
void SetVehicleSampleForTest187(const VehicleSample170* sample){
    g_vehicleSampleForTest187=sample!=nullptr;
    g_vehicleTest187=sample?*sample:VehicleSample170{};
}
void SetVehicleStateForTest(int state){g_vehicleStateForTest=state;}
void EndVisibility68(){AcquireSRWLockExclusive(&g_nativeLock);if(g_command.visibility)g_command.active=false;g_visibility68.owned=false;ReleaseSRWLockExclusive(&g_nativeLock);}
bool DriveVisibility68(double headYaw,double headPitch){
    if(!Ready()||!std::isfinite(headYaw)||!std::isfinite(headPitch)||fabs(headPitch)>kPi+.001)return false;
    AcquireSRWLockExclusive(&g_nativeLock);
    const bool valid=g_sample.valid&&SameHandle(g_sample.handle,g_boundHandle)&&g_yawForm>=0&&g_pitchForm>=0;
    if(valid){memcpy(g_command.handle,g_sample.handle,sizeof(g_command.handle));g_command.visibility=true;g_command.yawSign=kYawForms[g_yawForm].sign;g_command.pitchSign=kPitchForms[g_pitchForm].sign;g_command.pitchOffset=kPitchForms[g_pitchForm].offset;g_command.headYaw=Wrap(headYaw);g_command.headPitch=headPitch;g_command.ms=GetTickCount64();g_command.active=true;g_visibility68.owned=true;}
    ReleaseSRWLockExclusive(&g_nativeLock);return valid;
}
void Drive(double yaw,double pitch)
{
    if(!Ready()||!std::isfinite(yaw)||!std::isfinite(pitch))return;
    AcquireSRWLockExclusive(&g_nativeLock);
    if(g_sample.valid&&SameHandle(g_sample.handle,g_boundHandle))
    {
        g_command.visibility=false;g_visibility68.owned=false;
        memcpy(g_command.handle,g_sample.handle,sizeof(g_command.handle));
        g_command.angles[0]=float(kPitchForms[g_pitchForm].sign*pitch+kPitchForms[g_pitchForm].offset);
        g_command.angles[1]=g_sample.actual[1];
        const double target=kYawForms[g_yawForm].sign*yaw+kYawForms[g_yawForm].offset;
        g_command.angles[2]=float(g_sample.angles[2]+Wrap(target-g_sample.angles[2]));
        g_command.ms=GetTickCount64();g_command.active=true;
    }
    ReleaseSRWLockExclusive(&g_nativeLock);
}
}
