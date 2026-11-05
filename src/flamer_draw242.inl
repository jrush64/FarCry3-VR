// Candidate 242/243: the game's pooled NDC particle draw.
// VS 8CBC4FD2A59FD8A3 is a w=1 pass-through (240's disassembly), over a 147456-byte dynamic VB, stride 36, non-indexed.
// The generic stereo patch places a w=1 output on the flat UI panel, i.e. HEAD-LOCKED: that is the flamethrower jet
// and the flare's muzzle smoke "following the head". 243: (1) every draw of this pool gets the depth-aware eye
// reprojection (ParticlePoolDraw243 -> BeginFlamerProjection241), any weapon, any time in motion gameplay; (2) while a
// muzzle pose is confirmed, the particles born at the native nozzle/muzzle are unprojected with the main camera, moved
// by the native->drawn transport and reprojected (flamethrower: nozzle cone; other weapons: 0.5 m around the native
// muzzle for 1 s after a local shot or launch). Vertices that do not look like NDC are moved in world space instead.
// (3) ParticlePoolProbe243 logs every blended dynamic draw while a jet/launch/shot is live, so the next log names the
// draw if it is not this pool. The game's VB is never written.
struct FlamerDraw242 {LONG logs=0,draws=0,moved=0,unconnected=0,refused=0,gateLogs=0;} g_flamerDraw242;
bool FlamerKeepVertexShadow242(const D3D11_BUFFER_DESC& d){return MotionInteractionEnabled()&&d.Usage==D3D11_USAGE_DYNAMIC&&(d.BindFlags&D3D11_BIND_VERTEX_BUFFER)&&d.ByteWidth==147456;}
bool ParticlePoolDraw243(ID3D11DeviceContext* context,const DrawCall& call)
{
 if(!MotionInteractionEnabled()||call.kind!=1||call.a>16384)return false;
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const bool shader=ShaderHash(vs)==0x8CBC4FD2A59FD8A3ull;SafeRelease(vs);if(!shader)return false;
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC d={};if(vb)vb->GetDesc(&d);SafeRelease(vb);
 const bool pool=d.Usage==D3D11_USAGE_DYNAMIC&&d.ByteWidth==147456&&stride==36&&offset==0;
 auto& s=g_flamerDraw242;if(!pool&&s.gateLogs<12){++s.gateLogs;Log("[POOL243] 8CBC draw with an unexpected buffer: usage %d bytes %u stride %u offset %u count %u",int(d.Usage),d.ByteWidth,stride,offset,call.a);}
 return pool;
}
bool BeginFlamerDraw242(ID3D11DeviceContext* context,const DrawCall& call,ID3D11Buffer*& original,UINT& stride,UINT& offset)
{
 original=nullptr;if(!ParticlePoolDraw243(context,call))return false;
 auto& s=g_flamerDraw242;const auto& pose=g_muzzleFlashPose111;++s.draws;
 const bool jet=g_weaponMechanicProfile==15&&FlamerModel()&&fc3pad::Latest().rightTrigger>.3f;
 const bool shot=!jet&&MuzzleFlashWindow111()&&g_muzzleFlash111.localValid;
 if(!jet&&!shot){if(s.draws%300==1&&s.logs<60){++s.logs;Log("[POOL243] pool draw %ld: no jet/shot window (profile %ld, trigger %.2f, shot window %d); depth-aware only, nothing moved",s.draws,g_weaponMechanicProfile,fc3pad::Latest().rightTrigger,MuzzleFlashWindow111()?1:0);}return false;}
 if(!pose.valid||pose.epoch!=g_armPoseEpoch||g_frame-pose.frame>2||!g_frameCameraValid||(jet&&pose.profile!=15))
 {if(++s.refused<=12)Log("[POOL243] pool draw left in place: core pose valid %d profile %u age %ld frame(s) epoch %u/%u, frame camera %d",pose.valid?1:0,pose.profile,g_frame-pose.frame,pose.epoch,g_armPoseEpoch,g_frameCameraValid?1:0);return false;}
 ID3D11Buffer* vb=nullptr;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC desc={};if(vb)vb->GetDesc(&desc);if(!vb)return false;
 LONG copied=-100000;UINT frameBytes=sizeof(copied);
 if(FAILED(vb->GetPrivateData(kRepairVertexFrameGuid,&frameBytes,&copied))||g_frame-copied<0||g_frame-copied>1)
 {if(++s.refused<=12)Log("[POOL243] pool draw left in place: no CPU copy of the particle buffer this frame (copied frame %ld, now %ld)",copied,g_frame);SafeRelease(vb);return false;}
 std::vector<unsigned char> vertices(desc.ByteWidth);if(ReadCopy(vb,vertices.data(),desc.ByteWidth)!=desc.ByteWidth){SafeRelease(vb);return false;}
 std::vector<UINT> refs;DXGI_FORMAT indexFormat=DXGI_FORMAT_UNKNOWN;UINT indexOffset=0;
 if(!RepairReferencedVertices(context,call,desc.ByteWidth,stride,offset,refs,indexFormat,indexOffset)||refs.empty()||refs.size()>4096){SafeRelease(vb);return false;}
 const CameraSummary& camera=g_frameCamera;const Mat4 view=HullView(camera);Mat4 cameraInverse={},nativeInverse={};
 if(!camera.mainView||camera.xScale<=0||camera.yScale<=0||fabs(camera.zOffset)<1e-8||!Invert(view,cameraInverse)||!Invert(pose.native,nativeInverse))
 {if(++s.refused<=12)Log("[POOL243] pool draw left in place: frame camera unusable (main %u, scales %.4f/%.4f, z %.6f/%.6f)",camera.mainView,camera.xScale,camera.yScale,camera.zScale,camera.zOffset);SafeRelease(vb);return false;}
 // The native birthplace: the flamethrower nozzle, or the native muzzle of the last local shot/launch.
 const V3 centreLocal=jet?kFlamerNozzle239:g_muzzleFlash111.local;const V3 centreNative=Apply(pose.native,centreLocal),centreDrawn=Apply(pose.move,centreNative);
 auto inside=[&](const V3& world){const V3 local=Sub(Apply(nativeInverse,world),centreLocal);if(!jet)return Length(local)<=.5;const double radius=.75+(std::max)(0.,local.y)*.15;return local.y>=-.25&&local.y<=8&&local.x*local.x+local.z*local.z<=radius*radius;};
 // Classify the buffer once per draw: NDC (|x|,|y|<=1.5, z in [0,1.1]) or world.
 UINT finite=0,ndcLike=0;float first[3]={};
 for(UINT vertex:refs){const auto at=static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(vertex)*stride;if(at+12>vertices.size())continue;float p[3];memcpy(p,vertices.data()+at,12);if(!std::isfinite(p[0])||!std::isfinite(p[1])||!std::isfinite(p[2]))continue;++finite;if(finite==1)memcpy(first,p,12);if(fabs(p[0])<=1.5&&fabs(p[1])<=1.5&&p[2]>=-.1&&p[2]<=1.1)++ndcLike;}
 if(!finite){SafeRelease(vb);return false;}
 const bool ndc=ndcLike*10>=finite*8;
 struct Vertex{UINT index;V3 out;};std::vector<Vertex> selected;double nearestNative=1e18,nearestDrawn=1e18;double firstDepth=0;V3 firstWorld={};UINT unprojected=0;
 for(UINT vertex:refs)
 {
  const auto at=static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(vertex)*stride;if(at+12>vertices.size())continue;
  float p[3];memcpy(p,vertices.data()+at,12);if(!std::isfinite(p[0])||!std::isfinite(p[1])||!std::isfinite(p[2]))continue;
  V3 world={};
  if(ndc){const double denominator=double(p[2])-camera.zScale;if(fabs(denominator)<1e-8)continue;const double depth=camera.zOffset/denominator;if(!std::isfinite(depth)||depth<=1e-5||depth>10000)continue;
   world=Apply(cameraInverse,{p[0]*depth/camera.xScale,p[1]*depth/camera.yScale,depth});++unprojected;if(unprojected==1){firstDepth=depth;firstWorld=world;}}
  else{world={p[0],p[1],p[2]};++unprojected;if(unprojected==1)firstWorld=world;}
  const double dn=Length(Sub(world,centreNative)),dd=Length(Sub(world,centreDrawn));if(dn<nearestNative)nearestNative=dn;if(dd<nearestDrawn)nearestDrawn=dd;
  if(!inside(world))continue;
  const V3 moved=Apply(pose.move,world);V3 out=moved;
  if(ndc&&!FlamerProject240(camera,view,moved,out))continue;
  selected.push_back({vertex,out});
 }
 const bool connected=nearestNative<1.&&!selected.empty();
 if(s.logs<60||s.draws%300==0){++s.logs;Log("[POOL243] frame %ld pool draw %ld (%s, profile %ld): %zu referenced, %u finite, %u NDC-like -> treated as %s; first %.4f/%.4f/%.6f -> depth %.2f m world %.2f/%.2f/%.2f; nearest particle %.2f m from the NATIVE birthplace %.2f/%.2f/%.2f and %.2f m from the DRAWN one %.2f/%.2f/%.2f; %zu selected -> %s",
  g_frame,s.draws,jet?"jet":"shot window",g_weaponMechanicProfile,refs.size(),finite,ndcLike,ndc?"NDC (main camera)":"WORLD",first[0],first[1],first[2],firstDepth,firstWorld.x,firstWorld.y,firstWorld.z,nearestNative<1e17?nearestNative:-1.,centreNative.x,centreNative.y,centreNative.z,nearestDrawn<1e17?nearestDrawn:-1.,centreDrawn.x,centreDrawn.y,centreDrawn.z,selected.size(),connected?"moved to the drawn weapon":"left in place (not connected to the native birthplace)");}
 if(!connected){++s.unconnected;SafeRelease(vb);return false;}
 for(const auto& v:selected){float p[3]={float(v.out.x),float(v.out.y),float(v.out.z)};memcpy(vertices.data()+static_cast<size_t>(offset)+static_cast<size_t>(v.index)*stride,p,12);}
 if(g_muzzleFlashVbBytes111!=desc.ByteWidth){SafeRelease(g_muzzleFlashVb111);g_muzzleFlashVbBytes111=0;D3D11_BUFFER_DESC d={};d.ByteWidth=desc.ByteWidth;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;if(FAILED(g_oCreateBuffer(g_dev,&d,nullptr,&g_muzzleFlashVb111))){SafeRelease(vb);return false;}g_muzzleFlashVbBytes111=d.ByteWidth;}
 const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_muzzleFlashVb111,0,nullptr,vertices.data(),0,0);context->IASetVertexBuffers(0,1,&g_muzzleFlashVb111,&stride,&offset);g_own=own;original=vb;++s.moved;
 return true;
}
// Read-only census of every BLENDED draw from a DYNAMIC vertex buffer while the flamethrower fires, a launched
// projectile is in flight, or a local shot is within its 1 s window. No per-frame cap: one line per distinct
// signature every 20 frames, 150 lines per session.
void ParticlePoolProbe243(ID3D11DeviceContext* context,const DrawCall& call,int variant,bool consumed,bool depthAware)
{
 static LONG lines=0;if(lines>=150||!MotionInteractionEnabled())return;
 const bool jet=g_weaponMechanicProfile==15&&fc3pad::Latest().rightTrigger>.3f;
 if(!jet&&!fc3aim::ProjectileFlightActive242()&&!MuzzleFlashWindow111())return;
 ID3D11BlendState* blend=nullptr;float factor[4];UINT mask=0;context->OMGetBlendState(&blend,factor,&mask);D3D11_BLEND_DESC bd={};if(blend)blend->GetDesc(&bd);SafeRelease(blend);if(!bd.RenderTarget[0].BlendEnable)return;
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC d={};if(vb)vb->GetDesc(&d);
 if(!vb||d.Usage!=D3D11_USAGE_DYNAMIC){SafeRelease(vb);return;}
 ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;ID3D11GeometryShader* gs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);context->PSGetShader(&ps,nullptr,nullptr);context->GSGetShader(&gs,nullptr,nullptr);
 const unsigned long long vh=ShaderHash(vs),ph=ShaderHash(ps);const bool patched=vs&&HasFlag(vs,kVsPatchedGuid);const bool hasGs=gs!=nullptr;SafeRelease(vs);SafeRelease(ps);SafeRelease(gs);
 struct Seen{unsigned long long signature;LONG frame;};static Seen seen[64];static int seenCount=0;
 const unsigned long long signature=vh*31+ph*17+d.ByteWidth*7+stride+call.kind;
 for(int i=0;i<seenCount;++i)if(seen[i].signature==signature){if(g_frame-seen[i].frame<20)return;seen[i].frame=g_frame;goto log;}
 if(seenCount<64)seen[seenCount++]={signature,g_frame};
