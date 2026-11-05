void TestSupport349(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 float raw[2][576]={};for(int s=0;s<2;++s){for(int b=0;b<48;++b)StoreBone(raw[s],b,HandIdentity());memcpy(raw[s],kNeutralArmView[s],sizeof(kNeutralArmView[s]));}
 for(int profile:{7,8,41,28,14,9,31,25,21,17,35,36,22,2,23,43,15,18,32,40,0,34})for(int owner:{0,1}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_reload={};g_reloadContact={};g_manualReload={};g_reloadAlignment=false;g_pistolMenu.open=false;g_twoHand=false;g_pkm={};g_r700={};g_r700.clip=-1;g_r700.chambered=true;g_rpg={};g_gm94={};g_gm94Clip={};g_flamer={};g_flamerClip={};g_spas={};g_spas.clip=-1;g_lever={};g_lever.clip=-1;g_lever.enabled=g_lever.chambered=true;g_otherSkin319={};g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=g_pistolReload.chamberReady=true;
  SetRenderNative319(0);SetRenderHands303(owner,false,g_armPoseEpoch);SetRenderReceiver309(-1);PublishSidearmControls315(0,0);
  for(int s=0;s<2;++s){for(int n=0;n<16;++n)physical[s][n]=n%5==0?1.f:0.f;physical[s][3]=s?-.35f:.35f;physical[s][7]=-.2f;physical[s][11]=.5f;}
  auto build=[&](int side){fc3stereo::SetHandPoses(physical[0],physical[1]);cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw[side]),raw[side]);Check(BuildArmPalette(context,side)!=nullptr,"349 physical arm upload");};build(0);build(1);
  Mat4 root={},grip={},support={};Check(ControllerCurrentRoot124(root,grip)||ShotgunRoot318(root),"349 root available");
  g_pistolReload.rootBody=root;g_pistolReload.rootEpoch=g_armPoseEpoch;g_pistolReload.rootFrame=g_frame;g_pistolReload.parentValid=true;g_pistolReload.parentEpoch=g_armPoseEpoch;
  g_rpg.frameBody=g_gm94.frameBody=g_flamer.frameBody=root;g_rpg.frameFrame=g_gm94.frameFrame=g_flamer.frameFrame=g_frame;g_flamer.frameEpoch58=g_armPoseEpoch;
  g_twoHand=true;build(1-owner);PublishHandleBinding333();const auto binding=fc3grab333::Read();Check(binding.enabled&&binding.support,"349 published support available");
  Mat4 controller={};for(int n=0;n<16;++n)controller.m[n]=physical[owner][n];const auto published=Apply(controller,{binding.foregrip[0]*g_worldScale,binding.foregrip[1]*g_worldScale,-binding.foregrip[2]*g_worldScale});
  V3 guide={},palm={};const bool haveGuide=GrabGizmoPoints({},guide,palm);const auto drawn=Apply(g_armFinalBody[1-owner][2],{owner?.6781:-.6781,.0519,1.0387});
  Check(haveGuide&&Length(Sub(published,guide))<.003,"349 visible guide and XR support use same authored target in both owners");
  if(profile==32){
   Check(Length(Sub(published,drawn))<.003,"349 RPG actual GPU support palm matches authored foregrip in both owners");
   for(int side349=0;side349<2;++side349){
    build(side349);float skin349[576];Read(device,context,g_armPaletteCb[side349],skin349);const int role349=owner?1-side349:side349;
    const float* grip349=role349?kRpgLeftIdleFingers:kRpgRightIdleFingers;const auto wrist349=BoneMatrix(skin349,2),mirror349=SidearmMirror315();
    for(int bone349=3;bone349<=19;++bone349){auto finger349=BoneMatrix(grip349,bone349-3);if(owner)finger349=Multiply(mirror349,Multiply(finger349,mirror349));const auto expected349=Multiply(wrist349,finger349),actual349=BoneMatrix(skin349,bone349);for(int n349=0;n349<12;++n349)Check(fabs(expected349.m[n349]-actual349.m[n349])<.003,"349 RPG both actual hands use captured idle grip fingers,mirrored only for opposite anatomy");}
   }
  }
  printf("PASS:349 profile%d owner%d guide/XR error%.8f RPGsupportError%.8f\n",profile,owner,Length(Sub(published,guide)),profile==32?Length(Sub(published,drawn)):0.);
  if(profile==32){
   g_twoHand=false;g_rpg.clip=true;g_rpg.clipAt=W7Now();g_rpg.poseFrame=-100000;
   const auto cached349=g_rpg.frameBody;
   for(int sample349=0;sample349<5;++sample349){
    physical[owner][3]+=.031f;physical[owner][7]-=.013f;physical[owner][11]+=.021f;
    g_rpg.clipAt=W7Now()-sample349*50;g_rpg.poseFrame=-100000;build(0);build(1);
    Mat4 current349={},ignored349={};Check(ControllerCurrentRoot124(current349,ignored349),"349 moving RPG current tube root");
    const auto actual349=RpgRocketBodyNow(),expected349=Multiply(current349,RpgClipPose().rocket);
    for(int n349=0;n349<16;++n349)Check(fabs(actual349.m[n349]-expected349.m[n349])<2e-5,"349 inserted rocket uses current tube root,not cached prior draw");
    for(int side349=0;side349<2;++side349){
     float skin349[576];Read(device,context,g_armPaletteCb[side349],skin349);
     const int role349=owner?1-side349:side349;const auto native349=role349?RpgClipPose().left:RpgClipPose().right;
     const auto expectedHand349=owner?ReloadMirrorHand341(current349,native349,role349,true,false):Multiply(ArmBodyToCamera(),Multiply(current349,native349));
     for(int n349=0;n349<12;++n349)Check(fabs(BoneMatrix(skin349,2).m[n349]-expectedHand349.m[n349])<.003,"349 both RPG insertion wrists share current tube root and retained native clip");
    }
    Check(g_rpg.frameBody.m[3]==cached349.m[3],"349 draw-order regression retains deliberately stale cache");
   }
   g_rpg.clip=false;printf("PASS:349 RPG owner%d moving tube/rocket/both insertion wrists across5 samples,without refreshing cached draw\n",owner);
  }
 }
 g_twoHand=false;
}

