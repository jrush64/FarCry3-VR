#include "car_door357.h"
#include "car_door_assets363.inl"
fc3door357::State g_door357[2];
LONG g_doorFrame357[2]={-100,-100};
Mat4 g_doorWorld357={};ULONGLONG g_doorAt357=0;int g_doorModel363=-1;
const V3 kDoorHandle357=kDoorAssets363[0].handle;
int DoorModel363(const fc3weapon::Geometry& g){if(g.stride!=32||g.offset||g.format!=57)return -1;for(const auto& d:kDoorDraws363)if(g.vertexBytes==d.vb&&g.indexBytes==d.ib&&g.start==d.start&&g.count==d.count)return int(d.model);return -1;}
bool DoorDraw357(const fc3weapon::Geometry& g){return DoorModel363(g)>=0;}
void ObserveCarDoor357(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 if(!g_doors357Enabled||variant||call.kind||call.d)return;
 bool candidate=false;for(const auto& d:kDoorDraws363)if(call.b==d.start&&call.a==d.count){candidate=true;break;}if(!candidate)return;
 const int model=DoorModel363(NativeWeaponGeometry(context,call));if(model<0)return;
 const auto& asset=kDoorAssets363[model];const auto now=GetTickCount64();const auto c=fc3pad::Latest();const auto owner=fc3hands303::Read();
 const bool allowed=g_enabled&&g_headTracking&&g_inGameplay&&g_eyeViewsSet&&!g_flatViewActive&&!g_pistolMenu.open&&
  !fc3pad::GameplayInputBlocked()&&!fc3pad::ThrowableInputActive363()&&!fc3pad::NativeMenuOpen()&&!fc3pad::SelectorActive290()&&fc3aim::NativeFootCamera281()&&
  c.active&&!c.menu&&!c.a&&!c.b&&!c.x&&!c.y&&c.leftTrigger<.2f&&c.rightTrigger<.2f&&
  std::fabs(c.leftX)<.15f&&std::fabs(c.leftY)<.15f&&now>=g_physicalAt352&&now-g_physicalAt352<=100&&
  !g_reload.held&&!g_reload.pending&&!g_manualReload.pending&&!g_manualReload.cycle&&!g_reloadAlignment;
 Mat4 toView={},fromView={};if(!DrawSpace(context,nullptr,toView,fromView,3,true))return;
 const Mat4 bodyToDoor=Multiply(fromView,CarHeadView352());
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);float rows[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,rows,sizeof(rows));SafeRelease(cb);
 bool fractions=false;const int off=WeaponWorldOffset(context,&fractions);if(bytes<UINT((off+(fractions?15:12))*4))return;
 Mat4 world={};world.m[15]=1;for(int r=0;r<3;++r)for(int k=0;k<4;++k)world.m[r*4+k]=rows[off+r*4+k]+(k==3&&fractions?rows[off+12+r]:0);
 const bool same=model==g_doorModel363&&now>=g_doorAt357&&now-g_doorAt357<150&&Length(Sub({world.m[3],world.m[7],world.m[11]},{g_doorWorld357.m[3],g_doorWorld357.m[7],g_doorWorld357.m[11]}))<.04;
 if(!same&&(g_door357[0].held||g_door357[1].held)){if(now>=g_doorAt357&&now-g_doorAt357<150)return;g_door357[0].Cancel();g_door357[1].Cancel();fc3pad::SetDoorInput357(false,false);}
 static unsigned seen[2]={};if(seen[model]++<2)Log("[DOOR363 SEEN] model=%s exact native driver door; allowed=%d",asset.name,allowed);
 bool touched=false,held=false,pulse=false,show=false;
 for(int h=0;h<2;++h){
  if(g_doorFrame357[h]==g_frame)continue;
  Mat4 physical={};for(int i=0;i<16;++i)physical.m[i]=g_physicalBody303[h][i];
  const V3 palm=Apply(Multiply(bodyToDoor,physical),{0,-.015,.015});const double distance=Length(Sub(palm,asset.handle));if(distance>.5&&!g_door357[h].held)continue;
  touched=true;g_doorFrame357[h]=g_frame;
  fc3door357::Input input;input.now=now;input.epoch=g_armPoseEpoch;input.allowed=allowed&&(!owner.enabled||owner.empty||owner.hand!=h);
  input.tracked=g_physicalValid303[h];input.grip=c.weaponRoles303?(h?c.physicalLeftGrip303:c.physicalRightGrip303):(h?c.leftGrip:c.rightGrip);
  input.hand={palm.x,palm.y,palm.z};input.handle={asset.handle.x,asset.handle.y,asset.handle.z};input.anchor={world.m[3],world.m[7],world.m[11]};
  const auto result=g_door357[h].Step(input);held|=result.held;pulse|=result.enter;show|=input.allowed&&input.tracked;
  if(result.grabbed||result.enter){fc3xr::InteractionPulse285(h);Log("[DOOR363] model=%s hand=%d grip=%d pull=%d distance=%.3f native X entry request",asset.name,h,result.grabbed,result.enter,distance);}
 }
 if(touched){g_doorModel363=model;g_doorWorld357=world;g_doorAt357=now;fc3pad::SetDoorInput357(held,pulse);
  if(show){Mat4 inverse={};if(Invert(bodyToDoor,inverse)){
   Mat4 plane={};plane.m[2]=-1;plane.m[4]=-1;plane.m[9]=1;plane.m[15]=1;plane.m[3]=asset.frame.x;plane.m[7]=asset.frame.y;plane.m[11]=asset.frame.z;
   const Mat4 frame=Multiply(inverse,plane);plane.m[3]=asset.handle.x-.008;plane.m[7]=asset.handle.y;plane.m[11]=asset.handle.z;const Mat4 handle=Multiply(inverse,plane);
   float frameBody[16],handleBody[16];for(int n=0;n<16;++n){frameBody[n]=float(frame.m[n]);handleBody[n]=float(handle.m[n]);}
   fc3xr::SetDoorHighlight363(frameBody,handleBody,float(asset.width),float(asset.height),held);
  }}
 }
}
