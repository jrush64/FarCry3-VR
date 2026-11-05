
void TestPersistentAmmoPreview(ID3D11Device* device,ID3D11DeviceContext* context)
{
 const auto oldMenu=g_pistolMenu;const auto oldStates0=g_magazineStates[0],oldStates1=g_magazineStates[1];
 const auto oldGuide0=g_magazineGuides[0],oldGuide1=g_magazineGuides[1];const auto oldCache0=g_ammoPreviewPoses[0],oldCache1=g_ammoPreviewPoses[1];
 const LONG oldProfile=g_weaponMechanicProfile,oldProfileFrame=g_weaponMechanicFrame,oldFrame=g_frame;const int oldFamily=g_menuGuideFamily;
 const bool oldRight=g_handRightValid,oldLeft=g_handLeftValid,oldViews=g_eyeViewsSet,oldMove=g_frameGunValid,oldGuard=g_weaponMotionGuard;const LONG oldVisible=g_visibleWeaponDrawsLastFrame;
 const auto oldControllers=fc3pad::Latest();g_dev=device;g_oCreateBuffer=PistolFixtureCreate;g_weaponMotionGuard=true;g_visibleWeaponDrawsLastFrame=0;
 context->ClearState();ID3D11Buffer* world=Buffer(device,288,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 CameraSummary cam={};cam.right[0]=cam.up[1]=cam.forward[2]=1;cam.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
 ID3D11Buffer* cb[2]={world,camera};context->VSSetConstantBuffers(0,2,cb);
 float source[72]={};source[0]=source[5]=source[10]=1;world->SetPrivateData(kCbCopyGuid,sizeof(source),source);
 for(int family=0;family<2;++family){
  g_weaponMechanicProfile=family?39:20;g_weaponMechanicFrame=g_frame;g_menuGuideFamily=family;g_magazineGuides[family]=PistolGuideSettings{};g_magazineGuides[family].ammoPreview=true;
  g_magazineStates[family]=PistolReloadState{};auto& p=g_magazineStates[family];p.enabled=true;p.held=true;p.magazinePresent=false;p.lostObjects=9;
  auto& cache=g_ammoPreviewPoses[family];cache=AmmoPreviewPose{};cache.valid=true;cache.epoch=g_armPoseEpoch;
  for(int b=0;b<48;++b)StoreBone(cache.bodyBones,b,HandIdentity());cache.palm=HandIdentity();cache.palm.m[3]=-.29;cache.palm.m[7]=-.18;cache.palm.m[11]=.42;StoreBone(cache.bodyBones,2,cache.palm);
  g_pistolMenu.open=false;SetInsertOpen(true);Check(p.held&&!p.magazinePresent&&p.lostObjects==9,"opening INSERT preserves existing held magazine and inventory state");
  g_menuGuideFamily=family;fc3pad::Controllers inactive={};fc3pad::Publish(inactive);g_handLeftValid=g_handRightValid=g_eyeViewsSet=g_frameGunValid=false;
  const auto stateBefore=p;const auto cacheBefore=cache;
  ID3D11Buffer* vb=Buffer(device,family?157984:177904,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,family?35322:32682,D3D11_BIND_INDEX_BUFFER);
  UINT stride=32,offset=family?0:240;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
  for(int f=0;f<90;++f){
   ++g_frame;g_magazineGuides[family].ammoPosition[0]=-.04f+f*.0005f;g_magazineGuides[family].ammoRotation[2]=f*1.f;
   TickPistolReload(false);Check(PistolAmmoPreviewActive()&&AmmoPreviewPoseAvailable(),"checkbox keeps preview active across lost controller tracking and old model timestamps");
   const Mat4 expected=Multiply(ArmBodyToCamera(),Multiply(cache.palm,MagazineHeldFromHand()));
   auto* out=BuildWeaponWorld(context,family?0:8664,family?1548:1338);Check(out!=nullptr,"preview uploads independently of right-hand gun movement and motion-area guard");
   float actual[88]={};Read(device,context,out,actual);const Mat4 rendered=HullWorld(actual);
   for(int i=0;i<16;++i)Check(std::fabs(rendered.m[i]-expected.m[i])<.0001,"preview magazine stays on completed left wrist and follows mouse offsets every frame");
   Check(!memcmp(&stateBefore,&p,sizeof(p)),"persistent visual preview cannot change held magazine or transaction state");
  }
  // Restore the complete visible hand/arm in the same cached body space with no tracked poses.
  ID3D11Buffer* arm=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER),*bonesCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);UINT armStride=40,armOffset=0;
  float rawBones[576]={};for(int b=0;b<48;++b)StoreBone(rawBones,b,HandIdentity());bonesCb->SetPrivateData(kCbCopyGuid,sizeof(rawBones),rawBones);
  context->IASetVertexBuffers(0,1,&arm,&armStride,&armOffset);context->VSSetConstantBuffers(3,1,&bonesCb);
  auto* palette=BuildArmPalette(context,1);Check(palette!=nullptr,"left preview arm remains visible with both poses and eye tracking withheld");float armOut[576];Read(device,context,palette,armOut);
  const Mat4 expectedWrist=Multiply(ArmBodyToCamera(),cache.palm),actualWrist=BoneMatrix(armOut,2);
  for(int i=0;i<16;++i)Check(std::fabs(actualWrist.m[i]-expectedWrist.m[i])<.0001,"cached preview wrist matches magazine body-space attachment");
  for(int b=3;b<=19;++b){const Mat4 expected=Multiply(expectedWrist,BoneMatrix(kPistolGripRelative,b-3)),actual=BoneMatrix(armOut,b);for(int i=0;i<16;++i)Check(std::fabs(actual.m[i]-expected.m[i])<.0002,"visible cached fingers retain ammo holding pose");}
  Check(!memcmp(&stateBefore,&p,sizeof(p))&&!memcmp(&cacheBefore,&cache,sizeof(cache)),"cached GPU arm and magazine draws mutate no gameplay or pose-source state");
  g_magazineGuides[family].ammoPreview=false;Check(!PistolAmmoPreviewActive(),"checkbox off stops cached preview immediately");g_magazineGuides[family].ammoPreview=true;
  ++g_armPoseEpoch;Check(!AmmoPreviewPoseAvailable(),"real recalibration invalidates old cached arm");--g_armPoseEpoch;
  SetInsertOpen(false);Check(!PistolAmmoPreviewActive(),"closing INSERT ends visual preview even with checkbox remembered");
  p.pending=true;p.cycle=false;p.entryAccepted=false;p.deadline=GetTickCount64()+3000;g_pistolMenu.open=true;const auto pendingBefore=p;TickPistolReload(false);Check(!memcmp(&pendingBefore,&p,sizeof(p)),"menu polling preserves pending contact even when gameplay and controllers are inactive");g_pistolMenu.open=false;
  p.pending=p.cycle=false;p.held=true;p.magazinePresent=false;p.needsRack=true;p.chamberReady=false;const unsigned lostBefore=p.lostObjects;
  TickPistolReload(false);Check(p.held&&!p.magazinePresent&&p.needsRack&&!p.chamberReady&&p.lostObjects==lostBefore,"outside menu tracking loss cannot drop held magazine or bypass manual charging");
  ID3D11Buffer* none=nullptr;context->VSSetConstantBuffers(3,1,&none);vb->Release();ib->Release();arm->Release();bonesCb->Release();
 }
 context->ClearState();world->Release();camera->Release();g_magazineStates[0]=oldStates0;g_magazineStates[1]=oldStates1;g_magazineGuides[0]=oldGuide0;g_magazineGuides[1]=oldGuide1;g_ammoPreviewPoses[0]=oldCache0;g_ammoPreviewPoses[1]=oldCache1;
 g_pistolMenu=oldMenu;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldProfileFrame;g_frame=oldFrame;g_menuGuideFamily=oldFamily;
 g_handRightValid=oldRight;g_handLeftValid=oldLeft;g_eyeViewsSet=oldViews;g_frameGunValid=oldMove;g_weaponMotionGuard=oldGuard;g_visibleWeaponDrawsLastFrame=oldVisible;fc3pad::SetPistolCalibration(false);fc3pad::Publish(oldControllers);
 std::puts("PASS: both weapons retain actual GPU ammo and holding arm for 90 untracked mouse-edit frames with 288-byte Globals; visual cache cannot discard held/pending ammo or authorize reload");
}
