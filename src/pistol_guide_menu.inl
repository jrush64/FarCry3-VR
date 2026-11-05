bool KeyPressed(int);bool R700TwoHandGuide(V3&,V3&,bool&);bool R700BoltBoxDue();bool R700RightPalmBody(V3&);bool R700Grabbing();
V3 PistolHandGuide()
{
    auto& p=g_pistolReload;
    return Apply(p.palmBody,{-.591+g_pistolGuides.hand[0],.011+g_pistolGuides.hand[1],1.149+g_pistolGuides.hand[2]});
}

// Observe completed displayed meshes. Calibration observations never authorize
// actions. Gameplay contact may share its own fresh draw root; reload ownership,
// native ammo authority and fire readiness remain separate guarded state.
struct PistolGuidePreview {Mat4 root={},palm={},physicalRoot={};LONG rootFrame=-100000,palmFrame=-100000,profile=-1;UINT epoch=0;int weaponTarget=0;bool physicalKnown=false;}g_pistolGuidePreview;
bool NativeWeaponContactRoot51(const Mat4&,Mat4&);
Mat4 PistolInteractionRoot(const Mat4& source){
 const auto& p=g_pistolReload;const auto& draw=g_pistolGuidePreview;
 Mat4 captured={};if(NativeWeaponContactRoot51(source,captured))return captured;
 // Only the current actionable weapon's completed draw may refine its contact
 // root. Calibration previews and stale/other-weapon observations cannot.
 if(&source==&p.rootBody&&!g_pistolMenu.open&&p.rootEpoch==g_armPoseEpoch&&p.rootFrame==g_frame&&draw.epoch==g_armPoseEpoch&&draw.profile==g_weaponMechanicProfile&&draw.rootFrame==g_frame&&draw.physicalKnown&&memcmp(&draw.physicalRoot,&source,sizeof(source))==0)return draw.root;
 return source;
}
Mat4 PistolGuideDrawToBody(const CameraSummary& camera,int variant)
{
 Mat4 inverse={};if(!Invert(ArmBodyToCamera(),inverse))return {};
 Mat4 place={};place.m[0]=place.m[5]=place.m[10]=place.m[15]=1;
 if(variant>=0&&variant<kEyeVariants&&g_frame-g_guideShaderPlacementFrame[variant]<=1)place=g_guideShaderPlacement[variant];
 return Multiply(inverse,Multiply(place,HullView(camera)));
}
bool PistolGuideCamera(ID3D11DeviceContext* context,CameraSummary& camera)
{
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);UINT bytes=sizeof(camera);
 const bool ok=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera);SafeRelease(cb);return ok;
}
 #include "pistol_frame_anchor45.inl"
 #include "pistol_trace46.inl"
