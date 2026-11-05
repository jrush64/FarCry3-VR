void CancelMagazinePull286(PistolReloadState& p){
 if(p.returning286)p.looseVelocity=p.returnVelocity286;
 p.returning286=false;p.returnAt286=0;p.returnKey286={};p.pull286.Reset();
}
bool ReadMagazineKey286(fc3pull286::Key& key){
 auto& p=g_pistolReload;unsigned long long owner=0;unsigned wrapper=0,weapon=0,strategy=0;
 if(!fc3aim::LocalPlayerIdentity143(owner)||!fc3aim::EquippedWeapon206(wrapper,weapon,strategy)||!wrapper||!weapon)return false;
 key={owner,(static_cast<unsigned long long>(wrapper)<<32)|weapon,p.looseId,g_armPoseEpoch,int(g_weaponMechanicProfile),fc3xr::ReferenceGeneration286()};return key.Valid();
}
bool MagazinePullContext286(const fc3pull286::Key& key){
 auto& p=g_pistolReload;
 bool seated=false;int seat=-1;
 if(!g_pistolGuides.pullAssist286||!MagazineProfile(g_weaponMechanicProfile)||g_weaponMechanicFrame>g_frame||g_frame-g_weaponMechanicFrame>1||!g_enabled||!g_inGameplay||!g_eyeViewsSet||!g_handLeftValid||!g_handRightValid||
  g_pistolMenu.open||fc3pad::GameplayInputBlocked()||fc3pad::WeaponSelectionRequested()||fc3aim::NativeActionOwnsArms128()||
  !fc3aim::VehicleState(seated,seat)||seated||!PistolLooseFresh()||p.handFrame>g_frame||p.palmFrame>g_frame||
  p.rootFrame>g_frame||g_frame-p.rootFrame>1||p.rootEpoch!=g_armPoseEpoch||p.magazinePresent||p.held||!p.looseFlying||p.pending||p.cycle||p.removalHeld||
  p.rackHeld||p.rackReturning||!std::isfinite(g_worldScale)||g_worldScale<.1f||!key.Valid()||!(key==p.looseKey286))return false;
 return true;
}
bool TickMagazinePullCore286(const fc3pad::Controllers& c,ULONGLONG now,const fc3pull286::Key& key){
 auto& p=g_pistolReload;const double scale=(std::max)(.1f,g_worldScale);
 // Body-coordinate rotation cannot impersonate a physical hand pull.
 if(!std::isfinite(g_bodyYaw)||((p.pull286.active||p.returning286)&&fabs(g_bodyYaw-p.pullYaw301)>1e-6)){CancelMagazinePull286(p);return false;}
 p.pullYaw301=g_bodyYaw;
 const bool context=c.active&&std::isfinite(c.leftGrip)&&!c.x&&!c.y&&!c.menu&&c.leftTrigger<.25f&&c.rightTrigger<.25f&&MagazinePullContext286(key);
 if(!context){CancelMagazinePull286(p);return false;}
 const V3 hand={g_handBody[1][3]/scale,g_handBody[1][7]/scale,g_handBody[1][11]/scale};
 const V3 target=Scale(Apply(p.looseBody,{0,0,0}),1/scale);
 if(p.returning286){
  if(!(key==p.returnKey286)||c.leftGrip<.5f||now<p.returnAt286||now-p.returnAt286>750||now<p.looseTick||now-p.looseTick>100){CancelMagazinePull286(p);return false;}
  const V3 anchor=Scale(PistolLooseAnchor(),1/scale);fc3pull286::Vec next;
  if(!fc3pull286::Advance({target.x,target.y,target.z},{anchor.x,anchor.y,anchor.z},double(now-p.looseTick)/1000,next)){CancelMagazinePull286(p);return false;}
  p.looseBody.m[3]=next.x*scale;p.looseBody.m[7]=next.y*scale;p.looseBody.m[11]=next.z*scale;p.looseTick=now;
  return true;
 }
 fc3pull286::Input input;input.allowed=context;input.eligible=p.catchArmed&&now>=p.looseAt&&now-p.looseAt<(p.looseToss?3000:1000);
 input.grip=c.leftGrip;input.range=g_pistolGuides.pullRange286;input.pull=g_pistolGuides.pullDistance286;
 input.hand={hand.x,hand.y,hand.z};input.forward={g_handBody[1][2],g_handBody[1][6],g_handBody[1][10]};input.target={target.x,target.y,target.z};input.key=key;input.now=now;
 const auto result=p.pull286.Step(input);
 if(result.acquired)fc3xr::InteractionPulse285(1);
 if(!result.committed)return false;
 p.returning286=true;p.returnAt286=now;p.returnKey286=key;p.returnVelocity286=p.looseVelocity;p.looseTick=now;
 fc3xr::InteractionPulse285(1);Log("[MAGPULL286] recover existing loose magazine %u, profile %ld; no pickup, reload or ammo request",p.looseId,g_weaponMechanicProfile);
 return true;
}
bool TickMagazinePull286(const fc3pad::Controllers& c,ULONGLONG now){
 if(!g_pistolGuides.pullAssist286){CancelMagazinePull286(g_pistolReload);return false;}
 fc3pull286::Key key;if(!ReadMagazineKey286(key)){CancelMagazinePull286(g_pistolReload);return false;}
 return TickMagazinePullCore286(c,now,key);
}
