// Shared primary-handle binding for every supported inventory weapon. Bind to
// the displayed holding palm, then follow the owning controller without a
// render-frame delay. No per-weapon transfer gestures or facing requirements.
bool GrabBusy333(){
 const auto& p=g_pistolReload;
 if(g_reload.held||g_reload.pending||g_reloadAlignment||ReloadPoseActive())return true;
 if((PistolModel()||fc3hands303::OtherMagazine319(g_weaponMechanicProfile))&&(p.held||p.removalHeld||p.rackHeld||p.rackReturning||p.pending||p.cycle||p.slideClosing||(p.insertActive&&p.magazinePresent&&GetTickCount64()-p.insertAt<280)))return true;
 if(RevolverModel())return RevolverReloadLease321();
 if(FlareModel())return g_flare.open||g_flare.grab||g_flare.carrying||g_flare.pending||g_flare.shot.pending;
 if(SpasModel())return g_spas.carrying||g_spas.pending||(g_spas.grab&&g_spas.needPump)||g_spas.pump>0||SpasClipPlaying()||SpasNativeLeft();
 if(Lever1887Model())return Lever1887Holding()||g_lever.grabbing||g_lever.shell||g_lever.pending||g_lever.clip>=0||g_lever.leverOpen||g_lever.frac>0;
 if(M133SupportProfile56())return g_manualReload.pending||g_manualReload.cycle;
 if(R700Model())return R700Hold()||R700ClipPlaying()||g_r700.native||g_r700.grabbing;
 if(PkmModel())return PkmNativeActive();
 if(RpgModel())return g_rpg.carrying||g_rpg.pending||RpgClipPlaying();
 if(Gm94Model())return g_gm94.carrying||g_gm94.spentCarrying||g_gm94.coverGrab||g_gm94.coverOpen||g_gm94.pending||g_gm94Clip.kind!=0||g_gm94.pumping;
 if(FlamerModel())return g_flamer.leverGrab||g_flamer.carrying||g_flamer.pending||g_flamer.leverDown||g_flamerClip.kind!=0;
 if(BowModel())return g_bow.held||g_bow.nocked||g_bow.shooting||g_bow.pending||g_bow.releasePending;
 return false;
}
// One authored target for the XR resolver, visible guide and legacy contact.
// Never learn a target from a native firing/reload animation.
bool WeaponSupportTarget349(V3& point){
 Mat4 root={},grip={},support={};bool have=false;
 if(fc3hands303::MagazineRifle317(g_weaponMechanicProfile)){have=ControllerCurrentRoot124(root,grip);support=*CurrentRifle().support;}
 else if(fc3hands303::Other319(g_weaponMechanicProfile))have=OtherRoot319(root)&&OtherGrip319(grip,support);
 else if(SpasModel()){have=ShotgunRoot318(root);support=SpasForendHand();}
 else if(Lever1887Model()){have=ShotgunRoot318(root);support=kL1887LeftSupportFromFrame;}
 else return false;
 if(!have)return false;
 auto local=Apply(support,{-.6781,.0519,1.0387});
 if(fc3hands303::MagazineRifle317(g_weaponMechanicProfile))local=RifleSupportPoint343(local);
 else if(fc3hands303::Other319(g_weaponMechanicProfile))local=OtherSupportPoint344(local);
 else if(LeftShotgun318())local.x=-local.x;
 point=Apply(root,local);SupportGrabAdjustPoint214(point);return true;
}
// Turn around the actual holding palm toward the same calibrated foregrip
// which accepted the free hand. A wrist-based nominal target causes a snap
// as soon as g_twoHand changes, even when the hand was at the guide centre.
bool CalibratedSupportTurn350(Mat4& root,const Mat4& hand,const Mat4& support){
 const int profile=g_weaponMechanicProfile;
 if(!(fc3hands303::MagazineRifle317(profile)||fc3hands303::OtherMagazine319(profile)||profile==32))return false;
 V3 actual={};if(!FreeHandPalm(1,actual))return false;
 V3 local=Apply(support,{-.6781,.0519,1.0387});local=fc3hands303::MagazineRifle317(profile)?RifleSupportPoint343(local):OtherSupportPoint344(local);
 V3 nominal=Apply(root,local);const auto offset=SupportGrabOffset350(profile);
 if(SupportGrabEditable214(profile))nominal=Add(nominal,{root.m[0]*offset.x+root.m[1]*offset.y+root.m[2]*offset.z,root.m[4]*offset.x+root.m[5]*offset.y+root.m[6]*offset.z,root.m[8]*offset.x+root.m[9]*offset.y+root.m[10]*offset.z});
 V3 pivot=Apply(root,Apply(hand,{.6781,.0519,1.0387}));const auto held=fc3hands303::ReadPresentation314();
 if(held.hand>=0&&held.hand<2&&g_physicalValid303[held.hand]){const auto& a=g_freeHandAttachment[held.hand];if(a.epoch==g_armPoseEpoch&&a.profile==profile&&g_frame-a.frame<=2){Mat4 controller={};for(int k=0;k<16;++k)controller.m[k]=g_physicalBody303[held.hand][k];pivot=Apply(Multiply(controller,a.relative),{held.hand?-.6781:.6781,.0519,1.0387});}}
 if(Length(Sub(nominal,pivot))>.05&&Length(Sub(actual,pivot))>.05)root=Multiply(TurnAndMove(Sub(nominal,pivot),Sub(actual,pivot),pivot,pivot),root);
 return true;
}
void PublishHandleBinding333(){
 const auto h=fc3hands303::Read();fc3grab333::Binding b;b.entity=h.entity;b.generation=h.generation;b.epoch=g_armPoseEpoch;b.reference=fc3xr::ReferenceGeneration286();b.profile=g_weaponMechanicProfile;b.at=GetTickCount64();b.busy=GrabBusy333();
 b.physicalReload356=M133SupportProfile56()&&(g_reload.held||g_reload.pending||g_reloadAlignment||g_manualReload.pending||g_manualReload.cycle);
 const bool family=fc3hands303::Sidearm315(b.profile)||fc3hands303::MagazineRifle317(b.profile)||fc3hands303::Shotgun318(b.profile)||fc3hands303::Other319(b.profile)||BowModel();
 b.mode=family&&h.enabled&&h.engaged&&!h.empty;
 if(family&&h.enabled&&h.active&&h.engaged&&!h.empty&&h.hand>=0&&h.hand<2&&g_frame-g_weaponMechanicFrame<=2&&g_frame-g_armFinalFrame[h.hand]<=2&&g_physicalValid303[0]&&g_physicalValid303[1]&&!MacheteHeld()&&!fc3pad::GameplayInputBlocked()&&!fc3pad::NativeMenuOpen()&&!fc3pad::MenuOpen()){
  const int other=1-h.hand;Mat4 owner={},inverse={};for(int i=0;i<16;++i)owner.m[i]=g_physicalBody303[h.hand][i];
  if(Invert(owner,inverse)){
   const double scale=(std::max)(.1f,g_worldScale);const auto primary=Apply(g_armFinalBody[h.hand][2],{h.hand?-.6781:.6781,.0519,1.0387});
   const auto local=Apply(inverse,primary);b.handle[0]=float(local.x/scale);b.handle[1]=float(local.y/scale);b.handle[2]=float(-local.z/scale);b.enabled=std::isfinite(local.x)&&std::isfinite(local.y)&&std::isfinite(local.z);
   Mat4 root={},grip={},support={};V3 front={};bool haveRoot=BowModel()?BowRootBody(root):ControllerCurrentRoot124(root,grip);
   if(fc3hands303::Shotgun318(b.profile))haveRoot=ShotgunRoot318(root)&&ShotgunGrip318(grip);
   else if(fc3hands303::Other319(b.profile))haveRoot=OtherRoot319(root)&&OtherGrip319(grip,support);
   b.enabled=b.enabled&&(haveRoot||b.busy);
   if(haveRoot){
    const auto action=[&](const V3& world,float radius=.10f){if(b.actionCount>=6)return;const auto local=Apply(inverse,world);auto& a=b.actions[b.actionCount++];a.point[0]=float(local.x/scale);a.point[1]=float(local.y/scale);a.point[2]=float(-local.z/scale);a.radius=radius;};
    const auto actionBox=[&](const Mat4& box){if(std::isfinite(box.m[15])&&std::fabs(box.m[15])>.9)action({box.m[3],box.m[7],box.m[11]});};
    // Prefer the weapon's authored firing-grip binding to the arm fallback:
    // an imperfect displayed left hand must not move the weapon's grab area.
    const auto primary=BowModel()?Apply(Multiply(root,BowMat(kBowLeftHandFromRoot)),{-.6781,.0519,1.0387}):Apply(root,A2000ChargePoint336(Apply(grip,{.6781,.0519,1.0387})));
    const auto handle=Apply(inverse,primary);b.handle[0]=float(handle.x/scale);b.handle[1]=float(handle.y/scale);b.handle[2]=float(-handle.z/scale);
    if(fc3hands303::MagazineRifle317(b.profile)){support=*CurrentRifle().support;b.support=true;}
    else if(fc3hands303::Other319(b.profile))b.support=OtherGrip319(grip,support);
    else if(SpasModel()){support=SpasForendHand();b.support=true;}
    else if(Lever1887Model()){support=kL1887LeftSupportFromFrame;b.support=true;}
    if(b.support){auto point=fc3hands303::MagazineRifle317(b.profile)?RifleSupportPoint343(Apply(support,{-.6781,.0519,1.0387})):fc3hands303::Other319(b.profile)?OtherSupportPoint344(Apply(support,{-.6781,.0519,1.0387})):A2000ChargePoint336(Apply(support,{-.6781,.0519,1.0387}));if((Lever1887Model()||SpasModel())&&LeftShotgun318())point.x=-point.x;front=Apply(root,point);}
    else if(M133SupportProfile56())b.support=M133SupportTarget342(front);
    else if(fc3hands303::SupportedSidearm315(b.profile)){V3 palm={};if(LeftSupportContact311(front,palm))b.support=true;else{const auto& point=g_supportLocal236[b.profile];if(point.valid&&point.epoch==g_armPoseEpoch){auto local=point.point;if(h.hand==1)local.x=-local.x;front=Apply(root,local);b.support=true;}}}
    if(b.support){if(!WeaponSupportTarget349(front))SupportGrabAdjustPoint214(front);const auto point=Apply(inverse,front);b.foregrip[0]=float(point.x/scale);b.foregrip[1]=float(point.y/scale);b.foregrip[2]=float(-point.z/scale);b.supportActive=g_twoHand||SpasForendHeld();}
    if(PistolModel()||fc3hands303::OtherMagazine319(b.profile)){actionBox(PistolGuideBox(root,0));actionBox(PistolGuideBox(root,AkModel()?3:1));}
    else if(RevolverModel()){for(int box=0;box<2;++box)actionBox(Multiply(root,RevolverBoxFrame(box)));}
    else if(FlareModel()){for(int box=0;box<2;++box)actionBox(Multiply(root,FlareBox(box)));}
    else if(SpasModel()){for(int box=0;box<2;++box)actionBox(Multiply(root,SpasBoxFrame(box)));}
    else if(Lever1887Model()){action(Apply(root,{g_lever.lever[0],g_lever.lever[1],g_lever.lever[2]}));action(Apply(root,{g_lever.gate[0],g_lever.gate[1],g_lever.gate[2]}));}
    else if(M133SupportProfile56()){if(g_reloadContact.port)action(g_reloadContact.portBody);}
    else if(RpgModel())actionBox(Multiply(root,RpgMouthBox()));
    else if(Gm94Model()){actionBox(Multiply(root,Gm94CoverBox()));actionBox(Multiply(root,Gm94PortBoxMat()));actionBox(Multiply(root,Gm94PumpBox54()));}
    else if(FlamerModel()){actionBox(Multiply(root,FlamerLeverBox()));actionBox(Multiply(root,FlamerSeatBox()));}
    else if(BowModel()){
     if(g_bowSettings.pickup==2)action(BowNockPoint(root),.30f);
     else{Mat4 eye={},inverseEye={};for(int k=0;k<16;++k)eye.m[k]=g_eyeViewsBody[0].view[k];if(Invert(eye,inverseEye)){
      if(g_bowSettings.pickup==3){for(double side:{-.22,.22})action(Apply(inverseEye,Scale({side,-.10,.16},scale)),.22f);}
      else{const V3 zone=g_bowSettings.pickup==0?V3{RightBow319()?-.18:.18,-.65,.03}:V3{0,-.38,.12};action(Apply(inverseEye,Scale(zone,scale)),.20f);}
     }}
    }
    else if(RepairModel247()&&b.support)action(front);
    const auto& a=g_freeHandAttachment[other];Mat4 free={};for(int i=0;i<16;++i)free.m[i]=g_physicalBody303[other][i];
    if(a.epoch==g_armPoseEpoch&&a.profile==b.profile&&g_frame-a.frame<=2){
     const auto wrist=Multiply(free,a.relative);const auto palm=Apply(wrist,{other?-.6781:.6781,.0519,1.0387});Mat4 ri={};
     if(Invert(root,ri)){const auto point=Apply(ri,palm);const double radius=.05*scale;
      if(PistolModel()||fc3hands303::OtherMagazine319(b.profile)){const auto sphere=TransferSphere307(wrist,other);b.reloadNear=PistolGuideContact(root,0,sphere)||PistolGuideContact(root,AkModel()?3:1,sphere);}
      else if(RevolverModel()){for(int box=0;box<2;++box)b.reloadNear=b.reloadNear||RevolverBoxContact(box,point,radius);}
      else if(FlareModel()){for(int box=0;box<2;++box)b.reloadNear=b.reloadNear||FlareBoxDistance(box,point)<=radius;}
      else if(SpasModel()){for(int box=0;box<2;++box)b.reloadNear=b.reloadNear||SpasBoxDistance(box,point)<=radius;}
      else if(Lever1887Model())b.reloadNear=Lever1887GuideContact52(0,point)||Lever1887GuideContact52(1,point);
      else if(M133SupportProfile56())b.reloadNear=g_reloadContact.port&&Length(Sub(palm,g_reloadContact.portBody))<=.10*scale;
      else if(RpgModel())b.reloadNear=RpgMouthContact(point)<=1;
      else if(Gm94Model())b.reloadNear=Gm94BoxDistance(Gm94CoverBox(),kGm94CoverBoxSize,point)<=radius||Gm94BoxDistance(Gm94PortBoxMat(),kGm94PortBoxSize,point)<=radius||Gm94PumpDistance54()<=radius;
      else if(FlamerModel())b.reloadNear=FlamerBoxDistance(FlamerLeverBox(),kFlamerLeverBoxSize,point)<=radius||FlamerBoxDistance(FlamerSeatBox(),FlamerInsertSize55(),point)<=radius;
      else if(RepairModel247())b.reloadNear=b.support&&Length(Sub(palm,front))<=.12*scale;
     }
    }
   }
  }
 }
 fc3grab333::Publish(b);
 // Old gesture paths receive no contact or modifier permission. The XR grab
 // resolver alone claims the current physical squeeze and transfers the gun.
 fc3hands303::PublishTransfer307(b.enabled,false,b.enabled&&!b.busy,h.generation,h.entity,g_armPoseEpoch,false,false);
 static int last=-1;static unsigned count=0;if(last!=b.profile&&count++<80){last=b.profile;Log("[GRAB333] binding profile=%d entity=%08X enabled=%d busy=%d support=%d",b.profile,b.entity,b.enabled,b.busy,b.support);}
}
