// Fallback support contacts (pistols/other supported non-rifle guns): retain one
// neutral gun-local point. Opening Insert cannot adopt its different arm pose.
struct SupportLocal236 {V3 point={};UINT epoch=~0u;bool valid=false;};
SupportLocal236 g_supportLocal236[44];
bool StableSupportContact236(const V3& supportCamera,const Mat4& move,V3& point,V3& palm){
 const int profile=g_weaponMechanicProfile;
 if(profile<0||profile>=44||!g_handLeftValid||!g_handRightValid)return false;
 if(fc3hands303::SupportedSidearm315(profile)&&LeftSupportContact311(point,palm)){SupportGrabAdjustPoint214(point);return true;}
 if(RpgModel()&&LeftOther319(true)){
  Mat4 root={},grip={};if(!ControllerCurrentRoot124(root,grip))return false;point=Apply(root,OtherSupportPoint344(Apply(RpgKeyMat(kRpgLeftSupportFromFrame),{-.6781,.0519,1.0387})));
 }else if(Gm94Model()){
  if(!Gm94FrameReady())return false;const Mat4 box=Multiply(g_gm94.frameBody,Gm94PumpBox54());point={box.m[3],box.m[7],box.m[11]};
 }else{
  Mat4 root={},grip={},inverse={},viewInverse={};if(!ControllerCurrentRoot124(root,grip))return false;
  auto& local=g_supportLocal236[profile];if(local.epoch!=g_armPoseEpoch)local={};
  if(!local.valid){
   if(g_pistolMenu.open||PistolOwnsLeft()||g_reload.held||g_twoHand||fc3pad::Latest().rightTrigger>.3f||
      !Invert(root,inverse)||!Invert(ArmBodyToCamera(),viewInverse))return false;
   const bool fresh=g_supportPalmProfile==profile&&g_supportPalmEpoch==g_armPoseEpoch&&g_supportPalmFrame<=g_frame&&g_frame-g_supportPalmFrame<=2;
   const V3 d=fresh?g_supportPalmDelta:V3{};
   const V3 delta={move.m[0]*d.x+move.m[1]*d.y+move.m[2]*d.z,
                   move.m[4]*d.x+move.m[5]*d.y+move.m[6]*d.z,
                   move.m[8]*d.x+move.m[9]*d.y+move.m[10]*d.z};
   local.point=Apply(inverse,Apply(viewInverse,Add(supportCamera,delta)));local.epoch=g_armPoseEpoch;local.valid=true;
  }
  V3 localPoint309=local.point;const auto held309=fc3hands303::Read();
  // Same native support target, opposite side of the supported pistol for left ownership.
  if(fc3hands303::SupportedSidearm315(profile)&&held309.enabled&&held309.active&&held309.engaged&&!held309.empty&&held309.hand==1)localPoint309.x=-localPoint309.x;
  point=Apply(root,localPoint309);
 }
 if(!FreeHandPalm(1,palm))palm=ControllerWrist(g_handBody[1],1);
 SupportGrabAdjustPoint214(point);return true;
}
