#include "body_interaction285.h"
fc3body285::Settings g_bodySettings285;
fc3body285::State g_bodyState285;
struct BodyRuntime285 {
 bool loaded=false,headValid=false,yawValid=false;float yaw=0;fc3body285::Vec head;
 int hover=-1,pointSector=-1,phase=0,sector=0;ULONGLONG phaseAt=0,lastAt=0;
 unsigned epoch=0;unsigned long long owner=0;
} g_bodyRuntime285;

float BodySetting285(const wchar_t* path,const wchar_t* key,float fallback,float minimum,float maximum){
 wchar_t text[64]={};GetPrivateProfileStringW(L"BodyInteractions",key,L"",text,64,path);
 wchar_t* end=nullptr;const float v=float(wcstod(text,&end));
 return end==text||*end||!std::isfinite(v)||v<minimum||v>maximum?fallback:v;
}
void LoadBodyInteractions285(const wchar_t* path){
 g_bodySettings285=fc3body285::Settings{};auto& s=g_bodySettings285;
 s.enabled=GetPrivateProfileIntW(L"BodyInteractions",L"Holsters",1,path)!=0;
 s.pointWheel=GetPrivateProfileIntW(L"BodyInteractions",L"PointWheel",1,path)!=0;
 s.pull=BodySetting285(path,L"DrawDistance",.12f,.08f,.30f);
 for(int n=0;n<5;++n){wchar_t key[40];float* values[]={&s.slots[n].position.x,&s.slots[n].position.y,&s.slots[n].position.z};
  for(int a=0;a<3;++a){swprintf_s(key,L"Slot%d%c",n+1,L"XYZ"[a]);*values[a]=BodySetting285(path,key,*values[a],-1.f,1.f);}
  swprintf_s(key,L"Slot%dRadius",n+1);s.slots[n].radius=BodySetting285(path,key,s.slots[n].radius,.06f,.3f);
  swprintf_s(key,L"Slot%dDirection",n+1);s.slots[n].sector=int(BodySetting285(path,key,float(s.slots[n].sector),0,8));
 }
 g_bodyRuntime285.loaded=true;g_bodyState285.Reset();fc3pad::SetBodyInput285(0);
}
bool SaveBodyInteractions285(const wchar_t* path){
 bool ok=true;auto save=[&](const wchar_t* key,float v){wchar_t value[40];swprintf_s(value,L"%.6f",v);ok=(WritePrivateProfileStringW(L"BodyInteractions",key,value,path)!=0)&&ok;};
 save(L"Holsters",g_bodySettings285.enabled?1.f:0.f);save(L"PointWheel",g_bodySettings285.pointWheel?1.f:0.f);save(L"DrawDistance",g_bodySettings285.pull);
 for(int n=0;n<5;++n){wchar_t key[40];const auto& slot=g_bodySettings285.slots[n];const float values[]={slot.position.x,slot.position.y,slot.position.z};
  for(int a=0;a<3;++a){swprintf_s(key,L"Slot%d%c",n+1,L"XYZ"[a]);save(key,values[a]);}
  swprintf_s(key,L"Slot%dRadius",n+1);save(key,slot.radius);swprintf_s(key,L"Slot%dDirection",n+1);save(key,float(slot.sector));
 }
 return ok;
}
bool BodyHead285(){
 auto& r=g_bodyRuntime285;r.headValid=false;if(!g_eyeViewsSet||!std::isfinite(g_worldScale)||g_worldScale<.1f)return false;
 const auto a=fc3body285::HeadPosition(g_eyeViewsBody[0].view),b=fc3body285::HeadPosition(g_eyeViewsBody[1].view);
 r.head={(a.x+b.x)*.5f,(a.y+b.y)*.5f,(a.z+b.z)*.5f};if(!fc3body285::Finite(r.head))return false;
 const float* view=g_eyeViewsBody[0].view;const float length=std::sqrt(view[8]*view[8]+view[10]*view[10]);
 if(!std::isfinite(length)||(length<.25f&&!r.yawValid))return false;
 // Level yaw only. Freeze while drawing so a head turn cannot pull a slot away.
 if(length>=.25f&&g_bodyState285.grab<0){r.yaw=std::atan2(view[8],view[10]);r.yawValid=true;}
 r.headValid=true;return true;
}
fc3body285::Vec BodyHand285(){return fc3body285::Relative({g_handBody[0][3],g_handBody[0][7],g_handBody[0][11]},g_bodyRuntime285.head,g_bodyRuntime285.yaw,g_worldScale);}
bool BodyBusy285(bool includeSupport=true){
 return (includeSupport&&g_twoHand)||g_reload.held||g_reload.pending||ReloadPoseActive()||g_reloadAlignment||
  ManualReloadOwnsLeftHand()||PistolOwnsLeft()||Lever1887LeftHold()||R700LeftHold()||FlamerBusyLeft()||Gm94BusyLeft()||
  (PistolHandling()&&PistolBusy())||(RpgActive()&&(RpgCarrying()||RpgClipPlaying()))||SpasClipPlaying()||
  (Lever1887Model()&&(g_lever.shell||g_lever.pending||g_lever.grabbing||g_lever.clip>=0))||
  (R700Model()&&(g_r700.grabbing||g_r700.clip>=0||g_r700.native))||
  (RevolverModel()&&(g_revolver.open||g_revolver.carrying||g_revolver.pending||g_revolver.grab||g_revolver.clip>=0||g_revolver.native))||
  (SpasModel()&&(g_spas.carrying||g_spas.pending||(g_spas.grab&&(includeSupport||g_spas.needPump))||g_spas.native))||
  (FlareModel()&&(g_flare.open||g_flare.carrying||g_flare.pending||g_flare.grab))||
  (Gm94Model()&&(g_gm94.coverOpen||g_gm94.carrying||g_gm94.pending||g_gm94.pumping))||
  (FlamerModel()&&(g_flamer.leverDown||g_flamer.carrying||g_flamer.pending||g_flamer.leverGrab))||
  (BowModel()&&(g_bow.held||g_bow.pending||g_bow.shooting||g_bow.releasePending))||
  ThrowableOwns144()||ThrowableArmLease150();
}
void BodyMachete285(){
 if(MacheteHeld()){StowMachete("physical belt stow");return;}
 if(!g_macheteEnabled||BodyBusy285())return;
 g_machete.held=true;g_machete.bodyDraw285=true;g_machete.started=GetTickCount64();g_machete.motionUntil=g_machete.started+3000;g_machete.handFrame=-100000;
 g_machete.logged=false;fc3pad::CancelReloadRequest();fc3pad::SetMacheteHandling(true);
 if(!MacheteReady())fc3pad::RequestMacheteSource();
 Log("[BODY285] physical belt draw uses existing machete source and contact damage");
}
void TickBodyInteractions285(bool headset){
 // 285 body zones failed live. Paused while the visible four-slot selector is tested.
 g_bodyRuntime285.phase=0;g_bodyState285.Reset();fc3pad::SetBodyInput285(0);return;
 auto& r=g_bodyRuntime285;const auto now=GetTickCount64();
 if(!r.loaded){wchar_t path[MAX_PATH];ManualSettingsPath(path);LoadBodyInteractions285(path);}
 const auto c=fc3pad::BodyControls285();bool seated=false;int seat=-1;unsigned long long owner=0;
 const bool ordinaryWheel=!r.phase&&fc3pad::WeaponWheelRequested145();
 const bool valid=headset&&g_enabled&&g_motionGun&&g_inGameplay&&g_frameCameraValid&&!g_flatViewActive&&(MotionInteractionEnabled()||r.phase||ordinaryWheel)&&
  !g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&c.active&&g_handRightValid&&BodyHead285()&&
  fc3aim::VehicleState(seated,seat)&&!seated&&!fc3aim::NativeActionOwnsArms128()&&fc3aim::LocalPlayerIdentity143(owner);
 const bool discontinuity=(r.lastAt&&(now<r.lastAt||now-r.lastAt>150))||r.epoch!=g_armPoseEpoch||r.owner!=owner;
 r.lastAt=now;r.epoch=g_armPoseEpoch;r.owner=owner;
 const bool explicitAction=c.menu||c.x||c.y||c.a||c.b||c.leftTrigger>.25f||c.rightTrigger>.25f||c.leftStickClick||c.rightStickClick;
 if(!valid||discontinuity||(r.phase&&explicitAction)){
  r.phase=0;r.hover=-1;r.pointSector=-1;if(discontinuity)r.yawValid=false;g_bodyState285.Reset();fc3pad::SetBodyInput285(0);return;
 }
 if(r.phase){
  float x=0,y=0;fc3body285::Sector(r.sector,x,y);
  const auto elapsed=now-r.phaseAt;
  if(elapsed<300)fc3pad::SetBodyInput285(1,x,y);
  else if(elapsed<400)fc3pad::SetBodyInput285(2,x,y);
  else {r.phase=0;r.hover=-1;fc3pad::SetBodyInput285(0);Log("[BODY285] wheel direction %d released; native inventory owns selection",r.sector);}
  return;
 }
 if(ordinaryWheel){
  g_bodyState285.Reset();r.hover=-1;float x=0,y=0;
  const bool point=g_bodySettings285.pointWheel&&fc3body285::Point(g_handBody[0],g_eyeViewsBody[0].view,x,y);
  fc3pad::SetBodyInput285(point?3:0,x,y);
  if(point&&std::fabs(c.rightX)<.25f&&std::fabs(c.rightY)<.25f){const int sector=std::fabs(x)>std::fabs(y)?(x>0?0:2):(y>0?1:3);
   if(sector!=r.pointSector){fc3xr::InteractionPulse285(0);r.pointSector=sector;}
  }else r.pointSector=-1;
  return;
 }
 fc3pad::SetBodyInput285(0);r.pointSector=-1;
 fc3body285::Input input;input.allowed=valid;input.tracked=g_handRightValid;
 input.conflict=explicitAction||c.leftGrip>.5f||BodyBusy285();input.grip=c.rightGrip;input.hand=BodyHand285();
 input.motion={g_handBody[0][3]/g_worldScale,g_handBody[0][7]/g_worldScale,g_handBody[0][11]/g_worldScale};input.now=now;input.epoch=g_armPoseEpoch;
 const auto result=g_bodyState285.Step(g_bodySettings285,input);
 if(result.holding)fc3pad::SetBodyInput285(4);
 if(result.hover>=0&&result.hover!=r.hover)fc3xr::InteractionPulse285(0);
 r.hover=result.hover;
 if(result.pulse)fc3xr::InteractionPulse285(0);
 if(result.draw==8){BodyMachete285();fc3xr::InteractionPulse285(0);return;}
 if(result.draw>=0){if(MacheteHeld())StowMachete("physical firearm draw");r.phase=1;r.phaseAt=now;r.sector=result.draw;
  float x=0,y=0;fc3body285::Sector(r.sector,x,y);fc3pad::SetBodyInput285(1,x,y);fc3pad::CancelReloadRequest();
  fc3xr::InteractionPulse285(0);Log("[BODY285] deliberate holster pull: native wheel direction %d",r.sector);
 }
}
void BodyInteractionsMenu285(const wchar_t* explicitPath){
 auto holsters=fc3xr::WeaponHolsterSettings297();const char* modes[]={"Four body holsters","Single holster + stick","Front rack"};static int saved297=0;
 bool changed297=ImGui::Combo("Weapon holsters",&holsters.mode,modes,3);
 float height297=holsters.height*100,radius297=holsters.radius*100;changed297=ImGui::SliderFloat("Holster height offset (cm)",&height297,-25,25,"%.0f")||changed297;changed297=ImGui::SliderFloat("Holster grab radius (cm)",&radius297,8,20,"%.0f")||changed297;holsters.height=height297*.01f;holsters.radius=radius297*.01f;

 changed297=ImGui::Checkbox("Show holster grab areas",&holsters.guides)||changed297;
 changed297=ImGui::Checkbox("Show held weapon stow outline",&holsters.stowOutline)||changed297;
 if(ImGui::Button("Seated: bring all four holsters in front")){fc3holster297::Seated(holsters);holsters.mode=fc3holster297::Four;holsters.guides=true;changed297=true;}
 ImGui::SameLine();if(ImGui::Button("Reset holster positions")){for(int i=0;i<4;++i)holsters.custom[i]=false;holsters.height=0;changed297=true;}
 static int selected304=0;const char* names304[]={"Holster 1","Holster 2 / Single location","Holster 3","Holster 4"};ImGui::Combo("Adjust holster",&selected304,names304,4);
 auto point304=fc3holster297::Centre(holsters,selected304);if(holsters.mode==fc3holster297::Single&&selected304!=1){auto four304=holsters;four304.mode=fc3holster297::Four;point304=fc3holster297::Centre(four304,selected304);}point304.y-=holsters.height;
 float x304=point304.x*100,y304=point304.y*100,z304=-point304.z*100;
 bool move304=ImGui::SliderFloat("Left / right (cm)",&x304,-100,100,"%.0f");
 move304=ImGui::SliderFloat("Height below head (cm)",&y304,-140,15,"%.0f")||move304;
 move304=ImGui::SliderFloat("Behind / in front (cm)",&z304,-100,100,"%.0f")||move304;
 if(move304){holsters.positions[selected304]={x304*.01f,y304*.01f,-z304*.01f};holsters.custom[selected304]=true;changed297=true;}
 ImGui::TextWrapped("Put a controller where you want this holster, then place it there. The visible grab area moves with it.");
 static int placed345=0;
 if(ImGui::Button("Place selected holster at LEFT hand")){placed345=fc3xr::CaptureHolsterPosition345(holsters,selected304,1)?1:2;changed297=placed345==1||changed297;}
 ImGui::SameLine();if(ImGui::Button("At RIGHT hand")){placed345=fc3xr::CaptureHolsterPosition345(holsters,selected304,0)?1:2;changed297=placed345==1||changed297;}
 if(placed345==2)ImGui::TextDisabled("Keep that controller tracked and within reach, then try again.");
 if(changed297)saved297=fc3xr::ConfigureWeaponHolsters297(holsters,explicitPath)?1:2;
 if(saved297==1)ImGui::TextDisabled("Holster settings saved");else if(saved297==2)ImGui::TextDisabled("Could not save holster settings; close selection and try again");
 ImGui::TextWrapped("Zone colours: 1 blue, 2 orange, 3 purple, 4 mint. Positions save automatically and follow the existing body anchor. Positive right moves right; positive front moves in front. Rear positions are supported. Single mode uses holster 2's position. Show grab areas draws the actual handle trigger boundaries, including while this menu is open. Gold weapon outline marks the held weapon's return slot; green means the holding hand is in range and ready to stow. Return there and release grip. Transfers and reload controls are unchanged.");return;
 if(!ImGui::CollapsingHeader("Body holsters and pointed weapon wheel"))return;
 if(!g_bodyRuntime285.loaded){wchar_t path[MAX_PATH];ManualSettingsPath(path);LoadBodyInteractions285(path);}
 ImGui::Checkbox("Body weapon selection",&g_bodySettings285.enabled);ImGui::Checkbox("Point right controller to select in wheel",&g_bodySettings285.pointWheel);
 ImGui::TextWrapped("With your right hand, reach to a hip or shoulder, pause briefly, squeeze grip and pull away. Each zone selects a native weapon-wheel direction. The game decides which unlocked weapon occupies it. Existing wheel buttons and right stick remain available; stick movement takes priority over pointing.");
 const char* names[]={"Right hip","Left hip","Right shoulder","Left shoulder","Front belt / machete"};const char* directions[]={"Right","Up","Left","Down","Up-right","Up-left","Down-left","Down-right","Draw / stow machete"};
 static int selected=0,result=0;ImGui::Combo("Body zone",&selected,names,5);auto& slot=g_bodySettings285.slots[selected];
 ImGui::Combo("Weapon-wheel direction",&slot.sector,directions,9);
 float p[3]={slot.position.x*100,slot.position.y*100,slot.position.z*100};
 if(ImGui::SliderFloat3("Position from head (cm)",p,-100,100,"%.1f"))slot.position={p[0]*.01f,p[1]*.01f,p[2]*.01f};
 float radius=slot.radius*100;if(ImGui::SliderFloat("Zone radius (cm)",&radius,6,30,"%.1f"))slot.radius=radius*.01f;
 float pull=g_bodySettings285.pull*100;if(ImGui::SliderFloat("Pull to draw (cm)",&pull,8,30,"%.1f"))g_bodySettings285.pull=pull*.01f;
 if(ImGui::Button("Place zone at current right controller")&&g_handRightValid&&BodyHead285())slot.position=BodyHand285();
 if(ImGui::Button("Reset selected zone"))slot=fc3body285::Settings{}.slots[selected];
 if(ImGui::Button("Save body interactions")){wchar_t path[MAX_PATH];ManualSettingsPath(path);result=SaveBodyInteractions285(explicitPath?explicitPath:path)?1:2;}
 ImGui::SameLine();ImGui::TextUnformatted(result==1?"Saved":result==2?"SAVE FAILED":"Live edits; save to keep");
 ImGui::TextWrapped("Reloads, two-hand support, explosives, native actions and vehicles take priority. Release grip after tracking returns before drawing. These zones switch weapons; they do not add visible weapon models on your body.");
}
