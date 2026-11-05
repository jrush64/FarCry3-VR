#include "options_pointer299.h"
volatile LONG g_optionsToggle299=0;bool g_optionsClose299=false;fc3ui::RayState298 g_optionsPointer299;
void ShopUnlockMenu() // INSERT: shop unlock (SPAS-12 etc.): the game's g_UnlockAllShopItems switch
{
 if(ImGui::Button("Unlock all shop weapons"))RequestShopUnlock(true);
 ImGui::SameLine();if(ImGui::Button("Normal shop"))RequestShopUnlock(false);
 const LONG r=g_shopUnlockResult;
 ImGui::SameLine();ImGui::TextUnformatted(r==1?"Shop unlocked: every weapon is listed (reopen the shop)":r==2?"Shop normal":r==3?"Not available":ShopUnlocked()?"Shop unlocked":"");
}

// Native mouse UI. ImGui is MIT licensed; see third_party/imgui/LICENSE.txt.
struct WeaponMenuEntry {const char* name;const char* family;};
const WeaponMenuEntry kMenuWeapons[]={
    {"KW1911", "Pistol"},
    {"AK47", "Assault"},
    {"Famas", "Assault"},
    {"GalilAce52", "Assault"},
    {"MK16", "Assault"},
    {"P416", "Assault"},
    {"STGW-90", "Assault"},
    {"BOW", "Bow"},
    {"LPO-50", "Flamethrower"},
    {"MKG", "LMG"},
    {"PKM", "LMG"},
    {"U100", "LMG"},
    {"GM-94", "Launcher"},
    {"RPG-7", "Launcher"},
    {"ChineseSword", "Machete"},
    {"MACHETE", "Machete"},
    {"SHIV", "Machete"},
    {"TANTO", "Machete"},
    {"ULC_TribalKnife", "Machete"},
    {"6P9", "Pistol"},
    {"DesertEagle", "Pistol"},
    {"FLARE_GUN", "Pistol"},
    {"HUNTER", "Pistol"},
    {"A2000", "SMG"},
    {"BZ19", "SMG"},
    {"KSV", "SMG"},
    {"MP5", "SMG"},
    {"VZ61", "SMG"},
    {"1887", "Shotgun"},
    {"M133", "Shotgun"},
    {"Spas12", "Shotgun"},
    {"ULC_Type10FlareGun", "Shotgun"},
    {"M93", "Sniper"},
    {"R700", "Sniper"},
    {"SVD", "Sniper"},
};
const char* MenuWeaponName();
#ifdef FC3_AMMO_TEST
ImVec2 g_menuSliderPos[41]={},g_menuDefaultPos[41]={},g_menuSavePos[41]={};ImVec2 g_menuSaveAllPos={},g_menuCheckPos={},g_menuAmmoCheckPos={},g_menuHideGuidesPos={};
void MenuTestPoint(ImVec2& pos){pos=ImVec2((ImGui::GetItemRectMin().x+ImGui::GetItemRectMax().x)*.5f,(ImGui::GetItemRectMin().y+ImGui::GetItemRectMax().y)*.5f);}
#endif
int g_menuWeapon=0;const wchar_t* g_menuSavePath=nullptr;
const char* MenuWeaponName(){return kMenuWeapons[g_menuWeapon].name;}
#include "guide_visibility44.inl"
ImGuiContext* g_menuContext=nullptr;bool g_menuBackend=false;
HWND g_menuWindow=nullptr;WNDPROC g_menuWndProc=nullptr;
volatile LONG g_menuWheel=0;SRWLOCK g_menuTextLock=SRWLOCK_INIT;wchar_t g_menuText[64]={};UINT g_menuTextCount=0;
ID3D11Texture2D* g_menuTexture=nullptr;ID3D11RenderTargetView* g_menuRtv=nullptr;
UINT g_menuWidth=0,g_menuHeight=0;float g_menuScale=1;int g_menuCrop[4]={};
using CursorFn=BOOL(WINAPI*)(int,int);using ClipFn=BOOL(WINAPI*)(const RECT*);
CursorFn g_menuSetCursor=nullptr;ClipFn g_menuClipCursor=nullptr;
RECT g_menuPreviousClip={};POINT g_menuPreviousCursor={};
BOOL WINAPI MenuSetCursor(int x,int y){return fc3pad::MenuOpen()?TRUE:g_menuSetCursor(x,y);}
BOOL WINAPI MenuClipCursor(const RECT* rect){return fc3pad::MenuOpen()?TRUE:g_menuClipCursor(rect);}
LRESULT CALLBACK InsertWndProc(HWND h,UINT m,WPARAM w,LPARAM l)
{
 if(fc3pad::NativeInputBlocked())
 {
  if(fc3pad::MenuOpen()&&m==WM_CHAR){AcquireSRWLockExclusive(&g_menuTextLock);if(g_menuTextCount<64&&w>=32)g_menuText[g_menuTextCount++]=static_cast<wchar_t>(w);ReleaseSRWLockExclusive(&g_menuTextLock);return 0;}
  if(fc3pad::MenuOpen()&&m==WM_MOUSEWHEEL){InterlockedExchangeAdd(&g_menuWheel,GET_WHEEL_DELTA_WPARAM(w));return 0;}
  if(fc3pad::MenuOpen()&&m==WM_SETCURSOR){SetCursor(LoadCursorW(nullptr,MAKEINTRESOURCEW(32512)));return TRUE;}
  if(m==WM_INPUT)return DefWindowProcW(h,m,w,l); // clean up raw-input allocation
  if((m>=WM_MOUSEFIRST&&m<=WM_MOUSELAST)||(m>=WM_KEYFIRST&&m<=WM_KEYLAST))return 0;
 }
 return g_menuWndProc?CallWindowProcW(g_menuWndProc,h,m,w,l):DefWindowProcW(h,m,w,l);
}
#include "weapons7_menu.inl"
#include "bow_menu.inl"
ULONGLONG g_insertPauseAt=0;
void SetInsertOpen(bool open)
{
 if(g_pistolMenu.open==open)return;
 if(open)for(auto& p:g_magazineStates)CancelMagazinePull286(p);
 g_optionsPointer299=fc3ui::RayState298{};
 if(!open)g_pistolGuidePreview=PistolGuidePreview{};
 g_pistolMenu.open=open;fc3pad::SetPistolCalibration(open);
 if(open)
 {
  {const LONG profile=g_weaponMechanicProfile;const char* name=MacheteHeld()?"MACHETE":profile>=3&&profile<=6?"M133":profile==10?"BOW":profile==15?"LPO-50":profile==16?"FLARE_GUN":profile==18?"GM-94":profile==32?"RPG-7":profile==8?"AK47":profile==7?"A2000":profile==41?"STGW-90":profile==39?"VZ61":profile==1?"6P9":profile==0?"1887":profile==42?"HUNTER":profile==34?"Spas12":profile==28?"MP5":profile==14?"Famas":profile==9?"BZ19":profile==31?"P416":profile==25?"MK16":profile==12?"DesertEagle":profile==21?"KSV":profile==17?"GalilAce52":profile==35?"SVD":profile==2?"R700":profile==36?"U100":profile==22?"MKG":profile==23?"M93":profile==43?"PKM":"KW1911";
   for(int i=0;i<static_cast<int>(sizeof(kMenuWeapons)/sizeof(*kMenuWeapons));++i)if(strcmp(kMenuWeapons[i].name,name)==0)g_menuWeapon=i;
   g_menuGuideFamily=MagazineFamilyOfProfile(profile);}
  GetClipCursor(&g_menuPreviousClip);GetCursorPos(&g_menuPreviousCursor);
  if(g_menuClipCursor)g_menuClipCursor(nullptr);else ClipCursor(nullptr);
  g_insertPauseAt=GetTickCount64();g_w7PausedAt=g_insertPauseAt;
  g_pistolMenu.result=0;
 }
 else
 {
  PauseCalibration136();
  const ULONGLONG paused=g_insertPauseAt?GetTickCount64()-g_insertPauseAt:0;g_insertPauseAt=0;
  g_w7PausedAt=0;W7Resume(paused);if(g_bow.shooting)g_bow.fireAt+=paused;
  ResumeMagazineInsertPause224(paused);
  if(g_pistolReload.pending&&!g_pistolReload.entryAccepted)fc3pad::RequestReload(); // resume one outstanding physical contact
  fc3pad::GuardMenuExit();
  fc3xr::SetOptionsTexture(nullptr);
  if(g_menuClipCursor)g_menuClipCursor(&g_menuPreviousClip);else ClipCursor(&g_menuPreviousClip);
  if(g_menuSetCursor)g_menuSetCursor(g_menuPreviousCursor.x,g_menuPreviousCursor.y);
  g_pistolReload.x=fc3pad::Latest().x;
  g_pistolReload.waitTriggerRelease=true;g_pistolReload.rackArmed=false;
 }
 Log("[INSERT MENU] %s; all gameplay input %s",open?"OPEN":"CLOSED",open?"blocked":"restored");
}
void TickInsertMenu(HWND window)
{
 if(window&&window!=g_menuWindow)
 {
  if(g_menuWindow&&g_menuWndProc&&reinterpret_cast<WNDPROC>(GetWindowLongPtrW(g_menuWindow,GWLP_WNDPROC))==InsertWndProc)
   SetWindowLongPtrW(g_menuWindow,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(g_menuWndProc));
  g_menuWindow=window;g_menuWndProc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(InsertWndProc)));
  if(!g_menuSetCursor)
  {
   if(MH_CreateHook(reinterpret_cast<void*>(&SetCursorPos),reinterpret_cast<void*>(&MenuSetCursor),reinterpret_cast<void**>(&g_menuSetCursor))==MH_OK)MH_EnableHook(reinterpret_cast<void*>(&SetCursorPos));
   if(MH_CreateHook(reinterpret_cast<void*>(&ClipCursor),reinterpret_cast<void*>(&MenuClipCursor),reinterpret_cast<void**>(&g_menuClipCursor))==MH_OK)MH_EnableHook(reinterpret_cast<void*>(&ClipCursor));
  }
 }
 const bool toggle299=InterlockedExchange(&g_optionsToggle299,0)!=0,close299=g_optionsClose299;g_optionsClose299=false;
 if(GetForegroundWindow()==window)
 {
  if(close299)SetInsertOpen(false);else if(toggle299)SetInsertOpen(!g_pistolMenu.open);
  if(KeyPressed(VK_INSERT))SetInsertOpen(!g_pistolMenu.open);
  if(g_pistolMenu.open&&KeyPressed(VK_ESCAPE))SetInsertOpen(false);
 }
}
bool SaveWeaponCalibration53(const wchar_t* path){const bool guides=SavePistolGuides(path);const bool bolt=(g_menuGuideFamily==14||g_menuGuideFamily==17)?SaveR700Settings(path):true;return guides&&bolt;}
#ifdef FC3_AMMO_TEST
ImGuiID g_weaponSaveId53=0;
#endif
void GuideSlider(int n,const char* label)
{
 ImGui::PushID(n);float* value=PistolGuideValue(n);const bool angular=(n>=18&&n<24)||(n>=27&&n<30)||n>=36;float cm=*value*(angular?1.f:100.f);
 const float min=angular?-180.f:n>=33?1.f:n>=30?-50.f:n>=24?-50.f:n>=12?1.f:n<9?-50.f:n==11?1.f:1.5f,max=angular?180.f:n>=33?40.f:n>=30?50.f:n>=24?50.f:n>=12?40.f:n<9?50.f:n==11?20.f:15.f;
 ImGui::SetNextItemWidth((std::max)(55.f*g_menuScale,(std::min)(250.f*g_menuScale,ImGui::GetContentRegionAvail().x-180.f*g_menuScale)));
 if(ImGui::SliderFloat(label,&cm,min,max,angular?"%.1f deg":"%.2f cm")){*value=cm*(angular?1.f:.01f);if(n>=30)g_pistolMenu.target=3;else if(n<24)g_pistolMenu.target=n>=18?(n-18)/3:n>=12?(n-12)/3:n<3||n==9?0:n<6||n==10?1:2;g_pistolMenu.result=0;}

#ifdef FC3_AMMO_TEST
 MenuTestPoint(g_menuSliderPos[n]);
#endif
 ImGui::SameLine();if(ImGui::Button("Default")){*value=PistolGuideDefault(n);g_pistolMenu.result=0;}

#ifdef FC3_AMMO_TEST
 MenuTestPoint(g_menuDefaultPos[n]);
#endif
 ImGui::SameLine();if(ImGui::Button("Save"))g_pistolMenu.result=SavePistolGuideValue(n,g_menuSavePath)?1:2;

#ifdef FC3_AMMO_TEST
 MenuTestPoint(g_menuSavePos[n]);
#endif
 ImGui::PopID();
}
void M133Menu54(){static int result=0;const char* guides[]={"Loading port (blue)","Two-handed holding (yellow)"};ImGui::Combo("Adjust guide",&g_m133GuideTarget56,guides,2);if(g_m133GuideTarget56==1){if(M133SupportMenu56())result=0;}else{const char* axes[]={"Position X","Position Y","Position Z"};for(int a=0;a<3;++a){float v=g_manualSettings.port[a]*100.f;if(ImGui::SliderFloat(axes[a],&v,-50,50,"%.2f cm")){g_manualSettings.port[a]=v*.01f;result=0;}}float size=g_manualSettings.radius*200.f;if(ImGui::SliderFloat("Contact diameter",&size,3.f,30.f,"%.2f cm")){g_manualSettings.radius=size*.005f;result=0;}}
 if(ImGui::CollapsingHeader("Reload mode")){int mode=g_manualSettings.mode;const char* modes[]={"Physical shells","One shell with X","Native bulk reload"};if(ImGui::Combo("Reload mode##M133",&mode,modes,3)){g_manualSettings.mode=mode;ApplyManualReloadMode();result=0;}}
 if(ImGui::Button("Save All - M133")){const bool reload=SaveManualReloadSettings(g_menuSavePath),support=SaveM133Support56(g_menuSavePath);result=reload&&support?1:2;}ImGui::SameLine();ImGui::TextUnformatted(result==1?"Saved":result==2?"SAVE FAILED":"Live edits; save to keep them");ImGui::TextWrapped("Grip the holding box to aim with both hands. Grip at the waist/chest pouch takes a shell; grip + X remains the explicit ammo override. Regular and signature share these settings.");}
