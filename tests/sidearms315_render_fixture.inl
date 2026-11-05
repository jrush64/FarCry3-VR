extern void PublishSidearmControls315(float,float,float=0,float=0,bool=false,bool=false);
void TestSidearms315(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 const int profiles[]={1,12,16,20,39,42};
 fc3pad::SetGameplayControls(true);fc3pad::SetSwapBX(false);fc3pad::SetGripOnlyAmmo(true);fc3pad::SetGripContext(true,true);fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);
 for(int profile:profiles){
  printf("SIDEARM315 profile%d\n",profile);
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=g_frame;g_pistolReload={};g_pistolReload.enabled=true;g_pistolReload.magazinePresent=true;g_pistolReload.chamberReady=true;
  g_reload={};g_reloadAlignment=false;g_pistolMenu.open=false;g_revolver={};g_revolver.clip=-1;g_flare={};g_nativeSupport310={};g_twoHand=false;
  for(int h=0;h<2;++h){memset(physical[h],0,64);physical[h][0]=physical[h][5]=physical[h][10]=physical[h][15]=1;physical[h][3]=h?-.35f:.35f;physical[h][7]=-.2f;physical[h][11]=.5f;}
  SetRenderHands303(1,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);
  float nativeRight[576]={};for(int b=0;b<48;++b)StoreBone(nativeRight,b,HandIdentity());memcpy(nativeRight,kNeutralArmView[0],sizeof(kNeutralArmView[0]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(nativeRight),nativeRight);
  Check(BuildArmPalette(context,0)!=nullptr,"315 actual free-right palette records native grip and reload contact");float right[576];Read(device,context,g_armPaletteCb[0],right);
  float nativeLeft[576]={};for(int b=0;b<48;++b)StoreBone(nativeLeft,b,HandIdentity());memcpy(nativeLeft,kNeutralArmView[1],sizeof(kNeutralArmView[1]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(nativeLeft),nativeLeft);
  Check(BuildArmPalette(context,1)!=nullptr,"315 actual holding-left palette builds for each sidearm");float left[576];Read(device,context,g_armPaletteCb[1],left);
  Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=physical[1][i];
  const V3 tracked=Apply(Multiply(controller,g_freeHandAttachment[1].relative),{-.6781,.0519,1.0387});
  Check(Length(Sub(tracked,Apply(BoneMatrix(left,2),{-.6781,.0519,1.0387})))<.002,"315 each sidearm native grip reflected onto actual tracked left palm");
  const Mat4 physicalRight=BoneMatrix(right,2),mirror=SidearmMirror315();
  if(RevolverModel()){
   V3 actual={};Check(RevolverPalmBody(actual),"315 revolver free-right contact published");
   const auto& R=g_revolver;const V3 expected=Apply(physicalRight,{.591-R.hand[0],.011+R.hand[1],1.149+R.hand[2]});Check(Length(Sub(actual,expected))<.002,"315 revolver reload sphere uses actual right palm");
   const Mat4 carried=RevolverLoaderBody(),expectedBody=Multiply(physicalRight,Multiply(mirror,Multiply(Multiply(RevolverRoundsFromWrist(),kRevolverLoaderFromRounds),mirror)));
   for(int i=0;i<16;++i)Check(fabs(carried.m[i]-expectedBody.m[i])<2e-4,"315 loader has proper mirrored attachment to physical right wrist");
   {extern void StallRender314();StallRender314();const Mat4 paused=RevolverLoaderBody();for(int i=0;i<16;++i)Check(fabs(paused.m[i]-carried.m[i])<2e-4,"315 paused input cannot invert carried loader model");SetRenderHands303(1,false,g_armPoseEpoch);}

  }else if(FlareModel()){
   V3 actual={};Check(FlarePalmBody(actual),"315 flare free-right contact published");const V3 expected=Apply(physicalRight,{.591-kFlareHand[0],.011+kFlareHand[1],1.149+kFlareHand[2]});Check(Length(Sub(actual,expected))<.002,"315 flare reload sphere uses actual right palm");
   const Mat4 shell=SidearmCarried315(g_flare.leftWrist,FlareKeyMat(kFlareShellFromLeftWrist)),expectedBody=Multiply(physicalRight,Multiply(mirror,Multiply(FlareKeyMat(kFlareShellFromLeftWrist),mirror)));
   for(int i=0;i<16;++i)Check(fabs(shell.m[i]-expectedBody.m[i])<2e-4,"315 flare shell proper attachment follows physical right wrist");
   {extern void StallRender314();StallRender314();const Mat4 paused=SidearmCarried315(g_flare.leftWrist,FlareKeyMat(kFlareShellFromLeftWrist));for(int i=0;i<16;++i)Check(fabs(paused.m[i]-shell.m[i])<2e-4,"315 paused input cannot invert carried flare model");SetRenderHands303(1,false,g_armPoseEpoch);}

  }else{
   const V3 expected=Apply(physicalRight,{.591-g_pistolGuides.hand[0],.011+g_pistolGuides.hand[1],1.149+g_pistolGuides.hand[2]});Check(Length(Sub(PistolHandGuide(),expected))<.002,"315 magazine contact for each sidearm follows physical right hand");
  }
  // Actual game-thread publisher, followed by actual weapon-role mapping.
  Publish1911Transfer307();Check(fc3hands303::TransferContact307().enabled,"315 real transfer publisher recognizes every sidearm family");
  PublishSidearmControls315(0,0);PublishSidearmControls315(0,1);Check(fc3pad::Latest().rightTrigger>.9f&&fc3pad::FireOutput()>0,"315 left trigger fires while sidearm is left-owned");
  PublishSidearmControls315(0,0);PublishSidearmControls315(1,0);Check(fc3pad::Latest().leftTrigger>.9f&&fc3pad::FireOutput()==0,"315 free-right trigger cannot fire left-owned sidearm");
  PublishSidearmControls315(0,0);PublishSidearmControls315(0,0,1,0,true);Check(fc3pad::Latest().x&&fc3pad::Latest().leftGrip>.9f,"315 physical right-grip+B reaches original support-hand X reload action");PublishSidearmControls315(0,0);
  // Confirm entry from real support geometry or the real configured reload sphere.
  Mat4 root315={},grip315={};Check(ControllerCurrentRoot124(root315,grip315),"315 actual native weapon root available for contact");
  if(fc3hands303::SupportedSidearm315(profile)){
   g_autoPistolSupport=g_autoLongGunSupport=true;auto& native=g_supportLocal236[profile];native.valid=true;native.epoch=g_armPoseEpoch;native.point={.02,-.013,-.015};
   for(int align=0;align<2;++align){
    V3 target={},palm={};if(!LeftSupportContact311(target,palm)){V3 local=native.point;local.x=-local.x;target=Apply(root315,local);Check(FreeHandPalm(1,palm),"315 free support palm resolves");}
    const V3 delta=Sub(target,palm);physical[0][3]+=float(delta.x);physical[0][7]+=float(delta.y);physical[0][11]+=float(delta.z);fc3stereo::SetHandPoses(physical[0],physical[1]);
   }
   UpdateTwoHand();Check(g_twoHand,"315 native two-hand support acquisition works for each magazine sidearm");Publish1911Transfer307();Check(fc3hands303::TransferContact307().contact,"315 existing native support authorizes transfer for each supported sidearm");
   SetRenderHands303(0,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);g_twoHand=true;g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);Check(g_frameGunValid,"344 pistol native gun transform available");
   cb[3]->SetPrivateData(kCbCopyGuid,sizeof(nativeLeft),nativeLeft);Check(BuildArmPalette(context,1)!=nullptr&&g_nativeSupport310.valid&&g_nativeSupport310.confirmed,"344 capture actual right-dominant support reference for each pistol");
   SetRenderHands303(1,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);g_twoHand=true;g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);
   cb[3]->SetPrivateData(kCbCopyGuid,sizeof(nativeRight),nativeRight);Check(BuildArmPalette(context,0)!=nullptr,"344 each two-hand pistol support builds");
   Check(ControllerCurrentRoot124(root315,grip315)&&g_nativeSupport310.valid,"344 each pistol captured support current");float support344[576];Read(device,context,g_armPaletteCb[0],support344);
   for(int bone344=2;bone344<=19;++bone344){const auto expected344=Multiply(ArmBodyToCamera(),Multiply(root315,Multiply(mirror,Multiply(g_nativeSupport310.local[bone344-2],mirror))));const auto actual344=BoneMatrix(support344,bone344);for(int k=0;k<16;++k)Check(fabs(actual344.m[k]-expected344.m[k])<.003,"344 every two-hand pistol uploaded support wrist/fingers follow reflected native grasp");}
   printf("PASS:344 pistol profile%d all18 left-dominant support bones verified\n",profile);
   if(g_nativeSupport310.valid){const int saved=g_weaponMechanicProfile;g_weaponMechanicProfile=profile==20?12:20;{PhysicalArmScope303 scope;Mat4 desired={};Check(!Right1911Support310(scope,0,desired),"315 reused entity cannot replay another profile's cached support skin");}g_weaponMechanicProfile=saved;}
   g_twoHand=false;
  }else{
   const float* hand=RevolverModel()?g_revolver.hand:FlareModel()?kFlareHand:g_pistolGuides.hand;
   const V3 target=Apply(root315,Apply(grip315,{.6781,.0519,1.0387}));
   V3 actual={};if(RevolverModel())Check(RevolverPalmBody(actual),"315 revolver contact exists");else if(FlareModel())Check(FlarePalmBody(actual),"315 flare contact exists");else actual=PistolHandGuide();
   const V3 delta=Sub(target,actual);physical[0][3]+=float(delta.x);physical[0][7]+=float(delta.y);physical[0][11]+=float(delta.z);fc3stereo::SetHandPoses(physical[0],physical[1]);cb[3]->SetPrivateData(kCbCopyGuid,sizeof(nativeRight),nativeRight);Check(BuildArmPalette(context,0)!=nullptr,"315 actual receiving arm reaches handle");
   Publish1911Transfer307();Check(fc3hands303::TransferContact307().contact,"315 reload contact sphere at actual handle authorizes unsupported sidearm transfer");
   if(!SkorpionModel()){
   // Open/empty the real mechanism, take ammunition with physical right grip+B,
   // then bring that same physical right-hand sphere to the actual insertion box.
   physical[0][3]+=.8f;fc3stereo::SetHandPoses(physical[0],physical[1]);Check(BuildArmPalette(context,0)!=nullptr,"315 right reload hand moves away from handle");
   if(RevolverModel()){g_revolver.open=g_revolver.empty=true;g_revolver.angle=100;g_revolver.frameBody=root315;g_revolver.frameFrame=g_frame;}
   else{g_flare.open=g_flare.known=true;g_flare.chamber=0;g_flare.angle=kFlareBarrelOpenDeg;g_flare.frameBody=root315;g_flare.frameFrame=g_frame;}
   auto tick315=[&](){if(RevolverModel()){g_revolver.lastTick=GetTickCount64()-100;TickRevolver(true);}else{g_flare.lastTick=GetTickCount64()-100;TickFlare(true);}};
   PublishSidearmControls315(0,0);tick315();PublishSidearmControls315(0,0,1,0,true);tick315();
   Check(RevolverModel()?g_revolver.carrying:g_flare.carrying,"315 actual physical right grip+B picks up speedloader or flare shell");
   V3 current={};if(RevolverModel())RevolverPalmBody(current);else FlarePalmBody(current);
   Mat4 box=FlareModel()?FlareBox(1):HandIdentity();const V3 local=RevolverModel()?RevolverBoxCentre(0):V3{box.m[3],box.m[7],box.m[11]};const V3 insert=Apply(root315,local),move=Sub(insert,current);
   physical[0][3]+=float(move.x);physical[0][7]+=float(move.y);physical[0][11]+=float(move.z);fc3stereo::SetHandPoses(physical[0],physical[1]);Check(BuildArmPalette(context,0)!=nullptr,"315 right ammo hand reaches insertion box");
   PublishSidearmControls315(0,0,1,0);tick315();Check(RevolverModel()?RevolverClipPlaying():g_flare.pending,"315 right-hand contact starts native speedloader insertion clip or flare fill request");
   PublishSidearmControls315(0,0);
   }
  }
  // Grip and pull in either direction uses the same production state machine.
  for(int owner=0;owner<2;++owner){
   fc3hands303::State state;state.initialized=true;state.empty=false;state.player=123;state.entity=100;state.slot=0;state.hand=owner;state.reference=1;state.epoch=1;state.last=1000;state.down[0]=state.down[1]=false;state.ready[0]=state.ready[1]=true;
   fc3hands303::Input in;in.inventory=in.context=in.canChange=in.sphereTransfer307=in.sphereContact307=in.transferAllowed307=true;in.player=123;in.equipped=in.entities[0]=100;in.reference=in.epoch=1;in.tracked[0]=in.tracked[1]=true;in.now=1010;
   const int receiver=1-owner;in.grip[receiver]=1;state.Step(in);Check(state.receiver==receiver,"315 fresh receiving grip arms each sidearm in either direction");in.now+=10;const float pull=receiver?-.08f:.08f;in.receivingRelative309[receiver][0]=in.receivingPosition309[receiver][0]=pull;
   const auto event=state.Step(in);Check(event.transfer&&state.hand==receiver&&state.entity==100&&event.drawSlot<0,"315 lateral receiving pull transfers same native sidearm without re-equip");
  }
  if(RevolverModel())g_revolver.pending=true;else if(FlareModel())g_flare.pending=true;else g_pistolReload.held=true;
  Publish1911Transfer307();Check(!fc3hands303::TransferContact307().allowed,"315 each reload transaction preempts hand transfer");
  SetRenderHands303(0,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);Check(!LeftSidearm315(),"315 returned right ownership restores canonical reload path");
 }
 g_weaponMechanicProfile=38;g_weaponMechanicFrame=g_frame;Publish1911Transfer307();Check(!fc3hands303::TransferContact307().enabled,"315 long guns retain their existing handover policy");g_weaponMechanicProfile=20;
 puts("PASS:315 six sidearm families actualGPU left grips/free-right contacts/ammo attachments,physical firing/reload buttons,bidirectional transfer and busy/long-gun isolation");
}

