// Integration regression: actual skinned FRAME upload versus actual arm upload,
// captured MP5 grip and the ray used for reticle/native shot publication.
void TestMp5Root142(ID3D11Device* device,ID3D11DeviceContext* context,int profile=28,int shaderPaletteSlot=0,bool splitWorld=false){
 context->ClearState();g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;
 g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=true;g_weaponMotionGuard=g_flatViewActive=false;g_machete.held=false;g_pistolMenu.open=false;g_reload={};g_r700={};g_lever={};g_reloadAlignment=false;g_aimActive=false;g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;
 fc3pad::SetPistolCalibration(false);fc3pad::SetGameplayControls(true);fc3pad::Controllers input={};input.active=true;fc3pad::Publish(input);
 const Mat4 id=HandIdentity();for(int side=0;side<2;++side){for(int i=0;i<16;++i)g_handBody[side][i]=g_handRef[side][i]=float(id.m[i]);g_handBody[side][3]=side?-.18f:.18f;g_handBody[side][7]=-.22f;g_handBody[side][11]=side?.7f:.4f;memcpy(side?g_handLeft:g_handRight,g_handBody[side],64);}
 for(int eye=0;eye<2;++eye){for(int i=0;i<16;++i)g_eyeViews[eye].view[i]=g_eyeViewsBody[eye].view[i]=float(i%5==0);g_eyeViews[eye].tanLeft=g_eyeViews[eye].tanDown=-1;g_eyeViews[eye].tanRight=g_eyeViews[eye].tanUp=1;}
 ++g_armPoseEpoch;g_scopeEyes60[0]={};g_scopeEyes60[1]={};g_frameGunValid=true;g_frameGunMove=id;g_armJoints[0]={{.2,-.2,-.3},{.3,-.3,0},{.2,-.2,.3},true};g_armJoints[1]={{-.2,-.2,-.3},{-.3,-.3,0},{-.2,-.2,.3},true};
 auto* previousGun=g_weaponPaletteCb;auto* previousArm=g_armPaletteCb[0];g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);g_armPaletteCb[0]=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
 const auto& rifle=RifleForProfile(profile);auto* gun=Buffer(device,rifle.vb,D3D11_BIND_VERTEX_BUFFER);auto* arm=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);auto* indices=Buffer(device,rifle.ib,D3D11_BIND_INDEX_BUFFER);auto* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);auto* camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);auto* palette=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
 CameraSummary cam={};cam.mainView=1;cam.right[0]=cam.up[1]=cam.forward[2]=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);ID3D11Buffer* cb[4]={globals,camera,nullptr,palette};context->VSSetConstantBuffers(0,4,cb);context->IASetIndexBuffer(indices,DXGI_FORMAT_R16_UINT,0);
 ID3D11VertexShader* gunShader=nullptr;ID3D11Buffer* clip=nullptr;
 if(shaderPaletteSlot){
  char hlsl[512]={};sprintf_s(hlsl,"row_major float3x4 _WorldMatrix;float3 _WorldPositionFractions;cbuffer BlendMatricesParameterProvider:register(b%d){float4 p[144];};float4 main(uint id:SV_VertexID):SV_Position{return float4(mul(_WorldMatrix,float4(p[id].w,p[id+1].w,0,1))+_WorldPositionFractions,1);}",shaderPaletteSlot);
  auto* code=HullCompile(hlsl,"vs_5_0");Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&gunShader)),"binding218 real skinned shader");gunShader->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();
  clip=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);const float plane[4]={1,2,3,4};clip->SetPrivateData(kCbCopyGuid,sizeof(plane),plane);context->UpdateSubresource(clip,0,nullptr,plane,0,0);
 }
 auto equal=[](const Mat4& a,const Mat4& b){for(int i=0;i<12;++i)if(fabs(a.m[i]-b.m[i])>3e-4)return false;return true;};
 int reticleSamples=0;
 for(bool two:{false,true})for(bool firing:{false,true})for(bool owned:{false,true})for(int step=0;step<9;++step){
  ++g_frame;g_weaponMechanicFrame=g_frame;g_twoHand=two;auto& reload=g_pistolReload;reload={};reload.enabled=reload.magazinePresent=true;reload.held=owned;reload.parentEpoch=g_armPoseEpoch;input.rightTrigger=firing?1.f:0;fc3pad::Publish(input);
  // Native animation and legacy move disagree with the controller by design,
  // reproducing the separate code paths seen in the failed live141 run.
  const Mat4 native=TurnAndMove({0,0,1},Unit({.15+step*.07,.35-step*.04,1}),{},{.03*step,-.02*step,.1});
  g_frameGunMove=TurnAndMove({0,0,1},Unit({-.3,.1*step,1}),{},{-.04,.06,0});
  float rows[88]={};rows[0]=rows[5]=rows[10]=1;if(splitWorld){rows[3]=1;rows[7]=-2;rows[11]=3;rows[12]=.25f;rows[13]=-.375f;rows[14]=.125f;}globals->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);
  float raw[576];for(int b=0;b<48;++b)StoreBone(raw,b,native);palette->SetPrivateData(kCbCopyGuid,sizeof(raw),raw);
  if(shaderPaletteSlot){context->VSSetShader(gunShader,nullptr,0);ID3D11Buffer* drawCb[5]={globals,camera,nullptr,shaderPaletteSlot==3?palette:clip,shaderPaletteSlot==4?palette:clip};context->VSSetConstantBuffers(0,5,drawCb);}
  UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&gun,&stride,&offset);
  Mat4 expected={},grip={};Check(ControllerCurrentRoot124(expected,grip),"MP5 controller target exists before arm/FRAME draw");
  for(const auto& part:rifle.frame){if(!part[1])continue;Mp5Slots mapped={};if(!Mp5DrawSlots(part[0],part[1],mapped)||mapped.anchor==mapped.clip)continue;const std::pair<UINT,UINT> draw={part[0],part[1]};
   palette->SetPrivateData(kCbCopyGuid,sizeof(raw),raw);UINT outputSlot=99;auto* uploaded=BuildWeaponPalette(context,draw.first,draw.second,&outputSlot);Check(uploaded!=nullptr,"actual MP5 skinned GPU placement path used");if(shaderPaletteSlot)Check(outputSlot==UINT(shaderPaletteSlot),"binding218 output uses the actual reflected slot, not the stale clip-plane slot");float out[576];Read(device,context,uploaded,out);Mp5Slots slots={};Check(Mp5DrawSlots(draw.first,draw.second,slots),"actual MP5 draw anchor");const Mat4 displayed=BoneMatrix(out,slots.anchor);
   const Mat4 rendered=Multiply(HullWorld(rows),displayed);
   Check(equal(rendered,Multiply(ArmBodyToCamera(),expected)),"actual displayed MP5 FRAME including high/low world position matches controller/hand root, not legacy gun move");
   const Mat4 displayedWrist=Multiply(rendered,(*rifle.hand));Mat4 targetWrist={};Check(ControllerRightHand124(targetWrist)&&equal(displayedWrist,targetWrist),"actual displayed MP5 captured hand attachment equals fixed right wrist");
   if(!owned){V3 ray={};Check(FirearmAxis129(ray),"MP5 normal native/reticle ray ready");const V3 barrel=Unit({rendered.m[1],rendered.m[5],rendered.m[9]});Check(Length(Sub(ray,barrel))<1e-5,"actual uploaded MP5 barrel equals shot/reticle axis in either grip and while firing");ResetScopeStabilization72();PublishReticleAim73(ray);g_aimActive=true;for(int eye=0;eye<2;++eye){V3 target={};if(ReticleTarget73(eye,id,target)){++reticleSamples;const V3 projected=Unit(Sub(target,ControllerWrist(g_handBody[0],0)));Check(Length(Sub(projected,barrel))<1e-5,"both-eye physical reticle targets lie on the actual uploaded barrel direction");}}g_aimActive=false;}
  }
  if(splitWorld){float armRows[88]={};armRows[0]=armRows[5]=armRows[10]=1;globals->SetPrivateData(kCbCopyGuid,sizeof(armRows),armRows);}
  context->IASetVertexBuffers(0,1,&arm,&stride,&offset);if(shaderPaletteSlot){context->VSSetShader(nullptr,nullptr,0);context->VSSetConstantBuffers(3,1,&palette);}float original[576]={};for(int b=0;b<48;++b)StoreBone(original,b,id);memcpy(original,kNeutralArmView[0],sizeof(kNeutralArmView[0]));palette->SetPrivateData(kCbCopyGuid,sizeof(original),original);auto* armUpload=BuildArmPalette(context,0);Check(armUpload!=nullptr,"actual firing hand uploaded after gun draw");float hand[576];Read(device,context,armUpload,hand);Check(equal(BoneMatrix(hand,2),Multiply(expected,(*rifle.hand))),"actual uploaded right hand stays attached to the actual uploaded MP5");if(profile==28||profile==21)for(int b=3;b<=19;++b)Check(equal(BoneMatrix(hand,b),Multiply(Multiply(expected,*rifle.hand),BoneMatrix(rifle.rightGrip,b-3))),"actual displayed SMG firing fingers keep their native attachment to the actual displayed FRAME");
 }
 Check(reticleSamples>0,"physical reticle projection was exercised against real uploaded skinned receivers");context->ClearState();SafeRelease(g_weaponPaletteCb);SafeRelease(g_armPaletteCb[0]);g_weaponPaletteCb=previousGun;g_armPaletteCb[0]=previousArm;for(auto* b:{gun,arm,indices,globals,camera,palette})b->Release();SafeRelease(gunShader);SafeRelease(clip);g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=true;g_twoHand=false;
 printf("PASS: actual skinned profile%d (palette b%d) FRAME uploads share fixed hand attachment and shot/reticle barrel across one/two hands, firing/released trigger, ordinary/reload ownership and native animation\n",profile,shaderPaletteSlot?shaderPaletteSlot:3);
}
