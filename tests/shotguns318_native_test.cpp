#define main ExistingAmmoSuite318
#include "ammo_probe_test.cpp"
#undef main
extern void SetRenderHands303(int,bool,unsigned);
extern void PublishLeverSupport340(int,bool,bool);
extern void PublishSidearmControls315(float,float,float=0,float=0,bool=false,bool=false);
namespace native318 {
int begins=0,fills=0,ends=0;
void __stdcall Begin(UINT s,UINT w){++begins;PistolFixtureBegin(s,w);}
void __stdcall Fill(UINT,UINT w,UINT flags,int amount){++fills;Check(flags==0&&amount==-1,"318 original native shell ABI");const UINT gun=ReloadRead(w,0x48),before=ReloadRead(gun,0xcc),reserve=ReloadRead(g_pistolFixtureHud,0x140);if(!reserve)return;*reinterpret_cast<UINT*>(gun+0xcc)=before+1;*reinterpret_cast<UINT*>(g_pistolFixtureHud+0x134)=before+1;*reinterpret_cast<UINT*>(g_pistolFixtureHud+0x140)=reserve-1;}
void __stdcall End(UINT s,UINT w,UINT f){++ends;PistolFixtureEnd(s,w,f);}
}
void TestManualOwners318(ID3D11Device* device,ID3D11DeviceContext* context)
{
    const LONG priorFrame=g_frame;const bool priorMotion=g_motionGun,priorEnabled=g_singleShellEnabled;
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,32768,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(page&&code,"single shell native fixture allocation");
    memcpy(code,kNativeReloadCountStore,sizeof(kNativeReloadCountStore));code[sizeof(kNativeReloadCountStore)]=0xc3;DWORD protection=0;
    Check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&protection)!=0,"single-shell real native count store code");FlushInstructionCache(GetCurrentProcess(),code,8);
    const auto init=MH_Initialize();Check(init==MH_OK||init==MH_ERROR_ALREADY_INITIALIZED,"single-shell hook library");
    Check(MH_CreateHook(code,reinterpret_cast<void*>(ReloadCountStoreBoundary),&g_nativeReloadCountStore)==MH_OK&&MH_EnableHook(code)==MH_OK,"actual count-store MinHook for one-shell mode");
    g_traceFixtureStore=code;
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192;
    const UINT weapon=actor+12288,hud=actor+16384,property=actor+20480,reference=actor+24576,manager=reference+256;
    *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=actor;*reinterpret_cast<UINT*>(reference+4)=0x5544;
    *reinterpret_cast<UINT*>(manager)=0x123456;*reinterpret_cast<UINT*>(actor+0x24)=component;
    *reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
    g_traceFixtureProperty=property;g_singleFixtureActor=actor;g_reloadFixtureWeapon=weapon;g_reloadFixtureHud=hud;
    g_singleReloadAuthority=FixtureSingleAuthority;g_singleReloadManager=manager;
    g_nativeReloadBegin=FixtureSingleBegin;g_nativeReloadEnd=FixtureReloadEnd;g_nativeReload=FixtureSingleTransfer;
    g_frame=80000;g_motionGun=true;g_singleShellEnabled=true;g_physicalReloadEnabled=false;

    TestManualInsertion(state,wrapper,weapon,hud,device,context);
    Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"318 count-store cleanup");if(init==MH_OK)MH_Uninitialize();
    g_nativeReloadCountStore=nullptr;g_traceFixtureStore=nullptr;g_singleReloadAuthority=nullptr;g_singleReloadManager=0;g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReload=nullptr;
    g_reload={};g_reloadPose={};g_reloadCountTrace={};g_frame=priorFrame;g_motionGun=priorMotion;g_singleShellEnabled=priorEnabled;
    VirtualFree(page,0,MEM_RELEASE);VirtualFree(code,0,MEM_RELEASE);
}

