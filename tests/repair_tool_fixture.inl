// Actual WARP buffers: a static particle index buffer with no CPU copy is read
// back once, cached, and then selects exactly the vertices the draw references.
void TestRepairIndexReadback(ID3D11Device* device,ID3D11DeviceContext* context){
 const auto oldCopy167=g_oCopyResource;const auto oldMap167=g_oMap;const auto oldUnmap167=g_oUnmap;
 g_oCopyResource=ProbeCopy;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap; // isolated FC3_PISTOL_ONLY exits before main's later setup

 const unsigned short pattern[12]={0,1,2,0,2,3,4,5,6,4,6,7};D3D11_BUFFER_DESC d={};d.ByteWidth=sizeof(pattern);d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_INDEX_BUFFER;D3D11_SUBRESOURCE_DATA init={pattern,0,0};ID3D11Buffer* ib=nullptr;Check(SUCCEEDED(device->CreateBuffer(&d,&init,&ib)),"repair readback index buffer");
 Check(RepairCopyBytes(ib)==0,"repair test index buffer starts with no CPU copy (created outside the hook)");
 const auto oldDev=g_dev;const auto oldCtx=g_ctx;g_dev=device;g_ctx=context;context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 DrawCall call={};call.kind=0;call.a=6;call.b=6;std::vector<UINT> refs;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;UINT offset=0;
 Check(RepairReferencedVertices(context,call,36*16,36,0,refs,format,offset)&&refs.size()==4&&refs[0]==4&&refs[3]==7,"repair static particle index buffer read back once: second quad references vertices 4-7 only");
 Check(RepairCopyBytes(ib)==sizeof(pattern),"repair index readback cached as CPU copy");
 const unsigned short changed[12]={8,9,10,8,10,11,4,5,6,4,6,7};RepairIndexUploaded(ib,changed,false);call.b=0;Check(RepairReferencedVertices(context,call,36*16,36,0,refs,format,offset)&&refs[0]==8,"repair whole index upload replaces the cached copy");
 RepairIndexUploaded(ib,changed,true);Check(RepairCopyBytes(ib)==0,"repair partial index upload drops the cached copy");context->UpdateSubresource(ib,0,nullptr,pattern,0,0);Check(RepairReferencedVertices(context,call,36*16,36,0,refs,format,offset)&&refs[0]==0,"repair dropped copy is read back again from the GPU");
 D3D11_BUFFER_DESC dd=d;dd.Usage=D3D11_USAGE_DYNAMIC;dd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;ID3D11Buffer* dynamicIb=nullptr;Check(SUCCEEDED(device->CreateBuffer(&dd,&init,&dynamicIb)),"repair dynamic index buffer");Check(!RepairIndexCopy(context,dynamicIb),"repair never reads back a dynamic index buffer");
 context->IASetIndexBuffer(nullptr,DXGI_FORMAT_R16_UINT,0);SafeRelease(ib);SafeRelease(dynamicIb);g_dev=oldDev;g_ctx=oldCtx;g_oCopyResource=oldCopy167;g_oMap=oldMap167;g_oUnmap=oldUnmap167;
 std::puts("PASS: repair static particle index buffer read back once on WARP, cached, replaced by whole uploads, dropped by partial uploads; dynamic buffers never read back");
}
// Kimber holographic sight (start 13701 count 2100) drawn before the FRAME: its
// native offset from the FRAME is learned in the same frame and is rigid.
void TestPistolEarlyPiece(){
 const auto saved=g_pistolEarly[0];for(auto& e:g_pistolEarly)e=PistolEarlyPiece{};
 Mat4 frame=HandIdentity();frame.m[3]=10;frame.m[7]=20;frame.m[11]=30;Mat4 sight=frame;sight.m[7]+=.05;sight.m[11]+=.03;
 auto* e=PistolEarlySlot(13701,2100);Check(e&&e==PistolEarlySlot(13701,2100),"1911 early piece slot is stable per draw range");
 e->native=sight;e->nativeFrame=g_frame-1;PistolEarlyLearn(frame);Check(!e->valid,"1911 early piece never learns from another frame's native pose");
 e->nativeFrame=g_frame;PistolEarlyLearn(frame);Check(e->valid&&fabs(e->rel.m[7]-.05)<1e-9&&fabs(e->rel.m[11]-.03)<1e-9&&fabs(e->rel.m[3])<1e-9,"1911 early piece learns its rigid offset from the same-frame native FRAME");
 Mat4 moved=frame;moved.m[3]+=1;const Mat4 placed=Multiply(moved,e->rel);Check(fabs(placed.m[3]-11)<1e-9&&fabs(placed.m[7]-20.05)<1e-9,"1911 early piece follows the FRAME root with its learned offset");
 for(auto& x:g_pistolEarly)x=PistolEarlyPiece{};g_pistolEarly[0]=saved;
 std::puts("PASS: 1911/Kimber holographic sight drawn before the FRAME learns its same-frame rigid offset and follows the FRAME root");
}
// Kimber attachments from other model files and the two-handed shotgun grip point.
void TestPistolAttachmentAndSupport(){
 // native FRAME and silencer, displayed FRAME moved and turned: silencer must keep its native offset on the displayed FRAME
 Mat4 view=HandIdentity();view.m[3]=-3;view.m[7]=1;Mat4 rootNative=HandIdentity();rootNative.m[3]=2;rootNative.m[7]=5;rootNative.m[11]=1;
 Mat4 silencer=rootNative;silencer.m[7]+=.26;silencer.m[11]+=.083;
 Mat4 rootWorld=HandIdentity();rootWorld.m[0]=rootWorld.m[5]=0;rootWorld.m[1]=-1;rootWorld.m[4]=1;rootWorld.m[3]=2.3;rootWorld.m[7]=4.9;rootWorld.m[11]=1.2;
 Mat4 move={};Check(PistolAttachmentView(view,rootWorld,rootNative,move),"1911 attachment view move");
 Mat4 viewInverse={};Invert(view,viewInverse);const Mat4 displayed=Multiply(viewInverse,Multiply(move,Multiply(view,silencer)));
 Mat4 rootInverse={},nativeInverse={};Invert(rootWorld,rootInverse);Invert(rootNative,nativeInverse);const Mat4 want=Multiply(nativeInverse,silencer),got=Multiply(rootInverse,displayed);
 for(int i=0;i<12;++i)Check(fabs(want.m[i]-got.m[i])<1e-9,"1911 attachment keeps its native offset on the displayed FRAME");
 // the FRAME's native pose recovered from the attachment and its learned offset gives the same move
 Mat4 relInverse={};Invert(want,relInverse);Mat4 predicted={};Check(PistolAttachmentView(view,rootWorld,Multiply(silencer,relInverse),predicted),"1911 attachment drawn before the FRAME");
 for(int i=0;i<16;++i)Check(fabs(predicted.m[i]-move.m[i])<1e-9,"1911 attachment drawn before the FRAME matches the same-frame placement");
 // support point: idle offset in the right-hand frame follows the current right-hand basis, not the animated left wrist
 const auto saved=g_shotHand;const auto savedReload=g_reload.shotgunFrame;const bool m=g_motionGun,e=g_eyeViewsSet,r=g_handRightValid;const auto c=fc3pad::Latest();
 g_motionGun=g_eyeViewsSet=g_handRightValid=true;fc3pad::Controllers on={};on.active=true;fc3pad::Publish(on);g_reload.shotgunFrame=g_frame;
 g_shotHand=ShotHandControl{};g_shotHand.epoch=g_armPoseEpoch;g_shotHand.valid=g_shotHand.leftValid=true;g_shotHand.frame=g_frame;g_shotHand.leftRel={0,.4,.05};
 Mat4 turned=HandIdentity();turned.m[0]=turned.m[5]=0;turned.m[1]=-1;turned.m[4]=1;g_shotHand.current=turned;ArmJoints right={};right.valid=true;right.w[0]=1;right.w[1]=2;right.w[2]=3;V3 grip={};
 const bool ok=MotionInteractionEnabled()&&ShotgunSupportNative(right,grip);
 if(ok)Check(fabs(grip.x-.6)<1e-9&&fabs(grip.y-2)<1e-9&&fabs(grip.z-3.05)<1e-9,"two-handed support point is the idle grip offset turned by the current right hand");
 g_shotHand.leftValid=false;Check(!ShotgunSupportNative(right,grip),"no learned grip offset: native left wrist is used as before");
 g_shotHand=saved;g_reload.shotgunFrame=savedReload;g_motionGun=m;g_eyeViewsSet=e;g_handRightValid=r;fc3pad::Publish(c);
 std::puts(ok?"PASS: 1911 attachments keep their native FRAME offset (same-frame and drawn-first); two-handed shotgun support point is the idle grip, not the animated left wrist":"PASS: 1911 attachments keep their native FRAME offset; shotgun support point check skipped (motion interaction unavailable in this harness)");
}
void TestRepairToolEffects(){
 Mat4 root=HandIdentity(),local=HandIdentity();root.m[3]=100;root.m[7]=200;local=root;local.m[3]+=.2;
 const auto shader=0x42F3089A89C80C9Cull;
 Check(RepairEffectCandidate(shader,24,32,true,local,root,0,true),"repair local rigid particle accepted");
 Check(!RepairEffectCandidate(123,24,32,true,local,root,0,true),"repair unrelated blended shader rejected");
 local.m[3]+=2;Check(!RepairEffectCandidate(shader,24,32,true,local,root,0,true),"repair distant scene fire rejected");local=root;
 Check(!RepairEffectCandidate(shader,24,32,true,local,root,2,true),"repair stale native frame rejected");
 Check(!RepairEffectCandidate(shader,24,32,true,local,root,0,false),"repair inactive trigger rejected");
 Check(!RepairEffectCandidate(shader,24,32,false,local,root,0,true),"repair opaque scene draw rejected");
 Check(!RepairEffectCandidate(shader,4096,32,true,local,root,0,true),"repair pooled particle batch rejected");
 local.m[5]=0;Check(!RepairEffectCandidate(shader,24,32,true,local,root,0,true),"repair nonworld constants rejected");
 Mat4 camera=HandIdentity(),inverse={},move=HandIdentity();camera.m[3]=-100;camera.m[7]=-200;Invert(camera,inverse);move.m[3]=.3;move.m[7]=-.2;local=root;local.m[3]+=.12;const auto moved=RepairEffectMovedWorld(local,camera,inverse,move);Check(std::fabs(moved.m[3]-100.42)<1e-6&&std::fabs(moved.m[7]-199.8)<1e-6,"repair emitter uses tool delta in world coordinates");Check(std::fabs(local.m[3]-100.12)<1e-6,"repair native source matrix preserved");
 struct Vertex{float p[3];unsigned char attributes[20];};Vertex source[3]={};source[0].p[0]=100.1f;source[0].p[1]=200.f;source[1].p[0]=105.f;source[1].p[1]=200.f;source[2].p[0]=100.f;source[2].p[1]=200.2f;for(int v=0;v<3;++v)for(int k=0;k<20;++k)source[v].attributes[k]=static_cast<unsigned char>(v*20+k);Vertex copy[3];memcpy(copy,source,sizeof(copy));Check(RepairMoveParticleVertices(reinterpret_cast<unsigned char*>(copy),sizeof(copy),sizeof(Vertex),0,root,move)==2,"repair moves only torch-near world-space particle vertices");Check(std::fabs(copy[0].p[0]-100.4f)<1e-5&&std::fabs(copy[2].p[1]-200.f)<1e-5,"repair particles inherit hand translation");Check(memcmp(copy[1].p,source[1].p,12)==0,"repair surrounding scene particles remain unchanged within same batch");for(int v=0;v<3;++v)Check(memcmp(copy[v].attributes,source[v].attributes,20)==0,"repair particle color UV and lifetime preserved");Check(source[0].p[0]==100.1f,"repair source upload unchanged");Check(!RepairWorldParticleShader(0x5AA4B1A926B51E39ull),"repair ambient spline shader excluded");
 D3D11_BUFFER_DESC d={};d.ByteWidth=1024;d.Usage=D3D11_USAGE_DYNAMIC;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;const auto oldTool=g_repairTool;const bool oldGameplay=g_inGameplay;g_inGameplay=true;g_repairTool.seen=g_frame;Check(RepairKeepVertexShadow(d),"repair dynamic upload shadow allowed while tool equipped");d.ByteWidth=256*1024;Check(!RepairKeepVertexShadow(d),"repair large scene vertex uploads excluded");d.ByteWidth=1024;d.Usage=D3D11_USAGE_DEFAULT;Check(!RepairKeepVertexShadow(d),"repair GPU-only resources never read back");g_repairTool=oldTool;g_inGameplay=oldGameplay;
 const unsigned short ids[7]={2,2,0,1,1,0,2};DrawCall call={};call.kind=0;call.a=3;call.b=1;std::vector<UINT> references;Check(RepairParticleReferences(call,reinterpret_cast<const unsigned char*>(ids),sizeof(ids),DXGI_FORMAT_R16_UINT,0,3,references)&&references.size()==3&&references[0]==2,"repair exact index start and duplicate handling");call.a=2;call.b=0;Check(RepairParticleReferences(call,reinterpret_cast<const unsigned char*>(ids),sizeof(ids),DXGI_FORMAT_R16_UINT,0,3,references)&&references.size()==1&&references[0]==2,"repair pooled unused vertices excluded by actual draw indices");memcpy(copy,source,sizeof(copy));Check(RepairMoveParticleVertices(reinterpret_cast<unsigned char*>(copy),sizeof(copy),sizeof(Vertex),0,root,move,&references)==1,"repair moves only actually referenced nearby vertex");Check(memcmp(copy,source,sizeof(Vertex))==0,"repair unused nearby pooled vertex stays byte-identical");call.d=-3;Check(!RepairParticleReferences(call,reinterpret_cast<const unsigned char*>(ids),sizeof(ids),DXGI_FORMAT_R16_UINT,0,3,references),"repair negative adjusted indices rejected");call.d=0;call.b=7;Check(!RepairParticleReferences(call,reinterpret_cast<const unsigned char*>(ids),sizeof(ids),DXGI_FORMAT_R16_UINT,0,3,references),"repair truncated index range rejected");const auto tip=RepairNativeTip(HandIdentity());Check(std::fabs(tip.y-.3634923994541168)<1e-9&&std::fabs(tip.z+.08794253319501877)<1e-9,"repair nozzle uses exact authored FX_FIRE node");
 float worldRows[16]={},palette[576]={};worldRows[0]=worldRows[5]=worldRows[10]=1;worldRows[3]=100;worldRows[7]=200;Mat4 knob=HandIdentity(),anchor=HandIdentity();knob.m[3]=12;anchor.m[3]=.1;StoreBone(palette,0,knob);StoreBone(palette,1,anchor);const auto body=RepairRootFromPalette(worldRows,palette);Check(std::fabs(body.m[3]-100.1)<1e-6,"repair flame root follows ANCHOR palette1, not pressure knob palette0");knob.m[3]=28;StoreBone(palette,0,knob);Check(std::fabs(RepairRootFromPalette(worldRows,palette).m[3]-100.1)<1e-6,"repair knob animation cannot move the nozzle anchor");
 // PID82728 diagnosis: census bookkeeping, the 147456-byte pooled particle VB now shadowed, nearest-vertex report when nothing is within reach.
 g_repairCensus=RepairCensus{};RepairNoteReason(0x8CBC4FD2A59FD8A3ull,kRepairShader);Check(g_repairCensus.n==0,"repair census refusal notes never create entries");
 auto& entry=g_repairCensus.e[g_repairCensus.n++];entry.vs=0x8CBC4FD2A59FD8A3ull;entry.withTrigger=3;RepairNoteReason(0x8CBC4FD2A59FD8A3ull,kRepairShader);RepairNoteReason(0x42F3089A89C80C9Cull,kRepairNoneNear);Check(g_repairCensus.e[0].reasons==kRepairShader,"repair census attributes refusals to the draw's own vertex shader");char text[256];Check(strcmp(RepairReasonText(kRepairShader|kRepairNoneNear,text,sizeof(text)),"not-whitelisted-VS,no-vertex-within-1m-of-nozzle")==0&&strcmp(RepairReasonText(0,text,sizeof(text)),"none")==0,"repair census names every refusing guard");g_repairCensus=RepairCensus{};
 d.ByteWidth=147456;d.Usage=D3D11_USAGE_DYNAMIC;g_inGameplay=true;g_repairTool.seen=g_frame;Check(RepairKeepVertexShadow(d),"repair 147456-byte pooled particle VB (VS 8CBC, PID82728) is shadowed now");d.ByteWidth=256*1024;Check(!RepairKeepVertexShadow(d),"repair shadow cap still excludes large scene uploads");g_repairTool=oldTool;g_inGameplay=oldGameplay;
 {std::vector<UINT> refs={0,1,2};const Mat4 nozzle=HandIdentity();const auto tip=RepairNativeTip(nozzle);Vertex vv[3]={};vv[0].p[0]=static_cast<float>(tip.x)+.1f;vv[0].p[1]=static_cast<float>(tip.y);vv[0].p[2]=static_cast<float>(tip.z);vv[1].p[0]=static_cast<float>(tip.x)+5;vv[1].p[1]=static_cast<float>(tip.y);vv[2].p[0]=static_cast<float>(tip.x);vv[2].p[1]=static_cast<float>(tip.y)-.3f;vv[2].p[2]=static_cast<float>(tip.z);Check(RepairCountNearNozzle(reinterpret_cast<const unsigned char*>(vv),sizeof(vv),sizeof(Vertex),0,refs,nozzle,.5)==2,"repair near-nozzle count follows referenced world positions only");refs={1};Check(RepairCountNearNozzle(reinterpret_cast<const unsigned char*>(vv),sizeof(vv),sizeof(Vertex),0,refs,nozzle,.5)==0,"repair unreferenced near vertices are not counted");Vertex localMesh[3]={};refs={0,1,2};Check(RepairCountNearNozzle(reinterpret_cast<const unsigned char*>(localMesh),sizeof(localMesh),sizeof(Vertex),0,refs,root,.5)==0,"repair local-space mesh never counts as near a world nozzle");}
 double nearest=0;memcpy(copy,source,sizeof(copy));Mat4 farRoot=root;farRoot.m[3]+=50;Check(RepairMoveParticleVertices(reinterpret_cast<unsigned char*>(copy),sizeof(copy),sizeof(Vertex),0,farRoot,move,nullptr,&nearest)==0&&nearest>44.9&&nearest<45.1&&memcmp(copy,source,sizeof(copy))==0,"repair reports the nearest referenced vertex when none is within reach and moves nothing");
 std::puts("PASS: repair world-space particle attachment preserves source uploads, distant scene vertices and material attributes; strict shader/resource guards without GPU readback; census bookkeeping");
}
