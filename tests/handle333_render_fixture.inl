extern bool ConsumeNativeHandle333(int);
void TestHandleBinding333(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 (void)device;float raw[2][576]={};for(int s=0;s<2;++s){for(int b=0;b<48;++b)StoreBone(raw[s],b,HandIdentity());memcpy(raw[s],kNeutralArmView[s],sizeof(kNeutralArmView[s]));}
 unsigned cases=0;
 for(int profile=0;profile<44;++profile){
  if(!(fc3hands303::Sidearm315(profile)||fc3hands303::MagazineRifle317(profile)||fc3hands303::Shotgun318(profile)||fc3hands303::Other319(profile)||profile==10))continue;
  for(int owner:{0,1}){
   fc3grab333::Publish({});g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=g_pistolReload.chamberReady=true;g_reload={};g_manualReload={};g_reloadContact={};g_reloadAlignment=false;g_pistolMenu.open=false;g_twoHand=false;g_pumpStage=0;g_chambered=true;g_slide=g_slideGrab=0;
   g_spas={};g_spas.clip=-1;g_lever={};g_lever.clip=-1;g_lever.enabled=g_lever.chambered=true;g_revolver={};g_revolver.clip=-1;g_r700={};g_r700.clip=-1;g_bow={};g_bow.epoch=g_armPoseEpoch;g_bowVisualRoot58={};g_rpg={};g_gm94={};g_gm94Clip={};g_flamer={};g_flamerClip={};g_flare={};g_shotgunSkin318={};g_otherSkin319={};
   fc3pad::SetBowInput(false,0);fc3pad::SetMagazineHandling(false,false);fc3pad::BlockFire(false);fc3pad::SetGameplayControls(true);SetRenderNative319(profile==10?1:0);SetRenderHands303(owner,false,g_armPoseEpoch);SetRenderReceiver309(-1);PublishSidearmControls315(0,0);
   for(int s=0;s<2;++s){memset(physical[s],0,64);physical[s][0]=physical[s][5]=physical[s][10]=physical[s][15]=1;physical[s][3]=s?-.35f:.35f;physical[s][7]=-.2f;physical[s][11]=.5f;}
   for(int s=0;s<2;++s){fc3stereo::SetHandPoses(physical[0],physical[1]);cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw[s]),raw[s]);Check(BuildArmPalette(context,s)!=nullptr,"333 actual weapon/owner GPU arm palette");}
   // Refresh the permission boundary, without changing the captured geometry.
   SetRenderHands303(owner,false,g_armPoseEpoch);PublishHandleBinding333();auto b=fc3grab333::Read();
   if(!b.enabled||b.busy)printf("BIND333 profile%d owner%d enabled%d busy%d\n",profile,owner,b.enabled,b.busy);
   Check(b.mode&&b.enabled&&!b.busy&&b.entity==100&&b.profile==profile,"333 every inventory family publishes an idle primary handle");
   Mat4 root={},grip={};const bool ok=profile==10?BowRootBody(root):fc3hands303::Shotgun318(profile)?ShotgunRoot318(root)&&ShotgunGrip318(grip):ControllerCurrentRoot124(root,grip);Check(ok,"333 native weapon root and grip bind available");
   const auto expected=profile==10?Apply(Multiply(root,BowMat(kBowLeftHandFromRoot)),{-.6781,.0519,1.0387}):Apply(Multiply(root,grip),{.6781,.0519,1.0387});Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[owner][i];
   const auto expectedPhysical=Apply(root,A2000ChargePoint336(Apply(profile==10?BowMat(kBowLeftHandFromRoot):grip,profile==10?V3{-.6781,.0519,1.0387}:V3{.6781,.0519,1.0387})));
   const auto actual=Apply(controller,{b.handle[0]*g_worldScale,b.handle[1]*g_worldScale,-b.handle[2]*g_worldScale});Check(Length(Sub(actual,expectedPhysical))<.0001,"333 controller-local grab follows actual native weapon firing handle in either hand");
   Check(ConsumeNativeHandle333(owner),"333 actual native renderer handle packet is consumed by real tracked-controller path and transfers immediately");
   const auto t=fc3hands303::TransferContact307();Check(t.enabled&&!t.contact&&!t.modifier321&&!t.rearOnly316,"333 production publisher cannot arm any old handover gesture");
   if(PistolModel()){g_pistolReload.magazinePresent=false;g_pistolReload.chamberReady=false;g_pistolReload.needsRack=g_pistolReload.rackMode=true;PublishHandleBinding333();Check(!fc3grab333::Read().busy,"333 missing magazine or chamber readiness alone cannot block hand switching");g_pistolReload.needsRack=g_pistolReload.rackMode=false;}
   if(profile==39){
    auto& p=g_pistolReload;p.insertActive=true;p.insertAt=GetTickCount64()-500;p.magazinePresent=true;p.needsRack=p.rackMode=true;
    PublishHandleBinding333();Check(!fc3grab333::Read().busy,"334 Skorpion completed seating cache and waiting charge do not lock handle");
    p.rackHeld=true;PublishHandleBinding333();Check(fc3grab333::Read().busy,"334 active Skorpion charge retains reload grip");p.rackHeld=false;
    p.rackReturning=true;PublishHandleBinding333();Check(fc3grab333::Read().busy,"334 charging return retains transaction");p.rackReturning=p.rackMode=p.needsRack=false;
    PublishHandleBinding333();Check(!fc3grab333::Read().busy&&ConsumeNativeHandle333(owner),"334 completed Skorpion charge permits real XR primary handle transfer despite insertActive cache");
    SetRenderHands303(owner,false,g_armPoseEpoch);
    p.insertAt=GetTickCount64();PublishHandleBinding333();Check(fc3grab333::Read().busy,"334 finite Skorpion seating animation still prevents premature transfer");
    p.insertAt=GetTickCount64()-500;p.magazinePresent=false;PublishHandleBinding333();Check(!fc3grab333::Read().busy,"334 removed magazine with cached insert does not lock handle");
    p={};p.enabled=true;p.basis=true;p.magazinePresent=false;g_pistolGuides.ammoPickup=2;
    fc3pad::Controllers c={};c.active=true;c.leftGrip=1;c.ammoPouch334=true;
    TickPistolLoose(c,GetTickCount64());Check(p.held&&!p.magazinePresent&&!p.pending,"334 Skorpion current physical pouch creates magazine even with saved button-only pickup");
    p={};p.enabled=true;p.basis=true;p.magazinePresent=false;c.ammoPouch334=false;TickPistolLoose(c,GetTickCount64());Check(!p.held,"334 empty-space grip still creates no Skorpion ammo");
    p={};p.enabled=p.magazinePresent=p.chamberReady=true;
    puts("PASS:334 Skorpion seating-charge-transfer lifecycle and actual physical magazine pickup, both owners");
   }
   g_reload.held=true;PublishHandleBinding333();Check(fc3grab333::Read().busy&&!fc3hands303::TransferContact307().allowed,"333 active carried reload part prevents transfer for every family");g_reload.held=false;fc3grab333::Publish({});
   printf("PASS:333 actual GPU/native handle profile%d owner%d,controller-space binding,legacy-method exclusion,empty-weapon and active-reload distinction\n",profile,owner);++cases;
  }
 }

 SetRenderNative319(0);fc3grab333::Publish({});g_weaponMechanicProfile=20;g_weaponMechanicFrame=g_frame;fc3aim::SetVehicleStateForTest(0);fc3aim::SetNativeActionForTest128(0);g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=true;g_pistolMenu.open=false;
 g_throw144={};g_throw144.enabled=true;fc3pad::Controllers input={};input.active=true;input.rightGrip=1;input.b=true;fc3pad::Publish(input);TickThrowable144(true);Check(!g_throw144.owned&&!g_throw144.drawing,"333 actual throwable tick cannot start grip+B drawing");
 g_throw144.kind=2;g_throw144.seen=g_frame;TickThrowable144(true);Check(!g_throw144.owned,"333 visible native Molotov cannot activate paused handling");
 g_throw144.owned=true;g_throw144.kind=1;TickThrowable144(false);Check(!g_throw144.owned,"333 paused grenade ownership clears even without XR tracking");
 puts("PASS:333 production grenade/Molotov draw,takeover and ownership are paused;C4/mine implementation retained");

 printf("PASS:333 %u model/owner handle bindings through real arm palettes and production publisher;offline only\n",cases);
}
