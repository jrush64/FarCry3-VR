// The same calibrated hand sphere used for physical1911 magazine insertion.
// Right receiving hand mirrors the left-hand wrist/offset in bind X only.
V3 TransferSphere307(const Mat4& hand,int side)
{
 const double x=side?-.591+g_pistolGuides.hand[0]:.591-g_pistolGuides.hand[0];
 return Apply(hand,{x,.011+g_pistolGuides.hand[1],1.149+g_pistolGuides.hand[2]});
}
bool TransferHandleContact307(const Mat4& root,V3 sphere)
{
 Mat4 inverse={};if(!Invert(root,inverse))return false;const V3 point=Apply(inverse,sphere);
 if(!std::isfinite(point.x)||!std::isfinite(point.y)||!std::isfinite(point.z))return false;
 // Native Shadow/Kimber handle. Separate from the calibrated magazine port
 // and slide: the closest action region wins; an actual reload always preempts.
 const V3 centre={0,-.013,-.015},port={g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]},slide={g_pistolGuides.slide[0],g_pistolGuides.slide[1],g_pistolGuides.slide[2]};
 const double distance=Length(Sub(point,centre));if(Length(Sub(point,port))<=distance||Length(Sub(point,slide))<=distance)return false;
 const double scale=(std::max)(.1f,g_worldScale),half[]={.025*scale,.035*scale,.040*scale};
 const double delta[]={point.x-centre.x,point.y-centre.y,point.z-centre.z};double outside=0;
 for(int i=0;i<3;++i){const double d=(std::max)(0.,std::fabs(delta[i])-half[i]);outside+=d*d;}
 const double radius=g_pistolGuides.handDiameter*.5*scale;
 return std::isfinite(radius)&&radius>0&&outside<=radius*radius;
}
// Reload targets retain priority; the handle no longer needs a small contact hit.
bool ReloadContact308(const Mat4& root,V3 sphere){
 Mat4 inverse={};if(!Invert(root,inverse))return true;const V3 point=Apply(inverse,sphere);
 if(!std::isfinite(point.x)||!std::isfinite(point.y)||!std::isfinite(point.z))return true;
 const V3 handle={0,-.013,-.015},port={g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]},slide={g_pistolGuides.slide[0],g_pistolGuides.slide[1],g_pistolGuides.slide[2]};
 const double handleDistance=Length(Sub(point,handle));
 return (Length(Sub(point,port))<handleDistance&&PistolGuideContact(root,0,sphere))||
        (Length(Sub(point,slide))<handleDistance&&PistolGuideContact(root,1,sphere));
}
void PublishAkTransfer316();
void PublishShotgunTransfer318();
void PublishOtherTransfer319();
void PublishBowTransfer319();
void PublishRevolverModifier321();
#include "handle_binding333.inl"
void PublishSmgTransfer332(const fc3hands303::Snapshot& held,bool enabled,bool contact,bool allowed,bool rearOnly,bool reloadContact){
 float right=0,left=0,rt=0,lt=0;fc3hands303::RawControls(right,left,rt,lt);
 const auto prior=fc3hands303::TransferContact307();
 const bool armed=held.receiver>=0&&prior.enabled&&prior.entity==held.entity&&prior.generation==held.generation;
 const bool modifier=held.hand>=0&&held.hand<2&&(armed?prior.modifier321:(held.hand?left:right)>.35f);
 // A new magazine/charging grab wins over a pass, just like an active reload.
 if(modifier&&reloadContact)allowed=false;
 fc3hands303::PublishTransfer307(enabled,modifier?allowed:contact,allowed,held.generation,held.entity,g_armPoseEpoch,modifier?false:rearOnly,modifier);
}
void PublishLegacyTransfer332()
{
 fc3grab333::Publish({}); // offline preservation fixture; never used by live controls
 if(g_weaponMechanicProfile==42){PublishRevolverModifier321();return;}
 if(BowModel()){PublishBowTransfer319();return;}
 if(fc3hands303::Other319(g_weaponMechanicProfile)){PublishOtherTransfer319();return;}
 if(fc3hands303::Shotgun318(g_weaponMechanicProfile)){PublishShotgunTransfer318();return;}
 if(fc3hands303::MagazineRifle317(g_weaponMechanicProfile)){PublishAkTransfer316();return;}
 const auto held=fc3hands303::Read();
 const bool enabled=held.enabled&&held.engaged&&!held.empty&&held.nativeHoldingSide==0&&fc3hands303::Sidearm315(g_weaponMechanicProfile);
 bool contact=false,allowed=false,reloadContact332=false;
 if(enabled&&held.active&&g_frame-g_weaponMechanicFrame<=2&&held.hand>=0&&held.hand<2){
  Mat4 root={},grip={};const auto& p=g_pistolReload;
  const bool magazineBusy=fc3hands303::MagazineSidearm315(g_weaponMechanicProfile)&&(PistolBusy()||p.held||p.removalHeld||p.rackHeld||p.rackReturning||p.pending||p.cycle);
  allowed=g_physicalValid303[0]&&g_physicalValid303[1]&&
   !magazineBusy&&!g_reload.held&&!g_reload.pending&&!g_reloadAlignment&&!ReloadPoseActive()&&!g_pistolMenu.open&&!fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()&&ControllerCurrentRoot124(root,grip);
  if(RevolverModel())allowed=allowed&&!g_revolver.open&&!g_revolver.grab&&!g_revolver.carrying&&!g_revolver.pending&&!RevolverClipPlaying()&&!RevolverNativeLeft();
  if(FlareModel())allowed=allowed&&!g_flare.open&&!g_flare.grab&&!g_flare.carrying&&!g_flare.pending&&!g_flare.shot.pending;
  contact=allowed&&g_twoHand; // native support stays intact while arming
  if(allowed&&!fc3hands303::SupportedSidearm315(g_weaponMechanicProfile)){
   const int other=1-held.hand;const auto& attachment=g_freeHandAttachment[other];
   if(attachment.epoch==g_armPoseEpoch&&attachment.profile==g_weaponMechanicProfile){
    Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[other][i];
    const Mat4 wrist=Multiply(controller,attachment.relative);
    const float* hand=RevolverModel()?g_revolver.hand:FlareModel()?kFlareHand:g_pistolGuides.hand;
    const double diameter=RevolverModel()?g_revolver.handDiameter:FlareModel()?kFlareHandDiameter:g_pistolGuides.handDiameter;
    const V3 sphere=Apply(wrist,{other?-.591+hand[0]:.591-hand[0],.011+hand[1],1.149+hand[2]});
    Mat4 inverse={};if(Invert(root,inverse)){
     const V3 local=Apply(inverse,sphere),centre=Apply(grip,{.6781,.0519,1.0387});
     const double distance=Length(Sub(local,centre)),radius=.5*diameter*(std::max)(.1f,g_worldScale);
     contact=std::isfinite(radius)&&radius>0&&distance<=radius+.04*(std::max)(.1f,g_worldScale);
     if(RevolverModel())for(int b=0;b<2;++b)if(RevolverBoxContact(b,local,radius)&&Length(Sub(local,RevolverBoxCentre(b)))<=distance)contact=false;
     if(SkorpionModel()){
      const V3 port={g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]},charge=PistolRackGuide();
      if((PistolGuideContact(root,0,sphere)&&Length(Sub(local,port))<=distance)||(PistolGuideContact(root,1,sphere)&&Length(Sub(local,charge))<=distance))contact=false;
      reloadContact332=(PistolGuideContact(root,0,sphere)&&Length(Sub(local,port))<=distance)||(PistolGuideContact(root,1,sphere)&&Length(Sub(local,charge))<=distance);
     }
     if(FlareModel())for(int b=0;b<2;++b){const Mat4 box=FlareBox(b);const V3 q={box.m[3],box.m[7],box.m[11]};if(Length(Sub(local,q))<=distance&&Length(Sub(local,q))<=radius+.08)contact=false;}
    }
   }
  }
 }
 if(fc3hands303::Smg332(g_weaponMechanicProfile)){PublishSmgTransfer332(held,enabled,contact,allowed,false,reloadContact332);return;}
 //1911/Shadow,6P9 and Desert Eagle: retain both entry methods. Without the owning grip, the original
 // native support contact + receiving grip/pull remains available. Holding the
 // owning grip selects the optional .44-style modifier/pull instead. Once a
 // receiver is armed, keep that entry policy until it commits or cancels.
 float rightGrip327=0,leftGrip327=0,rightTrigger327=0,leftTrigger327=0;
 fc3hands303::RawControls(rightGrip327,leftGrip327,rightTrigger327,leftTrigger327);
 const auto prior327=fc3hands303::TransferContact307();
 const bool armed327=held.receiver>=0&&prior327.enabled&&prior327.entity==held.entity&&prior327.generation==held.generation;
 const bool modifier326=fc3hands303::SupportedSidearm315(g_weaponMechanicProfile)&&held.hand>=0&&held.hand<2&&
  (armed327?prior327.modifier321:(held.hand?leftGrip327:rightGrip327)>.35f);
 fc3hands303::PublishTransfer307(enabled,modifier326?allowed:contact,allowed,held.generation,held.entity,g_armPoseEpoch,false,modifier326);
}
void Publish1911Transfer307(){PublishHandleBinding333();}