#include "machete_collider_menu118.inl"
void DrawInsertContents()
{
 CalibrationMenu136();
 if(ImGui::Button("Close (Insert / Esc)"))SetInsertOpen(false);
 ImGui::SameLine();ImGui::TextDisabled("Gameplay controls disabled while open");
 ImGui::Separator();ImGui::TextUnformatted("Weapon guides");
 bool hideHolding=!g_grabGuides,hideReload=!g_showWeaponReloadGuides;
 if(ImGui::Checkbox("Hide all weapon holding guides",&hideHolding)){
  g_grabGuides=!hideHolding;wchar_t path[MAX_PATH];ManualSettingsPath(path);
  WritePrivateProfileStringW(L"VRGuides",L"GrabGuides",g_grabGuides?L"1":L"0",g_menuSavePath?g_menuSavePath:path);
 }
 if(ImGui::Checkbox("Hide all physical reload guides",&hideReload)){
  g_showWeaponReloadGuides=!hideReload;wchar_t path[MAX_PATH];ManualSettingsPath(path);
  WritePrivateProfileStringW(L"VRGuides",L"ReloadGuides",g_showWeaponReloadGuides?L"1":L"0",g_menuSavePath?g_menuSavePath:path);
 }
#ifdef FC3_AMMO_TEST
 MenuTestPoint(g_menuHideGuidesPos);
#endif
 ImGui::TextDisabled("Holding: red hand sphere, yellow weapon cube. Reloading: yellow hand sphere, blue weapon target.");
 ImGui::TextWrapped("Enabled guides stay visible in Insert. These switches change visibility only; your saved positions and sizes are retained.");
 fc3xr::SetGuideGroupVisibility(g_showWeaponReloadGuides&&PhysicalReloadGuidesEnabled(),g_grabGuides,g_pistolMenu.open);
 ImGui::Separator();
 if(ImGui::Checkbox("God mode",&g_godMode123)){wchar_t path[MAX_PATH];ManualSettingsPath(path);g_godModeSaveFailed123=!SaveGodMode123(g_menuSavePath?g_menuSavePath:path);}
 if(g_godModeSaveFailed123)ImGui::TextDisabled("God mode could not be saved");
 ImGui::TextDisabled("God mode takes priority over infinite health while enabled.");
 ImGui::Checkbox("Infinite health",&g_infiniteHealth);
 ImGui::SameLine();ImGui::TextDisabled(g_healthStatus==1?(g_godMode123?"God mode applied":"Infinite health applied"):(g_godMode123||g_infiniteHealth)?"Waiting for game settings":"Off");
 ScopeMenu60();ThrowableMenu144();
 if(ImGui::Checkbox("Aiming dot (all weapons)",&g_redDot)){wchar_t path[MAX_PATH];ManualSettingsPath(path);g_scopeSaveFailed60=!SaveScopeSettings60(g_menuSavePath?g_menuSavePath:path);}
 ImGui::SetNextItemWidth(180*g_menuScale);
 const char* colours52[]={"Red","Green","Cyan","Blue","Yellow","White","Magenta","Orange"};
 if(ImGui::Combo("Crosshair colour",&g_redDotColour52,colours52,8)){wchar_t path[MAX_PATH];ManualSettingsPath(path);g_redDotColourSaveFailed52=!SaveReticleColour52(g_menuSavePath?g_menuSavePath:path);}
 const char* laserModes194[]={"Off","Surface dot","Laser line","Line + surface dot"};
 ImGui::SameLine();ImGui::SetNextItemWidth(160*g_menuScale);
 if(ImGui::Combo("Laser sight",&g_laserMode194,laserModes194,4)){fc3aim::SetLaserEnabled194(g_laserMode194!=0);wchar_t path[MAX_PATH];ManualSettingsPath(path);g_laserSaveFailed194=!SaveLaserMode194(g_menuSavePath?g_menuSavePath:path);}
 ImGui::SetNextItemWidth(180*g_menuScale);
 if(ImGui::SliderInt("Bullet spread",&g_spreadPercent206,0,100,"%d%%")){fc3aim::SetSpreadPercent206(g_spreadPercent206);wchar_t path[MAX_PATH];ManualSettingsPath(path);g_spreadSaveFailed206=!SaveSpreadPercent206(g_menuSavePath?g_menuSavePath:path);}
 ImGui::SameLine();ImGui::TextDisabled("0 = shots exactly on the laser / sight dot. Shotguns keep their pattern.");
 if(g_redDotColourSaveFailed52)ImGui::TextDisabled("Colour could not be saved");
 if(g_laserSaveFailed194)ImGui::TextDisabled("Laser mode could not be saved");
 if(g_spreadSaveFailed206)ImGui::TextDisabled("Bullet spread could not be saved");
 ImGui::Separator();
 if(ImGui::CollapsingHeader("Motion controls")){
  ImGui::Checkbox("Automatic pistol support grab",&g_autoPistolSupport);
  ImGui::Checkbox("Automatic long-gun support grab",&g_autoLongGunSupport);
  ImGui::TextWrapped("Auto: move your free hand near the support point to grab; move away to release. Manual: hold left grip near the support point; release grip to let go. Reloads take priority. Special weapons with dedicated loading-hand controls retain those controls.");
  bool gripOnly=fc3pad::GripOnlyAmmoEnabled();
  if(ImGui::Checkbox("Grip-only ammo pickup",&gripOnly))fc3pad::SetGripOnlyAmmo(gripOnly);
  ImGui::TextWrapped("Grip-only: free grip at the waist/chest pouch takes replacement ammo when ready. Handle, support and reload grabs keep their own action until release. Left grip + Y opens the weapon wheel; release left grip to select. LT does not open inventory. Left grip + X releases a partial magazine away from its removal target. 1887 retains its hand-switched right-hand loading gestures.");
  ImGui::Checkbox("VR native-menu mouse cursor",&g_nativeUiMouseEnabled);
  ImGui::Checkbox("Point controller at native menus",&g_nativeUiRayEnabled298);
  ImGui::TextWrapped("Settings: RIGHT grip + Menu. Point/trigger in settings; right stick up/down scrolls. Map: LEFT grip + Menu. Point the right controller and squeeze trigger to click. Right stick moves the cursor instead; aim elsewhere on the panel to resume pointing. Right-stick click also clicks; A remains native confirm. Hold right grip with the trigger released for native trigger map zoom. Back follows your B/X swap setting.");
  bool swap=fc3pad::SwapBXEnabled();
  if(ImGui::Checkbox("Swap Quest B and X",&swap))fc3pad::SetSwapBX(swap);
  if(!gripOnly)ImGui::TextWrapped(swap?"B interacts. LEFT grip + B: release partial magazine or take replacement when empty. Both grips + B: pickup override. Native menu Back is X.":"X interacts. LEFT grip + X: release partial magazine or take replacement when empty. Both grips + X: pickup override. Native menu Back is B.");
  ImGui::SliderFloat("Two-hand recoil aim assist",&g_twoHandRecoilAssist,0.0f,1.0f,"%.2f");
  ImGui::SliderFloat("One-hand rifle recoil aim assist",&g_rifleRecoilAssist,0.0f,1.0f,"%.2f");
  ImGui::TextWrapped("0 = original aim; 1 = maximum reduction of animated rifle kick while firing. Gun animation and bullet spread remain. Does not smooth headset movement.");
  bool stickThrottle365=fc3pad::StickThrottle365();
  if(ImGui::Checkbox("Left stick up = gas, down = brake/reverse (cars, jetski, ATV)",&stickThrottle365))fc3pad::SetStickThrottle365(stickThrottle365);
  bool carWheelEnabled=fc3pad::CarWheelSteeringEnabled();
  if(ImGui::Checkbox("Car wheel grip steering (353)",&carWheelEnabled))fc3pad::SetCarWheelSteeringEnabled(carWheelEnabled);
  float carWheelDegrees=fc3pad::CarWheelFullLockDegrees();
  float carSensitivity363=90.f/carWheelDegrees;
  if(ImGui::SliderFloat("Car steering sensitivity",&carSensitivity363,.25f,3.f,"%.2fx")){fc3pad::SetCarWheelFullLockDegrees(90.f/carSensitivity363);carWheelDegrees=fc3pad::CarWheelFullLockDegrees();}
  if(ImGui::SliderFloat("Car wheel turn for full steering",&carWheelDegrees,30.f,360.f,"%.0f degrees"))fc3pad::SetCarWheelFullLockDegrees(carWheelDegrees);
  ImGui::TextWrapped("Car hand placement: automatic closed-grip fit with forearm twist correction.");
  float carSmooth363=fc3pad::VehicleSmoothing363(false);if(ImGui::SliderFloat("Car steering smoothing",&carSmooth363,0.f,150.f,"%.0f ms"))fc3pad::SetVehicleSmoothing363(false,carSmooth363);
  ImGui::TextWrapped("Higher sensitivity steers more with the same hand movement. More degrees for full steering makes it gentler. Smoothing removes small hand jitter; start around 45 ms. Hands remain attached to the visible rim while gripping. Release and regrip after closing the menu.");
  ImGui::TextWrapped("Cars: both hands follow your controllers automatically. Hold either or both grips near the visible wheel rim to grab; move around the axle to steer. Release to let go; regrip anywhere without resetting. Start at 90 degrees for full steering. The left stick overrides the grab. Accelerator and brake stay on the triggers. Sedan doors use the separate physical entry option. Save motion controls to keep this setting.");
  bool barsEnabled355=fc3pad::HandlebarEnabled355();
  if(ImGui::Checkbox("Handlebar steering (ATV / jetski)",&barsEnabled355))fc3pad::SetHandlebarEnabled355(barsEnabled355);
  float barsDegrees355=fc3pad::HandlebarLock355();
  float barSensitivity363=35.f/barsDegrees355;
  if(ImGui::SliderFloat("Handlebar steering sensitivity",&barSensitivity363,.5f,2.33f,"%.2fx")){fc3pad::SetHandlebarLock355(35.f/barSensitivity363);barsDegrees355=fc3pad::HandlebarLock355();}
  if(ImGui::SliderFloat("Handlebar turn for full steering",&barsDegrees355,15.f,70.f,"%.0f degrees"))fc3pad::SetHandlebarLock355(barsDegrees355);
  float barSmooth363=fc3pad::VehicleSmoothing363(true);if(ImGui::SliderFloat("Handlebar steering smoothing",&barSmooth363,0.f,150.f,"%.0f ms"))fc3pad::SetVehicleSmoothing363(true,barSmooth363);
  ImGui::TextWrapped("ATV/jetski: higher sensitivity needs less turning. Start around 70 ms smoothing and lower sensitivity if small movements steer too much. Release remains immediate.");
  ImGui::TextWrapped("Hold either or both grips near the corresponding handle. Turn around the steering stem. Release to free your hands; left stick overrides. Triggers keep accelerator/brake.");
  if(ImGui::Button("Save motion controls")){
   wchar_t path[MAX_PATH];ManualSettingsPath(path);const wchar_t* target=g_menuSavePath?g_menuSavePath:path;
   wchar_t value[16];swprintf_s(value,L"%d",int(g_rifleRecoilAssist*100+.5f));
   const bool a=WritePrivateProfileStringW(L"MotionControls",L"SwapBX",swap?L"1":L"0",target)!=0;
   const bool b=WritePrivateProfileStringW(L"MotionControls",L"RifleRecoilAssist",value,target)!=0;
   const bool c=WritePrivateProfileStringW(L"MotionControls",L"AutoPistolSupport",g_autoPistolSupport?L"1":L"0",target)!=0;
   const bool d=WritePrivateProfileStringW(L"MotionControls",L"AutoLongGunSupport",g_autoLongGunSupport?L"1":L"0",target)!=0;
   swprintf_s(value,L"%d",int(g_twoHandRecoilAssist*100+.5f));
   const bool e=WritePrivateProfileStringW(L"MotionControls",L"TwoHandRecoilAssist",value,target)!=0;
   const bool f=WritePrivateProfileStringW(L"MotionControls",L"GripOnlyAmmo",gripOnly?L"1":L"0",target)!=0;
   const bool g=WritePrivateProfileStringW(L"MotionControls",L"NativeMenuMouse",g_nativeUiMouseEnabled?L"1":L"0",target)!=0;
   const bool j298=WritePrivateProfileStringW(L"MotionControls",L"NativeMenuRay",g_nativeUiRayEnabled298?L"1":L"0",target)!=0;
   swprintf_s(value,L"%d",int(fc3pad::CarWheelFullLockDegrees()+.5f));
   const bool h=WritePrivateProfileStringW(L"Vehicle",L"WheelFullLockDegrees",value,target)!=0;
   const bool i=WritePrivateProfileStringW(L"Vehicle",L"PhysicalWheel353",carWheelEnabled?L"1":L"0",target)!=0;
   swprintf_s(value,L"%d",int(fc3pad::HandlebarLock355()+.5f));
   const bool barsSave355=WritePrivateProfileStringW(L"Vehicle",L"Handlebars355",barsEnabled355?L"1":L"0",target)&&WritePrivateProfileStringW(L"Vehicle",L"HandlebarFullLock355",value,target);
   swprintf_s(value,L"%d",int(fc3pad::VehicleSmoothing363(false)));const bool smoothSave363=WritePrivateProfileStringW(L"Vehicle",L"CarSmoothing363",value,target)!=0;
   swprintf_s(value,L"%d",int(fc3pad::VehicleSmoothing363(true)));const bool barSmoothSave363=WritePrivateProfileStringW(L"Vehicle",L"BarSmoothing363",value,target)!=0;
   const bool doorSave357=WritePrivateProfileStringW(L"Vehicle",L"Doors357",L"0",target)!=0; // 365: door removed
   bool pullSave365=true,outSave365=true;
   for(int h=0;h<2;++h){const wchar_t* pk[2]={L"HandRimPullCm366R",L"HandRimPullCm366L"},*ok[2]={L"HandRimOutCm366R",L"HandRimOutCm366L"};
    swprintf_s(value,L"%d",int(std::lround(g_handRimPullCm366[h])));pullSave365=pullSave365&&WritePrivateProfileStringW(L"Vehicle",pk[h],value,target)!=0;
    swprintf_s(value,L"%d",int(std::lround(g_handRimOutCm366[h])));outSave365=outSave365&&WritePrivateProfileStringW(L"Vehicle",ok[h],value,target)!=0;}
   const bool throttleSave365=WritePrivateProfileStringW(L"Vehicle",L"StickThrottle365",fc3pad::StickThrottle365()?L"1":L"0",target)!=0;
   g_pistolMenu.result=a&&b&&c&&d&&e&&f&&g&&h&&i&&j298&&barsSave355&&doorSave357&&smoothSave363&&barSmoothSave363&&pullSave365&&outSave365&&throttleSave365?1:2;
  }
  bool dedicated=fc3pad::DedicatedInteractionsEnabled();
  if(ImGui::Checkbox("Dedicated interaction / stick down crouch, up stand",&dedicated))fc3pad::SetDedicatedInteractions(dedicated);
  ImGui::TextWrapped("Hold the interaction button shown above for prompts. Reload gestures stay the same. Tap Y to swap; hold Y to heal. Grip near a support/reload point handles the weapon. Left grip + Y opens the weapon wheel; the right stick or enabled hand pointing selects. Release left grip to select. LT remains available for physical weapon actions. Each squeeze keeps its action until release.");
  ImGui::TextWrapped("Right stick down requests crouch; up requests stand. LT native aiming is disabled during motion-weapon gameplay. Physical LT actions remain.");
  ImGui::TextWrapped("Native menus: A confirms, B goes back. Menu opens pause; left grip + Menu opens the map. Release controls after closing Insert.");
  if(ImGui::Checkbox("Shotgun loading-port calibration",&g_reloadAlignment)){
   ManualDrop();g_manualReload.result=0;
  }
  ImGui::TextWrapped("Calibration only: close Insert to adjust with sticks; X saves. Reopen Insert and uncheck calibration to restore movement.");
  BodyInteractionsMenu285(g_menuSavePath);
  if(ImGui::CollapsingHeader("Weapon hands")){
   auto setting=fc3hands303::Configuration();bool changed=ImGui::Checkbox("Either hand can draw and hold weapons",&setting.enabled);
   changed=ImGui::Checkbox("Holster current weapon before drawing another",&setting.requireHolster)||changed;
   static bool failed=false;if(changed){wchar_t path[MAX_PATH];ManualSettingsPath(path);failed=!fc3hands303::Configure(setting,g_menuSavePath?g_menuSavePath:path);}
   ImGui::TextWrapped("Holster-first starts with empty hands. Grip a body holster with either hand. Left trigger fires a left-held gun; the other hand handles support/reload actions. To stow: move the holding hand out of the holster first, then return to that slot and release grip. For weapons other than1911: grip near the holding hand with the other hand, then release the original grip. Release elsewhere keeps the gun held.");
   ImGui::TextWrapped("Grab the firing handle with the free hand grip to take the weapon immediately. Foregrips support the weapon. Each squeeze keeps one action until released; active reloads retain their hand. Take ammo at the waist or chest pouch. Grenades and Molotovs are temporarily disabled. Movement, menus and holster positions stay unchanged.");
   if(failed)ImGui::TextDisabled("Current mode active; settings could not be saved.");
  }
  if(ImGui::CollapsingHeader("Controller feedback")){
   int strength=fc3xr::FeedbackStrength302();static bool failed=false;
   if(ImGui::SliderInt("Vibration strength",&strength,0,100,"%d%%")){wchar_t path[MAX_PATH];ManualSettingsPath(path);failed=!fc3xr::ConfigureFeedback302(strength,g_menuSavePath?g_menuSavePath:path);}
   ImGui::TextWrapped("Overall vibration strength, including gunfire, reload, holsters and melee. 0 mutes vibration. Changes save automatically.");
   int gunfire=fc3xr::GunfireStrength339();
   if(ImGui::SliderInt("Gunfire vibration",&gunfire,0,100,"%d%%")){wchar_t path[MAX_PATH];ManualSettingsPath(path);failed=!fc3xr::ConfigureGunfire339(gunfire,g_menuSavePath?g_menuSavePath:path);}
   ImGui::TextWrapped("Gunfire vibrates the holding hand for native bullet shots. Empty trigger pulls stay quiet. 0 disables gunfire vibration only.");
   if(ImGui::Button("Test controller feedback"))fc3xr::Buzz(1);
   if(failed)ImGui::TextDisabled("Current strength is active, but could not be saved.");
  }
  if(ImGui::CollapsingHeader("Avatar appearance")){
   ImGui::Checkbox("Hands only during motion-controlled gameplay",&g_handsOnly287);
   float edge=g_wristCut287*100;if(ImGui::SliderFloat("Wrist edge",&edge,54,62,"%.1f cm"))g_wristCut287=edge*.01f;
   ImGui::TextWrapped("Hides forearms and Jason's body while keeping tracked hands and weapons. Native scripted actions keep their normal animated arms. Full arms remain the default. Raise the wrist edge if too much forearm remains; lower it if the hand is cut too far.");
   static int saved=0;if(ImGui::Button("Save avatar appearance")){wchar_t path[MAX_PATH],value[32];ManualSettingsPath(path);const wchar_t* target=g_menuSavePath?g_menuSavePath:path;swprintf_s(value,L"%.6f",g_wristCut287);const bool a=WritePrivateProfileStringW(L"Avatar",L"HandsOnly",g_handsOnly287?L"1":L"0",target)!=0;const bool b=WritePrivateProfileStringW(L"Avatar",L"WristCut",value,target)!=0;saved=a&&b?1:2;}
   ImGui::SameLine();ImGui::TextUnformatted(saved==1?"Saved":saved==2?"SAVE FAILED":"Live preview; save to keep");
  }
 }
 if(ImGui::CollapsingHeader("Left hand UI")){
  ImGui::Checkbox("Native HUD on Jason's left wrist",&g_handUiSettings.enabled);ImGui::Checkbox("Health panel",&g_handUiSettings.health);ImGui::SameLine();ImGui::Checkbox("Ammo panel",&g_handUiSettings.ammo);ImGui::SameLine();ImGui::Checkbox("Weapon wheel panel",&g_handUiSettings.wheel);
  ImGui::Checkbox("Keep wrist text level when reading",&g_handUiSettings.levelText345);
  if(ImGui::Button("Level panels and clear wrist centre"))fc3handui::ClearWrist345(g_handUiSettings);
  ImGui::TextWrapped("The clear-centre preset moves the HUD beside your wrist so it does not cover the watch area. Save left hand UI keeps the placement.");
  ImGui::Checkbox("Show health and ammo only when looking at wrist",&g_handUiSettings.glanceOnly285);
  ImGui::TextWrapped("Glance mode hides the wrist panels when lowered or facing away. The weapon wheel and Insert previews stay available. Save left hand UI keeps this choice.");
  ImGui::TextWrapped("Live health and ammo stay visible while Insert is open. The minimap interior is removed from the hand health graphic. Position and rotation changes preview immediately; Save keeps them for next launch.");
  ImGui::Checkbox("Position with motion controllers",&g_handUiControllerPlacement);
  const char* placementNames[]={"Health","Ammo","Weapon wheel"};ImGui::Combo("Panel to position",&g_handUiPlacementPanel,placementNames,3);
  ImGui::TextWrapped("With placement enabled, hold RIGHT GRIP: right stick moves the panel left/right and up/down; left stick up/down moves it toward/away from the hand. Also hold RIGHT TRIGGER to rotate: right stick turns yaw/pitch, left stick left/right rolls. These controls only adjust panels while Insert is open.");
  const char* names[]={"Health","Ammo","Weapon wheel"};
  for(int i=0;i<3;++i){ImGui::PushID(i);if(ImGui::TreeNode(names[i])){auto& p=g_handUiSettings.panels[i];float position[]={p.x,p.y,p.z},rotation[]={p.pitch,p.yaw,p.roll};if(ImGui::SliderFloat3("Position (metres)",position,-.5f,.5f)){p.x=position[0];p.y=position[1];p.z=position[2];}if(ImGui::SliderFloat3("Rotation (degrees)",rotation,-180,180)){p.pitch=rotation[0];p.yaw=rotation[1];p.roll=rotation[2];}ImGui::SliderFloat("Panel width (metres)",&p.width,.04f,.70f);float start[]={p.left,p.top},end[]={p.right,p.bottom};if(ImGui::SliderFloat2("Source left / top",start,0,.99f)){p.left=start[0];p.top=start[1];}if(ImGui::SliderFloat2("Source right / bottom",end,.01f,1)){p.right=end[0];p.bottom=end[1];}if(!fc3handui::Valid(p))ImGui::TextUnformatted("Invalid source bounds: panel stays on screen until corrected.");ImGui::TreePop();}ImGui::PopID();}
  if(ImGui::Button("Reset left hand UI"))g_handUiSettings=fc3handui::Settings{};
  ImGui::SameLine();if(ImGui::Button("Save left hand UI")){wchar_t path[MAX_PATH];ManualSettingsPath(path);const wchar_t* target=g_menuSavePath?g_menuSavePath:path;bool ok=true;const wchar_t* names[]={L"Health",L"Ammo",L"Wheel"};const wchar_t* fields[]={L"X",L"Y",L"Z",L"Pitch",L"Yaw",L"Roll",L"Width",L"Left",L"Top",L"Right",L"Bottom"};
   ok=(WritePrivateProfileStringW(L"HandUI",L"GlanceOnly",g_handUiSettings.glanceOnly285?L"1":L"0",target)!=0)&&ok;
   ok=(WritePrivateProfileStringW(L"HandUI",L"LevelText",g_handUiSettings.levelText345?L"1":L"0",target)!=0)&&ok;
   ok=(WritePrivateProfileStringW(L"HandUI",L"Enabled",g_handUiSettings.enabled?L"1":L"0",target)!=0)&&ok;const bool flags[]={g_handUiSettings.health,g_handUiSettings.ammo,g_handUiSettings.wheel};for(int i=0;i<3;++i){ok=(WritePrivateProfileStringW(L"HandUI",names[i],flags[i]?L"1":L"0",target)!=0)&&ok;if(!fc3handui::Valid(g_handUiSettings.panels[i])){ok=false;continue;}for(int j=0;j<11;++j){wchar_t key[64],value[64];swprintf_s(key,L"%s%s",names[i],fields[j]);swprintf_s(value,L"%.6f",fc3handui::Field(g_handUiSettings.panels[i],j));ok=(WritePrivateProfileStringW(L"HandUI",key,value,target)!=0)&&ok;}}g_pistolMenu.result=ok?1:2;
  }
 }
 ShopUnlockMenu();
 ImGui::Separator();
 ImGui::BeginChild("Weapons",ImVec2(180*g_menuScale,0),true);
 for(int i=0;i<static_cast<int>(sizeof(kMenuWeapons)/sizeof(kMenuWeapons[0]));++i)
 {
  ImGui::PushID(i);
  if(ImGui::Selectable(strcmp(kMenuWeapons[i].name,"M133")==0?"M133 / signature (shared)":strcmp(kMenuWeapons[i].name,"VZ61")==0?"Skorpion":kMenuWeapons[i].name,g_menuWeapon==i))g_menuWeapon=i;
  ImGui::PopID();
 }
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("Settings",ImVec2(0,0),false);
 const auto& weapon=kMenuWeapons[g_menuWeapon];
 g_menuGuideFamily=strcmp(weapon.name,"AK47")==0?2:strcmp(weapon.name,"VZ61")==0?1:strcmp(weapon.name,"A2000")==0?3:strcmp(weapon.name,"STGW-90")==0?4:strcmp(weapon.name,"MP5")==0?5:strcmp(weapon.name,"Famas")==0?6:strcmp(weapon.name,"BZ19")==0?7:strcmp(weapon.name,"P416")==0?8:strcmp(weapon.name,"MK16")==0?9:strcmp(weapon.name,"DesertEagle")==0?10:strcmp(weapon.name,"KSV")==0?11:strcmp(weapon.name,"GalilAce52")==0?12:strcmp(weapon.name,"SVD")==0?13:strcmp(weapon.name,"R700")==0?14:strcmp(weapon.name,"U100")==0?15:strcmp(weapon.name,"MKG")==0?16:strcmp(weapon.name,"M93")==0?17:strcmp(weapon.name,"PKM")==0?18:0;
 const char* familyLabel=g_menuGuideFamily==18?"PKM":g_menuGuideFamily==17?"Z93":g_menuGuideFamily==16?"MKG":g_menuGuideFamily==15?"U100":g_menuGuideFamily==14?"R700":g_menuGuideFamily==13?"SVD":g_menuGuideFamily==12?"Galil ACE52":g_menuGuideFamily==11?"KSV Vector":g_menuGuideFamily==10?"Desert Eagle":g_menuGuideFamily==9?"MK16":g_menuGuideFamily==8?"P416":g_menuGuideFamily==7?"BZ19":g_menuGuideFamily==6?"F1":g_menuGuideFamily==5?"MP5":g_menuGuideFamily==4?"STG-90":g_menuGuideFamily==3?"A2000":g_menuGuideFamily==2?"AK":g_menuGuideFamily==1?"Skorpion":"1911 / 6P9";
 const bool menuCharge=RifleFamily(g_menuGuideFamily)&&(RifleForFamily(g_menuGuideFamily).emptyNeedsCharge||g_menuGuideFamily==14||g_menuGuideFamily==17); // 14 R700: the BOLT grab box
 if(!menuCharge&&g_pistolMenu.target==3)g_pistolMenu.target=0;
 ImGui::Text("%s | %s",strcmp(weapon.name,"KW1911")==0?"1911":strcmp(weapon.name,"VZ61")==0?"Skorpion":weapon.name,weapon.family);
 WeaponGuideCheckbox(weapon.name);

#ifdef FC3_AMMO_TEST
 MenuTestPoint(g_menuCheckPos);
#endif
 ReloadActionGuideMenu227(g_menuSavePath);
 if(strcmp(weapon.name,"R700")==0||strcmp(weapon.name,"M93")==0)R700Menu(g_menuSavePath); // the bolt-action family shares the bolt settings // bolt mode; the rifle guide page (magazine / BOLT box) follows
 if(strcmp(weapon.name,"1887")==0)Lever1887Menu(g_menuSavePath);
 else if(strcmp(weapon.family,"Machete")==0)MacheteColliderMenu118(g_menuSavePath);
 else if(strcmp(weapon.name,"HUNTER")==0)RevolverMenu(g_menuSavePath);
 else if(strcmp(weapon.name,"M133")==0)M133Menu54();
 else if(strcmp(weapon.name,"Spas12")==0)SpasMenu(g_menuSavePath);
 else if(strcmp(weapon.name,"BOW")==0)BowMenu(g_menuSavePath);
 else if(W7MenuIndex(weapon.name)>=0)Weapons7Menu(W7MenuIndex(weapon.name));
 else if(strcmp(weapon.name,"KW1911")!=0&&strcmp(weapon.name,"6P9")!=0&&strcmp(weapon.name,"VZ61")!=0&&strcmp(weapon.name,"AK47")!=0&&strcmp(weapon.name,"A2000")!=0&&strcmp(weapon.name,"STGW-90")!=0&&strcmp(weapon.name,"MP5")!=0&&strcmp(weapon.name,"Famas")!=0&&strcmp(weapon.name,"PKM")!=0&&strcmp(weapon.name,"M93")!=0&&strcmp(weapon.name,"U100")!=0&&strcmp(weapon.name,"MKG")!=0&&strcmp(weapon.name,"KSV")!=0&&strcmp(weapon.name,"GalilAce52")!=0&&strcmp(weapon.name,"SVD")!=0&&strcmp(weapon.name,"R700")!=0&&strcmp(weapon.name,"BZ19")!=0&&strcmp(weapon.name,"P416")!=0&&strcmp(weapon.name,"MK16")!=0&&strcmp(weapon.name,"DesertEagle")!=0)
 {
  ImGui::TextWrapped("Guide settings for this weapon will be added as its physical handling is mapped. The 1911 is the first configurable profile.");
 }
 else
 {
  int guideShape=g_pistolGuides.box?1:0;const char* guideShapes[]={"Round target (saved radius)","Box target (saved dimensions)"};
  if(ImGui::Combo("Reload target shape",&guideShape,guideShapes,2))g_pistolGuides.box=guideShape==1;
  ImGui::TextWrapped("Choose a guide below to preview it. Changes apply immediately. Save stores just that slider; Save All stores this weapon.");
  const char* guides[]={"Magazine insertion","Slide grab","Left-hand contact"};
  ImGui::SetNextItemWidth(220*g_menuScale);
  if(RifleFamily(g_menuGuideFamily)){const char* akGuides[]={"Magazine insertion","Magazine removal grab","Left-hand contact",(g_menuGuideFamily==14||g_menuGuideFamily==17)?"Bolt handle box":"Top charging knob"};ImGui::Combo("Preview guide",&g_pistolMenu.target,akGuides,menuCharge?4:3);}
  else if(g_menuGuideFamily==1){const char* skGuides[]={"Magazine insertion","Right-side charging handle","Left-hand contact"};ImGui::Combo("Preview guide",&g_pistolMenu.target,skGuides,3);}
  else ImGui::Combo("Preview guide",&g_pistolMenu.target,guides,3);
  ImGui::TextDisabled(RifleFamily(g_menuGuideFamily)?(AkModel()&&MagazineFamilyOfProfile(g_weaponMechanicProfile)==g_menuGuideFamily?"Rifle equipped":"Equip this rifle to see its markers"):g_menuGuideFamily==1?(SkorpionModel()?"Skorpion equipped":"Equip the Skorpion to see its markers"):(g_weaponMechanicProfile==20?"1911 equipped":g_weaponMechanicProfile==1?"6P9 equipped (shares 1911 settings)":"Equip the 1911 or 6P9 to see its markers"));
  {char label[96];_snprintf_s(label,96,_TRUNCATE,"Save All - %s##WeaponCalibration",familyLabel);if(ImGui::Button(label)){g_pistolMenu.result=SaveWeaponCalibration53(g_menuSavePath)?1:2;}}
#ifdef FC3_AMMO_TEST
  g_weaponSaveId53=ImGui::GetItemID();MenuTestPoint(g_menuSaveAllPos);
#endif
  ImGui::SameLine();ImGui::TextUnformatted(g_pistolMenu.result==1?"Saved":g_pistolMenu.result==2?"SAVE FAILED":"Unsaved edits stay live until restart");
  ImGui::Separator();
  {char label[64];_snprintf_s(label,64,_TRUNCATE,"Position %s ammo in hand",familyLabel);ImGui::Checkbox(label,&g_pistolGuides.ammoPreview);}
#ifdef FC3_AMMO_TEST
  MenuTestPoint(g_menuAmmoCheckPos);
#endif
  if(g_pistolGuides.ammoPreview){
   ImGui::TextWrapped("Visual preview only. Move and rotate the magazine in its local axes. Save All stores these offsets for real pickups. Close INSERT to end the preview.");
   for(int n=24;n<30;++n)GuideSlider(n,n==24?"Ammo X":n==25?"Ammo Y":n==26?"Ammo Z":n==27?"Ammo rotate X":n==28?"Ammo rotate Y":"Ammo rotate Z");
  }
  ImGui::Separator();ImGui::TextUnformatted("Magazine insertion (weapon local axes)");
  for(int n=0;n<3;++n)GuideSlider(n,n==0?"X":n==1?"Y":"Z");if(!g_pistolGuides.box)GuideSlider(9,"Radius");
  else {for(int n=12;n<15;++n)GuideSlider(n,n==12?"Width (X)":n==13?"Length (Y)":"Height (Z)");for(int n=18;n<21;++n)GuideSlider(n,n==18?"Rotate X":n==19?"Rotate Y":"Rotate Z");}
  {
  ImGui::Separator();ImGui::TextUnformatted(RifleFamily(g_menuGuideFamily)?"Magazine removal grab (weapon local axes)":g_menuGuideFamily==1?"Charging handle (weapon local axes)":"Slide grab (pistol local axes)");
  if(RifleFamily(g_menuGuideFamily))ImGui::TextWrapped(g_menuGuideFamily==3?"LEFT grip + X away from the removal target drops a magazine that is not full, or LEFT grip + X on the seated magazine, pull down 5 cm and release to drop. Insert a replacement: it is live at once. After an EMPTY reload, pull the top knob back first.":menuCharge?"LEFT grip + X away from the removal target drops a magazine that is not full, or LEFT grip + X on the seated magazine, pull down 5 cm and release to drop. Insert a replacement: it is live at once. After an EMPTY reload, pull the charging handle back first (partial reload: no pull).":"LEFT grip + X away from the removal target drops a magazine that is not full, or LEFT grip + X on the seated magazine, pull down 5 cm and release to drop. Insert a replacement: it fires at once. No charging.");
  if(g_menuGuideFamily==11)ImGui::TextWrapped("KSV removal uses the insertion target above, including its size and rotation. Left grip on that target grabs the magazine; grip at the support point holds the gun.");
  else {
  for(int n=3;n<6;++n)GuideSlider(n,n==3?"X":n==4?"Y":"Z");if(!g_pistolGuides.box)GuideSlider(10,"Radius");
  else {for(int n=15;n<18;++n)GuideSlider(n,n==15?"Width (X)":n==16?"Length (Y)":"Height (Z)");for(int n=21;n<24;++n)GuideSlider(n,n==21?"Rotate X":n==22?"Rotate Y":"Rotate Z");}
  }
  }
  if(menuCharge){
   ImGui::Separator();ImGui::TextUnformatted(g_menuGuideFamily==3?"Top charging knob (weapon local axes)":g_menuGuideFamily==6?"Middle charging handle, under the carry handle (weapon local axes)":"Charging handle (weapon local axes)");
   ImGui::TextWrapped("After an empty reload: release LEFT grip, grip this box, pull backward and release. The native charging finger pose follows your wrist. Magazine removal has its own box.");
   for(int n=30;n<39;++n)GuideSlider(n,n==30?"Charge X":n==31?"Charge Y":n==32?"Charge Z":n==33?"Charge width":n==34?"Charge length":n==35?"Charge height":n==36?"Charge rotate X":n==37?"Charge rotate Y":"Charge rotate Z");
  }
  ImGui::Separator();ImGui::TextUnformatted("Left hand (offset from palm in wrist axes)");
  for(int n=6;n<9;++n)GuideSlider(n,n==6?"X":n==7?"Y":"Z");GuideSlider(11,"Diameter");
  ImGui::Separator();ImGui::TextUnformatted("Ammo pickup and loose magazines");
  const char* pickup[]={"Waist","Chest / upper stomach","Grip + X"};
  ImGui::Combo("Ammo pickup",&g_pistolGuides.ammoPickup,pickup,3);
  ImGui::SliderFloat("Replacement delay",&g_pistolGuides.replacementDelay,0,10,"%.2f seconds");
  ImGui::TextWrapped("After a drop, the clip stays catchable for 1 second; after an upward toss, 3 seconds. Replacement delay starts when it disappears.");
  ImGui::SliderFloat("Magazine toss height",&g_pistolGuides.tossHeight,.5f,2.5f,"%.2fx");
  ImGui::SliderFloat("Catch distance",&g_pistolGuides.catchRadius,.05f,.3f,"%.2f m");
  if(ImGui::CollapsingHeader("Point/pull loose magazine recovery")){
   ImGui::Checkbox("Recover the same loose magazine by pointing and pulling",&g_pistolGuides.pullAssist286);
   ImGui::SliderFloat("Point/pull range",&g_pistolGuides.pullRange286,.5f,3.f,"%.2f m");
   ImGui::SliderFloat("Recovery pull distance",&g_pistolGuides.pullDistance286,.08f,.3f,"%.2f m");
   ImGui::TextWrapped("Point your LEFT controller at an existing dropped/tossed magazine, squeeze grip, then pull back. Keep grip held as it returns. This recovers the same loose item; ordinary nearby catches and replacement timing are retained. Retired magazines and ground collision are not added.");
  }
  ImGui::TextWrapped("Grip at the selected body zone takes a magazine. Keep LEFT GRIP held, flick the hand upward and press X to toss the held magazine. Keep LEFT GRIP held near the returning magazine to catch the same item. Release grip to drop it.");
  if(ImGui::Button("Save ammo settings"))g_pistolMenu.result=SavePistolGuides(g_menuSavePath)?1:2;
 }
 if(ImGui::CollapsingHeader("Two-hand grab area",ImGuiTreeNodeFlags_DefaultOpen)){
  const int profile=WeaponGuideProfile(weapon.name);ImGui::PushID("SupportGrab214");
  if(SupportGrabEditable214(profile)){
   auto& grip=g_supportGrab214[profile];
   ImGui::TextWrapped("Move and scale the two-hand grab box in the gun's local axes. The displayed box and grip check use the same size. Equip this weapon to preview changes immediately.");
   ImGui::SliderFloat("Position X",&grip.offset[0],-.30f,.30f,"%.3f m");
   ImGui::SliderFloat("Position Y",&grip.offset[1],-.30f,.30f,"%.3f m");
   ImGui::SliderFloat("Position Z",&grip.offset[2],-.30f,.30f,"%.3f m");
   ImGui::SliderFloat("Overall grab reach",&grip.reach,.06f,.35f,"%.3f m");
   ImGui::SliderFloat("Box scale X",&grip.sizeScale[0],.25f,3.f,"%.2fx");
   ImGui::SliderFloat("Box scale Y",&grip.sizeScale[1],.25f,3.f,"%.2fx");
   ImGui::SliderFloat("Box scale Z",&grip.sizeScale[2],.25f,3.f,"%.2fx");
   if(ImGui::Button("Reset grab area"))grip=SupportGrab214{};
   ImGui::SameLine();if(ImGui::Button("Save grab area"))g_pistolMenu.result=SaveSupportGrab214(profile,g_menuSavePath)?1:2;
   ImGui::SameLine();ImGui::TextUnformatted(g_pistolMenu.result==1?"Saved":g_pistolMenu.result==2?"SAVE FAILED":"Live preview; save to keep it");
  }else if(profile==0||(profile>=3&&profile<=6)||profile==18||profile==34){
   ImGui::TextWrapped("This weapon has a physical support box in its settings above. Its position and size are saved there.");
  }else ImGui::TextWrapped("This weapon does not use the shared two-hand grab area; its left hand operates another weapon part.");
  ImGui::PopID();
 }
 ImGui::EndChild();
}
void MenuBeginFrame(float width,float height,bool poll=true)
{
 ULONGLONG stageAt57=GetTickCount64();auto stage57=[&](const char* name){auto now=GetTickCount64();static LONG reports=0;if(now-stageAt57>=40&&reports++<64)Log("[INSERT PERF57] %s blocked %llu ms frame %ld",name,now-stageAt57,g_frame);stageAt57=now;};
 if(!g_menuContext){IMGUI_CHECKVERSION();g_menuContext=ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;ImGui::StyleColorsDark();ImGui::GetStyle().WindowRounding=8;}
 ImGui::SetCurrentContext(g_menuContext);CalibrationFonts139();auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(width,height);io.DeltaTime=1.f/60;
 io.MouseDrawCursor=true;
 if(poll)
 {
  POINT mouse={};GetCursorPos(&mouse);ScreenToClient(g_menuWindow,&mouse);
  const bool focused=GetForegroundWindow()==g_menuWindow,desktopDown=focused&&(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
  const auto controls299=fc3pad::NativeUiControls();static ULONGLONG prior299=0;const auto now299=GetTickCount64();const float dt299=prior299?float(now299-prior299)*.001f:0;prior299=now299;
  float uv299[2]={};const bool ready299=focused&&controls299.active&&fc3xr::Running()&&g_pistolMenu.open;
  const bool hit299=ready299&&(!desktopDown||g_optionsPointer299.pressed)&&fc3xr::OptionsMenuRay299(width,height,g_optionsPointer299.pressed,uv299);
  const auto point299=g_optionsPointer299.Step(ready299,hit299,uv299[0],uv299[1],controls299.rightTrigger,0,0,0,dt299);
  io.AddMousePosEvent(point299.point?point299.u*width:float(mouse.x),point299.point?point299.v*height:float(mouse.y));
  io.AddMouseButtonEvent(0,desktopDown||point299.click);
  if(point299.point&&std::isfinite(controls299.rightY)&&std::fabs(controls299.rightY)>.2f&&std::isfinite(dt299)&&dt299>=0&&dt299<=.12f)io.AddMouseWheelEvent(0,controls299.rightY*dt299*8.f);
  io.AddMouseButtonEvent(1,focused&&(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0);
  io.AddMouseWheelEvent(0,InterlockedExchange(&g_menuWheel,0)/120.f);
  const int vk[]={VK_BACK,VK_RETURN,VK_LEFT,VK_RIGHT,VK_DELETE,VK_HOME,VK_END,VK_TAB};
  const ImGuiKey key[]={ImGuiKey_Backspace,ImGuiKey_Enter,ImGuiKey_LeftArrow,ImGuiKey_RightArrow,ImGuiKey_Delete,ImGuiKey_Home,ImGuiKey_End,ImGuiKey_Tab};
  for(int k=0;k<8;++k)io.AddKeyEvent(key[k],focused&&(GetAsyncKeyState(vk[k])&0x8000)!=0);
  io.AddKeyEvent(ImGuiMod_Ctrl,focused&&(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0);io.AddKeyEvent(ImGuiMod_Shift,focused&&(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0);
  AcquireSRWLockExclusive(&g_menuTextLock);if(focused)for(UINT i=0;i<g_menuTextCount;++i)io.AddInputCharacter(g_menuText[i]);g_menuTextCount=0;ReleaseSRWLockExclusive(&g_menuTextLock);
  if(focused){if(g_menuClipCursor)g_menuClipCursor(nullptr);SetCursor(LoadCursorW(nullptr,MAKEINTRESOURCEW(32512)));}
 }
 stage57("input polling");
 g_menuScale=(std::max)(1.f,(std::min)(2.5f,height/900.f));ImGui::GetStyle()=ImGuiStyle();ImGui::StyleColorsDark();ImGui::GetStyle().ScaleAllSizes(g_menuScale);io.FontGlobalScale=g_menuScale;
 ImGui::NewFrame();
 const float w=(std::min)(1020.f*g_menuScale,width-16),h=(std::min)(790.f*g_menuScale,height-16);
 ImGui::SetNextWindowPos(ImVec2((width-w)*.5f,(height-h)*.5f),ImGuiCond_FirstUseEver);
 ImGui::SetNextWindowSize(ImVec2(w,h),ImGuiCond_FirstUseEver);
 ImGui::SetNextWindowSizeConstraints(ImVec2((std::min)(480.f*g_menuScale,width-16),(std::min)(300.f*g_menuScale,height-16)),ImVec2(width,height));
 bool open299=true;ImGui::Begin("Far Cry 3 VR - Weapon guides",&open299,ImGuiWindowFlags_NoCollapse);if(!open299)g_optionsClose299=true;
 const auto pos=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
 const int x=(std::max)(0,(std::min)(static_cast<int>(width)-1,static_cast<int>(std::floor(pos.x))));
 const int y=(std::max)(0,(std::min)(static_cast<int>(height)-1,static_cast<int>(std::floor(pos.y))));
 g_menuCrop[0]=x;g_menuCrop[1]=y;
 g_menuCrop[2]=(std::max)(1,(std::min)(static_cast<int>(width)-x,static_cast<int>(std::ceil(pos.x+size.x))-x));
 g_menuCrop[3]=(std::max)(1,(std::min)(static_cast<int>(height)-y,static_cast<int>(std::ceil(pos.y+size.y))-y));
 stage57("window layout");DrawInsertContents();stage57("properties");ImGui::End();ImGui::Render();stage57("draw-list generation");
}
void RenderInsertMenu(ID3D11Texture2D* backbuffer,bool desktop)
{
 struct MenuTiming56{ULONGLONG at=GetTickCount64();bool desktop;~MenuTiming56(){const auto elapsed=GetTickCount64()-at;static LONG reports=0;if(elapsed>=40&&reports++<64)Log("[INSERT PERF56] %s rendering blocked %llu ms; focus %d frame %ld",desktop?"desktop":"headset",elapsed,GetForegroundWindow()==g_menuWindow,g_frame);}} timing56{GetTickCount64(),desktop};
 if(!g_pistolMenu.open||!backbuffer||!g_dev||!g_ctx)return;
 const bool wasOwn=g_own;g_own=true;
 ImGuiContext* previous=ImGui::GetCurrentContext();
 if(!g_menuContext){g_menuContext=ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;ImGui::StyleColorsDark();}
 ImGui::SetCurrentContext(g_menuContext);CalibrationFonts139();
 if(!g_menuBackend)g_menuBackend=ImGui_ImplDX11_Init(g_dev,g_ctx);
 if(!g_menuBackend){g_own=wasOwn;ImGui::SetCurrentContext(previous);return;}
 D3D11_TEXTURE2D_DESC desc={};backbuffer->GetDesc(&desc);
 if(!desktop)
 {
  if(!g_menuTexture||g_menuWidth!=desc.Width||g_menuHeight!=desc.Height)
  {
   fc3xr::SetOptionsTexture(nullptr);if(g_menuRtv)g_menuRtv->Release();if(g_menuTexture)g_menuTexture->Release();g_menuRtv=nullptr;g_menuTexture=nullptr;
   D3D11_TEXTURE2D_DESC td=desc;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.SampleDesc.Quality=0;td.MipLevels=td.ArraySize=1;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;td.CPUAccessFlags=td.MiscFlags=0;
   if(FAILED(g_dev->CreateTexture2D(&td,nullptr,&g_menuTexture))||FAILED(g_dev->CreateRenderTargetView(g_menuTexture,nullptr,&g_menuRtv))){g_own=wasOwn;ImGui::SetCurrentContext(previous);return;}
   g_menuWidth=desc.Width;g_menuHeight=desc.Height;
  }
  const ULONGLONG backendAt57=GetTickCount64();ImGui_ImplDX11_NewFrame();if(GetTickCount64()-backendAt57>=40)Log("[INSERT PERF57] backend new-frame blocked %llu ms",GetTickCount64()-backendAt57);MenuBeginFrame(static_cast<float>(desc.Width),static_cast<float>(desc.Height));
 }
 ID3D11RenderTargetView* rtv=nullptr;
 if(desktop){if(FAILED(g_dev->CreateRenderTargetView(backbuffer,nullptr,&rtv))){g_own=wasOwn;ImGui::SetCurrentContext(previous);return;}}
 else {rtv=g_menuRtv;rtv->AddRef();const float clear[4]={0,0,0,0};g_ctx->ClearRenderTargetView(rtv,clear);}
 ID3D11RenderTargetView* old[8]={};ID3D11DepthStencilView* depth=nullptr;g_ctx->OMGetRenderTargets(8,old,&depth);
 const ULONGLONG uploadAt57=GetTickCount64();g_ctx->OMSetRenderTargets(1,&rtv,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());if(GetTickCount64()-uploadAt57>=40)Log("[INSERT PERF57] %s draw upload blocked %llu ms",desktop?"desktop":"headset",GetTickCount64()-uploadAt57);
 g_ctx->OMSetRenderTargets(8,old,depth);for(auto* v:old)if(v)v->Release();if(depth)depth->Release();rtv->Release();
 if(!desktop&&g_pistolMenu.open)fc3xr::SetOptionsTexture(g_menuTexture,g_menuCrop[0],g_menuCrop[1],g_menuCrop[2],g_menuCrop[3]);
 ImGui::SetCurrentContext(previous);g_own=wasOwn;
}
