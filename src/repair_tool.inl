// Repair tool keeps the game's RT repair action and generic two-hand support.
// The native torch mesh was captured as VB 213320 / IB 34200, stride 40.
// Captured DXBC proves particle v0.xyz is already in WORLD space; their
// cb0 _WorldMatrix is unused. Move nearby vertices in a separate CPU-shadowed
// VB, never the source VB, lighting globals, viewport or surrounding particles.
struct RepairToolState { LONG seen=-100000,rootFrame=-100000;Mat4 nativeRoot={};LONG moved=0;ULONGLONG nextProbe=0;UINT probes=0,particlePackets=0,nearLines=0,promotedLines=0;LONG particleFrame=-1; };
RepairToolState g_repairTool;
#include "repair_grip247.inl"
ID3D11Buffer* g_repairEffectCb=nullptr;
UINT g_repairEffectCbBytes=0;
bool RepairToolModel(){return g_frame-g_repairTool.seen<=2&&g_inGameplay;}
const GUID kRepairVertexFrameGuid={0x70165232,0x9a15,0x4b24,{0xa1,0x70,0x21,0x45,0x65,0x07,0x01,0x10}};
ID3D11Buffer* g_repairEffectVb=nullptr;UINT g_repairEffectVbBytes=0;
// 192 KiB: PID82728's only blended pooled particle draws while RT was held used a 147456-byte dynamic VB (VS 8CBC4FD2A59FD8A3), over the old 128 KiB cap.
bool RepairKeepVertexShadow(const D3D11_BUFFER_DESC& d){return RepairToolModel()&&d.Usage==D3D11_USAGE_DYNAMIC&&(d.BindFlags&(D3D11_BIND_VERTEX_BUFFER|D3D11_BIND_INDEX_BUFFER))&&d.ByteWidth>=12&&d.ByteWidth<=192*1024;}
void RepairRememberVertexCopy(ID3D11Resource* resource){D3D11_BUFFER_DESC d={};static_cast<ID3D11Buffer*>(resource)->GetDesc(&d);if(d.BindFlags&D3D11_BIND_VERTEX_BUFFER){const LONG frame=g_frame;resource->SetPrivateData(kRepairVertexFrameGuid,sizeof(frame),&frame);}}
bool RepairRigidMatrix(const Mat4& m){
 for(int r=0;r<3;++r){double norm=0;for(int c=0;c<3;++c){if(!std::isfinite(m.m[r*4+c]))return false;norm+=m.m[r*4+c]*m.m[r*4+c];}if(norm<.75||norm>1.25||!std::isfinite(m.m[r*4+3]))return false;}
 for(int a=0;a<3;++a)for(int b=a+1;b<3;++b){double dot=0;for(int c=0;c<3;++c)dot+=m.m[a*4+c]*m.m[b*4+c];if(std::fabs(dot)>.15)return false;}return true;
}
bool RepairParticleShader(unsigned long long h){return h==0x42F3089A89C80C9Cull||h==0xDB23497C92F47D21ull||h==0x8D2F62CE1FDB1DC2ull||h==0xB9C22C7670E71A08ull;}
bool RepairWorldParticleShader(unsigned long long h){return h==0x42F3089A89C80C9Cull||h==0xDB23497C92F47D21ull||h==0x8D2F62CE1FDB1DC2ull;}
V3 RepairNativeTip(const Mat4& root){return Apply(root,V3{0,.3634923994541168,-.08794253319501877});} // welding_torch.xbg FX_FIRE node, exact authored nozzle
Mat4 RepairRootFromPalette(const float* world,const float* bones){return Multiply(HullWorld(world),BoneMatrix(bones,1));} // metal draw palette[0]=PRESSURE_KNOB, [1]=ANCHOR (bind identity), [2]=WIRES1
bool RepairParticleReferences(const DrawCall& call,const unsigned char* data,UINT bytes,DXGI_FORMAT format,UINT offset,UINT vertices,std::vector<UINT>& out){
 out.clear();if(!call.a||call.a>16384||!vertices)return false;std::vector<unsigned char> seen(vertices,0);const bool indexed=call.kind==0||call.kind==2;const UINT first=call.kind>=2?call.c:call.b;const UINT width=format==DXGI_FORMAT_R16_UINT?2:format==DXGI_FORMAT_R32_UINT?4:0;
 if(indexed&&(!data||!width||static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(first+static_cast<unsigned long long>(call.a))*width>bytes))return false;
 for(UINT i=0;i<call.a;++i){long long vertex=first+i;if(indexed){UINT n=0;memcpy(&n,data+offset+(first+i)*width,width);vertex=static_cast<long long>(n)+call.d;}if(vertex<0||vertex>=vertices){out.clear();return false;}if(!seen[static_cast<UINT>(vertex)]){seen[static_cast<UINT>(vertex)]=1;out.push_back(static_cast<UINT>(vertex));}}return true;
}
// PID(13:07 build) census: every RT-only particle draw (DB23497C, F753F984, BAF9EAA1)
// was refused with "indices" and "ib 0": the shared particle index buffer is a
// static buffer filled before the tool was equipped, so it never had a CPU copy
// and no build since PID124060 could tell which vertices the flame uses.
// A static/default index buffer is read back from the GPU ONCE and cached as its
// CPU copy (static buffers do not change; UpdateSubresource uploads are mirrored
// by the hook). Dynamic index buffers keep using their Map/Unmap copy.
const GUID kRepairIndexTriedGuid={0x70165232,0x9a15,0x4b24,{0xa1,0x70,0x21,0x45,0x65,0x07,0x01,0x11}};
bool ReadBytes(ID3D11DeviceContext*,ID3D11Buffer*,std::vector<unsigned char>&);
bool RepairIndexCopy(ID3D11DeviceContext* context,ID3D11Buffer* ib){
 if(!ib)return false;D3D11_BUFFER_DESC d={};ib->GetDesc(&d);if(!(d.BindFlags&D3D11_BIND_INDEX_BUFFER)||!d.ByteWidth||d.ByteWidth>1024*1024)return false;
 UINT size=0;ib->GetPrivateData(kCbCopyGuid,&size,nullptr);if(size==d.ByteWidth)return true;
 if(d.Usage==D3D11_USAGE_DYNAMIC||d.Usage==D3D11_USAGE_STAGING)return false;
 UINT tried=0,tb=sizeof(tried);if(SUCCEEDED(ib->GetPrivateData(kRepairIndexTriedGuid,&tb,&tried))&&tried)return false;tried=1;ib->SetPrivateData(kRepairIndexTriedGuid,sizeof(tried),&tried);
 std::vector<unsigned char> bytes;if(!ReadBytes(context,ib,bytes)||bytes.size()!=d.ByteWidth)return false;
 const bool ok=SUCCEEDED(ib->SetPrivateData(kCbCopyGuid,d.ByteWidth,bytes.data()));
 static LONG lines=0;if(lines<8){++lines;Log("[REPAIR EFFECT] particle index buffer read back once (%u bytes, usage %d): %s",d.ByteWidth,static_cast<int>(d.Usage),ok?"cached as its CPU copy":"COPY FAILED");}
 return ok;
}
// UpdateSubresource on an index buffer: a whole-buffer upload replaces the cached copy;
// a partial one drops it (and re-allows one readback) so a stale copy is never used.
void RepairIndexUploaded(ID3D11Resource* resource,const void* source,bool partial){
 D3D11_BUFFER_DESC d={};static_cast<ID3D11Buffer*>(resource)->GetDesc(&d);if(!(d.BindFlags&D3D11_BIND_INDEX_BUFFER)||!d.ByteWidth||d.ByteWidth>1024*1024)return;
 if(!partial&&source){resource->SetPrivateData(kCbCopyGuid,d.ByteWidth,source);return;}
 resource->SetPrivateData(kCbCopyGuid,0,nullptr);const UINT tried=0;resource->SetPrivateData(kRepairIndexTriedGuid,sizeof(tried),&tried);
}
// The vertices this draw actually references (index copy for indexed draws; the start/count range otherwise).
bool RepairReferencedVertices(ID3D11DeviceContext* context,const DrawCall& call,UINT vbBytes,UINT stride,UINT offset,std::vector<UINT>& references,DXGI_FORMAT& indexFormat,UINT& indexOffset){
 ID3D11Buffer* ib=nullptr;indexFormat=DXGI_FORMAT_UNKNOWN;indexOffset=0;context->IAGetIndexBuffer(&ib,&indexFormat,&indexOffset);D3D11_BUFFER_DESC id={};if(ib)ib->GetDesc(&id);if(ib&&(call.kind==0||call.kind==2))RepairIndexCopy(context,ib);std::vector<unsigned char> indices(id.ByteWidth<=1024*1024?id.ByteWidth:0);const UINT indexBytes=ib&&!indices.empty()?ReadCopy(ib,indices.data(),static_cast<UINT>(indices.size())):0;SafeRelease(ib);
 return stride>=12&&RepairParticleReferences(call,indexBytes?indices.data():nullptr,indexBytes,indexFormat,indexOffset,offset<vbBytes?(vbBytes-offset)/stride:0,references);
}
// Referenced vertices within `radius` of the authored nozzle. Positions are the
// leading float3 (world space for the captured particle shaders); a local-space
// mesh never lands within a metre of a world point, so this is the world test too.
UINT RepairCountNearNozzle(const unsigned char* data,UINT bytes,UINT stride,UINT offset,const std::vector<UINT>& references,const Mat4& root,double radius){
 if(!data||stride<12||offset>=bytes||!RepairRigidMatrix(root))return 0;const V3 tip=RepairNativeTip(root);UINT nearCount=0;
 for(UINT v:references){const unsigned long long at=static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(v)*stride;if(at+12>bytes)continue;float p[3];memcpy(p,data+at,sizeof(p));const double x=p[0]-tip.x,y=p[1]-tip.y,z=p[2]-tip.z;if(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&x*x+y*y+z*z<=radius*radius)++nearCount;}return nearCount;
}
// nearest (optional): distance from the authored nozzle to the closest referenced vertex, moved or not.
UINT RepairMoveParticleVertices(unsigned char* data,UINT bytes,UINT stride,UINT offset,const Mat4& root,const Mat4& delta,const std::vector<UINT>* references=nullptr,double* nearest=nullptr){
 if(nearest)*nearest=1e18;if(!data||stride<12||stride>64||offset>=bytes||!RepairRigidMatrix(root))return 0;UINT moved=0;
 const UINT count=references?static_cast<UINT>(references->size()):(bytes-offset)/stride;const V3 tip=RepairNativeTip(root);
 for(UINT k=0;k<count;++k){const UINT vertex=references?(*references)[k]:k;const unsigned long long at=static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(vertex)*stride;if(at+12>bytes)continue;float p[3];memcpy(p,data+at,sizeof(p));const double x=p[0]-tip.x,y=p[1]-tip.y,z=p[2]-tip.z;if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))continue;const double d2=x*x+y*y+z*z;if(nearest&&d2<*nearest)*nearest=d2;if(d2>1.0)continue;const V3 out=Apply(delta,V3{p[0],p[1],p[2]});for(int i=0;i<3;++i)p[i]=static_cast<float>(i==0?out.x:i==1?out.y:out.z);memcpy(data+at,p,sizeof(p));++moved;}
 if(nearest)*nearest=*nearest<1e18?std::sqrt(*nearest):-1.0;return moved;
}
// ---- Repair draw census (PID82728 / PID25656 diagnosis) ----------------------
// PID82728: the twelve candidate probes were spent on the hose spline (VS 5AA4)
// and a water tile; the 160-signature shot dumps end before the particle pass.
// PID25656: a census keyed on VB size filled all 48 slots with that same spline
// (one static VB per rope/wire segment, drawn with and without RT) before any
// particle draw was seen. Now keyed on VS/PS/stride/kind/variant/usage with a
// VB size range, and every entry counts REFERENCED vertices within 0.5 m of the
// authored nozzle when its VB has a CPU copy: the flame is the entry whose near
// count is non-zero only while RT is held. Read-only; bytecode saved as
// fc3_repair_vs|ps_<hash>.dxbc for fxc /dumpbin.
enum RepairReason:UINT{kRepairVariant=1,kRepairTrigger=2,kRepairShader=4,kRepairBlend=8,kRepairRoot=16,kRepairVbShadow=32,kRepairVbStale=64,kRepairVbRead=128,kRepairIndices=256,kRepairCamera=512,kRepairView=1024,kRepairEmitterFar=2048,kRepairNoneNear=4096,kRepairBuffer=8192,kRepairPromoted=16384};
struct RepairCensusEntry{unsigned long long vs=0,ps=0;UINT stride=0,kind=0,src=0,dst=0,topology=0,vbMin=0,vbMax=0;bool dynamic=false;int variant=0;float xScale=0;LONG first=0,last=0;UINT withTrigger=0,withoutTrigger=0,minCount=0,maxCount=0,reasons=0,vbCopy=0,ibCopy=0,cb0=0,nearMax=0,nearDrawsRt=0,nearDrawsIdle=0,loggedDraws=0,loggedReasons=0,loggedNear=0;bool dumped=false;};
struct RepairCensus{RepairCensusEntry e[48];int n=0;ULONGLONG nextSummary=0;UINT lines=0,dumps=0;LONG frame=-1;UINT drawsThisFrame=0;bool overflow=false;}g_repairCensus;
const char* RepairReasonText(UINT reasons,char* text,size_t size){
 static const struct{UINT bit;const char* name;}kNames[]={{kRepairPromoted,"PROMOTED-near-nozzle"},{kRepairShader,"not-whitelisted-VS"},{kRepairVariant,"variant"},{kRepairTrigger,"RT-released"},{kRepairBlend,"blend-off"},{kRepairRoot,"root/motion/hand"},{kRepairVbShadow,"vb-not-shadowed"},{kRepairVbStale,"vb-upload-stale"},{kRepairVbRead,"vb-copy-short"},{kRepairIndices,"indices"},{kRepairCamera,"camera"},{kRepairView,"view/hand-move"},{kRepairEmitterFar,"emitter-far"},{kRepairNoneNear,"no-vertex-within-1m-of-nozzle"},{kRepairBuffer,"buffer"}};
 text[0]=0;if(!reasons){strncpy_s(text,size,"none",_TRUNCATE);return text;}for(const auto& k:kNames)if(reasons&k.bit){if(text[0])strncat_s(text,size,",",_TRUNCATE);strncat_s(text,size,k.name,_TRUNCATE);}return text;
}
void RepairNoteReason(unsigned long long vs,UINT reason){auto& c=g_repairCensus;for(int i=0;i<c.n;++i)if(c.e[i].vs==vs)c.e[i].reasons|=reason;}
void RepairDumpShader(ID3D11DeviceChild* shader,bool pixel,unsigned long long hash){
 if(!shader)return;UINT size=0;shader->GetPrivateData(kEjectionBytecodeGuid,&size,nullptr);if(!size||size>65536)return;
 std::vector<unsigned char> code(size);if(FAILED(shader->GetPrivateData(kEjectionBytecodeGuid,&size,code.data())))return;
 wchar_t name[80]={};_snwprintf_s(name,80,_TRUNCATE,L"fc3_repair_%s_%016llX.dxbc",pixel?L"ps":L"vs",hash);WriteBlob(name,code);
}
UINT RepairCopyBytes(ID3D11DeviceChild* resource){if(!resource)return 0;UINT size=0;resource->GetPrivateData(kCbCopyGuid,&size,nullptr);return size;}
void RepairEffectCensus(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 if(!RepairToolModel())return;auto& c=g_repairCensus;if(c.frame!=g_frame){c.frame=g_frame;c.drawsThisFrame=0;}if(++c.drawsThisFrame>6000)return;
 ID3D11BlendState* blend=nullptr;float factor[4];UINT mask=0;context->OMGetBlendState(&blend,factor,&mask);D3D11_BLEND_DESC bd={};if(blend)blend->GetDesc(&bd);SafeRelease(blend);if(!bd.RenderTarget[0].BlendEnable)return;
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC vd={};if(vb)vb->GetDesc(&vd);const UINT vbCopy=RepairCopyBytes(vb);
 ID3D11Buffer* ib=nullptr;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;UINT indexOffset=0;context->IAGetIndexBuffer(&ib,&format,&indexOffset);const UINT ibCopy=RepairCopyBytes(ib);SafeRelease(ib);
 ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;context->VSGetShader(&vs,nullptr,nullptr);context->PSGetShader(&ps,nullptr,nullptr);const auto vh=ShaderHash(vs),ph=ShaderHash(ps);
 ID3D11Buffer* cb[2]={};context->VSGetConstantBuffers(0,2,cb);const UINT cb0=RepairCopyBytes(cb[0]);CameraSummary camera={};UINT size=sizeof(camera);const float xScale=cb[1]&&SUCCEEDED(cb[1]->GetPrivateData(kCameraSummaryGuid,&size,&camera))&&size==sizeof(camera)?(camera.mainView?camera.xScale:-1.f):0.f;SafeRelease(cb[0]);SafeRelease(cb[1]);
 const bool trigger=fc3pad::Latest().rightTrigger>=.2f,dynamic=vd.Usage==D3D11_USAGE_DYNAMIC;
 RepairCensusEntry* e=nullptr;for(int i=0;i<c.n&&!e;++i){auto& k=c.e[i];if(k.vs==vh&&k.ps==ph&&k.stride==stride&&k.kind==static_cast<UINT>(call.kind)&&k.variant==variant&&k.dynamic==dynamic)e=&k;}
 if(!e){if(c.n>=48){c.overflow=true;SafeRelease(vb);SafeRelease(vs);SafeRelease(ps);return;}e=&c.e[c.n++];*e=RepairCensusEntry{};e->vs=vh;e->ps=ph;e->stride=stride;e->kind=static_cast<UINT>(call.kind);e->src=bd.RenderTarget[0].SrcBlend;e->dst=bd.RenderTarget[0].DestBlend;e->dynamic=dynamic;e->variant=variant;e->xScale=xScale;e->first=g_frame;e->minCount=e->maxCount=call.a;e->vbMin=e->vbMax=vd.ByteWidth;D3D11_PRIMITIVE_TOPOLOGY topology=D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;context->IAGetPrimitiveTopology(&topology);e->topology=static_cast<UINT>(topology);}
 e->last=g_frame;if(trigger)++e->withTrigger;else ++e->withoutTrigger;e->minCount=(std::min)(e->minCount,call.a);e->maxCount=(std::max)(e->maxCount,call.a);e->vbMin=(std::min)(e->vbMin,vd.ByteWidth);e->vbMax=(std::max)(e->vbMax,vd.ByteWidth);e->vbCopy=vbCopy;e->ibCopy=ibCopy;e->cb0=cb0;
 if(vb&&vbCopy==vd.ByteWidth&&stride>=12&&stride<=64&&g_frame-g_repairTool.rootFrame<=1&&call.a<=16384){
  std::vector<unsigned char> vertices(vd.ByteWidth);std::vector<UINT> references;DXGI_FORMAT indexFormat;UINT io=0;
  if(ReadCopy(vb,vertices.data(),vd.ByteWidth)==vd.ByteWidth&&RepairReferencedVertices(context,call,vd.ByteWidth,stride,offset,references,indexFormat,io)){const UINT nearCount=RepairCountNearNozzle(vertices.data(),vd.ByteWidth,stride,offset,references,g_repairTool.nativeRoot,.5);e->nearMax=(std::max)(e->nearMax,nearCount);if(nearCount){if(trigger)++e->nearDrawsRt;else ++e->nearDrawsIdle;}}
 }
 if(!e->dumped&&c.dumps<32){e->dumped=true;++c.dumps;RepairDumpShader(vs,false,vh);RepairDumpShader(ps,true,ph);}
 SafeRelease(vb);SafeRelease(vs);SafeRelease(ps);
}
void RepairCensusTick(){ // once per frame from the tool's own draw: one line per changed entry every 4 s, 160 lines in all
 auto& c=g_repairCensus;const ULONGLONG now=GetTickCount64();if(now<c.nextSummary||c.lines>=160)return;c.nextSummary=now+4000;
 auto changed=[](const RepairCensusEntry& e){return (e.withTrigger||e.nearMax)&&(e.withTrigger+e.withoutTrigger!=e.loggedDraws||e.reasons!=e.loggedReasons||e.nearMax!=e.loggedNear);};
 int pending=0;for(int i=0;i<c.n;++i)if(changed(c.e[i]))++pending;
 if(!pending)return;++c.lines;Log("[REPAIR CENSUS] frame %ld: %d blended draw signature(s) while the tool is equipped%s; entries below changed since the last summary (near = referenced vertices within 0.5 m of the nozzle)",static_cast<long>(g_frame),c.n,c.overflow?" (table full)":"");
 for(int i=0;i<c.n&&c.lines<160;++i){auto& e=c.e[i];if(!changed(e))continue;e.loggedDraws=e.withTrigger+e.withoutTrigger;e.loggedReasons=e.reasons;e.loggedNear=e.nearMax;++c.lines;char reasons[256];
  Log("[REPAIR CENSUS]  vs %016llX ps %016llX | vb %u-%u %s stride %u | kind %u count %u-%u topo %u blend %u/%u variant %d camera x%.3f | frames %ld-%ld | draws with RT %u, without RT %u | near max %u (draws with RT %u, without %u) | CPU copies: vb %u ib %u cb0 %u | refused: %s",e.vs,e.ps,e.vbMin,e.vbMax,e.dynamic?"dynamic":"static",e.stride,e.kind,e.minCount,e.maxCount,e.topology,e.src,e.dst,e.variant,e.xScale,static_cast<long>(e.first),static_cast<long>(e.last),e.withTrigger,e.withoutTrigger,e.nearMax,e.nearDrawsRt,e.nearDrawsIdle,e.vbCopy,e.ibCopy,e.cb0,RepairReasonText(e.reasons,reasons,sizeof(reasons)));}
}
bool BeginRepairToolParticleDraw(ID3D11DeviceContext* context,const DrawCall& call,int variant,ID3D11Buffer*& original,UINT& stride,UINT& offset){
 original=nullptr;if(!RepairToolModel())return false;
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto shader=ShaderHash(vs);SafeRelease(vs);const bool listed=RepairWorldParticleShader(shader);
 if(variant!=0){RepairNoteReason(shader,kRepairVariant);return false;}
 if(g_frame-g_repairTool.rootFrame>1||!MotionInteractionEnabled()||!g_eyeViewsSet||!g_handRightValid){RepairNoteReason(shader,kRepairRoot);return false;}
 // No RT condition: PID85832 showed the idle pilot flame (RT released) drawn with the
 // same particle shaders at the nozzle (42F3 near-nozzle in 17 idle draws, 8D2F in 3).
 // Proximity to the native nozzle alone decides which vertices follow the tip.
 ID3D11BlendState* blend=nullptr;float factor[4];UINT mask=0;context->OMGetBlendState(&blend,factor,&mask);D3D11_BLEND_DESC bd={};if(blend)blend->GetDesc(&bd);SafeRelease(blend);if(!bd.RenderTarget[0].BlendEnable){RepairNoteReason(shader,kRepairBlend);return false;}
 ID3D11Buffer* vb=nullptr;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC desc={};if(vb)vb->GetDesc(&desc);if(!vb||!RepairKeepVertexShadow(desc)||stride<12||stride>64){SafeRelease(vb);RepairNoteReason(shader,listed?kRepairVbShadow:kRepairShader);return false;}
 LONG copied=-100000;UINT frameBytes=sizeof(copied);if(FAILED(vb->GetPrivateData(kRepairVertexFrameGuid,&frameBytes,&copied))||g_frame-copied<0||g_frame-copied>1){SafeRelease(vb);RepairNoteReason(shader,listed?kRepairVbStale:kRepairShader);return false;}
 std::vector<unsigned char> vertices(desc.ByteWidth);if(ReadCopy(vb,vertices.data(),desc.ByteWidth)!=desc.ByteWidth){SafeRelease(vb);RepairNoteReason(shader,listed?kRepairVbRead:kRepairShader);return false;}
 std::vector<UINT> references;DXGI_FORMAT indexFormat=DXGI_FORMAT_UNKNOWN;UINT indexOffset=0;if(!RepairReferencedVertices(context,call,desc.ByteWidth,stride,offset,references,indexFormat,indexOffset)){SafeRelease(vb);RepairNoteReason(shader,listed?kRepairIndices:kRepairShader);return false;}
 // Not whitelisted: only a draw whose referenced vertices sit at the native nozzle
 // (world positions within 0.5 m of the authored FX_FIRE point while RT is held)
 // is treated as the flame. A local-space mesh cannot pass this test.
 if(!listed){const UINT nearCount=RepairCountNearNozzle(vertices.data(),desc.ByteWidth,stride,offset,references,g_repairTool.nativeRoot,.5);if(nearCount<4){RepairNoteReason(shader,kRepairShader);SafeRelease(vb);return false;}RepairNoteReason(shader,kRepairPromoted);if(g_repairTool.promotedLines<8){++g_repairTool.promotedLines;Log("[REPAIR EFFECT] census-promoted draw: VS %016llX, %u of %zu referenced vertices within 0.5 m of the nozzle (stride %u, kind %u count %u, VB %u bytes)",shader,nearCount,references.size(),stride,call.kind,call.a,desc.ByteWidth);}}
 ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);CameraSummary camera={};UINT size=sizeof(camera);const bool main=cameraBuffer&&SUCCEEDED(cameraBuffer->GetPrivateData(kCameraSummaryGuid,&size,&camera))&&size==sizeof(camera)&&camera.mainView;SafeRelease(cameraBuffer);if(!main){SafeRelease(vb);RepairNoteReason(shader,kRepairCamera);return false;}
 Mat4 view={};for(int r=0;r<3;++r){const float* a=r==0?camera.right:r==1?camera.up:camera.forward;for(int c=0;c<3;++c)view.m[r*4+c]=a[c];view.m[r*4+3]=-(a[0]*camera.position[0]+a[1]*camera.position[1]+a[2]*camera.position[2]);}view.m[15]=1;Mat4 delta={};if(!RepairTransport247(view,delta)){SafeRelease(vb);RepairNoteReason(shader,kRepairView);return false;}
 // The lit whitelisted particle shaders expose EmitterWorldPos at cb0 byte352.
 // Exclude other nearby effects even if their pooled VB includes vertices near Jason.
 if(listed&&shader!=0x42F3089A89C80C9Cull){float globals[256]={};ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);const UINT n=ReadCopy(cb,globals,sizeof(globals));SafeRelease(cb);const double dx=globals[88]-g_repairTool.nativeRoot.m[3],dy=globals[89]-g_repairTool.nativeRoot.m[7],dz=globals[90]-g_repairTool.nativeRoot.m[11];if(n<364||dx*dx+dy*dy+dz*dz>1.0){SafeRelease(vb);RepairNoteReason(shader,kRepairEmitterFar);return false;}}
 //247: the pilot effect follows the same fixed ANCHOR as the tool mesh.
 if(g_animation.enabled&&g_repairTool.particlePackets<96&&g_repairTool.particleFrame!=g_frame){g_repairTool.particleFrame=g_frame;++g_repairTool.particlePackets;AnimationRecord packet;AnimationStamp(packet);packet.kind=21;packet.shader=shader;packet.drawKind=call.kind;packet.count=call.a;packet.start=call.kind>=2?call.c:call.b;packet.base=call.d;packet.vertexBytes=desc.ByteWidth;packet.stride=stride;packet.offset=offset;packet.indexFormat=indexFormat;packet.stateWords[0]=static_cast<UINT>(references.size());packet.stateWords[1]=indexOffset;for(int i=0;i<12;++i)packet.finalPose[i]=static_cast<float>(g_repairTool.nativeRoot.m[i]);for(int i=0;i<16;++i)packet.finalPose[12+i]=static_cast<float>(delta.m[i]);const V3 tip=RepairNativeTip(g_repairTool.nativeRoot);packet.finalPose[28]=static_cast<float>(tip.x);packet.finalPose[29]=static_cast<float>(tip.y);packet.finalPose[30]=static_cast<float>(tip.z);for(UINT k=0;k<references.size()&&k<128;++k){memcpy(packet.raw+k*4,vertices.data()+offset+references[k]*stride,12);packet.raw[k*4+3]=static_cast<float>(references[k]);packet.rawBytes=(k+1)*16;}AnimationStore(packet);}
 double nearest=-1;const UINT moved=RepairMoveParticleVertices(vertices.data(),desc.ByteWidth,stride,offset,g_repairTool.nativeRoot,delta,&references,&nearest);
 if(!moved){SafeRelease(vb);RepairNoteReason(shader,kRepairNoneNear);if(g_repairTool.nearLines<8){++g_repairTool.nearLines;const V3 tip=RepairNativeTip(g_repairTool.nativeRoot);Log("[REPAIR EFFECT] whitelisted particle draw refused: no referenced vertex within 1 m of the nozzle (nearest %.2f m; VS %016llX, %zu referenced of %u, kind %u count %u; nozzle %.2f %.2f %.2f)",nearest,shader,references.size(),offset<desc.ByteWidth?(desc.ByteWidth-offset)/stride:0,call.kind,call.a,tip.x,tip.y,tip.z);}return false;}
 if(g_repairEffectVbBytes!=desc.ByteWidth){SafeRelease(g_repairEffectVb);g_repairEffectVbBytes=0;D3D11_BUFFER_DESC d={};d.ByteWidth=desc.ByteWidth;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;if(FAILED(g_oCreateBuffer(g_dev,&d,nullptr,&g_repairEffectVb))){SafeRelease(vb);RepairNoteReason(shader,kRepairBuffer);return false;}g_repairEffectVbBytes=d.ByteWidth;}
 const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_repairEffectVb,0,nullptr,vertices.data(),0,0);context->IASetVertexBuffers(0,1,&g_repairEffectVb,&stride,&offset);g_own=own;original=vb;if(++g_repairTool.moved<=6)Log("[REPAIR EFFECT] drawn flame vertices attached: VS %016llX%s, %u/%zu REFERENCED vertices, stride %u, kind %u start %u base %d; native FX_FIRE %.3f %.3f %.3f",shader,listed?"":" (census-promoted)",moved,references.size(),stride,call.kind,call.kind>=2?call.c:call.b,call.d,RepairNativeTip(g_repairTool.nativeRoot).x,RepairNativeTip(g_repairTool.nativeRoot).y,RepairNativeTip(g_repairTool.nativeRoot).z);return true;
}
void EndRepairToolParticleDraw(ID3D11DeviceContext* context,ID3D11Buffer*& original,UINT stride,UINT offset){if(!original)return;context->IASetVertexBuffers(0,1,&original,&stride,&offset);SafeRelease(original);}
bool RepairEffectCandidate(unsigned long long shader,UINT count,UINT stride,bool blend,const Mat4& world,const Mat4& root,LONG age,bool firing){
 if(!firing||age<0||age>1||!blend||!RepairParticleShader(shader)||!count||count>1024||stride<12||stride>64||!RepairRigidMatrix(world)||!RepairRigidMatrix(root))return false;
 const double dx=world.m[3]-root.m[3],dy=world.m[7]-root.m[7],dz=world.m[11]-root.m[11];return dx*dx+dy*dy+dz*dz<.55*.55;
}
Mat4 RepairEffectMovedWorld(const Mat4& world,const Mat4& cameraView,const Mat4& cameraInverse,const Mat4& handMove){return Multiply(cameraInverse,Multiply(handMove,Multiply(cameraView,world)));}
void ObserveRepairToolDraw(ID3D11DeviceContext* context,const DrawCall& call){
 const auto geometry=NativeWeaponGeometry(context,call);if(geometry.vertexBytes!=213320||geometry.indexBytes!=34200||geometry.stride!=40||geometry.offset!=0||geometry.format!=57)return;
 ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);CameraSummary camera={};UINT cameraBytes=sizeof(camera);const bool mainCamera=cameraBuffer&&SUCCEEDED(cameraBuffer->GetPrivateData(kCameraSummaryGuid,&cameraBytes,&camera))&&cameraBytes==sizeof(camera)&&camera.mainView;SafeRelease(cameraBuffer);if(!mainCamera)return; // shadow/world copies never acquire the equipped-tool lease
 g_repairTool.seen=g_frame;
 // Main metal cluster maps palette 0 to PRESSURE_KNOB, palette 1 to ANCHOR,
 // palette 2 to WIRES1. The animated knob must never own the flame's frame.
 if(call.b!=4824||call.a!=12276)return;
 RepairCensusTick();
 const int profile=fc3weapon::ProfileIndex(geometry);if(profile>=0){g_weaponMechanicProfile=profile;g_weaponMechanicFrame=g_frame;}
 float rows[88]={},bones[576]={};ID3D11Buffer* globals=nullptr,*palette=nullptr;context->VSGetConstantBuffers(0,1,&globals);const int slot=MuzzlePaletteSlot58(context);if(slot<0){SafeRelease(globals);return;}context->VSGetConstantBuffers(UINT(slot),1,&palette);
 const bool read=ReadCopy(globals,rows,sizeof(rows))>=48&&ReadCopy(palette,bones,sizeof(bones))==sizeof(bones);SafeRelease(globals);SafeRelease(palette);
 const int worldOffset=WeaponWorldOffset(context);if(!read||worldOffset+15>88)return;const Mat4 root=RepairRootFromPalette(rows+worldOffset,bones);if(!RepairRigidMatrix(root))return;g_repairTool.nativeRoot=root;g_repairTool.rootFrame=g_frame;
}
ID3D11Buffer* BuildRepairToolEffectWorld(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 if(variant!=0||!RepairToolModel()||!MotionInteractionEnabled()||!g_eyeViewsSet||!g_handRightValid)return nullptr; // idle pilot flame too: no RT condition
 ID3D11BlendState* blend=nullptr;float factor[4];UINT mask=0;context->OMGetBlendState(&blend,factor,&mask);D3D11_BLEND_DESC bd={};if(blend)blend->GetDesc(&bd);SafeRelease(blend);if(!bd.RenderTarget[0].BlendEnable)return nullptr;
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto shader=ShaderHash(vs);SafeRelease(vs);
 if(RepairWorldParticleShader(shader))return nullptr; // captured DXBC: cb0 world matrix is unused; vertex correction owns these
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC vd={};if(vb)vb->GetDesc(&vd);SafeRelease(vb);
 float rows[kSmallCbMax/4]={};ID3D11Buffer* globals=nullptr;context->VSGetConstantBuffers(0,1,&globals);D3D11_BUFFER_DESC cb={};if(globals)globals->GetDesc(&cb);const UINT bytes=ReadCopy(globals,rows,sizeof(rows));SafeRelease(globals);
 // Bounded diagnostics for unknown or pooled effects, including layout/CB
 // ownership. No GPU readback, synchronization or control mutation.
 const ULONGLONG now=GetTickCount64();if(g_repairTool.probes<12&&now>=g_repairTool.nextProbe){g_repairTool.nextProbe=now+500;++g_repairTool.probes;Log("[REPAIR EFFECT] candidate VS %016llX VB %u stride %u offset %u draw %u/%u cb0 %u rootAge %ld rigid %d",shader,vd.ByteWidth,stride,offset,call.b,call.a,bytes,g_frame-g_repairTool.rootFrame,bytes>=48&&RepairRigidMatrix(HullWorld(rows)));if(g_animation.enabled){AnimationRecord packet;AnimationStamp(packet);packet.kind=20;packet.drawKind=call.kind;packet.shader=shader;packet.vertexBytes=vd.ByteWidth;packet.stride=stride;packet.offset=offset;packet.start=call.b;packet.count=call.a;packet.rawBytes=bytes;memcpy(packet.raw,rows,(std::min)(bytes,static_cast<UINT>(sizeof(rows))));memcpy(packet.globals,rows,(std::min)(bytes,static_cast<UINT>(sizeof(packet.globals))));for(int i=0;i<12;++i)packet.finalPose[i]=static_cast<float>(g_repairTool.nativeRoot.m[i]);packet.stateWords[0]=g_repairTool.rootFrame;AnimationStore(packet);}}
 if(bytes<48||bytes!=cb.ByteWidth||!RepairEffectCandidate(shader,call.a,stride,true,HullWorld(rows),g_repairTool.nativeRoot,g_frame-g_repairTool.rootFrame,true))return nullptr;
 // Confirm this draw's actual main camera. Camera rows convert the same hand
 // delta as the torch mesh from view space into native world space.
 CameraSummary camera={};bool cameraFound=false;ID3D11Buffer* buffers[6]={};context->VSGetConstantBuffers(0,6,buffers);for(auto*& b:buffers){UINT size=sizeof(camera);CameraSummary sample={};if(b&&SUCCEEDED(b->GetPrivateData(kCameraSummaryGuid,&size,&sample))&&size==sizeof(sample)&&sample.mainView){camera=sample;cameraFound=true;}SafeRelease(b);}if(!cameraFound)return nullptr;
 Mat4 view={};for(int r=0;r<3;++r){const float* axis=r==0?camera.right:r==1?camera.up:camera.forward;for(int c=0;c<3;++c)view.m[r*4+c]=axis[c];view.m[r*4+3]=-(axis[0]*camera.position[0]+axis[1]*camera.position[1]+axis[2]*camera.position[2]);}view.m[15]=1;
 Mat4 delta={};if(!RepairTransport247(view,delta))return nullptr;const Mat4 moved=Multiply(delta,HullWorld(rows));for(int i=0;i<12;++i)rows[i]=static_cast<float>(moved.m[i]);rows[12]=rows[13]=rows[14]=0; // fractions were folded into moved translation by HullWorld
 if(g_repairEffectCbBytes!=bytes){SafeRelease(g_repairEffectCb);g_repairEffectCbBytes=0;D3D11_BUFFER_DESC d={};d.ByteWidth=bytes;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(FAILED(g_oCreateBuffer(g_dev,&d,nullptr,&g_repairEffectCb)))return nullptr;g_repairEffectCbBytes=bytes;}
 const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_repairEffectCb,0,nullptr,rows,0,0);g_own=own;if(++g_repairTool.moved==1)Log("[REPAIR EFFECT] local rigid torch emitter attached to displayed right-hand tool; native CB preserved");return g_repairEffectCb;
}
bool RepairToolNote(){if(!RepairToolModel())return false;fc3xr::SetTestNote(L"REPAIR TOOL\nRT: use tool. LEFT grip: support with both hands.\nCheck welding flame follows the tip when moving.");return true;}
void ReleaseRepairToolResources(){SafeRelease(g_repairEffectCb);g_repairEffectCbBytes=0;SafeRelease(g_repairEffectVb);g_repairEffectVbBytes=0;g_repairTool=RepairToolState{};g_repairCensus=RepairCensus{};}
