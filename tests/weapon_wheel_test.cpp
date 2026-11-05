#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "../src/vr_pad.cpp"
#include "../src/native_ui_pointer.h"
namespace fc3 { void Log(const char*,...) {} }
namespace fc3aim {bool bodyAction285=false;bool NativeActionOwnsArms128(){return bodyAction285;} }
namespace fc3aim {bool GetVehicleSample170(VehicleSample170& out){out={};return false;} }
namespace fc3aim {unsigned long long bodyOwner285=1;bool LocalPlayerIdentity143(unsigned long long& id){id=bodyOwner285;return id!=0;} }
namespace fc3aim {bool testVehicle128=false;bool VehicleState(bool& seated,int& seat){seated=testVehicle128;seat=seated?0:-1;return true;}int aimCalls=0,suspends=0;void InputTick(const void*) {++aimCalls;}void Suspend(){++suspends;} }
extern "C" MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*) {return MH_OK;}
extern "C" MH_STATUS WINAPI MH_EnableHook(LPVOID) {return MH_OK;}
void Check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
DWORD WINAPI RealPad(DWORD,PadReading* reading){reading->packet=17;reading->pad={};reading->pad.buttons=kA|kX;reading->pad.rightTrigger=255;reading->pad.thumbLX=20000;return ERROR_SUCCESS;}
#include "controls20_fixture.inl"

void TestAmmo27(){
 using A=fc3pad::AmmoButtonAction;
 fc3pad::SetGameplayControls(true);fc3pad::SetDedicatedInteractions(true);
 fc3pad::SetMacheteHandling(false);fc3pad::SetMacheteContext(false);fc3pad::BlockFire(false);
 fc3pad::SetManualReload(false,false);fc3pad::SetShotgunHandling(false);fc3pad::SetMagazineHandling(false);fc3pad::CancelReloadRequest();
 for(bool swapped:{false,true}){
  fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);fc3pad::SetSwapBX(swapped);fc3pad::Publish(c);
  auto press=[&](bool down){c.x=swapped?false:down;c.b=swapped?down:false;};
  c.rightGrip=1;c.rightTrigger=1;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kRightShoulder)&&FromControllers(g_state).rightTrigger==255,"right grip modifier alone cannot throw a grenade or consume firing");c.rightGrip=c.rightTrigger=0;fc3pad::Publish(c);
  for(int grips=0;grips<4;++grips){
   c.leftGrip=c.rightGrip=0;press(false);fc3pad::Publish(c);
   c.leftGrip=(grips&1)?1.f:0.f;c.rightGrip=(grips&2)?1.f:0.f;press(true);fc3pad::Publish(c);
   const A expected=grips==0?A::Interaction:grips==1?A::Release:grips==2?A::Consumed:A::PickupOverride;
   for(int held=0;held<120;++held){fc3pad::Publish(c);Check(fc3pad::AmmoAction(fc3pad::Latest())==expected,"one press retains exactly one semantic owner");
    Check(fc3pad::AmmoControls().x==(grips==1||grips==3),"only grip chords reach physical handlers");
    Check(((FromControllers(g_state).buttons&kX)!=0)==(grips==0),"interaction and ammo chord never reach native X together");
    Check(!(FromControllers(g_state).buttons&kRightShoulder),"right-grip ammo chord cannot throw a grenade");}
   if(grips){c.leftGrip=c.rightGrip=0;fc3pad::Publish(c);Check(!fc3pad::AmmoControls().x&&!(FromControllers(g_state).buttons&kX),"releasing modifier with X held cannot become interaction");
    c.leftGrip=(grips&1)?1.f:0.f;c.rightGrip=(grips&2)?1.f:0.f;fc3pad::Publish(c);Check(!fc3pad::AmmoControls().x,"regripping cannot repeat held ammo press");}
   else{c.leftGrip=c.rightGrip=1;fc3pad::Publish(c);Check(fc3pad::InteractionOnly()&&!fc3pad::AmmoControls().x,"adding grips to held interaction cannot eject or pick up");}
  }
  press(false);c.leftGrip=c.rightGrip=0;fc3pad::Publish(c);
  fc3pad::SetGripContext(true,false);c.leftGrip=1;fc3pad::Publish(c);Check(fc3pad::WheelOwnsGrip(),"fixture enters contextual wheel");
  c.rightGrip=1;press(true);fc3pad::Publish(c);Check(!fc3pad::WheelOwnsGrip()&&fc3pad::AmmoPickupOverride(fc3pad::AmmoControls()),"explicit both-grip pickup overrides contextual wheel");
  press(false);c.leftGrip=c.rightGrip=0;fc3pad::Publish(c);
 }
 fc3pad::SetSwapBX(false);fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 fc3pad::SetGameplayControls(false);
 puts("PASS: controls27 unswapped/swapped interaction, three grip combinations, one owner per press, modifier release/regrip guards and explicit pickup override of contextual wheel");
}

