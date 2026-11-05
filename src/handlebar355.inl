#include "handlebar_assets355.inl"
fc3bars355::Bars g_bars355;
unsigned long long g_barOwner355=0;int g_barModel355=-1;
LONG g_barTick355=-100;ULONGLONG g_barSeen355=0,g_barBodyAt355=0;
double g_barNativeAngle355=0;Mat4 g_bodyToBar355={};
ID3D11Buffer* g_barCb355[8]={};UINT g_barCbSize355[8]={};
Mat4 BarTurn355(double a){Mat4 m={};m.m[0]=m.m[5]=std::cos(a);m.m[1]=-std::sin(a);m.m[4]=std::sin(a);m.m[10]=m.m[15]=1;return m;}
bool HandlebarDriver355(const fc3aim::VehicleSample170& v){return g_barModel355>=0&&v.identityValid&&v.vehicleId==g_barOwner355&&v.seatValid&&v.actualSeat==1&&v.type==kBarAssets355[g_barModel355].type;}
void DropBars355(){g_bars355.Drop();g_bars355.wait[0]=g_bars355.wait[1]=true;fc3pad::SetHandlebarInput355(0,-1,0);}
void TickHandlebars355(const CameraSummary& camera){
 if(g_barModel355<0||!g_barOwner355||!camera.mainView)return;
 // A water/reflection view cannot become the steering reference ahead of
 // the main world draw. Use the existing read-only native camera proof.
 if(g_nativeVehicleCamera279){float body[9]={};UINT serial=0;
  if(!fc3aim::VehicleCameraBasis279(camera.right,camera.up,camera.forward,body,serial))return;}
 fc3aim::VehicleSample170 v={};if(!fc3aim::GetVehicleDrawPose352(v)||!HandlebarDriver355(v)){
  DropBars355();g_barOwner355=0;g_barModel355=-1;fc3pad::SetHandlebarContext355(0,-1);return;
 }
 const auto& asset=kBarAssets355[g_barModel355];Mat4 inverse={};if(!Invert(Multiply(CarPose352(v),asset.rest),inverse)){DropBars355();return;}
 Mat4 cameraWorld={};cameraWorld.m[15]=1;
 const float* axes[3]={camera.right,camera.up,camera.forward};
 for(int r=0;r<3;++r){for(int c=0;c<3;++c)cameraWorld.m[r*4+c]=axes[c][r];cameraWorld.m[r*4+3]=camera.position[r];}
 g_bodyToBar355=Multiply(inverse,Multiply(cameraWorld,CarHeadView352()));
 for(double value:g_bodyToBar355.m)if(!std::isfinite(value)){DropBars355();return;}
 g_barBodyAt355=GetTickCount64();
 if(g_barTick355==g_frame)return;
 const auto controls=fc3pad::Latest();fc3bars355::Input in;in.id=v.vehicleId;in.model=g_barModel355;in.now=g_barBodyAt355;
 in.lock=fc3pad::HandlebarLock355()*fc3bars355::Pi/180;in.smoothing363=fc3pad::VehicleSmoothing363(true)*.001;
 in.nativeAngle=in.now-g_barSeen355<120?g_barNativeAngle355:0;
 in.allowed=fc3pad::HandlebarEnabled355()&&g_enabled&&g_headTracking&&g_eyeViewsSet&&g_inGameplay&&!g_flatViewActive&&
  !g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&!fc3pad::NativeMenuOpen()&&controls.active&&!controls.menu&&!controls.y&&
  std::fabs(controls.leftX)<.2f&&in.now-g_physicalAt352<120;
 for(int h=0;h<2;++h){in.end[h]={asset.grip[h].x,asset.grip[h].y,asset.grip[h].z};auto& hand=in.hand[h];hand.tracked=g_physicalValid303[h];
  hand.grip=controls.weaponRoles303?(h?controls.physicalLeftGrip303:controls.physicalRightGrip303):(h?controls.leftGrip:controls.rightGrip);
  if(hand.tracked){Mat4 pose={};for(int i=0;i<16;++i)pose.m[i]=g_physicalBody303[h][i];const V3 p=Apply(Multiply(g_bodyToBar355,pose),{0,-.015,.015});hand.p={p.x,p.y,p.z};}
 }
 const int previous=(g_bars355.held[0]?1:0)|(g_bars355.held[1]?2:0);const bool held=g_bars355.Update(in);g_barTick355=g_frame;
 fc3pad::SetHandlebarInput355(held?v.vehicleId:0,v.type,float(g_bars355.Demand(in.lock)));
 const int current=(g_bars355.held[0]?1:0)|(g_bars355.held[1]?2:0);
 if(previous!=current)Log("[BARS355 GRIP] vehicle=%llu model=%s mask=%d angle=%.4f steer=%.4f release=%u/%u (1 tracking,2 grip,3 depth,4 radius,5 jump; right1 left2 both3)",v.vehicleId,asset.name,current,g_bars355.angle,g_bars355.Demand(in.lock),g_bars355.release363[0],g_bars355.release363[1]);
 static unsigned rows=0;static ULONGLONG next=0;
 if(in.now>=next&&rows<160){next=in.now+500;++rows;Log("[BARS355 CONTACT] vehicle=%llu model=%s type=%d seat=%d R=%.3f/%.3f/%.3f grip=%.2f L=%.3f/%.3f/%.3f grip=%.2f mask=%d",
  v.vehicleId,asset.name,v.type,v.actualSeat,in.hand[0].p.x,in.hand[0].p.y,in.hand[0].p.z,in.hand[0].grip,in.hand[1].p.x,in.hand[1].p.y,in.hand[1].p.z,in.hand[1].grip,current);}
}
int BarModel355(const fc3weapon::Geometry& g){
 if(g.stride!=32||g.offset||g.format!=57)return -1;
 for(const auto& d:kBarDraws355)if(g.vertexBytes==d.vb&&g.indexBytes==d.ib&&g.start==d.start&&g.count==d.count)return int(d.model);return -1;
}
ID3D11Buffer* BuildHandlebarWorld355(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 if(variant!=0||call.kind||call.d)return nullptr;
 bool candidate=false;for(const auto& d:kBarDraws355)if(d.start==call.b&&d.count==call.a){candidate=true;break;}if(!candidate)return nullptr;
 const int model=BarModel355(NativeWeaponGeometry(context,call));if(model<0)return nullptr;
 fc3aim::VehicleSample170 v={};if(!fc3aim::GetVehicleDrawPose352(v))return nullptr;
 static unsigned rejected[2]={};const auto& asset=kBarAssets355[model];
 if(!v.seatValid||v.actualSeat!=1||v.type!=asset.type){if(rejected[model]++<6)Log("[BARS355 BIND] rejected model=%s type=%d expectedType=%d seat=%d; driver role1 required",asset.name,v.type,asset.type,v.actualSeat);return nullptr;}
 ID3D11Buffer* cb[2]={};context->VSGetConstantBuffers(0,2,cb);D3D11_BUFFER_DESC desc={};if(cb[0])cb[0]->GetDesc(&desc);
 float globals[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb[0],globals,sizeof(globals));CameraSummary camera={};UINT size=sizeof(camera);
 const bool cameraValid=cb[1]&&SUCCEEDED(cb[1]->GetPrivateData(kCameraSummaryGuid,&size,&camera))&&size==sizeof(camera)&&camera.mainView;
 SafeRelease(cb[0]);SafeRelease(cb[1]);bool fractions=false;const int off=WeaponWorldOffset(context,&fractions);
 if(!cameraValid||bytes!=desc.ByteWidth||bytes<UINT((off+(fractions?15:12))*4))return nullptr;
 Mat4 native={};native.m[15]=1;for(int r=0;r<3;++r)for(int c=0;c<4;++c)native.m[r*4+c]=globals[off+r*4+c]+(c==3&&fractions?globals[off+12+r]:0);
 for(double value:native.m)if(!std::isfinite(value))return nullptr;
 const Mat4 base=Multiply(CarPose352(v),asset.rest);Mat4 inverse={};if(!Invert(base,inverse))return nullptr;const Mat4 relative=Multiply(inverse,native);
 if(Length({relative.m[3],relative.m[7],relative.m[11]})>.12||std::fabs(relative.m[2])>.04||std::fabs(relative.m[6])>.04||
    std::fabs(relative.m[10]-1)>.04||std::fabs(std::hypot(relative.m[0],relative.m[4])-1)>.04){
  if(rejected[model]++<10)Log("[BARS355 BIND] other-instance/rest rejected model=%s relativeOrigin=%.4f/%.4f/%.4f axisZ=%.4f/%.4f/%.4f",asset.name,relative.m[3],relative.m[7],relative.m[11],relative.m[2],relative.m[6],relative.m[10]);return nullptr;
 }
 if(g_barOwner355!=v.vehicleId||g_barModel355!=model){DropBars355();g_bars355={};g_barOwner355=v.vehicleId;g_barModel355=model;g_barTick355=-100;fc3pad::SetHandlebarContext355(v.vehicleId,v.type);
  Log("[BARS355 BIND] matched current driver vehicle=%llu model=%s type=%d seat=%d; fixed stem localZ, free hands available",v.vehicleId,asset.name,v.type,v.actualSeat);}
 g_barNativeAngle355=std::atan2(relative.m[4],relative.m[0]);g_barSeen355=GetTickCount64();TickHandlebars355(camera);
 if(!g_bars355.Active())return nullptr;
 const Mat4 controlled=Multiply(base,BarTurn355(g_bars355.angle));
 for(int r=0;r<3;++r)for(int c=0;c<4;++c)globals[off+r*4+c]=float(controlled.m[r*4+c]-(c==3&&fractions?globals[off+12+r]:0));
 int slot=-1;for(int i=0;i<8;++i)if(g_barCbSize355[i]==bytes){slot=i;break;}
 if(slot<0)for(int i=0;i<8;++i)if(!g_barCb355[i]){D3D11_BUFFER_DESC d={};d.ByteWidth=bytes;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(SUCCEEDED(g_oCreateBuffer(g_dev,&d,nullptr,&g_barCb355[i]))){slot=i;g_barCbSize355[i]=bytes;}break;}
 if(slot<0){DropBars355();return nullptr;}
 const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_barCb355[slot],0,nullptr,globals,0,0);g_own=own;return g_barCb355[slot];
}
bool BarAttached355(unsigned long long id,int side){return side>=0&&side<2&&g_barModel355>=0&&g_barOwner355==id&&g_bars355.held[side]&&GetTickCount64()-g_barBodyAt355<120;}
void BarPlaceController355(unsigned long long id,int side,Mat4& pose){
 if(!BarAttached355(id,side))return;Mat4 inverse={};if(!Invert(g_bodyToBar355,inverse))return;
 const V3 target=Apply(inverse,Apply(BarTurn355(g_bars355.angle),kBarAssets355[g_barModel355].grip[side]));
 const V3 d=Sub(target,Apply(pose,{0,-.015,.015}));pose.m[3]+=d.x;pose.m[7]+=d.y;pose.m[11]+=d.z;
}
struct BarHand355{unsigned long long id=0;LONG frame=-100;unsigned quiet=0;bool valid=false;Mat4 hand={},finger[17]={},candidate[17]={};};
BarHand355 g_barHand355[2];
void CaptureBarHand355(unsigned long long id,int side,const Mat4& toView,const float* bones){
 if(side<0||side>1||id!=g_barOwner355||g_barModel355<0||GetTickCount64()-g_barSeen355>=120)return;
 auto& p=g_barHand355[side];if(p.id!=id){p={};p.id=id;}if(p.valid||p.frame==g_frame)return;
 Mat4 inverseHead={},inverseWrist={};if(!Invert(CarHeadView352(),inverseHead)||!BoneUsable(bones+24)||!Invert(BoneMatrix(bones,2),inverseWrist))return;
 const Mat4 native=Multiply(g_bodyToBar355,Multiply(inverseHead,Multiply(toView,BoneMatrix(bones,2))));
 const Mat4 rest=Multiply(BarTurn355(-g_barNativeAngle355),native);
 for(double value:rest.m)if(!std::isfinite(value))return;
 const V3 palm=Apply(rest,{side?-.6781:.6781,.0519,1.0387});
 if(Length(Sub(palm,kBarAssets355[g_barModel355].grip[side]))>.18)return;
 Mat4 finger[17]={};bool quiet=p.quiet&&g_frame-p.frame<=2;
 for(int b=3;b<=19;++b){if(!BoneUsable(bones+b*12))return;finger[b-3]=Multiply(inverseWrist,BoneMatrix(bones,b));
  for(int i=0;i<12;++i){if(!std::isfinite(finger[b-3].m[i]))return;if(std::fabs(finger[b-3].m[i]-p.candidate[b-3].m[i])>.012)quiet=false;}}
 p.frame=g_frame;p.quiet=quiet?p.quiet+1:1;memcpy(p.candidate,finger,sizeof(finger));if(p.quiet<6)return;
 p.hand=rest;memcpy(p.finger,finger,sizeof(finger));p.valid=true;
 Log("[BARS355 HAND] native handle grip captured vehicle=%llu side=%d model=%s",id,side,kBarAssets355[g_barModel355].name);
}
bool BarGripMove355(unsigned long long id,int side,const Mat4& neutral,Mat4& move){
 const auto& p=g_barHand355[side];if(!BarAttached355(id,side)||!p.valid||p.id!=id)return false;Mat4 inverse={},invNeutral={};
 if(!Invert(g_bodyToBar355,inverse)||!Invert(neutral,invNeutral))return false;
 Mat4 desired=Multiply(BarTurn355(g_bars355.angle),p.hand);const auto target=Apply(BarTurn355(g_bars355.angle),kBarAssets355[g_barModel355].grip[side]);const auto delta=Sub(target,Apply(desired,{side?-.6781:.6781,.0519,1.0387}));desired.m[3]+=delta.x;desired.m[7]+=delta.y;desired.m[11]+=delta.z;
 move=Multiply(Multiply(CarHeadView352(),Multiply(inverse,desired)),invNeutral);return true;
}
bool BarGripFingers355(unsigned long long id,int side,float* bones){
 const auto& p=g_barHand355[side];if(!BarAttached355(id,side)||!p.valid||p.id!=id)return false;const Mat4 wrist=BoneMatrix(bones,2);
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,p.finger[b-3]));return true;
}
