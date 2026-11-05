//Scoped1911 ownership adapter: keep the native reload transaction canonical,
//but source its contact and carried magazine from the physical RIGHT reload hand.
bool Left1911Reload312(){
 const auto held=fc3hands303::Read();return (fc3hands303::MagazineSidearm315(g_weaponMechanicProfile)||(fc3hands303::MagazineRifle317(g_weaponMechanicProfile)||fc3hands303::OtherMagazine319(g_weaponMechanicProfile)))&&g_frame-g_weaponMechanicFrame<=2&&held.enabled&&held.active&&held.engaged&&!held.empty&&held.hand==1&&held.nativeHoldingSide==0&&!MacheteHeld();
}
V3 PhysicalReloadWrist312(const V3& fallback){
 if(!Left1911Reload312()||!g_physicalValid303[0])return fallback;
 return ControllerWrist(g_physicalBody303[0],0);
}
void RecordRightReload312(ID3D11DeviceContext* context,int side,const float* bones){
 if(side!=0||!Left1911Reload312()||!g_physicalValid303[0]||!g_physicalValid303[1])return;
 Mat4 view={},inverse={},mirror=HandIdentity();mirror.m[0]=-1;
 if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
 auto& p=g_pistolReload;const Mat4 right=Multiply(inverse,view);
 //Canonical mechanics address negative-X left bind points. A bind reflection
 //lets those queries describe the actual physical-right palm without guessing.
 p.palmBody=Multiply(right,mirror);p.leftPalm=Apply(p.palmBody,{-.591,.011,1.149});
 p.tip=Apply(p.palmBody,{-.591+g_pistolGuides.hand[0],.011+g_pistolGuides.hand[1],1.149+g_pistolGuides.hand[2]});
 //Mirror the hand-to-magazine attachment on both sides; the magazine stays a
 //proper native model transform, with its wrist/fingers on the other hand.
 p.hand=Multiply(view,Multiply(mirror,Multiply(MagazineHeldFromHand(),mirror)));
 p.palmFrame=p.handFrame=g_frame;ObservePistolGuidePalm(p.palmBody);
}
bool RightReloadSlide312(const PhysicalArmScope303& scope,int side,Mat4& desired){
 if(side!=0||!scope.active||!Left1911Reload312()||!g_pistolReload.rackHeld||g_pistolReload.held||g_pistolReload.removalHeld||g_pistolReload.pending||!g_physicalValid303[0])return false;
 Mat4 hand={},root=g_pistolReload.rootBody,mirror=HandIdentity();mirror.m[0]=-1;float fingers[204];
 if(SkorpionModel()){
  hand=kSkorpionChargingWristFromFrame52;const V3 pinch=Apply(hand,kSkorpionPinch229),knob={.026,.14320966,.04219692};
  hand.m[3]+=knob.x-pinch.x;hand.m[7]+=knob.y-pinch.y;hand.m[11]+=knob.z-pinch.z;hand.m[7]-=(std::max)(0.0,(std::min)(.05,g_pistolReload.slidePull));
  mirror.m[3]=A2000LeftCharge336()?0:2*knob.x; // physical charging knob stays on its actual side of the gun
 }else if(!RifleChargeClipSample(g_pistolReload.slidePull/.05,hand,fingers))return false;
 if(g_weaponMechanicProfile==20&&!Resolve1911Insertion(g_pistolReload.rootBody,root,false))return false;
 // Keep each rifle's charging mechanism on its actual native side.
 if(fc3hands303::MagazineRifle317(g_weaponMechanicProfile)&&CurrentRifle().partRest[0]&&!A2000LeftCharge336()){
  double handleX=CurrentRifle().partRest[0]->m[3];
  if(g_weaponMechanicProfile==28||g_weaponMechanicProfile==25||g_weaponMechanicProfile==21){const auto parts=CurrentSkinnedParts();handleX=PistolRigidBlend(*parts.handleRest,*parts.handleBack,g_pistolReload.slidePull/.05).m[3];}
  mirror.m[3]=2*handleX;
 }
 Mat4 bind=SidearmMirror315();
 desired=Multiply(ArmBodyToCamera(),Multiply(root,Multiply(mirror,Multiply(hand,bind))));return true;

}
bool RightReloadFingers312(int side,float* bones){
 if(side!=0||!Left1911Reload312())return false;
 const auto& p=g_pistolReload;const float* fingers=nullptr;
 if(p.held||p.removalHeld)fingers=(fc3hands303::MagazineRifle317(g_weaponMechanicProfile)||fc3hands303::OtherMagazine319(g_weaponMechanicProfile))?CurrentRifle().magazineGrip:PistolMagazineGripCur();
 else if(p.rackHeld)fingers=fc3hands303::MagazineRifle317(g_weaponMechanicProfile)?RifleChargeFingers():SkorpionModel()?kSkorpionChargingGripRelative:RifleChargeFingers();
 if(!fingers)return false;
 Mat4 mirror=HandIdentity();mirror.m[0]=-1;const Mat4 wrist=BoneMatrix(bones,2);
 for(int bone=3;bone<=19;++bone)StoreBone(bones,bone,Multiply(wrist,Multiply(mirror,Multiply(BoneMatrix(fingers,bone-3),mirror))));
 return true;
}
