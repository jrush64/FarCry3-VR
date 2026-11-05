void TestArms74(ID3D11Device* device,ID3D11DeviceContext* context){
 g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_aimActive=false;g_neutralQuietGate=true;
 g_weaponMechanicProfile=8;g_weaponMechanicFrame=g_frame;g_visibleWeaponDrawsLastFrame=6;g_pistolReload={};g_reload={};g_r700={};g_reloadAlignment=false;g_manualReloadEnabled=true;g_armFixedOrdinary74=true;
 fc3pad::SetPistolCalibration(false);fc3pad::SetGameplayControls(true);fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 const auto id=HandIdentity();ArmJoints fixed[2]={};for(int side=0;side<2;++side){
  float* pose=side?g_handLeft:g_handRight;for(int i=0;i<16;++i)pose[i]=g_handBody[side][i]=g_handRef[side][i]=float(id.m[i]);pose[3]=g_handBody[side][3]=side?-.3f:.3f;pose[7]=g_handBody[side][7]=-.2f;pose[11]=g_handBody[side][11]=.45f;
  fixed[side]={{side?-.2:.2,-.15,-.2},{side?-.35:.35,-.3,.05},{side?-.35:.35,-.2,.4},true};g_armJoints[side]=fixed[side];g_armPaletteCb[side]=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);g_armProbe74[side].samples=2400;
 }
 auto* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
 ID3D11Buffer* cb[4]={Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER),nullptr,Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER)};
 float globals[88]={};globals[0]=globals[5]=globals[10]=1;cb[0]->SetPrivateData(kCbCopyGuid,sizeof(globals),globals);CameraSummary camera={};camera.right[0]=camera.up[1]=camera.forward[2]=1;camera.mainView=1;cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);context->VSSetConstantBuffers(0,4,cb);
 float original[576]={};for(int b=0;b<48;++b)StoreBone(original,b,id);
 bool oldMoved=false;
 for(int mode=0;mode<3;++mode){g_armFixedOrdinary74=mode!=2;g_twoHand=mode!=0;g_frameGunValid=true;g_frameGunMove=id;g_barrelCam[0]=g_barrelCam[1]=0;g_barrelCam[2]=1;float baseline[2][576]={};
  for(int frame=0;frame<40;++frame){++g_frame;g_weaponMechanicFrame=g_frame;float animated[576];memcpy(animated,original,sizeof(animated));Mat4 turn=id;const double a=.3*sin(frame*.2);turn.m[0]=turn.m[5]=cos(a);turn.m[1]=-sin(a);turn.m[4]=sin(a);turn.m[3]=.1*sin(frame*.4);turn.m[7]=.08*cos(frame*.3);
   for(int b=0;b<25;++b)StoreBone(animated,b,Multiply(turn,BoneMatrix(original,b)));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(animated),animated);
   for(int side=0;side<2;++side){g_armJoints[side]=fixed[side];g_armJoints[side].e[1]+=.1*sin(frame*.3);g_armJoints[side].w[2]+=.08*cos(frame*.2);Check(BuildArmPalette(context,side)!=nullptr,"arms74 animated ordinary arm draw");float result[576];Read(device,context,g_armPaletteCb[side],result);
    if(!frame)memcpy(baseline[side],result,sizeof(result));else if(mode!=2)Compare(result,baseline[side],300,"sprint root/elbow/wrist animation cannot move ordinary arms with fixed tracking");else for(int i=0;i<300;++i)oldMoved=oldMoved||fabs(result[i]-baseline[side][i])>.001;
   }
  }
 }
 Check(oldMoved,"negative control reproduces native arm movement with the new policy disabled");g_armFixedOrdinary74=true;g_twoHand=false;
 cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);Check(BuildArmPalette(context,0)!=nullptr,"tracked moving arm baseline");float before[576],after[576];Read(device,context,g_armPaletteCb[0],before);g_handRight[3]+=.1f;g_handBody[0][3]+=.1f;Check(BuildArmPalette(context,0)!=nullptr,"tracked moving arm final");Read(device,context,g_armPaletteCb[0],after);Check(fabs(after[2*12+3]-before[2*12+3]-.1)<1e-5,"real controller translation still reaches final wrist exactly");
 // The shirt section is a separate draw. Its mapped upper-arm bones must be
 // the same final pose as the visible arm, including the new shoulder reach.
 g_armFixedOrdinary74=true;g_twoHand=false;g_aimActive=false;++g_frame;g_weaponMechanicFrame=g_frame;
 cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);
 for(int side=0;side<2;++side){g_armJoints[side]=fixed[side];Check(BuildArmPalette(context,side)!=nullptr,"arm78 sleeve-source palette");}
 g_bodyPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);g_bodyDrawStart=0;g_bodyDrawCount=3687;
 Check(BuildBodyPalette(context)!=nullptr,"arm78 shared shirt sleeve palette");
 float shirt[576],rightArm[576],leftArm[576];Read(device,context,g_bodyPaletteCb,shirt);Read(device,context,g_armPaletteCb[0],rightArm);Read(device,context,g_armPaletteCb[1],leftArm);
 for(int k=0;k<12;++k){Check(fabs(shirt[11*12+k]-rightArm[k])<2e-5,"right shirt reaches visible upper arm");Check(fabs(shirt[7*12+k]-leftArm[k])<2e-5,"left shirt reaches visible upper arm");}
 g_bodyPaletteCb->Release();g_bodyPaletteCb=nullptr;
 g_motionGun=false;Check(!BuildArmPalette(context,0),"motion toggle retains native arm path");context->ClearState();for(auto* b:cb)if(b)b->Release();vb->Release();for(int side=0;side<2;++side){g_armPaletteCb[side]->Release();g_armPaletteCb[side]=nullptr;}
 puts("PASS: arms74 actual GPU palettes reject sprint root/elbow/wrist animation one- and two-handed, negative control reproduces movement, real tracking remains responsive");
}
