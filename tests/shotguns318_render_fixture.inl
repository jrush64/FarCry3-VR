#include "reload_pose341_assert.inl"
void TestShotguns318(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 float raw[2][576]={};for(int s=0;s<2;++s){for(int b=0;b<48;++b)StoreBone(raw[s],b,HandIdentity());memcpy(raw[s],kNeutralArmView[s],sizeof(kNeutralArmView[s]));}
 for(int profile:{0,3,4,5,6,34})for(int owner:{0,1}){
  printf("SHOTGUN318 profile%d owner%d\n",profile,owner);
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_reload={};g_reload.shotgunFrame=g_frame;g_reloadContact={};g_manualReload={};g_reloadAlignment=false;g_pistolReload={};g_pistolMenu.open=false;g_twoHand=false;g_chambered=true;g_pumpStage=0;g_slide=g_slideGrab=0;g_shotgunSkin318={};g_m133SupportLoaded56=true;g_m133Support56.positioned=false;g_spas={};g_spas.clip=-1;g_lever={};g_lever.clip=-1;g_lever.enabled=true;g_lever.chambered=true;
  fc3pad::SetGameplayControls(true);fc3pad::SetGripContext(true,true);fc3pad::SetMagazineHandling(false,false);fc3pad::BlockFire(false);SetRenderReceiver309(-1);PublishSidearmControls315(0,0);
  for(int s=0;s<2;++s){memset(physical[s],0,64);physical[s][0]=physical[s][5]=physical[s][10]=physical[s][15]=1;physical[s][3]=s?-.35f:.35f;physical[s][7]=-.2f;physical[s][11]=.5f;}
  SetRenderHands303(owner,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);
  auto build=[&](int side){fc3stereo::SetHandPoses(physical[0],physical[1]);cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw[side]),raw[side]);Check(BuildArmPalette(context,side)!=nullptr,"318 actual anatomical GPU arm builds");};
  build(0);build(1);Mat4 root={},grip={};Check(ShotgunRoot318(root)&&ShotgunGrip318(grip),"318 native shotgun grip root available");
  if(owner){float skin[576];Read(device,context,g_armPaletteCb[1],skin);Mat4 wrist={};Check(ShotgunPhysicalWrist318(0,wrist),"318 actual left wrist available");const V3 target=Apply(wrist,{-.6781,.0519,1.0387}),actual=Apply(BoneMatrix(skin,2),{-.6781,.0519,1.0387});Check(Length(Sub(actual,target))<.003,"318 firing palm fits tracked physical left palm");}
  if(owner){
   g_twoHand=true;build(0);float skin[576];Read(device,context,g_armPaletteCb[0],skin);Mat4 nativeSupport={};
   if(profile==34)nativeSupport=SpasKeyMat(kSpasLeftSupportFromFrame);else if(profile==0)nativeSupport=kL1887LeftSupportFromFrame;else{Check(g_shotgunSkin318.support,"318 M133 native forend capture exists");nativeSupport=g_shotgunSkin318.left[0];}
   Check(ShotgunRoot318(root),"318 supported root ready");V3 localSupport341=Apply(nativeSupport,{-.6781,.0519,1.0387});localSupport341.x=-localSupport341.x;const V3 target=Apply(Multiply(ArmBodyToCamera(),root),localSupport341),actual=Apply(BoneMatrix(skin,2),{.6781,.0519,1.0387});Check(Length(Sub(target,actual))<.003,"318 physical right support palm stays on native forend,not opposite side");
   if(profile==0||profile==34||(profile>=3&&profile<=6)){PublishHandleBinding333();const auto binding341=fc3grab333::Read();Check(binding341.mode&&binding341.enabled&&binding341.support,"341 actual foregrip binding published");const double scale341=(std::max)(.1f,g_worldScale);Mat4 owner341={};for(int i=0;i<16;++i)owner341.m[i]=physical[1][i];const V3 bound341=Apply(owner341,{binding341.foregrip[0]*scale341,binding341.foregrip[1]*scale341,-binding341.foregrip[2]*scale341});V3 authored341=Apply(root,localSupport341);SupportGrabAdjustPoint214(authored341);Check(Length(Sub(bound341,authored341))<.003,"341 actual foregrip trigger and mirrored forend grasp share position");fc3grab333::Publish({});}

   if(profile==34){CheckReloadPose341(skin,root,SpasForendHand(),kSpasLeftIdleFingers,1,true);puts("PASS:342 SPAS left-dominant actual GPU wrist/fingers wrap native forend");}
   if(profile>=3&&profile<=6){Mat4 inverse342={};Check(Invert(g_shotgunSkin318.left[0],inverse342),"342 captured M133 forend invertible");float relative342[204];for(int b=3;b<=19;++b)StoreBone(relative342,b-3,Multiply(inverse342,g_shotgunSkin318.left[b-2]));CheckReloadPose341(skin,root,g_shotgunSkin318.left[0],relative342,1,true);puts("PASS:342 M133 family left-dominant actual GPU wrist/fingers wrap captured native forend");}
   const V3 raw=ShotgunCameraWrist318({99,99,99});Check(Length(Sub(raw,Apply(ArmBodyToCamera(),ControllerWrist(physical[0],0))))<1e-6,"318 aim and M133 pump use real opposite wrist in camera axes");
   g_twoHand=false;
  }
  const int other=1-owner;Mat4 wrist={};auto sphere=[&](){Mat4 ctrl={};for(int i=0;i<16;++i)ctrl.m[i]=physical[other][i];return Apply(Multiply(ctrl,g_freeHandAttachment[other].relative),{other?-.6781:.6781,.0519,1.0387});};
  for(int a=0;a<2;++a){Check(ShotgunRoot318(root),"318 rear root current");const V3 d=Sub(Apply(root,Apply(grip,{.6781,.0519,1.0387})),sphere());physical[other][3]+=float(d.x);physical[other][7]+=float(d.y);physical[other][11]+=float(d.z);build(other);}
  Publish1911Transfer307();auto t=fc3hands303::TransferContact307();Check(t.enabled&&t.allowed&&t.contact,"318 exact rear palm contact authorizes transfer both ways");
  if(profile==34)g_spas.pending=true;else if(profile==0)g_lever.hold=true;else g_reload.held=true;
  Publish1911Transfer307();Check(!fc3hands303::TransferContact307().allowed,"318 shell/pump/lever lease cannot become transfer");g_spas.pending=false;g_lever.hold=false;g_reload.held=false;
  const Mat4 attachment=profile==34?SpasShellFromWrist():profile==0?kL1887ShellFromHand:kCapturedShellToHand;
  Mat4 semantic=HandIdentity();semantic.m[3]=.4;
  const Mat4 old=Multiply(semantic,attachment),held=ShotgunHeld318(semantic,attachment,profile==0?0:1);
  if(!owner){for(int i=0;i<16;++i)Check(old.m[i]==held.m[i],"318 right-owner shell transform exactly unchanged");}
  else{
   Mat4 actual={};Check(ShotgunPhysicalWrist318(profile==0?0:1,actual),"318 carried shell actual wrist");const auto mirror=SidearmMirror315(),expected=Multiply(actual,Multiply(mirror,Multiply(attachment,mirror)));for(int i=0;i<16;++i)Check(fabs(held.m[i]-expected.m[i])<2e-5,"318 carried shell follows correct opposite anatomical wrist");
   const V3 motion=ShotgunMotion318(1,{99,99,99});Check(Length(Sub(motion,ControllerWrist(physical[0],0)))<1e-6,"318 pump reads physical right controller instead of pinned skin");
   if(profile>=3&&profile<=6){g_reload.held=true;g_reload.mode=1;build(0);Check(g_reloadContact.hand&&g_reloadContact.mirrored318,"318 M133 shell contact recorded from physical right draw");Mat4 shell={};Check(CapturedShellTarget(shell),"318 shell replay matrix available");Mat4 inv={};Check(Invert(ArmBodyToCamera(),inv),"318 camera inverse");Check(Length(Sub(Apply(Multiply(inv,shell),kCapturedShellCentre),g_reloadContact.shellBody))<.001,"318 replayed shell and insertion contact coincide");const auto saved=g_reloadContact.shellBody;build(1);Check(g_reloadContact.hand&&Length(Sub(saved,g_reloadContact.shellBody))<1e-8,"318 holding-left draw cannot overwrite shell contact");g_reload.held=false;}
   if(profile==34){g_spas.frameBody=root;g_spas.frameFrame=g_frame;V3 pos={};Check(SpasRealLeftFrame(pos),"318 SPAS raw pump point available");Mat4 inv={};Check(Invert(root,inv),"318 SPAS inverse");Check(Length(Sub(pos,Apply(inv,ControllerWrist(physical[0],0))))<1e-6,"318 SPAS stroke uses physical right raw wrist");}
   if(profile==0){g_lever.hold=true;build(0);build(1);Mat4 heldRoot={};Check(ShotgunRoot318(heldRoot),"318 temporary right support-hand gun parent valid");const double determinant=heldRoot.m[0]*(heldRoot.m[5]*heldRoot.m[10]-heldRoot.m[6]*heldRoot.m[9])-heldRoot.m[1]*(heldRoot.m[4]*heldRoot.m[10]-heldRoot.m[6]*heldRoot.m[8])+heldRoot.m[2]*(heldRoot.m[4]*heldRoot.m[9]-heldRoot.m[5]*heldRoot.m[8]);const double baseDet=root.m[0]*(root.m[5]*root.m[10]-root.m[6]*root.m[9])-root.m[1]*(root.m[4]*root.m[10]-root.m[6]*root.m[8])+root.m[2]*(root.m[4]*root.m[9]-root.m[5]*root.m[8]);Check(determinant*baseDet>.9,"318 temporary1887 gun parent preserves native FRAME parity");V3 palm={};Check(Lever1887PalmBody56(0,palm),"318 physical left lever palm available");Mat4 actual={};ShotgunPhysicalWrist318(0,actual);Check(Length(Sub(palm,Apply(actual,{-.6781-g_lever.palmOffset56[0][0],.0519+g_lever.palmOffset56[0][1],1.0387+g_lever.palmOffset56[0][2]})))<.001,"318 lever contact follows physical left operator");
    float pose341[576];Read(device,context,g_armPaletteCb[0],pose341);
    CheckReloadPose341(pose341,heldRoot,kL1887LeftSupportFromFrame,kL1887LeftQuietGripRelative,1,true);Mat4 physicalSupport341={};Check(ShotgunPhysicalWrist318(1,physicalSupport341),"341 physical supporting wrist current");const auto fittedSupport341=Multiply(ArmBodyToCamera(),physicalSupport341);for(int k=0;k<12;++k)Check(fabs(BoneMatrix(pose341,2).m[k]-fittedSupport341.m[k])<.003,"341 fitted1887 support wrist matches actual controller attachment");
    float savedPhysical341[16];memcpy(savedPhysical341,physical[0],64);
    for(int clip341:{kL1887Cycle,kL1887Open,kL1887ShellFirst,kL1887ShellLoop,kL1887Close})for(double phase341:{.15,.5,.85})for(bool reverse341:{false,true}){
     const double roll341=phase341*.6;physical[0][0]=float(cos(roll341));physical[0][1]=float(-sin(roll341));physical[0][4]=float(sin(roll341));physical[0][5]=float(cos(roll341));
     g_lever.clip=clip341;const auto& clip=kL1887Clips[clip341];g_lever.clipAt=GetTickCount64()-ULONGLONG(phase341*clip.keys[clip.count-1].t*1000);g_lever.clipPauseAt223=0;g_lever.poseFrame=-100000;g_lever.poseValid=false;
     build(reverse341?1:0);build(reverse341?0:1);Check(ShotgunRoot318(heldRoot),"3411887 current temporary reload root");
     Read(device,context,g_armPaletteCb[0],pose341);CheckReloadPose341(pose341,heldRoot,kL1887LeftSupportFromFrame,kL1887LeftQuietGripRelative,1,true);Mat4 physicalSupport341={};Check(ShotgunPhysicalWrist318(1,physicalSupport341),"341 physical supporting wrist current");const auto fittedSupport341=Multiply(ArmBodyToCamera(),physicalSupport341);for(int k=0;k<12;++k)Check(fabs(BoneMatrix(pose341,2).m[k]-fittedSupport341.m[k])<.003,"341 fitted1887 support wrist matches actual controller attachment");
     const auto& pose=Lever1887SimplePose();Read(device,context,g_armPaletteCb[1],pose341);const auto& part341=(clip341==kL1887ShellFirst||clip341==kL1887ShellLoop)?pose.shell:pose.lever;CheckReloadPose341(pose341,heldRoot,pose.hand,pose.fingers,0,false,&part341);
    }
    memcpy(physical[0],savedPhysical341,64);g_lever.clip=-1;g_lever.poseValid=false;g_lever.hold=false;
    puts("PASS:3411887 left-dominant temporary support grasp and operating-hand wrist/fingers fit native part across30 animated phase/draw-order cases and controller rolls");}
  }
 }
 for(int profile:{2,23,38,43,20}){g_weaponMechanicProfile=profile;g_weaponMechanicFrame=g_frame;Check(!fc3hands303::Shotgun318(profile)&&!LeftShotgun318(),"318 bolts/DLC/specials excluded");}
 puts("PASS:318 twelve shotgun owner/profile GPU cases,actual palm/rear contact,busy leases,raw pump,carried shell,replay-contact parity,1887 native-parity temporary parent;headset pending");
}