void TestFlamerLever349(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 auto* vb349=Buffer(device,349160,D3D11_BIND_VERTEX_BUFFER);auto* ib349=Buffer(device,68436,D3D11_BIND_INDEX_BUFFER);
 UINT stride349=40,offset349=0;context->IASetVertexBuffers(0,1,&vb349,&stride349,&offset349);context->IASetIndexBuffer(ib349,DXGI_FORMAT_R16_UINT,0);context->VSSetConstantBuffers(0,4,cb);
 if(!g_weaponPaletteCb)g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
 g_weaponMechanicProfile=15;g_weaponMechanicFrame=++g_frame;g_pistolMenu.open=false;g_twoHand=false;g_flamer={};g_flamerClip={};g_pistolReload={};g_frameGunValid=true;g_frameGunMove=HandIdentity();g_armMovesFrame[0]=g_frame;
 float source349[576];for(int b349=0;b349<48;++b349)StoreBone(source349,b349,HandIdentity());
 const V3 leverVertex349={kFlamerHandleTipBind[0],kFlamerHandleTipBind[1],kFlamerHandleTipBind[2]};
 for(int owner349:{0,1}){
  SetRenderHands303(owner349,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);
  for(double phase349:{0.,.25,.5,.75,1.}){
   g_flamerClip.kind=1;g_flamerClip.scrub=phase349;g_flamerClip.poseFrame=-100000;
   for(auto draw349:{std::pair<UINT,UINT>{3630,8004},{20838,13380},{13590,7248}}){
    cb[3]->SetPrivateData(kCbCopyGuid,sizeof(source349),source349);auto* palette349=BuildWeaponPalette(context,draw349.first,draw349.second);Check(palette349!=nullptr,"349 actual flamethrower palette upload");
    float result349[576];Read(device,context,palette349,result349);const auto fp349=Multiply(ArmBodyToCamera(),g_flamer.frameBody);
    BOOL tag349=FALSE;UINT bytes349=sizeof(tag349);Check(SUCCEEDED(palette349->GetPrivateData(kA2000Mirror337,&bytes349,&tag349)),"349 palette winding ownership tag");
    Check(MirrorRasterSource349(cb[0],palette349)==(tag349==2?palette349:cb[0]),"349 lever palette owns mixed winding even with simultaneous rigid world override;other selection unchanged");
    Check(tag349==((owner349&&draw349.first!=13590)?2:0),"349 mixed winding limited to left lever assembly;canister/native right remain normal");
    if(draw349.first==20838){
     auto vertex349=Apply(Multiply(FlamerHandle(),FlamerTranslate(kFlamerBindHandle,-1)),leverVertex349);if(owner349)vertex349.x=-vertex349.x;
     Check(Length(Sub(Apply(BoneMatrix(result349,1),leverVertex349),Apply(fp349,vertex349)))<.003,"349 actual uploaded lever tip geometry follows opposite side through animation");
     const auto box349=Multiply(g_flamer.frameBody,FlamerLeverBox());const auto tip349=Apply(g_flamer.frameBody,vertex349);
     // Default guide is the measured bind-tip; explicit user calibration remains additive.
     Mat4 expectedBox349=FlamerHandle();const auto nativeTip349=Apply(expectedBox349,{leverVertex349.x-kFlamerBindHandle[0],leverVertex349.y-kFlamerBindHandle[1],leverVertex349.z-kFlamerBindHandle[2]});expectedBox349.m[3]=nativeTip349.x;expectedBox349.m[7]=nativeTip349.y;expectedBox349.m[11]=nativeTip349.z;expectedBox349=W7Adjust(3,0,expectedBox349);if(owner349)expectedBox349=Multiply(SidearmMirror315(),Multiply(expectedBox349,SidearmMirror315()));
     const auto expectedWorld349=Multiply(g_flamer.frameBody,expectedBox349);for(int n349=0;n349<12;++n349)Check(fabs(box349.m[n349]-expectedWorld349.m[n349])<.003,"349 calibrated lever trigger mirrors with actual mechanism");(void)tip349;
     for(int bone349:{0,2,3}){const auto native349=bone349==0?kFlamerStaticDIAL239:bone349==3?kFlamerStaticMETAL_RING239:HandIdentity();const auto expected349=Multiply(fp349,native349);for(int n349=0;n349<12;++n349)Check(fabs(BoneMatrix(result349,bone349).m[n349]-expected349.m[n349])<.003,"349 receiver/dial/ring geometry stays native around reflected lever");}
    }
    if(tag349==2){ID3D11RasterizerState* old349=nullptr;bool changed349=false;Check(BeginA2000Raster337(context,MirrorRasterSource349(cb[0],palette349),old349,changed349)&&changed349,"349 mixed lever raster guard active");ID3D11RasterizerState* active349=nullptr;context->RSGetState(&active349);D3D11_RASTERIZER_DESC desc349={};active349->GetDesc(&desc349);Check(desc349.CullMode==D3D11_CULL_NONE,"349 native/reflected bones both survive mixed draw winding");active349->Release();EndA2000Raster337(context,old349,changed349);}
   }
   printf("PASS:349 flamer owner%d phase%.2f actual lever geometry/trigger/winding,receiver and canister preserved\n",owner349,phase349);
  }
 }
 g_flamerClip={};context->ClearState();vb349->Release();ib349->Release();
}
