#include <cstdio>
#include <cstdlib>
#include "../src/aim_probe.cpp"
namespace fc3 {void Log(const char*,...){} }
unsigned shotReceipts339=0,shotEntity339=0;int shotProfile339=-1;
namespace fc3xr {void SetTestNote(const wchar_t*){} void Gunshot339(unsigned entity,int profile){++shotReceipts339;shotEntity339=entity;shotProfile339=profile;} }
extern "C" MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*){return MH_ERROR_UNSUPPORTED_FUNCTION;}
extern "C" MH_STATUS WINAPI MH_EnableHook(LPVOID){return MH_ERROR_NOT_CREATED;}
static int failures58=0;
void Check58(bool value,const char* name){if(!value){std::printf("FAIL: %s\n",name);++failures58;}}
UINT localId58[2]={0x12345678,0x87654321};
UINT* __cdecl Local58(UINT* out){memcpy(out,localId58,8);return out;}
float tip58[3]={1,2,3};
UINT rayReceived194[6]={};
float rayHitRecords194[24]={2,3,4,0,0,1};
__declspec(naked) void NativeRayFixture194(){__asm{
 mov dword ptr [rayReceived194],eax
 mov dword ptr [rayReceived194+4],esi
 mov eax,[esp+4]
 mov dword ptr [rayReceived194+8],eax
 mov eax,[esp+12]
 mov dword ptr [rayReceived194+12],eax
 mov eax,[esp+16]
 mov dword ptr [rayReceived194+16],eax
 mov eax,[esp+20]
 mov dword ptr [rayReceived194+20],eax
 mov eax,[esp+8]
 mov dword ptr [eax],offset rayHitRecords194
 mov dword ptr [eax+4],1
 mov dword ptr [eax+8],1
 ret 20
}}
UINT rayDisposeReceived194=0;
__declspec(naked) void NativeDisposeFixture194(){__asm{
 mov dword ptr [rayDisposeReceived194],eax
 ret
}}
bool Renderable58(UINT){return true;}
bool equipped58=true;
bool Equipped58(UINT,UINT){return equipped58;}
UINT flameActor58=0;
UINT Actor58(UINT){return flameActor58;}
void __fastcall NativeTip58(void*,void*,float* origin,float* direction,BYTE){memcpy(origin,tip58,12);direction[0]=0;direction[1]=1;direction[2]=0;}
UINT received58[5]={};
__declspec(naked) void OriginalVectors58(){__asm{
    mov dword ptr [received58],edi
    mov dword ptr [received58+4],esi
    mov eax,[esp+4]
    mov dword ptr [received58+8],eax
    mov ecx,[esp+8]
    mov dword ptr [received58+12],ecx
    mov edx,[esp+12]
    mov dword ptr [received58+16],edx
    mov dword ptr [eax],0
    mov dword ptr [eax+4],0
    mov dword ptr [eax+8],0
    mov dword ptr [ecx],0
    mov dword ptr [ecx+4],040000000h
    mov dword ptr [ecx+8],0
    mov eax,013579bdfh
    mov edx,02468ace0h
    movd xmm0,eax
    pshufd xmm0,xmm0,0
    stc
    ret 12
}}
UINT projectileReceived58[6]={};
__declspec(naked) void OriginalProjectile58(){__asm{
    mov eax,[esp+4]
    mov dword ptr [projectileReceived58],eax
    mov eax,[esp+8]
    mov dword ptr [projectileReceived58+4],eax
    mov eax,[esp+12]
    mov dword ptr [projectileReceived58+8],eax
    mov dword ptr [eax],03f800000h
    mov dword ptr [eax+4],040000000h
    mov dword ptr [eax+8],040400000h
    mov eax,[esp+16]
    mov dword ptr [projectileReceived58+12],eax
    mov dword ptr [eax],0
    mov dword ptr [eax+4],0
    mov dword ptr [eax+8],0
    mov eax,[esp+20]
    mov dword ptr [projectileReceived58+16],eax
    mov byte ptr [eax],1
    mov eax,[esp+24]
    mov dword ptr [projectileReceived58+20],eax
    mov dword ptr [eax],042c60000h
    mov dword ptr [eax+4],042c40000h
    mov dword ptr [eax+8],042c20000h
    mov eax,013579bdfh
    mov edx,02468ace0h
    movd xmm0,eax
    pshufd xmm0,xmm0,0
    stc
    ret 24
}}
float flameParticles58[18]={};UINT flameReceived58[2]={};
UINT componentEntity58=0,componentDescriptor58=0;
void __cdecl MakeFlame58(UINT strategy,UINT record){
    flameReceived58[0]=strategy;flameReceived58[1]=record;
    const float native[18]={1,2,3,1,3,3,0,1,0,1.1f,2.2f,3.3f,1.7f,3,3.3f,.6f,.8f,0};memcpy(flameParticles58,native,sizeof(native));
    const float nativeMatrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,1,2,3,1},axis[3]={0,1,0},angles[3]={};
    *reinterpret_cast<UINT*>(record+4)=reinterpret_cast<UINT>(flameParticles58);*reinterpret_cast<UINT*>(strategy+0xac)=2;
    memcpy(reinterpret_cast<void*>(record+0x60),nativeMatrix,64);memcpy(reinterpret_cast<void*>(record+0x4c),axis,12);memcpy(reinterpret_cast<void*>(record+0xa0),angles,12);
}
__declspec(naked) void OriginalFlame58(){__asm{
    push edi
    push dword ptr [esp+8]
    call MakeFlame58
    add esp,8
    mov eax,013579bdfh
    mov edx,02468ace0h
    movd xmm0,eax
    pshufd xmm0,xmm0,0
    stc
    ret 4
}}
UINT applyCalls210=0,applySelf210=0;float applyAmount210=0;
__declspec(naked) void ApplyDamageFixture210(){__asm{
 inc dword ptr [applyCalls210]
 mov dword ptr [applySelf210],ecx
 mov eax,[esp+4]
 mov dword ptr [applyAmount210],eax
 mov eax,0x13579b01
 mov edx,0x2468ace0
 stc
 ret 4
}}
void Run210(UINT self,float amount,UINT& eax,UINT& edx,UINT& flags,UINT& esiOut,UINT& xmm1Out,UINT& before,UINT& after)
{
    UINT a=0,d=0,f=0,s=0,x=0,b0=0,b1=0;const UINT marker=0x0badf00d;
    __asm {
        push esi
        mov esi,0x5eed5eed
        movd xmm1,marker
        mov b0,esp
        push amount
        mov ecx,self
        call ApplyDamageBoundary210
        mov b1,esp
        mov a,eax
        mov d,edx
        pushfd
        pop f
        mov s,esi
        movd x,xmm1
        pop esi
    }
    eax=a;edx=d;flags=f;esiOut=s;xmm1Out=x;before=b0;after=b1;
}
int main(){
    Check58(LaserSignatures202(0x1f)&&LaserSignatures202(0x1b)&&LaserSignatures202(0x17),
        "laser ray ABI accepts both native callers or either caller after the other is hooked");
    Check58(!LaserSignatures202(0x13)&&!LaserSignatures202(0x1e)&&!LaserSignatures202(0x1d)&&!LaserSignatures202(0x0f),
        "laser ABI rejects missing callers, ray function, cleanup function, or cleanup caller");
    RayHits194 rayHits194={};const RayFilter194 rayFilter194={0x12345678,0x25bf,0};const float rayStart194[3]={1,2,3},rayDelta194[3]={0,0,50};
    UINT beforeRay194=0,afterRay194=0;__asm mov beforeRay194,esp
    NativeSceneRay194(reinterpret_cast<UINT>(NativeRayFixture194),0x33445566,&rayHits194,&rayFilter194,rayStart194,rayDelta194);
    __asm mov afterRay194,esp
    Check58(beforeRay194==afterRay194&&rayReceived194[0]==reinterpret_cast<UINT>(rayStart194)&&rayReceived194[1]==reinterpret_cast<UINT>(rayDelta194)&&
        rayReceived194[2]==0x33445566&&rayReceived194[3]==reinterpret_cast<UINT>(&rayFilter194)&&rayReceived194[4]==0&&rayReceived194[5]==1&&
        rayHits194.data==reinterpret_cast<UINT>(rayHitRecords194)&&rayHits194.count==1&&rayHits194.capacity==1,
        "194 scene-ray adapter passes EAX origin, ESI delta, five ret20 arguments and the engine hit vector");
    RayDispose194 rayDispose194={0x11223344,reinterpret_cast<UINT>(&rayHits194.data),reinterpret_cast<UINT>(&rayHits194.data),0,reinterpret_cast<UINT>(&rayHits194.count),0x30};
    NativeSceneDispose194(reinterpret_cast<UINT>(NativeDisposeFixture194),&rayDispose194);
    Check58(rayDisposeReceived194==reinterpret_cast<UINT>(&rayDispose194),"194 native hit-vector destructor receives EAX descriptor");
    const float turn[12]={0,-1,0,10,1,0,0,20,0,0,1,30};
    float a[3]={},b[3]={},direction[3]={0,2,0};
    Check58(fc3aim::TransformMuzzle58(turn,tip58,direction,a,b)&&a[0]==8&&a[1]==21&&a[2]==33&&b[0]==-2&&b[1]==0&&b[2]==0,"full world tip transport and spread vector length preserved");
    float bad[12];memcpy(bad,turn,sizeof(bad));bad[0]=2;
    Check58(!fc3aim::TransformMuzzle58(bad,tip58,direction,a,b),"non-rigid transform rejected");
    float zero[3]={};Check58(!fc3aim::TransformMuzzle58(turn,tip58,zero,a,b),"zero shot direction rejected");
    float angles58[3]={},turnedAngles58[3]={};Check58(fc3aim::TransformProjectileMuzzle58(turn,tip58,angles58,a,turnedAngles58)&&std::fabs(turnedAngles58[0])<1e-5&&std::fabs(turnedAngles58[2]-float(kPi/2))<1e-5,"native projectile Euler convention rotates forward correctly");
    const float tiltedAngles58[3]={.4f,.3f,-1.2f};Check58(fc3aim::TransformProjectileMuzzle58(turn,tip58,tiltedAngles58,a,turnedAngles58)&&std::fabs(turnedAngles58[0]-.4f)<1e-5&&std::fabs(turnedAngles58[1]-.3f)<1e-5&&std::fabs(turnedAngles58[2]-float(-1.2+kPi/2))<1e-5,"native full Euler composition retains existing pitch and roll under world yaw");
    const float rollTurn58[12]={0,0,1,0,0,1,0,0,-1,0,0,0};Check58(fc3aim::TransformProjectileMuzzle58(rollTurn58,tip58,angles58,a,turnedAngles58)&&std::fabs(turnedAngles58[1]-float(kPi/2))<1e-5&&std::fabs(turnedAngles58[0])<1e-5&&std::fabs(turnedAngles58[2])<1e-5,"native full Euler gimbal case retains equivalent rotation");
    BYTE* engine58=static_cast<BYTE*>(VirtualAlloc(nullptr,48332800,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check58(engine58!=nullptr,"fake exact engine-size reader fixture allocated");if(!engine58)return 1;
    UINT manager58[4]={0,0,0,1};g_engine=reinterpret_cast<UINT>(engine58);const UINT managerPointer58=reinterpret_cast<UINT>(manager58);memcpy(engine58+0x2829474,&managerPointer58,4);g_localHandle=Local58;g_muzzleRenderable58=Renderable58;g_muzzleEquipped58=Equipped58;
    UINT entity58[32]={},pawnEntity58[32]={},reference58[4]={localId58[0],localId58[1],0,reinterpret_cast<UINT>(pawnEntity58)},weaponReference58[4]={0xaaaabbbb,0xbbbbcccc,0,reinterpret_cast<UINT>(entity58)};
    UINT actor58[16]={},wrapper58[32]={},strategy58[400]={};actor58[1]=reinterpret_cast<UINT>(reference58);wrapper58[1]=reinterpret_cast<UINT>(weaponReference58);wrapper58[0x28/4]=reinterpret_cast<UINT>(reference58);wrapper58[0x48/4]=0x1234;strategy58[0x48/4]=reinterpret_cast<UINT>(wrapper58);strategy58[0x88/4]=3;strategy58[0x548/4]=5;
    // Exercise the actual production component-lookup adapter with tiny x86
    // native fixtures at the same RVAs in an isolated executable image.
    UINT component58[16]={},descriptor58[16]={};entity58[0x20/4]=reinterpret_cast<UINT>(component58);component58[0x20/4]=reinterpret_cast<UINT>(strategy58);
    BYTE getDescriptor58[]={0xb8,0,0,0,0,0xc3};const UINT descriptorPointer58=reinterpret_cast<UINT>(descriptor58);memcpy(getDescriptor58+1,&descriptorPointer58,4);
    BYTE lookup58[]={0xa3,0,0,0,0,0x8b,0x54,0x24,0x04,0x89,0x15,0,0,0,0,0x8b,0x40,0x20,0xc2,0x04,0x00};const UINT entityOutput58=reinterpret_cast<UINT>(&componentEntity58),descriptorOutput58=reinterpret_cast<UINT>(&componentDescriptor58);memcpy(lookup58+1,&entityOutput58,4);memcpy(lookup58+11,&descriptorOutput58,4);
    DWORD priorProtection58=0;for(UINT rva58:{0x64250u,0x2749d0u,0x2535c0u}){memcpy(engine58+rva58,getDescriptor58,sizeof(getDescriptor58));Check58(VirtualProtect(engine58+rva58,sizeof(getDescriptor58),PAGE_EXECUTE_READWRITE,&priorProtection58)!=FALSE,"native descriptor fixture executable");}
    memcpy(engine58+0x1706640,lookup58,sizeof(lookup58));Check58(VirtualProtect(engine58+0x1706640,sizeof(lookup58),PAGE_EXECUTE_READWRITE,&priorProtection58)!=FALSE,"native component fixture executable");FlushInstructionCache(GetCurrentProcess(),engine58,48332800);
    Check58(NativeMuzzleComponent58(reinterpret_cast<UINT>(entity58),0x64250)==reinterpret_cast<UINT>(component58)&&componentEntity58==reinterpret_cast<UINT>(entity58)&&componentDescriptor58==descriptorPointer58+0x18,"production custom component adapter passes EAX entity and descriptor+18, ret4 balanced");
    Check58(NativeMuzzleEquipped58(reinterpret_cast<UINT>(entity58),reinterpret_cast<UINT>(strategy58)),"production CWeapon component strategy check uses native adapter");
    g_nativeMuzzle58=reinterpret_cast<NativeMuzzle58>(NativeTip58);
    const float root58[3]={1,2,3};fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(wrapper58),0x1234,3,1,true);
    float origin58[3]={},shot58[3]={0,2,0};
    CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);
    Check58(origin58[0]==8&&origin58[1]==21&&origin58[2]==33&&shot58[0]==-2,"production callback substitutes native tip instead of camera origin");
    Check58(shotReceipts339==1&&shotEntity339==reinterpret_cast<UINT>(entity58)&&shotProfile339==3,"339 actual native shot emits exact local entity/profile receipt");
    tip58[1]=2.4f;shot58[0]=0;shot58[1]=2;shot58[2]=0;
    CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);
    Check58(std::fabs(origin58[0]-7.6f)<1e-5,"changed native suppressor tip transported without baked offset");tip58[1]=2;
    reference58[0]^=1;origin58[0]=777;CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);Check58(origin58[0]==777,"non-local actor untouched");reference58[0]^=1;
    wrapper58[0x48/4]=0x5678;CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);Check58(origin58[0]==777,"same wrapper newly equipped weapon untouched");wrapper58[0x48/4]=0x1234;
    equipped58=false;CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);Check58(origin58[0]==777,"local but unequipped strategy untouched");equipped58=true;
    g_muzzle58.ms=GetTickCount64()-101;CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);Check58(origin58[0]==777,"stale rendered association untouched");
    Check58(shotReceipts339==2,"339 nonlocal, unequipped, replaced and stale native shots emit no haptic receipts");
    {
     float mirror340[12]={-1,0,0,0,0,1,0,0,0,0,1,0},a340[3]={},b340[3]={};
     const float axis340[3]={0,1,0},angles340[3]={0,0,0};
     Check58(!fc3aim::TransformMuzzle58(mirror340,root58,axis340,a340,b340),"340 untagged reflected muzzle remains rejected");
     Check58(!fc3aim::TransformProjectileMuzzle58(mirror340,root58,angles340,a340,b340),"340 projectile Euler rotation keeps proper-matrix guard");
     for(int profile340:fc3mirror338::kProfiles){
      fc3aim::PublishMuzzle58(mirror340,root58,reinterpret_cast<UINT>(wrapper58),0x1234,profile340,1,true,true);
      Check58(g_muzzle58.active&&g_muzzle58.mirrored340,"340 confirmed mirrored bullet profile publishes active muzzle");
      const auto receipts340=shotReceipts339;float o340[3]={},d340[3]={0,2,0};
      CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),o340,d340);
      Check58(o340[0]==-1&&o340[1]==2&&o340[2]==3&&d340[1]==2&&shotReceipts339==receipts340+1,"340 actual mirrored native shot follows visible muzzle and retains gunfire receipt");
      const bool ready340=g_laserQueryReady194;g_laserQueryReady194=true;fc3aim::PublishMuzzleAim107(axis340,axis340);TickLaser194();g_laserQueryReady194=ready340;
      fc3aim::LaserSample194 sample340;Check58(fc3aim::ReadLaser194(sample340)&&sample340.profile==unsigned(profile340)&&sample340.origin[0]==-1&&sample340.origin[1]==2&&sample340.origin[2]==3,"340 actual mirrored native laser query supplies fresh crosshair/beam muzzle sample");
     }
     fc3aim::PublishMuzzle58(mirror340,root58,reinterpret_cast<UINT>(wrapper58),0x1234,31,1,true,true);
     Check58(!g_muzzle58.active,"340 unsupported weapon cannot opt into mirror transport");
     mirror340[0]=-2;Check58(!fc3aim::TransformMuzzle58(mirror340,root58,axis340,a340,b340,true),"340 mirror permission still rejects scaled/nonrigid matrix");
     puts("PASS:340 all15 mirrored profiles retain native bullet muzzle/direction and haptic receipts;untagged/unsupported/scaled/projectile guards preserved");
    }
    fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(wrapper58),0x1234,3,1,true);
    g_shotVectorsOriginal58=reinterpret_cast<void*>(OriginalVectors58);
    UINT before58=0,after58=0,eax58=0,edx58=0,flags58=0,xmm58[4]={};
    UINT strategyPointer58=reinterpret_cast<UINT>(strategy58),actorPointer58=reinterpret_cast<UINT>(actor58),context58=0xabcdef01;
    float* originPointer58=origin58;float* shotPointer58=shot58;
    __asm{
        push edi
        push esi
        mov edi,strategyPointer58
        mov esi,actorPointer58
        mov before58,esp
        push context58
        push shotPointer58
        push originPointer58
        call ShotVectorsBoundary58
        mov after58,esp
        mov eax58,eax
        mov edx58,edx
        pushfd
        pop flags58
        movdqu xmm58,xmm0
        pop esi
        pop edi
    }
    Check58(before58==after58&&received58[0]==strategyPointer58&&received58[1]==actorPointer58&&received58[2]==reinterpret_cast<UINT>(origin58)&&received58[3]==reinterpret_cast<UINT>(shot58)&&received58[4]==context58,"actual naked ABI bridge passes strategy/actor/three args and ret12 balances stack");
    Check58(eax58==0x13579bdf&&edx58==0x2468ace0&&(flags58&1)&&xmm58[0]==0x13579bdf&&xmm58[3]==0x13579bdf,"original EAX EDX carry and SSE outputs preserved across correction");
    Check58(origin58[0]==8&&origin58[1]==21&&shot58[0]==-2,"actual boundary runs native vectors then production muzzle callback");
    g_projectileVectorsOriginal58=reinterpret_cast<void*>(OriginalProjectile58);BYTE hit58=0;float hitPoint58[3]={};float* anglesPointer58=angles58;BYTE* hitPointer58=&hit58;float* hitPointPointer58=hitPoint58;
    UINT holder242[8]={},entity242[64]={};holder242[3]=reinterpret_cast<UINT>(entity242);const UINT holderPointer242=reinterpret_cast<UINT>(holder242);
    __asm{
        push edi
        mov edi,holderPointer242
        mov before58,esp
        push hitPointPointer58
        push hitPointer58
        push anglesPointer58
        push originPointer58
        push actorPointer58
        push strategyPointer58
        call ProjectileVectorsBoundary58
        mov after58,esp
        mov eax58,eax
        mov edx58,edx
        pushfd
        pop flags58
        movdqu xmm58,xmm0
        pop edi
    }
    Check58(g_flight242.active&&g_flight242.entity==reinterpret_cast<UINT>(entity242),"242 projectile bridge hands the caller's EDI holder (+0xC entity) to the flight readback");
    {const float launched242[3]={8,26,33};memcpy(entity242+0x50/4,launched242,12);g_flight242.nextMs=0;PollProjectileFlight242();Check58(g_flight242.samples==1&&g_flight242.active,"242 flight readback reads the launched entity position at +0x50");}
    Check58(before58==after58&&projectileReceived58[0]==strategyPointer58&&projectileReceived58[1]==actorPointer58&&projectileReceived58[2]==reinterpret_cast<UINT>(origin58)&&projectileReceived58[3]==reinterpret_cast<UINT>(angles58)&&projectileReceived58[4]==reinterpret_cast<UINT>(&hit58)&&projectileReceived58[5]==reinterpret_cast<UINT>(hitPoint58),"actual projectile bridge six args and ret24 balances stack");
    Check58(eax58==0x13579bdf&&edx58==0x2468ace0&&(flags58&1)&&xmm58[3]==0x13579bdf,"projectile bridge preserves original machine results");
    Check58(origin58[0]==8&&origin58[1]==21&&std::fabs(angles58[2]-float(kPi/2))<1e-5&&hit58==1&&hitPoint58[0]==99&&hitPoint58[1]==98&&hitPoint58[2]==97,"projectile position/rotation align while native collision outputs unchanged");
    UINT flameRecord58[64]={};UINT flameRecordPointer58=reinterpret_cast<UINT>(flameRecord58);flameActor58=actorPointer58;g_muzzleActor58=Actor58;g_flameSpawnOriginal58=reinterpret_cast<void*>(OriginalFlame58);strategy58[0x7c/4]=3;
    __asm{
        push edi
        mov edi,flameRecordPointer58
        mov before58,esp
        push strategyPointer58
        call FlameSpawnBoundary58
        mov after58,esp
        mov eax58,eax
        mov edx58,edx
        pushfd
        pop flags58
        movdqu xmm58,xmm0
        pop edi
    }
    Check58(before58==after58&&flameReceived58[0]==strategyPointer58&&flameReceived58[1]==flameRecordPointer58,"actual flame bridge EDI record/strategy arg and ret4 balances stack");
    Check58(eax58==0x13579bdf&&edx58==0x2468ace0&&(flags58&1)&&xmm58[3]==0x13579bdf,"flame bridge preserves original machine results");
    Check58(flameParticles58[0]==8&&flameParticles58[1]==21&&flameParticles58[2]==33&&flameParticles58[3]==7&&flameParticles58[6]==-1&&std::fabs(flameParticles58[15]+.8f)<1e-5&&std::fabs(flameParticles58[16]-.6f)<1e-5,"fresh native flame endpoints and distinct spray directions transported coherently");
    const float* flameMatrix58=reinterpret_cast<const float*>(flameRecord58)+0x60/4;const float* flameAngles58=reinterpret_cast<const float*>(flameRecord58)+0xa0/4;
    Check58(flameMatrix58[12]==8&&flameMatrix58[13]==21&&flameMatrix58[14]==33&&std::fabs(flameAngles58[2]-float(kPi/2))<1e-5,"flame record pose/Euler match final birth origin");

    const float frame241[12]={1,0,0,0,0,1,0,0,0,0,1,0};
    for(float headYaw241:{-.7f,.3f,.9f}){
        fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(wrapper58),0x1234,16,1,true);fc3aim::PublishMuzzleFrame206(frame241);
        memcpy(origin58,tip58,12);angles58[0]=.25f;angles58[1]=0;angles58[2]=headYaw241;
        {float head242[3]={};EulerForward242(angles58,head242);const float barrel242[3]={0,1,0};fc3aim::PublishMuzzleAim107(barrel242,head242);}
        float target242[3]={tip58[0],tip58[1]+40,tip58[2]};const bool readyWas242=g_laserQueryReady194;g_laserQueryReady194=false;
        CorrectProjectileVectors58(strategyPointer58,actorPointer58,origin58,angles58,nullptr,target242,0);g_laserQueryReady194=readyWas242;
        Check58(std::fabs(angles58[0])<1e-4&&std::fabs(angles58[2])<1e-4&&origin58[0]==8&&origin58[1]==21&&origin58[2]==33,"242 flare launch turns onto the published barrel ray while head yaw/pitch change");
        Check58(std::fabs(target242[0]-8)<1e-3&&std::fabs(target242[1]-61)<1e-3&&std::fabs(target242[2]-33)<1e-3,"242 flare target point re-cast along the barrel ray from the drawn muzzle at the native target distance");
        fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(wrapper58),0x1234,15,1,true);fc3aim::PublishMuzzleFrame206(frame241);
        flameActor58=0;MakeFlame58(strategyPointer58,flameRecordPointer58);
        float* matrix=reinterpret_cast<float*>(flameRecord58)+0x60/4;const float cs=std::cos(headYaw241),sn=std::sin(headYaw241);
        matrix[0]=matrix[5]=cs;matrix[1]=sn;matrix[4]=-sn;
        auto* euler=reinterpret_cast<float*>(flameRecord58)+0xa0/4;euler[2]=headYaw241;
        auto* axis=reinterpret_cast<float*>(flameRecord58)+0x4c/4;axis[0]=-sn;axis[1]=cs;
        for(int i=0;i<2;++i){float* v=flameParticles58+i*9;v[6]=-sn;v[7]=cs;v[8]=0;}
        CorrectFlameSpawn58(strategyPointer58,flameRecordPointer58);
        std::printf("241 yaw %.3f axis %.7f %.7f origin %.7f %.7f %.7f angle %.7f\n",headYaw241,flameParticles58[6],flameParticles58[7],matrix[12],matrix[13],matrix[14],euler[2]);
        Check58(std::fabs(flameParticles58[6]+1)<1e-5&&std::fabs(flameParticles58[7])<1e-5&&std::fabs(matrix[12]-8)<1e-5&&std::fabs(matrix[13]-21)<1e-5&&std::fabs(matrix[14]-33)<1e-5&&std::fabs(euler[2]-float(kPi/2))<1e-5,"241 native flame birth uses local equipped owner without bullet actor interface and removes head yaw from spray");
    }
    // A foreign owner cannot borrow the local displayed nozzle.
    MakeFlame58(strategyPointer58,flameRecordPointer58);reference58[0]^=1;CorrectFlameSpawn58(strategyPointer58,flameRecordPointer58);reference58[0]^=1;
    Check58(flameParticles58[0]==1&&flameParticles58[1]==2&&flameParticles58[6]==0&&flameParticles58[7]==1,"241 NPC/foreign-owner flame stays native");
    flameActor58=actorPointer58;
    // Also vary the native animated FRAME itself while keeping its displayed
    // destination fixed. This catches reapplying native camera motion twice.
    for(float yaw:{-.6f,0.f,.8f}){
        const float cs=std::cos(yaw),sn=std::sin(yaw);float nativeFrame[12]={cs,-sn,0,1,sn,cs,0,2,0,0,1,3},inverse[12]={},move[12]={};
        const float target[12]={0,-1,0,10,1,0,0,20,0,0,1,30};InvertFrame206(nativeFrame,inverse);
        for(int r=0;r<3;++r){for(int c=0;c<3;++c)for(int k=0;k<3;++k)move[r*4+c]+=target[r*4+k]*inverse[k*4+c];move[r*4+3]=target[r*4+3];for(int k=0;k<3;++k)move[r*4+3]+=target[r*4+k]*inverse[k*4+3];}
        fc3aim::PublishMuzzle58(move,root58,reinterpret_cast<UINT>(wrapper58),0x1234,16,1,true);fc3aim::PublishMuzzleFrame206(nativeFrame);
        origin58[0]=1-sn*.5f;origin58[1]=2+cs*.5f;origin58[2]=3;angles58[0]=.2f;angles58[1]=0;angles58[2]=yaw+.15f;
        {float head242[3]={};EulerForward242(angles58,head242);const float level242[3]={-std::sin(yaw+.15f),std::cos(yaw+.15f),0};fc3aim::PublishMuzzleAim107(level242,head242);}
        CorrectProjectileVectors58(strategyPointer58,actorPointer58,origin58,angles58,nullptr,nullptr,0);
        Check58(std::fabs(origin58[0]-9.5f)<1e-5&&std::fabs(origin58[1]-20)<1e-5&&std::fabs(origin58[2]-30)<1e-5&&std::fabs(angles58[0])<1e-4&&std::fabs(angles58[2]-(yaw+.15f))<1e-4,"242 changing native camera/FRAME keeps the physical flare muzzle; launch pitch/yaw follow the barrel ray, not the head");
    }
    { // 242: ballistic launch turned onto the barrel ray, target point re-cast, RPG and no-aim fallback unchanged
        const bool readyWas242=g_laserQueryReady194;g_laserQueryReady194=false;
        const float head242[3]={0,1,0},barrel242[3]={0,.70710678f,.70710678f};
        fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(wrapper58),0x1234,10,1,true);fc3aim::PublishMuzzleAim107(barrel242,head242);
        memcpy(origin58,tip58,12);angles58[0]=angles58[1]=angles58[2]=0;float target242[3]={tip58[0],tip58[1]+30,tip58[2]};
        CorrectProjectileVectors58(strategyPointer58,actorPointer58,origin58,angles58,nullptr,target242,0);
        float forward242[3]={};EulerForward242(angles58,forward242);
        Check58(origin58[0]==8&&origin58[1]==21&&origin58[2]==33&&std::fabs(forward242[0])<1e-4&&std::fabs(forward242[1]-.70710678f)<1e-4&&std::fabs(forward242[2]-.70710678f)<1e-4,"242 arrow launch forward equals the published barrel ray (45 deg up here), not the head or the gun move");
        Check58(std::fabs(target242[0]-8)<1e-3&&std::fabs(target242[1]-42.2132f)<1e-3&&std::fabs(target242[2]-54.2132f)<1e-3,"242 arrow target point lies on the barrel ray at the native target distance when no scene hit answers");
        fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(wrapper58),0x1234,32,1,true);fc3aim::PublishMuzzleAim107(barrel242,head242);
        memcpy(origin58,tip58,12);angles58[0]=angles58[1]=angles58[2]=0;target242[0]=tip58[0];target242[1]=tip58[1]+30;target242[2]=tip58[2];
        CorrectProjectileVectors58(strategyPointer58,actorPointer58,origin58,angles58,nullptr,target242,0);
        Check58(origin58[0]==8&&std::fabs(angles58[2]-float(kPi/2))<1e-5&&target242[1]==tip58[1]+30,"242 RPG keeps its accepted gun-move transport and native target");
        fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(wrapper58),0x1234,16,1,true);
        memcpy(origin58,tip58,12);angles58[0]=angles58[1]=angles58[2]=0;target242[1]=tip58[1]+30;
        CorrectProjectileVectors58(strategyPointer58,actorPointer58,origin58,angles58,nullptr,target242,0);
        Check58(std::fabs(angles58[2]-float(kPi/2))<1e-5&&target242[1]==tip58[1]+30,"242 flare without a fresh barrel ray falls back to the gun-move transport and leaves the native target");
        g_laserQueryReady194=readyWas242;
    }
    // Candidate 107: aim ray published for the core draw; the shot turns onto it.
    fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(wrapper58),0x1234,3,1,true);
    const float cameraForward107[3]={0,1,0};float aim107[3]={1,1,0};fc3aim::PublishMuzzleAim107(aim107,cameraForward107);
    origin58[0]=origin58[1]=origin58[2]=0;shot58[0]=0;shot58[1]=2;shot58[2]=0;
    CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);
    Check58(origin58[0]==8&&origin58[1]==21&&origin58[2]==33&&std::fabs(shot58[0]-1.41421356f)<1e-5&&std::fabs(shot58[1]-1.41421356f)<1e-5&&std::fabs(shot58[2])<1e-6,"107: drawn-muzzle origin with direction on the published aim ray (not the gun move)");
    aim107[1]=-1.f;fc3aim::PublishMuzzleAim107(aim107,cameraForward107);shot58[0]=0;shot58[1]=2;shot58[2]=0;
    CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);
    Check58(shot58[0]==-2,"107: an aim more than 75 degrees from the native camera keeps the gun-move transport");
    shot58[0]=0;shot58[1]=2;shot58[2]=0;fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(wrapper58),0x1234,3,1,true);
    CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);
    Check58(shot58[0]==-2,"107: a new publication without aim falls back to the gun-move transport");
    // Candidate 107: HUD widget still names the previous wrapper (PID63244 first M133 shot).
    UINT otherWrapper107[32]={};fc3aim::PublishMuzzle58(turn,root58,reinterpret_cast<UINT>(otherWrapper107),0x9999,6,1,true);
    origin58[0]=777;shot58[0]=0;shot58[1]=2;shot58[2]=0;
    CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);
    Check58(origin58[0]==8&&origin58[1]==21&&origin58[2]==33,"107: fresh publication with a stale HUD wrapper still moves the shot to the drawn muzzle");
    tip58[1]=3.2f;origin58[0]=777;CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);
    Check58(origin58[0]==777,"107: stale HUD wrapper requires the native muzzle within 1 m of the published root");tip58[1]=2;
    equipped58=false;origin58[0]=777;CorrectShotVectors58(reinterpret_cast<UINT>(strategy58),reinterpret_cast<UINT>(actor58),origin58,shot58);
    Check58(origin58[0]==777,"107: stale HUD wrapper with an unequipped strategy untouched");equipped58=true;
    { // 210: Jason's ApplyDamage bridge and signatures
        auto* image210=static_cast<BYTE*>(VirtualAlloc(nullptr,0x2010000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));Check58(image210!=nullptr,"210 fake engine image");
        const UINT b210=reinterpret_cast<UINT>(image210);
        const BYTE apply210[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x5c,0x53,0x8b,0xd9,0x8b,0x03,0x8b,0x90,0x80,0x00,0x00,0x00,0x56,0x57,0xff,0xd2,0x84,0xc0,0x0f,0x85,0xaa,0x05,0x00,0x00};
        const BYTE exit210[]={0x5f,0x5e,0x32,0xc0,0x5b,0x8b,0xe5,0x5d,0xc2,0x04,0x00};
        const BYTE invulnerable210[]={0x55,0x8b,0xec,0x83,0xec,0x0c,0x53,0x56,0x57,0x8b,0xf9};
        const BYTE modeOne210[]={0x83,0xf8,0x01,0x74,0x76};const BYTE type210[]={0xe9,0xab,0x40,0xf6,0xff};
        memcpy(image210+0x10d0f40,apply210,sizeof(apply210));memcpy(image210+0x10d150a,exit210,sizeof(exit210));memcpy(image210+0x10ce420,invulnerable210,sizeof(invulnerable210));
        memcpy(image210+0x10ce4d7,modeOne210,sizeof(modeOne210));memcpy(image210+0x44e8b0,type210,sizeof(type210));image210[0x3b2979]=0x68;
        *reinterpret_cast<UINT*>(image210+0x3b297a)=b210+0x1fe16bc;memcpy(image210+0x1fe16bc,"CFCXCountersComponentPlayerSP",30);
        UINT* vt210=reinterpret_cast<UINT*>(image210+0x2008e38);vt210[0x158/4]=b210+0x10d0f40;vt210[0x80/4]=b210+0x10ce420;vt210[1]=b210+0x44e8b0;
        Check58(ValidateGodMode210(image210),"210 verified PlayerSP vtable, ApplyDamage entry, native invulnerable exit and mode1 test");
        image210[0x10ce4d9]=0x02;Check58(!ValidateGodMode210(image210),"210 refuses an engine whose IsInvulnerable does not test mode1");image210[0x10ce4d9]=0x01;
        vt210[0x158/4]=b210+0x10cf7b0;Check58(!ValidateGodMode210(image210),"210 refuses a PlayerSP vtable whose slot 158 is not ApplyDamage");vt210[0x158/4]=b210+0x10d0f40;
        image210[0x1fe16bc+19]='M';Check58(!ValidateGodMode210(image210),"210 refuses a vtable whose type is not CFCXCountersComponentPlayerSP");image210[0x1fe16bc+19]='S';
        const UINT oldEngine210=g_engine;g_engine=b210;g_godAttempted210=false;
        Check58(!InstallGodMode210()&&g_godVtable210==0&&!g_godInstalled210,"210 MinHook refusal fails closed with no armed identity");
        g_engine=oldEngine210;g_godAttempted210=false;
        g_godVtable210=b210+0x2008e38;g_applyDamageOriginal210=reinterpret_cast<void*>(ApplyDamageFixture210);
        UINT jason210[4]={b210+0x2008e38},enemy210[4]={b210+0x2008b48};
        UINT eax210,edx210,flags210,esi210,xmm210,before210,after210;
        fc3aim::SetGodMode210(false);applyCalls210=0;Run210(reinterpret_cast<UINT>(jason210),12.5f,eax210,edx210,flags210,esi210,xmm210,before210,after210);
        Check58(applyCalls210==1&&applySelf210==reinterpret_cast<UINT>(jason210)&&applyAmount210==12.5f&&before210==after210,"210 God mode off: Jason's damage reaches the native ApplyDamage with ECX/float and ret4 balance");
        Check58((eax210&0xff)==1&&edx210==0x2468ace0&&(flags210&1)&&esi210==0x5eed5eed,"210 pass-through keeps the native result, EDX, carry and callee-saved registers");
        fc3aim::SetGodMode210(true);applyCalls210=0;const LONG blockedBefore210=g_godBlocked210;
        Run210(reinterpret_cast<UINT>(jason210),40.f,eax210,edx210,flags210,esi210,xmm210,before210,after210);
        Check58(applyCalls210==0&&eax210==0&&before210==after210&&esi210==0x5eed5eed&&xmm210==0x0badf00d&&g_godBlocked210==blockedBefore210+1,"210 God mode on: Jason's hit takes the native false exit, ret4, registers and SSE preserved, counted");
        Run210(reinterpret_cast<UINT>(enemy210),40.f,eax210,edx210,flags210,esi210,xmm210,before210,after210);
        Check58(applyCalls210==1&&applySelf210==reinterpret_cast<UINT>(enemy210)&&(eax210&0xff)==1&&before210==after210,"210 any other counters component keeps native damage");
        Run210(0,40.f,eax210,edx210,flags210,esi210,xmm210,before210,after210);
        Check58(applyCalls210==2&&before210==after210,"210 null component is never refused");
        Run210(0x10,40.f,eax210,edx210,flags210,esi210,xmm210,before210,after210);
        Check58(applyCalls210==3,"210 unreadable component faults closed into the native path");
        fc3aim::SetGodMode210(false);g_godVtable210=0;g_applyDamageOriginal210=nullptr;VirtualFree(image210,0,MEM_RELEASE);
        std::puts("PASS: god210 PlayerSP ApplyDamage signatures, fail-closed install, native false exit for Jason only, exact ABI and machine state");
    }
    fc3aim::ClearMuzzle58();VirtualFree(engine58,0,MEM_RELEASE);
    if(failures58)return 1;std::puts("PASS: muzzle58 production transport, local ownership, native tip, freshness, exact custom ABI and original machine state");std::puts("PASS: muzzle107 aim-ray direction with fallback, stale HUD wrapper accepted only for an equipped strategy within 1 m");return 0;
}
