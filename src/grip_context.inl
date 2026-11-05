// Geometry only: never claim a new action during a held squeeze. Unknown/stale
// poses reserve grip for weapon handling; grip+Y remains the explicit override.
void UpdateGripContext()
{
 const bool valid=g_enabled&&g_inGameplay&&MotionInteractionEnabled()&&g_eyeViewsSet&&
  g_handLeftValid&&g_handRightValid&&fc3pad::Latest().active&&!fc3pad::GameplayInputBlocked()&&
  g_frame-g_weaponMechanicFrame<=2;
 if(!valid){fc3pad::SetGripContext(false,true);return;}
 bool weaponZone=g_twoHand||g_reload.held||ManualReloadOwnsLeftHand()||PistolOwnsLeft()||
  Lever1887LeftHold()||R700LeftHold()||FlamerBusyLeft()||Gm94BusyLeft()||
  (RpgActive()&&(RpgCarrying()||RpgClipPlaying()))||BowActive()||SpasForendHeld();
 const double scale=(std::max)(.1f,g_worldScale);
 const V3 left=ControllerWrist(g_handLeft,1),right=ControllerWrist(g_handRight,0);
 // Reserve the immediate gun-hand area for pistol support and loading controls.
 weaponZone=weaponZone||Length(Sub(left,right))<.25*scale;
 Mat4 move={};const bool held=g_twoHand;g_twoHand=false;
 const bool supportValid=g_armJoints[0].valid&&g_armJoints[1].valid&&GunMove(g_armJoints[0],g_armJoints[1],move);
 if(supportValid){
  const auto& j=g_armJoints[1];V3 support=Apply(move,{j.w[0],j.w[1],j.w[2]});
  if(!Lever1887SupportPosition(support)&&!R700SupportPosition(support)&&!Gm94SupportPosition(support))AkSupportPosition(support);
  V3 target={},contactHand={};
  if(GrabGizmoPoints(support,target,contactHand,&move))weaponZone=weaponZone||Length(Sub(contactHand,target))<kGrabReach+.04*scale;
  else weaponZone=true; // unavailable attachment reserves grip; never misroute to wheel
 }
 g_twoHand=held;
 if(!supportValid){fc3pad::SetGripContext(false,true);return;}
 // Existing body ammo regions, with a small margin to avoid a boundary misroute.
 Mat4 view={},head={};for(int i=0;i<16;++i)view.m[i]=g_eyeViewsBody[0].view[i];
 if(!Invert(view,head)){fc3pad::SetGripContext(false,true);return;}
 const V3 origin=Apply(head,{0,0,0}),palm=ControllerWrist(g_handBody[1],1);
 weaponZone=weaponZone||Length(Sub(palm,Add(origin,Scale({-.18,-.65,.03},scale))))<.22*scale||
            Length(Sub(palm,Add(origin,Scale({0,-.38,.12},scale))))<.22*scale;
 if(PistolHandling()){
  const auto& p=g_pistolReload;
  if(g_frame-p.rootFrame>1||g_frame-p.palmFrame>1||p.rootEpoch!=g_armPoseEpoch){fc3pad::SetGripContext(false,true);return;}
  weaponZone=weaponZone||PistolBodyPickupNear()||PistolGuideContact(p.rootBody,0,p.tip)||
       PistolGuideContact(p.rootBody,1,p.tip)||(AkModel()&&PistolGuideContact(p.rootBody,3,p.tip));
  // Button-only ammo pickup needs grip before X, even without a spatial target.
  weaponZone=weaponZone||(!p.magazinePresent&&g_pistolGuides.ammoPickup==2)||p.held||p.removalHeld||p.rackHeld;
 }
 // Dedicated loading modes with their own hand leases retain that ownership.
 // Their override remains available away from any safe generic grab geometry.
 weaponZone=weaponZone||RevolverActive()||FlareActive()||SpasClipPlaying();
 auto needsAmmo=[](UINT state,UINT wrapper){UINT n=0,cap=0,spare=0;return !ReloadSignalLocal(state,wrapper)||!Lever1887Ammo(wrapper,n,cap,spare)||n<cap;};
 weaponZone=weaponZone||(SpasManual()&&g_spas.pickup==2&&needsAmmo(g_spas.ammoState,g_spas.ammoWrapper))||
    (Lever1887Active()&&g_lever.pickup==2&&needsAmmo(g_lever.ammoState,g_lever.ammoWrapper))||
    (RpgActive()&&RpgLoadedRounds()<=0);
 fc3pad::SetGripContext(true,weaponZone);
}

