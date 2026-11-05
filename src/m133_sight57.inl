// PID28416: receiver FRAME->reflex_b rigid relation measured from1981 native
// same-frame pairs; matrix spread below1e-6, including physical shell insertion.
const Mat4 kM133ReflexBFromFrame57={{1.00000450,-.0000862948423,.0000861780756,-.00176759098,.0000862476172,1.00000050,-.0000863899567,.201637344,-.0000862100067,.0000863236579,1.00000067,.0948919471,0,0,0,1}};
Mat4 g_m133SightParent57={};LONG g_m133SightFrame57=-100000;UINT g_m133SightEpoch57=~0u;ID3D11DeviceContext* g_m133SightContext57=nullptr;
bool M133SightScope57(){return M133SupportProfile56()&&g_frame-g_weaponMechanicFrame<=2&&MotionInteractionEnabled()&&g_handRightValid&&!fc3pad::NativeMenuOpen();}
bool M133SightDraw57(ID3D11DeviceContext* context,UINT start,UINT count){
 if(!M133SightScope57()||!context)return false;ID3D11Buffer* vb=nullptr,*ib=nullptr;UINT stride=0,offset=0,indexOffset=0;DXGI_FORMAT format={};context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);context->IAGetIndexBuffer(&ib,&format,&indexOffset);D3D11_BUFFER_DESC vd={},id={};if(vb)vb->GetDesc(&vd);if(ib)ib->GetDesc(&id);SafeRelease(vb);SafeRelease(ib);
 if(vd.ByteWidth!=67448||id.ByteWidth!=11280||format!=DXGI_FORMAT_R16_UINT||indexOffset)return false;ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto hash=ShaderHash(vs);SafeRelease(vs);
 bool route=false;UINT paletteSlot=3;
 if(stride==28&&offset==0){route=hash==0xD75F2E75490BBC23ull&&start==0&&count==192;paletteSlot=4;}
 else if(stride==40&&offset==1848){
  if(hash==0xA38F7CD233BCE12Bull)route=(start==672&&count==984)||(start==1656&&count==3984);
  else if(hash==0x985261CC30DCD0DAull||hash==0x894FAE9F7F8E0B5Eull)route=(start==192&&count==192)||(start==384&&count==288);
  else if(hash==0xCB43EE096F50DBDBull){route=start==672&&count==984;paletteSlot=4;}
 }
 if(!route)return false;
 // Every asset vertex uses palette slot0. Never replace a borrowed or animated
 // palette with a rigid-world assumption; preserve its native draw as fallback.
 ID3D11Buffer* palette=nullptr;context->VSGetConstantBuffers(paletteSlot,1,&palette);float matrices[576]={};const UINT copied=ReadCopy(palette,matrices,sizeof(matrices));SafeRelease(palette);if(copied<48)return false;
 const Mat4 active=BoneMatrix(matrices,0),identity=HandIdentity();for(int i=0;i<12;++i)if(!std::isfinite(active.m[i])||fabs(active.m[i]-identity.m[i])>1e-6)return false;return true;
}
void M133SightObserveFinal57(ID3D11DeviceContext* context,const Mat4& toView,const float* bones,UINT start,UINT count){
 if(!M133SightScope57()||!bones||!((start==9633&&count==6795)||(start==4557&&count==5076)||(start==16428&&count==7311)))return;
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC d={};if(vb)vb->GetDesc(&d);SafeRelease(vb);if(d.ByteWidth!=257896||stride!=40||offset!=336)return;
 Mat4 inverse={};if(!Invert(ArmBodyToCamera(),inverse))return;g_m133SightParent57=Multiply(inverse,Multiply(toView,BoneMatrix(bones,0)));g_m133SightFrame57=g_frame;g_m133SightEpoch57=g_armPoseEpoch;g_m133SightContext57=context;
}
bool M133SightWorld57(ID3D11DeviceContext* context,UINT start,UINT count,float* globals){
 if(!globals||!M133SightDraw57(context,start,count))return false;
 ID3D11Buffer* boundWorld=nullptr;context->VSGetConstantBuffers(0,1,&boundWorld);D3D11_BUFFER_DESC worldDesc={};if(boundWorld)boundWorld->GetDesc(&worldDesc);SafeRelease(boundWorld);if(worldDesc.ByteWidth<80)return false; // world+fractions and decompression rows
 ID3D11Buffer* camera=nullptr;context->VSGetConstantBuffers(1,1,&camera);CameraSummary summary={};UINT size=sizeof(summary);const bool valid=camera&&SUCCEEDED(camera->GetPrivateData(kCameraSummaryGuid,&size,&summary))&&size==sizeof(summary)&&summary.mainView;SafeRelease(camera);Mat4 inverseView={};if(!valid||!Invert(HullView(summary),inverseView))return false;
 Mat4 parent={};const bool confirmed=g_m133SightFrame57==g_frame&&g_m133SightEpoch57==g_armPoseEpoch&&g_m133SightContext57==context;
 Mat4 fixedGrip249={};
 if(ControllerCurrentRoot124(parent,fixedGrip249)){}
 else if(confirmed)parent=g_m133SightParent57;
 else{
  // The first optic metal draw precedes the receiver. Recover that SAME
  // native receiver from its exact rigid optic relation, rather than using a
  // previous rendered frame. Existing native shotgun placement owns the move.
  Mat4 relativeInverse={},bodyInverse={};if(!g_frameGunValid||!g_armJoints[0].valid||!Invert(kM133ReflexBFromFrame57,relativeInverse)||!Invert(ArmBodyToCamera(),bodyInverse))return false;
  const Mat4 nativeFrameView=Multiply(HullView(summary),Multiply(HullWorld(globals),relativeInverse)),hand=Multiply(nativeFrameView,kShotgunHandFromGun);Mat4 basis={};if(!HandBasis(hand,basis))return false;ObserveNativeShotBasis(basis);
  ArmJoints predicted=g_armJoints[0];const V3 wrist=Apply(hand,{.591,.011,1.149});for(int a=0;a<3;++a){const double value=a==0?wrist.x:a==1?wrist.y:wrist.z;predicted.e[a]+=value-predicted.w[a];predicted.w[a]=value;}
  Mat4 move={};if(!GunMove(predicted,g_armJoints[1],move))return false;parent=Multiply(bodyInverse,Multiply(move,nativeFrameView));
 }
 const Mat4 world=Multiply(inverseView,Multiply(ArmBodyToCamera(),Multiply(parent,kM133ReflexBFromFrame57)));if(!WritePistolSplitWorld(globals,world))return false;
 static LONG logs=0;if(logs++<32)Log("[M133 SIGHT57] world child draw%u/%u frame%ld, %s parent; native palette preserved",start,count,g_frame,confirmed?"current receiver":"same-draw optic-derived receiver");return true;
}
void M133SightUploaded57(ID3D11DeviceContext* context,UINT start,UINT count,UINT bytes){if(!M133SightDraw57(context,start,count))return;static LONG logs=0;if(logs++<32)Log("[M133 SIGHT57] uploaded corrected CB0 draw%u/%u frame%ld bytes%u; material/decompression preserved",start,count,g_frame,bytes);}
