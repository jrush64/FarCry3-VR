// PKM (pkm.xbg, profile 43, rifle family 18; skinned like the MP5 - see mp5.inl). Design: the box goes in by hand,
// then the game's original animations open the top compartment and load the belt, for full and partial reloads;
// after that both hands play the regular animation.
//  - The box comes out and goes in by hand on the rifle path (LEFT grip + X on the box, X alone drops a box that is not full).
//  - Seating it does NOT run the instant reload: the game's own reload is permitted (PkmNativeInsert) and plays.
//  - The game's reload always starts by taking the OLD box off (0.74-1.73 s, captures/weapons6-115332): until the new box is settled
//    (kPkmEmptyClipSettledAt, 1.93 s) the box, its lid and the 12 belt rounds stay seated where the player put them and the left
//    arm stays on the controller. From then on BOTH arms play the game's animation on the gun in the hand (feed cover, belt, close,
//    bolt): the arms take the gun's own native->corrected move, the gun stays on the right controller.
//  - Taking the box out by hand carries its lid and belt rounds with it (they are separate palette slots / a separate draw).
struct Pkm
{
 bool native=false;ULONGLONG nativeAt=0,nativeEndAt=0;
 Mat4 lidIdleRel={};bool lidValid=false;UINT lidEpoch=0; // lid (slot 8 of draw 14397) relative to the gun at rest, learned live
 Mat4 beltIdleRel[12]={};bool beltValid=false;UINT beltEpoch=0; // the 12 belt rounds (draw 38397) relative to the gun at rest
 Mat4 boxDelta={};bool boxHidden=false,boxMoved=false;LONG boxFrame=-100000; // this frame's box placement vs its seated rest
} g_pkm;
bool PkmModel(){return g_weaponMechanicProfile==43&&g_frame-g_weaponMechanicFrame<=2;}
bool PkmNativeActive()
{
 auto& K=g_pkm;if(!K.native)return false;
 const ULONGLONG now=GetTickCount64();
 if(!PkmModel()||now-K.nativeAt>12000||(K.nativeEndAt&&now-K.nativeEndAt>150)){K.native=false;return false;}
 return true;
}
double PkmNativeSeconds(){return static_cast<double>(GetTickCount64()-g_pkm.nativeAt)/1000.0;}
bool PkmBoxPhase(){return PkmNativeActive()&&PkmNativeSeconds()<kPkmEmptyClipSettledAt-kPkmEmptyBeginAt;} // the game is still swapping the old box
bool PkmArmsNative(){return PkmNativeActive()&&!PkmBoxPhase();}
// The arms' move while the game animates: native gun -> corrected gun (view space), this frame.
bool PkmGunMove(Mat4& move)
{
 if(!PkmArmsNative()||g_rifleAttachFrame!=g_frame||g_rifleAttachEpoch!=g_armPoseEpoch)return false;
 move=g_rifleAttachMove;return true;
}
bool AkInstantNativeTriple(UINT,UINT);
void PkmNativeInsert(UINT state,UINT wrapper)
{
 auto& p=g_pistolReload;
 p.pending=p.entryAccepted=p.cycle=false;fc3pad::CancelReloadRequest();
 ProtectSeatedMagazine54();p.magazinePresent=true;p.held=p.looseFlying=false;
 p.needsRack=p.rackMode=p.rackArmed=p.rackHeld=p.rackReturning=p.rackRear=p.emptyLocked=false;p.slidePull=0;
 p.chamberReady=true;p.awaitingNativeClose=false;p.reloadChamberTracked=false;p.akRackComplete=true;
 p.fireResumeFrame=-100000;p.poseHoldUntil=0;p.waitTriggerRelease=false;p.result=1;p.contactFrames=0;
 const bool granted=GrantPistolReloadPermission(state,wrapper);
 if(!granted)AkInstantNativeTriple(state,wrapper);
 fc3xr::Buzz(1);
 Log("[PKM] box seated by hand at the request seam: %s",granted?"the game's own reload takes over (cover, belt, bolt)":"permission refused, seated instantly");
}
void PkmNativeBegin(UINT state,UINT wrapper)
{
 if(!PkmModel()||!ReloadSignalLocal(state,wrapper))return;
 auto& K=g_pkm;K.native=true;K.nativeAt=GetTickCount64();K.nativeEndAt=0;
 Log("[PKM] native reload: box stays seated, left arm on the controller until %.2f s, then both arms follow the game",kPkmEmptyClipSettledAt-kPkmEmptyBeginAt);
}
void PkmNativeEnd(UINT state,UINT wrapper)
{
 auto& K=g_pkm;if(!K.native||!ReloadSignalLocal(state,wrapper))return;
 K.nativeEndAt=GetTickCount64();Log("[PKM] native reload end");
}
bool PkmNearIdentity(const Mat4& m){double d=0;for(int r=0;r<3;++r)for(int c=0;c<4;++c)d=(std::max)(d,std::fabs(m.m[r*4+c]-(r==c?1.0:0.0)));return d<1e-3;}
void PkmHideSlot(float* bones,int slot){bones[slot*12+7]=-100;}
// After Mp5PlaceSlots (same draw): the lid and the belt rounds follow the box; early in the game's reload they stay seated.
void PkmPostSlots(float* bones,UINT start,UINT count,const Mat4& frameModel)
{
 if(!PkmModel())return;
 auto& K=g_pkm;Mat4 frameInverse={};if(!Invert(frameModel,frameInverse))return;
 if(start==14397&&count==23700)
 { // slot 7 = BOX, slot 8 = BOX_LID
  const Mat4 boxRestPalette=Multiply(frameModel,Multiply(kPkmClipRest,Mp5BindInverse(kPkmClipRest)));Mat4 restInverse={};
  K.boxHidden=bones[7*12+7]==-100.f;K.boxDelta=HandIdentity();
  if(!K.boxHidden&&Invert(boxRestPalette,restInverse))K.boxDelta=Multiply(BoneMatrix(bones,7),restInverse);
  K.boxMoved=K.boxHidden||!PkmNearIdentity(K.boxDelta);K.boxFrame=g_frame;
  if(K.lidEpoch!=g_armPoseEpoch)K.lidValid=false;
  if(!K.boxMoved&&!PkmNativeActive()){K.lidIdleRel=Multiply(frameInverse,BoneMatrix(bones,8));K.lidValid=true;K.lidEpoch=g_armPoseEpoch;}
  if(K.boxHidden)PkmHideSlot(bones,8);
  else if(K.lidValid&&(K.boxMoved||PkmBoxPhase()))StoreBone(bones,8,Multiply(K.boxDelta,Multiply(frameModel,K.lidIdleRel)));
  return;
 }
 if(start==38397&&count==3528&&K.boxFrame==g_frame)
 { // the 12 belt rounds (slots 0..11): on the gun as the game animates them; seated early in the reload; ride the box when it is out
  if(K.beltEpoch!=g_armPoseEpoch)K.beltValid=false;
  if(!K.boxMoved&&!PkmNativeActive()){for(int b=0;b<12;++b)K.beltIdleRel[b]=Multiply(frameInverse,BoneMatrix(bones,b));K.beltValid=true;K.beltEpoch=g_armPoseEpoch;}
  if(K.boxHidden){for(int b=0;b<12;++b)PkmHideSlot(bones,b);return;}
  if(K.beltValid&&(K.boxMoved||PkmBoxPhase()))for(int b=0;b<12;++b)StoreBone(bones,b,Multiply(K.boxDelta,Multiply(frameModel,K.beltIdleRel[b])));
 }
}
