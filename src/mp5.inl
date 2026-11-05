// MP5 on the rifle (AK / A2000) physical-reload path, but SKINNED: its three draws (index start 0, 4149, 12234; vb 306360,
// stride 40) share one world matrix and a bone palette; FRAME = world x palette[ANCHOR]. The rigid-draw code (AkWorld /
// AkFinalWorld) never sees it: BuildWeaponPalette hands the raw palette here. Every slot is conjugated onto the corrected
// FRAME (the same root AkFinalWorld computes: controller attachment, two-hand turn), then the magazine slot follows the
// reload state exactly like the rigid clip draw (hidden / in the left hand / Jason's seating path / dropped / at rest) and
// the charging handle + bolt slots ride the physical pull after an EMPTY reload (kMp5HandleRest -> kMp5HandleBack).
// Palette convention (work/mp5_clips.py): a slot is identity at rest; a part at FRAME-relative pose X (X = its bind at rest)
// needs palette[slot] = FRAME_model x X x inv(T_bind).
struct Mp5Slots{int anchor,clip,handle,bolt;};
// The raw FRAME palette of the last anchored skinned draw this frame: draws without a FRAME slot (the PKM belt) use it - every draw
// of the gun shares one world. (An earlier build anchored the belt on its own first round: the belt sat frozen where it never reached the gun.)
Mat4 g_mp5RawFrame={};LONG g_mp5RawFrameFrame=-1;
bool Mp5DrawSlots(UINT start,UINT count,Mp5Slots& s)
{
 if(g_weaponMechanicProfile==25)
 { // MK16 (work/mk16_clips.py): the BULLET draw has no ANCHOR; BULLET is CLIP's child with CLIP's skin matrix, and the native
   // clip never moves in VR (the reload is instant), so that slot is its own anchor and follows the clip.
  if(start==0&&count==5412){s={2,0,-1,-1};return true;}
  if(start==5412&&count==2076){s={0,-1,-1,-1};return true;}
  if(start==7488&&count==2808){s={0,-1,-1,-1};return true;}
  if(start==10296&&count==732){s={0,0,-1,-1};return true;}
  if(start==11028&&count==13881){s={3,-1,0,1};return true;}
  return false;
 }
 if(g_weaponMechanicProfile==43)
 { // PKM (work/pkm-report.md): ANCHOR slot 0 in 0/9801, 9801/840 and 14397/23700; BOX = slot 0 of 10641 and 11661, slot 7 of 14397;
   // the belt draw 38397 has no anchor (slot 0 = round 1, its own anchor like the MK16 bullet draw). Lid / rounds follow the box: pkm.inl.
  if(start==0&&count==9801){s={0,-1,-1,-1};return true;}
  if(start==9801&&count==840){s={0,-1,-1,-1};return true;}
  if(start==10641&&count==1020){s={0,0,-1,-1};return true;}
  if(start==11661&&count==2736){s={0,0,-1,-1};return true;}
  if(start==14397&&count==23700){s={0,7,-1,-1};return true;}
  if(start==38397&&count==3528){s={-1,-1,-1,-1};return true;}
  if(start==38097&&count==300){s={-1,-1,-1,-1};return true;} // cover label (slots Top/LeftLid; 544-byte $Globals) - was unmapped: drifted off the gun in the game's reload (the 3D text did not follow the gun) // no FRAME slot: the gun frame of this frame's anchored draws (same world)
  return false;
 }
 if(g_weaponMechanicProfile==21)
 { // KSV Vector (work/vector_clips.py): ANCHOR slot 0 in every draw; CLIP slot 1 of draw 0; SLIDE_HANDLE slot 4 / SLIDE slot 3 of draw 3615.
  if(start==0&&count==3615){s={0,1,-1,-1};return true;}
  if(start==3615&&count==10623){s={0,-1,4,3};return true;}
  if(start==14238&&count==3906){s={0,-1,-1,-1};return true;}
  if(start==18144&&count==5091){s={0,-1,-1,-1};return true;}
  return false;
 }
 if(start==0&&count==4149){s={5,-1,2,1};return true;}
 if(start==4149&&count==8085){s={0,-1,-1,-1};return true;}
 if(start==12234&&count==15534){s={4,1,2,-1};return true;}
 return false;
}
struct SkinnedParts{const Mat4 *clipRest,*handleRest,*handleBack,*boltRest,*boltBack;};
SkinnedParts CurrentSkinnedParts()
{
 if(g_weaponMechanicProfile==25)return {&kMk16ClipRest,&kMk16HandleRest,&kMk16HandleBack,&kMk16BoltRest,&kMk16BoltBack};
 if(g_weaponMechanicProfile==43)return {&kPkmClipRest,&kPkmSlideRest,&kPkmSlideBack,&kPkmSlideRest,&kPkmSlideBack};
 if(g_weaponMechanicProfile==21)return {&kVectorClipRest,&kVectorHandleRest,&kVectorHandleBack,&kVectorBoltRest,&kVectorBoltBack};
 return {&kMp5ClipRest,&kMp5HandleRest,&kMp5HandleBack,&kMp5BoltRest,&kMp5BoltBack};
}
Mat4 Mp5BindInverse(const Mat4& rest){Mat4 m=rest;m.m[3]=-rest.m[3];m.m[7]=-rest.m[7];m.m[11]=-rest.m[11];for(int r=0;r<3;++r)for(int c=0;c<3;++c)m.m[r*4+c]=r==c?1:0;return m;}
void ObserveMp5GuideRoot(const Mat4& rootBody);bool PkmNativeActive();void PkmPostSlots(float*,UINT,UINT,const Mat4&);
// Pure slot placement (tests drive it directly). bones = raw palette (model space); frameModel = corrected FRAME in model
// space; view = camera view (world -> view); fromView = view -> model.
void Mp5PlaceSlots(float* bones,const Mp5Slots& s,const Mat4& frameModel,const Mat4& fromView,const Mat4& view)
{
 auto& p=g_pistolReload;
 const Mat4 rawFrame=s.anchor>=0?BoneMatrix(bones,s.anchor):g_mp5RawFrame;Mat4 rawFrameInverse={};if(!Invert(rawFrame,rawFrameInverse))return;
 float raw[48*12];std::memcpy(raw,bones,sizeof(raw));
 for(int b=0;b<48;++b)StoreBone(bones,b,Multiply(frameModel,Multiply(rawFrameInverse,BoneMatrix(raw,b))));
 Mat4 viewInverse={};if(!Invert(view,viewInverse))return;
 const Mat4 rootWorld=Multiply(viewInverse,Multiply(view,Multiply(ArmBodyToCamera(),p.rootBody)));(void)rootWorld;
 const SkinnedParts parts=CurrentSkinnedParts();
 if(s.clip>=0)
 {
  const Mat4 clipBind=Mp5BindInverse(*parts.clipRest);Mat4 clip={};bool set=false,hidden=false;
  auto place=[&](const Mat4& clipModel){clip=Multiply(clipModel,clipBind);set=true;};
  auto physical=[&](const Mat4& model){place(CanonicalPhysical338(model,frameModel));};
  if(p.removalHeld){Mat4 palmInverse={};if(Invert(p.removalStartPalm,palmInverse)){
    const Mat4 clipBody=AkCarried316(p.palmBody,MagazineHeldFromHand());(void)palmInverse; // snaps into the left-hand grip on grab (no float)
    physical(Multiply(fromView,Multiply(ArmBodyToCamera(),clipBody)));p.clipWorld=Multiply(viewInverse,Multiply(ArmBodyToCamera(),clipBody));p.clipFrame=g_frame;}}
  else if(PistolAmmoPreviewActive()&&AmmoPreviewPoseAvailable())physical(Multiply(fromView,Multiply(ArmBodyToCamera(),Multiply(g_ammoPreviewPoses[g_menuGuideFamily].palm,MagazineHeldFromHand()))));
  else if(p.held){if(g_frame-p.handFrame<=1)physical(Multiply(fromView,p.hand));else hidden=true;}
  else if(p.looseFlying)physical(Multiply(fromView,Multiply(ArmBodyToCamera(),p.looseBody)));
  else if(p.insertActive&&(p.pending||p.cycle||p.magazinePresent)){const Mat4 rel=PistolInsertRelative(static_cast<double>(GetTickCount64()-p.insertAt)/1000);
    place(Multiply(frameModel,rel));p.clipWorld=Multiply(p.rootWorld,rel);p.clipFrame=g_frame;}
  else if(p.dropActive){const double age=static_cast<double>(GetTickCount64()-p.dropAt)/1000;
    if(age<.65)physical(Multiply(fromView,Multiply(view,PistolDropWorld(p.dropWorld,age))));else p.dropActive=false;}
  if(!set&&!hidden){if(!p.magazinePresent||p.pending||p.cycle)hidden=true;else{place(Multiply(frameModel,*parts.clipRest));p.clipWorld=Multiply(p.rootWorld,*parts.clipRest);p.clipFrame=g_frame;}}
  if(hidden)bones[s.clip*12+7]=-100;else if(set)StoreBone(bones,s.clip,clip);
 }
 if(p.pending||p.cycle||p.needsRack||p.rackHeld||p.rackReturning)
 { // charging handle on its native path: rest -> back by the physical pull (5 cm of hand = the native stroke)
  const double t=(p.rackHeld||p.rackReturning)?p.slidePull/.05:0;
  if(s.handle>=0)StoreBone(bones,s.handle,Multiply(frameModel,Multiply(PistolRigidBlend(*parts.handleRest,*parts.handleBack,t),Mp5BindInverse(*parts.handleRest))));
  if(s.bolt>=0)StoreBone(bones,s.bolt,Multiply(frameModel,Multiply(PistolRigidBlend(*parts.boltRest,*parts.boltBack,t),Mp5BindInverse(*parts.boltRest))));
 }
}
// BuildWeaponPalette seam, before the generic hand move. Returns true when it owned the palette.
bool Mp5Palette(ID3D11DeviceContext* context,float* bones,const Mat4& toView,const Mat4& fromView,const Mat4& handMove,UINT start,UINT count)
{
 if(!AkModel()||!CurrentRifle().skinned)return false;
 Mp5Slots s={};if(!Mp5DrawSlots(start,count,s))return false;
 DrawCall call={};call.a=count;call.b=start;if(!RifleMain(CurrentRifle(),NativeWeaponGeometry(context,call)))return false;
 if(!PistolHandling()&&!PistolAmmoPreviewActive()&&!AkWeaponVisualActive())return false;
 auto& p=g_pistolReload;CameraSummary camera={};if(!PistolGuideCamera(context,camera))return false;
 const Mat4 view=HullView(camera);Mat4 viewInverse={},bodyInverse={};
 if(!Invert(view,viewInverse)||!Invert(ArmBodyToCamera(),bodyInverse))return false;
 if(s.anchor>=0){g_mp5RawFrame=BoneMatrix(bones,s.anchor);g_mp5RawFrameFrame=g_frame;}else if(g_mp5RawFrameFrame!=g_frame)return false;
 const Mat4 rawFrameView=Multiply(toView,g_mp5RawFrame);
 { // right-hand basis (AkWorld's FRAME branch)
  Mat4 basis={};if(HandBasis(Multiply(rawFrameView,*CurrentRifle().hand),basis)){p.current=basis;p.basisFrame=g_frame;if(!PistolPoseOwned()&&fc3pad::Latest().rightTrigger<.3f){p.idle=basis;p.basis=true;}}
 }
 if(p.rootFrame!=g_frame||p.rootContext!=context||p.rootEpoch!=g_armPoseEpoch)
 { // corrected FRAME, once per frame (AkFinalWorld's FRAME branch)
  p.rootNative=Multiply(viewInverse,rawFrameView);
  const Mat4 renderedBody=Multiply(bodyInverse,Multiply(handMove,rawFrameView));
  Mat4 pkmRoot={};
  if(PkmNativeActive()&&p.parentValid&&p.parentEpoch==g_armPoseEpoch&&g_handRightValid)
  { // PKM: the game's reload moves its right hand off the grip (bolt pull); the gun stays on the right controller
   Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];p.rootBody=Multiply(controller,p.controllerParent);
  }
  // The skinned FRAME must use the SAME controller parent as the firing hand
  // and shot axis. Previously PistolParentBody's fixed result was discarded,
  // leaving the actual mesh on renderedBody while the hand/aim used fixed124.
  else if(!PistolPoseOwned())p.rootBody=PistolParentBody(renderedBody);
  else p.rootBody=AkParentBody();(void)pkmRoot;
  for(int i=0;i<16;++i)g_akRootController.m[i]=g_handBody[0][i];g_akRootControllerEpoch=g_armPoseEpoch;
  p.rootFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.rootContext=context;
  p.rootWorld=Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.rootBody));
  {Mat4 nativeInverse={};if(Invert(rawFrameView,nativeInverse)){g_rifleAttachMove=Multiply(Multiply(ArmBodyToCamera(),p.rootBody),nativeInverse);g_rifleAttachFrame=g_frame;g_rifleAttachEpoch=g_armPoseEpoch;}}
  p.port=Apply(p.rootBody,{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]});p.portFrame=g_frame;
  ObserveMp5GuideRoot(p.rootBody);
  static LONG logged=0;if(InterlockedIncrement(&logged)==1)Log("[MP5] skinned rifle: FRAME from palette slot %d of draw %u/%u; magazine / handle / bolt are palette slots",s.anchor,start,count);
 }
 const Mat4 frameModel=Multiply(fromView,Multiply(ArmBodyToCamera(),p.rootBody));
 Mp5PlaceSlots(bones,s,frameModel,fromView,view);
 PkmPostSlots(bones,start,count,frameModel); // PKM: lid + belt rounds follow the box; seated early in the game's reload
 const BOOL reflected338=MirrorSkinned338(bones,frameModel)?TRUE:FALSE;g_weaponPaletteCb->SetPrivateData(kA2000Mirror337,sizeof(reflected338),&reflected338);
 return true;
}
