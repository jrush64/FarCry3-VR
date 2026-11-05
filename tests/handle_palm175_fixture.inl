void TestHandlePalm175()
{
 PistolReloadState saved[20];memcpy(saved,g_magazineStates,sizeof(saved));PistolGuideSettings guides[20];memcpy(guides,g_magazineGuides,sizeof(guides));const int loadFamily=g_guideLoadFamily;g_guideLoadFamily=-1;const auto menu=g_pistolMenu;const auto preview=g_pistolGuidePreview;
 const LONG profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame;
 const bool left=g_handLeftValid,right=g_handRightValid,motion=g_motionGun,guard=g_weaponMotionGuard;
 const auto controls=fc3pad::Latest();auto c=controls;c.active=true;fc3pad::Publish(c);
 g_handLeftValid=g_handRightValid=g_motionGun=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_pistolGuidePreview={};
 g_weaponMechanicFrame=g_frame;
 // Exact saved LIVE174 contacts, deliberately different from old hardcoded anchors.
 const float charge[3]={-.0318f,.1339f,.0887f},slide[3]={-.022f,.2162f,.0369f};
 float bones[576]={};StoreBone(bones,2,HandIdentity());
 for(int turn=0;turn<3;++turn){
  const double angle=turn*.83;
  for(int weapon=0;weapon<2;++weapon){
   g_weaponMechanicProfile=weapon?39:7;const float* anchor=weapon?slide:charge;const double travel=weapon?.05:kA2000ChargeTravel;
   auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=true;p.rackHeld=true;p.rootFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.rackWristValid167=true;p.rackWrist167=HandIdentity();
   memcpy(g_pistolGuides.charge,charge,12);memcpy(g_pistolGuides.slide,slide,12);
   p.rootBody=HandIdentity();p.rootBody.m[0]=p.rootBody.m[5]=cos(angle);p.rootBody.m[1]=-sin(angle);p.rootBody.m[4]=sin(angle);p.rootBody.m[3]=.3;p.rootBody.m[7]=-.2;p.rootBody.m[11]=.7;
   for(double fraction:{0.,.5,1.,2.}){
    p.slidePull=fraction*.05;Mat4 move={};
    Check(weapon?SkorpionChargeHandMove52(HandIdentity(),bones,move):RifleChargeHandMove(HandIdentity(),bones,move),"175 charging palm attachment active");
    const double clamped=(std::min)(1.,fraction);
    V3 contact=weapon?V3{.026,.14320966-clamped*travel,.04219692}:Apply(kA2000ChargeWrist229,{-.591,.011,1.149});
    if(!weapon)contact.y-=clamped*travel;
    const V3 expected=Apply(Multiply(ArmBodyToCamera(),p.rootBody),contact);
    const V3 actual=Apply(move,weapon?kSkorpionPinch229:V3{-.591,.011,1.149});
    Check(Length(Sub(actual,expected))<1e-5,"229 native grip contact follows real mechanism through rotated clamped rail, independent of saved contact box");
    Check(p.slidePull==fraction*.05,"175 placement never modifies mechanism travel state");
   }
  }
 }
 memcpy(g_magazineStates,saved,sizeof(saved));memcpy(g_magazineGuides,guides,sizeof(guides));g_guideLoadFamily=loadFamily;g_pistolMenu=menu;g_pistolGuidePreview=preview;g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_handLeftValid=left;g_handRightValid=right;g_motionGun=motion;g_weaponMotionGuard=guard;fc3pad::Publish(controls);
}