void ObservePistolGuideRoot(ID3D11DeviceContext* context,UINT start,UINT count,const float* globals,int variant)
{
 if(!g_pistolMenu.open&&!g_pistolGuides.preview&&!PistolHandling())return;
 if(AkModel()){if(CurrentRifle().skinned||!AkFrame(start,count))return;} // MP5: mp5.inl reports its root
 else if(SkorpionModel()){if(!(start==2016&&count==1056)&&!(start==3072&&count==10347))return;}
 else if(!PistolMeshFrame(start,count))return;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(AkModel()){if(!RifleMain(CurrentRifle(),d))return;}
 else if(SkorpionModel()){if(d.vertexBytes!=157984||d.indexBytes!=35322||d.stride!=32||d.offset!=0||d.format!=57)return;}
 else if(!PistolMeshMain(d))return;
 CameraSummary camera={};if(!PistolGuideCamera(context,camera))return;
 auto& preview=g_pistolGuidePreview;
 if(preview.epoch!=g_armPoseEpoch){preview=PistolGuidePreview{};preview.epoch=g_armPoseEpoch;}
 const Mat4 root=Multiply(PistolGuideDrawToBody(camera,variant),HullWorld(globals));Mat4 inverse={};if(!Invert(root,inverse))return;
 preview.root=root;preview.rootFrame=g_frame;preview.profile=g_weaponMechanicProfile;
 preview.physicalKnown=g_pistolReload.rootFrame==g_frame&&g_pistolReload.rootEpoch==g_armPoseEpoch&&g_pistolReload.rootContext==context;
 if(preview.physicalKnown)preview.physicalRoot=g_pistolReload.rootBody;
}
void ObservePistolGuidePalm(const Mat4& bodyPalm)
{
 if(!g_pistolMenu.open&&!g_pistolGuides.preview&&!PistolHandling())return;
 auto& p=g_pistolGuidePreview;if(p.epoch!=g_armPoseEpoch){p=PistolGuidePreview{};p.epoch=g_armPoseEpoch;}
 p.palm=bodyPalm;p.palmFrame=g_frame;
}
void ObservePistolGuideDraw(ID3D11DeviceContext* context,const DrawCall& call,int variant)
{
 if((!g_pistolMenu.open&&!g_pistolGuides.preview&&!PistolHandling())||call.kind!=0||call.d!=0||(variant!=1&&variant!=3&&variant!=4))return;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);float globals[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,globals,sizeof(globals));SafeRelease(cb);
 if(bytes<48)return;
 // BuildWeaponWorld already observed the completed solid upload. The original
 // CB is still bound here, so it must not replace that same-frame final root.
 if(!(g_pistolReload.rootFrame==g_frame&&g_pistolReload.rootContext==context)&&!(variant==1&&g_pistolGuidePreview.rootFrame==g_frame))ObservePistolGuideRoot(context,call.b,call.a,globals,variant);
 if(Left1911Reload312())return; // physical right reload hand is the only gameplay contact source
 if(variant!=3||g_pistolReload.palmFrame==g_frame)return; // fallback left-arm draw after live tracking has stopped
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const bool arm=ShaderHash(vs)==kArmVsHash;SafeRelease(vs);if(!arm)return;
 context->VSGetConstantBuffers(3,1,&cb);float bones[25*12]={};const UINT boneBytes=ReadCopy(cb,bones,sizeof(bones));SafeRelease(cb);
 CameraSummary camera={};if(boneBytes<36*sizeof(float)||!PistolGuideCamera(context,camera))return;
 ObservePistolGuidePalm(Multiply(PistolGuideDrawToBody(camera,variant),Multiply(HullWorld(globals),BoneMatrix(bones,2))));
}
void ObserveMp5GuideRoot(const Mat4& rootBody)
{
 if(!g_pistolMenu.open&&!g_pistolGuides.preview&&!PistolHandling())return;
 auto& preview=g_pistolGuidePreview;if(preview.epoch!=g_armPoseEpoch){preview=PistolGuidePreview{};preview.epoch=g_armPoseEpoch;}
 preview.root=rootBody;preview.rootFrame=g_frame;preview.profile=g_weaponMechanicProfile;
 preview.physicalRoot=rootBody;preview.physicalKnown=true;
}
bool PistolMenuGuidePoints(V3& port,V3& hand,float& radius,bool& palm)
{
 auto& p=g_pistolGuidePreview;const float scale=(std::max)(.1f,g_worldScale);
 const int selectedFamily=g_menuGuideFamily;struct RestoreFamily{int family;~RestoreFamily(){g_menuGuideFamily=family;}} restore{selectedFamily};
 if(g_pistolMenu.open)g_menuGuideFamily=MagazineFamilyOfProfile(g_weaponMechanicProfile);
 if(!PhysicalReloadGuidesEnabled()||!MagazineProfile(g_weaponMechanicProfile)||(!g_pistolMenu.open&&!PistolModel())||p.epoch!=g_armPoseEpoch||p.profile!=g_weaponMechanicProfile||p.rootFrame<0||(!g_pistolMenu.open&&g_frame-p.rootFrame>2))return false;
 const bool chargeBox=AkModel()&&(CurrentRifle().emptyNeedsCharge||R700Model());
 if(!chargeBox&&p.weaponTarget==3)p.weaponTarget=0;
 if(g_pistolMenu.target==0||g_pistolMenu.target==1||(chargeBox&&g_pistolMenu.target==3))p.weaponTarget=g_pistolMenu.target;

 const bool slide=PistolEffectiveGuideTarget(p.weaponTarget)==1;
 const float* value=PistolGuidePoint(p.weaponTarget);const V3 local={value[0],value[1],value[2]};
 Mat4 root=p.root;if(p.weaponTarget==0&&!Resolve1911Insertion(p.root,root,true))return false;
 port=Apply(root,local);
 hand=Apply(p.palm,{-.591+g_pistolGuides.hand[0],.011+g_pistolGuides.hand[1],1.149+g_pistolGuides.hand[2]});
 palm=p.palmFrame>=0;radius=(slide?g_pistolGuides.slideRadius:g_pistolGuides.portRadius)*scale;return true;
}
void PistolMenuGuideMarkers()
{
 V3 port={},hand={};float radius=0;bool palm=false;
 if(!PistolMenuGuidePoints(port,hand,radius,palm)){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 float a[3]={static_cast<float>(port.x),static_cast<float>(port.y),static_cast<float>(port.z)},b[3]={static_cast<float>(hand.x),static_cast<float>(hand.y),static_cast<float>(hand.z)};
 fc3xr::SetReloadMarkers(a,palm?b:nullptr,radius,0,g_pistolGuides.handDiameter);
 const int selected=g_menuGuideFamily;g_menuGuideFamily=MagazineFamilyOfProfile(g_weaponMechanicProfile);
 NativeReloadPoints50(port,hand,radius,g_pistolGuides.handDiameter,true,palm,g_pistolGuidePreview.weaponTarget);
 PistolBoxMarker(g_pistolGuidePreview.root,g_pistolGuidePreview.weaponTarget,0);g_menuGuideFamily=selected;
 fc3xr::SetHandSphere();
}
int PistolGameplayGuideTarget()
{
    if(g_weaponTriggerDiagnostics)return g_weaponTriggerTarget==2?3:g_weaponTriggerTarget;
    const auto& p=g_pistolReload;
    if(R700BoltBoxDue())return 3; // R700 physical: gun in the left hand, bolt due
    if(AkModel()&&(p.needsRack||p.rackHeld||p.rackReturning))return 3;
    return p.removalHeld||(AkModel()&&fc3pad::Latest().leftGrip>.7f&&p.magazinePresent&&!p.held&&!p.pending&&!p.cycle)?1:
        p.rackMode||p.rackHeld||p.rackReturning||(SkorpionModel()&&p.needsRack)?1:0;
}
void PistolGuideMarkers()
{
    if(g_pistolMenu.open){PistolMenuGuideMarkers();return;}
    if(!PhysicalReloadGuidesEnabled()){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
    auto& p=g_pistolReload;const float scale=(std::max)(.1f,g_worldScale);
    V3 port={},hand=PistolHandGuide();float radius=0;int boxTarget=0;bool show=false,showHand=false;
    if(PistolGameplayGuideTarget()==3)
    {boxTarget=3;if(g_frame-p.rootFrame<=1){port=Apply(p.rootBody,PistolRackGuide());radius=.05f*scale;show=true;showHand=g_frame-p.palmFrame<=1;}}
    else if(PistolGameplayGuideTarget()==1)
    {boxTarget=1;if(g_frame-p.rootFrame<=1){const float* at=PistolGuidePoint(1);port=Apply(p.rootBody,{at[0],at[1],at[2]});radius=(PistolEffectiveGuideTarget(1)==0?g_pistolGuides.portRadius:g_pistolGuides.slideRadius)*scale;show=true;showHand=g_frame-p.palmFrame<=1;}}
    else if(PistolGameplayGuideTarget()==0&&g_frame-p.rootFrame<=1)
    {port=Apply(p.rootBody,{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]});radius=g_pistolGuides.portRadius*scale;show=true;showHand=g_frame-p.palmFrame<=1;}
    if(R700BoltBoxDue()){V3 rp={};if(R700RightPalmBody(rp)){hand=rp;showHand=true;}} // the RIGHT hand works the bolt: its palm sphere
    const bool grabGuide=false,grabInRange=false; // the two-hand grab gizmo is drawn for every weapon by UpdateTwoHand (GrabGizmo)
    float a[3]={static_cast<float>(port.x),static_cast<float>(port.y),static_cast<float>(port.z)};
    float b[3]={static_cast<float>(hand.x),static_cast<float>(hand.y),static_cast<float>(hand.z)};
    const bool contact=g_weaponTriggerDiagnostics?showHand&&PistolGuideContact(p.rootBody,boxTarget,p.tip):(grabGuide&&grabInRange)||p.rackHeld||R700Grabbing()||(p.held&&PistolGuideContact(p.rootBody,boxTarget,p.tip));
    // Use final displayed mesh anchors for visuals, never for reload ownership.
    const auto& displayed=g_pistolGuidePreview;
    const bool drawn=displayed.epoch==g_armPoseEpoch&&displayed.rootFrame==g_frame;
    const bool drawnHand=displayed.epoch==g_armPoseEpoch&&displayed.palmFrame==g_frame;
    Mat4 visualRoot=PistolInteractionRoot(p.rootBody);
    if(boxTarget==0&&!Resolve1911Insertion(p.rootBody,visualRoot,false)){show=showHand=false;}
    if(show){const float* c=PistolGuidePoint(boxTarget);port=Apply(visualRoot,{c[0],c[1],c[2]});}
    // Use the physical detector's root/hand; an independent visual pose can
    // make a guide look attached while its actual contact zone is elsewhere.
    
    a[0]=static_cast<float>(port.x);a[1]=static_cast<float>(port.y);a[2]=static_cast<float>(port.z);
    b[0]=static_cast<float>(hand.x);b[1]=static_cast<float>(hand.y);b[2]=static_cast<float>(hand.z);
    fc3xr::SetReloadMarkers(show?a:nullptr,showHand?b:nullptr,radius,contact?1:0,grabGuide?.04f:R700BoltBoxDue()?g_r700.rightDiameter:g_pistolGuides.handDiameter);
    NativeReloadPoints50(port,hand,radius,R700BoltBoxDue()?g_r700.rightDiameter:g_pistolGuides.handDiameter,show,showHand,boxTarget);
    if(show&&!grabGuide&&(drawn||g_frame-p.rootFrame<=1))PistolBoxMarker(visualRoot,boxTarget,contact?1:0);
    if(show&&AkModel())
    { // [GUIDE BOX] probe: is the box glued to the drawn gun (box-in-gun constant) and how does the gun move against the
      // controller? Every 10th frame, 60 lines per session.
     static LONG lines=0;
     if(lines<60&&g_frame%10==0)
     {
      ++lines;const Mat4 box=PistolGuideBox(visualRoot,boxTarget);Mat4 rootInverse={},controller={},controllerInverse={};
      for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
      if(Invert(visualRoot,rootInverse)&&Invert(controller,controllerInverse))
      {
       const Mat4 boxInGun=Multiply(rootInverse,box),gunInHand=Multiply(controllerInverse,visualRoot);
       Log("[GUIDE BOX] %ls target %d | box in gun pos %.3f %.3f %.3f x-axis %.3f %.3f %.3f | gun in controller pos %.3f %.3f %.3f y-axis %.3f %.3f %.3f | gun y-axis body %.3f %.3f %.3f | root age %ld drawnRoot %d twoHand %d poseOwned %d held %d",
        CurrentRifle().name,boxTarget,boxInGun.m[3],boxInGun.m[7],boxInGun.m[11],boxInGun.m[0],boxInGun.m[4],boxInGun.m[8],
        gunInHand.m[3],gunInHand.m[7],gunInHand.m[11],gunInHand.m[1],gunInHand.m[5],gunInHand.m[9],visualRoot.m[1],visualRoot.m[5],visualRoot.m[9],
        g_frame-p.rootFrame,drawn?1:0,g_twoHand?1:0,PistolPoseOwned()?1:0,p.held?1:0);
      }
     }
    }
      if(showHand)fc3xr::SetHandSphere();
}
bool TickPistolGuideMenu(const fc3pad::Controllers& c,bool)
{
 if(!g_pistolMenu.open)return false;
 auto& p=g_pistolReload;p.x=c.x;PistolGuideMarkers();
 // Let an already accepted engine transaction/rack settle; never create a new one.
 return true;
}

