
// Promote only an actual core weapon draw. Never use optics, reload parts or a
// controller estimate as the authoritative native->rendered muzzle transport.
const GUID kMuzzlePaletteSlotGuid58={0x58d3d110,0x6c41,0x481b,{0xa4,0x80,0x6d,0x75,0x7a,0x7a,0x6c,0x65}};
struct MuzzleRenderStage58 {Mat4 native={},finalWorld={},view={};ID3D11DeviceContext* context=nullptr;ID3D11Buffer* buffer=nullptr;LONG frame=-1;UINT epoch=0,start=0,count=0,profile=~0u;bool valid=false;float forward[3]={};}g_muzzleRender58;
bool MuzzleMirrorReceipt340(ID3D11Buffer* buffer,int profile){
 BOOL reflected=FALSE;UINT bytes=sizeof(reflected);
 return buffer&&fc3mirror338::Supported(profile)&&SUCCEEDED(buffer->GetPrivateData(kA2000Mirror337,&bytes,&reflected))&&bytes==sizeof(reflected)&&reflected;
}
// A confirmed core draw remains the laser's eye anchor through later optics,
// magazine and arm draws. The staging record above is cleared on every draw.
struct MuzzleLaserFrame203 {Mat4 view={},move={};LONG frame=-1;UINT epoch=0,profile=~0u;bool valid=false;Mat4 native={};V3 barrelWorld={};bool barrelValid=false;}g_muzzleLaserFrame203;
// Candidate 206: ONE barrel ray for shot, laser and sight dot, measured in the drawn weapon's own frame.
// The aim ray (controller root +Y, steadiness-filtered) is expressed in the confirmed core draw's FRAME each
// frame and lightly averaged; the published barrel is that gun-local axis turned by the CURRENT drawn frame,
// so everything that follows it is rigid with the visible gun instead of one or more frames behind it.
// Keyed on the weapon FAMILY (870 variants 3-6 flap every frame), never on the exact profile.
int BarrelFamily206(int profile){return profile>=3&&profile<=6?3:profile;}
struct BarrelLocal206 {V3 local={};LONG frame=-1000;UINT epoch=0;int family=-1;bool twoHand=false;int samples=0;double jumpDeg=0,aimDeltaDeg=0;V3 lastAim={},lastAxis={};bool learned=false;}g_barrel206;
V3 g_barrelBody206={},g_barrelWorld206={};LONG g_barrelFrame206=-1000;UINT g_barrelEpoch206=0;int g_barrelFamily206=-1;
extern double g_bodyAxes[3][3];
bool BarrelBody206(V3& body){
 if(std::abs(g_frame-g_barrelFrame206)>1||g_barrelEpoch206!=g_armPoseEpoch||g_barrelFamily206!=BarrelFamily206(int(g_weaponMechanicProfile))||Length(g_barrelBody206)<.5)return false;
 body=g_barrelBody206;return true;
}
// Pure: update the gun-local barrel average from this frame's aim ray and drawn frame axes.
// 209: the barrel is RIGID with the drawn gun: its own +Y axis (measured within 1 deg at rest on every weapon in
// the 206/207 logs) plus a small offset learned ONLY while both the gun and the aim ray are still. During a
// right-stick turn the raw aim ray of the magazine-fed guns swings 6-7 deg off the drawn gun for a few frames
// (A2000 log: 'jump 7.05 deg, 1 samples'); 206 followed it and the laser trailed the gun. Now it never does.
double AngleDeg206(V3 a,V3 b){return std::acos((std::max)(-1.,(std::min)(1.,Dot(Unit(a),Unit(b)))))*57.2957795;}
V3 BarrelUpdate206(BarrelLocal206& b,const V3& aimWorld,const V3& axisX,const V3& axisY,const V3& axisZ,int family,bool twoHand,LONG frame,UINT epoch){
 const V3 local=Unit({Dot(aimWorld,axisX),Dot(aimWorld,axisY),Dot(aimWorld,axisZ)});
 const bool continuous=b.samples>0&&b.epoch==epoch&&b.family==family&&b.twoHand==twoHand&&frame>=b.frame&&frame-b.frame<=10;
 if(!continuous){b=BarrelLocal206{};b.local={0,1,0};}
 const double aimMove=continuous?AngleDeg206(aimWorld,b.lastAim):1e9,gunMove=continuous?AngleDeg206(axisY,b.lastAxis):1e9;
 const double offY=AngleDeg206(local,{0,1,0});
 const bool still=aimMove<.25&&gunMove<.25;
 double jump=0;
 if(still&&offY<=3){const V3 before=b.local;b.local=b.learned?Unit(Add(b.local,Scale(Sub(local,b.local),.1))):local;b.learned=true;jump=AngleDeg206(before,b.local);++b.samples;}
 else if(!b.learned)b.local={0,1,0};
 b.lastAim=aimWorld;b.lastAxis=axisY;b.frame=frame;b.epoch=epoch;b.family=family;b.twoHand=twoHand;b.jumpDeg=jump;if(!b.samples)b.samples=1;
 const V3 world=Unit(Add(Add(Scale(axisX,b.local.x),Scale(axisY,b.local.y)),Scale(axisZ,b.local.z)));
 b.aimDeltaDeg=AngleDeg206(world,aimWorld);
 {static ULONGLONG nextLag209=0;static LONG lagLines209=0;const ULONGLONG now=GetTickCount64();
  if(b.aimDeltaDeg>2&&lagLines209<60&&now>=nextLag209){nextLag209=now+1000;++lagLines209;Log("[BARREL209] family%d frame%ld raw aim ray %.2f deg off the drawn barrel (aim moved %.2f, gun moved %.2f deg this frame); laser and shot stay on the drawn barrel",family,frame,b.aimDeltaDeg,aimMove<1e8?aimMove:0.,gunMove<1e8?gunMove:0.);}}
 return world;
}
// The aim/reticle ray of the frame in world axes (AimFrame, before the head-relative camera cone).
LONG g_shotAimProfile129=-1;
V3 g_shotAimWorld107={};LONG g_shotAimFrame107=-1000;UINT g_shotAimEpoch107=0;
// Candidate 111: the confirmed native FRAME and native->displayed transport of the latest core draw,
// for the muzzle flash/tracer particles (muzzle_flash111.inl).
struct MuzzleFlashPose111 {Mat4 native={},move={};LONG frame=-100000;UINT epoch=0,profile=~0u;bool valid=false;} g_muzzleFlashPose111;
void MuzzleDiscardStage58(){g_muzzleRender58.valid=false;}
int MuzzleCore58(ID3D11DeviceContext* context,UINT start,UINT count,int& anchor){DrawCall call={};call.a=count;call.b=start;const auto geom=NativeWeaponGeometry(context,call);int profile=fc3weapon::ProfileIndex(geom);anchor=0;
 if(geom.vertexBytes==230760&&geom.indexBytes==44856&&geom.stride==40&&geom.offset==0&&geom.format==57&&start==14982&&count==7140)return 10;
 switch(profile){case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 9:case 12:case 14:case 16:case 17:case 18:case 20:case 21:case 22:case 23:case 26:case 31:case 32:case 34:case 35:case 36:case 38:case 39:case 41:case 42:case 43:return profile;case 15:anchor=2;return profile;case 25:anchor=3;return profile;case 28:anchor=4;return profile;default:return -1;}
}
// Reflect the original shader once. A2304B arm buffer left bound to a rigid
// gun shader is not evidence that the gun consumes a skinning palette.
int MuzzlePaletteSlot58(ID3D11DeviceContext* context){ID3D11VertexShader* shader=nullptr;context->VSGetShader(&shader,nullptr,nullptr);if(!shader)return -2;UINT encoded=~0u,size=sizeof(encoded);if(SUCCEEDED(shader->GetPrivateData(kMuzzlePaletteSlotGuid58,&size,&encoded))){SafeRelease(shader);return encoded==~0u?-2:int(encoded)-1;}
 UINT bytes=0;std::vector<BYTE> code;int result=-2;if(SUCCEEDED(shader->GetPrivateData(kEjectionBytecodeGuid,&bytes,nullptr))&&bytes&&bytes<=65536){code.resize(bytes);if(SUCCEEDED(shader->GetPrivateData(kEjectionBytecodeGuid,&bytes,code.data()))){ID3D11ShaderReflection* reflection=nullptr;if(SUCCEEDED(D3DReflect(code.data(),bytes,IID_ID3D11ShaderReflection,reinterpret_cast<void**>(&reflection)))){D3D11_SHADER_DESC desc={};if(SUCCEEDED(reflection->GetDesc(&desc))){result=-1;for(UINT i=0;i<desc.BoundResources;++i){D3D11_SHADER_INPUT_BIND_DESC binding={};if(SUCCEEDED(reflection->GetResourceBindingDesc(i,&binding))&&binding.Type==D3D_SIT_CBUFFER&&binding.Name&&strstr(binding.Name,"BlendMatrices")){if(result>=0){result=-2;break;}result=int(binding.BindPoint);}}}SafeRelease(reflection);}}}
 encoded=result==-2?~0u:UINT(result+1);shader->SetPrivateData(kMuzzlePaletteSlotGuid58,sizeof(encoded),&encoded);SafeRelease(shader);return result;
}
bool MuzzleReadNative58(ID3D11DeviceContext* context,UINT start,UINT count,const float* palette,Mat4& native,int& profile,int& anchor,UINT paletteSlot){profile=MuzzleCore58(context,start,count,anchor);if(profile<0)return false;const int slot=MuzzlePaletteSlot58(context);if(slot==-2)return false;ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);float rows[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,rows,sizeof(rows));SafeRelease(cb);const int offset=WeaponWorldOffset(context);if(bytes<UINT((offset+15)*4))return false;native=HullWorld(rows+offset);
 if(slot>=0){if(palette){if(slot!=int(paletteSlot))return false;native=Multiply(native,BoneMatrix(palette,anchor));}else{context->VSGetConstantBuffers(UINT(slot),1,&cb);float source[576]={};const UINT boneBytes=ReadCopy(cb,source,sizeof(source));SafeRelease(cb);if(boneBytes<UINT((anchor+1)*48))return false;native=Multiply(native,BoneMatrix(source,anchor));}}
 return true;
}
void MuzzleStage58(ID3D11DeviceContext* context,UINT start,UINT count,ID3D11Buffer* buffer,const Mat4& native,const Mat4& finalWorld,int profile){if(!buffer||profile<0)return;ID3D11Buffer* camera=nullptr;context->VSGetConstantBuffers(1,1,&camera);CameraSummary summary={};UINT bytes=sizeof(summary);const bool main=camera&&SUCCEEDED(camera->GetPrivateData(kCameraSummaryGuid,&bytes,&summary))&&bytes==sizeof(summary)&&summary.mainView;SafeRelease(camera);if(!main)return;g_muzzleRender58={native,finalWorld,HullView(summary),context,buffer,g_frame,g_armPoseEpoch,start,count,UINT(profile),true};memcpy(g_muzzleRender58.forward,summary.forward,sizeof(g_muzzleRender58.forward));}
bool MuzzleDrawTransport58(ID3D11DeviceContext* context,const DrawCall& call,ID3D11Buffer* buffer,int variant,Mat4& move,V3& source){const auto& p=g_muzzleRender58;if(variant!=1||!p.valid||p.context!=context||p.buffer!=buffer||p.frame!=g_frame||p.epoch!=g_armPoseEpoch||call.kind||call.b!=p.start||call.a!=p.count||context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||g_guideShaderPlacementFrame[1]!=g_frame)return false;Mat4 inverseView={},inverseNative={};if(!Invert(p.view,inverseView)||!Invert(p.native,inverseNative))return false;const Mat4 final=Multiply(inverseView,Multiply(g_guideShaderPlacement[1],Multiply(p.view,p.finalWorld)));move=Multiply(final,inverseNative);source={p.native.m[3],p.native.m[7],p.native.m[11]};return true;}
void MuzzleConfirm58(ID3D11DeviceContext* context,const DrawCall& call,ID3D11Buffer* buffer,int variant){Mat4 move={};V3 root={};if(!MuzzleDrawTransport58(context,call,buffer,variant,move,root))return;
 // 211: the core weapon draw is confirmed up to three times per frame (G-buffer, then later passes such as
 // velocity/depth-only that may bind LAST frame's camera). Only the FIRST pass of a frame publishes: it is the pass
 // whose depth the laser is tested against and whose camera drew the gun the player sees. A later pass with a
 // different camera used to overwrite the frame, so the beam was computed from a stale camera and projected with
 // the current one: beside the gun, only while turning with the right stick (reported on SMGs).
 {static LONG passFrame211=-1;static int passIndex211=0;static Mat4 firstView211={};static V3 firstRoot211={};
  if(passFrame211!=g_frame){passFrame211=g_frame;passIndex211=0;}
  ++passIndex211;
  if(passIndex211==1){firstView211=g_muzzleRender58.view;firstRoot211=Apply(Multiply(move,g_muzzleRender58.native),{0,0,0});}
  else{static LONG diag211=0;static ULONGLONG next211=0;const ULONGLONG t=GetTickCount64();
   const double df=AngleDeg206({firstView211.m[8],firstView211.m[9],firstView211.m[10]},{g_muzzleRender58.view.m[8],g_muzzleRender58.view.m[9],g_muzzleRender58.view.m[10]});
   const double dr=Length(Sub(Apply(Multiply(move,g_muzzleRender58.native),{0,0,0}),firstRoot211));
   if((df>.05||dr>.005)&&diag211<60&&t>=next211){next211=t+1000;++diag211;Log("[PASS211] frame%ld core pass %d differs from pass 1: camera forward %.3f deg, drawn root %.1f mm; pass 1 owns shot/laser/sight",g_frame,passIndex211,df,dr*1000);}
   return;}
 }
 const ULONGLONG now=GetTickCount64();
 // 208: the pawn's equipped weapon first; the HUD ammo widget's wrapper (refreshes only when the ammo count changes)
 // is the fallback. A weapon switch therefore publishes on its first drawn frame, before any shot.
 UINT wrapper=0,weapon=0,strategy208=0;if(!fc3aim::EquippedWeapon206(wrapper,weapon,strategy208)){wrapper=UINT(g_reload.lastWrapper);weapon=wrapper?ReloadRead(wrapper,0x48):0;}
 const bool active=MotionInteractionEnabled()&&g_handRightValid&&fc3pad::Latest().active&&!fc3pad::GameplayInputBlocked()&&!g_pistolMenu.open&&!fc3pad::NativeMenuOpen()&&!fc3pad::WeaponSelectionRequested()&&g_guideShaderPlacementFrame[1]==g_frame;
 // 206: the drawn weapon frame (game world) and the barrel ray measured in it.
 const Mat4 drawn206=Multiply(move,g_muzzleRender58.native);
 V3 axisX206={drawn206.m[0],drawn206.m[4],drawn206.m[8]},axisY206={drawn206.m[1],drawn206.m[5],drawn206.m[9]},axisZ206={drawn206.m[2],drawn206.m[6],drawn206.m[10]};
 const bool rigid206=Length(axisX206)>.5&&Length(axisY206)>.5&&Length(axisZ206)>.5;axisX206=Unit(axisX206);axisY206=Unit(axisY206);axisZ206=Unit(axisZ206);
 const int family206=BarrelFamily206(int(g_muzzleRender58.profile));
 const bool aimFresh206=active&&g_aimActive&&BarrelFamily206(int(g_shotAimProfile129))==family206&&g_shotAimEpoch107==g_armPoseEpoch&&std::abs(g_frame-g_shotAimFrame107)<=1&&Length(g_shotAimWorld107)>.5;
 V3 barrelWorld206={};bool barrelValid206=false;
 if(rigid206&&aimFresh206){barrelWorld206=BarrelUpdate206(g_barrel206,Unit(g_shotAimWorld107),axisX206,axisY206,axisZ206,family206,g_twoHand,g_frame,g_armPoseEpoch);
  barrelValid206=std::isfinite(barrelWorld206.x)&&std::isfinite(barrelWorld206.y)&&std::isfinite(barrelWorld206.z)&&Length(barrelWorld206)>.5;}
 if(barrelValid206){g_barrelWorld206=barrelWorld206;g_barrelBody206={Dot(barrelWorld206,{g_bodyAxes[0][0],g_bodyAxes[0][1],g_bodyAxes[0][2]}),Dot(barrelWorld206,{g_bodyAxes[1][0],g_bodyAxes[1][1],g_bodyAxes[1][2]}),Dot(barrelWorld206,{g_bodyAxes[2][0],g_bodyAxes[2][1],g_bodyAxes[2][2]})};g_barrelFrame206=g_frame;g_barrelEpoch206=g_armPoseEpoch;g_barrelFamily206=family206;}
 if(active){g_muzzleLaserFrame203={g_muzzleRender58.view,move,g_frame,g_armPoseEpoch,g_muzzleRender58.profile,true};g_muzzleLaserFrame203.native=g_muzzleRender58.native;g_muzzleLaserFrame203.barrelWorld=barrelWorld206;g_muzzleLaserFrame203.barrelValid=barrelValid206;}
 if(active){g_muzzleFlashPose111.native=g_muzzleRender58.native;g_muzzleFlashPose111.move=move;g_muzzleFlashPose111.frame=g_frame;g_muzzleFlashPose111.epoch=g_armPoseEpoch;g_muzzleFlashPose111.profile=g_muzzleRender58.profile;g_muzzleFlashPose111.valid=true;}
 const bool mirrorReceipt340=MuzzleMirrorReceipt340(buffer,int(g_muzzleRender58.profile));
 float transport[12],origin[3]={float(root.x),float(root.y),float(root.z)};for(int i=0;i<12;++i)transport[i]=float(move.m[i]);fc3aim::PublishMuzzle58(transport,origin,wrapper,weapon,g_muzzleRender58.profile,g_armPoseEpoch,active,mirrorReceipt340);
 if(active){float nativeFrame[12];for(int i=0;i<12;++i)nativeFrame[i]=float(g_muzzleRender58.native.m[i]);fc3aim::PublishMuzzleFrame206(nativeFrame);}
 // The shot follows the measured barrel ray; the raw aim ray is only the fallback for the first frame of a weapon.
 if(barrelValid206){const float aim[3]={float(barrelWorld206.x),float(barrelWorld206.y),float(barrelWorld206.z)};fc3aim::PublishMuzzleAim107(aim,g_muzzleRender58.forward);}
 else if(aimFresh206){const float aim[3]={float(g_shotAimWorld107.x),float(g_shotAimWorld107.y),float(g_shotAimWorld107.z)};fc3aim::PublishMuzzleAim107(aim,g_muzzleRender58.forward);}
 {static ULONGLONG nextBarrelLog206[65]={};static LONG barrelLogs206=0;const int slot=family206>=0&&family206<64?family206:64;
  if(barrelValid206&&barrelLogs206<400&&now>=nextBarrelLog206[slot]){nextBarrelLog206[slot]=now+5000;++barrelLogs206;const auto& b=g_barrel206;
   Log("[BARREL206] profile%u %s: barrel in gun frame %.4f/%.4f/%.4f (%.2f deg from gun +Y), %d samples, frame-to-frame jump %.2f deg, barrel-vs-raw-aim %.2f deg; shot, laser and sight dot share this ray",
    g_muzzleRender58.profile,g_twoHand?"two-handed":"one-handed",b.local.x,b.local.y,b.local.z,std::acos((std::max)(-1.,(std::min)(1.,b.local.y)))*57.2957795,b.samples,b.jumpDeg,b.aimDeltaDeg);}
  else if(!barrelValid206&&active&&barrelLogs206<400&&now>=nextBarrelLog206[slot]){nextBarrelLog206[slot]=now+5000;++barrelLogs206;
   Log("[BARREL206] profile%u: no barrel this frame (rigid %d, aim fresh %d: aimActive %d aimProfile %ld epoch %u/%u aimFrame %ld/%ld); shot falls back to the raw aim ray",g_muzzleRender58.profile,rigid206?1:0,aimFresh206?1:0,g_aimActive?1:0,g_shotAimProfile129,g_shotAimEpoch107,g_armPoseEpoch,g_shotAimFrame107,g_frame);}}
 static LONG logs=0;if(active&&wrapper&&weapon&&logs++<20)Log("[MUZZLE FRAME58] actual core draw%u/%u profile%u frame%ld wrapper%08X weapon%08X, current stereo placement HUDage%llu ms",call.b,call.a,g_muzzleRender58.profile,g_frame,wrapper,weapon,now-g_reload.lastWidgetAt);
}
