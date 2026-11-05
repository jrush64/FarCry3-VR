// Bounded CPU upload/binding trace. ANP1 layout stays unchanged; kinds40..48
// carry complete CB chunks and double-precision pose bundles. No GPU readback.
struct PistolTrace46 {
 bool pending=false,world=false,palette=false,haveHand[2]={};LONG frame=-1,sampled=-1;unsigned samples[2]={},draws=0;ULONGLONG at[2]={};Mat4 hand[2]={};
 ID3D11DeviceContext* context=nullptr;ID3D11Buffer* worldBuffer=nullptr,*paletteBuffer=nullptr;
 AnimationRecord record={};float worldData[2048]={},paletteData[576]={};UINT worldBytes=0,paletteBytes=0;
}g_pistolTrace46;
void Trace46Payload(AnimationRecord record,UINT kind,ID3D11Buffer* buffer,const void* data,UINT copied,UINT declared){
 record.kind=kind;record.weapon=reinterpret_cast<UINT>(buffer);record.explicitCount=declared;
 if(!copied){record.rawBytes=0;record.base=0;AnimationStore(record);return;}
 for(UINT off=0;off<copied;off+=sizeof(record.raw)){
  record.base=off;record.rawBytes=(std::min)(UINT(sizeof(record.raw)),copied-off);memset(record.raw,0,sizeof(record.raw));memcpy(record.raw,static_cast<const unsigned char*>(data)+off,record.rawBytes);AnimationStore(record);
 }
}
void Trace46Bound(AnimationRecord record,ID3D11DeviceContext* context,UINT slot,UINT kind){
 ID3D11Buffer* buffer=nullptr;context->VSGetConstantBuffers(slot,1,&buffer);D3D11_BUFFER_DESC desc={};if(buffer)buffer->GetDesc(&desc);
 float data[2048]={};const UINT copied=desc.ByteWidth<=sizeof(data)?ReadCopy(buffer,data,sizeof(data)):0;
 Trace46Payload(record,kind,buffer,data,copied,desc.ByteWidth);SafeRelease(buffer);
}
void Trace46Pose(AnimationRecord record,UINT phase){
 record.kind=48;record.event=phase;record.stateWords[0]=g_pistolReload.rootFrame;record.stateWords[1]=g_pistolReload.rootNativeFrame;
 record.stateWords[2]=g_pistolReload.rootEpoch;record.stateWords[3]=g_weaponMechanicProfile;record.stateWords[4]=g_pistolReload.parentValid;
 record.stateWords[5]=g_1911FrameFinal.valid;record.stateWords[6]=g_1911FrameFinal.frame;record.stateWords[7]=g_armPoseEpoch;
 Mat4 poses[]={g_pistolReload.rootNative,g_pistolReload.rootWorld,g_pistolReload.rootBody,g_pistolReload.controllerParent,ArmBodyToCamera(),g_guideShaderPlacement[1],g_1911FrameFinal.body};
 record.rawBytes=sizeof(poses);memcpy(record.raw,poses,sizeof(poses));AnimationStore(record);
}
void Begin1911Trace46(ID3D11DeviceContext* context,const DrawCall& call){
 auto& t=g_pistolTrace46;t.pending=false;
 if(!g_animation.enabled||g_weaponMechanicProfile!=20||g_pistolMenu.open||!g_inGameplay||call.kind!=0)return;
 const auto geometry=NativeWeaponGeometry(context,call);if(geometry.vertexBytes!=177904&&geometry.vertexBytes!=48520&&geometry.vertexBytes!=10560)return;
 const int mode=g_twoHand?1:0;const auto now=GetTickCount64();Mat4 hand={};for(int i=0;i<16;++i)hand.m[i]=g_handBody[0][i];
 if(t.frame!=g_frame){
  t.frame=g_frame;t.draws=0;double move=0;for(int i=0;i<12;++i)move=(std::max)(move,std::fabs(hand.m[i]-t.hand[mode].m[i]));
  if(t.samples[mode]<48&&(!t.haveHand[mode]||(now-t.at[mode]>=100&&move>.01))){t.sampled=g_frame;t.haveHand[mode]=true;t.hand[mode]=hand;t.at[mode]=now;++t.samples[mode];g_animation.until=(std::max)(g_animation.until,now+1500);}
 }
 if(t.sampled!=g_frame||t.draws++>=32)return;
 t.pending=true;t.context=context;t.world=t.palette=false;t.worldBuffer=t.paletteBuffer=nullptr;
 t.record=AnimationRecord{};AnimationStamp(t.record);auto& r=t.record;r.drawKind=call.kind;r.count=call.a;r.start=call.b;r.base=call.d;r.vertexBytes=geometry.vertexBytes;r.stride=geometry.stride;r.offset=geometry.offset;r.indexFormat=geometry.format;r.state=reinterpret_cast<UINT>(context);
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);r.shader=ShaderHash(vs);SafeRelease(vs);
 ID3D11PixelShader* ps=nullptr;context->PSGetShader(&ps,nullptr,nullptr);r.index=ShaderHash(ps);SafeRelease(ps); // trace kinds: native pixel shader identity
 CameraSummary camera={};if(PistolGuideCamera(context,camera)){memcpy(r.camera,camera.right,12);memcpy(r.camera+3,camera.up,12);memcpy(r.camera+6,camera.forward,12);memcpy(r.camera+9,camera.position,12);r.camera[12]=camera.xScale;r.camera[13]=camera.yScale;r.camera[14]=camera.zScale;r.camera[15]=camera.zOffset;}
 Trace46Bound(r,context,0,40);Trace46Bound(r,context,3,41);Trace46Bound(r,context,4,42);Trace46Bound(r,context,1,43);Trace46Pose(r,0);
}
void Trace1911World46(ID3D11DeviceContext* context,const float* data,UINT bytes,ID3D11Buffer* buffer){
 auto& t=g_pistolTrace46;if(!t.pending||t.context!=context||bytes>sizeof(t.worldData))return;t.world=true;t.worldBytes=bytes;t.worldBuffer=buffer;memcpy(t.worldData,data,bytes);
}
void Trace1911Palette46(ID3D11DeviceContext* context,const float* data,UINT bytes,ID3D11Buffer* buffer){
 auto& t=g_pistolTrace46;if(!t.pending||t.context!=context||bytes>sizeof(t.paletteData))return;t.palette=true;t.paletteBytes=bytes;t.paletteBuffer=buffer;memcpy(t.paletteData,data,bytes);
}
void End1911Trace46(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 auto& t=g_pistolTrace46;if(!t.pending||t.context!=context||t.record.count!=call.a||t.record.start!=call.b)return;
 auto r=t.record;r.side=0;r.event=variant;
 ID3D11Buffer* bound=nullptr;context->VSGetConstantBuffers(0,1,&bound);
 if(t.world&&bound==t.worldBuffer)Trace46Payload(r,44,bound,t.worldData,t.worldBytes,t.worldBytes);else Trace46Bound(r,context,0,44);SafeRelease(bound);
 context->VSGetConstantBuffers(3,1,&bound);
 if(t.palette&&bound==t.paletteBuffer)Trace46Payload(r,45,bound,t.paletteData,t.paletteBytes,t.paletteBytes);else Trace46Bound(r,context,3,45);SafeRelease(bound);
 Trace46Bound(r,context,4,46);Trace46Bound(r,context,dxbc::kEyeCbSlot,47);Trace46Pose(r,1);t.pending=false;
}
