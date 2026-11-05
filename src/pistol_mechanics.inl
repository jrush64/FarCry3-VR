#include "pistol_piece_rest.inl"
#include "pistol_insert_visual.inl"
#include "weapon_sight_pose51.inl"
Mat4 PistolParentBody(const Mat4& renderedBody);
extern LONG g_rifleAttachFrame;extern UINT g_rifleAttachEpoch;void RifleAttachLearnFromFrame(const Mat4& rootNative);
// D50: this frame's corrected root without the FRAME draw (controller x learned attachment). Two-handed (not pose-owned)
// the root is the plain moved gun, which every piece already gets, so there is nothing to predict.
bool D50PredictRoot(ID3D11DeviceContext* context,uintptr_t vertexIdentity)
{
 auto& p=g_pistolReload;
 // only while the learned hold is in use (pose-owned); otherwise every piece has the plain hand move already
 Mat4 controlled124={};const bool fixed124=ControllerMagazineRoot124(controlled124);
 if(!fixed124&&(!p.parentValid||p.parentEpoch!=g_armPoseEpoch||!PistolPoseOwned()||!g_handRightValid))return false;
 CameraSummary camera={};ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);UINT bytes=sizeof(camera);
 const bool ok=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera);SafeRelease(cb);
 Mat4 controller={},viewInverse={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
 if(!ok||!Invert(HullView(camera),viewInverse))return false;
 p.rootBody=fixed124?controlled124:Multiply(controller,p.controllerParent);
 p.rootWorld=Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.rootBody));
 p.rootFrame=g_frame;p.rootContext=context;p.rootEpoch=g_armPoseEpoch;p.rootVertex=vertexIdentity;
 p.port=Apply(p.rootBody,{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]});p.portFrame=g_frame;
 static LONG logged=0;if(InterlockedIncrement(&logged)==1)Log("[D50] pieces drawn before the FRAME placed on this frame's controller-attached root");
 return true;
}
// 1911 pieces outside the part table that the game draws BEFORE the FRAME.
// PID85832 (signature Kimber): the holographic sight, start 13701 count 2100,
// came before the FRAME in ~93% of frames, so it never had a same-frame root
// and kept the plain hand move: median 18 cm, up to 65 cm off the gun. Its
// offset from the native FRAME is constant (rigid, deviation 0 when drawn
// after the FRAME), so it is learned at the FRAME draw of the same frame and
// applied next frame on the predicted controller-attached root.
void WritePistolWorld(float* globals,const Mat4& world);
// Same rendered frame can contain a later native camera/aim reconciliation.
// Keep the established body attachment; only express it in THIS draw's space.
void Pistol1911DrawRoot(ID3D11DeviceContext* context){
 // The established profile20 repair also applies to other classified pistol
 // FRAME/attachment callers. Identity, context and epoch gates live in the
 // helper; no new model/draw ownership is inferred here.
 RefreshWeaponDrawRoot51(context);
}
struct PistolEarlyPiece{UINT start=0,count=0;Mat4 native={},rel={};LONG nativeFrame=-100000;UINT epoch=0;bool valid=false;};
PistolEarlyPiece g_pistolEarly[4];
PistolEarlyPiece* PistolEarlySlot(UINT start,UINT count){for(auto& e:g_pistolEarly)if(e.count&&e.start==start&&e.count==count)return &e;for(auto& e:g_pistolEarly)if(!e.count){e=PistolEarlyPiece{};e.start=start;e.count=count;return &e;}return nullptr;}
void PistolEarlyLearn(const Mat4& rootNative){
 Mat4 inverse={};if(!Invert(rootNative,inverse))return;
 for(auto& e:g_pistolEarly)if(e.count&&e.nativeFrame==g_frame){if(!e.valid||e.epoch!=g_armPoseEpoch)Log("[1911] piece start %u count %u is drawn before the FRAME: offset from the native FRAME learned; placed on the predicted root from now on",e.start,e.count);e.rel=Multiply(inverse,e.native);e.epoch=g_armPoseEpoch;e.valid=true;}
}
// 1911 root before its FRAME draw, by the 1911's own rule (PistolParentBody):
// controller x learned hold whenever the hold is valid and not plain two-handed.
// PID14076: the D50 predictor (pose-owned only) refused during normal holding, so
// the Kimber sight kept the plain hand move, native recoil included, ~3.7 cm off
// and jumping on every shot. Plain two-handed: the FRAME is the plain moved gun,
// which the sight already gets, so nothing is predicted.
bool Pistol1911PredictRoot(ID3D11DeviceContext* context,uintptr_t vertexIdentity){
 auto& p=g_pistolReload;
 Mat4 fixed124={};const bool controlled124=ControllerMagazineRoot124(fixed124);
 if(!controlled124&&(!p.parentValid||p.parentEpoch!=g_armPoseEpoch||!g_handRightValid||(g_twoHand&&!PistolPoseOwned())))return false;
 CameraSummary camera={};ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);UINT bytes=sizeof(camera);
 const bool ok=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera);SafeRelease(cb);
 Mat4 controller={},viewInverse={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
 if(!ok||!Invert(HullView(camera),viewInverse))return false;
 p.rootBody=controlled124?fixed124:Multiply(controller,p.controllerParent);
 p.rootWorld=Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.rootBody));
 p.rootFrame=g_frame;p.rootContext=context;p.rootEpoch=g_armPoseEpoch;p.rootVertex=vertexIdentity;
 p.port=Apply(p.rootBody,{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]});p.portFrame=g_frame;
 return true;
}
bool PistolEarlyWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& nativeWorld,float* globals,uintptr_t vertexIdentity){
 auto* e=PistolEarlySlot(start,count);if(!e)return false;e->native=nativeWorld;e->nativeFrame=g_frame;
 if(!e->valid||e->epoch!=g_armPoseEpoch||!Pistol1911PredictRoot(context,vertexIdentity))return false;
 WritePistolWorld(globals,Multiply(g_pistolReload.rootWorld,e->rel));return true;
}
// 1911-family attachments from OTHER model files (PID57824, signature Kimber):
// holographic sight vb 48520 (skinned body A38F 168/3030, rigid D75F 0/168) and
// silencer vb 10560 (0/1080, drawn before the FRAME every frame) kept the plain
// hand move while the FRAME is controller-parented: 5 cm median, up to 40 cm,
// native offsets constant. Each attachment learns its offset from the native
// FRAME in a frame where the FRAME came first; otherwise the FRAME's native pose
// this frame is recovered from the attachment's own native pose and that offset.
// The result is a view-space move V W V^-1 (W = displayed root x native root^-1)
// used by both the rigid (cb0 world) and skinned (palette) paths.
struct PistolAttachment{UINT vb=0,start=0,count=0;Mat4 rel={};UINT epoch=0;bool valid=false;};
PistolAttachment g_pistolAttach[32];LONG g_pistolAttachProfile220=-1;
PistolAttachment* PistolAttachSlot(UINT vb,UINT start,UINT count){
 if(g_pistolAttachProfile220!=g_weaponMechanicProfile){for(auto& a:g_pistolAttach)a={};g_pistolAttachProfile220=g_weaponMechanicProfile;}
 for(auto& a:g_pistolAttach)if(a.vb==vb&&a.start==start&&a.count==count)return &a;
 for(auto& a:g_pistolAttach)if(!a.vb){a={};a.vb=vb;a.start=start;a.count=count;return &a;}return nullptr;
}
// Pure placement math (fixture-tested): view-space move for an attachment whose native world is `att`.
bool PistolAttachmentView(const Mat4& view,const Mat4& rootWorld,const Mat4& rootNative,Mat4& handMove){
 Mat4 rootNativeInverse={},viewInverse={};if(!Invert(rootNative,rootNativeInverse)||!Invert(view,viewInverse))return false;
 handMove=Multiply(view,Multiply(Multiply(rootWorld,rootNativeInverse),viewInverse));return true;
}
bool PistolAttachmentMove(ID3D11DeviceContext* context,UINT start,UINT count,Mat4& handMove,const float* palette){
 if(D50Model()||AkModel()||SkorpionModel()||!PistolHandling()||!MotionInteractionEnabled())return false;
 DrawCall call={};call.a=count;call.b=start;const auto g=NativeWeaponGeometry(context,call);
 const auto& mesh=CurrentPistolMesh();if(!g.vertexBytes||g.vertexBytes==mesh.vb||g.vertexBytes==316240)return false;
 if(!RifleAttachmentModel(g.vertexBytes,g.indexBytes))return false;
 ID3D11Buffer* cb[2]={};context->VSGetConstantBuffers(0,2,cb);float globals[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb[0],globals,sizeof(globals));
 CameraSummary camera={};UINT size=sizeof(camera);const bool cam=cb[1]&&SUCCEEDED(cb[1]->GetPrivateData(kCameraSummaryGuid,&size,&camera))&&size==sizeof(camera)&&camera.mainView;for(auto*& b:cb)SafeRelease(b);
 const int worldOffset=WeaponWorldOffset(context);
 if(bytes<UINT((worldOffset+15)*4)||!cam)return false;
 const Mat4 world=HullWorld(globals+worldOffset);
 // A skinned sight's physical FRAME is world * bone0. Learning world alone
 // makes its early draw follow the previous native pose whenever bone0 moves.
 const Mat4 att=palette?Multiply(world,BoneMatrix(palette,0)):world,view=HullView(camera);auto& p=g_pistolReload;
 PistolAttachment* slot=PistolAttachSlot(g.vertexBytes,start,count);if(!slot)return false;
 const bool fresh=p.rootFrame==g_frame&&p.rootNativeFrame==g_frame&&p.rootEpoch==g_armPoseEpoch&&p.rootContext==context;
 if(fresh)
 {
  Pistol1911DrawRoot(context);
  Mat4 inverse={};if(!Invert(p.rootNative,inverse))return false;const Mat4 rel=Multiply(inverse,att);
  if(std::sqrt(rel.m[3]*rel.m[3]+rel.m[7]*rel.m[7]+rel.m[11]*rel.m[11])>1.0)return false; // not a world matrix on this gun (e.g. reticle glow)
  if(!slot->valid||slot->epoch!=g_armPoseEpoch)Log("[1911 ATTACH] vb %u start %u count %u: offset from the native FRAME learned (%.3f %.3f %.3f m); follows the corrected FRAME",g.vertexBytes,start,count,rel.m[3],rel.m[7],rel.m[11]);
  slot->rel=rel;slot->epoch=g_armPoseEpoch;slot->valid=true;
  return PistolAttachmentView(view,p.rootWorld,p.rootNative,handMove);
 }
 if(!slot->valid||slot->epoch!=g_armPoseEpoch)return false;
 if(p.rootFrame!=g_frame&&!Pistol1911PredictRoot(context,p.rootVertex))return false; // plain two-handed: the FRAME has the plain move too
 Pistol1911DrawRoot(context); // current predictor can also predate a later camera reconciliation
 Mat4 relInverse={};if(!Invert(slot->rel,relInverse))return false;
 return PistolAttachmentView(view,p.rootWorld,Multiply(att,relInverse),handMove);
}
bool PistolBusy()
{
    const auto& p=g_pistolReload;
    return (AkModel()&&p.fireResumeFrame>=0&&p.rootFrame<=p.fireResumeFrame)||p.pending||p.cycle||p.removalHeld||p.slideClosing||!p.magazinePresent||PistolOwnsLeft()||p.needsRack||p.waitTriggerRelease||
        (p.reloadChamberTracked&&(!p.chamberReady||p.awaitingNativeClose))||g_pistolMenu.open;
}
V3 PistolRackGuide()
{const float* v=AkModel()?g_pistolGuides.charge:g_pistolGuides.slide;const V3 point={v[0],v[1],v[2]};return A2000ChargePoint336(point);}
void AdvancePistolRack(const fc3pad::Controllers& c,ULONGLONG now)
{
    auto& p=g_pistolReload;
    if(c.rightTrigger<.3f)p.waitTriggerRelease=false;
    // A player can release the inserted magazine before FillAmmo arrives.
    // Remember that release; transfer must not demand a second release.
    if(D50Model()&&p.enabled&&p.cycle&&p.reloadWasEmpty&&!p.held&&!p.pending&&c.leftGrip<.35f)
        p.d50GripReleased175=true;
    if(p.held||p.pending||(p.cycle&&!((AkModel()||(D50Model()&&p.enabled&&p.reloadWasEmpty))&&p.committed))||p.slideClosing||g_pistolMenu.open)return;
    // Release cannot depend on a fresh mesh, invertible root or proximity.
    if(c.leftGrip<.35f)
    {
        p.rackMode=p.rackArmed=false;
        if(p.rackHeld)
        {
            p.rackHeld=false;p.waitTriggerRelease=c.rightTrigger>.3f;
            if(p.rackRear){p.rackReturning=true;p.rackReturnAt=now;p.rackStartPull=p.slidePull;
                if(D50Model()&&p.enabled&&p.committed&&(p.reloadWasEmpty||p.needsRack)&&PistolLocal(p.state,p.wrapper)&&
                   fc3throw::RequestD50Sound225(p.state,p.wrapper,p.weapon,g_armPoseEpoch))fc3pad::RequestReload();
            }
            else {p.rackRear=false;p.slidePull=p.emptyLocked?.05:0;Log("[PISTOL SLIDE] short pull released; prior chamber state preserved");}
        }
    }
    // Once a verified full stroke is released, its return uses time, not hand tracking.
    if(p.rackReturning)
    {
        const double t=(std::min)(1.0,static_cast<double>(now-p.rackReturnAt)/100.0);
        p.slidePull=p.rackStartPull*(1-t);
        if(t>=1)
        {
            p.rackMode=p.rackArmed=p.rackReturning=p.rackRear=p.needsRack=p.emptyLocked=false;p.slidePull=0;
            if(D50Model()&&p.enabled&&p.committed)p.d50RackComplete168=true;
            if(g_weaponMechanicProfile==1||g_weaponMechanicProfile==20||g_weaponMechanicProfile==39||g_weaponMechanicProfile==12||AkModel()){p.chamberReady=true;p.awaitingNativeClose=false;if(AkModel()){p.akRackComplete=true;p.fireResumeFrame=g_frame;}}
            p.waitTriggerRelease=c.rightTrigger>.3f;fc3xr::Buzz(0);
            Log("[PISTOL SLIDE] physical rear/release/return confirmed; slide mode exited, trigger release=%d",p.waitTriggerRelease);
        }
        return;
    }
    if((g_weaponMechanicProfile==1||g_weaponMechanicProfile==20||SkorpionModel()||AkModel()||D50Model())&&p.needsRack){p.rackMode=true;if(c.leftGrip<.35f)p.rackArmed=true;}
    if(!p.rackMode)return;
    const bool fresh=g_frame-p.rootFrame<=1&&g_frame-p.palmFrame<=1&&p.rootEpoch==g_armPoseEpoch&&((g_weaponMechanicProfile!=20&&!SkorpionModel())||(g_handLeftValid&&g_handRightValid));
    Mat4 inverse={};
    Mat4 rackRoot=p.rootBody;const bool rootReady=g_weaponMechanicProfile!=20||Resolve1911Insertion(p.rootBody,rackRoot,false);
    if(!fresh||!rootReady||!Invert(rackRoot,inverse))
    {
        if(p.rackHeld&&c.rightTrigger>.3f)p.waitTriggerRelease=true;
        p.rackMode=p.rackArmed=p.rackHeld=p.rackRear=false;p.slidePull=p.emptyLocked?.05:0;
        Log("[PISTOL SLIDE] tracking/root unavailable; slide ownership released, chamber unchanged");return;
    }
    // Rifles whose hand sits ON the handle during the pull: the drawn palm is pinned there, so the stroke is read from the
    // real controller wrist (body space) - the SPAS forend lesson.
    const bool realWrist=(g_weaponMechanicProfile==1||g_weaponMechanicProfile==20||SkorpionModel()||(AkModel()&&(CurrentRifle().chargeHand||g_weaponMechanicProfile==7||g_weaponMechanicProfile==28))||D50Model())&&g_handLeftValid;
    const V3 local=Apply(inverse,realWrist?PhysicalReloadWrist312(ControllerWrist(g_handBody[1],1)):p.tip);
    if(!p.rackHeld&&p.rackArmed&&c.leftGrip>.7f&&PistolGuideContact(p.rootBody,AkModel()?3:1,p.tip))
    {
        p.rackWristValid167=false;
        p.rackHeld=true;p.rackArmed=false;p.rackRear=false;p.rackStart=local;
        p.rackStartPull=0;p.slidePull=0;g_twoHand=false;
        Log("[PISTOL SLIDE] explicit physical grip acquired; local -Y rail; locked open=%d",p.emptyLocked);
    }
    if(p.rackHeld)
    {
        p.slidePull=(std::max)(0.0,(std::min)(.05,p.rackStartPull+p.rackStart.y-local.y));
        if(p.slidePull>=.04)p.rackRear=true;
    }
}
void TickPistolMechanics(const fc3pad::Controllers& c)
{
    const auto now=GetTickCount64();auto& p=g_pistolReload;
    if(p.slideClosing&&now-p.closeAt>=160){p.slideClosing=false;p.waitTriggerRelease=c.rightTrigger>.3f;}
    AdvancePistolRack(c,now);
}
double PistolClosePull(ULONGLONG now)
{
    const auto& p=g_pistolReload;
    if(!p.slideClosing)return 0;
    const double t=(std::min)(1.0,static_cast<double>(now-p.closeAt)/160.0);
    return .05*(1-t);
}
// Learn only a quiet, completed FRAME relative to the tracked right controller.
// Subsequent reload FRAME draws use this attachment, including native animation tail.
Mat4 PistolParentBody(const Mat4& renderedBody)
{
    auto& p=g_pistolReload;Mat4 controller={},inverse={};
    for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
    if(!g_handRightValid||!Invert(controller,inverse))return renderedBody;
    if(p.parentEpoch!=g_armPoseEpoch){p.parentValid=false;p.parentEpoch=g_armPoseEpoch;}
    Mat4 fixed124={};if(ControllerMagazineRoot124(fixed124)){
        p.controllerParent=Multiply(inverse,fixed124);p.parentValid=true;
        return fixed124;
    }
    if(D50Model())
    { // D50 (revision 5: two-handed was wrong and one-handed had misplaced pieces - revision 4 re-learned the hold from the
      // drawn gun while the drawn gun came from that hold: a feedback loop, the gun drifted off the hand). Now: normal
      // holding (one- or two-handed) is the plain hand move, exactly what two-handed always was (two-handed
      // was fine); the learned hold is used ONLY while a reload / slide pull owns the pose, and it is learned from 15
      // steady frames while NOT in use, so it can never feed back on itself.
     static Mat4 last={};static int steady=0;static UINT epoch=0xffffffffu;
     if(epoch!=g_armPoseEpoch){epoch=g_armPoseEpoch;steady=0;}
     if(!g_twoHand&&!PistolPoseOwned()&&fc3pad::Latest().rightTrigger<.3f)
     {
      const Mat4 candidate=Multiply(inverse,renderedBody);double change=0;
      for(int i=0;i<12;++i)change=(std::max)(change,std::fabs(candidate.m[i]-last.m[i]));
      steady=change<.004?steady+1:0;last=candidate;
      if(steady>=15)
      {
       double moved=0;for(int i=0;i<12;++i)moved=(std::max)(moved,std::fabs(candidate.m[i]-p.controllerParent.m[i]));
       if(!p.parentValid||moved>.02){static LONG logs=0;if(InterlockedIncrement(&logs)<=12)Log("[D50 HOLD] one-handed hold %s from a steady grip (change %.3f)",p.parentValid?"re-learned":"learned",moved);}
       p.controllerParent=candidate;p.parentValid=true;
      }
     }
     else steady=0;
     return p.parentValid&&PistolPoseOwned()?Multiply(controller,p.controllerParent):renderedBody;
    }
    if(!p.parentValid&&!g_twoHand&&!PistolPoseOwned()&&fc3pad::Latest().rightTrigger<.3f)
    {static UINT loggedEpoch=0xffffffffu;if(loggedEpoch!=g_armPoseEpoch){loggedEpoch=g_armPoseEpoch;Log("[1911 PARENT] quiet FRAME attached to tracked controller, pose epoch %u",g_armPoseEpoch);}p.controllerParent=Multiply(inverse,renderedBody);p.parentValid=true;}
    return p.parentValid&&(!g_twoHand||PistolPoseOwned())?Multiply(controller,p.controllerParent):renderedBody;
}
// Visual ownership needs the tracked gun hand; missing support-hand actions
// must never detach the suppressor or change the barrel used for native aim.
bool SkorpionWeaponVisualActive()
{return SkorpionModel()&&g_pistolReload.enabled&&MotionInteractionEnabled()&&g_handRightValid;}
bool SkorpionRigidDraw(ID3D11DeviceContext* context,UINT start,UINT count)
{
 if(!SkorpionModel())return false;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(d.stride!=32||d.offset!=0||d.format!=57)return false;
 if(d.vertexBytes==157984&&d.indexBytes==35322)
  return (start==0&&count==1548)||(start==1548&&count==468)||
         (start==2016&&count==1056)||(start==3072&&count==10347)||(start==13419&&count==4242);
 return d.vertexBytes==10560&&d.indexBytes==2160&&start==0&&count==1080;
}
bool SkorpionAimBody(V3& forward)
{
 if(!SkorpionWeaponVisualActive())return false;
 const auto& p=g_pistolReload;
 if(!p.parentValid||p.parentEpoch!=g_armPoseEpoch)return false;
 Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
 const Mat4 root=Multiply(controller,p.controllerParent);
 const V3 barrel={root.m[1],root.m[5],root.m[9]};const double length=Length(barrel);
 if(!std::isfinite(length)||length<1e-6)return false;
 forward=Scale(barrel,1/length);return true;
}
// The bare 1911 FRAME owns its muzzle direction, independently of attachments.
// Use the same root as its corrected Globals draw, including two-hand rotation.
Mat4 AkParentBody();
bool PistolAimBody(V3& forward)
{
 if(!PistolModel()||SkorpionModel()||!g_pistolReload.enabled||
    !MotionInteractionEnabled()||!g_handRightValid)return false;
 const auto& p=g_pistolReload;
 if(!p.parentValid||p.parentEpoch!=g_armPoseEpoch||p.rootEpoch!=g_armPoseEpoch||
    g_frame-p.rootFrame>2)return false;
 const Mat4 root=AkModel()?AkParentBody():PistolParentBody(p.rootBody);
 const V3 barrel={root.m[1],root.m[5],root.m[9]};const double length=Length(barrel);
 if(!std::isfinite(length)||length<1e-6)return false;
 forward=Scale(barrel,1/length);return true;
}
#include "pistol_split_translation.inl"
void WritePistolWorld(float* globals,const Mat4& world)
{
    if(PistolPreciseTranslation()&&WritePistolSplitWorld(globals,world))return;
    for(int r=0;r<3;++r)for(int c=0;c<4;++c)
        globals[r*4+c]=static_cast<float>(world.m[r*4+c]-(c==3?globals[12+r]:0));
}
Mat4 PistolDropWorld(const Mat4& start,double seconds)
{
    Mat4 result=start;
    const double exit=(std::min)(1.0,seconds/.12)*.06;
    result.m[3]-=start.m[2]*exit;result.m[7]-=start.m[6]*exit;result.m[11]-=start.m[10]*exit;
    // Dunia world is Z-up. Cosmetic trajectory, no engine physics entity.
    result.m[11]-=.5*9.81*seconds*seconds;
    return result;
}
bool MagazineHideBullet(ID3D11DeviceContext* context,const DrawCall& call)
{
 if(!g_magazineStates[0].enabled||(g_weaponMechanicProfile!=20&&g_weaponMechanicProfile!=1)||g_frame-g_weaponMechanicFrame>2||!MotionInteractionEnabled())return false;
 const auto d=NativeWeaponGeometry(context,call);
 return PistolMeshMain(d)&&PistolMeshPart(CurrentPistolMesh().bullet,call.b,call.a);
}
void AkFinalWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,float*);
bool PistolGuideCamera(ID3D11DeviceContext*,CameraSummary&);
bool MagazineAttachmentWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& nativeWorld,float* globals)
{
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=10560||d.indexBytes!=2160||d.stride!=32||d.offset!=0||d.format!=57||start!=0||count!=1080)return false;
 // 1911/Kimber silencer: PistolAttachmentMove already placed it on the corrected FRAME.
 // PID104568: this older path overwrote it in ~1/3 of one-handed frames (drawn before
 // the FRAME, it paired this frame's silencer with last frame's native FRAME: 1-52 cm).
 if(!SkorpionModel())return false;
 auto& p=g_pistolReload;
 if(!p.parentValid||p.parentEpoch!=g_armPoseEpoch)return false;
 CameraSummary camera={};ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);UINT bytes=sizeof(camera);
 const bool cameraOk=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera);SafeRelease(cb);
 Mat4 viewInverse={},nativeInverse={};if(!cameraOk||!Invert(HullView(camera),viewInverse))return false;
 // The SMG suppressor is rigid. Its asset socket is verified against idle
 // native draw pairs; a reload's animated wrist is not its attachment parent.
 if(SkorpionModel()){
  const Mat4 root=Multiply(viewInverse,Multiply(ArmBodyToCamera(),PistolParentBody(p.rootBody)));
  p.silencerRelative=kSkorpionSilencerFromFrame;p.silencerValid=true;p.silencerEpoch=g_armPoseEpoch;
  WritePistolWorld(globals,Multiply(root,kSkorpionSilencerFromFrame));return true;
 }
 Mat4 nativeRoot={};
 if(p.rootFrame==g_frame&&p.rootContext==context&&p.rootEpoch==g_armPoseEpoch)nativeRoot=p.rootNative;
 else if(p.nativeGunFrame==g_frame)nativeRoot=Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.nativeGunBody));
 else if(!p.silencerValid||p.silencerEpoch!=g_armPoseEpoch)return false;
 if((p.rootFrame==g_frame&&p.rootContext==context&&p.rootEpoch==g_armPoseEpoch)||p.nativeGunFrame==g_frame){
  if(!Invert(nativeRoot,nativeInverse))return false;
  p.silencerRelative=Multiply(nativeInverse,nativeWorld);p.silencerValid=true;p.silencerEpoch=g_armPoseEpoch;
 }
 const Mat4 root=Multiply(viewInverse,Multiply(ArmBodyToCamera(),PistolParentBody(p.rootBody)));
 WritePistolWorld(globals,Multiply(root,p.silencerRelative));return true;
}
void SkorpionFinalWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,float*);
bool Lever1887FinalWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,float*);
bool FlareFinalWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,float*);bool RpgFinalWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,float*);bool Gm94FinalWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,float*);
void PistolFinalWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& nativeWorld,float* globals)
{
    if(Lever1887FinalWorld(context,start,count,nativeWorld,globals))return;
    if(FlareFinalWorld(context,start,count,nativeWorld,globals))return;
    if(RpgFinalWorld(context,start,count,nativeWorld,globals))return;
    if(Gm94FinalWorld(context,start,count,nativeWorld,globals))return;
    if(!PistolHandling()&&!PistolAmmoPreviewActive()&&!SkorpionWeaponVisualActive()&&!AkWeaponVisualActive())return;
    if(AkModel()){AkFinalWorld(context,start,count,nativeWorld,globals);return;}
    if(MagazineAttachmentWorld(context,start,count,nativeWorld,globals))return;
    if(SkorpionModel()){SkorpionFinalWorld(context,start,count,nativeWorld,globals);return;}
    DrawCall call={};call.a=count;call.b=start;const auto geometry=NativeWeaponGeometry(context,call);
    const auto& mesh=CurrentPistolMesh();
    if(geometry.vertexBytes!=mesh.vb||geometry.indexBytes!=mesh.ib||geometry.format!=57)return;
    const bool mainStream=geometry.stride==32&&geometry.offset==mesh.offset;
    const bool tinyStream=geometry.stride==20&&geometry.offset==0;
    const bool tinySlide=tinyStream&&PistolMeshPart(mesh.tinySlide,start,count);
    const bool tinyFrame=tinyStream&&PistolMeshPart(mesh.tinyFrame,start,count);
    if(!mainStream&&!tinySlide&&!tinyFrame)return;
    const bool clipDraw=mainStream&&PistolMeshPart(mesh.clip,start,count),bulletDraw=mainStream&&PistolMeshPart(mesh.bullet,start,count);
    ID3D11Buffer* vertexBuffer=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vertexBuffer,&stride,&offset);
    const uintptr_t vertexIdentity=reinterpret_cast<uintptr_t>(vertexBuffer);SafeRelease(vertexBuffer);
    auto& p=g_pistolReload;
    bool fresh=p.rootFrame==g_frame&&p.rootContext==context&&p.rootEpoch==g_armPoseEpoch&&p.rootVertex==vertexIdentity;
    if(fresh)Pistol1911DrawRoot(context);
    if(mainStream&&PistolMeshFrame(start,count))
    {
        if(!fresh)
        {
            p.rootWorld=HullWorld(globals);p.rootNative=nativeWorld;p.rootNativeFrame=g_frame;p.rootFrame=g_frame;
            p.rootContext=context;p.rootEpoch=g_armPoseEpoch;p.rootVertex=vertexIdentity;
            ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);
            CameraSummary camera={};UINT bytes=sizeof(camera);Mat4 bodyInverse={};
            if(cameraBuffer&&SUCCEEDED(cameraBuffer->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera)&&Invert(ArmBodyToCamera(),bodyInverse))
            {
                p.rootBody=PistolParentBody(Multiply(bodyInverse,Multiply(HullView(camera),p.rootWorld)));
                p.port=Apply(p.rootBody,{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]});p.portFrame=g_frame;
                Mat4 viewInverse={};if(Invert(HullView(camera),viewInverse))
                {p.rootWorld=Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.rootBody));WritePistolWorld(globals,p.rootWorld);}
            }
            else p.rootFrame=-100000;
            SafeRelease(cameraBuffer);
        }
        else{if(p.rootNativeFrame!=g_frame){p.rootNative=nativeWorld;p.rootNativeFrame=g_frame;}WritePistolWorld(globals,p.rootWorld);}
        if(D50Model()){g_rifleAttachFrame=g_frame;g_rifleAttachEpoch=g_armPoseEpoch;RifleAttachLearnFromFrame(p.rootNative);} // the sight (drawn first) learns its offset this frame
        else if(!P6p9Model()&&p.rootNativeFrame==g_frame)PistolEarlyLearn(p.rootNative); // 1911/Kimber pieces drawn before the FRAME (holographic sight)
        return;
    }
    if(bulletDraw){globals[7]=-10000;return;}
    // D50 (the gun showed up split in two, with loose parts to the left): its slide, hammer
    // and second slide piece are drawn BEFORE the FRAME in most frames, so they kept the plain hand move while the FRAME
    // got the controller attachment. They now take this frame's corrected root directly (controller x learned attachment).
    if(!fresh&&D50Model()&&mainStream){if(!D50PredictRoot(context,vertexIdentity))return;fresh=true;}
    if(clipDraw&&PistolAmmoPreviewActive())return;
    // Never borrow a root from another frame/context/vertex resource/pose epoch.
    // Pieces drawn before the main FRAME keep their existing path, except 1911
    // pieces outside the part table (holographic sight): learned offset on the predicted root.
    const bool unlisted1911=!D50Model()&&!P6p9Model()&&mainStream&&!clipDraw&&!bulletDraw&&!PistolMeshSlide(start,count);
    // An earlier suppressor/sight can predict THIS corrected root while its native
    // FRAME still belongs to LAST frame. Use the learned rigid relation in that
    // case; inverse(lastNative)*currentPart injects Jason's native movement.
    if(unlisted1911&&(!fresh||p.rootNativeFrame!=g_frame)){
        if(PistolEarlyWorld(context,start,count,nativeWorld,globals,vertexIdentity))return;
        return; // no current native relation or learned early relation yet
    }
    if(!fresh)return;
    if(unlisted1911&&p.rootNativeFrame==g_frame){auto* e=PistolEarlySlot(start,count);if(e){e->native=nativeWorld;e->nativeFrame=g_frame;}PistolEarlyLearn(p.rootNative);} // drawn after the FRAME this time: refresh the learned offset
    if(tinyFrame){WritePistolWorld(globals,p.rootWorld);return;} // 6P9 grip piece is FRAME-rigid
    if(clipDraw)
    {
        if(p.held)return; // existing proven completed-left-hand attachment
        if(p.looseFlying)
        {
            ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);CameraSummary cam={};UINT bytes=sizeof(cam);Mat4 inverse={};
            const bool ok=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&cam))&&bytes==sizeof(cam)&&Invert(HullView(cam),inverse);SafeRelease(cb);
            if(ok)WritePistolWorld(globals,Multiply(inverse,Multiply(ArmBodyToCamera(),p.looseBody)));
            else globals[7]=-10000;return;
        }
        if(p.insertActive&&(p.pending||p.cycle||p.magazinePresent))
        {
            const Mat4 clip=Multiply(p.rootWorld,PistolInsertRelative(static_cast<double>(GetTickCount64()-p.insertAt)/1000.0));
            WritePistolWorld(globals,clip);p.clipWorld=clip;p.clipFrame=g_frame;return;
        }
        if(p.dropActive)
        {
            const double age=static_cast<double>(GetTickCount64()-p.dropAt)/1000.0;
            if(age<.65){WritePistolWorld(globals,PistolDropWorld(p.dropWorld,age));return;}
            p.dropActive=false;
        }
        if(!p.magazinePresent||p.pending||p.cycle){globals[7]=-10000;return;}
        const Mat4 clip=Multiply(p.rootWorld,D50Model()?kD50ClipRestM:kPistolClipRest);
        WritePistolWorld(globals,clip);p.clipWorld=clip;p.clipFrame=g_frame;return;
    }
    if(bulletDraw&&(p.pending||p.cycle||!p.magazinePresent)){globals[7]=-10000;return;}
    Mat4 inverse={};if(!Invert(p.rootNative,inverse))return;
    Mat4 relative=Multiply(inverse,nativeWorld);
    const bool staleNative=D50Model()&&p.rootNativeFrame!=g_frame; // FRAME not drawn yet this frame: use last frame's relation
    if(D50Model()){static UINT relStart[8]={};static Mat4 relLast[8]={};int k=-1;for(int i=0;i<8&&k<0;++i)if(relStart[i]==start+1)k=i;
     if(k<0)for(int i=0;i<8&&k<0;++i)if(!relStart[i]){relStart[i]=start+1;k=i;}
     if(k>=0){if(staleNative)relative=relLast[k];else relLast[k]=relative;}}
    if((mainStream&&PistolMeshSlide(start,count))||tinySlide)
    {
        const Mat4& slideRest=D50Model()?kD50SlideRestM:kPistolSlideRest;
        const double lockPull=D50Model()?kD50LockPull:.05,stroke=D50Model()?kD50SlideTravel/.05:1; // D50: 5.6 cm lock, 7 cm stroke
        const double nativePull=staleNative?p.nativeSlidePull:(std::max)(0.0,(std::min)(lockPull,slideRest.m[7]-relative.m[7]));
        p.nativeSlidePull=nativePull;p.nativeSlideFrame=g_frame;
        double pull=nativePull;
        if(p.rackHeld||p.rackReturning)pull=p.slidePull*stroke;
        else if(p.slideClosing)pull=PistolClosePull(GetTickCount64());
        else if(p.cycle)pull=p.reloadChamberTracked?(p.reloadWasEmpty&&(!p.committed||(D50Model()&&!p.d50RackComplete168))?lockPull:0):nativePull; // D50: stays locked back until the pull (no close-then-reopen snap)
        else if(p.pending||p.needsRack)pull=p.emptyLocked?lockPull:0;
        else if(p.emptyLocked)pull=lockPull;
        relative=slideRest;relative.m[7]-=pull;
    }
    WritePistolWorld(globals,Multiply(p.rootWorld,relative));
}

