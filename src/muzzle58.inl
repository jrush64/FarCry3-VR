// Included in aim_probe.cpp so the verified local-player identity and native
// engine guards are shared. No origin modification occurs outside the actual
// bullet strategy's vector dispatcher (verified sole caller RVA130631C).
namespace {
struct MuzzlePublication58 {float move[12]={},root[3]={},aim[3]={},cameraForward[3]={},native[12]={};UINT wrapper=0,weapon=0,profile=~0u,epoch=0;ULONGLONG ms=0;bool active=false,aimValid=false,nativeValid=false,mirrored340=false;} g_muzzle58;
volatile LONG g_spreadPercent206=0;volatile LONG g_shotCorrected206=0,g_shotRejected206=0;
bool ShotgunProfile206(UINT profile){return profile==0||profile==3||profile==4||profile==5||profile==6||profile==34;} // 1887, 870 family, SPAS: pellets keep the native pattern
// Row-major 3x4 rigid inverse: R^T and -R^T t.
bool InvertFrame206(const float* m,float* out){for(int i=0;i<12;++i)if(!std::isfinite(m[i]))return false;for(int r=0;r<3;++r){for(int c=0;c<3;++c)out[r*4+c]=m[c*4+r];out[r*4+3]=0;}for(int r=0;r<3;++r){double t=0;for(int c=0;c<3;++c)t-=double(out[r*4+c])*m[c*4+3];out[r*4+3]=float(t);}return true;}
SRWLOCK g_muzzleLock58=SRWLOCK_INIT;
struct ShotMuzzle111 {float muzzle[3]={};unsigned sequence=0;bool valid=false;} g_shotMuzzle111;
fc3aim::LaserSample194 g_laser194;
// 208: equipped weapon of the local pawn. Written on the input thread under g_muzzleLock58.
struct Equipped206 {UINT pawn=0,entity=0,strategy=0,wrapper=0,weapon=0,renderable=0;ULONGLONG ms=0;bool valid=false;} g_equipped206;
volatile LONG g_laserEnabled194=0;
bool g_laserQueryReady194=false;
// The C4 ray caller is also an inline hook site for physical placeables.
// Its original CALL may be replaced after the exact engine image is loaded.
bool LaserSignatures202(unsigned mask){return (mask&0x13)==0x13&&(mask&0x0c)!=0;}
void* g_shotVectorsOriginal58=nullptr;
void* g_projectileVectorsOriginal58=nullptr;
void* g_flameSpawnOriginal58=nullptr;
using NativeMuzzle58=void(__thiscall*)(void*,float*,float*,BYTE);
NativeMuzzle58 g_nativeMuzzle58=nullptr;
bool g_muzzleInstalled58=false;
bool g_muzzleAttempted58=false;
volatile LONG g_muzzleCorrected58=0;
UINT MuzzleRead58(UINT address,UINT offset=0){UINT value=0;if(!address||offset>0xffffffffu-address)return 0;__try{memcpy(&value,reinterpret_cast<void*>(address+offset),4);}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}return value;}
bool MuzzleCopy58(UINT address,void* out,UINT size){if(!address||size>0xffffffffu-address)return false;__try{memcpy(out,reinterpret_cast<void*>(address),size);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
UINT NativeMuzzleComponent58(UINT entity,UINT getter)
{
    UINT component=0;void* descriptor=reinterpret_cast<void*>(g_engine+getter);void* lookup=reinterpret_cast<void*>(g_engine+0x1706640);
    __try {__asm {call descriptor
        add eax,18h
        push eax
        mov eax,entity
        call lookup
        mov component,eax
    }}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
    return component;
}
bool NativeMuzzleRenderable58(UINT entity){return NativeMuzzleComponent58(entity,0x64250)!=0;}
bool NativeMuzzleEquipped58(UINT entity,UINT strategy){const UINT component=NativeMuzzleComponent58(entity,0x2749d0);return component&&MuzzleRead58(component,0x20)==strategy;}
using MuzzleRenderable58=bool(*)(UINT);
MuzzleRenderable58 g_muzzleRenderable58=NativeMuzzleRenderable58;
using MuzzleEquipped58=bool(*)(UINT,UINT);
MuzzleEquipped58 g_muzzleEquipped58=NativeMuzzleEquipped58;
UINT NativeMuzzleActor58(UINT wrapper){return NativeMuzzleComponent58(MuzzleRead58(MuzzleRead58(wrapper,0x28),0xc),0x2535c0);}
using MuzzleActor58=UINT(*)(UINT);
MuzzleActor58 g_muzzleActor58=NativeMuzzleActor58;
volatile LONG g_rejectedByReason206[10]={};
bool RejectMuzzleLocal58(const MuzzlePublication58& p,UINT wrapper,int reason,const char* label)
{
    InterlockedIncrement(&g_shotRejected206);
    static volatile LONG rejected[10]={};if(reason>=0&&reason<10){InterlockedIncrement(&g_rejectedByReason206[reason]);if(InterlockedIncrement(&rejected[reason])<=12)
        fc3::Log("[MUZZLE58] local shot unchanged: %s profile %u epoch %u active %d publicationAge %llu wrapper %08X expected %08X",label,p.profile,p.epoch,p.active?1:0,GetTickCount64()-p.ms,wrapper,p.wrapper);}
    // After the per-reason caps: a running summary every 20 rejections, so a late-session native-origin
    // shot (the "shot left from the top of the gun" report) is never hidden by the startup lines.
    const LONG total=InterlockedCompareExchange(&g_shotRejected206,0,0);if(total%20==0)fc3::Log("[MUZZLE206] local shots left native so far: %ld (owner %ld, noPublication %ld, stale %ld, wrapper %ld, weapon %ld, strategy %ld, noBone %ld, noModel %ld, attachment %ld, farRoot %ld); corrected %ld",total,g_rejectedByReason206[0],g_rejectedByReason206[1],g_rejectedByReason206[2],g_rejectedByReason206[3],g_rejectedByReason206[4],g_rejectedByReason206[5],g_rejectedByReason206[6],g_rejectedByReason206[7],g_rejectedByReason206[8],g_rejectedByReason206[9],InterlockedCompareExchange(&g_shotCorrected206,0,0));
    return false;
}
// refreshed: the HUD widget still names the previous wrapper. The caller then requires the
// native muzzle within 1 m of the published FRAME root instead of 3 m.
bool LocalMuzzlePublication58(UINT strategy,UINT actor,MuzzlePublication58& p,bool& refreshed)
{
    refreshed=false;
    AcquireSRWLockShared(&g_muzzleLock58);p=g_muzzle58;ReleaseSRWLockShared(&g_muzzleLock58);
    const UINT wrapper=MuzzleRead58(strategy,0x48),weaponReference=MuzzleRead58(wrapper,4),entity=MuzzleRead58(weaponReference,0xc);
    const UINT reference=MuzzleRead58(actor,4),pawnEntity=MuzzleRead58(reference,0xc),ownerReference=MuzzleRead58(wrapper,0x28);
    UINT local[2]={},owner[2]={};
    if(!NativeLocal(local)||!MuzzleCopy58(reference,owner,8)||!SameHandle(local,owner))return false;
    if(!wrapper||!entity||!pawnEntity||pawnEntity!=MuzzleRead58(ownerReference,0xc))return RejectMuzzleLocal58(p,wrapper,0,"owner reference mismatch");
    if(!p.active)return RejectMuzzleLocal58(p,wrapper,1,"no displayed FRAME publication");
    if(GetTickCount64()-p.ms>100)return RejectMuzzleLocal58(p,wrapper,2,"stale displayed FRAME publication");
    // The publication's wrapper comes from the HUD ammo widget, which redraws only when the ammo
    // changes: PID63244's first M133 shot, 2.7 s after the switch, still saw the D50 wrapper and
    // left from the native muzzle. A fresh core-draw publication fired by the equipped strategy is
    // this weapon's drawn FRAME.
    if(wrapper!=p.wrapper)
    {
        if(!g_muzzleEquipped58(entity,strategy))return RejectMuzzleLocal58(p,wrapper,3,"equipped wrapper mismatch");
        refreshed=true;static volatile LONG refreshedLogs=0;
        if(InterlockedIncrement(&refreshedLogs)<=12)fc3::Log("[MUZZLE107] firing wrapper %08X uses the fresh core-draw publication (HUD widget still named %08X); muzzle-to-root limit 1 m",wrapper,p.wrapper);
        return true;
    }
    if(!p.weapon||MuzzleRead58(wrapper,0x48)!=p.weapon)return RejectMuzzleLocal58(p,wrapper,4,"equipped weapon changed");
    if(!g_muzzleEquipped58(entity,strategy))return RejectMuzzleLocal58(p,wrapper,5,"weapon component strategy mismatch");
    return true;
}
// Same exact-engine scene-ray primitive used by native C4 placement and camera
// obstruction. Unlike bullet consumer 1450910 it does not enqueue a shot.
// The three-word result vector and 0x30-byte hit stride are verified at the
// C4 and camera callers. Both callers dispose the vector through 1799A30.
struct RayHits194 {UINT data=0,count=0,capacity=0;};
struct RayFilter194 {UINT type=0,mask=0,reserved=0;};
struct RayDispose194 {UINT type=0,data=0,dataAgain=0,zero=0,count=0,stride=0;};
__declspec(naked) void NativeSceneRay194(UINT target,UINT world,RayHits194* hits,const RayFilter194* filter,const float* origin,const float* delta)
{
 __asm {
  push ebp
  mov ebp,esp
  push esi
  push edi
  mov eax,[ebp+24]
  mov esi,[ebp+28]
  push 1
  push 0
  push dword ptr [ebp+20]
  push dword ptr [ebp+16]
  push dword ptr [ebp+12]
  call dword ptr [ebp+8]
  pop edi
  pop esi
  pop ebp
  ret
 }
}
__declspec(naked) void NativeSceneDispose194(UINT target,RayDispose194* descriptor)
{
 __asm {
  push ebp
  mov ebp,esp
  mov eax,[ebp+12]
  call dword ptr [ebp+8]
  pop ebp
  ret
 }
}
bool SceneHit194(const float* origin,const float* delta,float* point)
{
 if(!g_laserQueryReady194||!origin||!delta||!point)return false;
 const UINT manager=MuzzleRead58(g_engine,0x282a3d4),world=MuzzleRead58(manager,0x90);
 if(!world)return false;
 RayHits194 hits={};const RayFilter194 filter={g_engine+0x1f703a4,0x25bf,0};
 __try {NativeSceneRay194(g_engine+0x14505a0,world,&hits,&filter,origin,delta);}
 __except(EXCEPTION_EXECUTE_HANDLER){hits={};return false;}
 bool found=false;double nearest=1e100;const double length=std::sqrt(double(delta[0])*delta[0]+double(delta[1])*delta[1]+double(delta[2])*delta[2]);
 if(hits.data&&hits.count<=256&&hits.count<=hits.capacity&&length>.1){
  for(UINT i=0;i<hits.count;++i){float hit[3]={};if(!MuzzleCopy58(hits.data+i*0x30,hit,sizeof(hit)))break;
   const double d[3]={hit[0]-origin[0],hit[1]-origin[1],hit[2]-origin[2]};
   const double along=(d[0]*delta[0]+d[1]*delta[1]+d[2]*delta[2])/length;
   const double cross=std::sqrt((std::max)(0.,d[0]*d[0]+d[1]*d[1]+d[2]*d[2]-along*along));
   if(std::isfinite(along)&&std::isfinite(cross)&&along>.05&&along<=length+.1&&cross<.25&&along<nearest){nearest=along;memcpy(point,hit,sizeof(hit));found=true;}
  }
 }
 if(hits.data){RayDispose194 dispose={g_engine+0x1f32858,reinterpret_cast<UINT>(&hits.data),reinterpret_cast<UINT>(&hits.data),0,reinterpret_cast<UINT>(&hits.count),0x30};
  __try{NativeSceneDispose194(g_engine+0x1799a30,&dispose);}__except(EXCEPTION_EXECUTE_HANDLER){}
 }
 return found;
}
// 208: exact engine path of the ammo HUD's own weapon lookup (F83C50): [pawn+0xBE4+0xB4] -> reference -> +0xC = weapon
// entity; entity -> CWeapon component (2749D0/1706640) -> +0x20 strategy -> +0x48 wrapper. Accepted only when the
// wrapper points back at that entity and its owner reference carries this player's handle.
void SelectorNative290(const UINT* handle,UINT pawn);
void SampleEquipped206(const UINT* handle)
{
 Equipped206 e;e.ms=GetTickCount64();
 if(g_pawnFromHandle&&handle){
  UINT pawn=0,h0=handle[0],h1=handle[1];void* fn=g_pawnFromHandle;
  __try{__asm{push h1
   push h0
   call fn
   add esp,8
   mov pawn,eax}}__except(EXCEPTION_EXECUTE_HANDLER){pawn=0;}
  // 209: F83C50 dereferences [pawn+0x24] first (mov eax,[ebx+24h]; add eax,0BE4h; mov eax,[eax+0B4h]; mov edi,[eax+0Ch]).
  // 208 skipped that hop and read the pawn's own self-pointers, so every lookup was refused (live log).
  e.pawn=pawn;const UINT owner=MuzzleRead58(pawn,0x24),holder=owner?MuzzleRead58(owner,0xc98):0,entity=MuzzleRead58(holder,0xc);
  if(entity){const UINT component=NativeMuzzleComponent58(entity,0x2749d0),strategy=MuzzleRead58(component,0x20),wrapper=MuzzleRead58(strategy,0x48);
   const UINT weaponEntity=MuzzleRead58(MuzzleRead58(wrapper,4),0xc),ownerReference=MuzzleRead58(wrapper,0x28);UINT ownerHandle[2]={~0u,~0u};
   if(strategy&&wrapper&&weaponEntity==entity&&ownerReference&&MuzzleCopy58(ownerReference,ownerHandle,8)&&SameHandle(handle,ownerHandle)){e.entity=entity;e.strategy=strategy;e.wrapper=wrapper;e.weapon=MuzzleRead58(wrapper,0x48);e.renderable=NativeMuzzleComponent58(entity,0x64250);e.valid=true;}
   else{static volatile LONG refused=0;if(InterlockedIncrement(&refused)<=8)fc3::Log("[EQUIP208] refused: pawn %08X holder %08X entity %08X strategy %08X wrapper %08X weaponEntity %08X owner %08X:%08X local %08X:%08X",pawn,holder,entity,strategy,wrapper,weaponEntity,ownerHandle[0],ownerHandle[1],handle[0],handle[1]);}
  }
 }
 static UINT lastWrapper=0;static volatile LONG changes=0;
 if(e.valid&&e.wrapper!=lastWrapper){lastWrapper=e.wrapper;if(InterlockedIncrement(&changes)<=40)fc3::Log("[EQUIP208] equipped weapon wrapper %08X weapon %08X strategy %08X entity %08X (pawn %08X); HUD widget wrapper %08X",e.wrapper,e.weapon,e.strategy,e.entity,e.pawn,UINT(g_muzzle58.wrapper));}
 AcquireSRWLockExclusive(&g_muzzleLock58);g_equipped206=e;ReleaseSRWLockExclusive(&g_muzzleLock58);
 if(e.pawn)SelectorNative290(handle,e.pawn);
}
// Candidate 246: hook the engine's bone world-matrix getter (RVA 16F09C0: EAX=renderable component, ESI=out 4x4,
// one stack arg = bone index, ret 4, AL=1 on success; out = palette[bone] * component world). Every consumer of the
// native first-person flamethrower's bones (flame seed 1249260, flame update 1249460, muzzle provider 1304F30 and,
// the point of this candidate, whatever positions the jet's particle emitter) receives the bone already moved to
// the DRAWN weapon while a fresh profile-15 FRAME publication exists. Live 245 showed why per-frame vertex moves
// cannot work: old particles were born from where the head-locked nozzle WAS, so the far end of the jet always
// trails the head. The mod's own reads bypass the rewrite (g_boneBypass246). Callers are censused by RVA.
void* g_boneOriginal246=nullptr;bool g_boneInstalled246=false;__declspec(thread) int g_boneBypass246=0;
struct BoneCaller246 {UINT rva=0;LONG count=0;UINT bone=0;bool weapon=false;} g_boneCallers246[48];volatile LONG g_boneCallerCount246=0,g_boneRewrites246=0;
bool BoneRewriteActive246(MuzzlePublication58& p){AcquireSRWLockShared(&g_muzzleLock58);p=g_muzzle58;ReleaseSRWLockShared(&g_muzzleLock58);return p.active&&p.profile==15&&GetTickCount64()-p.ms<=100;}
void __cdecl BoneRewrite246(UINT component,float* out,UINT bone,UINT caller)
{
    MuzzlePublication58 p;if(!BoneRewriteActive246(p)||!out)return;
    Equipped206 eq;AcquireSRWLockShared(&g_muzzleLock58);eq=g_equipped206;ReleaseSRWLockShared(&g_muzzleLock58);
    const bool weapon=eq.valid&&eq.renderable&&component==eq.renderable&&GetTickCount64()-eq.ms<=250;
    // census of who asks for this weapon's bones (the FX emitter's caller is the one to inspect)
    const UINT rva=caller>=g_engine&&caller-g_engine<0x3000000u?caller-g_engine:caller;
    {const LONG n=InterlockedCompareExchange(&g_boneCallerCount246,0,0);bool seen=false;
     for(LONG i=0;i<n&&i<48;++i)if(g_boneCallers246[i].rva==rva&&g_boneCallers246[i].weapon==weapon){InterlockedIncrement(&g_boneCallers246[i].count);seen=true;break;}
     if(!seen&&n<48){const LONG slot=InterlockedIncrement(&g_boneCallerCount246)-1;if(slot<48){g_boneCallers246[slot]={rva,1,bone,weapon};fc3::Log("[BONE246] caller RVA %08X asks for %s component %08X bone %u (bypass %d)",rva,weapon?"the LOCAL FLAMETHROWER's":"another",component,bone,g_boneBypass246);}}}
    if(!weapon||g_boneBypass246>0)return;
    float native[16]={};if(!MuzzleCopy58(reinterpret_cast<UINT>(out),native,64))return;for(int i=0;i<16;++i)if(!std::isfinite(native[i]))return;
    if(std::fabs(native[3])>1e-5||std::fabs(native[7])>1e-5||std::fabs(native[11])>1e-5||std::fabs(native[15]-1)>1e-5)return;
    float moved[16]={};
    for(int c=0;c<4;++c){for(int r=0;r<3;++r){double v=c==3?double(p.move[r*4+3]):0.;for(int k=0;k<3;++k)v+=double(p.move[r*4+k])*native[c*4+k];moved[c*4+r]=float(v);}moved[c*4+3]=native[c*4+3];}
    __try{memcpy(out,moved,64);}__except(EXCEPTION_EXECUTE_HANDLER){return;}
    const LONG n=InterlockedIncrement(&g_boneRewrites246);if(n<=12||n%2000==0)fc3::Log("[BONE246] rewrite %ld: caller %08X bone %u translation %.3f/%.3f/%.3f -> %.3f/%.3f/%.3f (publication age %llu ms)",n,rva,bone,native[12],native[13],native[14],moved[12],moved[13],moved[14],GetTickCount64()-p.ms);
}
__declspec(naked) void BoneBoundary246()
{
    __asm {
        push ebp
        mov ebp,esp
        push esi
        push eax
        push dword ptr [ebp+8]
        call dword ptr [g_boneOriginal246]
        pushfd
        pushad
        mov eax,esp
        sub esp,544
        and esp,0fffffff0h
        mov [esp+512],eax
        fxsave [esp]
        mov ecx,[eax+8]
        push dword ptr [ecx+4]
        push dword ptr [ecx+8]
        push dword ptr [ecx-4]
        push dword ptr [ecx-8]
        call BoneRewrite246
        add esp,16
        fxrstor [esp]
        mov esp,[esp+512]
        popad
        popfd
        mov esp,ebp
        pop ebp
        ret 4
    }
}
void PollProjectileFlight242();
void TickLaser194()
{
 PollProjectileFlight242();
 // Runs whenever a weapon FRAME is published: the regular crosshair uses the hit distance even with the laser off.
 if(!g_laserQueryReady194||!g_nativeMuzzle58)return;
 MuzzlePublication58 p;Equipped206 eq;AcquireSRWLockShared(&g_muzzleLock58);p=g_muzzle58;eq=g_equipped206;ReleaseSRWLockShared(&g_muzzleLock58);
 // 208: the pawn's equipped weapon, never the HUD widget's stale one (a holstered gun's muzzle on the back or hip
 // passed the 1 m gate and drew the beam "from the ground" on the 1887/SVD; a never-fired gun had no sample at all).
 UINT wrapper=p.wrapper,weapon=p.weapon,entity=0,strategy=0;
 if(eq.valid&&GetTickCount64()-eq.ms<=250){wrapper=eq.wrapper;weapon=eq.weapon;entity=eq.entity;strategy=eq.strategy;}
 else{entity=MuzzleRead58(MuzzleRead58(wrapper,4),0xc);strategy=MuzzleRead58(NativeMuzzleComponent58(entity,0x2749d0),0x20);}
 if(!p.active||!p.aimValid||GetTickCount64()-p.ms>100||!weapon||MuzzleRead58(wrapper,0x48)!=weapon||!entity||!g_muzzleRenderable58(entity))return;
 if(!strategy||!g_muzzleEquipped58(entity,strategy)||static_cast<int>(MuzzleRead58(strategy,0x7c))<0&&static_cast<int>(MuzzleRead58(strategy,0x88))<0)return;
 if(static_cast<int>(MuzzleRead58(strategy,0x88))>=0){const UINT attachment=MuzzleRead58(MuzzleRead58(strategy,0x8c),0xc);if(attachment&&!g_muzzleRenderable58(attachment))return;}
 float native[3]={NAN,NAN,NAN},axis[3]={NAN,NAN,NAN},origin[3]={},direction[3]={};
 ++g_boneBypass246;__try{g_nativeMuzzle58(reinterpret_cast<void*>(strategy),native,axis,0);}__except(EXCEPTION_EXECUTE_HANDLER){--g_boneBypass246;return;}--g_boneBypass246;
 double offset=0;for(int i=0;i<3;++i){const double d=native[i]-p.root[i];offset+=d*d;}if(!std::isfinite(offset)||offset>1.)return;
 if(!fc3aim::TransformMuzzle58(p.move,native,axis,origin,direction,p.mirrored340))return;
 double aimLength=0;for(float a:p.aim)aimLength+=double(a)*a;if(!std::isfinite(aimLength)||aimLength<.25)return;
 const float unit[3]={float(p.aim[0]/std::sqrt(aimLength)),float(p.aim[1]/std::sqrt(aimLength)),float(p.aim[2]/std::sqrt(aimLength))};
 double forwardLength=0,alignment=0;for(int i=0;i<3;++i){forwardLength+=double(p.cameraForward[i])*p.cameraForward[i];alignment+=double(p.cameraForward[i])*unit[i];}
 if(!std::isfinite(forwardLength)||forwardLength<.25||alignment/std::sqrt(forwardLength)<std::cos(75*kPi/180))return;
 float delta[3]={unit[0]*50,unit[1]*50,unit[2]*50},point[3]={};const bool hit=SceneHit194(origin,delta,point);
 if(!hit)for(int i=0;i<3;++i)point[i]=origin[i]+unit[i]*25;
 fc3aim::LaserSample194 sample={};memcpy(sample.origin,origin,sizeof(origin));memcpy(sample.point,point,sizeof(point));memcpy(sample.nativeMuzzle,native,sizeof(native));sample.profile=p.profile;sample.epoch=p.epoch;sample.ms=GetTickCount64();sample.hit=hit;
 {double d2=0;for(int i=0;i<3;++i){const double d=double(point[i])-origin[i];d2+=d*d;}sample.distance=float(std::sqrt(d2));}
 // Gun-local muzzle: the render thread re-anchors it to the weapon drawn THIS frame (no stale world position).
 float inverseNative[12]={};if(p.nativeValid&&InvertFrame206(p.native,inverseNative)){float local[3]={};for(int r=0;r<3;++r){double v=inverseNative[r*4+3];for(int c=0;c<3;++c)v+=double(inverseNative[r*4+c])*native[c];local[r]=float(v);}
  double len=0;for(float v:local)len+=double(v)*v;if(std::isfinite(len)&&len<2.25){memcpy(sample.muzzleLocal,local,sizeof(local));sample.localValid=true;}}
 AcquireSRWLockExclusive(&g_muzzleLock58);g_laser194=sample;ReleaseSRWLockExclusive(&g_muzzleLock58);
 static LONG logs194=0;if(InterlockedIncrement(&logs194)<=20)fc3::Log("[LASER194] profile%u epoch%u %s muzzle %.2f/%.2f/%.2f endpoint %.2f/%.2f/%.2f age%llu ms",
  p.profile,p.epoch,hit?"scene hit":"clear ray",origin[0],origin[1],origin[2],point[0],point[1],point[2],sample.ms-p.ms);
}
void AuditDualShot362(UINT,UINT);
void __cdecl CorrectShotVectors58(UINT strategy,UINT actor,float* origin,float* direction)
{
    if(!g_nativeMuzzle58||!origin||!direction)return;
    MuzzlePublication58 p;bool refreshed=false;if(!LocalMuzzlePublication58(strategy,actor,p,refreshed))return;const UINT wrapper=MuzzleRead58(strategy,0x48),entity=MuzzleRead58(MuzzleRead58(wrapper,4),0xc);
    // Native provider falls back to pawn position when no muzzle node exists.
    // Such a fallback is not a barrel tip and must not be disguised as one.
    if(static_cast<int>(MuzzleRead58(strategy,0x88))<0&&static_cast<int>(MuzzleRead58(strategy,0x7c))<0){RejectMuzzleLocal58(p,wrapper,6,"native muzzle bone unavailable");return;}
    if(!g_muzzleRenderable58(entity)){RejectMuzzleLocal58(p,wrapper,7,"native weapon model unavailable");return;}
    if(static_cast<int>(MuzzleRead58(strategy,0x88))>=0){const UINT attachment=MuzzleRead58(MuzzleRead58(strategy,0x8c),0xc);if(attachment&&!g_muzzleRenderable58(attachment)){RejectMuzzleLocal58(p,wrapper,8,"attachment model unavailable");return;}}
    float muzzle[3]={NAN,NAN,NAN},axis[3]={NAN,NAN,NAN},shot[3]={};
    ++g_boneBypass246;__try {g_nativeMuzzle58(reinterpret_cast<void*>(strategy),muzzle,axis,0);memcpy(shot,direction,sizeof(shot));}
    __except(EXCEPTION_EXECUTE_HANDLER){--g_boneBypass246;return;}--g_boneBypass246;
    double distance=0;for(int i=0;i<3;++i){if(!std::isfinite(muzzle[i])||!std::isfinite(axis[i]))return;const double d=muzzle[i]-p.root[i];distance+=d*d;}
    if(distance>(refreshed?1.:9.)){InterlockedIncrement(&g_shotRejected206);InterlockedIncrement(&g_rejectedByReason206[9]);static volatile LONG rejected=0;if(InterlockedIncrement(&rejected)<=12)fc3::Log("[MUZZLE58] rejected root association profile %u wrapper %08X muzzle %.3f %.3f %.3f root %.3f %.3f %.3f distance %.3f",p.profile,wrapper,muzzle[0],muzzle[1],muzzle[2],p.root[0],p.root[1],p.root[2],std::sqrt(distance));return;}
    float movedOrigin[3]={},movedDirection[3]={};
    if(!fc3aim::TransformMuzzle58(p.move,muzzle,shot,movedOrigin,movedDirection,p.mirrored340))return;
    AcquireSRWLockExclusive(&g_muzzleLock58);memcpy(g_shotMuzzle111.muzzle,muzzle,sizeof(muzzle));++g_shotMuzzle111.sequence;g_shotMuzzle111.valid=true;ReleaseSRWLockExclusive(&g_muzzleLock58);
    // Direction: the native shot turned onto the published aim/reticle ray. The gun-move transport
    // stays only as the fallback (no fresh aim, or a shot not near the camera forward).
    float aimed[3]={};const bool onAim=p.aimValid&&fc3aim::AimShotDirection107(p.cameraForward,p.aim,shot,aimed);
    double transportOff=0,spreadDeg=0;
    if(onAim)
    {
        double dot=0,la=0,lb=0;for(int i=0;i<3;++i){dot+=double(aimed[i])*movedDirection[i];la+=double(aimed[i])*aimed[i];lb+=double(movedDirection[i])*movedDirection[i];}
        transportOff=la>0&&lb>0?std::acos((std::max)(-1.,(std::min)(1.,dot/std::sqrt(la*lb))))*180/kPi:0;
        // Candidate 206: the native hip spread is scaled around the barrel ray (0 = exactly where the laser/sight
        // points). Shotgun pellets keep the game's full pattern; the user slider never collapses them to a slug.
        const int percent=ShotgunProfile206(p.profile)?100:(std::max)(0,(std::min)(100,int(InterlockedCompareExchange(&g_spreadPercent206,0,0))));
        double lenShot=0,lenAim=0,cosSpread=0;for(int i=0;i<3;++i){lenShot+=double(aimed[i])*aimed[i];lenAim+=double(p.aim[i])*p.aim[i];cosSpread+=double(aimed[i])*p.aim[i];}
        if(lenShot>0&&lenAim>0){cosSpread/=std::sqrt(lenShot*lenAim);spreadDeg=std::acos((std::max)(-1.,(std::min)(1.,cosSpread)))*180/kPi;}
        if(percent<100&&lenShot>0&&lenAim>0){const double t=percent/100.,scale=std::sqrt(lenShot/lenAim);float mixed[3]={};double lenMixed=0;
            for(int i=0;i<3;++i){mixed[i]=float(p.aim[i]*scale*(1-t)+aimed[i]*t);lenMixed+=double(mixed[i])*mixed[i];}
            if(lenMixed>1e-12){const double fix=std::sqrt(lenShot/lenMixed);for(int i=0;i<3;++i)aimed[i]=float(mixed[i]*fix);}}
        memcpy(movedDirection,aimed,sizeof(aimed));
    }
    {static volatile LONG aimLogs[65]={};if(InterlockedIncrement(&aimLogs[p.profile<64?p.profile:64])<=24)fc3::Log("[MUZZLE107] local shot profile %u direction %s; native spread %.2f deg kept %d%%; gun-move transport would differ by %.2f degrees",p.profile,onAim?"on the published barrel ray":(p.aimValid?"kept on the gun-move transport (shot not near the camera forward)":"kept on the gun-move transport (no fresh aim)"),spreadDeg,ShotgunProfile206(p.profile)?100:int(InterlockedCompareExchange(&g_spreadPercent206,0,0)),transportOff);}
    if(!onAim){static volatile LONG offAim=0;if(InterlockedIncrement(&offAim)%10==1)fc3::Log("[MUZZLE206] shot NOT on the barrel ray (%ld so far): aimValid %d; camera forward %.3f/%.3f/%.3f aim %.3f/%.3f/%.3f native shot %.3f/%.3f/%.3f",offAim,p.aimValid?1:0,p.cameraForward[0],p.cameraForward[1],p.cameraForward[2],p.aim[0],p.aim[1],p.aim[2],shot[0],shot[1],shot[2]);}
    __try{memcpy(origin,movedOrigin,sizeof(movedOrigin));memcpy(direction,movedDirection,sizeof(movedDirection));}
    __except(EXCEPTION_EXECUTE_HANDLER){return;}
    const LONG count=InterlockedIncrement(&g_muzzleCorrected58);InterlockedIncrement(&g_shotCorrected206);
    // Observes the accepted local native bullet boundary. Never a trigger edge,
    // laser query, dry fire, reload request or a new game/ammo callback.
    fc3xr::Gunshot339(entity,int(p.profile));
    AuditDualShot362(wrapper,entity);
    if(count<=40||count%50==0)fc3::Log("[MUZZLE58] local shot %ld profile %u epoch %u wrapper %08X native %.3f %.3f %.3f final %.3f %.3f %.3f bone %d attachmentBone %d attachmentRef %08X; native spread retained",count,p.profile,p.epoch,wrapper,muzzle[0],muzzle[1],muzzle[2],movedOrigin[0],movedOrigin[1],movedOrigin[2],static_cast<int>(MuzzleRead58(strategy,0x7c)),static_cast<int>(MuzzleRead58(strategy,0x88)),MuzzleRead58(strategy,0x8c));
}
// Candidate 242: where a launched projectile actually goes.
// The native builder (123AA40) aims every projectile from the muzzle bone toward a far point on the PAWN LOOK
// ray (eye + look * K) and hands the shot caller that ray's scene HIT POINT, which 123C29C feeds to the
// projectile for the strategies flagged at +595 (the ballistic ones). In VR the pawn look is the HEAD, so a flare
// or arrow whose Euler had merely been transported by the gun move still flew where the head looked (live 240/241).
// Now, exactly as bullets do in CorrectShotVectors58: the native launch forward (Euler +Y column, accepted only
// within 20 deg of the camera forward) is turned onto the published barrel ray, and the target point is re-cast
// along that ray from the moved origin with the engine's own scene ray. The RPG (profile 32) keeps the accepted
// gun-move transport. The native launch speed and collision byte are never rewritten.
void EulerForward242(const float* a,float* forward){const double cp=std::cos(a[0]),sp=std::sin(a[0]),cr=std::cos(a[1]),sr=std::sin(a[1]),cy=std::cos(a[2]),sy=std::sin(a[2]);forward[0]=float(cy*sr*sp-sy*cp);forward[1]=float(sy*sr*sp+cy*cp);forward[2]=float(cr*sp);}
double AngleDeg242(const float* a,const float* b){double la=0,lb=0,d=0;for(int i=0;i<3;++i){la+=double(a[i])*a[i];lb+=double(b[i])*b[i];d+=double(a[i])*b[i];}if(la<1e-12||lb<1e-12)return 0;return std::acos((std::max)(-1.,(std::min)(1.,d/std::sqrt(la*lb))))*180/kPi;}
// Row-major 3x4 rotation (zero translation) taking unit(from) onto unit(to) by the smallest angle.
bool MinimalRotation242(const float* from,const float* to,float* R)
{
    double a[3],b[3],la=0,lb=0;for(int i=0;i<3;++i){a[i]=from[i];b[i]=to[i];if(!std::isfinite(a[i])||!std::isfinite(b[i]))return false;la+=a[i]*a[i];lb+=b[i]*b[i];}
    if(la<1e-12||lb<1e-12)return false;la=std::sqrt(la);lb=std::sqrt(lb);for(int i=0;i<3;++i){a[i]/=la;b[i]/=lb;}
    const double c=a[0]*b[0]+a[1]*b[1]+a[2]*b[2];if(c<-.9)return false;
    const double v[3]={a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]},k=1/(1+c);
    const double M[9]={c+k*v[0]*v[0],-v[2]+k*v[0]*v[1],v[1]+k*v[0]*v[2],v[2]+k*v[1]*v[0],c+k*v[1]*v[1],-v[0]+k*v[1]*v[2],-v[1]+k*v[2]*v[0],v[0]+k*v[2]*v[1],c+k*v[2]*v[2]};
    for(int r=0;r<3;++r){for(int col=0;col<3;++col)R[r*4+col]=float(M[r*3+col]);R[r*4+3]=0;}return true;
}
// Flight readback: the launched entity (the caller's EDI holder, +0xC) is read at +0x50 for 1.5 s after the
// launch so the log shows the REAL flight direction against the barrel ray and the head ray.
struct ProjectileFlight242 {UINT entity=0,profile=~0u;float origin[3]={},aim[3]={},head[3]={};ULONGLONG ms=0,nextMs=0;int samples=0;bool active=false;} g_flight242;
volatile LONG g_flightShots242=0;
void NoteProjectileFlight242(UINT holder,UINT profile,const float* origin,const float* aim,const float* head)
{
    const UINT entity=MuzzleRead58(holder,0xc);if(!entity||InterlockedIncrement(&g_flightShots242)>12)return;
    ProjectileFlight242 f;f.entity=entity;f.profile=profile;memcpy(f.origin,origin,12);memcpy(f.aim,aim,12);memcpy(f.head,head,12);f.ms=GetTickCount64();f.nextMs=f.ms;f.active=true;
    AcquireSRWLockExclusive(&g_muzzleLock58);g_flight242=f;ReleaseSRWLockExclusive(&g_muzzleLock58);
}
void PollProjectileFlight242()
{
    ProjectileFlight242 f;AcquireSRWLockShared(&g_muzzleLock58);f=g_flight242;ReleaseSRWLockShared(&g_muzzleLock58);
    const ULONGLONG now=GetTickCount64();if(!f.active||now<f.nextMs)return;
    bool done=now-f.ms>1500||f.samples>=8;float pos[3]={};
    const bool readable=MuzzleCopy58(f.entity+0x50,pos,12)&&std::isfinite(pos[0])&&std::isfinite(pos[1])&&std::isfinite(pos[2]);
    if(readable){float d[3]={};double len=0;for(int i=0;i<3;++i){d[i]=pos[i]-f.origin[i];len+=double(d[i])*d[i];}len=std::sqrt(len);
        if(len>.3){++f.samples;fc3::Log("[PROJ242 FLIGHT] profile %u t %llu ms: travelled %.2f m to %.3f/%.3f/%.3f; direction %.1f deg from the barrel ray, %.1f deg from the head ray",f.profile,now-f.ms,len,pos[0],pos[1],pos[2],AngleDeg242(d,f.aim),AngleDeg242(d,f.head));}
        else if(now-f.ms>600){fc3::Log("[PROJ242 FLIGHT] profile %u t %llu ms: entity %08X still within %.2f m of the launch origin (not this entity, or not launched)",f.profile,now-f.ms,f.entity,len);done=true;}
    }else{fc3::Log("[PROJ242 FLIGHT] profile %u entity %08X position unreadable; no readback",f.profile,f.entity);done=true;}
    f.nextMs=now+60;if(done)f.active=false;
    AcquireSRWLockExclusive(&g_muzzleLock58);if(g_flight242.entity==f.entity&&g_flight242.ms==f.ms)g_flight242=f;ReleaseSRWLockExclusive(&g_muzzleLock58);
}
void __cdecl CorrectProjectileVectors58(UINT strategy,UINT actor,float* origin,float* angles,BYTE* hit,float* hitPoint,UINT holder)
{
    (void)hit; // the native collision byte is never rewritten
    if(!origin||!angles)return;MuzzlePublication58 p;bool refreshed=false;if(!LocalMuzzlePublication58(strategy,actor,p,refreshed))return;
    const UINT entity=MuzzleRead58(MuzzleRead58(MuzzleRead58(strategy,0x48),4),0xc);
    if(static_cast<int>(MuzzleRead58(strategy,0x548))<0||!g_muzzleRenderable58(entity))return;
    float nativeOrigin[3]={},nativeAngles[3]={},finalOrigin[3]={},finalAngles[3]={},nativeHit[3]={};
    if(!MuzzleCopy58(reinterpret_cast<UINT>(origin),nativeOrigin,12)||!MuzzleCopy58(reinterpret_cast<UINT>(angles),nativeAngles,12))return;
    const bool hitReadable=hitPoint&&MuzzleCopy58(reinterpret_cast<UINT>(hitPoint),nativeHit,12)&&std::isfinite(nativeHit[0])&&std::isfinite(nativeHit[1])&&std::isfinite(nativeHit[2]);
    double distance=0;for(int i=0;i<3;++i){const double d=nativeOrigin[i]-p.root[i];distance+=d*d;}if(!std::isfinite(distance)||distance>(refreshed?1.:9.))return;
    if(!fc3aim::TransformProjectileMuzzle58(p.move,nativeOrigin,nativeAngles,finalOrigin,finalAngles))return;
    float nativeForward[3]={},unitAim[3]={},aimed[3]={};EulerForward242(nativeAngles,nativeForward);
    bool onAim=false;const char* why="barrel ray";double aimLength=0;for(float a:p.aim)aimLength+=double(a)*a;float crosshair[3]={};bool crosshairValid=false;
    if(p.profile==32)why="RPG keeps its accepted gun-move transport";
    else if(!p.aimValid||!std::isfinite(aimLength)||aimLength<.25)why="no fresh barrel ray published";
    else if(!fc3aim::AimShotDirection107(p.cameraForward,p.aim,nativeForward,aimed))why="native launch not within 20 deg of the camera forward, or barrel ray beyond 75 deg of it";
    else{float rotation[12]={},unused[3]={},turned[3]={},aimVector[3]={p.aim[0],p.aim[1],p.aim[2]};const float zero[3]={};
        // 244: the crosshair is the laser sample's hit point, measured from the laser's own muzzle. On the bow that
        // muzzle sits 0.39 m below the arrow's launch point (live 243), so a launch PARALLEL to the barrel ray landed
        // 0.34 m above the crosshair. The launch now converges on the crosshair point itself when the sample is fresh.
        fc3aim::LaserSample194 laser={};
        if(fc3aim::ReadLaser194(laser)&&laser.profile==p.profile&&laser.epoch==p.epoch&&laser.distance>1&&GetTickCount64()-laser.ms<=150)
        {float toPoint[3]={};double len=0;for(int i=0;i<3;++i){toPoint[i]=laser.point[i]-finalOrigin[i];len+=double(toPoint[i])*toPoint[i];}
         if(len>1&&AngleDeg242(toPoint,p.aim)<15){memcpy(aimVector,toPoint,sizeof(aimVector));memcpy(crosshair,laser.point,sizeof(crosshair));crosshairValid=laser.hit;why="crosshair point";}}
        double vectorLength=0;for(float a:aimVector)vectorLength+=double(a)*a;
        if(MinimalRotation242(nativeForward,aimVector,rotation)&&fc3aim::TransformProjectileMuzzle58(rotation,zero,nativeAngles,unused,turned)){memcpy(finalAngles,turned,sizeof(turned));onAim=true;for(int i=0;i<3;++i)unitAim[i]=float(aimVector[i]/std::sqrt(vectorLength));}
        else why="minimal rotation unavailable";}
    __try{memcpy(origin,finalOrigin,12);memcpy(angles,finalAngles,12);}__except(EXCEPTION_EXECUTE_HANDLER){return;}
    // 243: the launch counts as a local shot for the muzzle-flash/smoke pool window (muzzle_flash111.inl).
    AcquireSRWLockExclusive(&g_muzzleLock58);memcpy(g_shotMuzzle111.muzzle,nativeOrigin,sizeof(nativeOrigin));++g_shotMuzzle111.sequence;g_shotMuzzle111.valid=true;ReleaseSRWLockExclusive(&g_muzzleLock58);
    double nativeTarget=0;if(hitReadable){for(int i=0;i<3;++i){const double d=double(nativeHit[i])-nativeOrigin[i];nativeTarget+=d*d;}nativeTarget=std::sqrt(nativeTarget);}
    bool targetWritten=false,sceneHit=false;float point[3]={};double targetDistance=0;
    if(onAim&&hitPoint)
    {
        const double reach=nativeTarget>=.5&&nativeTarget<=5000?nativeTarget:100.,rayLength=(std::min)(500.,(std::max)(50.,reach));
        const float delta[3]={float(unitAim[0]*rayLength),float(unitAim[1]*rayLength),float(unitAim[2]*rayLength)};
        if(crosshairValid){memcpy(point,crosshair,sizeof(point));sceneHit=true;}
        else{sceneHit=SceneHit194(finalOrigin,delta,point);
        if(!sceneHit)for(int i=0;i<3;++i)point[i]=float(finalOrigin[i]+unitAim[i]*reach);}
        __try{memcpy(hitPoint,point,12);targetWritten=true;}__except(EXCEPTION_EXECUTE_HANDLER){}
        if(targetWritten){double d2=0;for(int i=0;i<3;++i){const double d=double(point[i])-finalOrigin[i];d2+=d*d;}targetDistance=std::sqrt(d2);}
    }
    float finalForward[3]={};EulerForward242(finalAngles,finalForward);
    if(holder)NoteProjectileFlight242(holder,p.profile,finalOrigin,finalForward,p.cameraForward);
    // 243: where the crosshair/laser sat at this moment, measured from the launch origin (the "above the crosshair" question).
    {fc3aim::LaserSample194 laser={};const bool have=fc3aim::ReadLaser194(laser);float toLaser[3]={};double laserDeg=-1,laserDistance=0;
     if(have){for(int i=0;i<3;++i)toLaser[i]=laser.point[i]-finalOrigin[i];laserDeg=AngleDeg242(toLaser,finalForward);laserDistance=laser.distance;}
     static volatile LONG laserLogs=0;if(InterlockedIncrement(&laserLogs)<=40)fc3::Log("[PROJ243] profile %u crosshair check: laser sample %s (age %llu ms, %s at %.1f m, origin %.3f/%.3f/%.3f, point %.3f/%.3f/%.3f); launch origin %.3f/%.3f/%.3f; crosshair point is %.2f deg off the launch forward (vertical offset %.2f m, launch-origin-to-laser-origin %.3f m)",
      p.profile,have?"present":"MISSING",have?GetTickCount64()-laser.ms:0ull,laser.hit?"scene hit":"clear ray",laserDistance,laser.origin[0],laser.origin[1],laser.origin[2],laser.point[0],laser.point[1],laser.point[2],finalOrigin[0],finalOrigin[1],finalOrigin[2],laserDeg,
      have?double(laser.point[2])-(finalOrigin[2]+finalForward[2]/std::sqrt((std::max)(1e-12,double(finalForward[0])*finalForward[0]+double(finalForward[1])*finalForward[1]+double(finalForward[2])*finalForward[2]))*laserDistance):0.,
      have?std::sqrt(double(laser.origin[0]-finalOrigin[0])*(laser.origin[0]-finalOrigin[0])+double(laser.origin[1]-finalOrigin[1])*(laser.origin[1]-finalOrigin[1])+double(laser.origin[2]-finalOrigin[2])*(laser.origin[2]-finalOrigin[2])):0.);}
    static volatile LONG changed=0;if(InterlockedIncrement(&changed)<=40)fc3::Log("[PROJ242] local projectile profile %u epoch %u: origin native %.3f %.3f %.3f -> %.3f %.3f %.3f; launch %s (native launch %.1f deg from the camera forward, final %.1f deg; %s); target %s at %.1f m (native target %.1f m); native speed and collision byte untouched",
        p.profile,p.epoch,nativeOrigin[0],nativeOrigin[1],nativeOrigin[2],finalOrigin[0],finalOrigin[1],finalOrigin[2],onAim?"turned onto the published barrel ray":"left on the gun-move transport",AngleDeg242(nativeForward,p.cameraForward),AngleDeg242(finalForward,p.cameraForward),why,targetWritten?(sceneHit?"scene hit on the barrel ray":"clear barrel ray"):"left native",targetDistance,nativeTarget);
}
struct FlameBirth245 {float origin[3]={},axis[3]={};ULONGLONG ms=0;bool valid=false;} g_flameBirth245;
void __cdecl CorrectFlameSpawn58(UINT strategy,UINT record)
{
    const UINT wrapper=MuzzleRead58(strategy,0x48);
    MuzzlePublication58 p;bool refreshed=false;if(!record)return;
    // Flame's seed uses a different actor interface (descriptor+14). Do not
    // query it through the bullet adapter (+18). The firing strategy itself
    // carries the exact local owner and equipped-weapon references.
    AcquireSRWLockShared(&g_muzzleLock58);p=g_muzzle58;ReleaseSRWLockShared(&g_muzzleLock58);
    if(p.profile==15){static volatile LONG entry241=0;if(InterlockedIncrement(&entry241)<=6)fc3::Log("[FLAME241] native birth entry strategy %08X wrapper %08X published %08X active%d frame%d age%llu ms record%08X",strategy,wrapper,p.wrapper,p.active,p.nativeValid,GetTickCount64()-p.ms,record);}
    if(p.profile==15&&g_boneInstalled246){static volatile LONG once=0;if(InterlockedIncrement(&once)<=3)fc3::Log("[BONE246] flame birth record already built from the drawn nozzle; no second transport");return;}
    if(p.profile==15){UINT local[2]={},owner[2]={};const UINT ref=MuzzleRead58(wrapper,0x28),entity=MuzzleRead58(MuzzleRead58(wrapper,4),0xc);
        if(!NativeLocal(local)||!MuzzleCopy58(ref,owner,8)||!SameHandle(local,owner)||!p.active||!p.nativeValid||GetTickCount64()-p.ms>100||!entity||!g_muzzleEquipped58(entity,strategy))return;
        if(wrapper!=p.wrapper){refreshed=true;}else if(!p.weapon||MuzzleRead58(wrapper,0x48)!=p.weapon)return;
    }else{const UINT actor=g_muzzleActor58(wrapper);if(!actor||!LocalMuzzlePublication58(strategy,actor,p,refreshed))return;}
    const UINT entity=MuzzleRead58(MuzzleRead58(wrapper,4),0xc);
    if(static_cast<int>(MuzzleRead58(strategy,0x7c))<0||!g_muzzleRenderable58(entity))return;
    const UINT count=MuzzleRead58(strategy,0xac),particles=MuzzleRead58(record,4);
    if(!particles||!count||count>2048||particles>0xffffffffu-count*0x24)return;
    float nativeOrigin[3]={},nativeAxis[3]={},nativeAngles[3]={},nativeMatrix[16]={},finalOrigin[3]={},finalAxis[3]={},finalAngles[3]={};
    if(!MuzzleCopy58(record+0x90,nativeOrigin,12)||!MuzzleCopy58(record+0x4c,nativeAxis,12)||!MuzzleCopy58(record+0xa0,nativeAngles,12)||!MuzzleCopy58(record+0x60,nativeMatrix,64))return;
    double distance=0;for(int i=0;i<3;++i){const double d=nativeOrigin[i]-p.root[i];distance+=d*d;}if(!std::isfinite(distance)||distance>(refreshed?1.:9.))return;
    {FlameBirth245 b;memcpy(b.origin,nativeOrigin,12);memcpy(b.axis,nativeAxis,12);b.ms=GetTickCount64();b.valid=true;AcquireSRWLockExclusive(&g_muzzleLock58);g_flameBirth245=b;ReleaseSRWLockExclusive(&g_muzzleLock58);}
    if(!fc3aim::TransformMuzzle58(p.move,nativeOrigin,nativeAxis,finalOrigin,finalAxis)||!fc3aim::TransformProjectileMuzzle58(p.move,nativeOrigin,nativeAngles,finalOrigin,finalAngles))return;
    float finalMatrix[16]={};for(int i=0;i<16;++i)if(!std::isfinite(nativeMatrix[i]))return;
    if(std::fabs(nativeMatrix[3])>1e-5||std::fabs(nativeMatrix[7])>1e-5||std::fabs(nativeMatrix[11])>1e-5||std::fabs(nativeMatrix[15]-1)>1e-5)return;
    if(p.profile==15){
        float target[12]={},inverse[12]={},source[12]={},transport[12]={};
        for(int r=0;r<3;++r){for(int c=0;c<3;++c){source[r*4+c]=nativeMatrix[c*4+r];for(int k=0;k<3;++k)target[r*4+c]+=p.move[r*4+k]*p.native[k*4+c];}source[r*4+3]=nativeOrigin[r];target[r*4+3]=finalOrigin[r];}
        if(!InvertFrame206(source,inverse))return;
        for(int r=0;r<3;++r){for(int c=0;c<3;++c)for(int k=0;k<3;++k)transport[r*4+c]+=target[r*4+k]*inverse[k*4+c];transport[r*4+3]=target[r*4+3];for(int k=0;k<3;++k)transport[r*4+3]+=target[r*4+k]*inverse[k*4+3];}
        memcpy(p.move,transport,sizeof(transport));
        if(!fc3aim::TransformMuzzle58(p.move,nativeOrigin,nativeAxis,finalOrigin,finalAxis)||!fc3aim::TransformProjectileMuzzle58(p.move,nativeOrigin,nativeAngles,finalOrigin,finalAngles))return;
    }
    for(int c=0;c<4;++c){for(int r=0;r<3;++r){double v=double(p.move[r*4+3])*nativeMatrix[c*4+3];for(int k=0;k<3;++k)v+=double(p.move[r*4+k])*nativeMatrix[c*4+k];finalMatrix[c*4+r]=float(v);}finalMatrix[c*4+3]=nativeMatrix[c*4+3];}
    // Validate every fresh particle before writing any. The original generator
    // owns spray randomness/speed; rotate its completed world-space result.
    for(UINT i=0;i<count;++i){float v[9]={},a[3]={},d[3]={},b[3]={};if(!MuzzleCopy58(particles+i*0x24,v,36)||!fc3aim::TransformMuzzle58(p.move,v,v+6,a,d)||!fc3aim::TransformMuzzle58(p.move,v+3,v+6,b,d))return;}
    __try {
        for(UINT i=0;i<count;++i){float* v=reinterpret_cast<float*>(particles+i*0x24);float a[3]={},b[3]={},d[3]={};fc3aim::TransformMuzzle58(p.move,v,v+6,a,d);fc3aim::TransformMuzzle58(p.move,v+3,v+6,b,d);memcpy(v,a,12);memcpy(v+3,b,12);memcpy(v+6,d,12);}
        memcpy(reinterpret_cast<void*>(record+0x60),finalMatrix,64);memcpy(reinterpret_cast<void*>(record+0x4c),finalAxis,12);memcpy(reinterpret_cast<void*>(record+0xa0),finalAngles,12);
    }__except(EXCEPTION_EXECUTE_HANDLER){return;}
    static volatile LONG changed=0;if(InterlockedIncrement(&changed)<=12)fc3::Log("[FLAME241] local flame birth profile %u epoch %u particles %u native %.3f %.3f %.3f final %.3f %.3f %.3f; native spray rotated to displayed barrel",p.profile,p.epoch,count,nativeOrigin[0],nativeOrigin[1],nativeOrigin[2],finalOrigin[0],finalOrigin[1],finalOrigin[2]);
}
// The original custom ABI takes EDI=strategy, ESI=actor and three stack args;
// it returns with ret12. Copy its arguments while - Hinny that GOAT - preserving the caller frame.
// Afterwards preserve all original register/EFLAGS/x87/SSE outputs around the
// C++ correction, which edits only the two proven output vectors.
__declspec(naked) void ShotVectorsBoundary58()
{
    __asm {
        push ebp
        mov ebp,esp
        push dword ptr [ebp+16]
        push dword ptr [ebp+12]
        push dword ptr [ebp+8]
        call dword ptr [g_shotVectorsOriginal58]
        pushfd
        pushad
        mov eax,esp
        sub esp,544
        and esp,0fffffff0h
        mov [esp+512],eax
        fxsave [esp]
        mov ecx,[eax+8]
        push dword ptr [ecx+12]
        push dword ptr [ecx+8]
        push dword ptr [eax+4]
        push dword ptr [eax]
        call CorrectShotVectors58
        add esp,16
        fxrstor [esp]
        mov esp,[esp+512]
        popad
        popfd
        mov esp,ebp
        pop ebp
        ret 12
    }
}
// Local projectile builder: stdcall(strategy, actor, origin, Euler rotation,
// hit byte, hit point), ret24. 242 also passes the caller's EDI (the projectile holder
// 123BF30 keeps across the call; +0xC is the entity being launched) for the flight readback.
__declspec(naked) void ProjectileVectorsBoundary58()
{
    __asm {
        push ebp
        mov ebp,esp
        push dword ptr [ebp+28]
        push dword ptr [ebp+24]
        push dword ptr [ebp+20]
        push dword ptr [ebp+16]
        push dword ptr [ebp+12]
        push dword ptr [ebp+8]
        call dword ptr [g_projectileVectorsOriginal58]
        pushfd
        pushad
        mov eax,esp
        sub esp,544
        and esp,0fffffff0h
        mov [esp+512],eax
        fxsave [esp]
        mov ecx,[eax+8]
        push dword ptr [eax]
        push dword ptr [ecx+28]
        push dword ptr [ecx+24]
        push dword ptr [ecx+20]
        push dword ptr [ecx+16]
        push dword ptr [ecx+12]
        push dword ptr [ecx+8]
        call CorrectProjectileVectors58
        add esp,28
        fxrstor [esp]
        mov esp,[esp+512]
        popad
        popfd
        mov esp,ebp
        pop ebp
        ret 24
    }
}
// Flame birth generator: EDI=record, one stack strategy argument, ret4.
// Preserve its full machine output while transporting the completed new spray.
__declspec(naked) void FlameSpawnBoundary58()
{
    __asm {
        push ebp
        mov ebp,esp
        push dword ptr [ebp+8]
        call dword ptr [g_flameSpawnOriginal58]
        pushfd
        pushad
        mov eax,esp
        sub esp,544
        and esp,0fffffff0h
        mov [esp+512],eax
        fxsave [esp]
        mov ecx,[eax+8]
        push dword ptr [eax]
        push dword ptr [ecx+8]
        call CorrectFlameSpawn58
        add esp,8
        fxrstor [esp]
        mov esp,[esp+512]
        popad
        popfd
        mov esp,ebp
        pop ebp
        ret 4
    }
}
bool InstallMuzzle58()
{
    if(g_muzzleInstalled58)return true;if(!g_engine||g_muzzleAttempted58)return false;g_muzzleAttempted58=true;
    BYTE* base=reinterpret_cast<BYTE*>(g_engine);
    const BYTE dispatcher[]={0x55,0x8b,0xec,0xa1};
    const BYTE provider[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x83,0xec,0x48,0x56,0x57,0x8b,0xf9};
    const BYTE caller[]={0xe8,0xbf,0xfc,0xff,0xff}; //130631C ->1305FE0
    const BYTE projectile[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x81,0xec,0xc4,0x00,0x00,0x00};
    const BYTE projectileCaller[]={0xe8,0x8b,0xe9,0xff,0xff}; //123C0B0 ->123AA40
    const BYTE flame[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x58,0x53,0x8b,0x5d,0x08};
    if(memcmp(base+0x1305fe0,dispatcher,sizeof(dispatcher))||
       MuzzleRead58(g_engine,0x1305fe4)!=g_engine+0x2829bc4||
       memcmp(base+0x1304f30,provider,sizeof(provider))||
       memcmp(base+0x130631c,caller,sizeof(caller))||
       MuzzleRead58(g_engine,0x1fbb090)!=g_engine+0x1304f30||
       memcmp(base+0x123aa40,projectile,sizeof(projectile))||memcmp(base+0x123c0b0,projectileCaller,sizeof(projectileCaller)))return false;
    if(MH_CreateHook(base+0x1305fe0,reinterpret_cast<void*>(ShotVectorsBoundary58),&g_shotVectorsOriginal58)!=MH_OK)return false;
    g_nativeMuzzle58=reinterpret_cast<NativeMuzzle58>(base+0x1304f30);
    const BYTE ray194[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x8b,0x0d};
    const BYTE dispose194[]={0x56,0x8b,0xf0};
    const BYTE c4RayCall194[]={0xe8,0xe9,0xb7,0x21,0x00};
    const BYTE cameraRayCall194[]={0xe8,0x5b,0x30,0x14,0x00};
    const BYTE c4DisposeCall194[]={0xe8,0x75,0x47,0x56,0x00};
    const unsigned laserSignatures202=
        (!memcmp(base+0x14505a0,ray194,sizeof(ray194))?0x01u:0u)|
        (!memcmp(base+0x1799a30,dispose194,sizeof(dispose194))?0x02u:0u)|
        (!memcmp(base+0x1234db2,c4RayCall194,sizeof(c4RayCall194))?0x04u:0u)|
        (!memcmp(base+0x130d540,cameraRayCall194,sizeof(cameraRayCall194))?0x08u:0u)|
        (!memcmp(base+0x12352b6,c4DisposeCall194,sizeof(c4DisposeCall194))?0x10u:0u);
    g_laserQueryReady194=LaserSignatures202(laserSignatures202);
    fc3::Log("[LASER202] scene-ray signatures %02X/1F, native query %s; requires ray and cleanup functions, cleanup caller, and either native ray caller",
        laserSignatures202,g_laserQueryReady194?"ready":"disabled");
    if(MH_EnableHook(base+0x1305fe0)!=MH_OK){g_nativeMuzzle58=nullptr;return false;}
    if(MH_CreateHook(base+0x123aa40,reinterpret_cast<void*>(ProjectileVectorsBoundary58),&g_projectileVectorsOriginal58)==MH_OK&&MH_EnableHook(base+0x123aa40)==MH_OK)
        fc3::Log("[MUZZLE58] verified local projectile origin/rotation boundary installed; native projectile physics remain native");
    if(!memcmp(base+0x12455b0,flame,sizeof(flame))&&MuzzleRead58(g_engine,0x1fd4b8c)==g_engine+0x381470&&MH_CreateHook(base+0x12455b0,reinterpret_cast<void*>(FlameSpawnBoundary58),&g_flameSpawnOriginal58)==MH_OK&&MH_EnableHook(base+0x12455b0)==MH_OK)
        fc3::Log("[MUZZLE58] verified local flame birth boundary installed; completed native spray transported");
    {const BYTE bone246[]={0x53,0x8b,0xdc,0x83,0xec,0x08,0x83,0xe4,0xf0,0x83,0xc4,0x04,0x55,0x8b,0x6b,0x04};
     if(!memcmp(base+0x16f09c0,bone246,sizeof(bone246))&&MH_CreateHook(base+0x16f09c0,reinterpret_cast<void*>(BoneBoundary246),&g_boneOriginal246)==MH_OK&&MH_EnableHook(base+0x16f09c0)==MH_OK){g_boneInstalled246=true;fc3::Log("[BONE246] verified bone world-matrix getter hooked; local flamethrower bones answer at the drawn pose while its FRAME is published");}
     else fc3::Log("[BONE246] bone world-matrix getter NOT hooked (signature or MinHook failure)");}
    g_muzzleInstalled58=true;fc3::Log("[MUZZLE58] verified local bullet vector boundary installed; original raycast/spread remain native");return true;
}
}
namespace fc3aim {
bool TransformProjectileMuzzle58(const float* move,const float* origin,const float* angles,float* finalOrigin,float* finalAngles)
{
    if(!angles||!finalAngles)return false;for(int i=0;i<3;++i)if(!std::isfinite(angles[i]))return false;
    // Exact native89F40 Euler matrix: Rz(yaw)*Ry(roll)*Rx(pitch).
    // The projectile267860 conversion is the zero-roll subset of this convention.
    const double cp=std::cos(angles[0]),sp=std::sin(angles[0]),cr=std::cos(angles[1]),sr=std::sin(angles[1]),cy=std::cos(angles[2]),sy=std::sin(angles[2]);
    const double basis[9]={cy*cr,cy*sr*sp-sy*cp,cy*sr*cp+sy*sp,sy*cr,sy*sr*sp+cy*cp,sy*sr*cp-cy*sp,-sr,cr*sp,cr*cp};
    float forward[3]={float(basis[1]),float(basis[4]),float(basis[7])},turned[3]={};
    if(!TransformMuzzle58(move,origin,forward,finalOrigin,turned))return false;
    double finalBasis[9]={};for(int r=0;r<3;++r)for(int c=0;c<3;++c)for(int k=0;k<3;++k)finalBasis[r*3+c]+=double(move[r*4+k])*basis[k*3+c];
    const double horizontal=std::sqrt(finalBasis[0]*finalBasis[0]+finalBasis[3]*finalBasis[3]);
    double pitch=0,yaw=0;if(horizontal>1e-6){pitch=std::atan2(finalBasis[7],finalBasis[8]);yaw=std::atan2(finalBasis[3],finalBasis[0]);}else{pitch=0;yaw=std::atan2(-finalBasis[1],finalBasis[4]);}
    const double roll=std::atan2(-finalBasis[6],horizontal);
    finalAngles[0]=float(angles[0]+Wrap(pitch-angles[0]));finalAngles[1]=float(angles[1]+Wrap(roll-angles[1]));finalAngles[2]=float(angles[2]+Wrap(yaw-angles[2]));return true;
}
bool TransformMuzzle58(const float* move,const float* origin,const float* direction,float* finalOrigin,float* finalDirection,bool reflected340)
{
    if(!move||!origin||!direction||!finalOrigin||!finalDirection)return false;
    for(int i=0;i<12;++i)if(!std::isfinite(move[i]))return false;
    for(int r=0;r<3;++r)for(int s=0;s<3;++s){double dot=0;for(int c=0;c<3;++c)dot+=double(move[r*4+c])*move[s*4+c];if(std::fabs(dot-(r==s?1.:0.))>.025)return false;}
    const double determinant=move[0]*(move[5]*move[10]-move[6]*move[9])-move[1]*(move[4]*move[10]-move[6]*move[8])+move[2]*(move[4]*move[9]-move[5]*move[8]);
    // A confirmed full weapon mirror is orthogonal with determinant-1.
    // Points/directions support that transform; projectile Euler rotations
    // keep the original proper-rotation default and never opt in.
    if(std::fabs(determinant-1)>.025&&(!reflected340||std::fabs(determinant+1)>.025))return false;
    double length=0;for(int i=0;i<3;++i){if(!std::isfinite(origin[i])||!std::isfinite(direction[i]))return false;length+=double(direction[i])*direction[i];}
    if(length<1e-12||length>1e12)return false;
    float a[3]={},b[3]={};double movedLength=0;
    for(int r=0;r<3;++r){double point=move[r*4+3],axis=0;for(int c=0;c<3;++c){point+=double(move[r*4+c])*origin[c];axis+=double(move[r*4+c])*direction[c];}a[r]=float(point);b[r]=float(axis);movedLength+=axis*axis;}
    if(!std::isfinite(movedLength)||movedLength<1e-12)return false;
    const double preserveLength=std::sqrt(length/movedLength);for(int i=0;i<3;++i){if(!std::isfinite(a[i]))return false;finalOrigin[i]=a[i];finalDirection[i]=float(b[i]*preserveLength);}return true;
}
void ClearMuzzle58(){AcquireSRWLockExclusive(&g_muzzleLock58);g_muzzle58.active=false;g_muzzle58.aimValid=false;ReleaseSRWLockExclusive(&g_muzzleLock58);}
bool AimShotDirection107(const float* cameraForward,const float* aim,const float* shot,float* out)
{
    if(!cameraForward||!aim||!shot||!out)return false;
    double a[3],b[3],s[3],la=0,lb=0,ls=0;
    for(int i=0;i<3;++i){a[i]=cameraForward[i];b[i]=aim[i];s[i]=shot[i];if(!std::isfinite(a[i])||!std::isfinite(b[i])||!std::isfinite(s[i]))return false;la+=a[i]*a[i];lb+=b[i]*b[i];ls+=s[i]*s[i];}
    if(la<1e-12||lb<1e-12||ls<1e-12||ls>1e12)return false;
    la=std::sqrt(la);lb=std::sqrt(lb);ls=std::sqrt(ls);for(int i=0;i<3;++i){a[i]/=la;b[i]/=lb;}
    // The native shot is the camera forward plus the game's spread, never far from it.
    if((a[0]*s[0]+a[1]*s[1]+a[2]*s[2])/ls<std::cos(20*kPi/180))return false;
    const double c=a[0]*b[0]+a[1]*b[1]+a[2]*b[2];if(c<std::cos(75*kPi/180))return false;
    // Rodrigues from a to b: R x = x cos + (v x x) + v (v.x)/(1+cos), v = a x b.
    const double v[3]={a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    const double vs=v[0]*s[0]+v[1]*s[1]+v[2]*s[2],k=1/(1+c);
    const double vxs[3]={v[1]*s[2]-v[2]*s[1],v[2]*s[0]-v[0]*s[2],v[0]*s[1]-v[1]*s[0]};
    float result[3]={};for(int i=0;i<3;++i){const double r=s[i]*c+vxs[i]+v[i]*vs*k;if(!std::isfinite(r))return false;result[i]=float(r);}
    memcpy(out,result,sizeof(result));return true;
}
bool LastShotMuzzle111(float* muzzle,unsigned& sequence)
{
    if(!muzzle)return false;AcquireSRWLockShared(&g_muzzleLock58);const ShotMuzzle111 s=g_shotMuzzle111;ReleaseSRWLockShared(&g_muzzleLock58);
    if(!s.valid)return false;memcpy(muzzle,s.muzzle,sizeof(s.muzzle));sequence=s.sequence;return true;
}
void SetLaserEnabled194(bool enabled)
{
 InterlockedExchange(&g_laserEnabled194,enabled?1:0);
 AcquireSRWLockExclusive(&g_muzzleLock58);g_laser194={};ReleaseSRWLockExclusive(&g_muzzleLock58);
}
bool EquippedWeapon206(unsigned& wrapper,unsigned& weapon,unsigned& strategy)
{
 Equipped206 e;AcquireSRWLockShared(&g_muzzleLock58);e=g_equipped206;ReleaseSRWLockShared(&g_muzzleLock58);
 if(!e.valid||GetTickCount64()-e.ms>250)return false;wrapper=e.wrapper;weapon=e.weapon;strategy=e.strategy;return true;
}
bool BoneRewriteInstalled246(){return g_boneInstalled246;}
bool LastFlameBirth245(float* origin,float* axis,unsigned long long& ageMs){FlameBirth245 b;AcquireSRWLockShared(&g_muzzleLock58);b=g_flameBirth245;ReleaseSRWLockShared(&g_muzzleLock58);if(!b.valid)return false;memcpy(origin,b.origin,12);memcpy(axis,b.axis,12);ageMs=GetTickCount64()-b.ms;return true;}
bool ProjectileFlightActive242(){ProjectileFlight242 f;AcquireSRWLockShared(&g_muzzleLock58);f=g_flight242;ReleaseSRWLockShared(&g_muzzleLock58);return f.active&&GetTickCount64()-f.ms<=1500;}
bool ReadLaser194(LaserSample194& sample)
{
 // The sample is produced whether or not the laser is drawn; renderers gate on the laser mode themselves.
 AcquireSRWLockShared(&g_muzzleLock58);sample=g_laser194;ReleaseSRWLockShared(&g_muzzleLock58);
 return sample.ms&&GetTickCount64()-sample.ms<=100;
}
void PublishMuzzleFrame206(const float* nativeFrame)
{
 if(!nativeFrame)return;for(int i=0;i<12;++i)if(!std::isfinite(nativeFrame[i]))return;
 AcquireSRWLockExclusive(&g_muzzleLock58);
 if(g_muzzle58.active){memcpy(g_muzzle58.native,nativeFrame,sizeof(g_muzzle58.native));g_muzzle58.nativeValid=true;}
 ReleaseSRWLockExclusive(&g_muzzleLock58);
}
void SetSpreadPercent206(int percent){InterlockedExchange(&g_spreadPercent206,(std::max)(0,(std::min)(100,percent)));}
int SpreadPercent206(){return int(InterlockedCompareExchange(&g_spreadPercent206,0,0));}
void ShotCounters206(unsigned& corrected,unsigned& rejected){corrected=unsigned(InterlockedCompareExchange(&g_shotCorrected206,0,0));rejected=unsigned(InterlockedCompareExchange(&g_shotRejected206,0,0));}
void PublishMuzzleAim107(const float* aim,const float* cameraForward)
{
    if(!aim||!cameraForward)return;for(int i=0;i<3;++i)if(!std::isfinite(aim[i])||!std::isfinite(cameraForward[i]))return;
    AcquireSRWLockExclusive(&g_muzzleLock58);
    if(g_muzzle58.active){memcpy(g_muzzle58.aim,aim,sizeof(g_muzzle58.aim));memcpy(g_muzzle58.cameraForward,cameraForward,sizeof(g_muzzle58.cameraForward));g_muzzle58.aimValid=true;}
    ReleaseSRWLockExclusive(&g_muzzleLock58);
}
void PublishMuzzle58(const float* move,const float* root,unsigned wrapper,unsigned weapon,unsigned profile,unsigned epoch,bool active,bool mirrored340)
{
    float checkOrigin[3]={},checkDirection[3]={};const float axis[3]={0,1,0};
    mirrored340=mirrored340&&fc3mirror338::Supported(int(profile));
    if(!active||!wrapper||!weapon||!epoch||!TransformMuzzle58(move,root,axis,checkOrigin,checkDirection,mirrored340)){ClearMuzzle58();return;}
    MuzzlePublication58 p;memcpy(p.move,move,sizeof(p.move));memcpy(p.root,root,sizeof(p.root));p.wrapper=wrapper;p.weapon=weapon;p.profile=profile;p.epoch=epoch;p.ms=GetTickCount64();p.active=true;
    p.mirrored340=mirrored340;
    AcquireSRWLockExclusive(&g_muzzleLock58);g_muzzle58=p;ReleaseSRWLockExclusive(&g_muzzleLock58);
}
}