void TestUi33(){
 fc3ui::PointerState state;
 auto step=state.Step(true,true,0,0,.01f,100,50,400,200);Check(!step.down,"opening with held A cannot click a map destination");
 step=state.Step(true,false,0,0,.01f,100,50,400,200);Check(!step.down&&!step.up,"neutral A arms click");
 step=state.Step(true,true,0,0,.01f,100,50,400,200);Check(step.down&&!step.up,"fresh A makes one real click down");
 for(int i=0;i<100;++i){step=state.Step(true,true,0,0,.01f,100,50,400,200);Check(!step.down&&!step.up,"held A cannot repeat a click");}
 step=state.Step(false,true,0,0,.01f,100,50,400,200);Check(step.up&&!step.down,"menu close or focus loss releases owned click");
 step=state.Step(false,false,0,0,.01f,100,50,400,200);Check(!step.up,"inactive bridge never emits another release");
 for(float dt:{.01f,.016f,.05f,100.f}){
  step=state.Step(true,false,1,1,dt,399,0,400,200);Check(step.x==399&&step.y==0,"cursor stays within client on either screen edge");
  step=state.Step(true,false,-1,-1,dt,100,50,400,200);Check(step.x<100&&step.y>50&&step.x>=0&&step.y<200,"right stick axes move the real cursor with dt clamp");
 }
 step=state.Step(true,false,.15f,-.15f,.01f,100,50,400,200);Check(!step.move,"stick deadzone cannot drift map cursor");
 step=state.Step(true,false,NAN,INFINITY,NAN,100,50,400,200);Check(!step.move&&std::isfinite(step.x),"invalid tracking cannot move cursor");
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);ObserveNativePause(true);fc3pad::Publish(c);Check(fc3pad::NativeMenuConfirmed()&&fc3pad::GameplayInputBlocked()&&!fc3pad::NativeInputBlocked(),"native UI owns pointer while weapons stay blocked");
 c.a=c.b=true;c.rightX=.8f;c.rightY=-.6f;fc3pad::Publish(c);fc3pad::SetNativeMousePointer(true);
 c.leftTrigger=c.rightTrigger=1;c.rightStickClick=true;fc3pad::Publish(c);
 auto pad=FromControllers(g_state);Check((pad.buttons&kA)&&(pad.buttons&kB)&&pad.rightTrigger==255&&pad.leftTrigger==255&&!pad.thumbRX&&!pad.thumbRY&&!(pad.buttons&kRightThumb),"pointer menu preserves native A confirm, B back and map zoom; cursor click cannot emit melee");
 for(bool swapped:{false,true}){
  fc3pad::SetSwapBX(swapped);fc3pad::Controllers neutral={};neutral.active=true;fc3pad::Publish(neutral);fc3pad::Publish(c);pad=FromControllers(g_state);
  Check((pad.buttons&kA)&&pad.rightTrigger==255,"native A selection and zoom survive B/X swap");
  Check(!fc3pad::AmmoControls().x&&!fc3pad::Latest().a&&fc3pad::GameplayInputBlocked(),"map/menu confirm and clicks cannot reach physical reloads");
 }
 fc3pad::SetSwapBX(false);fc3pad::Controllers neutral={};neutral.active=true;fc3pad::Publish(neutral);fc3pad::Publish(c);
 Check(fc3pad::NativeUiControls().a&&!fc3pad::Latest().a,"UI bridge gets raw menu controls; physical reload consumers do not");
 fc3pad::SetNativePointerTrigger298(true);pad=FromControllers(g_state);Check(pad.rightTrigger==0&&pad.leftTrigger==255&&(pad.buttons&kA),"ray trigger click suppresses only native RT zoom, preserving A and LT");
 fc3pad::SetNativePointerTrigger298(false);Check(FromControllers(g_state).rightTrigger==255,"native RT map zoom restores when pointer trigger lease ends");
 InterlockedExchange(&g_nativePointerTriggerAt298,LONG(GetTickCount()-121));Check(FromControllers(g_state).rightTrigger==255,"stale pointer trigger lease cannot keep blocking native zoom");InterlockedExchange(&g_nativePointerTriggerAt298,0);
 fc3pad::SetNativeMousePointer(false);Check(FromControllers(g_state).buttons&kA,"native gamepad menu input restored after pointer mode");
 ObserveNativePause(false);c={};c.active=true;fc3pad::Publish(c);
 puts("PASS: UI33 native cursor movement/deadzone/edges, click ownership/focus exit, menu input routing and physical reload isolation");
}
void TestVehicleMachete128(){
 fc3pad::Controllers c={};c.active=true;fc3pad::SetGameplayControls(true);fc3pad::BlockFire(false);
 fc3pad::SetMagazineHandling(false);fc3pad::SetShotgunHandling(false);fc3pad::SetManualReload(false,false);
 fc3pad::SetLeverHandling(false);fc3pad::SetMacheteContext(true);fc3pad::SetMacheteHandling(true);
 c.rightTrigger=1;c.leftTrigger=1;fc3pad::Publish(c);
 Check(FromControllers(g_state).rightTrigger==0,"held machete blocks firearm trigger on foot");
 fc3aim::testVehicle128=true;
 Check(FromControllers(g_state).leftTrigger==255,"confirmed seat keeps native brake/reverse trigger despite firearm ADS suppression");
 Check(FromControllers(g_state).rightTrigger==255,"confirmed vehicle accelerator bypasses held machete mask before render stows it");
 fc3pad::SetMacheteHandling(false);
 Check(FromControllers(g_state).rightTrigger==255,"machete release latch cannot consume accelerator");
 fc3aim::testVehicle128=false;
 Check(FromControllers(g_state).rightTrigger==0,"leaving vehicle with trigger held retains firearm release guard");
 c.rightTrigger=c.leftTrigger=0;fc3pad::Publish(c);fc3pad::SetMacheteHandling(true);
 c.rightStickClick=true;c.rightGrip=0;fc3pad::Publish(c);
 Check(FromControllers(g_state).buttons&kRightThumb,"ungripped R3 can request native stealth takedown while machete held");
 c.rightGrip=1;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kRightThumb),"draw/stow chord never leaks native swipe");
 c={};c.active=true;fc3pad::Publish(c);fc3pad::SetMacheteHandling(false);fc3pad::SetMacheteContext(false);
 puts("PASS: vehicle accelerator survives held machete and release latch; on-foot fire guard and explicit native stealth input retained");
}
void TestSelector290(){
 fc3pad::SetGameplayControls(true);fc3pad::SetDedicatedInteractions(true);fc3pad::BlockFire(false);
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);fc3pad::SetSelector290(true);
 c.a=true;c.rightGrip=1;c.rightTrigger=1;fc3pad::Publish(c);
 Check(!(FromControllers(g_state).buttons&(kA|kRightShoulder))&&FromControllers(g_state).rightTrigger==0,"290 selector owns opening A, grip and firing");
 Check(!fc3pad::Latest().a&&fc3pad::Latest().rightGrip==0,"290 selector cannot feed physical weapon handlers");
 fc3pad::SetSelector290(false);fc3pad::Publish(c);
 Check(!(FromControllers(g_state).buttons&kA)&&FromControllers(g_state).rightTrigger==0&&fc3pad::Latest().rightGrip==0,"290 closing with buttons held cannot jump, fire or grab ammo");
 c={};c.active=true;fc3pad::Publish(c);c.a=true;c.rightTrigger=1;fc3pad::Publish(c);
 Check((FromControllers(g_state).buttons&kA)&&FromControllers(g_state).rightTrigger==255,"290 fresh actions after neutral keep normal jump and firing");
 c={};c.active=true;fc3pad::Publish(c);fc3pad::SetSelector290(true);
 InterlockedExchange(&g_selectorPadAt290,GetTickCount()-200);fc3pad::SetSelector290(false);c.a=true;c.rightGrip=c.rightTrigger=1;fc3pad::Publish(c);
 Check(!(FromControllers(g_state).buttons&kA)&&FromControllers(g_state).rightTrigger==0&&fc3pad::Latest().rightGrip==0,"290 a render-gap cancellation still guards fresh held inputs until neutral");
 c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(false);
 puts("PASS: selector290 native A/fire/grip ownership and neutral-release restoration");
}
int main()
{
 TestSelector290();
    {
      fc3pad::Controllers c={};c.active=true;fc3pad::SetGameplayControls(true);fc3pad::Publish(c);
      fc3pad::SetLookOverride(true,0,0);
      for(int n=0;n<8;++n){const float axes[8][2]={{1,0},{0,1},{-1,0},{0,-1},{.707f,.707f},{-.707f,.707f},{-.707f,-.707f},{.707f,-.707f}};
        fc3pad::SetBodyInput285(1,axes[n][0],axes[n][1]);auto pad=FromControllers(g_state);fc3pad::ApplyLookOverride(pad);
        Check((pad.buttons&kLeftShoulder)&&pad.thumbRX==Axis(axes[n][0])&&pad.thumbRY==Axis(axes[n][1]),"285 native wheel receives all body-zone directions through aim suppression");
        Check(fc3pad::WeaponSelectionRequested()&&fc3pad::WeaponWheelRequested145()&&fc3pad::WheelOwnsGrip(),"285 all existing consumers see explicit inventory ownership");
        fc3pad::SetBodyInput285(2,axes[n][0],axes[n][1]);pad=FromControllers(g_state);fc3pad::ApplyLookOverride(pad);
        Check(!(pad.buttons&kLeftShoulder)&&pad.thumbRX==Axis(axes[n][0])&&pad.thumbRY==Axis(axes[n][1]),"285 release emits selected direction without turning camera");
        float x=9,y=9;fc3pad::LookStick(x,y);Check(!x&&!y,"285 release frame cannot turn body");
      }
      fc3pad::SetBodyInput285(1,1,0);fc3aim::bodyAction285=true;Check(!(FromControllers(g_state).buttons&kLeftShoulder),"285 native action preempts body selection in input poll");fc3aim::bodyAction285=false;
      fc3aim::testVehicle128=true;Check(!(FromControllers(g_state).buttons&kLeftShoulder),"285 vehicle preempts body selection before next render");fc3aim::testVehicle128=false;
      fc3aim::bodyOwner285=2;Check(!(FromControllers(g_state).buttons&kLeftShoulder),"285 respawn/different native owner refuses old body input");fc3aim::bodyOwner285=1;
      g_bodyInput285.at=GetTickCount()-121;Check(!fc3pad::BodySelection285()&&!(FromControllers(g_state).buttons&kLeftShoulder),"285 stalled publication expires selection lease");
      fc3pad::SetBodyInput285(1,1,0);ObserveNativePause(true);Check(!(FromControllers(g_state).buttons&kLeftShoulder),"285 pause cannot inherit synthetic wheel");ObserveNativePause(false);
      fc3pad::SetBodyInput285(0);c={};c.active=true;fc3pad::Publish(c);fc3pad::Publish(c);
      c.leftGrip=1;c.y=true;fc3pad::Publish(c);c.y=false;fc3pad::Publish(c);
      fc3pad::SetBodyInput285(3,.6f,.8f);auto pointed=FromControllers(g_state);fc3pad::ApplyLookOverride(pointed);
      Check((pointed.buttons&kLeftShoulder)&&pointed.thumbRX==Axis(.6f)&&pointed.thumbRY==Axis(.8f),"285 pointing feeds existing native wheel");
      c.rightX=-.9f;c.rightY=-.4f;fc3pad::Publish(c);pointed=FromControllers(g_state);Check(pointed.thumbRX==Axis(-.9f)&&pointed.thumbRY==Axis(-.4f),"285 explicit right stick outranks pointing");
      fc3pad::SetBodyInput285(0);c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(false);
      fc3pad::SetGameplayControls(true);c.rightGrip=1;fc3pad::Publish(c);fc3pad::SetBodyInput285(4);
      Check(fc3pad::BodyGesture285()&&fc3pad::Latest().rightGrip==0&&fc3pad::BodyControls285().rightGrip==1,"285 body grab hides squeeze from reload handlers while retaining tracked producer controls");
      fc3pad::SetBodyInput285(0);Check(fc3pad::Latest().rightGrip==0,"285 cancelling body grab cannot transfer a held squeeze into shell pickup");
      c.rightGrip=0;fc3pad::Publish(c);c.rightGrip=1;fc3pad::Publish(c);Check(fc3pad::Latest().rightGrip==1,"285 fresh squeeze restores existing weapon grip after release");
      c.rightGrip=0;fc3pad::Publish(c);fc3pad::SetGameplayControls(false);
      fc3pad::SetGameplayControls(true);fc3pad::SetBodyInput285(1,1,0);c.rightTrigger=1;fc3pad::Publish(c);fc3pad::SetBodyInput285(0);
      Check(fc3pad::Latest().rightTrigger==0&&fc3pad::FireOutput()==0&&fc3pad::BodyControls285().rightTrigger==1,"285 held trigger after selection cannot fire or falsely advance weapon handlers");
      c.rightTrigger=0;fc3pad::Publish(c);c.rightTrigger=1;fc3pad::Publish(c);Check(fc3pad::Latest().rightTrigger==1&&fc3pad::FireOutput()==255,"285 released then repulled trigger restores ordinary firing");
      c.rightTrigger=0;fc3pad::Publish(c);fc3pad::SetGameplayControls(false);
      puts("PASS:285 production pad selection/release, aim cancellation, native action/vehicle/menu/stale guards and pointed wheel with stick priority");
    }
 TestUi33();
    fc3pad::Controllers c={};c.active=true;c.rightX=.8f;c.rightY=-.6f;
    fc3pad::SetLookOverride(true,0,0);
    PadState p=FromControllers(c);fc3pad::ApplyLookOverride(p);
    Check(p.thumbRX==0&&p.thumbRY==0,"normal gameplay stick still suppressed for native VR aim");
    // All cardinal and diagonal selections must reach the native virtual report.
    const float axes[8][2]={{1,0},{-1,0},{0,1},{0,-1},{.7f,.7f},{-.7f,.7f},{.7f,-.7f},{-.7f,-.7f}};
    for(bool shotgun:{false,true})for(bool support:{false,true})for(const auto& a:axes)
    {
        fc3pad::SetShotgunHandling(shotgun);fc3pad::SuppressLeftGrip(support);
        fc3pad::SetManualReload(shotgun,shotgun);
        c.y=true;c.leftGrip=1;c.rightX=a[0];c.rightY=a[1];
        p=FromControllers(c);fc3pad::ApplyLookOverride(p);
        Check((p.buttons&kLeftShoulder)&&!(p.buttons&kY),"wheel chord owns LB even over support/pump/reload grip, without native Y healing");
        Check(p.thumbRX==Axis(a[0])&&p.thumbRY==Axis(a[1]),"all wheel directions survive look override and alignment ownership");
        fc3pad::Publish(c);float x=9,y=9;fc3pad::LookStick(x,y);
        Check(x==0&&y==0,"wheel selection cannot turn VR body");
    }
    fc3pad::SetManualReload(false,false);fc3pad::SetShotgunHandling(false);fc3pad::SuppressLeftGrip(false);
    c.y=false;c.leftGrip=0;c.rightX=.8f;c.rightY=-.6f;fc3pad::Publish(c);
    p=FromControllers(c);fc3pad::ApplyLookOverride(p);
    float x=0,y=0;fc3pad::LookStick(x,y);
    Check(!(p.buttons&kLeftShoulder)&&p.thumbRX==0&&x==.8f&&y==-.6f,"release closes native wheel and restores VR turning without native camera look");
    c.leftGrip=1;p=FromControllers(c);fc3pad::ApplyLookOverride(p);
    Check(!(p.buttons&kLeftShoulder),"grip alone never opens wheel even outside dedicated gameplay layout");
    c.leftGrip=0;c.y=true;p=FromControllers(c);
    Check((p.buttons&kY)&&!(p.buttons&kLeftShoulder),"Y alone keeps native previous weapon and heal");
    c.menu=true;c.leftGrip=1;p=FromControllers(c);
    Check((p.buttons&kBack)&&!(p.buttons&kLeftShoulder),"map menu combo does not open wheel");
    c={};fc3pad::Publish(c);p=FromControllers(c);
    Check(p.buttons==0,"tracking/input loss releases wheel");
    fc3pad::SetMagazineHandling(true,false);c={};c.active=true;c.x=true;c.rightTrigger=1;
    p=FromControllers(c);Check((p.buttons&kX)&&p.rightTrigger==255,"pistol ordinary X remains interaction input while native reload gate owns denial");
    c.leftGrip=1;p=FromControllers(c);Check(!(p.buttons&(kX|kLeftShoulder)),"magazine pickup chord consumes X and support LB");
    c.y=true;c.rightX=.7f;p=FromControllers(c);fc3pad::ApplyLookOverride(p);Check((p.buttons&kLeftShoulder)&&p.thumbRX==Axis(.7f),"pistol physical handling still permits explicit wheel selection");
    fc3pad::Publish(c);c.y=false;fc3pad::Publish(c);p=FromControllers(c);fc3pad::ApplyLookOverride(p);
    Check((p.buttons&kLeftShoulder)&&!(p.buttons&kY)&&p.thumbRX==Axis(.7f),"tap Y then hold grip keeps wheel open over magazine ownership");
    c.leftGrip=0;fc3pad::Publish(c);p=FromControllers(c);Check(!(p.buttons&kLeftShoulder),"grip release closes latched wheel");
    c.leftGrip=1;fc3pad::SetMagazineHandling(true,true);p=FromControllers(c);Check(p.rightTrigger==0,"ejected/held/inserting magazine blocks fire");
    fc3pad::RequestReload();p=FromControllers(c);Check(p.buttons&kX,"only generated contact pulse reaches native reload through owned pickup chord");fc3pad::CancelReloadRequest();
    fc3pad::SetMagazineHandling(false);p=FromControllers(c);Check(!(p.buttons&kX)&&p.rightTrigger==255,"pickup combination remains consumed across weapon switch");
    c={};c.active=true;c.a=c.b=c.x=c.y=c.menu=true;c.leftGrip=c.rightGrip=c.leftTrigger=c.rightTrigger=1;c.leftX=c.leftY=c.rightX=c.rightY=.9f;
    fc3pad::SetPistolCalibration(true);p=FromControllers(c);fc3pad::ApplyLookOverride(p);
    Check(!p.buttons&&!p.leftTrigger&&!p.rightTrigger&&!p.thumbLX&&!p.thumbLY&&!p.thumbRX&&!p.thumbRY,"pistol guide menu consumes movement, wheel and combat input");
    g_padReady=true; // test the physical/ViGEm path, not the no-pad slot-0 fallback
    g_oGetState=RealPad;PadReading real={};Check(HookGetState(0,&real)==ERROR_SUCCESS&&real.packet==17&&!real.pad.buttons&&!real.pad.rightTrigger&&!real.pad.thumbLX&&fc3aim::aimCalls==0&&fc3aim::suspends>0,"physical XInput pad neutralized and native aim seam suspended in menu");
    g_padReady=false;
    fc3pad::SetPistolCalibration(false);c.menu=c.y=false;p=FromControllers(c);Check(p.rightTrigger==255&&p.thumbLX!=0,"menu exit restores combat and movement");

    fc3pad::GuardMenuExit();p=FromControllers(c);Check(!p.rightTrigger,"held trigger cannot leak from closing menu");fc3pad::Controllers released={};fc3pad::Publish(released);Check(!fc3pad::GameplayInputBlocked(),"release clears exit guard");
    fc3pad::SetMagazineHandling(true,false);fc3pad::SuppressLeftGrip(false);
    c={};c.active=true;c.rightX=.8f;
    for(int cycle=0;cycle<100;++cycle)
    {
        c.leftGrip=0;fc3pad::Publish(c);p=FromControllers(c);
        Check(!(p.buttons&kLeftShoulder),"every grip release emits native LB release");
        c.leftGrip=1;fc3pad::Publish(c);p=FromControllers(c);fc3pad::ApplyLookOverride(p);
        Check(!(p.buttons&kLeftShoulder),"repeat support grip never opens wheel without Y");
        float wx=1,wy=1;fc3pad::LookStick(wx,wy);Check(wx==.8f,"support grip does not steal body turning");
    }
    fc3pad::SuppressLeftGrip(true);Check(!(FromControllers(c).buttons&kLeftShoulder),"actual two-hand support still consumes wheel grip");
    fc3pad::SuppressLeftGrip(false);c.x=true;Check(!(FromControllers(c).buttons&kLeftShoulder),"ammo pickup still consumes grip X");
    c.x=false;fc3pad::SetMagazineHandling(true,true);Check(!(FromControllers(c).buttons&kLeftShoulder),"active physical magazine still owns left grip");
    c.y=true;Check(FromControllers(c).buttons&kLeftShoulder,"explicit grip Y can open wheel during physical ownership");
    c.leftGrip=0;c.y=false;fc3pad::Publish(c);fc3pad::SetMagazineHandling(false);
    fc3pad::SetShotgunHandling(true);fc3pad::SetManualReload(true,false);fc3pad::SetOneShellX(true);
    c={};c.active=true;fc3pad::Publish(c);c.x=true;
    for(int hold=0;hold<120;++hold){fc3pad::Publish(c);Check(FromControllers(g_state).buttons&kX,"manual pump shotgun holds X for mission interaction");}
    c.x=false;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kX),"interaction releases X normally");
    c.leftGrip=1;c.x=true;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kX),"physical shell pickup chord remains consumed");
    c.leftGrip=0;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kX),"owned pickup press cannot leak when grip releases");
    c.x=false;fc3pad::Publish(c);c.x=true;fc3pad::Publish(c);Check(FromControllers(g_state).buttons&kX,"fresh X after pickup returns to interaction");
    fc3pad::SetManualReload(true,true);fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kX),"alignment X remains calibration-only");
    fc3pad::RequestReload();Check(FromControllers(g_state).buttons&kX,"physical insertion still generates X through ownership");fc3pad::CancelReloadRequest();
    c.x=false;fc3pad::Publish(c);fc3pad::SetManualReload(false,false);fc3pad::SetShotgunHandling(false);fc3pad::SetOneShellX(false);
    puts("PASS: manual shotgun mission hold-X, release, pickup-chord ownership, alignment and insertion pulse");
    std::puts("PASS: 100 repeat grip-only support cycles cannot open wheel, including non-gameplay context.");
    std::puts("PASS: pistol guide menu isolates all pad axes/buttons/triggers and restores normal input.");
    std::puts("PASS: physical magazine interaction/pickup X ownership, fire inhibition, contact pulse, wheel selection and switch restoration.");
    std::puts("PASS: actual pad report eight-direction wheel routing, shotgun/support/alignment ownership, no body turn, release restoration, Y heal and map preserved, tracking loss.");
    TestControls20();
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(true);
    fc3pad::SetMagazineHandling(true,true);fc3pad::SetXOwned(true);fc3pad::SetLookOverride(true,.9f,-.9f);
    ObserveNativePause(true);
    c.b=c.x=true;c.leftGrip=c.rightGrip=1;c.leftTrigger=c.rightTrigger=1;c.rightX=.6f;c.rightY=.8f;
    fc3pad::Publish(c);p=FromControllers(g_state);fc3pad::ApplyLookOverride(p);
    Check((p.buttons&kB)&&(p.buttons&kX)&&!(p.buttons&kLeftShoulder),"native map/menu B is Back and X is UI X despite physical weapon ownership");
    Check(p.thumbRX==Axis(.6f)&&p.thumbRY==Axis(.8f),"native map gets both cursor axes without aim override or crouch");
    Check(fc3pad::GameplayInputBlocked()&&!fc3pad::Latest().active&&!fc3pad::Latest().x,"native menus block physical reload state consumers while preserving UI report");
    fc3pad::RequestReload();Check(!g_reloadPulseUntil,"no physical insertion pulse queued behind native menu");
    fc3aim::aimCalls=0;g_oGetState=RealPad;PadReading menuReading={};HookGetState(0,&menuReading);
    Check(!fc3aim::aimCalls,"native aim cannot write camera while menu owns controls");
    fc3pad::SetSwapBX(true);c={};c.active=true;fc3pad::Publish(c);c.x=true;fc3pad::Publish(c);
    Check(FromControllers(g_state).buttons&kB,"menu B/X swap applied once to hardware X");
    fc3pad::SetSwapBX(false);c={};c.active=true;fc3pad::Publish(c);
    c.x=true;fc3pad::Publish(c);ObserveNativePause(false);
    Check(fc3pad::GameplayInputBlocked()&&!fc3pad::Latest().x&&!FromControllers(g_state).buttons,"held menu X cannot eject on native UI close");
    c={};c.active=true;fc3pad::Publish(c);Check(!fc3pad::GameplayInputBlocked(),"native UI exit rearms only after neutral controls");
    fc3pad::SetMagazineHandling(false);fc3pad::SetXOwned(false);fc3pad::SetLookOverride(true,0,0);
    c.menu=true;fc3pad::Publish(c);Check(fc3pad::NativeMenuOpen()&&!fc3pad::Latest().active&&(FromControllers(g_state).buttons&kStart),"pause opening blocks same-frame physical actions and still sends Start");
    ObserveNativePause(true);c.menu=false;fc3pad::Publish(c);ObserveNativePause(false);fc3pad::Publish(c);
    puts("PASS: native pause/map UI B, X, cursor, physical-action isolation, remap and neutral exit");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(true);
    c.leftGrip=1;c.y=true;c.leftTrigger=0;c.rightTrigger=1;fc3pad::Publish(c);
    c.y=false;
    for(float grip:{.51f,.49f,.3f,.48f,.26f}){c.leftGrip=grip;fc3pad::Publish(c);p=FromControllers(fc3pad::Latest());Check((p.buttons&kLeftShoulder)&&!p.leftTrigger&&!p.rightTrigger,"wheel tolerates grip-pressure changes and excludes aim/fire");}
    c.leftGrip=.24f;c.leftTrigger=0;fc3pad::Publish(c);Check(!(FromControllers(fc3pad::Latest()).buttons&kLeftShoulder),"deliberate release of both wheel bindings closes wheel");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetSwapBX(true);fc3pad::Publish(c);
    c.x=true;fc3pad::Publish(c);Check(fc3pad::Latest().b&&!fc3pad::Latest().x&&!fc3pad::InteractionOnly(),"swapped hardware X is semantic B, never interaction or ammo X");
    c.x=false;c.b=true;fc3pad::Publish(c);Check(fc3pad::Latest().x&&!fc3pad::Latest().b&&fc3pad::InteractionOnly()&&!fc3pad::AmmoControls().x,"swapped hardware B alone interacts without physical ammo input");
    fc3pad::SetSwapBX(false);fc3pad::Publish(c);Check(fc3pad::GameplayInputBlocked()&&!FromControllers(fc3pad::Latest()).buttons,"changing swap with held buttons requires neutral release");
    c={};c.active=true;fc3pad::Publish(c);Check(!fc3pad::GameplayInputBlocked(),"neutral release rearms swapped controls");
    fc3pad::SetGameplayControls(false);
    puts("PASS: controls21 grip hysteresis, wheel trigger isolation, B/X normalization and remap release guard");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(true);
    fc3pad::SetGripContext(true,true);c.leftGrip=1;fc3pad::Publish(c);
    Check(!fc3pad::WheelOwnsGrip()&&fc3pad::Latest().leftGrip==1,"near-weapon squeeze belongs to physical weapon");
    fc3pad::SetGripContext(true,false);fc3pad::Publish(c);Check(!fc3pad::WheelOwnsGrip(),"moving away while holding cannot open wheel");
    c.leftGrip=0;fc3pad::Publish(c);c.leftGrip=1;fc3pad::Publish(c);
    Check(fc3pad::WheelOwnsGrip()&&(FromControllers(fc3pad::Latest()).buttons&kLeftShoulder),"new free-space squeeze opens wheel without Y");
    Check(fc3pad::Latest().leftGrip==0,"wheel grip is hidden from every physical reload consumer");
    fc3pad::SetGripContext(true,true);fc3pad::Publish(c);Check(fc3pad::WheelOwnsGrip(),"moving into weapon cannot steal held wheel");
    fc3pad::SetGameplayControls(false);Check(fc3pad::WheelOwnsGrip()&&fc3pad::DedicatedInteractionLayout(),"held contextual wheel survives inventory render context change");
    c.leftGrip=0;fc3pad::Publish(c);c.leftGrip=1;fc3pad::Publish(c);Check(!fc3pad::WheelOwnsGrip(),"next near-weapon squeeze returns to physical grip");
    c.y=true;fc3pad::Publish(c);Check(fc3pad::WheelOwnsGrip(),"Y explicitly overrides a weapon-owned squeeze");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGripContext(false,false);c.leftGrip=1;fc3pad::Publish(c);Check(!fc3pad::WheelOwnsGrip(),"unknown geometry reserves physical grip");
    c.leftGrip=0;fc3pad::Publish(c);fc3pad::SetGripContext(true,false);g_gripContextAt=static_cast<LONG>(GetTickCount()-151);c.leftGrip=1;fc3pad::Publish(c);Check(!fc3pad::WheelOwnsGrip(),"stale geometry cannot open wheel");
    c.leftGrip=0;fc3pad::Publish(c);fc3pad::SetGripContext(true,false);c.leftGrip=1;c.x=true;fc3pad::Publish(c);Check(!fc3pad::WheelOwnsGrip(),"explicit reload X chord outranks contextual wheel");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGripContext(false,true);fc3pad::SetGameplayControls(false);
    puts("PASS: controls23 per-squeeze weapon/wheel ownership, no mid-hold transfers, physical input isolation, Y override, reload priority and stale-context rejection");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(true);ObserveNativeStance(0);fc3pad::Publish(c);
    c.leftTrigger=1;fc3pad::Publish(c);Check(!fc3pad::AimOutput()&&g_state.leftTrigger==1&&fc3pad::Latest().leftTrigger==1&&!fc3pad::WheelOwnsGrip(),"LT alone does not open inventory; previous native ADS suppression remains");
    c.leftTrigger=0;fc3pad::Publish(c);c.rightY=-1;fc3pad::Publish(c);Check(FromControllers(g_state).buttons&kB,"down requests crouch");c.rightY=0;fc3pad::Publish(c);
    ObserveNativeStance(1);c.rightY=-1;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kB),"repeated down cannot stand");c.rightY=0;fc3pad::Publish(c);
    c.rightY=1;fc3pad::Publish(c);Check(FromControllers(g_state).buttons&kB,"up requests stand after crouch");c.rightY=0;fc3pad::Publish(c);
    ObserveNativeStance(0);c.rightY=1;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kB),"repeated up cannot crouch");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(false);
    puts("PASS: controls24 LT ADS isolation and down-crouch/up-stand transitions without repeated toggles");
    TestAmmo27();
    {
     fc3pad::SetGameplayControls(true);fc3pad::SetGripOnlyAmmo(true);
     fc3pad::Controllers t={};t.active=true;fc3pad::Publish(t);fc3pad::SuppressLeftGrip(true);
     for(bool supported:{false,true}){
      fc3pad::SetTwoHandWeaponHold(supported);
      for(float trigger:{1.f,.5f,0.f,1.f}){t.leftGrip=1;t.leftTrigger=trigger;fc3pad::Publish(t);Check(!fc3pad::WheelOwnsGrip()&&!(FromControllers(g_state).buttons&kLeftShoulder),"grip LT cannot request inventory at any pressure or support state");}
      t.x=true;fc3pad::Publish(t);Check(fc3pad::AmmoControls().x,"LT cannot consume the physical ammo chord");t.x=false;fc3pad::Publish(t);
      t.y=true;fc3pad::Publish(t);Check(FromControllers(g_state).buttons&kLeftShoulder,"deliberate grip Y remains the wheel override even with LT/support");
      t={};t.active=true;fc3pad::Publish(t);
     }
     fc3pad::SetTwoHandWeaponHold(false);
     t.leftStickClick=true;fc3pad::Publish(t);Check(FromControllers(g_state).buttons&kLeftThumb,"L3 retains sprint");t.leftStickClick=false;
     ObserveNativePause(true);t.leftGrip=t.leftTrigger=1;fc3pad::Publish(t);Check(!fc3pad::WheelOwnsGrip(),"native menu LT does not open gameplay wheel");
     ObserveNativePause(false);fc3pad::Publish(t);Check(!fc3pad::WheelOwnsGrip(),"held grip LT cannot open wheel after menu exit");
     t={};t.active=true;fc3pad::Publish(t);fc3pad::SuppressLeftGrip(false);fc3pad::SetGripOnlyAmmo(false);fc3pad::SetGameplayControls(false);
     fc3pad::SetScopeFocus61(true);t={};t.active=true;t.leftStickClick=true;t.leftTrigger=1;fc3pad::Publish(t);Check(!(FromControllers(g_state).buttons&kLeftThumb),"L3 cannot sprint while focused LT scope owns it");t.leftTrigger=0;fc3pad::Publish(t);Check(FromControllers(g_state).buttons&kLeftThumb,"L3 still sprints outside LT focus");fc3pad::SetScopeFocus61(false);t={};t.active=true;fc3pad::Publish(t);
     puts("PASS: controls59 LT inventory binding removed; grip Y override, ammo ownership, supported poses, menu exit and L3 retained");
    }

    fc3pad::SetLeverHandling(true);c={};c.active=true;c.rightGrip=1;c.a=true;fc3pad::Publish(c);
    Check(FromControllers(g_state).buttons&kA,"1887 grip-only shell pickup does not consume native A");
    fc3pad::SetLeverHandling(false);c={};c.active=true;fc3pad::Publish(c);
    puts("PASS: 1887 physical shell grip preserves native A");

    fc3pad::SetGripOnlyAmmo(true);fc3pad::SetGameplayControls(true);
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGripContext(true,false);
    c.leftGrip=1;fc3pad::Publish(c);
    Check(!fc3pad::WheelOwnsGrip()&&fc3pad::Latest().leftGrip==1,"grip-only pickup owns free-space squeeze instead of wheel");
    Check(!(FromControllers(g_state).buttons&kX),"grip-only ammo does not send native interaction/reload X");
    for(int i=0;i<120;++i){fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kX),"held ammo grip cannot leak X");}
    c.y=true;fc3pad::Publish(c);Check(fc3pad::WheelOwnsGrip()&&fc3pad::Latest().leftGrip==0,"explicit grip Y still opens wheel and hides physical grip");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGripOnlyAmmo(false);fc3pad::SetGameplayControls(false);
    puts("PASS: offline28 grip-only pickup/wheel separation, no native X, held grip isolation and explicit grip Y override");


 TestVehicleMachete128();

}


