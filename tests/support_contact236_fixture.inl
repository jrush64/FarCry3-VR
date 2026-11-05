void TestSupportContact236(){
 const auto profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame,frame=g_frame;const auto controls=fc3pad::Latest();
 const auto menu=g_pistolMenu;const auto reload=g_reload;const auto bolt=g_r700;const auto free=g_freeHandAttachment[1];
 const auto neutral=g_controllerNeutral124;const auto attachment=g_freeRightFromController;const UINT rightEpoch=g_freeRightEpoch;
 const bool left=g_handLeftValid,right=g_handRightValid,two=g_twoHand;float hands[2][16];memcpy(hands,g_handBody,sizeof(hands));
 const auto delta=g_supportPalmDelta;const auto palmFrame=g_supportPalmFrame,palmProfile=g_supportPalmProfile;const auto palmEpoch=g_supportPalmEpoch;
 PistolReloadState states[20];memcpy(states,g_magazineStates,sizeof(states));SupportLocal236 cached[44];memcpy(cached,g_supportLocal236,sizeof(cached));
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);g_handLeftValid=g_handRightValid=true;g_twoHand=false;g_reload={};g_r700={};g_pistolMenu.open=false;
 const auto id=HandIdentity();for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_handBody[side][i]=float(id.m[i]);
 for(int weapon:{20,1,12,32,34}){
  g_weaponMechanicProfile=weapon;g_weaponMechanicFrame=++g_frame;g_pistolReload={};g_supportLocal236[weapon]={};
  g_supportPalmFrame=g_frame;g_supportPalmProfile=weapon;g_supportPalmEpoch=g_armPoseEpoch;g_supportPalmDelta={.03,-.02,.01};
  V3 first={},second={},palm={};Check(GrabGizmoPoints({.2,.1,.4},first,palm,&id),"236 non-rifle gameplay support learns neutral gun-local contact");
  Mat4 root={},grip={},inverse={};Check(ControllerCurrentRoot124(root,grip)&&Invert(root,inverse),"236 non-rifle root available");const V3 local=Apply(inverse,first);
  for(int step=0;step<3;++step){g_handBody[0][3]+=.1f;g_handBody[0][7]-=.02f;Check(ControllerCurrentRoot124(root,grip),"236 moving support root");const V3 expected=Apply(root,local);
   g_pistolMenu.open=false;Check(GrabGizmoPoints({9,8,7},first,palm,&id),"236 gameplay retains fixed support");Mat4 gameplay={},preview={};Check(SupportGrabBox214(id,first,gameplay),"236 gameplay contact OBB");
   g_pistolMenu.open=true;g_supportPalmDelta={5,6,7};Check(GrabGizmoPoints({-9,-8,-7},second,palm,&id)&&SupportGrabBox214(id,second,preview),"236 Insert uses same fixed support");
   Check(Length(Sub(first,expected))<1e-6&&Length(Sub(first,second))<1e-8,"236 opening Insert or changing native wrist/palm cannot move non-rifle contact");for(int i=0;i<16;++i)Check(fabs(gameplay.m[i]-preview.m[i])<1e-8,"236 non-rifle preview matches real OBB");g_pistolMenu.open=false;
  }
 }
 for(int weapon:{2,23}){
  g_weaponMechanicProfile=weapon;g_weaponMechanicFrame=++g_frame;g_r700={};g_r700.freeEpoch=g_armPoseEpoch;g_r700.freeFromController=*Bolt().hand;
  g_freeHandAttachment[1].epoch=~g_armPoseEpoch;V3 first={},second={},palm={};Check(GrabGizmoPoints({1,2,3},first,palm),"236 both bolt rifles offer fixed support without palm cache");
  g_pistolMenu.open=true;g_freeHandAttachment[1].relative=id;g_freeHandAttachment[1].epoch=g_armPoseEpoch;g_freeHandAttachment[1].frame=g_frame;g_freeHandAttachment[1].profile=weapon;
  Check(GrabGizmoPoints({7,8,9},second,palm)&&Length(Sub(first,second))<1e-8,"236 R700/Z93 support target unchanged by Insert and palm attachment availability");g_pistolMenu.open=false;
 }
 memcpy(g_supportLocal236,cached,sizeof(cached));memcpy(g_magazineStates,states,sizeof(states));memcpy(g_handBody,hands,sizeof(hands));
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_frame=frame;g_pistolMenu=menu;g_reload=reload;g_r700=bolt;g_freeHandAttachment[1]=free;g_controllerNeutral124=neutral;g_freeRightFromController=attachment;g_freeRightEpoch=rightEpoch;g_handLeftValid=left;g_handRightValid=right;g_twoHand=two;
 g_supportPalmDelta=delta;g_supportPalmFrame=palmFrame;g_supportPalmProfile=palmProfile;g_supportPalmEpoch=palmEpoch;fc3pad::Publish(controls);
 puts("PASS:236 pistol/RPG/SPAS support contacts and OBBs follow fixed roots through Insert/native pose changes; both bolt rifles ignore palm-cache transitions");
}
