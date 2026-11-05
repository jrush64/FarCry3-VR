#include "a2000_charge_wrist229.inl"
// Verified AK47 = ak74m.xbg, profile8. Exact separate rigid draw ownership.
// Native parts precede FRAME in the live draw order. Keep their native motion
// in gun-local space; never parent against a previous frame's world-space root.
struct AkPartPose {Mat4 native={},relative={};LONG frame=-100000;UINT epoch=0;ID3D11DeviceContext* context=nullptr;bool valid=false;LONG profile=-1;};
AkPartPose g_akParts[6];
Mat4 g_rifleAttachMove={};LONG g_rifleAttachFrame=-100000;UINT g_rifleAttachEpoch=0;
bool ShotProbeGunMove(float* out12){if(g_frame-g_rifleAttachFrame>1)return false;for(int i=0;i<12;++i)out12[i]=static_cast<float>(g_rifleAttachMove.m[i]);return true;}
// Weapon attachment models (weapon_presence.h kModels): acog, optical, reflex_a, reflex_b, silencer.
// Attachment geometry by buffer sizes alone: NativeWeaponGeometry rejects draws with an index offset, which the skinned
// holographic sight (vb 48520, palette) uses - so it never reached the attachment path at all (log: no [RIFLE ATTACH] for
// the D50 in any session; capture 57124: the drawn sight wandered up to 7 cm against the drawn gun during each reload).
bool R700BoltPose(int index,const Mat4& nativeRelative,Mat4& relative);bool R700Hold();Mat4 R700HoldBody();bool R700GunMove(ID3D11DeviceContext* context,Mat4&);bool R700Model();bool R700ControllerRoot(Mat4&);
bool AttachmentBuffers(ID3D11DeviceContext* context,UINT& vertexBytes,UINT& indexBytes)
{
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);
 ID3D11Buffer* ib=nullptr;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;UINT indexOffset=0;context->IAGetIndexBuffer(&ib,&format,&indexOffset);
 D3D11_BUFFER_DESC vd={},id={};const bool ok=vb&&ib;if(vb)vb->GetDesc(&vd);if(ib)ib->GetDesc(&id);SafeRelease(vb);SafeRelease(ib);
 vertexBytes=vd.ByteWidth;indexBytes=id.ByteWidth;return ok;
}
bool RifleAttachmentModel(UINT vertexBytes,UINT indexBytes)
{return (vertexBytes==98144&&indexBytes==21168)||(vertexBytes==120816&&indexBytes==24948)||(vertexBytes==48520&&indexBytes==7032)||
        (vertexBytes==67448&&indexBytes==11280)||(vertexBytes==10560&&indexBytes==2160)||(vertexBytes==7680&&R700Model())||(R700Model()&&(vertexBytes==7744||vertexBytes==16800||vertexBytes==12544||vertexBytes==12928));} // + Z93 scope / brakes // R700 silencer (drawn before the FRAME, rigid on it to 0.4 mm)
