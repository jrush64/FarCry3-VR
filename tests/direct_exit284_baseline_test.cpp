#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
ULONGLONG testNow=1000;
ULONGLONG TestNow(){return testNow;}
#define GetTickCount64 TestNow
#include "../../native283/src/aim_probe.cpp"
#undef GetTickCount64
#include "../src/weapon_presence.h"
namespace fc3 {void Log(const char*,...) {} }
namespace fc3xr {void SetTestNote(const wchar_t*) {} }
extern "C" MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*){return MH_ERROR_UNSUPPORTED_FUNCTION;}
extern "C" MH_STATUS WINAPI MH_EnableHook(LPVOID){return MH_ERROR_NOT_CREATED;}
extern "C" MH_STATUS WINAPI MH_RemoveHook(LPVOID){return MH_ERROR_NOT_CREATED;}
UINT fixtureHandle[2]={17,23};float desired[3]={0,0,0},actual[3]={0,.13f,0};
bool fixtureAlive=true;int writes=0,reads=0;
UINT* __cdecl GetLocal(UINT* h)
{h[0]=fixtureAlive?fixtureHandle[0]:~0u;h[1]=fixtureAlive?fixtureHandle[1]:~0u;return h;}
void __cdecl ReadDesired(const UINT* h,float* output)
{++reads;if(SameHandle(h,fixtureHandle))memcpy(output,desired,12);}
void __cdecl ReadActual(const UINT* h,float* output)
{if(SameHandle(h,fixtureHandle))memcpy(output,actual,12);}
// Exercise the actual custom EAX/ret-4 ABI used by the engine getters.
__declspec(naked) void DesiredGetter()
{
    __asm {
        push dword ptr [esp+4]
        push eax
        call ReadDesired
        add esp,8
        ret 4
    }
}
__declspec(naked) void ActualGetter()
{
    __asm {
        push dword ptr [esp+4]
        push eax
        call ReadActual
        add esp,8
        ret 4
    }
}
void __stdcall SetNative(const UINT* h,float pitch,float roll,float yaw)
{if(SameHandle(h,fixtureHandle)){++writes;desired[0]=actual[0]=pitch;desired[1]=actual[1]=roll;desired[2]=actual[2]=yaw;}}
UINT fixtureComponent128=0;
UINT __cdecl FixtureBeautifier128(UINT,UINT){return fixtureComponent128;}
UINT __cdecl FixturePawn281(UINT,UINT){return 0x1234;}
__declspec(naked) void NoVehicle281(){__asm {
 xor eax,eax
 ret
}}
void Check(bool ok,const char* why)
{if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
void Camera(float* f,double yaw,double pitch)
{f[0]=float(std::cos(pitch)*std::cos(yaw));f[1]=float(std::cos(pitch)*std::sin(yaw));f[2]=float(std::sin(pitch));}
void Frame(const float* camera,bool weapon=true)
{testNow+=20;fc3aim::Probe(camera,true,true,weapon);fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));fc3aim::Probe(camera,true,true,weapon);}
#include "vehicle_sample170_fixture.inl"
int main()
{
    auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,48332800,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(image!=nullptr,"fixture image");g_engine=reinterpret_cast<UINT>(image);g_engineBytes=48332800;
    UINT manager[4]={0,0,0,1};*reinterpret_cast<UINT*>(image+0x2829474)=reinterpret_cast<UINT>(manager);
    g_localHandle=GetLocal;g_getAngles=reinterpret_cast<void*>(DesiredGetter);g_getActual=reinterpret_cast<void*>(ActualGetter);g_setAngles=SetNative;
    float camera[3];Camera(camera,.7,0);desired[2]=actual[2]=float(.7-kPi/2);
    for(int n=0;n<3000;++n)fc3aim::Probe(camera,false,false,false);
    Check(writes==0&&reads==0&&!fc3aim::Ready(),"loading never moves camera or scans memory");
    for(int n=0;n<14;++n)Frame(camera);
    Check(fc3aim::Ready()&&writes==0,"level aim binds without a pose ritual or test write");
    const float previous=desired[2];fc3aim::Drive(1.2,.2);
    Check(writes==0&&desired[2]==previous,"render thread queues and never writes engine state");
    testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine-1));
    Check(writes==0,"non-game polling caller cannot write");
    fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(writes==1&&std::fabs(desired[2]-(1.2-kPi/2))<.0001&&desired[0]==.2f&&desired[1]==.13f,
        "native setter ABI applies both vectors and retains roll");
    fc3aim::Drive(1.3,.3);testNow+=151;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(writes==1,"expired command cannot move player");
    fc3aim::Drive(1.4,.4);fixtureHandle[0]++;
    testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(writes==1,"respawn handle rejects old queued command");
    Camera(camera,1.2,.2);Frame(camera);Check(!fc3aim::Ready(),"new owner requires passive verification");
    for(int n=0;n<14;++n)Frame(camera);
    Check(fc3aim::Ready(),"new owner reacquires without startup sweep");
    fc3aim::Drive(1.5,.5);fc3aim::Probe(camera,true,true,false);testNow+=20;
    fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(writes==1,"hidden weapon cancels queued aim");
    fc3aim::Drive(1.5,.5);fc3aim::Suspend();testNow+=20;
    fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==1,"tracking loss cancellation");
    fixtureAlive=false;testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(!fc3aim::Ready(),"invalid native player immediately disables aim");
    fixtureAlive=true;Camera(camera,desired[2]+kPi/2,desired[0]);for(int n=0;n<14;++n)Frame(camera,false);
    Check(fc3aim::Ready(),"hidden-weapon camera owner passively binds for visibility");
    const double baseYaw=desired[2]+kPi/2,basePitch=desired[0];const int beforeHead=writes;
    Check(fc3aim::DriveVisibility68(2.1,.35),"head visibility queues behind view with hidden weapon");
    Check(writes==beforeHead,"head visibility render queue cannot call native setter");
    desired[2]+=.14f;actual[2]+=.14f;fc3aim::Probe(camera,true,true,false);testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(writes==beforeHead+1&&fabs(Wrap(desired[2]-(baseYaw+.14+2.1-kPi/2)))<1e-5&&fabs(desired[0]-(basePitch+.35))<1e-5&&actual[1]==.13f,"visibility preserves concurrent native stick yaw, pitch and roll");
    double bodyYaw=0,bodyPitch=0,bodyRoll=0;Check(fc3aim::VisibilityBasis68(bodyYaw,bodyPitch,bodyRoll)&&fabs(Wrap(bodyYaw-baseYaw-.14))<1e-5&&fabs(bodyPitch-basePitch)<1e-5,"render compensation receives native body basis without applied HMD offset");
    const int once=writes;testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==once,"visibility delta consumed once, never twice per game poll");
    Check(fc3aim::DriveVisibility68(2.1,.35),"unchanged head queue");desired[2]+=.21f;actual[2]+=.21f;testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==once,"steady head does not overwrite native stick smoothing");
    fc3aim::DriveVisibility68(2.5,.4);fc3aim::DriveVisibility68(2.8,.45);const float previousHeadYaw=desired[2];testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==once+1&&fabs(Wrap(desired[2]-previousHeadYaw-.7))<1e-5,"replacement queue consumes all latest head movement once");
    fc3aim::DriveVisibility68(3.13,.45);testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));const float wrapBefore=desired[2];fc3aim::DriveVisibility68(-3.13,.45);testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(fabs(desired[2]-wrapBefore)<.025f,"rear wrap crossing remains continuous");
    fc3aim::DriveVisibility68(-3.0,.5);const int paused=writes;fc3aim::Suspend();testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==paused,"menu/tracking suspension cancels visibility writes");
    Check(fc3aim::DriveVisibility68(-3.13,.45),"resume preserves applied offset metadata");testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==paused,"resume cannot accumulate the same HMD rotation again");
    const double retainedHead=g_visibility68.appliedYaw;fixtureAlive=false;testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));fc3aim::Probe(camera,true,true,false);Check(g_visibility68.appliedYaw==retainedHead&&!fc3aim::Ready(),"temporary native owner/camera dropout retains successful head offset accounting");fixtureAlive=true;Camera(camera,desired[2]+kPi/2,desired[0]);for(int n=0;n<14;++n)Frame(camera,false);Check(fc3aim::Ready()&&g_visibility68.appliedYaw==retainedHead,"same player reacquires without losing head offset");const int resumedHead=writes;fc3aim::DriveVisibility68(-3.13,.45);testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==resumedHead,"same-owner recovery cannot reapply the previous head turn");
    fc3aim::DriveVisibility68(-2.9,1.5);testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(desired[0]<=80*kPi/180+1e-6,"head visibility respects native camera pitch limit");
    fc3aim::Drive(.9,.1);testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(fabs(Wrap(desired[2]-(.9-kPi/2)))<1e-5&&g_visibility68.appliedYaw==0,"return to armed aim replaces visibility offset ownership");
    Camera(camera,.9,.1);Frame(camera,false);fc3aim::DriveVisibility68(2,.2);const int staleHead=writes;testNow+=151;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==staleHead,"stale head queue cannot move native camera");
    fc3aim::DriveVisibility68(2,.2);fixtureHandle[0]++;testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==staleHead,"visibility rejects changed player owner before native writes");
    Camera(camera,desired[2]+kPi/2,desired[0]);for(int n=0;n<14;++n)Frame(camera,false);Check(fc3aim::Ready(),"replacement owner passively reacquires");fc3aim::DriveVisibility68(2,.2);testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));const int refusedCamera=writes;for(int n=0;n<14;++n)Frame(camera,false);Check(!fc3aim::Ready()&&!g_command.active&&!fc3aim::DriveVisibility68(2.1,.2)&&writes==refusedCamera,"camera that does not follow native player look loses visibility ownership and receives no further writes");
    std::puts("PASS: visibility68 hidden camera queue, retained native stick/roll, consume-once/replacement/wrap semantics, compensated body basis, native pitch clamp, pause/resume, stale/respawn guards and armed handoff.");
    const fc3weapon::Geometry gun={257896,47478,40,336,486,4071,57};
    Check(fc3weapon::Present(gun),"known shotgun body detected");
    auto shell=gun;shell.start=18;shell.count=108;
    Check(fc3weapon::Model(shell)&&!fc3weapon::Present(shell),"shell retains weapon camera but cannot activate motion");
    auto world=gun;world.vertexBytes++;Check(!fc3weapon::Model(world),"scenery with different geometry excluded");
    const fc3weapon::Geometry box={81632,18306,32,0,2880,5949,57};
    Check(!fc3weapon::Model(box),"ammo box never qualifies as player weapon");
    Check(MatchForm(0,0,kYawForms,8,true)<0,"generic symmetric form search is ambiguous and is not used for startup");
    // Replay the native/render disagreement surrounding the captured climb.
    LoseBinding();g_visibility68={};for(auto& old:g_cameraHistory127)old={};
    desired[0]=actual[0]=.0908141f;desired[2]=actual[2]=9.3823795f;
    Camera(camera,desired[2]+kPi/2,desired[0]);for(int n=0;n<14;++n)Frame(camera,false);
    Check(fc3aim::Ready(),"127 climb replay begins with a verified normal owner");
    g_visibility68.appliedYaw=.60445;g_visibility68.appliedPitch=.46880;memcpy(g_visibility68.handle,fixtureHandle,8);
    Check(fc3aim::DriveVisibility68(.65,.50),"127 queues a pending head write before climb handoff");const int beforeClimb127=writes;
    Camera(camera,-1.59388,.67667);fc3aim::Probe(camera,true,true,false);
    Check(!fc3aim::Ready()&&fc3aim::HiddenCameraBlocked127()&&!g_command.active&&g_visibility68.appliedYaw==0&&g_visibility68.appliedPitch==0,"captured33-degree hidden climb disagreement releases immediately and rebases head metadata");
    testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(writes==beforeClimb127,"climb handoff cancels queued writes before the next native input poll");
    desired[0]=actual[0]=0;desired[2]=actual[2]=float(-kPi/2);Camera(camera,0,0);
    for(int n=0;n<14;++n)Frame(camera,false);Check(fc3aim::Ready()&&!fc3aim::HiddenCameraBlocked127(),"127 passively reacquires only after native/render camera agree again");
    double ry=0,rp=0,rr=0;Check(fc3aim::VisibilityBasis68(ry,rp,rr)&&fabs(ry)<1e-5&&fabs(rp)<1e-5,"post-climb body basis cannot subtract stale pre-climb headset offsets");
    Check(fc3aim::DriveVisibility68(.4,.1),"127 queues ordinary hidden head turn");testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    fc3aim::Probe(camera,true,true,false);Check(fc3aim::Ready(),"one-frame native input/draw latency retains normal head look via matching recent history");
    testNow+=61;fc3aim::Probe(camera,true,true,false);Check(!fc3aim::Ready(),"camera that stays at old view beyond60ms cannot retain ownership indefinitely");
    puts("PASS: camera127 captured climb handoff cancels pending look immediately; no stale head-offset subtraction; passive reacquisition; ordinary input/draw lag retained and stalled render owner rejected");
    // Replay the actual component/vtable/type ABI, not only a forced action boolean.
    auto* component128=reinterpret_cast<UINT*>(image+0x2000);auto* object128=reinterpret_cast<UINT*>(image+0x2100);
    auto* vtable128=reinterpret_cast<UINT*>(image+0x2200);auto* type128=reinterpret_cast<UINT*>(image+0x2300);
    fixtureComponent128=reinterpret_cast<UINT>(component128);component128[0x34/4]=reinterpret_cast<UINT>(object128);
    object128[0]=reinterpret_cast<UINT>(vtable128);vtable128[1]=reinterpret_cast<UINT>(image+0x2400);
    image[0x2400]=0xb8;const UINT typeAddress128=reinterpret_cast<UINT>(type128);memcpy(image+0x2401,&typeAddress128,4);image[0x2405]=0xc3;
    DWORD oldProtect128=0;VirtualProtect(image+0x2000,0x1000,PAGE_EXECUTE_READWRITE,&oldProtect128);FlushInstructionCache(GetCurrentProcess(),image+0x2400,6);
    type128[1]=4;g_beautifierOf128=reinterpret_cast<void*>(FixtureBeautifier128);
    const UINT descriptors128[]={0x2884e20,0x28855c4,0x2885758,0x2884c24,0x287df64,0x287de68,0x287df80};
    for(int i=0;i<7;++i){auto* descriptor=reinterpret_cast<UINT*>(image+descriptors128[i]);descriptor[1]=4;descriptor[5]=100+i;type128[5]=100+i;
      Check(NativeAction128(fixtureHandle)==(i<6?(1<<i):0),"six specific native actions detected; generic FirstNoControl must retain motion controls");}
    g_extraActionBound283=true;
    const UINT extraTypes283[]={0x2884ce8,0x287dfa0,0x2884e3c,0x2884e58,0x2884c08};
    for(int i=0;i<5;++i){
      auto* type=reinterpret_cast<UINT*>(image+extraTypes283[i]);type[1]=4;type[5]=1000+i;type128[5]=1000+i;
      Check(NativeAction128(fixtureHandle)==0,"283 shared ancestor/type ID alone cannot disable weapon hands");
      const UINT exact=reinterpret_cast<UINT>(type);memcpy(image+0x2401,&exact,4);FlushInstructionCache(GetCurrentProcess(),image+0x2400,6);
      Check(NativeAction128(fixtureHandle)==(256<<i),"283 exact pickup/cinematic/carry/door owner yields to native animated hands");
      memcpy(image+0x2401,&typeAddress128,4);FlushInstructionCache(GetCurrentProcess(),image+0x2400,6);
    }
    g_extraActionBound283=false;
    g_slideBound280=true;auto* slideDescriptor280=reinterpret_cast<UINT*>(image+0x287de84);slideDescriptor280[1]=4;slideDescriptor280[5]=901;type128[5]=901;int slideState280=-1;
    Check(NativeAction128(fixtureHandle,&slideState280)==0&&slideState280==0,"280 sibling RTTI sharing an ancestor ID cannot activate slide camera");
    const UINT slideTypeAddress280=reinterpret_cast<UINT>(slideDescriptor280);memcpy(image+0x2401,&slideTypeAddress280,4);FlushInstructionCache(GetCurrentProcess(),image+0x2400,6);
    Check(NativeAction128(fixtureHandle,&slideState280)==0&&slideState280==1,"280 exact native Slide RTTI is a camera owner without becoming a broad arm-action mask");
    memcpy(image+0x2401,&typeAddress128,4);FlushInstructionCache(GetCurrentProcess(),image+0x2400,6);type128[5]=999;Check(NativeAction128(fixtureHandle,&slideState280)==0&&slideState280==0,"ordinary first-person type retains controller arms and cannot activate slide camera");
    component128[0x34/4]=0;Check(NativeAction128(fixtureHandle,&slideState280)==-1&&slideState280==-1,"missing native owner stays unknown rather than claiming idle or slide");
    component128[0x34/4]=reinterpret_cast<UINT>(object128);memcpy(image+0x2401,&slideTypeAddress280,4);FlushInstructionCache(GetCurrentProcess(),image+0x2400,6);
    fc3aim::Drive(.2,.1);const int beforeSlide280=writes;testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(g_sample.slide280==1&&!g_command.active&&writes==beforeSlide280,"280 native slide cancels pending gun/head-look writes in the same input poll");
    g_sample.vehicle=0;Check(fc3aim::SlideCamera280(),"280 fresh confirmed on-foot slide owns camera");g_sample.vehicle=1;Check(!fc3aim::SlideCamera280(),"280 vehicle can never enter slide lane");g_sample.vehicle=0;
    testNow+=251;Check(!fc3aim::SlideCamera280(),"280 stale native slide cannot own camera");g_slideBound280=false;
    // Transition latch: action cancels pending writes and tolerates a short type gap.
    g_actionTest128=0;desired[0]=actual[0]=0;desired[2]=actual[2]=float(-kPi/2);Camera(camera,0,0);
    for(int n=0;n<14;++n)Frame(camera);fc3aim::Drive(.2,.1);const int beforeAction128=writes;
    g_actionTest128=64;testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(g_sample.action128==0&&!fc3aim::NativeActionOwnsArms128()&&writes==beforeAction128+1,"130 replay of observed mask64 leaves gameplay arms active and permits queued aim write");
    fc3aim::Drive(.3,.15);const int beforeSpecific130=writes;
    g_actionTest128=4;testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(g_sample.action128==4&&!g_command.active&&writes==beforeSpecific130,"takedown cancels queued camera write in the same native poll");
    g_sample.vehicle=0;const UINT actionEpoch282=g_sample.actionEpoch282;
    Check(fc3aim::NativeActionCamera282()==4&&actionEpoch282,"282 fresh native action has separate camera ownership");
    g_actionTest128=0;testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(g_sample.action128==4,"short action-to-idle handoff cannot immediately snap arms back");
    Check(g_sample.cameraAction282==0&&g_sample.actionEpoch282!=actionEpoch282&&!fc3aim::NativeActionCamera282(),"282 arm-release grace cannot extend camera writes or receipts after the real action ends");
    testNow+=160;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));Check(g_sample.action128==0,"confirmed idle restores controller ownership after150ms");
    g_actionTest128=4;Frame(camera);const UINT beforeGap282=g_sample.actionEpoch282;
    fixtureAlive=false;Frame(camera);
    Check(!g_sample.valid&&g_sample.actionEpoch282!=beforeGap282,"282 missing local sample invalidates the camera episode immediately");
    fixtureAlive=true;Frame(camera);
    Check(g_sample.valid&&g_sample.actionEpoch282!=beforeGap282,"282 same-player same-action recovery cannot resurrect the command from before the dropout");
    g_actionTest128=-1;g_beautifierOf128=nullptr;
    puts("PASS: native action128 component/current-owner RTTI ABI, six specific action identities, rejected generic FirstNoControl, missing owner guard, same-poll look cancellation and confirmed idle restoration");
    const auto oldPawn281=g_pawnFromHandle,oldVehicle281=g_vehicleSeatOf;
    g_pawnFromHandle=reinterpret_cast<void*>(FixturePawn281);g_vehicleSeatOf=reinterpret_cast<void*>(NoVehicle281);g_actionTest128=0;
    desired[0]=actual[0]=0;desired[2]=actual[2]=float(-kPi/2);Camera(camera,0,0);for(int n=0;n<14;++n)Frame(camera);
    Check(fc3aim::NativeFootCamera281(),"281 native on-foot view is available independently of weapon/camera classes");
    fc3aim::Drive(.2,.1);const int beforeFoot281=writes;
    vehicle279::command={};vehicle279::command.active=true;vehicle279::command.lane280=3;vehicle279::command.ms=testNow;
    memcpy(vehicle279::command.owner,fixtureHandle,sizeof(fixtureHandle));
    testNow+=20;fc3aim::InputTick(reinterpret_cast<void*>(g_engine+0x100));
    Check(g_sample.vehicle==0&&!g_command.active&&writes==beforeFoot281,"281 common native view cancels pending CPawn gun/head write in the same poll");
    g_sample.vehicle=1;Check(!fc3aim::NativeFootCamera281(),"281 on-foot native request rejects vehicle owner");g_sample.vehicle=0;
    g_sample.action128=4;Check(!fc3aim::NativeFootCamera281(),"281 protected native action keeps its ownership");g_sample.action128=0;
    testNow+=251;Check(!fc3aim::NativeFootCamera281(),"281 stale on-foot sample refuses native ownership");
    vehicle279::Set(false,nullptr);g_actionTest128=512;Frame(camera);
    vehicle279::command={};auto& directExit=vehicle279::command;directExit.active=true;directExit.lane280=4;directExit.action282=512;directExit.actionEpoch282=g_sample.actionEpoch282;directExit.ms=testNow;memcpy(directExit.owner,g_sample.handle,sizeof(directExit.owner));
    vehicle279::acquired=true;auto& proof=vehicle279::receipts[777%32];proof={};proof.serial=777;proof.lane280=4;proof.action282=512;proof.actionEpoch282=g_sample.actionEpoch282;proof.ms=testNow;memcpy(proof.owner,g_sample.handle,sizeof(proof.owner));
    g_actionTest128=0;Frame(camera);
    Check(vehicle279::command.active&&vehicle279::command.lane280==3,"284 baseline direct native script-to-gun exit must retain camera coverage before rendering");
    vehicle279::Set(false,nullptr);g_sample.ms=testNow;g_pawnFromHandle=oldPawn281;g_vehicleSeatOf=oldVehicle281;g_actionTest128=-1;
    puts("PASS:281 common on-foot native ownership, same-poll CPawn cancellation and vehicle/action/staleness guards");
    // Reproduce the observed release deadlock: plain native facing is level
    // while the actual draw already contains a turned HMD orientation.
    vehicle279::Set(false,nullptr);LoseBinding();g_sample.vehicle=0;g_sample.slide280=0;g_sample.action128=g_sample.cameraAction282=0;
    g_sample.valid=true;g_sample.angles[0]=0;g_sample.angles[2]=float(-kPi/2);
    float headTarget283[3][3]={{1,0,0},{0,0,1},{0,1,0}};
    const double turn283=.8;headTarget283[0][0]=float(sin(turn283));headTarget283[0][1]=float(-cos(turn283));headTarget283[2][0]=float(cos(turn283));headTarget283[2][1]=float(sin(turn283));
    for(int n=0;n<16;++n){testNow+=20;g_sample.ms=testNow;++g_sample.sequence;fc3aim::Probe(headTarget283[2],true,true,true);}
    Check(!fc3aim::Ready(),"283 negative control: composed head direction cannot passively bind plain pawn angles");
    for(int n=0;n<16;++n){
      testNow+=20;g_sample.ms=testNow;++g_sample.sequence;
      vehicle279::command={};auto& c=vehicle279::command;c.active=true;c.lane280=3;c.ms=testNow;memcpy(c.owner,g_sample.handle,sizeof(c.owner));
      auto& r=vehicle279::receipts[0];r={};r.serial=1;r.lane280=3;r.ms=testNow;memcpy(r.owner,g_sample.handle,sizeof(r.owner));memcpy(r.target,headTarget283,sizeof(r.target));r.sourceForward283[0]=1;
      fc3aim::ProbeNativeView283(headTarget283[0],headTarget283[1],headTarget283[2],true,true,true);
    }
    Check(fc3aim::Ready(),"283 matched unmodified native source binds ordinary gun aim while native head camera stays active");
    const int beforeBind283=writes;fc3aim::Drive(.2,.1);vehicle279::Set(false,nullptr);
    Check(writes==beforeBind283,"283 passive handoff performs no test/sweep camera write");
    float source283[3],basis283[9];UINT receipt283=0;
    Check(!vehicle279::Find(headTarget283[0],headTarget283[1],headTarget283[2],basis283,receipt283,source283),"283 released native receipt cannot authorize passive binding");
    puts("PASS:283 actual composed-view binding deadlock negative control and owner/draw-matched native-source reacquisition");
    unsigned long long identity143=0;const int identityReads143=reads,identityWrites143=writes;
    Check(fc3aim::LocalPlayerIdentity143(identity143)&&identity143==(static_cast<unsigned long long>(fixtureHandle[0])|(static_cast<unsigned long long>(fixtureHandle[1])<<32)),"143 throwable identity uses both native local handle words");
    Check(reads==identityReads143&&writes==identityWrites143,"143 identity snapshot does not call engine or move camera");
    AcquireSRWLockExclusive(&g_nativeLock);const bool recursiveIdentity143=fc3aim::LocalPlayerIdentity143(identity143);ReleaseSRWLockExclusive(&g_nativeLock);
    Check(!recursiveIdentity143,"143 launch bridge refuses contended identity instead of recursively blocking game input");
    testNow+=251;Check(!fc3aim::LocalPlayerIdentity143(identity143)&&identity143==0,"143 stale native local player cannot authorize throw");
    g_sample.ms=testNow;g_sample.valid=false;Check(!fc3aim::LocalPlayerIdentity143(identity143),"143 invalidated respawn owner cannot authorize throw");
    puts("PASS: throwable143 identity snapshot uses fresh verified native owner, no engine calls, no lock recursion and rejects stale/invalid lifecycle samples");
    TestVehicleSample170();
    VirtualFree(image,0,MEM_RELEASE);
    std::puts("PASS: native owner/ABI, passive level binding, no sweep, queued game-input writes, stale/respawn/hidden cancellation and weapon identity.");
}
