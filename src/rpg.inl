// RPG-7 (rpg7.xbg, profile 32). Design: fire one shot, then physically insert the next rocket with the
// left hand; inserting plays the real animation on the left hand (and possibly the right) together with the actual rocket.
//  - Rigid (captures/weapons7-29048, work/rpg_clips.py -> rpg_clips.inl): FRAME = vb 201024 draw 0/21786; the ROCKET is its own
//    model (vb 32128, draw 0/2916) that the game stops drawing after a shot until its reload brings it back.
//  - The game's own reload (automatic or X) is refused. LEFT grip + X with the tube empty: the engine's own Begin/transfer/End
//    load the round at once (so the game draws the rocket again) and the rocket rides in the left hand (Jason's carry pose and
//    fingers). It never drops: it stays in the hand until it goes in. Trigger dry until it is seated.
//  - Bring the rocket's tail to the tube mouth (within 8 cm of where Jason's rocket meets the tube axis): Jason's own insert
//    plays from that moment (t 1.745 s of the native reload to its end): left hand + fingers, right hand + fingers and the rocket,
//    all on the gun. At its end the rocket is seated and the trigger is live.
#include "rpg_clips.inl"
struct Rpg
{
 NativeShot219 shot;
 bool loaded=true,known=false,carrying=false,pending=false,probing=false,leftGripWas=false,xWas=false,xOwned=false,triggerWas=false;
 int clipFirst=-1;ULONGLONG clipAt=0,lastTick=0,deadline=0,pulseAt=0,probeAt=0,inputPauseAt219=0;bool clip=false;
 Mat4 frameBody={};LONG frameFrame=-100000;Mat4 leftWrist={};LONG leftFrame=-100000;UINT ammoState=0,ammoWrapper=0;LONG inserts=0;
 struct Pose{Mat4 left,right,rocket;bool rocketOn;float leftFingers[204],rightFingers[204];} pose={};LONG poseFrame=-100000;
} g_rpg;
const double kRpgMouthReach=.08;
bool RpgModel(){return g_weaponMechanicProfile==32&&g_frame-g_weaponMechanicFrame<=2;}
bool RpgActive(){return RpgModel()&&MotionInteractionEnabled()&&fc3pad::Latest().active&&g_handLeftValid&&g_handRightValid;}
bool RpgFrameReady(){return g_frame-g_rpg.frameFrame<=2;}
bool RpgLeftReady(){return g_frame-g_rpg.leftFrame<=2;}
Mat4 RpgKeyMat(const float* v){Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=v[i];return m;}
bool RpgClipPlaying(){return g_rpg.clip&&RpgModel();}
bool RpgCarrying(){return g_rpg.carrying;}
int RpgFirstKey(){static int first=-1;if(first<0){first=0;for(int i=0;i<kRpgClipReloadCount;++i)if(kRpgClipReload[i].t<=kRpgRocketEntersTubeAt)first=i;}return first;}
double RpgClipSeconds(){const auto now=g_rpg.inputPauseAt219?g_rpg.inputPauseAt219:W7Now();return kRpgClipReload[RpgFirstKey()].t+(now>=g_rpg.clipAt?now-g_rpg.clipAt:0)/1000.0;}
const Rpg::Pose& RpgClipPose()
{
 auto& R=g_rpg;if(R.poseFrame==g_frame)return R.pose;R.poseFrame=g_frame;
 const double s=(std::min)(RpgClipSeconds(),static_cast<double>(kRpgClipReload[kRpgClipReloadCount-1].t));
 int i=0;while(i+1<kRpgClipReloadCount&&kRpgClipReload[i+1].t<=s)++i;
 const RpgKey& a=kRpgClipReload[i];const RpgKey& b=kRpgClipReload[(std::min)(i+1,kRpgClipReloadCount-1)];const double span=b.t-a.t,w=span>1e-6?(std::max)(0.0,(std::min)(1.0,(s-a.t)/span)):0;
 R.pose.left=PistolRigidBlend(RpgKeyMat(a.leftHand),RpgKeyMat(b.leftHand),w);R.pose.right=PistolRigidBlend(RpgKeyMat(a.rightHand),RpgKeyMat(b.rightHand),w);
 R.pose.rocket=PistolRigidBlend(RpgKeyMat(a.rocket),RpgKeyMat(b.rocket),w);R.pose.rocketOn=true;
 for(int bone=0;bone<17;++bone){const Mat4 l=PistolRigidBlend(BoneMatrix(a.leftFingers,bone),BoneMatrix(b.leftFingers,bone),w),r=PistolRigidBlend(BoneMatrix(a.rightFingers,bone),BoneMatrix(b.rightFingers,bone),w);
  for(int j=0;j<12;++j){R.pose.leftFingers[bone*12+j]=static_cast<float>(l.m[j]);R.pose.rightFingers[bone*12+j]=static_cast<float>(r.m[j]);}}
 return R.pose;
}
// Rocket in the left hand (body space) and its origin in FRAME space.
Mat4 RpgCarriedBody(){return OtherCarried319(W7VisualWrist54(1,g_rpg.leftWrist),W7Ammo(1,RpgKeyMat(kRpgRocketFromLeftWrist)));}
Mat4 RpgMouthBox(){Mat4 m=HandIdentity();m.m[3]=kRpgTubeMouthRocketOriginFromFrame[0];m.m[7]=kRpgTubeMouthRocketOriginFromFrame[1];m.m[11]=kRpgTubeMouthRocketOriginFromFrame[2];return W7Adjust(1,0,m);}
double RpgMouthContact(const V3& point){Mat4 inv={};if(!Invert(RpgMouthBox(),inv))return 1e9;const V3 p=Apply(inv,point);const double v[3]={p.x,p.y,p.z};double d=0;for(int i=0;i<3;++i){const double a=v[i]/((g_w7Guides[1].size[0][i]+g_w7Guides[1].diameter)*.5*(std::max)(.1f,g_worldScale));d+=a*a;}return sqrt(d);}
V3 RpgPalmAt(const Mat4& wrist){const auto& h=g_w7Guides[1];return Apply(wrist,{-.591+h.hand[0],.011+h.hand[1],1.149+h.hand[2]});}
// Sensor and preview use the same rocket-local point, independent of model calibration.
V3 RpgInsertionAt(const Mat4& wrist){const auto& h=g_w7Guides[1];if(h.contactAnchor<.5f)return RpgPalmAt(wrist);return Apply(OtherCarried319(wrist,W7Ammo(1,RpgKeyMat(kRpgRocketFromLeftWrist))),{h.rocketContact[0],h.rocketContact[1],h.rocketContact[2]});}
bool RpgCarriedFrame(V3&);
Mat4 RpgHeldContactBox54(const Mat4& wrist){Mat4 box=OtherCarried319(wrist,W7Ammo(1,RpgKeyMat(kRpgRocketFromLeftWrist)));const V3 at=RpgInsertionAt(wrist);box.m[3]=at.x;box.m[7]=at.y;box.m[11]=at.z;return box;}
bool ReloadBoxesOverlap54(const Mat4& a,const float* da,const Mat4& b,const float* db,double scale){
 V3 axes[6];for(int i=0;i<3;++i){axes[i]={a.m[i],a.m[4+i],a.m[8+i]};axes[3+i]={b.m[i],b.m[4+i],b.m[8+i]};axes[i]=Scale(axes[i],1/(std::max)(1e-12,Length(axes[i])));axes[3+i]=Scale(axes[3+i],1/(std::max)(1e-12,Length(axes[3+i])));}
 const V3 delta={b.m[3]-a.m[3],b.m[7]-a.m[7],b.m[11]-a.m[11]};
 auto separated=[&](V3 n){if(Length(n)<1e-8)return false;double extent=0;for(int i=0;i<3;++i)extent+=(fabs(Dot(n,axes[i]))*da[i]+fabs(Dot(n,axes[3+i]))*db[i])*.5*scale;return fabs(Dot(delta,n))>extent+1e-8;};
 for(int i=0;i<6;++i)if(separated(axes[i]))return false;
 for(int i=0;i<3;++i)for(int j=0;j<3;++j){const auto x=axes[i],y=axes[3+j];if(separated({x.y*y.z-x.z*y.y,x.z*y.x-x.x*y.z,x.x*y.y-x.y*y.x}))return false;}return true;
}
bool RpgInsertionContact54(){V3 o={};if(!RpgCarriedFrame(o))return false;if(!g_w7Guides[1].rocketBox54)return RpgMouthContact(o)<=1;return ReloadBoxesOverlap54(Multiply(g_rpg.frameBody,RpgMouthBox()),g_w7Guides[1].size[0],RpgHeldContactBox54(g_rpg.leftWrist),g_w7Guides[1].rocketSize54,(std::max)(.1f,g_worldScale));}
bool RpgCarriedFrame(V3& origin){Mat4 inverse={};if(!RpgFrameReady()||!RpgLeftReady()||!Invert(g_rpg.frameBody,inverse))return false;origin=Apply(inverse,RpgInsertionAt(g_rpg.leftWrist));return true;}
// The rocket was invisible while Jason screwed it in and reappeared only after a weapon switch: after the
// engine load the game keeps the rocket draw off until the next equip. The rocket draw is snapshotted (state, buffers, material) the
// first time the game draws it and replayed on the launcher body's own draw (its camera, targets and this frame's globals, both
// eyes - the revolver speedloader method) whenever the game did not draw it in the previous frame.
// Only the rocket's ambient occlusion showed: the game draws the rocket in SEVERAL passes (depth/AO, then the
// shaded pass ...); an earlier build copied only the first draw it saw - the depth/AO pass - so only its shadow showed. Now every pass the game
// draws the rocket in is copied (keyed by shader + render-target formats) and each one is replayed on the launcher body's draw of
// the SAME pass.
struct RpgRocketPass{HullPart part;UINT stride=0,offset=0;unsigned long long vs=0;LONG frame=-1;};
struct RpgRocketVisual{RpgRocketPass pass[6];int count=0;ID3D11DeviceContext* deferred=nullptr;LONG gameFrame=-100000;bool failed=false,logged=false;}g_rpgRocket;
bool RpgRocketDraw(const fc3weapon::Geometry& d,UINT start,UINT count){return d.vertexBytes==32128&&d.stride==32&&start==0&&count==2916;}
// This draw's pass key: vertex shader + render-target / depth formats.
bool RpgPassKey(ID3D11DeviceContext* context,unsigned long long& vs,DXGI_FORMAT (&formats)[8],DXGI_FORMAT& depthFormat)
{
 ID3D11VertexShader* shader=nullptr;context->VSGetShader(&shader,nullptr,nullptr);vs=ShaderHash(shader);SafeRelease(shader);
 ID3D11RenderTargetView* targets[8]={};ID3D11DepthStencilView* depth=nullptr;context->OMGetRenderTargets(8,targets,&depth);
 for(UINT i=0;i<8;++i){formats[i]=HullFormat(targets[i]);SafeRelease(targets[i]);}
 depthFormat=HullFormat(depth);const bool ok=depth!=nullptr;SafeRelease(depth);return ok;
}
int RpgFindPass(unsigned long long vs,const DXGI_FORMAT (&formats)[8],DXGI_FORMAT depthFormat)
{
 auto& V=g_rpgRocket;
 for(int k=0;k<V.count;++k){const auto& p=V.pass[k].part;bool same=V.pass[k].vs==vs&&p.depthFormat==depthFormat;for(UINT i=0;i<8&&same;++i)same=p.formats[i]==formats[i];if(same)return k;}
 return -1;
}
void ObserveRpgRocketDraw(ID3D11DeviceContext* context,const DrawCall& call)
{
 if(call.kind!=0||call.d!=0||!RpgModel())return;const auto d=NativeWeaponGeometry(context,call);if(!RpgRocketDraw(d,call.b,call.a))return;
 auto& V=g_rpgRocket;V.gameFrame=g_frame;if(V.failed||V.count>=6)return;
 unsigned long long vs=0;DXGI_FORMAT formats[8]={},depthFormat=DXGI_FORMAT_UNKNOWN;if(!RpgPassKey(context,vs,formats,depthFormat)||RpgFindPass(vs,formats,depthFormat)>=0)return;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);std::vector<float> data(kSmallCbMax/4);const UINT bytes=ReadCopy(cb,data.data(),kSmallCbMax);SafeRelease(cb);
 if(bytes<80)return;data.resize(bytes/4);
 auto& P=V.pass[V.count];P.vs=vs;
 ID3D11Buffer* vb=nullptr;context->IAGetVertexBuffers(0,1,&vb,&P.stride,&P.offset);SafeRelease(vb);
 bool ok=SnapshotVisualPart(context,call,P.part,data);if(ok&&!V.deferred)ok=SUCCEEDED(g_dev->CreateDeferredContext(0,&V.deferred));
 if(!ok){P=RpgRocketPass{};Log("[RPG] a rocket pass could not be copied");return;}
 ++V.count;Log("[RPG] rocket pass %d copied (vs %016llX, %u target(s), first format %d): drawn by the mod in that pass",V.count,vs,P.part.targets,static_cast<int>(formats[0]));
}
Mat4 RpgRocketBodyNow();
void ReplayRpgRocket(ID3D11DeviceContext* context,const DrawCall& call)
{
 auto& V=g_rpgRocket;const auto& R=g_rpg;
 if(!V.count||!V.deferred||!RpgModel()||!MotionInteractionEnabled()||g_frame-V.gameFrame<=1)return; // the game draws it itself
 if(!(R.carrying||R.clip||R.loaded||W7PreviewWristReady54(1))||!RpgFrameReady())return;
 if(call.kind!=0||call.d!=0||call.b!=0||call.a!=21786)return;
 {const auto d=NativeWeaponGeometry(context,call);if(d.vertexBytes!=201024||d.stride!=32)return;}
 unsigned long long vs=0;DXGI_FORMAT formats[8]={},depthFormat=DXGI_FORMAT_UNKNOWN;if(!RpgPassKey(context,vs,formats,depthFormat))return;
 const int k=RpgFindPass(vs,formats,depthFormat);if(k<0||V.pass[k].frame==g_frame)return;
 auto& P=V.pass[k];const auto& p=P.part;
 ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);CameraSummary camera={};UINT cbytes=sizeof(camera);
 bool ok=cameraBuffer&&SUCCEEDED(cameraBuffer->GetPrivateData(kCameraSummaryGuid,&cbytes,&camera))&&cbytes==sizeof(camera);
 std::vector<float> current(p.data.size());
 {ID3D11Buffer* g=nullptr;context->VSGetConstantBuffers(0,1,&g);const UINT got=ReadCopy(g,current.data(),static_cast<UINT>(current.size()*4));SafeRelease(g);ok=ok&&got>=48;}
 ID3D11RenderTargetView* targets[8]={},*twins[8]={};ID3D11DepthStencilView* depth=nullptr,*twinDepth=nullptr;
 context->OMGetRenderTargets(8,targets,&depth);
 if(ok){for(UINT i=0;i<p.targets;++i)if(targets[i]){twins[i]=static_cast<ID3D11RenderTargetView*>(TwinView(targets[i],ViewKind::RenderTarget,false));ok=ok&&twins[i];}
  twinDepth=static_cast<ID3D11DepthStencilView*>(TwinView(depth,ViewKind::DepthStencil,false));ok=ok&&twinDepth&&g_eyeCb[1][0]&&g_eyeCb[1][1];}
 Mat4 viewInverse={};ok=ok&&Invert(HullView(camera),viewInverse);
 if(ok)
 {
  auto data=current;WritePistolWorld(data.data(),Multiply(viewInverse,Multiply(ArmBodyToCamera(),RpgRocketBodyNow())));
  D3D11_VIEWPORT viewport[16]={};UINT nvp=16;context->RSGetViewports(&nvp,viewport);D3D11_RECT scissor[16]={};UINT nsc=16;context->RSGetScissorRects(&nsc,scissor);
  const bool own=g_own;g_own=true;auto* dc=V.deferred;dc->ClearState();dc->RSSetViewports(nvp,viewport);dc->RSSetScissorRects(nsc,scissor);
  context->UpdateSubresource(p.globals,0,nullptr,data.data(),0,0);
  UINT stride=P.stride,offset=P.offset;dc->IASetVertexBuffers(0,1,&p.vb,&stride,&offset);
  dc->IASetIndexBuffer(p.ib,DXGI_FORMAT_R16_UINT,p.indexOffset);dc->IASetInputLayout(p.layout);dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  dc->VSSetShader(p.vs,nullptr,0);dc->PSSetShader(p.ps,nullptr,0);
  ID3D11Buffer* vertexCb[3]={p.globals,cameraBuffer,p.clip};dc->VSSetConstantBuffers(0,3,vertexCb);
  dc->PSSetConstantBuffers(0,8,const_cast<ID3D11Buffer**>(p.psCb));dc->PSSetShaderResources(0,16,const_cast<ID3D11ShaderResourceView**>(p.textures));dc->PSSetSamplers(0,16,const_cast<ID3D11SamplerState**>(p.samplers));
  dc->RSSetState(p.raster);dc->OMSetDepthStencilState(p.depth,p.stencil);dc->OMSetBlendState(p.blend,p.blendFactor,p.mask);
  for(int eye=0;eye<2;++eye)
  {
   dc->OMSetRenderTargets(p.targets,eye?twins:targets,eye?twinDepth:depth);
   dc->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyeCb[1][eye]);
   if(HasFlag(p.ps,kPsPatchedGuid))dc->PSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyePsCb[eye]);
   dc->DrawIndexed(p.draw.a,p.draw.b,p.draw.d);
  }
  ID3D11CommandList* list=nullptr;
  if(SUCCEEDED(dc->FinishCommandList(FALSE,&list))){context->ExecuteCommandList(list,TRUE);list->Release();P.frame=g_frame;
   if(!V.logged){Log("[RPG] rocket drawn by the mod (both eyes, %d pass(es) copied): the game is not drawing it",V.count);V.logged=true;}}
  g_own=own;
 }
 for(UINT i=0;i<8;++i){SafeRelease(targets[i]);SafeRelease(twins[i]);}
 SafeRelease(depth);SafeRelease(twinDepth);SafeRelease(cameraBuffer);
}
bool RpgFinalWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4&,float* globals)
{
 if(!RpgModel()||!MotionInteractionEnabled())return false;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 const bool body=d.vertexBytes==201024&&d.stride==32&&d.offset==0&&start==0&&count==21786,rocket=d.vertexBytes==32128&&d.stride==32&&start==0&&count==2916;
 if(!body&&!rocket)return false;
 CameraSummary camera={};if(!PistolGuideCamera(context,camera))return false;
 const Mat4 view=HullView(camera);Mat4 viewInverse={},bodyInverse={};if(!Invert(view,viewInverse)||!Invert(ArmBodyToCamera(),bodyInverse))return false;
 auto& R=g_rpg;
 if(body){Mat4 fixedWorld=HullWorld(globals),support=RpgKeyMat(kRpgLeftSupportFromFrame);ControllerGripWorld124(context,RpgKeyMat(kRpgRightHandFromFrame),&support,fixedWorld);WritePistolWorld(globals,fixedWorld);R.frameBody=Multiply(bodyInverse,Multiply(view,fixedWorld));R.frameFrame=g_frame;return true;}
 if(!RpgFrameReady())return false;
 if(!R.loaded&&!R.carrying&&!R.clip&&!W7PreviewWristReady54(1)){Mat4 hidden=HandIdentity();hidden.m[7]=-10000;WritePistolWorld(globals,hidden);return true;}
 WritePistolWorld(globals,Multiply(viewInverse,Multiply(ArmBodyToCamera(),RpgRocketBodyNow())));return true;
}
// Where the rocket is now (body space): in Jason's insert, in the hand, or seated (the game's own idle pose sits 8.6 cm further out).
Mat4 RpgRocketBodyNow()
{
 const auto& R=g_rpg;
 Mat4 root=R.frameBody,grip={};ControllerCurrentRoot124(root,grip);
 if(RpgClipPlaying())return Multiply(root,RpgClipPose().rocket);
 if((R.carrying&&RpgLeftReady())||W7PreviewWristReady54(1))return RpgCarriedBody();
 return Multiply(root,RpgKeyMat(kRpgRocketRestFromFrame));
}
void RecordRpgLeftHand(ID3D11DeviceContext* context,const float* bones)
{
 if(!RpgModel())return;Mat4 view={},inverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
 g_rpg.leftWrist=Multiply(inverse,view);g_rpg.leftFrame=g_frame;
}
void RpgFingers(float* bones,const float* fingers){const Mat4 wrist=BoneMatrix(bones,2);for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(fingers,b-3)));}
bool RpgLeftGrip(float* bones){if(!RpgActive())return false;if(RpgClipPlaying()){RpgFingers(bones,RpgClipPose().leftFingers);return true;}if(g_rpg.carrying){RpgFingers(bones,kRpgLeftRocketFingers);return true;}if(g_twoHand){RpgFingers(bones,kRpgLeftIdleFingers);return true;}return false;}
bool RpgRightGrip(float* bones){if(!RpgActive())return false;RpgFingers(bones,RpgClipPlaying()?RpgClipPose().rightFingers:kRpgRightIdleFingers);return true;}
// Wrists during Jason's insert: FRAME x clip wrist (the revolver clip placement).
bool RpgClipMove(int side,const Mat4& toView,const float* bones,Mat4& move)
{
 if(!RpgActive()||!RpgClipPlaying()||!RpgFrameReady())return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 Mat4 root=g_rpg.frameBody,grip={};ControllerCurrentRoot124(root,grip);const auto& p=RpgClipPose();move=Multiply(Multiply(ArmBodyToCamera(),Multiply(root,side?p.left:p.right)),inverse);return true;
}
int RpgLoadedRounds();
void UpdateRpgFire(const fc3pad::Controllers& c)
{
 auto& R=g_rpg;fc3pad::RequireFullFirePull(false);
 const bool active=RpgActive(),pull=c.rightTrigger>.7f;const int loaded=RpgLoadedRounds();const auto now=W7Now();
 if(active&&loaded<0&&R.known){R.known=false;R.probeAt=0;}
 // Native ammo may change after a timeout, equip or external refill. Only a
 // settled tube can reconcile; carrying/insertion owns a preloaded native round.
 if(active&&loaded>=0&&R.known&&!R.shot.pending&&!R.carrying&&!R.clip&&!R.pending&&R.loaded!=(loaded>0)){
  R.loaded=loaded>0;Log("[RPG239] settled tube reconciled to native count %d",loaded);
 }
 const bool ready=R.loaded&&!R.carrying&&!R.clip&&!R.pending;
 const bool wasPending=R.shot.pending;
 const bool confirmed=PollNativeShot219(R.shot,active,ready,R.ammoState,R.ammoWrapper,loaded,now);
 if(confirmed){
  R.loaded=false;Log("[RPG219] native shot confirmed: tube empty (LEFT grip + X: rocket)");
 }
 if(wasPending&&!R.shot.pending&&!confirmed){R.known=false;R.probeAt=0;}
 if(c.rightTrigger<.3f&&!R.shot.pending)R.shot.waitRelease=false;
 if(active&&ready&&loaded>0&&pull&&!R.triggerWas&&!R.shot.pending&&!R.shot.waitRelease)
  BeginNativeShot219(R.shot,R.ammoState,R.ammoWrapper,loaded,now);
 fc3pad::BlockFire(active&&(!ready||loaded<=0||R.shot.waitRelease)&&!R.shot.pending);
 R.triggerWas=pull;
}
int RpgLoadedRounds(){const auto& R=g_rpg;if(!R.ammoWrapper||!ReloadSignalLocal(R.ammoState,R.ammoWrapper))return -1;const UINT w=ReloadRead(R.ammoWrapper,0x48);if(!w)return -1;return static_cast<int>(ReloadRead(w,0xcc));}
// Reload request seam (game thread): learns the weapon; LEFT grip + X loads the round (engine Begin/transfer/End).
void RpgRequest(UINT state,UINT wrapper)
{
 if(g_pistolMenu.open)return;
 auto& R=g_rpg;if(!ReloadSignalLocal(state,wrapper))return;
 R.ammoState=state;R.ammoWrapper=wrapper;
 if(R.probing){R.probing=false;fc3pad::CancelReloadRequest();const int n=RpgLoadedRounds();if(!R.known&&n>=0){R.known=true;R.loaded=n>0;}Log("[RPG] weapon known: %d rocket(s) loaded",n);}
 if(!R.pending||W7Now()>R.deadline)return;
 R.pending=false;fc3pad::CancelReloadRequest();
 UINT loaded=0,capacity=0,reserve=0;const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 if(!Lever1887Ammo(wrapper,loaded,capacity,reserve))
 {fc3xr::Buzz(0);Log("[RPG] capacity unavailable: no native transfer");return;}
 if((std::max)(loaded,before)>=capacity||!reserve){fc3xr::Buzz(0);Log("[RPG] rocket refused: %u/%u, reserve %u",(std::max)(loaded,before),capacity,reserve);return;}
 const bool ok=AkInstantNativeTriple(state,wrapper);const UINT after=ReloadRead(weapon,0xcc);
 if(after>before){R.carrying=true;fc3xr::Buzz(1);}
 Log("[RPG] rocket taken: native Begin/transfer/End, loaded %u -> %u, engine ok=%d; it rides in the left hand until it goes in",before,after,ok);
}
void TickRpg(bool gameplay)
{
 if(g_pistolMenu.open)return;
 auto& R=g_rpg;const auto c=fc3pad::AmmoControls();
 const bool active=gameplay&&RpgActive();
 if(RpgModel()||R.xOwned){fc3pad::SetXOwned(active);R.xOwned=active;}
 const bool lg=c.leftGrip>.7f,lgEdge=lg&&!R.leftGripWas,xEdge=c.x&&!R.xWas;R.leftGripWas=fc3pad::GripOnlyAmmoEnabled()?(lg||(c.leftGrip>=.35f&&R.leftGripWas)):lg;R.xWas=c.x;
 const ULONGLONG now=W7Now();R.lastTick=now;
 if(!active){
  if(R.clip&&!R.inputPauseAt219)R.inputPauseAt219=now;
  if(!RpgModel()){
   if(R.pending)fc3pad::CancelReloadRequest();R.pending=false;
   if(R.clip){R.clip=false;R.carrying=true;R.poseFrame=-100000;}
   R.inputPauseAt219=0;
  }
  return;
 } // a carried/preloaded rocket stays available across switches
 if(R.inputPauseAt219){if(R.clip)R.clipAt+=now-R.inputPauseAt219;R.inputPauseAt219=0;R.poseFrame=-100000;}
 if(R.pending&&now>R.deadline){R.pending=false;fc3pad::CancelReloadRequest();Log("[RPG] the engine never answered: no rocket");}
 if(R.pending&&now-R.pulseAt>=250){R.pulseAt=now;fc3pad::RequestReload();}
 if(R.probing&&now-R.probeAt>400){R.probing=false;fc3pad::CancelReloadRequest();}
 if(!R.known&&!R.probing&&!R.pending&&now-R.probeAt>2000){R.probing=true;R.probeAt=now;fc3pad::RequestReload();Log("[RPG] probing the weapon once to read its loaded rocket (the reload itself stays refused)");}
 if(R.clip)
 {
  if(RpgClipSeconds()>=kRpgClipReload[kRpgClipReloadCount-1].t){R.clip=false;R.loaded=true;++R.inserts;fc3xr::Buzz(1);Log("[RPG] Jason's insert done: rocket seated, fire live");}
  return;
 }
 if(((xEdge&&lg)||(fc3pad::GripOnlyAmmoEnabled()&&lgEdge&&RpgFrameReady()&&!g_twoHand&&!R.loaded))&&!R.carrying&&!R.pending)
 {
  if(!R.loaded){R.pending=true;R.deadline=now+3000;R.pulseAt=now;fc3pad::RequestReload();Log("[RPG] LEFT grip + X: rocket requested");}
  else{fc3xr::Buzz(0);Log("[RPG] LEFT grip + X: nothing - a rocket is already in");}
 }
 V3 origin={};
 if(R.carrying&&RpgCarriedFrame(origin))
 {
  const V3 mouth={kRpgTubeMouthRocketOriginFromFrame[0],kRpgTubeMouthRocketOriginFromFrame[1],kRpgTubeMouthRocketOriginFromFrame[2]};
  const double dist=Length(Sub(origin,mouth)),scale=(std::max)(.1f,g_worldScale);
  if(RpgInsertionContact54()){R.carrying=false;R.clip=true;R.clipAt=now;R.poseFrame=-100000;fc3xr::Buzz(1);Log("[RPG] palm at the tube mouth (%.3f): Jason's insert plays",dist);}
 }
}
void RpgGuideMarkers()
{
 const auto& R=g_rpg;
 if(!RpgActive()||!RpgFrameReady()||!PhysicalReloadGuidesEnabled()||g_pistolMenu.open){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 const float scale=(std::max)(.1f,g_worldScale);
 const Mat4 box=Multiply(R.frameBody,RpgMouthBox());const V3 mouth={box.m[3],box.m[7],box.m[11]};
 const V3 palm=RpgInsertionAt(R.leftWrist);
 V3 o={};const bool atMouth=RpgInsertionContact54();
 float a[3]={static_cast<float>(mouth.x),static_cast<float>(mouth.y),static_cast<float>(mouth.z)},h[3]={static_cast<float>(palm.x),static_cast<float>(palm.y),static_cast<float>(palm.z)};
 fc3xr::SetReloadMarkers(a,h,static_cast<float>(kRpgMouthReach)*scale,atMouth?1:0,g_w7Guides[1].diameter*scale);fc3xr::SetHandSphere();
 Mat4 basis=box;for(int c=0;c<3;++c){double len=0;for(int j=0;j<3;++j)len+=basis.m[j*4+c]*basis.m[j*4+c];len=sqrt(len);if(len<1e-8)return;for(int j=0;j<3;++j)basis.m[j*4+c]/=len;}
 float transform[16],dims[3];for(int j=0;j<16;++j)transform[j]=static_cast<float>(basis.m[j]);for(int j=0;j<3;++j)dims[j]=g_w7Guides[1].size[0][j]*scale;
 fc3xr::SetWeaponBox(transform,dims,atMouth?1:0);
}
bool RpgNote()
{
 if(!RpgActive())return false;const auto& R=g_rpg;
 fc3xr::SetTestNote(R.clip?L"RPG-7 - LOADING":R.pending?L"RPG-7 - TAKING A ROCKET":R.carrying?L"RPG-7 - ROCKET IN HAND\nPut its tail into the front of the tube.":R.loaded?L"RPG-7":L"RPG-7 - EMPTY\nLEFT grip + X: rocket.");
 return true;
}
