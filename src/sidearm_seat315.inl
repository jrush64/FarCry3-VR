//1911 physical insertion completes its existing native data transaction at the
//local game-thread request seam. Never wait for an animation predicate to seat.
void Seat6P9Native315(UINT state,UINT wrapper){
 auto& p=g_pistolReload;
 if(g_weaponMechanicProfile!=1||!p.enabled||!p.pending||p.entryAccepted||p.cycle||GetTickCount64()>p.deadline||!PistolLocal(state,wrapper)||!ReloadSignalLocal(state,wrapper))return;
 if(!g_nativeReloadBegin||!g_nativeReload||!g_nativeReloadEnd||(ReloadRead(state,8)&255))return;
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 p.state=state;p.wrapper=wrapper;p.weapon=weapon;p.before=before;
 //Claim this insertion before invoking callbacks; re-entry cannot spend it twice.
 p.pending=p.entryAccepted=p.cycle=false;g_reloadSignal.pistolPermissionUntil=0;fc3pad::CancelReloadRequest();
 const bool ok=D50NativeSeat226(state,wrapper); // shared native Begin/transfer/End ABI, cleanup on failure
 const UINT after=ReloadRead(weapon,0xcc);
 const bool same=PistolLocal(state,wrapper)&&ReloadSignalLocal(state,wrapper)&&ReloadRead(wrapper,0x48)==weapon;
 const bool idle=(ReloadRead(state,8)&255)==0;
 const bool refilled=after>before;
 //A native no-op may still seat verified existing rounds (full/native capacity
 //or no reserve). Never invent a refill, and never seat an empty failed transfer.
 const bool existing=before>0&&after==before;
 if(!ok||!same||!idle||(!refilled&&!existing)){
  p.committed=p.magazinePresent=p.insertActive=false;p.held=true;p.looseFlying=false;p.result=2;
  p.insertionRetryBlocked=true;p.contactFrames=0;p.contactFrame=-1;p.poseHoldUntil=0;g_twoHand=false;
  Log("[6P9 SEAT315] unconfirmed; same magazine retained, loaded %u -> %u owner=%d idle=%d callbacks=%d",before,after,same,idle,ok);return;
 }
 ProtectSeatedMagazine54();p.magazinePresent=true;p.held=p.looseFlying=p.removalHeld=p.removalDetached=false;
 p.committed=refilled;if(refilled)++p.commits;
 p.reloadChamberTracked=true;p.reloadWasEmpty=before==0;p.result=1;p.contactFrames=0;p.contactFrame=-1;p.insertionRetryBlocked=false;
 p.poseHoldUntil=0;p.fireResumeFrame=-100000;p.waitTriggerRelease=fc3pad::Latest().rightTrigger>.3f;
 p.chamberReady=before>0;p.awaitingNativeClose=false;p.needsRack=p.rackMode=before==0;
 p.rackArmed=p.rackHeld=p.rackRear=p.rackReturning=p.emptyLocked=p.slideClosing=false;p.slidePull=0;
 fc3xr::Buzz(1);
 Log("[6P9 SEAT315] native insertion complete %u -> %u refill=%d existing-round-reseat=%d; seated, no loose item/pending animation; empty-magazine slide gate=%d",before,after,refilled,existing,before==0);
}