Mat4 AkParentBody();
// Attachment placement. On the F1 the silencer and scope moved and shook even though they were attached. The log
// ([RIFLE ATTACH]) showed every bad frame had "attach age 1": the scope / silencer draw BEFORE the FRAME in some
// frames, so they used LAST frame's FRAME move (up to 12 cm behind while the gun moved). Now:
//  - not pose-owned: the FRAME itself will get exactly this frame's hand move (AkFinalWorld: rootBody = rendered body),
//    so the attachment takes the same hand move - same frame, no lag;
//  - pose-owned (magazine / rack): the FRAME sits on the controller attachment (AkParentBody, fresh every frame), so the
//    attachment is placed from THIS frame's AkParentBody x its learned offset from the native FRAME (the AK parts rule).
struct RifleAttachmentRel{UINT vb,start;Mat4 rel;UINT epoch;bool valid;Mat4 native;LONG nativeFrame;};
// 32 entries, cleared when the weapon changes. The old 8-entry table was filled by the P416's nine optic draws, so the D50's
// sight never got an entry and silently kept the bare hand move (the holographic sight kept moving across four headset
// tests); the P416's own ninth optic draw was left 1 m off for the same reason.
RifleAttachmentRel g_rifleAttachRel[32]={};LONG g_rifleAttachProfile=-1;
RifleAttachmentRel* RifleAttachSlot(UINT vb,UINT key)
{
 if(g_rifleAttachProfile!=g_weaponMechanicProfile){for(auto& r:g_rifleAttachRel)r=RifleAttachmentRel{};g_rifleAttachProfile=g_weaponMechanicProfile;}
 for(auto& r:g_rifleAttachRel)if(r.vb==vb&&r.start==key)return &r;
 for(auto& r:g_rifleAttachRel)if(!r.vb){r.vb=vb;r.start=key;r.epoch=0;r.valid=false;r.nativeFrame=-100000;return &r;}
 static LONG warned=0;if(InterlockedIncrement(&warned)<=4)Log("[RIFLE ATTACH] attachment table full: vb %u key %u gets the bare hand move",vb,key);
 return nullptr;
}
// Attachments drawn BEFORE the FRAME (the D50 sight, every frame): their native world is kept, and the FRAME draw of the same
// frame turns it into the offset (exact: both from this frame).
void RifleAttachLearnFromFrame(const Mat4& rootNative)
{
 // Never while a reload / slide pull owns the pose (D50 partial reload: the holographic sight moved as if
 // playing an animation): the native reload animation moves the sight against the FRAME; the offset from steady holding is kept.
 if(PistolPoseOwned())return;
 Mat4 inverse={};if(!Invert(rootNative,inverse))return;
 for(auto& r:g_rifleAttachRel)if(r.vb&&r.nativeFrame==g_frame)
 {if(!r.valid||r.epoch!=g_armPoseEpoch)Log("[RIFLE ATTACH] attachment vb %u start %u (drawn before the FRAME): offset learned at the FRAME draw",r.vb,r.start);r.rel=Multiply(inverse,r.native);r.epoch=g_armPoseEpoch;r.valid=true;}
}
bool D50PredictRoot(ID3D11DeviceContext*,uintptr_t);
bool RifleAttachmentMove(ID3D11DeviceContext* context,UINT start,UINT count,Mat4& move)
{
 const bool d50=D50Model()&&PistolHandling(); // the D50's holographic sight drifted on its own
 if(!AkWeaponVisualActive()&&!d50)return false;
 UINT vertexBytes=0,indexBytes=0;AttachmentBuffers(context,vertexBytes,indexBytes);
 struct{UINT vertexBytes;}d={vertexBytes};
 if(!RifleAttachmentModel(vertexBytes,indexBytes))return false;
 const auto& p=g_pistolReload;
 RifleAttachmentRel* slot=RifleAttachSlot(d.vertexBytes,start);
 const int worldOffset=WeaponWorldOffset(context);
 if(slot){ID3D11Buffer* cb0=nullptr;context->VSGetConstantBuffers(0,1,&cb0);float g0[kSmallCbMax/4]={};if(ReadCopy(cb0,g0,sizeof(g0))>=UINT((worldOffset+15)*4)){slot->native=HullWorld(g0+worldOffset);slot->nativeFrame=g_frame;}SafeRelease(cb0);}
 const bool fresh=g_frame==g_rifleAttachFrame&&g_rifleAttachEpoch==g_armPoseEpoch&&p.rootFrame==g_frame&&p.rootEpoch==g_armPoseEpoch&&p.rootContext==context&&(!d50||p.rootNativeFrame==g_frame);
 if(fresh&&slot&&!PistolPoseOwned())
 { // learn this attachment's offset from the native FRAME (constant: both are game-animated together)
  ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);float globals[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,globals,sizeof(globals));SafeRelease(cb);
  Mat4 rootNativeInverse={};if(bytes>=UINT((worldOffset+15)*4)&&Invert(p.rootNative,rootNativeInverse)){if(!slot->valid||slot->epoch!=g_armPoseEpoch)Log("[RIFLE ATTACH] %ls attachment vb %u start %u: offset from the native FRAME learned; same-frame placement",CurrentRifle().name,d.vertexBytes,start);slot->rel=Multiply(rootNativeInverse,HullWorld(globals+worldOffset));slot->epoch=g_armPoseEpoch;slot->valid=true;}
 }
 if(d50)
 { // pistol FRAME = controller-attached root (two-handed: the plain moved gun, like every piece)
  Mat4 fixed124={};if(!PistolPoseOwned()&&!ControllerMagazineRoot124(fixed124))return RightHandMove(move); // normal hold: the D50 itself has the plain hand move
  if(!slot||!slot->valid||slot->epoch!=g_armPoseEpoch)return RightHandMove(move);
  if(p.rootFrame!=g_frame&&!D50PredictRoot(context,0))return RightHandMove(move);
 }
 else
 {
 // FRAME may have been drawn before a camera/aim reconciliation within this
 // rendered frame. A cached g_rifleAttachMove expresses the old view basis.
 // Refresh before the ordinary-hold fallback as the FRAME now retains its
 // established body pose across repeated materials too.
 if(fresh){if(!RefreshRifleAttachmentMove51(context,move))return false;g_rifleAttachMove=move;return true;}
 Mat4 fixed124={};if(!PistolPoseOwned()&&!R700Hold()&&!R700Model()&&!ControllerMagazineRoot124(fixed124))return RightHandMove(move); // the FRAME gets this very move this frame; R700: its root is controller-parented, place from the learned offset below
 if(!slot||!slot->valid||slot->epoch!=g_armPoseEpoch||(!p.parentValid&&!R700Hold()&&!ControllerMagazineRoot124(fixed124)))return false;
 }
 CameraSummary camera={};if(!PistolGuideCamera(context,camera))return false;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);float globals[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,globals,sizeof(globals));SafeRelease(cb);
 const Mat4 view=HullView(camera);Mat4 nativeInverse={};if(bytes<UINT((worldOffset+15)*4)||!Invert(Multiply(view,HullWorld(globals+worldOffset)),nativeInverse))return false;
 // the target pose is ALREADY view space (ArmBodyToCamera x body x rel): the old code applied the view matrix a second time
 // (Z93 scope: off the gun in 1208 of 1618 recorded frames; every attachment drawn before its gun took this path)
 move=Multiply(Multiply(Multiply(ArmBodyToCamera(),d50?p.rootBody:AkParentBody()),slot->rel),nativeInverse);return true;
}
Mat4 g_akRootController={};UINT g_akRootControllerEpoch=0;
Mat4 AkParentBody()
{
 auto& p=g_pistolReload;
 Mat4 fixed124={};if(!R700Hold()&&ControllerMagazineRoot124(fixed124))return fixed124;
 if(R700Hold())return R700HoldBody(); // R700 physical: the gun is pinned to the left hand where LEFT grip took it
 if(R700Model()&&!PistolPoseOwned()){Mat4 root={};if(R700ControllerRoot(root))return root;} // this frame's controller root (attachments drawn before the FRAME)
 if(PistolPoseOwned()){
  Mat4 root=PistolParentBody(p.rootBody);
  if(g_twoHand&&AkReloadSupport()&&g_handLeftValid){
   Mat4 inverse={};if(Invert(ArmBodyToCamera(),inverse)){
    const V3 pivot=Apply(Multiply(root,(*CurrentRifle().hand)),{.591,.011,1.149});
    const V3 support=Apply(Multiply(root,(*CurrentRifle().support)),{-.591,.011,1.149});
    const V3 real=Apply(inverse,ControllerWrist(g_handLeft,1));
    if(Length(Sub(support,pivot))>.05&&Length(Sub(real,pivot))>.05)root=Multiply(TurnAndMove(Sub(support,pivot),Sub(real,pivot),pivot,pivot),root);
   }
  }return root;
 }
 if(!g_twoHand&&g_handRightValid&&g_akRootControllerEpoch==g_armPoseEpoch){
  Mat4 inverse={},controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
  if(Invert(g_akRootController,inverse))return Multiply(controller,Multiply(inverse,p.rootBody));
 }
 return p.rootBody;
}
// The actual contact and both guide renderers use this same gun-local target.
// Native animated wrist/palm offsets cannot move the box when Insert takes the hand.
V3 RifleSupportPoint343(const V3& native){
 V3 point=A2000ChargePoint336(native);
 if((g_weaponMechanicProfile==14||g_weaponMechanicProfile==31)&&LeftAk316(true))point.x=-point.x;
 return point;
}
bool RifleSupportContact234(V3& point,V3& palm){
 if(!RifleProfile(g_weaponMechanicProfile))return false;
 if(!AkModel()||!g_handLeftValid||!g_handRightValid)return false;
 Mat4 root={};if(!ControllerMagazineRoot124(root))return false;
 if(!WeaponSupportTarget349(point))return false;
 if(!FreeHandPalm(1,palm))palm=ControllerWrist(g_handBody[1],1);
 return true;
}
bool AkSupportPosition(V3& support)
{
 Mat4 fixed124={};const bool controlled124=AkModel()&&!PistolOwnsLeft()&&ControllerMagazineRoot124(fixed124);
 if(!controlled124&&(!AkReloadSupport()||!PistolPoseOwned()||!g_pistolReload.parentValid))return false;
 support=Apply(Multiply(ArmBodyToCamera(),AkParentBody()),RifleSupportPoint343(Apply(*CurrentRifle().support,{-.591,.011,1.149})));return true;
}
bool AkSupportHandMove(const Mat4& toView,const float* bones,Mat4& move)
{
 Mat4 fixed124={};const bool controlled124=AkModel()&&!PistolOwnsLeft()&&ControllerMagazineRoot124(fixed124);
 if(!g_twoHand||(!controlled124&&(!AkReloadSupport()||!PistolPoseOwned())))return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(AkParentBody(),(*CurrentRifle().support))),inverse);return true;
}
void AkObserveParts(ID3D11DeviceContext* context,const Mat4& nativeRoot)
{
 Mat4 inverse={};if(!Invert(nativeRoot,inverse))return;
 for(auto& part:g_akParts)if(part.frame==g_frame&&part.epoch==g_armPoseEpoch&&part.context==context){part.relative=Multiply(inverse,part.native);part.valid=true;}
}
bool AkFrame(UINT start,UINT count)
{for(const auto& f:CurrentRifle().frame)if(f[1]&&f[0]==start&&f[1]==count)return true;return false;}
bool AkRigidDraw(ID3D11DeviceContext* context,UINT start,UINT count)
{
 if(!AkModel()||CurrentRifle().skinned)return false;DrawCall call={};call.a=count;call.b=start; // MP5: palette path (mp5.inl)
 const auto d=NativeWeaponGeometry(context,call);
 return RifleMain(CurrentRifle(),d)&&(AkFrame(start,count)||RifleClip(CurrentRifle(),start,count)||RiflePartIndex(start,count)>=0);
}
void AkWorld(ID3D11DeviceContext*,UINT start,UINT count,const Mat4& nativeToView,Mat4& handMove,float* globals)
{
 auto& p=g_pistolReload;
 if(AkFrame(start,count)){
  Mat4 basis={};if(HandBasis(Multiply(nativeToView,(*CurrentRifle().hand)),basis)){
   p.current=basis;p.basisFrame=g_frame;
   if(!PistolPoseOwned()&&fc3pad::Latest().rightTrigger<.3f){p.idle=basis;p.basis=true;}
   RightHandMove(handMove);
  }
 }
 if(RifleClip(CurrentRifle(),start,count)){
  if((p.held||PistolAmmoPreviewActive())&&g_frame-p.handFrame<=1){Mat4 inverse={};if(Invert(nativeToView,inverse))handMove=Multiply(p.hand,inverse);}
  else if(!p.magazinePresent||p.pending||p.cycle)globals[7]=-10000;
 }
}
void AkFinalWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& nativeWorld,float* globals)
{
 if(!AkRigidDraw(context,start,count))return;
 auto& p=g_pistolReload;CameraSummary camera={};if(!PistolGuideCamera(context,camera))return;
 Mat4 viewInverse={},bodyInverse={};if(!Invert(HullView(camera),viewInverse)||!Invert(ArmBodyToCamera(),bodyInverse))return;
 if(AkFrame(start,count)){
  if(p.rootFrame!=g_frame||p.rootContext!=context||p.rootEpoch!=g_armPoseEpoch){
   p.rootNative=nativeWorld;
   const Mat4 renderedBody=Multiply(bodyInverse,Multiply(HullView(camera),HullWorld(globals)));
   // Normal rifle holding keeps the existing native grip and controller move.
   // Freeze the learned attachment only while a physical reload owns the pose.
   if(R700Model()&&!PistolPoseOwned()&&!R700Hold()){
    Mat4 root={};
    if(R700ControllerRoot(root))
    { // controller only, never the game's hand (r700.inl). The attachment is this exact relation, so aim (barrel), reload
      // (PistolParentBody) and attachments all see the same gun.
     p.rootBody=root;Mat4 controller={},ci={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
     if(Invert(controller,ci)){p.controllerParent=Multiply(ci,root);p.parentValid=true;p.parentEpoch=g_armPoseEpoch;}
    }
    else p.rootBody=renderedBody;
   }
   else if(!PistolPoseOwned()&&!R700Hold()){
    Mat4 fixed124={};p.rootBody=ControllerMagazineRoot124(fixed124)?fixed124:renderedBody;
    if((!g_twoHand||!p.parentValid||p.parentEpoch!=g_armPoseEpoch)&&fc3pad::Latest().rightTrigger<.3f){
     // Learning from the native rendered root, never from an assisted root.
     Mat4 controller={},inverse={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
     if(Invert(controller,inverse)){p.controllerParent=Multiply(inverse,renderedBody);p.parentValid=true;p.parentEpoch=g_armPoseEpoch;}
    }
   }else p.rootBody=AkParentBody();
   {Mat4 fixed124={};if(!R700Hold()&&ControllerMagazineRoot124(fixed124))p.rootBody=fixed124;}
   for(int i=0;i<16;++i)g_akRootController.m[i]=g_handBody[0][i];g_akRootControllerEpoch=g_armPoseEpoch;
   p.rootFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.rootContext=context;
   if(!PistolPoseOwned()&&!R700Hold())RifleAttachLearnFromFrame(nativeWorld); // attachments drawn BEFORE this FRAME (R700 silencer): offset from this frame's native FRAME
   p.rootWorld=Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.rootBody));
   // View-space move taking the native FRAME to the corrected FRAME. Rigid attachments
   // (sights, scopes, suppressors) use it instead of the bare right-hand move.
   {const Mat4 view=HullView(camera);Mat4 nativeInverse={};
    if(Invert(Multiply(view,p.rootNative),nativeInverse)){g_rifleAttachMove=Multiply(Multiply(view,p.rootWorld),nativeInverse);g_rifleAttachFrame=g_frame;g_rifleAttachEpoch=g_armPoseEpoch;}}
   p.port=Apply(p.rootBody,{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]});p.portFrame=g_frame;
  }
  AkObserveParts(context,nativeWorld);
  if(!RefreshWeaponDrawRoot51(context))return;WritePistolWorld(globals,p.rootWorld);return;
 }
 if(!p.parentValid||p.parentEpoch!=g_armPoseEpoch)return;
 const Mat4 body=AkParentBody(),root=Multiply(viewInverse,Multiply(ArmBodyToCamera(),body));
 if(RifleClip(CurrentRifle(),start,count)){
  if(p.removalHeld){Mat4 palmInverse={};if(Invert(p.removalStartPalm,palmInverse)){
   const Mat4 clipBody=AkCarried316(p.palmBody,MagazineHeldFromHand());(void)palmInverse; // snaps into the left-hand grip on grab (no float)
   const Mat4 clipWorld=Multiply(viewInverse,Multiply(ArmBodyToCamera(),clipBody));
   WritePistolWorld(globals,clipWorld);p.clipWorld=clipWorld;p.clipFrame=g_frame;
  }return;}
  if(p.held||PistolAmmoPreviewActive())return;
  if(p.looseFlying){WritePistolWorld(globals,Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.looseBody)));return;}
  if(p.insertActive&&(p.pending||p.cycle||p.magazinePresent)){
   const Mat4 clip=Multiply(root,PistolInsertRelative(static_cast<double>(GetTickCount64()-p.insertAt)/1000));
   WritePistolWorld(globals,clip);p.clipWorld=clip;p.clipFrame=g_frame;return;
  }
  if(p.dropActive){const double age=static_cast<double>(GetTickCount64()-p.dropAt)/1000;
   if(age<.65){WritePistolWorld(globals,PistolDropWorld(p.dropWorld,age));return;}p.dropActive=false;
  }
  if(!p.magazinePresent||p.pending||p.cycle){globals[7]=-10000;return;}
  if(CurrentRifle().clipDRest&&RiflePart(CurrentRifle().clipD,start,count)){WritePistolWorld(globals,Multiply(root,*CurrentRifle().clipDRest));return;} // MKG belt: its own seated rest
  const Mat4 clip=Multiply(root,(*CurrentRifle().clipRest));WritePistolWorld(globals,clip);p.clipWorld=clip;p.clipFrame=g_frame;return;
 }
 // Charging handle and accessory motions remain native, relative to the
 // corrected FRAME. Never animate the whole rifle to simulate bolt travel.
 const auto& rifle=CurrentRifle();const int index=RiflePartIndex(start,count);if(index<0||!rifle.partRest[index])return;
 auto& part=g_akParts[index];
 if(part.epoch!=g_armPoseEpoch||part.profile!=rifle.profile)part=AkPartPose{};
 part.native=nativeWorld;part.frame=g_frame;part.epoch=g_armPoseEpoch;part.context=context;part.profile=rifle.profile;
 if(p.rootFrame==g_frame&&p.rootContext==context)AkObserveParts(context,p.rootNative);
 Mat4 relative=part.valid?part.relative:*rifle.partRest[index];
 {Mat4 bolt={};if(R700BoltPose(index,relative,bolt))relative=bolt;} // R700: Jason's cycle / native reload / at rest (never the game's automatic cycle)
 if(index==0&&(p.pending||p.cycle||p.needsRack||p.rackHeld||p.rackReturning)){
   // Charging handle on its FRAME-local -Y rail; the 5 cm physical stroke maps to the native travel.
   relative=*rifle.partRest[0];relative.m[7]=RifleChargeRestY();
   if(p.rackHeld||p.rackReturning)relative.m[7]-=p.slidePull*(rifle.chargeTravel/.05);
 }
 if(index==rifle.ringPart&&(p.pending||p.cycle||p.needsRack||p.rackHeld||p.rackReturning))relative=*rifle.partRest[index];
 if(index==1&&rifle.riderBack&&(p.rackHeld||p.rackReturning))relative=PistolRigidBlend(*rifle.partRest[1],*rifle.riderBack,p.slidePull/.05); // P416 latch rides the handle
 WritePistolWorld(globals,Multiply(root,relative));
}
// Left wrist on the charging handle while it is pulled (MP5 / F1): Jason's hand-on-handle pose slid along the rail by the
// pull (the stroke itself is read from the REAL controller, AdvancePistolRack).
bool RifleChargeHandHeld()
{
 const auto& p=g_pistolReload;
 // Spring return belongs to the mechanism, never to a hand whose grip was released.
 // A stale root or competing magazine action must not pin the tracked wrist either.
 if(!p.enabled||!MotionInteractionEnabled()||!g_handLeftValid||!g_handRightValid||
    !p.rackHeld||p.held||p.removalHeld||p.pending||p.rootEpoch!=g_armPoseEpoch||
    g_frame-p.rootFrame>1)return false;
 return (AkModel()&&(CurrentRifle().chargeHand||g_weaponMechanicProfile==7||g_weaponMechanicProfile==28))||D50Model()||g_weaponMechanicProfile==1||g_weaponMechanicProfile==20;
}
// SKINNED attachments (the holographic sight: vb 48520, draw 168/3030 carries a 2304-byte palette): BuildWeaponPalette moves
// every bone by the hand move, so during a reload (gun on the learned hold / Jason's insert path) the sight kept following the
// bare hand = "plays an animation". Its root this frame = toView x bone 0; its offset from the native FRAME is learned while
// plain holding, and while the pose is owned the bones are moved so bone 0 lands on this frame's corrected FRAME x offset.
bool RifleSkinnedAttachmentMove(ID3D11DeviceContext* context,UINT start,UINT count,const float* bones,const Mat4& toView,Mat4& move)
{
 (void)count;const bool d50=D50Model()&&PistolHandling();
 if(!AkWeaponVisualActive()&&!d50)return false;
 UINT vertexBytes=0,indexBytes=0;if(!AttachmentBuffers(context,vertexBytes,indexBytes)||!RifleAttachmentModel(vertexBytes,indexBytes))return false;
 if(!BoneUsable(bones))return false;
 auto& p=g_pistolReload;
 RifleAttachmentRel* slot=RifleAttachSlot(vertexBytes,start+0x40000000u);
 if(!slot)return false;
 CameraSummary camera={};if(!PistolGuideCamera(context,camera))return false;
 const Mat4 view=HullView(camera);Mat4 viewInverse={};if(!Invert(view,viewInverse))return false;
 const Mat4 nativeView=Multiply(toView,BoneMatrix(bones,0));
 slot->native=Multiply(viewInverse,nativeView);slot->nativeFrame=g_frame; // for the FRAME draw to learn from (drawn-before order)
 const bool rootFresh=p.rootFrame==g_frame&&p.rootEpoch==g_armPoseEpoch&&p.rootContext==context&&(!d50||p.rootNativeFrame==g_frame);
 if(rootFresh&&!PistolPoseOwned())
 {Mat4 inverse={};if(Invert(p.rootNative,inverse)){if(!slot->valid||slot->epoch!=g_armPoseEpoch)Log("[RIFLE ATTACH] skinned attachment vb %u start %u: offset from the native FRAME learned (bone 0)",vertexBytes,start);slot->rel=Multiply(inverse,slot->native);slot->epoch=g_armPoseEpoch;slot->valid=true;}}
 // Preserve the D50's separate native plain-hold rule. Rifles with a confirmed
 // current root instead share that root after later camera/aim changes.
 Mat4 fixed124={};if(!PistolPoseOwned()&&(d50||!rootFresh)&&!ControllerMagazineRoot124(fixed124))return false;
 if(!slot->valid||slot->epoch!=g_armPoseEpoch)return false;
 Mat4 rootWorld={};
 if(rootFresh){if(!RefreshWeaponDrawRoot51(context))return false;rootWorld=p.rootWorld;}
 else if(d50){if(!D50PredictRoot(context,0))return false;rootWorld=p.rootWorld;}
 else{if((!p.parentValid||p.parentEpoch!=g_armPoseEpoch)&&!ControllerMagazineRoot124(fixed124))return false;rootWorld=Multiply(viewInverse,Multiply(ArmBodyToCamera(),AkParentBody()));}
 Mat4 nativeInverse={};if(!Invert(nativeView,nativeInverse))return false;
 move=Multiply(Multiply(view,Multiply(rootWorld,slot->rel)),nativeInverse);return true;
}
// Revision 3 (the MP5 / F1 handle pull clipped and did not use the correct animation pose):
// the hand no longer slides a fixed pose along the rail; it plays Jason's OWN hand path from the
// captured native reload - the clip keys between the handle leaving rest and fully back - indexed by the pull fraction,
// wrist and fingers both. (F1: keys 2.818..2.954 s of the empty reload; MP5: 0..0.338 s, the native handle stroke.)
template<class K> void RifleSampleKeys(const K* keys,int count,double t,Mat4& hand,float* fingers)
{
 int i=0;while(i+1<count&&keys[i+1].t<=t)++i;const K& a=keys[i];const K& b=keys[(std::min)(i+1,count-1)];
 const double span=b.t-a.t,w=span>1e-6?(std::max)(0.0,(std::min)(1.0,(t-a.t)/span)):0;
 hand=PistolRigidBlend(RifleMat12(a.leftHand),RifleMat12(b.leftHand),w);
 for(int bone=0;bone<17;++bone){const Mat4 m=PistolRigidBlend(BoneMatrix(a.leftFingers,bone),BoneMatrix(b.leftFingers,bone),w);for(int j=0;j<12;++j)fingers[bone*12+j]=static_cast<float>(m.m[j]);}
}
bool RifleChargeClipSample(double fraction,Mat4& hand,float* fingers)
{
 fraction=(std::max)(0.0,(std::min)(1.0,fraction));
 switch(D50Model()?12:(g_weaponMechanicProfile==1||g_weaponMechanicProfile==20)?20:CurrentRifle().profile)
 {
  case 7:
   hand=kA2000ChargeWrist229;hand.m[7]-=fraction*kA2000ChargeTravel;
   memcpy(fingers,kA2000ChargingGripRelative,204*sizeof(float));return true;
  case 14:RifleSampleKeys(kFamasClipEmptyReload,kFamasClipEmptyReloadCount,kFamasEmptySlidePullStartAt+fraction*(kFamasEmptySlideBackAt-kFamasEmptySlidePullStartAt),hand,fingers);return true;
  case 9:RifleSampleKeys(kBz19ClipEmptyReload,kBz19ClipEmptyReloadCount,kBz19PullStart+fraction*(kBz19PullBack-kBz19PullStart),hand,fingers);return true;
  case 31:RifleSampleKeys(kP416ClipEmptyReload,kP416ClipEmptyReloadCount,kP416PullStart+fraction*(kP416PullBack-kP416PullStart),hand,fingers);return true;
  case 25:RifleSampleKeys(kMk16ClipEmptyReload,kMk16ClipEmptyReloadCount,kMk16EmptyChargePullStartAt+fraction*(kMk16EmptyChargeBackAt-kMk16EmptyChargePullStartAt),hand,fingers);return true;
  case 36:RifleSampleKeys(kU100ClipEmptyReload,kU100ClipEmptyReloadCount,kU100PullStart+fraction*(kU100PullBack-kU100PullStart),hand,fingers);return true;
  case 22:RifleSampleKeys(kMkgClipEmptyReload,kMkgClipEmptyReloadCount,kMkgPullStart+fraction*(kMkgPullBack-kMkgPullStart),hand,fingers);return true;
  case 17:RifleSampleKeys(kAceClipEmptyReload,kAceClipEmptyReloadCount,kAcePullStart+fraction*(kAcePullBack-kAcePullStart),hand,fingers);return true;
  case 35:RifleSampleKeys(kSvdClipEmptyReload,kSvdClipEmptyReloadCount,kSvdPullStart+fraction*(kSvdPullBack-kSvdPullStart),hand,fingers);return true;
  case 21:RifleSampleKeys(kVectorClipEmptyReload,kVectorClipEmptyReloadCount,kVectorEmptyChargePullStartAt+fraction*(kVectorEmptyChargeBackAt-kVectorEmptyChargePullStartAt),hand,fingers);return true;
  case 12:RifleSampleKeys(kD50ClipEmptyReload,kD50ClipEmptyReloadCount,kD50EmptySlidePullStartAt+fraction*(kD50EmptySlideBackAt-kD50EmptySlidePullStartAt),hand,fingers);return true;
  case 20:{
   // Reuse the verified native pistol slide-grasp animation on the1911 rail;
   // normalize its travel to the1911's5cm mechanical stop. Fingers rotate with
   // the sampled wrist instead of the free controller passing through the gun.
   Mat4 start={};float unused[204];RifleSampleKeys(kD50ClipEmptyReload,kD50ClipEmptyReloadCount,kD50EmptySlidePullStartAt,start,unused);
   RifleSampleKeys(kD50ClipEmptyReload,kD50ClipEmptyReloadCount,kD50EmptySlidePullStartAt+fraction*(kD50EmptySlideBackAt-kD50EmptySlidePullStartAt),hand,fingers);
   for(int axis=0;axis<3;++axis){const int i=axis*4+3;hand.m[i]=start.m[i]+(hand.m[i]-start.m[i])*(.05/kD50SlideTravel)+kPistolSlideRest.m[i]-kD50SlideRestM.m[i];}
   return true;
  }
  case 28:
  { // revision 3 (native MP5 hand path) was worse: the native MP5 hand never grips its handle. The hand now sits ON
    // the handle: Jason's F1 charging-handle grip (wrist and fingers relative to that handle) carried onto the MP5 handle,
    // riding its native path (6.6 cm back + the 50 degree swing).
   hand=Multiply(PistolRigidBlend(kMp5HandleRest,kMp5HandleBack,fraction),kMp5HandFromHandle);
   std::memcpy(fingers,kFamasLeftSlideFingers,sizeof(float)*204);return true;
  }
 }
 return false;
}
const float* RifleChargeFingers()
{
 static float fingers[204];Mat4 hand={};
 if(RifleChargeHandHeld()&&RifleChargeClipSample(g_pistolReload.slidePull/.05,hand,fingers))return fingers;
 return D50Model()||g_weaponMechanicProfile==1||g_weaponMechanicProfile==20?kD50SlideGripRelative:CurrentRifle().chargeGrip;
}
bool RifleChargeHandMove(const Mat4& toView,const float* bones,Mat4& move)
{
 if(!RifleChargeHandHeld())return false;const auto& p=g_pistolReload;
 Mat4 inverse={},hand={};float fingers[204];
 if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse)||!RifleChargeClipSample(p.slidePull/.05,hand,fingers))return false;
 Mat4 root=p.rootBody;if(g_weaponMechanicProfile==20&&!Resolve1911Insertion(p.rootBody,root,false))return false;
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(root,hand)),inverse);return true;
}
// Probe (F1: the silencer and the scope moved independently): for 10 s after a rifle with attachments appears, log
// every 20th attachment draw: which move it got and how far its final world is from the corrected FRAME x its native offset.
void RifleAttachmentProbe(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& nativeWorld,const float* globals,const Mat4& handMove)
{
 if(!AkModel())return;DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(!RifleAttachmentModel(d.vertexBytes,d.indexBytes))return;
 static LONG firstFrame=-1,lines=0;if(firstFrame<0)firstFrame=g_frame;if(g_frame-firstFrame>7200||lines>=160||(g_frame%45))return;
 const auto& p=g_pistolReload;Mat4 rootNativeInverse={};if(g_frame-p.rootFrame>1||!Invert(p.rootNative,rootNativeInverse))return;
 const Mat4 expected=Multiply(p.rootWorld,Multiply(rootNativeInverse,nativeWorld)),got=HullWorld(globals);
 double dist=0;for(int r=0;r<3;++r)dist+=(expected.m[r*4+3]-got.m[r*4+3])*(expected.m[r*4+3]-got.m[r*4+3]);
 Mat4 attach={};const bool viaAttach=RifleAttachmentMove(context,start,count,attach);(void)handMove;
 ++lines;Log("[RIFLE ATTACH] %ls vb %u start %u: %s move (attach age %ld, epoch %s); final vs FRAME-relative expected: %.1f mm; root %s, twoHand %d, poseOwned %d",
  CurrentRifle().name,d.vertexBytes,start,viaAttach?"attachment":"plain hand",g_frame-g_rifleAttachFrame,g_rifleAttachEpoch==g_armPoseEpoch?"ok":"STALE",std::sqrt(dist)*1000,
  g_frame-p.rootFrame<=1?"fresh":"old",g_twoHand?1:0,PistolPoseOwned()?1:0);
}
// Design: a two-handed grab must not remove the magazine; manual removal is LEFT grip + X.
// Removal by hand starts only on LEFT grip + X together (either order) with the left palm on the seated magazine; a plain LEFT
// grip there is the two-handed hold. (X with LEFT grip never reaches the game on magazine weapons: vr_pad g_magazineHandling.)
void TickAkRemoval(const fc3pad::Controllers& c)
{
 if(!AkModel()&&!SkorpionModel())return;auto& p=g_pistolReload;
 const bool grip=c.leftGrip>.7f,chord=grip&&(c.x||fc3pad::GripOnlyAmmoEnabled());
 const bool edge=chord&&!p.removalGripWait;p.removalGripWait=fc3pad::GripOnlyAmmoEnabled()?(grip||(c.leftGrip>=.35f&&p.removalGripWait)):chord;
 if(p.removalHeld){
  if(c.leftGrip<.35f){
   if(p.removalDetached){p.dropActive=true;p.dropWorld=p.clipWorld;p.dropAt=GetTickCount64();p.insertActive=false;}
   p.removalHeld=p.removalDetached=false;p.waitTriggerRelease=c.rightTrigger>.3f;
   Log("[AK MAGAZINE] removal released; seated=%d; no native ammo write",p.magazinePresent);return;
  }
  if(g_frame-p.palmFrame>1||g_frame-p.rootFrame>1)return;
  Mat4 inverse={};if(!Invert(p.rootBody,inverse))return;const V3 local=Apply(inverse,p.tip);
  if(!p.removalDetached&&Length(Sub(local,p.removalStart))>=.05&&local.z<=p.removalStart.z-.025){
   p.removalDetached=true;p.magazinePresent=false;p.insertActive=false;fc3xr::Buzz(1);
   Log("[AK MAGAZINE] physical pull detached old magazine; release grip to drop; native count unchanged");
  }
  return;
 }
 if(!edge||!MagazineRemovalAllowed54(GetTickCount64())||!p.magazinePresent||p.held||p.looseFlying||p.pending||p.cycle||p.needsRack||p.rackMode||p.rackHeld||p.rackReturning||g_pistolMenu.open||
    g_frame-p.rootFrame>1||g_frame-p.palmFrame>1||p.rootEpoch!=g_armPoseEpoch||!PistolGuideContact(p.rootBody,SkorpionModel()?0:1,p.tip))return;
 Mat4 inverse={};if(!Invert(p.rootBody,inverse))return;
 p.removalStart=Apply(inverse,p.tip);p.removalStartPalm=p.palmBody;p.removalClip=Multiply(p.rootBody,SkorpionModel()?kSkorpionClipRest:*CurrentRifle().clipRest);
 p.removalHeld=true;p.removalDetached=false;g_twoHand=false;fc3xr::Buzz(1);
 Log("[AK MAGAZINE] LEFT grip + X on the seated magazine: grabbed; pull down 5 cm, release to drop");
}
