fc3hands303::Settings g_handSettings303;
fc3hands303::State g_hands303;
bool g_ammoPouch334=false;
bool g_supportTracked342=false;
bool g_handsLoaded303=false,g_roleContext303=false,g_changeContext303=false,g_presentationContext326=false;
ULONGLONG g_roleAt303=0;unsigned g_roleEpoch303=0;int g_nativeHoldingSide303=0;
SRWLOCK g_handsLock303=SRWLOCK_INIT;
fc3hands303::Transfer307 g_transfer307;
fc3hands303::Snapshot g_handsSnapshot303;
fc3pad::Controllers g_rawHands303={};
bool g_handGripWait303[2]={},g_handTriggerWait303=true;
unsigned g_handGeneration303=~0u;
float g_handPalm303[2][3]={};unsigned g_handPalmEpoch303[2]={~0u,~0u};
void LoadHands303(){
 if(g_handsLoaded303)return;g_handsLoaded303=true;
 wchar_t path[MAX_PATH];_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_stereo.ini",fc3::DataFolder());
 g_handSettings303.enabled=GetPrivateProfileIntW(L"WeaponHands",L"Enabled",1,path)!=0;
 g_handSettings303.requireHolster=GetPrivateProfileIntW(L"WeaponHands",L"RequireHolster",1,path)!=0;
}
void PublishHands303(bool active){
 fc3hands303::Snapshot value;value.enabled=g_handSettings303.enabled&&g_hands303.initialized;
 value.engaged=g_roleContext303&&GetTickCount64()-g_roleAt303<=120;value.nativeHoldingSide=g_nativeHoldingSide303;
 value.active=active;value.empty=g_hands303.empty;value.hand=g_hands303.hand;value.slot=g_hands303.slot;
 value.entity=g_hands303.entity;value.generation=g_hands303.generation;value.receiver=g_hands303.receiver;value.at=GetTickCount64();
 if(fc3visual292::Preparing330()){value.active=false;value.empty=true;value.entity=0;value.receiver=-1;}
 value.ownerContext326=value.enabled&&(g_roleContext303||g_presentationContext326);
 // A clock deadline expires permission, not the last confirmed physical owner.
 // Hold presentation only while the same explicit context and owner survive.
 AcquireSRWLockShared(&g_handsLock303);const auto prior=g_handsSnapshot303;ReleaseSRWLockShared(&g_handsLock303);
 value.confirmed314=value.active?value.at:prior.confirmed314;
 value.presentation314=value.active||(value.ownerContext326&&prior.presentation314&&value.enabled&&!value.empty&&
  value.entity==prior.entity&&value.generation==prior.generation&&value.hand==prior.hand&&
  value.at>=value.confirmed314&&value.at-value.confirmed314<=1500);
 AcquireSRWLockExclusive(&g_handsLock303);g_handsSnapshot303=value;ReleaseSRWLockExclusive(&g_handsLock303);
}
void PauseHandsSession303(){
 g_handTriggerWait303=true;g_hands303.receiver=-1;g_hands303.pendingSlot=g_hands303.pendingHand=-1;g_hands303.pendingEntity=0;g_hands303.wasPaused=true;
 g_presentationContext326=false;AcquireSRWLockExclusive(&g_handsLock303);g_handsSnapshot303.active=g_handsSnapshot303.engaged=g_handsSnapshot303.presentation314=g_handsSnapshot303.ownerContext326=false;ReleaseSRWLockExclusive(&g_handsLock303);
 fc3pad::SetHandsStowed303(false);fc3select290::KeepAlive(false);
}
void RememberRaw303(const fc3pad::Controllers& c){AcquireSRWLockExclusive(&g_handsLock303);g_rawHands303=c;ReleaseSRWLockExclusive(&g_handsLock303);}
void WeaponRoleControls303(fc3pad::Controllers& c){
 c.ammoGripReserved326=false;c.ammoPouch334=false;c.shotgunSupport342=false;
 const auto s=fc3hands303::Read();
 fc3pad::SetHandsStowed303(s.enabled&&g_roleContext303&&(s.empty||!s.active));
 if(!s.enabled||c.menu||fc3pad::NativeMenuOpen()||fc3pad::MenuOpen())return;
 if(!s.active){g_handTriggerWait303=true;bool seated=false;int type=-1;if(g_roleContext303&&!fc3aim::NativeActionOwnsArms128()&&!(fc3aim::VehicleState(seated,type)&&seated))c.rightTrigger=c.leftTrigger=c.rightGrip=c.leftGrip=0;return;}
 c.weaponRoles303=true;c.physicalLeftGrip303=c.leftGrip;c.physicalRightGrip303=c.rightGrip;
 const float grip[2]={c.rightGrip,c.leftGrip};
 const auto now333=GetTickCount64();const int receiving356=1-s.hand;
 const bool latched356=receiving356>=0&&receiving356<2&&s.active&&s.engaged&&!s.empty&&g_hands303.gripAction333[receiving356]==fc3grab333::Support;
 const auto binding333=fc3support356::Live().Choose(fc3grab333::Read(),s.entity,s.generation,g_roleEpoch303,fc3xr::ReferenceGeneration286(),
    latched356&&c.active,g_supportTracked342,receiving356==0?c.rightGrip:c.leftGrip,now333);
 const bool direct333=binding333.mode&&binding333.entity==s.entity&&binding333.epoch==g_roleEpoch303&&binding333.reference==fc3xr::ReferenceGeneration286();
 const auto transfer309=fc3hands303::TransferContact307();const bool nativeSupport309=!transfer309.rearOnly316&&transfer309.enabled&&transfer309.generation==s.generation&&transfer309.entity==s.entity;
 const bool supportModifier326=nativeSupport309&&transfer309.modifier321;
 if(g_handGeneration303!=s.generation){
  g_handGeneration303=s.generation;g_handTriggerWait303=true;
  for(int h=0;h<2;++h)g_handGripWait303[h]=grip[h]>.25f;
 }
 if(c.rightTrigger<.2f&&c.leftTrigger<.2f)g_handTriggerWait303=false;
 for(int h=0;h<2;++h)if(grip[h]<.25f)g_handGripWait303[h]=false;
 if(direct333){
  g_handGripWait303[0]=g_handGripWait303[1]=false;
  const int other=1-s.hand,action=g_hands303.gripAction333[other];
  const bool reload=(binding333.busy&&now333>=binding333.at&&now333-binding333.at<=150)||action==fc3grab333::Reload;
  //1887: its authored foregrip becomes the temporary reload holding hand
  // after firing. AmmoControls must see that squeeze and its release; the
  // handle-transfer resolver still owns the firing handle, not the forend.
  const bool leverSupport340=binding333.profile==0&&action==fc3grab333::Support;
  const bool shotgun342=(binding333.profile>=3&&binding333.profile<=6)||binding333.profile==34;
  const bool support342=shotgun342&&g_supportTracked342&&action==fc3grab333::Support&&binding333.enabled&&binding333.generation==s.generation&&
   now333>=binding333.at&&now333-binding333.at<=120&&(!binding333.busy||binding333.supportActive);
  if(support342){c.shotgunSupport342=true;c.shotgunSupportProfile342=binding333.profile;c.shotgunSupportEntity342=s.entity;c.shotgunSupportGeneration342=s.generation;c.shotgunSupportAt342=now333;}
  c.ammoGripReserved326=!reload&&!leverSupport340&&!support342;
  c.ammoPouch334=reload&&binding333.profile==39&&g_ammoPouch334&&now333>=binding333.at&&now333-binding333.at<=150;
  // A latched intent never renews a stale/different weapon publication.
  if(!binding333.enabled||binding333.generation!=s.generation||now333<binding333.at||now333-binding333.at>150){c.ammoGripReserved326=true;c.ammoPouch334=c.shotgunSupport342=false;if(other)c.leftGrip=0;else c.rightGrip=0;}

  if(!reload&&action!=fc3grab333::Support){if(other)c.leftGrip=0;else c.rightGrip=0;}
 }else{
 // AK rear-grip transfer reserves this squeeze through cancellation/release.
 // The ordinary foregrip never arms a receiver, so its support input is intact.
 if(!supportModifier326&&(transfer309.rearOnly316||transfer309.modifier321)&&transfer309.enabled&&transfer309.generation==s.generation&&transfer309.entity==s.entity&&s.receiver>=0&&s.receiver<2&&grip[s.receiver]>.25f)g_handGripWait303[s.receiver]=true;
 // .44 modifier owns the opposite squeeze before any cylinder/pickup logic.
 // Existing reload leases publish allowed=false and retain their inputs.
 if(transfer309.modifier321&&transfer309.enabled&&transfer309.allowed&&transfer309.generation==s.generation&&transfer309.entity==s.entity&&s.hand>=0&&s.hand<2&&grip[s.hand]>.35f&&grip[1-s.hand]>.25f){if(supportModifier326)c.ammoGripReserved326=!c.a&&!c.b&&!c.x&&!c.y&&!c.menu;else g_handGripWait303[1-s.hand]=true;}
 if(g_handGripWait303[0]||(s.receiver==0&&!nativeSupport309))c.rightGrip=0;
 if(g_handGripWait303[1]||(s.receiver==1&&!nativeSupport309))c.leftGrip=0;
 }
 if(s.empty){c.rightTrigger=c.leftTrigger=c.rightGrip=c.leftGrip=0;return;}
 if(fc3hands303::PhysicalHand(s,0)==1){
  std::swap(c.rightTrigger,c.leftTrigger);std::swap(c.rightGrip,c.leftGrip);
  //1911 weapon actions follow the support hand; physical menu/stick chords stay above.
  const auto transfer=fc3hands303::TransferContact307();
  if(transfer.enabled&&transfer.generation==s.generation&&transfer.entity==s.entity)std::swap(c.x,c.b);
  std::swap(c.rightIndexTouch,c.leftIndexTouch);std::swap(c.rightThumbTouch,c.leftThumbTouch);
 }
 if(c.shotgunSupport342)c.x=false; // accepted forend grip cannot also take ammunition
 if(g_handTriggerWait303)c.rightTrigger=c.leftTrigger=0;
}