int main(){
 g_fixtureClock171=1000000;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_reloadAlignment=false;g_machete.held=false;g_inGameplay=g_enabled=true;
 fc3pad::SetGameplayControls(true);fc3pad::SetGripOnlyAmmo(true);fc3pad::SetSwapBX(false);fc3pad::SetGripContext(true,true);
 NativeAmmoFixture219 native;g_pistolFixtureHud=UINT(native.page)+16384;g_nativeReloadBegin=native318::Begin;g_nativeReload=native318::Fill;g_nativeReloadEnd=native318::End;
 for(int owner:{0,1})for(int profile:{0,34})for(int loaded:{0,2,8}){
  printf("NATIVE318 profile%d owner%d loaded%d\n",profile,owner,loaded);g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_reload={};g_reloadSignal={};g_pistolReload={};g_spas={};g_spas.clip=-1;g_spas.pending=true;g_spas.deadline=GetTickCount64()+3000;g_lever={};g_lever.enabled=g_lever.hold=g_lever.leverOpen=g_lever.pending=true;g_lever.clip=-1;g_lever.deadline=GetTickCount64()+3000;
  const UINT hud[5]={UINT(loaded),8,0,20,UINT(loaded)+20};memcpy(reinterpret_cast<void*>(g_pistolFixtureHud+0x134),hud,sizeof(hud));native.count()=loaded;*reinterpret_cast<UINT*>(native.state+8)=0;g_reload.widgets[0].wrapper=LONG(native.wrapper);g_reload.widgets[0].hud=LONG(g_pistolFixtureHud);
  SetRenderHands303(owner,false,g_armPoseEpoch);auto h=fc3hands303::Read();fc3hands303::PublishTransfer307(true,false,false,h.generation,h.entity,g_armPoseEpoch,true);PublishSidearmControls315(0,0);native318::begins=native318::fills=native318::ends=0;
  auto request=[&](){if(profile==0)Lever1887Request(native.state,native.wrapper);else SpasRequest(native.state,native.wrapper);};request();
  const int n=loaded<8?1:0;Check(native318::begins==n&&native318::fills==n&&native318::ends==n&&native.count()==loaded+n&&ReloadRead(g_pistolFixtureHud,0x140)==20-n,"318 one shell only with exact reserve debit and balanced native cleanup;full tube refuses");Check(fc3hands303::Read().hand==owner,"318 reload preserves physical owner");request();Check(native318::fills==n,"318 consumed shell request cannot repeat");if(profile==34&&loaded<8)Check(g_spas.needPump==(loaded==0),"318 SPAS empty-only pump requirement preserved");
  if(profile==0){g_lever.hold=g_lever.leverOpen=g_lever.grabbing=false;g_lever.chambered=true;g_lever.lastTrigger=0;}else g_spas.needPump=false;
  fc3pad::BlockFire(false);PublishSidearmControls315(0,0);PublishSidearmControls315(owner?0:1,owner?1:0);if(profile==0)UpdateLever1887Fire(fc3pad::Latest());else UpdateSpasFire(fc3pad::Latest());Check(fc3pad::FireOutput()>0,"318 actual owning trigger fires after mechanism ready");
  if(profile==0){
   g_twoHand=true;g_lever.simple=false;PublishLeverSupport340(owner,true,false);
   Check(fc3pad::AmmoControls().leftGrip>.9f,"340 actual role/grab resolver preserves1887 support squeeze for reload hold");
   TickLever1887(true);Check(g_lever.hold&&!g_twoHand,"340 fired1887 foregrip enters original temporary reload hold");
   PublishLeverSupport340(owner,true,true);TickLever1887(true);Check(g_lever.hold,"340 active reload keeps temporary holding grip");
   PublishLeverSupport340(owner,false,true);TickLever1887(true);Check(!g_lever.hold,"340 releasing support returns gun to original firing hand");
   fc3grab333::Publish({});
  }
 }
 wchar_t path[MAX_PATH];GetSystemDirectoryW(path,MAX_PATH);wcscat_s(path,L"\\d3d11.dll");auto create=reinterpret_cast<decltype(&D3D11CreateDevice)>(GetProcAddress(LoadLibraryW(path),"D3D11CreateDevice"));ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;Check(SUCCEEDED(create(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"318 WARP");
 for(int owner:{0,1}){printf("M133_NATIVE318 owner%d\n",owner);g_weaponMechanicProfile=3;g_weaponMechanicFrame=g_frame;fc3pad::SetGripOnlyAmmo(false);fc3pad::SetMagazineHandling(false,false);fc3pad::BlockFire(false);SetRenderHands303(owner,false,g_armPoseEpoch);PublishSidearmControls315(0,0);fc3pad::SetGripContext(false,false);fc3pad::SetGripOnlyAmmo(false);fc3pad::SetXOwned(false);TestManualOwners318(device,context);}
 context->ClearState();context->Release();device->Release();puts("PASS:318 SPAS/1887 both owners empty/partial/full single shell native callbacks,cleanup,reserve,pump gate,owning trigger;M133 real hooked count-store suite both owners;simulated native memory,no game launched");
}


