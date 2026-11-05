// Changes the actual native aim command, never the HMD pose or a cosmetic dot alone.
// Gun animation and native bullet spread are retained. Quiet grip must be known.
void RifleAimAssist(V3& direction)
{
 const float strength=g_twoHand?g_twoHandRecoilAssist:g_rifleRecoilAssist;
 if(strength<=0||!AkWeaponVisualActive()||R700Model()||PistolPoseOwned()||
    fc3pad::GameplayInputBlocked()||!fc3pad::Latest().active||!g_handRightValid)return;
 const auto& p=g_pistolReload;
 if(!p.parentValid||p.parentEpoch!=g_armPoseEpoch||p.rootEpoch!=g_armPoseEpoch||g_frame-p.rootFrame>2)return;
 // Retain normal aim whenever the trigger is released; no cached world direction.
 if(!g_twoHand&&fc3pad::Latest().rightTrigger<.3f)return; // supported aim stays stable through trigger release/recovery
 Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
 Mat4 root=Multiply(controller,p.controllerParent);
 Mat4 fixed124={};const bool controlled124=ControllerMagazineRoot124(fixed124);if(controlled124)root=fixed124;
 if(g_twoHand&&!controlled124){
  if(!g_handLeftValid)return;
  Mat4 inverse={};if(!Invert(ArmBodyToCamera(),inverse))return;
  const V3 pivot=Apply(Multiply(root,(*CurrentRifle().hand)),{.591,.011,1.149});
  const V3 support=Apply(Multiply(root,(*CurrentRifle().support)),{-.591,.011,1.149});
  const V3 real=Apply(inverse,ControllerWrist(g_handLeft,1));
  if(Length(Sub(support,pivot))<.05||Length(Sub(real,pivot))<.05)return;
  root=Multiply(TurnAndMove(Sub(support,pivot),Sub(real,pivot),pivot,pivot),root);
 }
 const V3 quiet=Unit({root.m[1],root.m[5],root.m[9]});
 const double dot=direction.x*quiet.x+direction.y*quiet.y+direction.z*quiet.z;
 // A gross disagreement indicates an invalid grip, not recoil. Fail closed.
 if(!std::isfinite(dot)||dot<.8660254||dot>1.001)return;
 const double t=strength;
 const V3 mixed={direction.x*(1-t)+quiet.x*t,direction.y*(1-t)+quiet.y*t,direction.z*(1-t)+quiet.z*t};
 if(Length(mixed)>1e-6)direction=Unit(mixed);
}
