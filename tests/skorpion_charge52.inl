void TestSkorpionCharge52(){
 const LONG profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame,frame=g_frame;const auto controls=fc3pad::Latest();const auto menu=g_pistolMenu;const auto preview=g_pistolGuidePreview;const bool left=g_handLeftValid,right=g_handRightValid,motion=g_motionGun,guard=g_weaponMotionGuard;float hand[16];memcpy(hand,g_handBody[1],sizeof(hand));
 g_weaponMechanicProfile=39;g_weaponMechanicFrame=++g_frame;const auto prior=g_pistolReload;g_pistolReload=PistolReloadState{};auto& p=g_pistolReload;p.enabled=true;p.rootBody=HandIdentity();p.rootFrame=p.palmFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.magazinePresent=p.needsRack=true;p.chamberReady=false;g_pistolMenu.open=false;g_pistolGuidePreview={};g_handLeftValid=g_handRightValid=g_motionGun=true;g_weaponMotionGuard=false;
 p.tip=PistolRackGuide();for(int i=0;i<16;++i)g_handBody[1][i]=i%5==0?1.f:0.f;auto c=controls;c.active=true;c.x=false;c.leftGrip=0;c.rightTrigger=0;fc3pad::Publish(c);AdvancePistolRack(c,1000);c.leftGrip=1;AdvancePistolRack(c,1010);Check(p.rackHeld,"Skorpion grip-only acquisition preserved");
 float bones[576]={};StoreBone(bones,2,HandIdentity());Mat4 move={};Check(SkorpionChargeHandMove52(HandIdentity(),bones,move),"Skorpion charging wrist follows native pose while held");const Mat4 first=move;p.tip=Apply(Multiply(p.rootBody,kSkorpionChargingWristFromFrame52),{-.591,.011,1.149});g_handBody[1][7]-=.045f;AdvancePistolRack(c,1030);Check(p.rackRear&&p.slidePull>.04,"Skorpion stroke reads raw controller despite pinned animated hand");Check(SkorpionChargeHandMove52(HandIdentity(),bones,move),"charging wrist pose retained through full pull");Mat4 inverse={};Invert(ArmBodyToCamera(),inverse);const Mat4 a=Multiply(inverse,first),b=Multiply(inverse,move);Check(fabs(a.m[7]-b.m[7]-.045)<1e-5,"native grip translates along same charging-handle rail");
 c.leftGrip=0;AdvancePistolRack(c,1040);Check(!SkorpionChargeHandMove52(HandIdentity(),bones,move),"release restores controller-driven wrist immediately");AdvancePistolRack(c,1140);Check(!p.needsRack&&p.chamberReady,"Skorpion rear/release/return chambering unchanged");p.rackHeld=true;p.held=true;Check(!SkorpionChargeHandMove52(HandIdentity(),bones,move),"magazine grip retains wrist priority");p.held=false;g_handLeftValid=false;Check(!SkorpionChargeHandMove52(HandIdentity(),bones,move),"missing tracking cannot pin charging wrist");
 g_pistolReload=prior;g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_frame=frame;g_pistolMenu=menu;g_pistolGuidePreview=preview;g_handLeftValid=left;g_handRightValid=right;g_motionGun=motion;g_weaponMotionGuard=guard;memcpy(g_handBody[1],hand,sizeof(hand));fc3pad::Publish(controls);
 {
 const auto savedProfile=g_weaponMechanicProfile,savedMechanic=g_weaponMechanicFrame;g_weaponMechanicProfile=7;g_weaponMechanicFrame=g_frame;
 const auto savedState=g_pistolReload;auto& a=g_pistolReload;a=PistolReloadState{};a.enabled=true;a.rackHeld=true;a.rootFrame=g_frame;a.rootEpoch=g_armPoseEpoch;a.rackWristValid167=true;a.rackWrist167=HandIdentity();
 const auto savedMotion=g_motionGun;const auto savedGuard=g_weaponMotionGuard;const bool savedLeft=g_handLeftValid,savedRight=g_handRightValid;g_handLeftValid=g_handRightValid=true;g_motionGun=true;g_weaponMotionGuard=false;
 a.rackWrist167.m[3]=kA2000ChargeDefault[0]+.591;a.rackWrist167.m[7]=kA2000ChargeDefault[1]-.011;a.rackWrist167.m[11]=kA2000ChargeDefault[2]-1.149;
 Mat4 atRest={},atRear={},pastRear={};float fingers167[204];
 Check(RifleChargeHandHeld()&&RifleChargeClipSample(0,atRest,fingers167),"167 A2000 has a bounded wrist while handle held");
 Check(!memcmp(&atRest,&kA2000ChargeWrist229,sizeof(Mat4)),"229 A2000 uses captured knob-local grip independently of contact-box edits");
 Check(RifleChargeClipSample(1,atRear,fingers167)&&RifleChargeClipSample(5,pastRear,fingers167),"167 A2000 full and excess strokes sampled");
 Check(fabs(atRest.m[7]-atRear.m[7]-kA2000ChargeTravel)<1e-6&&!memcmp(&atRear,&pastRear,sizeof(Mat4)),"167 wrist uses exact native stroke and stops beyond rear limit");
 a.rackHeld=false;a.rackReturning=true;Check(!RifleChargeHandHeld(),"167 A2000 releases visible wrist immediately with grip release");
 g_pistolReload=savedState;g_weaponMechanicProfile=savedProfile;g_weaponMechanicFrame=savedMechanic;g_motionGun=savedMotion;g_weaponMotionGuard=savedGuard;g_handLeftValid=savedLeft;g_handRightValid=savedRight;
 }
 {
 const LONG savedProfile=g_weaponMechanicProfile,savedMechanic=g_weaponMechanicFrame;
 const bool savedLeft=g_handLeftValid,savedRight=g_handRightValid,savedMotion=g_motionGun,savedGuard=g_weaponMotionGuard;
 g_handLeftValid=g_handRightValid=g_motionGun=true;g_weaponMotionGuard=false;
 const LONG profiles[]={9,14,21,12}; // accepted BZ19/Vector paths plus F1 and D50
 for(const LONG profile169:profiles){
  g_weaponMechanicProfile=profile169;g_weaponMechanicFrame=g_frame;
  const auto saved=g_pistolReload;auto& q=g_pistolReload;q=PistolReloadState{};
  q.enabled=q.rackHeld=true;q.rootFrame=g_frame;q.rootEpoch=g_armPoseEpoch;
  Check(RifleChargeHandHeld(),"169 valid held charging wrist remains attached");
  Mat4 rear={},pastRear={};float rearFingers[204],pastFingers[204];
  Check(RifleChargeClipSample(1,rear,rearFingers)&&RifleChargeClipSample(5,pastRear,pastFingers)&&!memcmp(&rear,&pastRear,sizeof rear)&&!memcmp(rearFingers,pastFingers,sizeof rearFingers),"169 held wrist and fingers retain bounded rear endpoint");
  q.rackHeld=false;q.rackReturning=true;Check(!RifleChargeHandHeld(),"169 grip release frees wrist while mechanism returns");q.rackHeld=true;q.rackReturning=false;
  q.held=true;Check(!RifleChargeHandHeld(),"169 magazine hold owns wrist");q.held=false;
  q.removalHeld=true;Check(!RifleChargeHandHeld(),"169 magazine removal owns wrist");q.removalHeld=false;
  q.pending=true;Check(!RifleChargeHandHeld(),"169 pending insertion cannot pin wrist");q.pending=false;
  g_handLeftValid=false;Check(!RifleChargeHandHeld(),"169 missing left tracking releases wrist");g_handLeftValid=true;
  g_handRightValid=false;Check(!RifleChargeHandHeld(),"169 missing weapon tracking releases wrist");g_handRightValid=true;
  q.rootEpoch=g_armPoseEpoch+1;Check(!RifleChargeHandHeld(),"169 stale pose epoch releases wrist");q.rootEpoch=g_armPoseEpoch;
  q.rootFrame=g_frame-2;Check(!RifleChargeHandHeld(),"169 stale root releases wrist");q.rootFrame=g_frame;
  q.enabled=false;Check(!RifleChargeHandHeld(),"169 disabled handling releases wrist");q.enabled=true;
  g_weaponMotionGuard=true;Check(!RifleChargeHandHeld(),"169 native action guard releases wrist");g_weaponMotionGuard=false;
  g_pistolReload=saved;
 }
 g_weaponMechanicProfile=savedProfile;g_weaponMechanicFrame=savedMechanic;
 g_handLeftValid=savedLeft;g_handRightValid=savedRight;g_motionGun=savedMotion;g_weaponMotionGuard=savedGuard;
 }
 puts("PASS: charge52 Skorpion native wrist/finger grip with raw-controller stroke, rail motion, release, chambering and ownership guards");
}
