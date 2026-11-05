// Native rifle/LMG and non-Vector SMG casing draws. Keep the casing's native physics and transport
// it by the gun correction captured at birth, independently of later hand poses.
struct RifleCasing235 {
 V3 native={},velocity={};Mat4 move={};ULONGLONG born=0,seen=0;
 LONG frame=-100000;UINT epoch=0;uintptr_t mesh=0;int profile=-1;
};
RifleCasing235 g_rifleCasings235[128];
ID3D11Buffer* g_rifleCasingCb235=nullptr;UINT g_rifleCasingBytes235=0;
void ReleaseRifleCasings235(){SafeRelease(g_rifleCasingCb235);g_rifleCasingBytes235=0;for(auto& c:g_rifleCasings235)c={};}
bool SmgCasingProfile237(int profile){return profile==39||profile==7||profile==28||profile==9;}
bool SmallCasingProfile250(int profile){return profile==20||SmgCasingProfile237(profile);}
bool ShotgunCasingProfile250(int profile){return profile>=3&&profile<=6;}
// Native bolt-action ejection can trail firing while its bolt animation runs.
ULONGLONG CasingBirthWindow238(int profile){return profile==2||profile==23||ShotgunCasingProfile250(profile)?1500:250;}
bool RifleCasingSocket235(int profile,V3& socket){
 // Original xbg FX_CASING expressed relative to the actual rendered FRAME.
 // AK/STG/PKM/Skorpion frames and MP5 ANCHOR have nonzero rest translations; subtract those.
 switch(profile){
 case 20:socket={.0150377499,.1011313014,.0959024809};return true; // Kimber FRAME rest subtracted (Shadow shares mesh)
 case 3:case 4:case 5:case 6:socket={.0247741293,.1364683136,.0978457276};return true; // M133/Bull FRAME rest subtracted
 case 35:socket={-.0236165002,.1992079169,.0812948495};return true;
 case 2:socket={.0142284902,.2782429680,.0744743516};return true; // R700 FRAME rest translation subtracted
 case 23:socket={.0096881604,.1689283699,.1082159281};return true;
 case 39:socket={.0025365199,.1462947577,.0521659479};return true; // VZ61 FRAME
 case 7:socket={.0246577300,.1162361130,.1002827063};return true;
 case 28:socket={.0279409103,.0706275329,.0269520432};return true; // skinned ANCHOR
 case 9:socket={-.0164498501,.1680046618,.0611924604};return true;
 case 8:socket={.0309715209,.2902656570,.1309201075};return true;
 case 41:socket={.0282527898,.2044577636,.0329001397};return true;
 case 14:socket={.0163421202,-.1368823946,.0681950971};return true;
 case 31:socket={.0273246896,.1301285475,.0949196592};return true;
 case 25:socket={-.0144518996,.1807743162,.0634579733};return true;
 case 36:socket={.0188630298,.0775939226,.0502337590};return true;
 case 22:socket={.0329419598,-.2659854889,.0736883730};return true;
 case 43:socket={-.0244284807,.5416072607,.1087546274};return true;
 case 17:socket={.0300856102,.1581005007,.1179508567};return true;
 default:return false;
 }
}
bool RifleCasingMove235(const V3& position,uintptr_t mesh,int profile,const V3& socket,const Mat4& shotMove,bool spawn,ULONGLONG now,Mat4& move,bool* created=nullptr){
 if(created)*created=false;
 for(double n:{position.x,position.y,position.z})if(!std::isfinite(n))return false;
 RifleCasing235* match=nullptr;double nearest=.18;
 for(auto& c:g_rifleCasings235){
  if(!c.born||c.epoch!=g_armPoseEpoch||c.mesh!=mesh||now<c.seen||now-c.born>=8000)continue;
  const double distance=Length(Sub(position,c.native));
  // Repeated eye/material passes may reuse a casing; a different casing in the
  // same frame cannot steal its birth correction during automatic fire.
  if(c.frame==g_frame){if(distance<1e-5){move=c.move;return true;}continue;}
  const double dt=double(now-c.seen)*.001;
  const V3 predicted=Add(c.native,Scale(c.velocity,(std::min)(dt,.05)));
  const double error=Length(Sub(position,predicted));
  if(error<nearest&&distance<.5){match=&c;nearest=error;}
 }
 if(!match){
  V3 local={};if(!spawn||!RifleCasingSocket235(profile,local)||Length(Sub(position,socket))>.18)return false;
  for(auto& c:g_rifleCasings235)if(!c.born||c.epoch!=g_armPoseEpoch||now<c.born||now-c.born>=8000){match=&c;break;}
  if(!match)return false;*match={};match->born=now;match->move=shotMove;match->epoch=g_armPoseEpoch;match->mesh=mesh;match->profile=profile;if(created)*created=true;
 }else if(now>match->seen){
  const V3 velocity=Scale(Sub(position,match->native),1000.0/double(now-match->seen));
  match->velocity=Length(velocity)<20?velocity:V3{};
 }
 match->native=position;match->seen=now;match->frame=g_frame;move=match->move;return true;
}
ID3D11Buffer* BuildRifleCasingWorld235(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 if(variant!=0||call.kind||call.d||context!=g_ctx||!MotionInteractionEnabled())return nullptr;
 const auto geometry=NativeWeaponGeometry(context,call);
 const bool shell=geometry.vertexBytes==4192&&geometry.indexBytes==1008&&geometry.stride==32&&geometry.format==57&&
  (((call.a==360||call.a==504)&&call.b==0&&geometry.offset==0)||(call.a==144&&call.b==360&&(geometry.offset==0||geometry.offset==2816)));
 const bool link=!shell&&call.a==708,pistolCalibre=!shell&&call.a==360;
 if(!shell&&(call.b||geometry.stride!=32||geometry.offset||geometry.format!=57||
  (pistolCalibre?(geometry.vertexBytes!=3072||geometry.indexBytes!=720):link?(geometry.vertexBytes!=6784||geometry.indexBytes!=1416):(call.a!=504||geometry.vertexBytes!=3968||geometry.indexBytes!=1008))))return nullptr;
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);const uintptr_t mesh=reinterpret_cast<uintptr_t>(vb);SafeRelease(vb);
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);D3D11_BUFFER_DESC desc={};if(cb)cb->GetDesc(&desc);
 float data[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,data,sizeof(data));SafeRelease(cb);const int worldOffset=WeaponWorldOffset(context);
 if(bytes!=desc.ByteWidth||bytes<UINT((worldOffset+15)*4))return nullptr;
 const Mat4 native=HullWorld(data+worldOffset);const V3 position={native.m[3],native.m[7],native.m[11]};
 MuzzleFlashPoll111();const auto& pose=g_muzzleFlashPose111;const auto& shot=g_muzzleFlash111;const ULONGLONG now=MuzzleFlashMs111();
 V3 local={},socket={};const bool rifle=RifleCasingSocket235(g_weaponMechanicProfile,local);
 const bool spawn=rifle&&(shell==ShotgunCasingProfile250(g_weaponMechanicProfile))&&(pistolCalibre==SmallCasingProfile250(g_weaponMechanicProfile))&&(!link||g_weaponMechanicProfile==36||g_weaponMechanicProfile==22||g_weaponMechanicProfile==43)&&pose.valid&&pose.profile==UINT(g_weaponMechanicProfile)&&pose.epoch==g_armPoseEpoch&&g_frame>=pose.frame&&g_frame-pose.frame<=2&&
  shot.localValid&&shot.epoch==g_armPoseEpoch&&shot.profile==pose.profile&&now>=shot.shotMs&&now-shot.shotMs<CasingBirthWindow238(g_weaponMechanicProfile);
 if(spawn)socket=Apply(pose.native,local);
 Mat4 move={};bool created=false;
 if(!RifleCasingMove235(position,mesh,g_weaponMechanicProfile,socket,pose.move,spawn,now,move,&created))return nullptr;
 if(created){static UINT samples[44]={};const int profile=g_weaponMechanicProfile;if(profile>=0&&profile<44&&samples[profile]++<3){const V3 placed=Apply(move,position);Log("[CASING235] profile%d native casing acquired at %.3f m from FX_CASING; %.3f/%.3f/%.3f -> %.3f/%.3f/%.3f, frozen birth correction",profile,Length(Sub(position,socket)),position.x,position.y,position.z,placed.x,placed.y,placed.z);}}
 WritePistolWorld(data+worldOffset,Multiply(move,native));
 if(g_rifleCasingBytes235!=bytes){SafeRelease(g_rifleCasingCb235);g_rifleCasingBytes235=0;D3D11_BUFFER_DESC out={};out.ByteWidth=bytes;out.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(FAILED(g_oCreateBuffer(g_dev,&out,nullptr,&g_rifleCasingCb235)))return nullptr;g_rifleCasingBytes235=bytes;}
 const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_rifleCasingCb235,0,nullptr,data,0,0);g_own=own;return g_rifleCasingCb235;
}
