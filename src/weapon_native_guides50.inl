bool NativeGuideScope50(){return g_weaponMechanicProfile==20||RifleProfile(g_weaponMechanicProfile);}
NativeGuideEye48 g_nativeWeaponEye50[2];LONG g_nativeWeaponProfile50=-1;
struct NativeGuidePacket50 {float clip[2][192][4]={};UINT count=0;int colour=0;bool holding=false;};
NativeGuidePacket50 g_nativeGuidePackets50[6];UINT g_nativeGuidePacketCount50=0;
void CaptureWeaponGuides50(ID3D11DeviceContext* context,const DrawCall& call,int variant,int eye){
 if(!NativeGuideScope50()||variant!=1||call.kind!=0||eye<0||eye>1)return;
 if(g_nativeWeaponProfile50!=g_weaponMechanicProfile){g_nativeWeaponProfile50=g_weaponMechanicProfile;g_nativeWeaponEye50[0]=g_nativeWeaponEye50[1]=NativeGuideEye48{};}
 if(g_weaponMechanicProfile==20){if(g_nativeGuideEye48[eye].valid&&g_nativeGuideEye48[eye].frame==g_frame)g_nativeWeaponEye50[eye]=g_nativeGuideEye48[eye];return;}
 if(!AkModel()||!AkFrame(call.b,call.a)||!RifleMain(CurrentRifle(),NativeWeaponGeometry(context,call)))return;
 if((g_weaponMechanicProfile==25&&call.b==10296)||(g_weaponMechanicProfile==43&&(call.b==10641||call.b==11661||call.b>=38097)))return;
 auto& out=g_nativeWeaponEye50[eye];if(out.valid&&out.frame==g_frame)return;
 ID3D11Buffer* cb[4]={};context->VSGetConstantBuffers(0,4,cb);float world[kSmallCbMax/4]={},camera[224]={},bones[576]={},stereo[64]={};
 const UINT worldBytes=ReadCopy(cb[0],world,sizeof(world)),cameraBytes=ReadCopy(cb[1],camera,sizeof(camera));Mat4 frame=HullWorld(world);
 bool valid=worldBytes>=60&&cameraBytes==sizeof(camera);
 if(CurrentRifle().skinned){Mp5Slots slots={};valid=valid&&Mp5DrawSlots(call.b,call.a,slots)&&slots.anchor>=0&&ReadCopy(cb[3],bones,sizeof(bones))==sizeof(bones);if(valid)frame=Multiply(frame,BoneMatrix(bones,slots.anchor));}
 if(A2000LeftCharge336()){BOOL reflected=FALSE;UINT size=sizeof(reflected);ID3D11Buffer* tag=cb[CurrentRifle().skinned?3:0];if(tag&&SUCCEEDED(tag->GetPrivateData(kA2000Mirror337,&size,&reflected))&&reflected){Mat4 m=HandIdentity();m.m[0]=-1;frame=Multiply(frame,m);}}
 for(auto*& b:cb)SafeRelease(b);ID3D11Buffer* eyeBuffer=nullptr;context->VSGetConstantBuffers(dxbc::kEyeCbSlot,1,&eyeBuffer);valid=valid&&ReadCopy(eyeBuffer,stereo,sizeof(stereo))==sizeof(stereo);SafeRelease(eyeBuffer);CameraSummary summary={};Mat4 inverse={};valid=valid&&PistolGuideCamera(context,summary)&&Invert(ArmBodyToCamera(),inverse);
 if(!valid)return;const Mat4 body=Multiply(inverse,Multiply(g_guideShaderPlacement[variant],Multiply(HullView(summary),frame)));Mat4 bodyInverse={};if(!Invert(body,bodyInverse))return;
 out=NativeGuideEye48{};out.anchor.world=frame;out.anchor.body=body;out.anchor.frame=g_frame;out.anchor.epoch=g_armPoseEpoch;out.anchor.context=context;out.anchor.weapon=g_pistolReload.weapon;out.anchor.wrapper=g_pistolReload.wrapper;out.anchor.valid=true;memcpy(out.camera,camera,sizeof(camera));for(int i=0;i<16;++i)out.eye[i]=stereo[i]+(i%5==0?1.f:0.f);out.frame=g_frame;out.valid=true;
}
bool NativeGuideReady50(){
 if(!NativeGuideScope50()||g_nativeWeaponProfile50!=g_weaponMechanicProfile)return false;
 for(const auto& e:g_nativeWeaponEye50)if(!e.valid||e.frame>g_frame||(!g_pistolMenu.open&&e.frame!=g_frame)||e.anchor.epoch!=g_armPoseEpoch||(e.anchor.weapon&&g_pistolReload.weapon&&e.anchor.weapon!=g_pistolReload.weapon)||(e.anchor.wrapper&&g_pistolReload.wrapper&&e.anchor.wrapper!=g_pistolReload.wrapper))return false;return true;
}
bool NativeWeaponContactRoot51(const Mat4& source,Mat4& root){
 if(&source!=&g_pistolReload.rootBody||g_pistolMenu.open||!NativeGuideReady50())return false;
 const auto& p=g_pistolReload;const auto& a=g_nativeWeaponEye50[0].anchor;
 if(p.rootFrame!=g_frame||p.rootEpoch!=g_armPoseEpoch||a.frame!=g_frame||p.rootContext!=a.context)return false;
 root=a.body;return true;
}
void NativeGuideShape50(const Mat4& shape,const V3* vertices,UINT count,int colour,bool holding){
 if(!NativeGuideReady50()||g_nativeGuidePacketCount50>=6||count>192||!count)return;
 auto& packet=g_nativeGuidePackets50[g_nativeGuidePacketCount50];packet.count=count;packet.colour=colour;packet.holding=holding;
 for(int eye=0;eye<2;++eye){Mat4 inverse={};const auto& draw=g_nativeWeaponEye50[eye];if(!Invert(draw.anchor.body,inverse))return;const Mat4 local=Multiply(inverse,shape);for(UINT i=0;i<count;++i)if(!Native1911Clip48(draw,Apply(local,vertices[i]),packet.clip[eye][i]))return;}
 ++g_nativeGuidePacketCount50;
}
void NativeGuideBox50(const Mat4& box,const float* dimensions,int colour,bool holding){
 const int edges[24]={0,1,2,3,4,5,6,7,0,2,1,3,4,6,5,7,0,4,1,5,2,6,3,7};V3 points[24];for(int i=0;i<24;++i){int corner=edges[i];points[i]={dimensions[0]*.5*(corner&1?1:-1),dimensions[1]*.5*(corner&2?1:-1),dimensions[2]*.5*(corner&4?1:-1)};}NativeGuideShape50(box,points,24,colour,holding);
}
void NativeGuideSphere50(V3 centre,double radius,int colour,bool holding){
 if(!std::isfinite(radius)||radius<=0)return;V3 points[192];UINT at=0;for(int axis=0;axis<3;++axis)for(int segment=0;segment<32;++segment)for(int end=0;end<2;++end){double a=(segment+end)*(6.283185307179586/32),v[3]={};v[(axis+1)%3]=cos(a)*radius;v[(axis+2)%3]=sin(a)*radius;points[at++]={v[0],v[1],v[2]};}Mat4 shape=HandIdentity();shape.m[3]=centre.x;shape.m[7]=centre.y;shape.m[11]=centre.z;NativeGuideShape50(shape,points,192,colour,holding);
}
void NativeWeaponBoxRequest50(const Mat4& root,int target){
 if(g_weaponMechanicProfile==20&&target==0){Native1911GuideRequest48(root,target);return;}
 // Weapon guides are local children of the actual captured FRAME, rather than
 // a later preview/contact observation projected through an earlier draw.
 if(!NativeGuideReady50()||g_nativeGuidePacketCount50>=6)return;
 auto& packet=g_nativeGuidePackets50[g_nativeGuidePacketCount50];packet.count=24;packet.colour=0;packet.holding=false;
 const int edges[24]={0,1,2,3,4,5,6,7,0,2,1,3,4,6,5,7,0,4,1,5,2,6,3,7};
 for(int eye=0;eye<2;++eye){const auto& draw=g_nativeWeaponEye50[eye];Mat4 inverse={};if(!Invert(draw.anchor.body,inverse))return;
  const Mat4 box=PistolGuideBox(draw.anchor.body,target),local=Multiply(inverse,box);V3 corners[8];
  for(int i=0;i<8;++i){const double scale=(std::max)(.1f,g_worldScale);const float* dimensions=g_pistolGuides.size[PistolBoxIndex(target)];corners[i]=Apply(local,{dimensions[0]*scale*.5*(i&1?1:-1),dimensions[1]*scale*.5*(i&2?1:-1),dimensions[2]*scale*.5*(i&4?1:-1)});}
  for(int i=0;i<24;++i)if(!Native1911Clip48(draw,corners[edges[i]],packet.clip[eye][i]))return;
 }
 ++g_nativeGuidePacketCount50;
}
void NativeReloadPoints50(V3 port,V3 hand,float radius,float handDiameter,bool weapon,bool palm,int target){
 if(!NativeGuideScope50())return;
 fc3xr::HideReloadMarkers();if(!g_showWeaponReloadGuides||!PhysicalReloadGuidesEnabled())return;
 if(weapon&&!g_pistolGuides.box&&target!=3&&NativeGuideReady50()&&g_nativeGuidePacketCount50<6){
  auto& packet=g_nativeGuidePackets50[g_nativeGuidePacketCount50];packet.count=192;packet.colour=0;packet.holding=false;
  const float* point=PistolGuidePoint(target);bool valid=true;
  for(int eye=0;eye<2;++eye){
   const auto& draw=g_nativeWeaponEye50[eye];double lengths[3]={};
   for(int col=0;col<3;++col){for(int r=0;r<3;++r)lengths[col]+=draw.anchor.body.m[r*4+col]*draw.anchor.body.m[r*4+col];lengths[col]=sqrt(lengths[col]);if(!std::isfinite(lengths[col])||lengths[col]<1e-8)valid=false;}
   if(!valid)break;UINT at=0;
   for(int axis=0;axis<3;++axis)for(int segment=0;segment<32;++segment)for(int end=0;end<2;++end){
    double v[3]={point[0],point[1],point[2]},a=(segment+end)*6.283185307179586/32;
    v[(axis+1)%3]+=cos(a)*radius/lengths[(axis+1)%3];v[(axis+2)%3]+=sin(a)*radius/lengths[(axis+2)%3];
    valid=Native1911Clip48(draw,{v[0],v[1],v[2]},packet.clip[eye][at++])&&valid;
   }
  }
  if(valid)++g_nativeGuidePacketCount50;
 }
 if(palm)NativeGuideSphere50(hand,handDiameter*.5*(std::max)(.1f,g_worldScale),1,false);
}
bool HoldingGuideEligible51(){
 if(g_pistolMenu.open)return true; // positioning preview never authorizes grip actions
 if(MacheteHeld()||BowActive()||Lever1887LeftHold()||R700LeftHold()||FlamerBusyLeft()||Gm94BusyLeft()||
  (RpgActive()&&(RpgClipPlaying()||RpgCarrying()))||FlareActive()||RevolverActive()||SpasManual()||
  g_reload.held||ManualReloadOwnsLeftHand()||PistolOwnsLeft())return false;
 return !fc3pad::GameplayInputBlocked()&&fc3pad::Latest().active&&!fc3pad::WeaponSelectionRequested()&&MotionInteractionEnabled()&&g_eyeViewsSet&&!(g_weaponMechanicProfile==39&&g_frame-g_weaponMechanicFrame<=2);
}
void NativeHoldingGuides50(){
 if(!NativeGuideReady50()||!g_grabGuides||!HoldingGuideEligible51()||!g_handLeftValid||!g_handRightValid||!g_armJoints[0].valid||!g_armJoints[1].valid)return;
 const bool held=g_twoHand;g_twoHand=false;Mat4 move={};const bool valid=GunMove(g_armJoints[0],g_armJoints[1],move);g_twoHand=held;if(!valid)return;
 V3 support=Apply(move,{g_armJoints[1].w[0],g_armJoints[1].w[1],g_armJoints[1].w[2]});if(!Lever1887SupportPosition(support)&&!R700SupportPosition(support)&&!Gm94SupportPosition(support))AkSupportPosition(support);V3 point={},palm={};if(!GrabGizmoPoints(support,point,palm,&move))return;
 Mat4 box={};if(!SupportGrabBox214(move,point,box))return;
 const float scale=(std::max)(.1f,g_worldScale);float dimensions[3];SupportGrabBoxSize214(g_weaponMechanicProfile,dimensions);for(float& d:dimensions)d*=scale;NativeGuideBox50(box,dimensions,1,true);NativeGuideSphere50(palm,(std::max)(.01f,(std::min)(.3f,float(kGrabPalmDiameter)))*.5*scale,2,true);
}
void RenderWeaponGuides50(ID3D11Texture2D* left,ID3D11Texture2D* right){
 if(!NativeGuideScope50()||!left||!right||fc3pad::NativeMenuOpen())return;fc3xr::HideGrabMarkers();NativeHoldingGuides50();for(UINT i=0;i<g_nativeGuidePacketCount50;++i){const auto& p=g_nativeGuidePackets50[i];if(p.holding?!g_grabGuides:(!g_showWeaponReloadGuides||!PhysicalReloadGuidesEnabled()))continue;DrawNative1911Guide48(g_ctx,left,&p.clip[0][0][0],p.count,p.colour);DrawNative1911Guide48(g_ctx,right,&p.clip[1][0][0],p.count,p.colour);}
}
