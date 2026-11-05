void TestTriggerParenting41(){
 const auto clip=g_gm94Clip;const auto free=g_freeHandAttachment[1];const auto delta=g_supportPalmDelta;
 const LONG deltaFrame=g_supportPalmFrame,profile=g_weaponMechanicProfile;const auto gunMove=g_frameGunMove;
 const LONG supportProfile=g_supportPalmProfile;const UINT supportEpoch=g_supportPalmEpoch;
 const auto lock=g_gm94Lock;const bool two=g_twoHand,aim=g_aimActive,lv=g_handLeftValid,rv=g_handRightValid;
 const auto rightFree=g_freeRightFromController;const UINT rightEpoch=g_freeRightEpoch;
 float hands[2][16];memcpy(hands,g_handBody,sizeof(hands));
 Mat4 base=HandIdentity();base.m[3]=.2;base.m[7]=-.1;base.m[11]=.5;
 const V3 start=Apply(base,{.1,.2,-.3});Check(Gm94BeginCoverMotion(base,start),"cover grab has invertible parent");
 for(double scale:{.5,1.,2.})for(int i=0;i<36;++i){
  Mat4 parent=TurnAndMove({0,0,1},{sin(i*.15),0,cos(i*.15)},{0,0,0},{i*.01,-i*.02,.3});
  // Nonuniform scale is not needed for native rigid attachment; test world scaling.
  for(int r=0;r<3;++r)for(int c=0;c<3;++c)parent.m[r*4+c]*=scale;
  const Mat4 root=Multiply(parent,base);const V3 moved=Apply(parent,start);
  Check(std::fabs(Gm94CoverTravel(root,moved))<1e-8,"rotating/translating gun and hand together cannot operate cover");
  const V3 lifted=Apply(parent,Add(start,{0,.04,0}));
  Check(std::fabs(Gm94CoverTravel(root,lifted)-.04)<1e-8,"gun-local relative lift preserves half opening under parent movement");
 }
 g_handLeftValid=g_handRightValid=true;g_aimActive=false;g_twoHand=false;g_frameGunMove=HandIdentity();
 g_freeRightFromController=HandIdentity();g_freeRightEpoch=g_armPoseEpoch;
 for(int i=0;i<16;++i)g_handBody[0][i]=i%5==0?1.f:0.f;
 Mat4 before={},after={};Check(Gm94LockedBody(before),"launcher controller root available");
 g_handBody[0][3]+=.2f;Check(Gm94LockedBody(after)&&std::fabs(after.m[3]-before.m[3]-.2)<1e-6,"same-frame controller movement cannot reuse stale launcher root");
 g_freeHandAttachment[1].relative=HandIdentity();g_freeHandAttachment[1].epoch=g_armPoseEpoch;
 g_freeHandAttachment[1].frame=g_frame;g_freeHandAttachment[1].profile=g_weaponMechanicProfile;
 g_supportPalmDelta={.04,.03,-.02};g_supportPalmFrame=g_frame;
 g_supportPalmProfile=g_weaponMechanicProfile;g_supportPalmEpoch=g_armPoseEpoch;
 const V3 support={.2,.1,.6};
 for(int i=0;i<36;++i){const Mat4 c=TurnAndMove({0,0,1},{sin(i*.15),0,cos(i*.15)},{0,0,0},{-.2,.1,.3});
  for(int k=0;k<16;++k)g_handBody[1][k]=float(c.m[k]);V3 point={},palm={};
  Check(GrabGizmoPoints(support,point,palm),"production support query has calibrated palm");
  Check(Length(Sub(point,Add(support,g_supportPalmDelta)))<1e-8,"free-hand rotation cannot drag weapon contact target");
  Check(Length(Sub(palm,Apply(c,{-.6781,.0519,1.0387})))<1e-6,"hand sphere follows palm bind point rather than wrist");
 }
 const auto oldEpoch=g_freeHandAttachment[1].epoch;g_freeHandAttachment[1].epoch=~g_armPoseEpoch;V3 p={};Check(!FreeHandPalm(1,p),"previous pose epoch cannot authorize current palm");g_freeHandAttachment[1].epoch=oldEpoch;
 // A mirrored native box is geometrically identical after reversing one local
 // axis. General asymmetric transforms retain strict reflection rejection.
 float mirrored[16]={-1,0,0,.2f,0,1,0,.3f,0,0,1,.4f,0,0,0,1};XrPosef pose={};
 Check(!fc3xr::BodyToReferencePose(mirrored,1,0,{},pose),"general reflected poses remain rejected");
 Check(fc3xr::BodyBoxToReferencePose(mirrored,1,0,{},pose),"mirrored symmetric contact box has valid XR orientation");
 Check(std::fabs(pose.position.x-.2f)<1e-6&&std::fabs(pose.position.y-.3f)<1e-6&&std::fabs(pose.position.z+.4f)<1e-6,"box reflection correction preserves contact center");
 const bool diagnostics=g_weaponTriggerDiagnostics;const int target=g_weaponTriggerTarget;
 const auto reload=g_pistolReload;const auto preview=g_pistolGuidePreview;const bool menu=g_pistolMenu.open;
 g_pistolMenu.open=false;g_pistolReload.rootBody=HandIdentity();g_pistolReload.rootEpoch=g_armPoseEpoch;g_pistolReload.rootFrame=g_frame;
 g_pistolGuidePreview.epoch=g_armPoseEpoch;g_pistolGuidePreview.rootFrame=g_frame;g_pistolGuidePreview.root=HandIdentity();g_pistolGuidePreview.root.m[3]=2;
 g_pistolGuidePreview.profile=g_weaponMechanicProfile;
 g_pistolGuidePreview.physicalRoot=g_pistolReload.rootBody;g_pistolGuidePreview.physicalKnown=true;
 const Mat4 contactRoot=PistolInteractionRoot(g_pistolReload.rootBody);Check(contactRoot.m[3]==2,"completed draw is shared by physical contact and guide root");
 const float* at=PistolGuidePoint(0);const V3 actual=Apply(contactRoot,{at[0],at[1],at[2]});
 Check(PistolGuideContact(g_pistolReload.rootBody,0,actual),"physical query hits final displayed insertion anchor");
 Check(!PistolGuideContact(g_pistolReload.rootBody,0,{at[0],at[1],at[2]}),"old independently stored contact position is no longer actionable");
 g_pistolGuidePreview.rootFrame=g_frame-1;Check(PistolInteractionRoot(g_pistolReload.rootBody).m[3]==0,"stale rendered root cannot move contact zone");
 g_pistolGuidePreview.rootFrame=g_frame;g_pistolReload.rootBody.m[7]=.5;Check(PistolInteractionRoot(g_pistolReload.rootBody).m[3]==0,"same-frame root replacement invalidates previous draw ownership");g_pistolReload.rootBody.m[7]=0;
 g_pistolGuidePreview.rootFrame=g_frame;g_pistolMenu.open=true;Check(PistolInteractionRoot(g_pistolReload.rootBody).m[3]==0,"Insert calibration observation cannot authorize weapon contact");
 g_pistolReload=reload;g_pistolGuidePreview=preview;g_pistolMenu.open=menu;
 g_weaponTriggerDiagnostics=true;for(int i=0;i<3;++i){g_weaponTriggerTarget=i;Check(PistolGameplayGuideTarget()==(i==2?3:i),"diagnostic target is visible independent of reload state");}
 g_weaponTriggerDiagnostics=diagnostics;g_weaponTriggerTarget=target;
 g_gm94Clip=clip;g_freeHandAttachment[1]=free;g_supportPalmDelta=delta;g_supportPalmFrame=deltaFrame;g_weaponMechanicProfile=profile;
 g_supportPalmProfile=supportProfile;g_supportPalmEpoch=supportEpoch;
 g_frameGunMove=gunMove;g_gm94Lock=lock;g_twoHand=two;g_aimActive=aim;g_handLeftValid=lv;g_handRightValid=rv;memcpy(g_handBody,hands,sizeof(hands));
 g_freeRightFromController=rightFree;g_freeRightEpoch=rightEpoch;
 puts("PASS: trigger41 rotated/scaled cover-parent motion, palm attachment and fixed weapon target, stale-pose rejection, mirrored box and idle diagnostic selection");
}
