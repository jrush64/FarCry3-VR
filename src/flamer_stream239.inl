// Continuous flame does not use the projectile-shot callback. Capture the
// original native FRAME before palette movement, independently of that callback.
struct FlamerStream239 {Mat4 native={},move={};LONG frame=-100000;UINT epoch=~0u;bool valid=false;} g_flamerStream239;
const V3 kFlamerNozzle239={.00000009,.5829535723,.0815982521};
void CaptureFlamerStream239(ID3D11DeviceContext* context,UINT start,UINT count,const float* bones){
 if(context!=g_ctx||g_weaponMechanicProfile!=15||!MotionInteractionEnabled())return;
 DrawCall call={};call.b=start;call.a=count;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=349160||d.indexBytes!=68436||d.stride!=40||d.offset||d.format!=57)return;
 const int anchor=start==0&&count==1548?0:start==1548&&count==2082?2:start==3630&&count==8004?4:start==13338&&count==252?0:start==20838&&count==13380?2:-1;
 if(anchor<0||(g_flamerStream239.valid&&g_flamerStream239.frame==g_frame&&g_flamerStream239.epoch==g_armPoseEpoch))return;
 CameraSummary camera={};Mat4 viewInverse={},root={},grip={},nativeInverse={};
 if(!PistolGuideCamera(context,camera)||!camera.mainView||!Invert(HullView(camera),viewInverse)||!ControllerCurrentRoot124(root,grip))return;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);float rows[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,rows,sizeof(rows));SafeRelease(cb);const int offset=WeaponWorldOffset(context);
 if(bytes<UINT((offset+15)*4))return;
 const Mat4 native=Multiply(HullWorld(rows+offset),BoneMatrix(bones,anchor));if(!Invert(native,nativeInverse))return;
 const Mat4 target=Multiply(viewInverse,Multiply(ArmBodyToCamera(),root));
 g_flamerStream239={native,Multiply(target,nativeInverse),g_frame,g_armPoseEpoch,true};
}
bool FlamerStreamActive239(){return g_weaponMechanicProfile==15&&MotionInteractionEnabled()&&fc3pad::Latest().active&&!fc3pad::GameplayInputBlocked()&&
 g_handRightValid&&fc3pad::Latest().rightTrigger>.3f&&FlamerReady()&&g_flamerStream239.valid&&g_flamerStream239.epoch==g_armPoseEpoch&&g_flamerStream239.frame<=g_frame&&g_frame-g_flamerStream239.frame<=2;}
UINT MoveFlamerStream239(unsigned char* data,UINT bytes,UINT stride,UINT offset,const std::vector<UINT>& refs,const FlamerStream239& stream){
 Mat4 inverse={};if(!data||stride!=36||!Invert(stream.native,inverse)||refs.empty()||refs.size()>2048)return 0;
 std::vector<UINT> selected;bool nearNozzle=false;
 for(UINT vertex:refs){const auto at=static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(vertex)*stride;if(at+12>bytes)continue;
  float p[3];memcpy(p,data+at,12);if(!std::isfinite(p[0])||!std::isfinite(p[1])||!std::isfinite(p[2]))continue;
  const V3 local=Sub(Apply(inverse,{p[0],p[1],p[2]}),kFlamerNozzle239);const double radius=.75+(std::max)(0.,local.y)*.15;
  if(local.y<-.25||local.y>8||local.x*local.x+local.z*local.z>radius*radius)continue;
  selected.push_back(vertex);if(Length(local)<1)nearNozzle=true;
 }
 if(!nearNozzle)return 0; // disconnected world fires cannot authorize a stream
 for(UINT vertex:selected){const auto at=static_cast<size_t>(offset)+static_cast<size_t>(vertex)*stride;float p[3];memcpy(p,data+at,12);const V3 moved=Apply(stream.move,{p[0],p[1],p[2]});p[0]=float(moved.x);p[1]=float(moved.y);p[2]=float(moved.z);memcpy(data+at,p,12);}
 return UINT(selected.size());
}
// Captured8CBC shader: mov o0.xyz,v0.xyz; mov o0.w,1. Its POSITION
// is already NDC. Undo the native perspective before applying world transport.
bool FlamerProject240(const CameraSummary& camera,const Mat4& view,const V3& world,V3& out){
 const V3 p=Apply(view,world);if(!std::isfinite(p.z)||p.z<=1e-5)return false;
 out={camera.xScale*p.x/p.z,camera.yScale*p.y/p.z,camera.zScale+camera.zOffset/p.z};
 return std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z);
}
UINT MoveFlamerClipStream240(unsigned char* data,UINT bytes,UINT stride,UINT offset,const std::vector<UINT>& refs,const FlamerStream239& stream,const CameraSummary& camera){
 Mat4 inverse={},view=HullView(camera),cameraInverse={};
 if(!data||stride!=36||refs.empty()||refs.size()>2048||!camera.mainView||camera.xScale<=0||camera.yScale<=0||fabs(camera.zOffset)<1e-8||!Invert(view,cameraInverse)||!Invert(stream.native,inverse))return 0;
 struct Vertex{UINT index;V3 projected;};std::vector<Vertex> selected;bool nearNozzle=false;
 for(UINT vertex:refs){const auto at=static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(vertex)*stride;if(at+12>bytes)return 0;
  float p[3];memcpy(p,data+at,12);const double denominator=double(p[2])-camera.zScale;
  if(!std::isfinite(p[0])||!std::isfinite(p[1])||!std::isfinite(p[2])||fabs(denominator)<1e-8)continue;
  const double depth=camera.zOffset/denominator;if(depth<=1e-5||depth>10000)continue;
  const V3 world=Apply(cameraInverse,{p[0]*depth/camera.xScale,p[1]*depth/camera.yScale,depth});
  const V3 local=Sub(Apply(inverse,world),kFlamerNozzle239);const double radius=.75+(std::max)(0.,local.y)*.15;
  if(local.y<-.25||local.y>8||local.x*local.x+local.z*local.z>radius*radius)continue;
  V3 projected={};if(!FlamerProject240(camera,view,Apply(stream.move,world),projected))return 0;
  selected.push_back({vertex,projected});if(Length(local)<1)nearNozzle=true;
 }
 if(!nearNozzle)return 0;
 for(const auto& v:selected){float p[3]={float(v.projected.x),float(v.projected.y),float(v.projected.z)};memcpy(data+static_cast<size_t>(offset)+static_cast<size_t>(v.index)*stride,p,12);}
 return UINT(selected.size());
}