log:
 {
  ID3D11Buffer* cbs[4]={};context->VSGetConstantBuffers(0,4,cbs);int cbCount=0;for(auto*& b:cbs){if(b)++cbCount;SafeRelease(b);}
  D3D11_PRIMITIVE_TOPOLOGY topology={};context->IAGetPrimitiveTopology(&topology);
  float first[3]={NAN,NAN,NAN};LONG copied=-100000;UINT frameBytes=sizeof(copied);bool haveCopy=false;
  if(SUCCEEDED(vb->GetPrivateData(kRepairVertexFrameGuid,&frameBytes,&copied))&&g_frame-copied>=0&&g_frame-copied<=1){std::vector<unsigned char> v(d.ByteWidth);if(ReadCopy(vb,v.data(),d.ByteWidth)==d.ByteWidth){const UINT at=offset+(call.kind==1?call.b:0)*stride;if(at+12<=d.ByteWidth){memcpy(first,v.data()+at,12);haveCopy=true;}}}
  ++lines;Log("[POOL243 CENSUS] frame %ld %s: kind %u count %u start %u | vs %016llX%s ps %016llX gs %s vsCBs %d | vb %u bytes stride %u topo %d | variant %d consumed %d depthAware %d | first vertex %s%.4f/%.4f/%.6f",
   g_frame,jet?"JET":fc3aim::ProjectileFlightActive242()?"LAUNCH":"SHOT",call.kind,call.a,call.kind==1?call.b:call.c,vh,patched?"(patched)":"(UNPATCHED)",ph,hasGs?"YES":"no",cbCount,d.ByteWidth,stride,int(topology),variant,consumed?1:0,depthAware?1:0,haveCopy?"":"(no copy) ",first[0],first[1],first[2]);
 }
 SafeRelease(vb);
}
