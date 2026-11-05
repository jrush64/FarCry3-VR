#include "reload_pose341_assert.inl"
extern bool PublishedForegripSupport343();
void TestAk316(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16],int profile=8){
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=g_frame;g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=g_pistolReload.chamberReady=true;g_reload={};g_reloadAlignment=false;g_pistolMenu.open=false;g_twoHand=false;
 fc3pad::SetGameplayControls(true);fc3pad::SetGripContext(true,true);fc3pad::BlockFire(false);fc3pad::SetMagazineHandling(true,false);
 float raw[2][576]={};for(int h=0;h<2;++h){for(int b=0;b<48;++b)StoreBone(raw[h],b,HandIdentity());memcpy(raw[h],kNeutralArmView[h],sizeof(kNeutralArmView[h]));}
 for(int owner:{0,1}){
  printf("RIFLE317 profile%d owner%d\n",profile,owner);
  g_twoHand=false;auto& p=g_pistolReload;p={};p.enabled=p.magazinePresent=p.chamberReady=true;p.rootEpoch=g_armPoseEpoch;p.rootFrame=g_frame;
  for(int h=0;h<2;++h){memset(physical[h],0,64);physical[h][0]=physical[h][5]=physical[h][10]=physical[h][15]=1;physical[h][3]=h?-.35f:.35f;physical[h][7]=-.2f;physical[h][11]=.5f;}
  SetRenderHands303(owner,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);
  for(int side=0;side<2;++side){cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw[side]),raw[side]);Check(BuildArmPalette(context,side)!=nullptr,"316 both anatomical AK arms build");}
  Mat4 root={},grip={};Check(ControllerCurrentRoot124(root,grip),"316 exact native AK root exists");p.rootBody=root;p.parentValid=true;p.parentEpoch=g_armPoseEpoch;p.rootFrame=g_frame;p.rootEpoch=g_armPoseEpoch;
  if(owner){float skin[576];Read(device,context,g_armPaletteCb[1],skin);Mat4 ctrl={};for(int i=0;i<16;++i)ctrl.m[i]=physical[1][i];const V3 actual=Apply(BoneMatrix(skin,2),{-.6781,.0519,1.0387}),target=Apply(Multiply(ctrl,g_freeHandAttachment[1].relative),{-.6781,.0519,1.0387});Check(Length(Sub(actual,target))<.002,"316 AK firing grip fits actual physical left palm");}
  const int other=1-owner;
  auto buildFree=[&](){fc3stereo::SetHandPoses(physical[0],physical[1]);cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw[other]),raw[other]);Check(BuildArmPalette(context,other)!=nullptr,"316 receiving/reload arm builds on correct physical hand");};
  auto sphere=[&](){Mat4 ctrl={};for(int i=0;i<16;++i)ctrl.m[i]=physical[other][i];return TransferSphere307(Multiply(ctrl,g_freeHandAttachment[other].relative),other);};
  auto moveTo=[&](const V3& local){Check(ControllerCurrentRoot124(root,grip),"316 root remains controller-owned");const V3 delta=Sub(Apply(root,local),sphere());physical[other][3]+=float(delta.x);physical[other][7]+=float(delta.y);physical[other][11]+=float(delta.z);buildFree();};
  const V3 rear=Apply((*CurrentRifle().hand),{.6781,.0519,1.0387}),front=Apply((*CurrentRifle().support),{-.6781,.0519,1.0387});
  moveTo(front);Publish1911Transfer307();auto t=fc3hands303::TransferContact307();Check(t.enabled&&t.allowed&&!t.contact,"316 foregrip proximity cannot arm AK transfer");
  g_autoLongGunSupport=true;
  for(int align=0;align<2;++align){V3 target={},palm={};Check(RifleSupportContact234(target,palm),"316 native foregrip contact available");const V3 d=Sub(target,palm);physical[other][3]+=float(d.x);physical[other][7]+=float(d.y);physical[other][11]+=float(d.z);buildFree();}
  UpdateTwoHand();Check(g_twoHand,"316 existing AK foregrip support still acquires in either ownership");
  if(owner&&(profile==14||profile==31)){PublishHandleBinding333();Check(PublishedForegripSupport343(),"343 real XR resolver classifies corrected authored forend as support,not transfer or reload");fc3grab333::Publish({});}
  else{Publish1911Transfer307();Check(!fc3hands303::TransferContact307().contact,"316 active foregrip hold is still not a transfer entry");}
  buildFree();if(owner){float skin[576];Read(device,context,g_armPaletteCb[0],skin);{PhysicalArmScope303 scope;Mat4 desired={};Check(RightAkSupport316(scope,0,desired),"316 right support skin follows real native foregrip");const Mat4 actual=BoneMatrix(skin,2);for(int i=0;i<16;++i)Check(fabs(actual.m[i]-desired.m[i])<2e-4,"316 GPU right wrist matches native foregrip binding");}}
  if(owner){float audit344[576];Read(device,context,g_armPaletteCb[other],audit344);Check(ControllerCurrentRoot124(root,grip),"344 rifle audit root current");CheckReloadPose341(audit344,root,*CurrentRifle().support,CurrentRifle().supportGrip,1,true);printf("PASS:344 rifle profile%d all18 uploaded support bones match authored mirrored grasp\n",profile);}
  if(profile==14||profile==31){
   Check(ControllerCurrentRoot124(root,grip),"343 current F1/P416 support root");float support343[576];Read(device,context,g_armPaletteCb[other],support343);
   if(owner)CheckReloadPose341(support343,root,*CurrentRifle().support,CurrentRifle().supportGrip,1,true);
   else{const auto expected343=Multiply(ArmBodyToCamera(),Multiply(root,*CurrentRifle().support));const auto actual343=BoneMatrix(support343,2);for(int i=0;i<12;++i)Check(fabs(actual343.m[i]-expected343.m[i])<.003,"343 original right-dominant native support wrist unchanged");}
   auto contact343=Apply(*CurrentRifle().support,{-.6781,.0519,1.0387});if(owner)contact343.x=-contact343.x;V3 point343={},palm343={};Check(RifleSupportContact234(point343,palm343)&&Length(Sub(point343,Apply(root,contact343)))<.003,"343 F1/P416 support contact uses correct dominant-hand face");
   PublishHandleBinding333();const auto binding343=fc3grab333::Read();Check(binding343.enabled&&binding343.support,"343 actual F1/P416 foregrip binding available");Mat4 controller343={};for(int i=0;i<16;++i)controller343.m[i]=physical[owner][i];const double scale343=(std::max)(.1f,g_worldScale);const V3 actualPoint343=Apply(controller343,{binding343.foregrip[0]*scale343,binding343.foregrip[1]*scale343,-binding343.foregrip[2]*scale343});auto local343=Apply(*CurrentRifle().support,{-.6781,.0519,1.0387});if(owner)local343.x=-local343.x;V3 expectedPoint343=Apply(root,local343);SupportGrabAdjustPoint214(expectedPoint343);Check(Length(Sub(actualPoint343,expectedPoint343))<.003,"343 published foregrip trigger follows exact authored support grasp");fc3grab333::Publish({});
   printf("PASS:343 F1/P416 profile%d owner%d actual GPU support wrist/fingers,native contact and published trigger;right-handed reference retained\n",profile,owner);
  }
  g_twoHand=false;moveTo(rear);Publish1911Transfer307();t=fc3hands303::TransferContact307();Check(t.allowed&&t.contact,"316 rear handle contact authorizes AK transfer in both directions");
  p.insertActive=true;p.insertAt=GetTickCount64();Publish1911Transfer307();Check(!fc3hands303::TransferContact307().allowed,"317 native fill alone cannot hand over during unfinished insertion animation");p.insertAt-=281;Publish1911Transfer307();Check(fc3hands303::TransferContact307().allowed,"317 original completed insertion threshold releases handover");p.insertActive=false;
  PublishSidearmControls315(0,0);SetRenderReceiver309(other);PublishSidearmControls315(0,0,other==0?1.f:0.f,other==1?1.f:0.f);Check(fc3pad::Latest().leftGrip<.1f,"316 armed rear transfer grip cannot also operate magazine");
  SetRenderReceiver309(-1);PublishSidearmControls315(0,0,other==0?1.f:0.f,other==1?1.f:0.f);Check(fc3pad::Latest().leftGrip<.1f,"316 canceled transfer cannot inherit held grip as new reload grab");PublishSidearmControls315(0,0);PublishSidearmControls315(0,0,other==0?1.f:0.f,other==1?1.f:0.f);Check(fc3pad::Latest().leftGrip>.9f,"316 fresh free-hand grip restores normal reload input after release");PublishSidearmControls315(0,0);
  fc3hands303::State state;state.initialized=true;state.empty=false;state.player=123;state.entity=100;state.slot=0;state.hand=owner;state.reference=state.epoch=1;state.last=1000;state.down[0]=state.down[1]=false;state.ready[0]=state.ready[1]=true;
  fc3hands303::Input in;in.context=in.canChange=in.inventory=in.sphereTransfer307=true;in.sphereContact307=t.contact;in.transferAllowed307=t.allowed;in.player=123;in.equipped=in.entities[0]=100;in.reference=in.epoch=1;in.tracked[0]=in.tracked[1]=true;in.now=1010;in.grip[other]=1;
  state.Step(in);Check(state.receiver==other,"316 rear grip arms receiving hand");in.now+=10;in.receivingPosition309[other][0]=in.receivingRelative309[other][0]=other?-.08f:.08f;const auto e=state.Step(in);Check(e.transfer&&state.hand==other&&state.entity==100&&e.drawSlot<0,"316 lateral pull transfers same AK without native re-equip");
  PublishSidearmControls315(0,0);PublishSidearmControls315(owner?0:1,owner?1:0);Check(fc3pad::FireOutput()>0,"316 owning physical AK trigger fires");PublishSidearmControls315(0,0);PublishSidearmControls315(owner?1:0,owner?0:1);Check(fc3pad::FireOutput()==0,"316 support trigger cannot fire AK");PublishSidearmControls315(0,0);
  moveTo({g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]});Publish1911Transfer307();Check(!fc3hands303::TransferContact307().contact,"316 magazine insertion contact cannot transfer AK");moveTo(PistolRackGuide());Publish1911Transfer307();Check(!fc3hands303::TransferContact307().contact,"316 charging handle contact cannot transfer AK");
  // Actual contact recording and proper native magazine attachment in either hand.
  p.held=true;moveTo(rear);Publish1911Transfer307();Check(!fc3hands303::TransferContact307().allowed,"316 held ammunition blocks transfer");float skin[576];Read(device,context,g_armPaletteCb[other],skin);const Mat4 wrist=BoneMatrix(skin,2);
  if(owner){Check(Length(Sub(PistolHandGuide(),Apply(wrist,{.591-g_pistolGuides.hand[0],.011+g_pistolGuides.hand[1],1.149+g_pistolGuides.hand[2]})))<.002,"316 AK magazine sphere follows physical right hand");const Mat4 expected=Multiply(wrist,Multiply(SidearmMirror315(),Multiply(MagazineHeldFromHand(),SidearmMirror315())));for(int i=0;i<16;++i)Check(fabs(p.hand.m[i]-expected.m[i])<2e-4,"316 AK held magazine has proper mirrored native attachment");}
  else Check(!Left1911Reload312(),"316 original right AK reload contact path stays canonical");
  {Mat4 qi={};Check(Invert(ArmBodyToCamera(),qi),"316 view/body transform invertible");const Mat4 renderer=AkCarried316(p.palmBody,MagazineHeldFromHand()),heldBody=Multiply(qi,p.hand);for(int i=0;i<16;++i)Check(fabs(renderer.m[i]-heldBody.m[i])<2e-4,"316 native extracted-magazine renderer agrees with actual held-magazine pose in both owners");}
  p.held=false;p.magazinePresent=true;p.rackMode=p.rackArmed=true;
  moveTo(PistolRackGuide());PublishSidearmControls315(0,0);PublishSidearmControls315(0,0,other==0?1.f:0.f,other==1?1.f:0.f);
  AdvancePistolRack(fc3pad::Latest(),GetTickCount64());Check(p.rackHeld,"316 existing enabled charging-hand mode acquires physical free hand");
  const V3 charge=PistolRackGuide();moveTo({charge.x,charge.y-.05,charge.z});AdvancePistolRack(fc3pad::Latest(),GetTickCount64());Check(p.rackRear&&p.slidePull>=.04,"316 actual free-hand contact travels native AK charging rail");
  if(owner){
   Mat4 nativeHand={};float nativeFingers[204];
   if(RifleChargeClipSample(p.slidePull/.05,nativeHand,nativeFingers)){
    buildFree();float chargeSkin[576];Read(device,context,g_armPaletteCb[0],chargeSkin);
    Mat4 qi={},ri={};Check(Invert(ArmBodyToCamera(),qi)&&Invert(p.rootBody,ri),"317 native charge reference frames invertible");
    const Mat4 local=Multiply(ri,Multiply(qi,BoneMatrix(chargeSkin,2)));
    double handleX=CurrentRifle().partRest[0]->m[3];if(profile==28||profile==25||profile==21){const auto parts=CurrentSkinnedParts();handleX=PistolRigidBlend(*parts.handleRest,*parts.handleBack,p.slidePull/.05).m[3];}
    Check(fabs(local.m[3]-((fc3mirror338::Supported(profile)?0:2*handleX)-nativeHand.m[3]))<2e-4,"336 A2000 GPU charging wrist uses opposite-side rail; other families preserve actual native side");
    const Mat4 wrist=BoneMatrix(chargeSkin,2),mirror=SidearmMirror315();for(int b=3;b<=19;++b){const Mat4 expected=Multiply(wrist,Multiply(mirror,Multiply(BoneMatrix(nativeFingers,b-3),mirror))),actual=BoneMatrix(chargeSkin,b);for(int i=0;i<16;++i)Check(fabs(expected.m[i]-actual.m[i])<2e-4,"317 GPU right charging fingers follow that family's native clip");}
   }
  }
  PublishSidearmControls315(0,0);const auto released=GetTickCount64();AdvancePistolRack(fc3pad::Latest(),released);AdvancePistolRack(fc3pad::Latest(),released+101);Check(!p.rackHeld&&!p.rackReturning&&p.chamberReady,"316 release returns original charging mechanism and clears grip lease");
  p.held=false;p.pending=true;Publish1911Transfer307();Check(!fc3hands303::TransferContact307().allowed,"316 pending native insertion blocks transfer");p.pending=false;p.rackHeld=true;Publish1911Transfer307();Check(!fc3hands303::TransferContact307().allowed,"316 operating charging handle blocks transfer");p.rackHeld=false;
 }
 g_weaponMechanicProfile=38;g_weaponMechanicFrame=g_frame;Publish1911Transfer307();Check(!fc3hands303::TransferContact307().enabled,"316 other long guns remain on existing handover behavior");g_weaponMechanicProfile=20;
 printf("PASS:317 profile%d actualGPU grips/support/magazines/native charge side/fingers,rear transfer,reservation,both owners;315 preserved\n",profile);
}