#include "pistol_right_grip.inl"
bool PistolRightHoldingMove(const Mat4& toView,const float* bones,Mat4& move)
{
    auto& p=g_pistolReload;
    if(AkModel()&&!PistolPoseOwned())return false; // preserve the rifle's original normal holding pose
    if((!PistolHandling()&&!SkorpionWeaponVisualActive()&&!AkWeaponVisualActive())||(g_twoHand&&!AkModel())||!p.parentValid||p.parentEpoch!=g_armPoseEpoch)return false;
    Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
    // Recompute current controller parent even when the arm draw precedes FRAME.
    const Mat4 root=AkModel()?AkParentBody():PistolParentBody(p.rootBody);
    const Mat4 desired=Multiply(ArmBodyToCamera(),Multiply(root,MagazineHandFromFrame()));
    move=Multiply(desired,inverse);return true;
}
const float* SmgRightHoldingGrip141(){
    if(g_frame-g_weaponMechanicFrame>2||MacheteHeld())return nullptr;
    switch(g_weaponMechanicProfile){
      case 7:return kA2000RightGripRelative;
      case 28:return kMp5RightIdleFingers;
      case 9:return kBz19RightIdleFingers;
      case 21:return kVectorRightIdleFingers;
      case 39:return kSkorpionRightGripRelative;
      default:return nullptr;
    }
}
bool PistolRightHoldingGrip(float* bones)
{
    // Body IK owns the arm lengths; each SMG keeps its captured firing-hand grip.
    // Restore fingers after the neutral wrist is pinned, including the first arm draw.
    const float* smgGrip=SmgRightHoldingGrip141();
    if(AkModel()&&!PistolPoseOwned()&&!smgGrip)return false;
    if(!smgGrip&&((!PistolHandling()&&!SkorpionWeaponVisualActive()&&!AkWeaponVisualActive())||(g_twoHand&&!AkModel())||!g_pistolReload.parentValid))return false;
    const float* grip=smgGrip?smgGrip:AkModel()?CurrentRifle().rightGrip:SkorpionModel()?kSkorpionRightGripRelative:D50Model()?kD50RightGripRelative:kPistolRightGripRelative;
    const Mat4 wrist=BoneMatrix(bones,2);
    for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(grip,b-3)));
    return true;
}

#include "skorpion_mechanics.inl"
#include "ak_mechanics.inl"
#include "mp5.inl"
#include "lever1887.inl"
const char* MenuWeaponName();
#include "revolver.inl"
#include "spas.inl"
#include "spas_attachment231.inl"
#include "r700.inl"
#include "pkm.inl"
#include "weapons7_guides.inl"
#include "native_shot219.inl"
#include "flare.inl"
#include "rpg.inl"
#include "gm94.inl"
#include "flamer.inl"
