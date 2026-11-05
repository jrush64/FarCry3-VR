// Native .45/9mm casing mesh, 96 vertices/120 triangles. Only a Vector's
// freshly spawned local casing near FX_CASING can acquire a frozen shot move.
// Subsequent native flight is transported by that birth move, never the hand.
struct VectorCasing231 {V3 native={};Mat4 move={};ULONGLONG seen=0;LONG frame=-100000;UINT epoch=0;};
VectorCasing231 g_vectorCasings231[96];ID3D11Buffer* g_vectorCasingCb231=nullptr;UINT g_vectorCasingBytes231=0;
void ReleaseVectorCasing231(){SafeRelease(g_vectorCasingCb231);g_vectorCasingBytes231=0;for(auto& c:g_vectorCasings231)c={};}
bool VectorCasingMove231(const V3& position,const V3& socket,const Mat4& shotMove,bool spawn,ULONGLONG now,Mat4& move){
 VectorCasing231* match=nullptr;double nearest=.18;
 for(auto& c:g_vectorCasings231)if(c.seen&&c.epoch==g_armPoseEpoch&&now>=c.seen&&now-c.seen<8000){
  const double d=Length(Sub(position,c.native));const double limit=c.frame==g_frame?1e-5:.18;
  if(d<limit&&d<nearest){match=&c;nearest=d;}
 }
 if(!match){
  if(!spawn||Length(Sub(position,socket))>.18)return false;
  for(auto& c:g_vectorCasings231)if(!c.seen||c.epoch!=g_armPoseEpoch||now-c.seen>=8000){match=&c;break;}
  if(!match)return false;match->move=shotMove;match->epoch=g_armPoseEpoch;
 }
 match->native=position;match->seen=now;match->frame=g_frame;move=match->move;return true;
}
ID3D11Buffer* BuildVectorCasingWorld231(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 if(variant!=0||call.kind||call.b||call.a!=360||g_weaponMechanicProfile!=21||!MotionInteractionEnabled())return nullptr;
 const auto d=NativeWeaponGeometry(context,call);if(d.vertexBytes!=3072||d.indexBytes!=720||d.stride!=32||d.offset||d.format!=57)return nullptr;
 const auto& pose=g_muzzleFlashPose111;if(!pose.valid||pose.profile!=21||pose.epoch!=g_armPoseEpoch||g_frame-pose.frame>2)return nullptr;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);D3D11_BUFFER_DESC desc={};if(cb)cb->GetDesc(&desc);
 float data[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,data,sizeof(data));SafeRelease(cb);const int offset=WeaponWorldOffset(context);
 if(bytes!=desc.ByteWidth||bytes<UINT((offset+15)*4)){static LONG missing=0;if(missing++<6)Log("[CASING231] Vector casing draw has no cached world rows: CB0 %u copied %u offset %d",desc.ByteWidth,bytes,offset);return nullptr;}
 const Mat4 native=HullWorld(data+offset);const V3 position={native.m[3],native.m[7],native.m[11]};
 MuzzleFlashPoll111();const ULONGLONG now=MuzzleFlashMs111();
 const bool spawn=g_muzzleFlash111.localValid&&g_muzzleFlash111.profile==21&&now>=g_muzzleFlash111.shotMs&&now-g_muzzleFlash111.shotMs<250;
 const V3 socket=Apply(pose.native,{.02282811,.18760136,.05210741});Mat4 move={};
 if(!VectorCasingMove231(position,socket,pose.move,spawn,now,move)){static LONG skipped=0;if(spawn&&skipped++<6)Log("[CASING231] exact casing mesh left native: %.3f m from Vector socket, source %.3f/%.3f/%.3f socket %.3f/%.3f/%.3f",Length(Sub(position,socket)),position.x,position.y,position.z,socket.x,socket.y,socket.z);return nullptr;}
 static LONG moved=0;if(moved++<6){const V3 final=Apply(move,position);Log("[CASING231] native Vector casing flight transported: %.3f/%.3f/%.3f -> %.3f/%.3f/%.3f",position.x,position.y,position.z,final.x,final.y,final.z);}
 WritePistolWorld(data+offset,Multiply(move,native));
 if(g_vectorCasingBytes231!=bytes){SafeRelease(g_vectorCasingCb231);g_vectorCasingBytes231=0;D3D11_BUFFER_DESC out={};out.ByteWidth=bytes;out.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(FAILED(g_oCreateBuffer(g_dev,&out,nullptr,&g_vectorCasingCb231)))return nullptr;g_vectorCasingBytes231=bytes;}
 const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_vectorCasingCb231,0,nullptr,data,0,0);g_own=own;return g_vectorCasingCb231;
}
