void TestSmgs332(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 (void)device;const auto savedGuides=g_pistolGuides;
 float raw[2][576]={};for(int h=0;h<2;++h){for(int b=0;b<48;++b)StoreBone(raw[h],b,HandIdentity());memcpy(raw[h],kNeutralArmView[h],sizeof(kNeutralArmView[h]));}
 for(int profile:{7,9,21,28,39})for(int owner:{0,1}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_pistolReload={};auto& p=g_pistolReload;p.enabled=p.magazinePresent=p.chamberReady=true;g_reload={};g_reloadAlignment=false;g_pistolMenu.open=false;g_twoHand=false;g_autoLongGunSupport=g_autoPistolSupport=false;g_pistolGuides=savedGuides;
  fc3pad::SetGameplayControls(true);fc3pad::SetGripContext(true,true);fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);fc3pad::SetMagazineHandling(true,false);SetRenderNative319(0);SetRenderHands303(owner,false,g_armPoseEpoch);SetRenderReceiver309(-1);
  for(int h=0;h<2;++h){memset(physical[h],0,64);physical[h][0]=physical[h][5]=physical[h][10]=physical[h][15]=1;physical[h][3]=h?-.35f:.35f;physical[h][7]=-.2f;physical[h][11]=.5f;}
  const int other=1-owner;
  auto controls=[&](float holding,float receiving){PublishSidearmControls315(0,0,owner?receiving:holding,owner?holding:receiving);};
  auto build=[&](int side){fc3stereo::SetHandPoses(physical[0],physical[1]);cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw[side]),raw[side]);Check(BuildArmPalette(context,side)!=nullptr,"332 anatomical SMG arm builds");};controls(0,0);build(0);build(1);
  Mat4 root={},grip={};Check(ControllerCurrentRoot124(root,grip),"332 SMG native root available");p.rootBody=root;p.parentValid=true;p.parentEpoch=p.rootEpoch=g_armPoseEpoch;p.rootFrame=g_frame;
  const V3 rear=Apply(grip,{.6781,.0519,1.0387});
  auto moveTo=[&](V3 local){Mat4 controller={};for(int j=0;j<16;++j)controller.m[j]=physical[other][j];const V3 sphere=TransferSphere307(Multiply(controller,g_freeHandAttachment[other].relative),other),delta=Sub(Apply(root,local),sphere);physical[other][3]+=float(delta.x);physical[other][7]+=float(delta.y);physical[other][11]+=float(delta.z);build(other);};
  moveTo({rear.x+1,rear.y+1,rear.z+1});controls(0,0);Publish1911Transfer307();Check(!fc3hands303::TransferContact307().modifier321,"332 owner grip released retains original SMG entry policy");
  controls(1,0);Publish1911Transfer307();auto policy=fc3hands303::TransferContact307();Check(policy.enabled&&policy.allowed&&policy.contact&&policy.modifier321&&!policy.rearOnly316,"332 all five actual SMG publishers allow both-grip method without rear handle contact");
  controls(1,1);Publish1911Transfer307();controls(1,1);Check(fc3pad::Latest().leftGrip>.9f&&fc3pad::AmmoControls().leftGrip<.1f,"332 transfer preserves support input and reserves only ammunition grab");
  // Either native forend support or no support must preserve the chosen method.
  g_twoHand=profile!=39;Publish1911Transfer307();Check(fc3hands303::TransferContact307().modifier321&&fc3pad::Latest().leftGrip>.9f,"332 both grips do not consume two-hand support");
  fc3hands303::State state;state.initialized=true;state.player=123;state.entity=100;state.slot=0;state.hand=owner;state.reference=state.epoch=1;state.last=1000;state.down[0]=state.down[1]=false;state.ready[0]=state.ready[1]=true;
  fc3hands303::Input in;in.context=in.inventory=in.canChange=in.sphereTransfer307=in.modifierTransfer321=in.transferAllowed307=true;in.player=123;in.entities[0]=in.equipped=100;in.reference=in.epoch=1;in.tracked[0]=in.tracked[1]=true;in.grip[owner]=1;in.now=1010;state.Step(in);in.grip[other]=1;in.now+=10;auto event=state.Step(in);Check(!event.transfer&&state.receiver==other,"332 both grips arm without teleporting");in.receivingRelative309[other][0]=in.receivingPosition309[other][0]=other?-.08f:.08f;in.now+=10;event=state.Step(in);Check(event.transfer&&state.hand==other&&state.entity==100&&event.drawSlot<0,"332 receiving-side pull passes same SMG without native equip request");
  // A new reload contact and an active reload must win even with both grips held.
  g_twoHand=false;controls(0,0);Publish1911Transfer307();controls(1,1);Publish1911Transfer307();
  p.held=true;Publish1911Transfer307();controls(1,1);Check(!fc3hands303::TransferContact307().allowed&&fc3pad::AmmoControls().leftGrip>.9f,"332 held magazine keeps original reload grip with both grips down");p.held=false;p.rackHeld=true;Publish1911Transfer307();Check(!fc3hands303::TransferContact307().allowed,"332 active charging handle blocks transfer");p.rackHeld=false;p.pending=true;Publish1911Transfer307();Check(!fc3hands303::TransferContact307().allowed,"332 pending native reload blocks transfer");p.pending=false;
  g_pistolGuides.box=true;g_pistolGuides.handDiameter=.06f;
  for(int j=0;j<3;++j){g_pistolGuides.port[j]=float(j==0?rear.x:j==1?rear.y+.25:rear.z);g_pistolGuides.charge[j]=g_pistolGuides.slide[j]=float(j==0?rear.x+.25:j==1?rear.y+.25:rear.z);for(int b=0;b<3;++b){g_pistolGuides.size[b][j]=.04f;g_pistolGuides.angles[b][j]=0;}}
  for(int target:{0,profile==39?1:3}){const auto box=PistolGuideBox(root,target);Mat4 inverse={};Check(Invert(root,inverse),"332 action root invertible");moveTo(Apply(inverse,{box.m[3],box.m[7],box.m[11]}));controls(1,1);Publish1911Transfer307();controls(1,1);Check(!fc3hands303::TransferContact307().allowed&&fc3pad::AmmoControls().leftGrip>.9f,"332 first magazine/charge target contact preempts transfer before reload starts");}
  moveTo({rear.x+1,rear.y+1,rear.z+1});controls(0,0);Publish1911Transfer307();PublishSidearmControls315(owner?0:1,owner?1:0);Check(fc3pad::FireOutput()>0,"332 physical holding trigger still fires");PublishSidearmControls315(0,0);PublishSidearmControls315(owner?1:0,owner?0:1);Check(fc3pad::FireOutput()==0,"332 support trigger cannot fire");controls(0,0);
  printf("PASS:332 SMG profile%d owner%d actual GPU/root and publisher,both-grip support/ammo separation,pull handover,first-contact/active reload priority,firing\n",profile,owner);
 }
 g_pistolGuides=savedGuides;g_weaponMechanicProfile=20;g_weaponMechanicFrame=g_frame;
}
