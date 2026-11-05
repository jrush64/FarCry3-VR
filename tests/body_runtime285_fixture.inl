void TestBodyRuntime285(){
 wchar_t folder[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,folder);GetTempFileNameW(folder,L"bdy",0,path);
 g_bodySettings285=fc3body285::Settings{};g_bodySettings285.enabled=false;g_bodySettings285.pointWheel=false;
 g_bodySettings285.slots[2].position={.31f,-.22f,-.08f};g_bodySettings285.slots[2].sector=7;g_bodySettings285.pull=.18f;
 Check(SaveBodyInteractions285(path),"285 body interactions save to separate settings section");LoadBodyInteractions285(path);
 Check(!g_bodySettings285.enabled&&!g_bodySettings285.pointWheel&&g_bodySettings285.slots[2].sector==7&&std::fabs(g_bodySettings285.pull-.18f)<1e-5f&&std::fabs(g_bodySettings285.slots[2].position.x-.31f)<1e-5f,"285 body settings roundtrip including flags, diagonal, metre offsets and pull distance");
 WritePrivateProfileStringW(L"BodyInteractions",L"Slot1Radius",L"nan",path);WritePrivateProfileStringW(L"BodyInteractions",L"DrawDistance",L"-10",path);LoadBodyInteractions285(path);
 Check(g_bodySettings285.slots[0].radius==.16f&&g_bodySettings285.pull==.12f,"285 corrupt body settings return to bounded defaults");DeleteFileW(path);
 g_worldScale=2;g_eyeViewsSet=true;g_bodyRuntime285=BodyRuntime285{};g_bodyState285.Reset();
 const float view[16]={0,0,-1,4,0,1,0,-3,1,0,0,-2,0,0,0,1};
 for(int e=0;e<2;++e)memcpy(g_eyeViewsBody[e].view,view,sizeof(view));g_handBody[0][3]=3;g_handBody[0][7]=3;g_handBody[0][11]=4;
 Check(BodyHead285()&&fc3body285::Distance(g_bodyRuntime285.head,{2,3,4})<1e-5f&&fc3body285::Distance(BodyHand285(),{0,0,.5f})<1e-5f,"285 shipping runtime uses untouched tracked body poses and scaled head-relative yaw");
 const float cp=.08715574f,sp=.9961947f;
 const float down[16]={0,0,-1,4,-sp,cp,0,2*sp-3*cp,cp,sp,0,-2*cp-3*sp,0,0,0,1};
 for(int e=0;e<2;++e)memcpy(g_eyeViewsBody[e].view,down,sizeof(down));
 Check(BodyHead285()&&std::fabs(g_bodyRuntime285.yaw-1.5707963f)<1e-5f,"285 looking almost straight down retains level body-zone heading");
 g_weaponMechanicProfile=-1;g_pistolMenu.open=false;g_reload={};g_pistolReload=PistolReloadState{};g_twoHand=false;g_reloadAlignment=false;
 Check(!BodyBusy285(),"285 idle weapon state does not block holsters");g_reload.held=true;Check(BodyBusy285(),"285 carried physical shell blocks new body grab");g_reload.held=false;
 g_weaponMechanicProfile=0;g_weaponMechanicFrame=g_frame;g_lever.shell=true;Check(BodyBusy285(),"285 existing1887 shell transaction outranks body zones");g_lever.shell=false;
 g_weaponMechanicProfile=42;g_revolver.open=true;Check(BodyBusy285(),"285 open revolver cylinder protects its reload transaction");g_revolver.open=false;g_weaponMechanicProfile=-1;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(true);
 g_macheteEnabled=true;g_macheteHoldToDraw=true;BodyMachete285();
 Check(MacheteHeld()&&g_machete.bodyDraw285,"285 belt draw uses existing persistent machete even with button hold mode configured");
 BodyMachete285();Check(!MacheteHeld()&&!g_machete.bodyDraw285,"285 belt stow clears only its machete ownership");
 g_macheteEnabled=false;BodyMachete285();Check(!MacheteHeld(),"285 disabled machete cannot be drawn through a body zone");
 puts("PASS:285 shipping settings/head basis, steep downward glance, reload priority and physical machete draw/stow");
}
