void TestBoth1911Methods327(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 (void)device;float raw[2][576]={};for(int h=0;h<2;++h){for(int b=0;b<48;++b)StoreBone(raw[h],b,HandIdentity());memcpy(raw[h],kNeutralArmView[h],sizeof(kNeutralArmView[h]));}
 for(int profile:{1,12,20})for(int owner:{0,1})for(int method:{0,1}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=g_pistolReload.chamberReady=true;g_reload={};g_reloadAlignment=false;g_pistolMenu.open=false;g_twoHand=false;SetRenderNative319(0);SetRenderHands303(owner,false,g_armPoseEpoch);SetRenderReceiver309(-1);
  for(int h=0;h<2;++h){memset(physical[h],0,64);physical[h][0]=physical[h][5]=physical[h][10]=physical[h][15]=1;physical[h][3]=h?-.35f:.35f;physical[h][7]=-.2f;physical[h][11]=.5f;}
  fc3stereo::SetHandPoses(physical[0],physical[1]);for(int h=0;h<2;++h){cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw[h]),raw[h]);Check(BuildArmPalette(context,h)!=nullptr,"327 existing held/free1911 hands build");}
  const int other=1-owner;auto controls=[&](float holding,float receiving){PublishSidearmControls315(0,0,owner?receiving:holding,owner?holding:receiving);};
  controls(0,0);Publish1911Transfer307();Check(!fc3hands303::TransferContact307().modifier321&&!fc3hands303::TransferContact307().contact,"327 without support or owning grip neither entry starts");
  g_twoHand=method==0;controls(float(method),0);Publish1911Transfer307();auto policy=fc3hands303::TransferContact307();Check(policy.allowed&&policy.contact&&policy.modifier321==bool(method),"327 real publisher selects original support or optional owning-grip method");
  fc3hands303::State state;state.initialized=true;state.player=123;state.entity=100;state.slot=0;state.hand=owner;state.reference=state.epoch=1;state.last=1000;state.down[0]=state.down[1]=false;state.ready[0]=state.ready[1]=true;
  fc3hands303::Input in;in.context=in.inventory=in.canChange=in.sphereTransfer307=true;in.player=123;in.entities[0]=in.equipped=100;in.reference=in.epoch=1;in.tracked[0]=in.tracked[1]=true;in.now=1010;in.transferAllowed307=policy.allowed;in.sphereContact307=policy.contact;in.modifierTransfer321=policy.modifier321;in.grip[owner]=float(method);state.Step(in);
  controls(float(method),1);Publish1911Transfer307();in.grip[other]=1;in.now+=10;const auto armed=state.Step(in);Check(!armed.transfer&&state.receiver==other,"327 either entry arms without instant transfer");
  Check((fc3pad::AmmoControls().leftGrip>.9f)==(method==0),"327 original reload grip path retained;optional plain chord reserves ammo only");
  SetRenderReceiver309(other);controls(float(1-method),1);Publish1911Transfer307();Check(fc3hands303::TransferContact307().modifier321==bool(method),"327 changing owning grip cannot silently switch an armed entry policy");
  auto probe=state;auto alternate=in;alternate.grip[owner]=float(1-method);alternate.now+=10;probe.Step(alternate);Check(method?probe.receiver<0:probe.receiver==other,"327 modifier release cancels optional pass;adding owner grip keeps original pass");
  controls(float(method),1);g_pistolReload.held=true;Publish1911Transfer307();Check(!fc3hands303::TransferContact307().allowed,"327 active magazine reload blocks either entry method");g_pistolReload.held=false;Publish1911Transfer307();
  const float pull=other?-.08f:.08f;in.receivingRelative309[other][0]=in.receivingPosition309[other][0]=pull;in.now+=20;const auto event=state.Step(in);Check(event.transfer&&state.hand==other&&state.entity==100&&!state.empty&&event.drawSlot<0,"327 both entry methods complete same-weapon lateral transfer in either direction");
  SetRenderReceiver309(-1);controls(0,0);
  printf("PASS:328 pistol%d owner%d method%d real publisher,old support/no-owner grip,optional modifier,no teleport,entry latch,reload priority,same-weapon pull\n",profile,owner,method);
 }
}
