#include "pistol_mechanics_data.inl"
#include "pistol_right_grip_data.inl"
void TestPistolMechanics(ID3D11Device* device,ID3D11DeviceContext* context)
{
    const auto prior=g_pistolReload;const auto savedFinal50=g_1911FrameFinal;g_1911FrameFinal={};float savedHand50[16];memcpy(savedHand50,g_handBody[1],sizeof(savedHand50));
    auto rawWrist50=[&](V3 point){for(int i=0;i<16;++i)g_handBody[1][i]=i%5==0?1.f:0.f;g_handBody[1][3]=float(point.x-g_wristOffset[1][0]);g_handBody[1][7]=float(point.y-g_wristOffset[1][1]);g_handBody[1][11]=float(point.z-g_wristOffset[1][2]);};
    g_pistolReload=PistolReloadState{};auto& p=g_pistolReload;p.enabled=true;
    g_motionGun=g_handRightValid=g_handLeftValid=true;g_weaponMotionGuard=false;
    g_weaponMechanicProfile=20;g_weaponMechanicFrame=++g_frame;
    fc3pad::Controllers c={};c.active=true;c.leftGrip=1;c.rightTrigger=1;fc3pad::Publish(c);
    fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);
    // Repeated eject presses cannot restart departure, reload or reopen firing.
    { // the X eject drops only a magazine that is not full: give this pistol a HUD widget showing 5/7 with spare ammo
     static BYTE ammoPage[12288]={};const UINT wrapper=reinterpret_cast<UINT>(ammoPage),weapon=wrapper+4096,hud=wrapper+8192;
     *reinterpret_cast<UINT*>(wrapper+0x48)=weapon;*reinterpret_cast<UINT*>(weapon+0xcc)=5;const UINT w[5]={5,7,0,30,35};memcpy(reinterpret_cast<void*>(hud+0x134),w,sizeof(w));
     ObserveAmmoWidget(wrapper,hud);p.ammoWrapper=wrapper;}
    c.leftGrip=1;c.rightGrip=0;c.x=true;p.clipFrame=g_frame;p.clipWorld=HandIdentity();fc3pad::Publish(c);TickPistolReload(true);
    Check(!p.magazinePresent&&p.dropActive,"first eject starts one visible departure");const ULONGLONG firstDrop=p.dropAt;
    for(int i=0;i<1000;++i){c.x=(i%3)!=0;fc3pad::Publish(c);TickPistolReload(true);Check(!p.magazinePresent&&!p.pending&&!p.cycle&&p.dropAt==firstDrop&&PistolBusy(),"held/repeated eject cannot restart drop, native reload or fire");}
    p=PistolReloadState{};p.enabled=true;c.x=false;c.leftGrip=1;fc3pad::Publish(c);
    p.magazinePresent=p.needsRack=true;p.rootBody=HandIdentity();p.rootFrame=p.palmFrame=g_frame;p.rootEpoch=g_armPoseEpoch;
    p.tip=PistolRackGuide();rawWrist50(p.tip);AdvancePistolRack(c,960);Check(!p.rackHeld,"ordinary grip held cannot acquire slide");
    c.leftGrip=0;AdvancePistolRack(c,980);c.leftGrip=1;AdvancePistolRack(c,1000);Check(p.rackHeld,"fresh grip acquires required slide without X");
    c.rightGrip=0;AdvancePistolRack(c,1000);Check(p.rackMode&&p.rackHeld&&!p.rackRear,"grip X explicitly acquires slide with seated magazine");
    p.tip.y-=.02;rawWrist50(p.tip);AdvancePistolRack(c,1020);c.leftGrip=0;AdvancePistolRack(c,1040);
    Check(p.needsRack&&!p.rackReturning&&p.rackMode&&!p.rackHeld,"short required stroke releases hand and stays ready for retry without chambering");
    AdvancePistolRack(c,1050);c.leftGrip=1;p.tip=PistolRackGuide();rawWrist50(p.tip);AdvancePistolRack(c,1060);p.tip.y-=.045;rawWrist50(p.tip);AdvancePistolRack(c,1080);
    Check(p.rackRear&&p.slidePull>.04,"rear threshold requires local backwards travel");
    c.leftGrip=0;AdvancePistolRack(c,1100);Check(p.needsRack&&p.rackReturning,"release waits for actual forward return");
    AdvancePistolRack(c,1150);Check(p.needsRack,"half return cannot fire");AdvancePistolRack(c,1200);
    Check(!p.needsRack&&!p.rackMode&&p.waitTriggerRelease,"complete return exits mode and cannot fire held trigger");
    fc3pad::SetMagazineHandling(true,PistolBusy());fc3pad::Publish(c);Check(!fc3pad::FireOutput(),"actual RT inhibited until trigger release");
    c.rightTrigger=0;AdvancePistolRack(c,1220);fc3pad::SetMagazineHandling(true,PistolBusy());c.rightTrigger=1;fc3pad::Publish(c);Check(fc3pad::FireOutput()==255,"new trigger pull fires after manual rack");
    // Releasing grip while the native root is absent must still end ownership.
    c.leftGrip=1;p.tip=PistolRackGuide();rawWrist50(p.tip);PistolXAction(c,true);AdvancePistolRack(c,1240);Check(p.rackHeld,"explicit optional grip acquired");
    p.rootBody={};p.palmFrame=g_frame-3;c.leftGrip=0;c.rightTrigger=0;AdvancePistolRack(c,1260);
    Check(!p.rackHeld&&!p.rackMode&&!PistolOwnsLeft()&&!PistolBusy(),"stale singular root cannot latch slide ownership or fire inhibition on release");
    // Full return finishes even after the hand leaves the rendered frame.
    p.rootBody=HandIdentity();p.palmFrame=g_frame;c.leftGrip=1;p.tip=PistolRackGuide();rawWrist50(p.tip);PistolXAction(c,true);AdvancePistolRack(c,1280);
    p.tip.y-=.045;rawWrist50(p.tip);AdvancePistolRack(c,1300);p.palmFrame=g_frame-3;c.leftGrip=0;AdvancePistolRack(c,1320);AdvancePistolRack(c,1420);
    Check(!p.rackReturning&&!p.rackMode&&!PistolBusy(),"verified released slide return never waits forever for mesh freshness");p.palmFrame=g_frame;
    // Racking follows the pistol frame even after a 90-degree rotation.
    c.leftGrip=1;p.rootBody=HandIdentity();p.rootBody.m[0]=p.rootBody.m[5]=0;p.rootBody.m[1]=-1;p.rootBody.m[4]=1;
    p.tip=Apply(p.rootBody,PistolRackGuide());rawWrist50(p.tip);PistolXAction(c,true);AdvancePistolRack(c,1500);
    V3 rear=PistolRackGuide();rear.y-=.045;p.tip=Apply(p.rootBody,rear);rawWrist50(p.tip);AdvancePistolRack(c,1520);
    Check(p.rackRear&&p.slidePull>.04,"yawed pistol uses its own rail instead of world left/right");
    // Retain native contact-capture guards.1911/Shadow now samples the native pistol grasp;
    // this finger pass preserves the wrist established by the separate bounded arm pass.
    float rackBones[576]={};StoreBone(rackBones,2,HandIdentity());
    for(int b=3;b<=19;++b)StoreBone(rackBones,b,BoneMatrix(kPistolGripRelative,b-3));
    p.rackHeld=false;p.cycle=true;p.nativeSlidePull=.03;p.nativeSlideFrame=g_frame;p.rootNative=HandIdentity();p.rootFrame=g_frame;
    ID3D11Buffer* gripGlobals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);float gripWorld[88]={};gripWorld[0]=gripWorld[5]=gripWorld[10]=1;gripGlobals->SetPrivateData(kCbCopyGuid,sizeof(gripWorld),gripWorld);context->VSSetConstantBuffers(0,1,&gripGlobals);
    CapturePistolSlideGrip(context,rackBones);Check(!p.slideGripValid,"slide recoil without native palm contact is rejected as rack pose");
    Mat4 nativeWrist=HandIdentity();V3 nativeGrab=PistolRackGuide();nativeWrist.m[3]=nativeGrab.x+.591;nativeWrist.m[7]=nativeGrab.y-.011;nativeWrist.m[11]=nativeGrab.z-1.149;StoreBone(rackBones,2,nativeWrist);
    for(int b=3;b<=19;++b)StoreBone(rackBones,b,Multiply(nativeWrist,BoneMatrix(kPistolGripRelative,b-3)));
    CapturePistolSlideGrip(context,rackBones);gripGlobals->Release();
    Check(p.slideGripValid,"native rack phase captures wrist-relative finger pose");
    p.cycle=false;p.rackHeld=true;p.held=false;Mat4 trackedWrist=HandIdentity();trackedWrist.m[3]=.37;StoreBone(rackBones,2,trackedWrist);
    Check(PistolGrip(rackBones),"rack uses sampled native finger pose");
    Check(std::fabs(BoneMatrix(rackBones,2).m[3]-.37)<.00001,"native grip pose preserves controller wrist position");
    Mat4 sampledWrist248={};float sampledFingers248[204];RifleSampleKeys(kD50ClipEmptyReload,kD50ClipEmptyReloadCount,kD50EmptySlidePullStartAt+(p.slidePull/.05)*(kD50EmptySlideBackAt-kD50EmptySlidePullStartAt),sampledWrist248,sampledFingers248);
    for(int b=3;b<=19;++b){Mat4 expected=Multiply(trackedWrist,BoneMatrix(sampledFingers248,b-3));for(int i=0;i<12;++i)Check(std::fabs(rackBones[b*12+i]-expected.m[i])<.00001,"248 rack fingers follow the native pistol grasp sample");}
    p.slideGripValid=false;p.cycle=true;p.nativeSlideFrame=g_frame-3;CapturePistolSlideGrip(context,rackBones);Check(!p.slideGripValid,"stale native rack phase cannot capture fingers");p.cycle=false;
    // Idle support grip: saved ShowGuides never activates slide or gameplay guides.
    p.slideGripValid=false;p.needsRack=p.emptyLocked=false;p.rackMode=p.rackHeld=p.rackReturning=p.rackArmed=false;p.magazinePresent=true;
    p.rootBody=HandIdentity();p.rootFrame=p.palmFrame=g_frame;p.tip=PistolRackGuide();c.leftGrip=1;c.rightTrigger=0;c.x=false;
    auto oldGuide=g_pistolGuides;g_pistolGuides.preview=true;
    for(int i=0;i<1000;++i){AdvancePistolRack(c,2000+i);Check(!PistolOwnsLeft()&&!PistolBusy()&&PistolGameplayGuideTarget()==0,"ordinary support grip never owns slide; idle guide shows insertion only");}
    const auto oldRight=g_armJoints[0],oldLeft=g_armJoints[1];const bool oldEye=g_eyeViewsSet,oldTwo=g_twoHand;
    const bool oldAuto=g_autoPistolSupport;g_autoPistolSupport=false;
    g_armJoints[0]={{0,0,0},{0,0,.2},{.1,.2,.4},true};g_armJoints[1]=g_armJoints[0];g_eyeViewsSet=true;g_twoHand=true;fc3pad::Publish(c);UpdateTwoHand();Check(g_twoHand,"actual two-hand update retains support grip while pistol slide mode is off");
    {
     float savedLeft[16];memcpy(savedLeft,g_handLeft,sizeof(savedLeft));
     float savedBody[16];memcpy(savedBody,g_handBody[1],sizeof(savedBody));
     g_autoPistolSupport=true;g_twoHand=false;c.leftGrip=0;fc3pad::Publish(c);
     Mat4 supportMove={};Check(GunMove(g_armJoints[0],g_armJoints[1],supportMove),"pistol unsupported grip reference valid");
     const auto& lj=g_armJoints[1];const V3 support=Apply(supportMove,{lj.w[0],lj.w[1],lj.w[2]});
     for(int i=0;i<16;++i)g_handLeft[i]=(i%5==0)?1.0f:0.0f;
     g_handLeft[3]=float(support.x-g_wristOffset[1][0]);g_handLeft[7]=float(support.y-g_wristOffset[1][1]);g_handLeft[11]=float(support.z-g_wristOffset[1][2]);
     Mat4 bodyInverse={},cameraHand={};Check(Invert(ArmBodyToCamera(),bodyInverse),"support fixture body conversion");for(int i=0;i<16;++i)cameraHand.m[i]=g_handLeft[i];
     Mat4 bodyHand=Multiply(bodyInverse,cameraHand);for(int i=0;i<16;++i)g_handBody[1][i]=float(bodyHand.m[i]);
     g_frameGunMove=supportMove;V3 target={},palm={};Check(GrabGizmoPoints(support,target,palm),"support fixture actual contact centers");
     const V3 correction=Sub(target,palm);g_handBody[1][3]+=float(correction.x);g_handBody[1][7]+=float(correction.y);g_handBody[1][11]+=float(correction.z);
     for(int i=0;i<16;++i)bodyHand.m[i]=g_handBody[1][i];cameraHand=Multiply(ArmBodyToCamera(),bodyHand);for(int i=0;i<16;++i)g_handLeft[i]=float(cameraHand.m[i]);
     UpdateTwoHand();Check(g_twoHand,"pistol automatically grabs near support without grip");
     UpdateTwoHand();Check(g_twoHand,"pistol automatic support does not release every other frame");
     g_handLeft[3]+=.5f;g_handBody[1][3]+=.5f;UpdateTwoHand();Check(!g_twoHand,"pistol automatic support releases when hand moves away");
     memcpy(g_handLeft,savedLeft,sizeof(savedLeft));c.leftGrip=1;fc3pad::Publish(c);
     memcpy(g_handBody[1],savedBody,sizeof(savedBody));
     puts("PASS: controls22 actual pistol auto contact, stable hold and pull-away release; manual pistol option retains grip ownership");
    }
    g_autoPistolSupport=oldAuto;
    g_armJoints[0]=oldRight;g_armJoints[1]=oldLeft;g_eyeViewsSet=oldEye;g_twoHand=oldTwo;
    PistolXAction(c,true);AdvancePistolRack(c,3100);Check(p.rackHeld&&PistolGameplayGuideTarget()==1,"grip X shows slide guide only with seated magazine");
    float fallback[576]={};StoreBone(fallback,2,HandIdentity());Check(PistolGrip(fallback),"explicit slide has measured grasp fallback");
    p.tip.y-=.045;AdvancePistolRack(c,3120);c.leftGrip=0;AdvancePistolRack(c,3140);AdvancePistolRack(c,3240);
    Check(!PistolOwnsLeft()&&!PistolBusy()&&PistolGameplayGuideTarget()==0,"completed optional slide pull restores shooting/support grip while insertion guide stays visible");
    c.leftGrip=1;AdvancePistolRack(c,3260);Check(!p.rackHeld,"ordinary regrip cannot silently reacquire slide");
    p.magazinePresent=false;p.basis=true;PistolXAction(c,true);Check(p.held&&!p.rackMode&&PistolGameplayGuideTarget()==0,"same grip X picks magazine when ejected, never enables slide");
    PistolXAction(c,true);Check(p.held&&!p.rackMode&&!p.pending,"repeated grip X with held magazine cannot switch mode or reload");
    p.held=false;p.magazinePresent=false;p.pending=p.cycle=p.removalHeld=false;p.basis=true;p.nextAmmoAt=0;p.looseFlying=false;
    g_pistolGuides.ammoPickup=0;c.x=true;c.leftGrip=c.rightGrip=1;c.ammoAction=fc3pad::AmmoButtonAction::None;PistolXAction(c,true);
    Check(p.held&&!p.magazinePresent&&!p.rackMode&&!p.pending,"both-grip override takes magazine in body pickup mode without release or rack");
    PistolXAction(c,true);Check(p.held&&!p.looseFlying,"override cannot drop an occupied hand's magazine");
    c.rightGrip=0;p.held=false;g_pistolGuides=oldGuide;
    std::puts("PASS: explicit grip X isolates slide/pickup, 1000 ordinary support frames never steal grip, release handles stale/singular roots, full return finishes without fresh mesh and gameplay guides hide outside interaction");
    // Production loose-ammo runtime: one identity, contextual X, exact gravity and cooldown.
    const auto oldPlay=p;const auto oldPlaySettings=g_pistolGuides;const bool oldPlayAim=g_aimActive;const auto oldPlayView=g_eyeViewsBody[0];const double oldPlayYaw=g_bodyYaw;
    p=PistolReloadState{};p.enabled=true;p.basis=true;p.magazinePresent=false;p.hand=HandIdentity();p.palmFrame=p.handFrame=g_frame;
    g_aimActive=false;g_bodyYaw=0;g_pistolGuides.ammoPickup=2;g_pistolGuides.replacementDelay=1;g_pistolGuides.catchRadius=.15f;
    c.leftGrip=1;c.x=true;c.rightTrigger=0;PistolXAction(c,true);const unsigned clipIdentity=p.looseId;
    Check(p.held&&clipIdentity&&!p.looseFlying,"actual X dispatcher spawns one detached magazine");
    p.hand=HandIdentity();p.handVelocity={1*g_worldScale,2.4*g_worldScale,-.7*g_worldScale};p.handFrame=p.palmFrame=g_frame;PistolXAction(c,true);
    Check(p.held&&!p.looseFlying&&p.looseId==clipIdentity&&!p.pending,"pickup chord cannot throw an already held magazine");
    PistolThrowLoose(GetTickCount64(),false);Check(p.looseFlying&&!p.held,"physical release still throws same identity");
    const auto thrownAt=p.looseAt;const double oldScale=g_worldScale;g_worldScale=1;
    TickPistolLoose(c,thrownAt);Check(p.looseFlying&&!p.held&&!p.catchArmed,"held grip cannot immediately recatch outgoing magazine");
    TickPistolLoose(c,thrownAt+200);Check(std::fabs(p.looseBody.m[7]-(2.4*oldScale*.2-4.905*.04))<.001,"production flight integrates gravity on the rendered clip centre");
    Check(std::fabs(p.looseBody.m[3]-.2*oldScale)<.001&&std::fabs(p.looseBody.m[11]+.14*oldScale)<.001,"hand release velocity throws freely along horizontal and depth axes without upward boost");
    Check(!PistolTakeLoose(thrownAt+200)&&p.looseId==clipIdentity,"flying item prevents duplicate ammo pickup");
    p.hand=HandIdentity();p.hand.m[3]=3;p.handFrame=p.palmFrame=g_frame;PistolXAction(c,true);Check(p.looseFlying&&!p.held,"distant grip X cannot catch or spawn another item");
    p.hand=p.looseBody;p.handFrame=p.palmFrame=g_frame;c.x=false;TickPistolLoose(c,thrownAt+200);Check(p.held&&!p.looseFlying&&p.looseId==clipIdentity,"held grip automatically catches identical item without X or ammo grant");
    PistolXAction(c,true);Check(p.held&&!p.looseFlying,"same frame catch and X cannot immediately retoss");
    p.handFrame=p.palmFrame=g_frame;PistolThrowLoose(thrownAt+300,true);c.leftGrip=0;TickPistolLoose(c,thrownAt+3301);
    Check(!p.looseFlying&&!p.held&&p.lostObjects==1,"expired flight retires exactly one loose object");
    Check(!PistolTakeLoose(p.nextAmmoAt-1),"replacement delay blocks immediate pickup after loss");
    Check(PistolTakeLoose(p.nextAmmoAt)&&p.looseId!=clipIdentity,"replacement allowed after configured delay with a new identity");
    p.hand=HandIdentity();p.handFrame=p.palmFrame=g_frame;c.leftGrip=0;TickPistolLoose(c,thrownAt+4400);Check(p.looseFlying&&!p.held,"grip release drops current item visibly");
    p.handFrame=g_frame-3;c.leftGrip=1;PistolXAction(c,true);Check(p.looseFlying&&!p.held,"stale hand cannot catch moving magazine");
    // Actual waist/chest pickup state, each requires a fresh grip edge at selected zone.
    p=PistolReloadState{};p.enabled=p.basis=true;p.magazinePresent=false;p.palmFrame=g_frame;
    const Mat4 headIdentity=HandIdentity();for(int i=0;i<16;++i)g_eyeViewsBody[0].view[i]=static_cast<float>(headIdentity.m[i]);
    g_pistolGuides.ammoPickup=0;p.tip={-.18,-.65,.03};c.leftGrip=1;c.x=false;TickPistolLoose(c,5000);Check(p.held,"waist zone grip picks up ammo without X");
    p.held=false;TickPistolLoose(c,5010);Check(!p.held,"held grip cannot repeatedly spawn body-zone magazines");
    c.leftGrip=0;TickPistolLoose(c,5020);c.leftGrip=1;p.tip={2,2,2};TickPistolLoose(c,5030);Check(!p.held,"grip away from selected zone cannot pick up ammo");
    c.leftGrip=0;TickPistolLoose(c,5040);c.leftGrip=1;g_pistolGuides.ammoPickup=1;p.tip={0,-.38,.12};TickPistolLoose(c,5050);Check(p.held,"chest zone grip picks up ammo without X");
    p=PistolReloadState{};p.basis=p.held=true;p.magazinePresent=false;p.hand=HandIdentity();p.handFrame=p.palmFrame=g_frame;p.handVelocity={1,2,-.5};g_pistolGuides.tossHeight=1.75f;
    PistolThrowLoose(5100,true);Check(std::fabs(p.looseVelocity.y-3.5)<.001&&p.looseVelocity.x==1&&p.looseVelocity.z==-.5,"magazine height slider scales only measured upward release");
    p.held=true;p.handVelocity={1,0,-.5};PistolThrowLoose(5200,false);Check(p.looseVelocity.y==0&&p.looseVelocity.x==1,"still-height release gains no artificial upward boost");
    Check(!p.looseToss,"ordinary stationary-height release uses short drop window");
    TickPistolLoose(c,6199);Check(p.looseFlying,"dropped clip remains recoverable before one second");
    TickPistolLoose(c,6200);Check(!p.looseFlying&&p.nextAmmoAt==7200,"ordinary drop retires at one second then applies saved one-second replacement delay");
    Check(!PistolTakeLoose(7199)&&PistolTakeLoose(7200),"new clip pickup is allowed two seconds after ordinary drop");
    p=PistolReloadState{};p.basis=p.held=true;p.magazinePresent=false;p.looseId=77;p.hand=HandIdentity();p.rootBody=HandIdentity();
    p.tip={2,2,2};p.handFrame=p.palmFrame=g_frame;p.sampleFrame=g_frame;p.handVelocity={0,0,0};c.leftGrip=1;c.x=true;
    PistolXAction(c,true);Check(p.held&&!p.looseFlying,"stationary grip-held X does not toss a magazine");
    p.handVelocity={0,1.2,0};PistolXAction(c,true);const unsigned grippedTossId=p.looseId;
    Check(p.looseFlying&&!p.held&&p.looseToss&&grippedTossId==p.looseId,"upward grip-held X tosses the existing magazine");
    PistolXAction(c,true);Check(p.looseFlying&&p.looseId==grippedTossId,"held X cannot duplicate or retoss flying magazine");
    p.hand=p.looseBody;p.handFrame=p.palmFrame=g_frame;
    Check(PistolCatchLoose()&&p.held&&p.looseId==grippedTossId,"held grip catches the same X-tossed magazine identity");
    g_worldScale=oldScale;g_bodyYaw=oldPlayYaw;g_eyeViewsBody[0]=oldPlayView;g_aimActive=oldPlayAim;g_pistolGuides=oldPlaySettings;p=oldPlay;c.leftGrip=0;c.x=false;
    std::puts("PASS: production grip X toss/catch same identity, no duplicates/reload, gravity, stale/distant catch rejection, visible drop, missed-item cooldown and selected waist/chest grip pickup");
    // Actual BuildWeaponWorld + GPU readback, captured native globals.
    context->ClearState();g_dev=device;g_oCreateBuffer=PistolFixtureCreate;
    ID3D11Buffer* vb=Buffer(device,177904,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,32682,D3D11_BIND_INDEX_BUFFER);
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
    UINT stride=32,offset=240;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
    CameraSummary cam={};memcpy(cam.right,kMechanicCamera,12);memcpy(cam.up,kMechanicCamera+3,12);memcpy(cam.forward,kMechanicCamera+6,12);memcpy(cam.position,kMechanicCamera+9,12);
    cam.xScale=kMechanicCamera[12];cam.yScale=kMechanicCamera[13];cam.zScale=kMechanicCamera[14];cam.zOffset=kMechanicCamera[15];cam.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
    ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
    p.pending=p.cycle=p.held=false;p.emptyLocked=false;p.needsRack=true;p.basis=false;p.rootFrame=-100000;
    g_frameGunValid=true;g_frameGunMove=HandIdentity();
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicFrameGlobals),kMechanicFrameGlobals);
    auto* output=BuildWeaponWorld(context,18,276);Check(output!=nullptr,"frame GPU override available");
    float frame[88]={};Read(device,context,output,frame);Mat4 root=HullWorld(frame),inverse={};Check(Invert(root,inverse),"frame inverse");
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicSlideGlobals),kMechanicSlideGlobals);p.rackHeld=true;p.slidePull=.045;
    for(UINT start:{10866u,12843u})
    {
        g_frameGunMove.m[3]+=.2; // later hand updates must not split the pieces
        output=BuildWeaponWorld(context,start,start==10866?1977:858);float slide[88]={};Read(device,context,output,slide);
        const Mat4 relative=Multiply(inverse,HullWorld(slide));
        for(int i=0;i<16;++i)Check(std::fabs(relative.m[i]-(kPistolSlideRest.m[i]-(i==7?.045:0)))<.001,"both slide GPU meshes stay on frame-local -Y rail");
    }
    // Later FRAME materials must preserve the early frame root.
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicFrameGlobals),kMechanicFrameGlobals);
    output=BuildWeaponWorld(context,294,4470);float lateFrame[88]={};Read(device,context,output,lateFrame);Compare(lateFrame,frame,12,"late frame matches earlier anchor despite changed hand movement");
    // Do not borrow a previous frame root.
    ++g_frame;g_weaponMechanicFrame=g_frame;float unchanged[88];memcpy(unchanged,kMechanicSlideGlobals,sizeof(unchanged));
    PistolFinalWorld(context,10866,1977,HullWorld(unchanged),unchanged);Compare(unchanged,kMechanicSlideGlobals,88,"stale frame leaves native piece unchanged");
    --g_frame;g_weaponMechanicFrame=g_frame;p.rackHeld=false;p.needsRack=false;p.magazinePresent=true;
    // Normal reload retains native slide displacement while gun/arm transforms stay controlled.
    p.cycle=true;p.pending=false;p.emptyLocked=true;p.reloadChamberTracked=true;p.reloadWasEmpty=true;p.committed=false;
    for(UINT start:{10866u,12843u}){
      globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicSlideGlobals),kMechanicSlideGlobals);output=BuildWeaponWorld(context,start,start==10866?1977:858);float autoSlide[88]={};Read(device,context,output,autoSlide);
      Mat4 nativeRelative=Multiply(p.rootNative,HullWorld(kMechanicSlideGlobals));Mat4 invNative={};Invert(p.rootNative,invNative);nativeRelative=Multiply(invNative,HullWorld(kMechanicSlideGlobals));
      const double pull=(std::max)(0.0,(std::min)(.05,kPistolSlideRest.m[7]-nativeRelative.m[7]));Mat4 renderedRelative=Multiply(inverse,HullWorld(autoSlide));
      Check(std::fabs(renderedRelative.m[7]-(kPistolSlideRest.m[7]-.05))<.001,"empty reload holds both slide meshes open independently of native FRAME movement");
    }
    p.slideClosing=true;p.closeAt=1000;
    Check(std::fabs(PistolClosePull(1000)-.05)<1e-9&&std::fabs(PistolClosePull(1080)-.025)<1e-9&&PistolClosePull(1160)==0,"empty closure traverses exact 5 cm rail over 160 ms");
    p.committed=true;p.closeAt=GetTickCount64()-80;
    for(UINT start:{10866u,12843u}){
        globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicSlideGlobals),kMechanicSlideGlobals);output=BuildWeaponWorld(context,start,start==10866?1977:858);float closing[88]={};Read(device,context,output,closing);
        const Mat4 rel=Multiply(inverse,HullWorld(closing));const double travel=kPistolSlideRest.m[7]-rel.m[7];
        Check(travel>.005&&travel<.045,"both actual GPU slide pieces close along rail after ammo transfer");
        for(int i:{0,1,2,3,4,5,6,8,9,10,11})Check(std::fabs(rel.m[i]-kPistolSlideRest.m[i])<.001,"slide closure cannot rotate or move sideways");
    }
    Check(PistolBusy(),"visual slide closure holds fire gate independently of native end");
    p.slideClosing=false;p.cycle=false;p.emptyLocked=false;
    std::puts("PASS: independent empty slide closure moves both GPU slide meshes without moving FRAME");
    p.cycle=true;p.reloadChamberTracked=true;p.reloadWasEmpty=false;p.chamberReady=true;
    for(UINT start:{10866u,12843u}){
        globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicSlideGlobals),kMechanicSlideGlobals);output=BuildWeaponWorld(context,start,start==10866?1977:858);float partialSlide[88]={};Read(device,context,output,partialSlide);
        Mat4 partialRelative=Multiply(inverse,HullWorld(partialSlide));Check(std::fabs(partialRelative.m[7]-kPistolSlideRest.m[7])<.001,"partial 1911 reload keeps both slide meshes closed instead of replaying a rack");
    }
    p.cycle=false;p.reloadChamberTracked=false;
    std::puts("PASS: partial 1911 reload keeps slide closed on both actual GPU meshes");
    // Actual magazine draw uses only the measured final native seating segment.
    p.insertActive=true;p.pending=true;p.insertStart=kPistolInsertKeys[0];p.insertAt=GetTickCount64()-150;
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicClipGlobals),kMechanicClipGlobals);
    output=BuildWeaponWorld(context,8664,1338);float inserting[88]={};Read(device,context,output,inserting);
    Mat4 entering=Multiply(inverse,HullWorld(inserting));Check(Length(Sub(Apply(entering,{0,0,0}),Apply(kPistolClipRest,{0,0,0})))>.02,"pending magazine stays visible and moves along captured insertion path");
    const Mat4 halfway=PistolRigidBlend(kPistolInsertKeys[0],kPistolClipRest,.5);V3 xx={halfway.m[0],halfway.m[4],halfway.m[8]},yy={halfway.m[1],halfway.m[5],halfway.m[9]};Check(std::fabs(Length(xx)-1)<.00001&&std::fabs(Dot(xx,yy))<.00001,"insertion blend stays rigid without shearing magazine");
    p.insertAt=GetTickCount64()-600;output=BuildWeaponWorld(context,8664,1338);Read(device,context,output,inserting);entering=Multiply(inverse,HullWorld(inserting));for(int i=0;i<16;++i)Check(std::fabs(entering.m[i]-kPistolClipRest.m[i])<.001,"native seating path ends at exact frame magazine rest pose");
    p.pending=p.insertActive=false;
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicClipGlobals),kMechanicClipGlobals);output=BuildWeaponWorld(context,8664,1338);
    float clip[88]={};Read(device,context,output,clip);p.dropWorld=HullWorld(clip);p.dropActive=true;p.magazinePresent=false;p.dropAt=GetTickCount64()-100;
    output=BuildWeaponWorld(context,8664,1338);float drop[88]={};Read(device,context,output,drop);
    const Mat4 droppedWorld=HullWorld(drop);
    const V3 delta={droppedWorld.m[3]-p.dropWorld.m[3],droppedWorld.m[7]-p.dropWorld.m[7],droppedWorld.m[11]-p.dropWorld.m[11]};
    Check(Length(delta)>.02&&Length(delta)<1,"ejected magazine visibly departs instead of instantly hiding");
    p.dropAt=GetTickCount64()-700;output=BuildWeaponWorld(context,8664,1338);Read(device,context,output,drop);Check(!p.dropActive&&std::fabs(drop[7])>100,"cosmetic drop ends without persistent floor object");
    // Identical controller pose, violently translated/rotated native reload FRAME.
    float oldBody[16];std::memcpy(oldBody,g_handBody[0],sizeof(oldBody));const auto oldControllers=fc3pad::Latest();
    auto active=oldControllers;active.active=true;active.rightTrigger=0;fc3pad::Publish(active);
    const Mat4 identity=HandIdentity();for(int i=0;i<16;++i)g_handBody[0][i]=static_cast<float>(identity.m[i]);
    p.held=p.pending=p.cycle=p.rackHeld=p.rackReturning=p.needsRack=false;p.poseHoldUntil=0;p.parentValid=false;
    ++g_frame;g_weaponMechanicFrame=g_frame;p.rootFrame=-100000;g_frameGunMove=HandIdentity();
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicFrameGlobals),kMechanicFrameGlobals);output=BuildWeaponWorld(context,18,276);
    float anchored[88];Read(device,context,output,anchored);Check(p.parentValid,"quiet completed FRAME creates controller attachment");
    const Mat4 idleWorld=HullWorld(anchored);const Mat4 idleBody=p.rootBody;
    V3 pistolAim={};Check(PistolAimBody(pistolAim),"bare 1911 FRAME provides native aim");
    Mat4 aimBodyInverse={};Check(Invert(ArmBodyToCamera(),aimBodyInverse),"1911 aim body inverse");
    const Mat4 actualPistolRoot=Multiply(aimBodyInverse,Multiply(HullView(cam),idleWorld));
    const V3 actualPistolBarrel=Scale(V3{actualPistolRoot.m[1],actualPistolRoot.m[5],actualPistolRoot.m[9]},1/Length(V3{actualPistolRoot.m[1],actualPistolRoot.m[5],actualPistolRoot.m[9]}));
    Check(Length(Sub(pistolAim,actualPistolBarrel))<.0001,"bare 1911 aim matches actual uploaded FRAME barrel, no suppressor draw");
    p.reloadWasEmpty=true;p.reloadChamberTracked=true;p.committed=false;
    Mat4 changed=HullWorld(kMechanicFrameGlobals);changed.m[3]+=.3;changed.m[7]-=.2;
    for(int row=0;row<3;++row){const double x=changed.m[row*4],y=changed.m[row*4+1];changed.m[row*4]=-y;changed.m[row*4+1]=x;}
    float animated[88];std::memcpy(animated,kMechanicFrameGlobals,sizeof(animated));WritePistolWorld(animated,changed);
    ++g_frame;g_weaponMechanicFrame=g_frame;globals->SetPrivateData(kCbCopyGuid,sizeof(animated),animated);output=BuildWeaponWorld(context,18,276);
    float idleAgain[88];Read(device,context,output,idleAgain);const Mat4 sameIdle=HullWorld(idleAgain);
    for(int i=0;i<16;++i)Check(std::fabs(idleWorld.m[i]-sameIdle.m[i])<.001,"idle FRAME cannot relearn native tilt before insertion");
    p.held=true;++g_frame;g_weaponMechanicFrame=g_frame;output=BuildWeaponWorld(context,18,276);float pickupFrame[88];Read(device,context,output,pickupFrame);
    for(int i=0;i<12;++i)Check(std::fabs(pickupFrame[i]-idleAgain[i])<.001,"pickup transition cannot change whole gun position");
    p.held=false;p.cycle=true;++g_frame;g_weaponMechanicFrame=g_frame;output=BuildWeaponWorld(context,18,276);
    float reloadFrame[88];Read(device,context,output,reloadFrame);const Mat4 fixedWorld=HullWorld(reloadFrame);
    for(int i=0;i<16;++i)Check(std::fabs(idleWorld.m[i]-fixedWorld.m[i])<.001,"native reload translation and tilt cannot move whole gun with stationary controller");
    DrawCall dc={};dc.a=276;dc.b=18;ObservePistolGuideDraw(context,dc,4);
    for(int i=0;i<16;++i)Check(std::fabs(g_pistolGuidePreview.root.m[i]-p.rootBody.m[i])<.001,"shader fallback cannot replace completed collider parent");
    // Continue tracking after the native end, while its animation blends out.
    p.cycle=false;p.poseHoldUntil=GetTickCount64()+1200;g_handBody[0][3]=.1f;
    ++g_frame;g_weaponMechanicFrame=g_frame;output=BuildWeaponWorld(context,18,276);Read(device,context,output,reloadFrame);
    Check(std::fabs(p.rootBody.m[3]-idleBody.m[3]-.1)<.001,"right controller moves gun and contact parent through reload tail");
    for(int i:{0,1,2,4,5,6,8,9,10})Check(std::fabs(p.rootBody.m[i]-idleBody.m[i])<.001,"controller parent holds orientation without native tilt");
    Mat4 turn=HandIdentity();turn.m[0]=turn.m[5]=0;turn.m[1]=-1;turn.m[4]=1;
    for(int i=0;i<16;++i)g_handBody[0][i]=static_cast<float>(turn.m[i]);
    ++g_frame;g_weaponMechanicFrame=g_frame;output=BuildWeaponWorld(context,18,276);
    const Mat4 expectedTurn=Multiply(turn,idleBody);for(int i=0;i<16;++i)Check(std::fabs(p.rootBody.m[i]-expectedTurn.m[i])<.001,"controller rotation remains live during protected reload tail");
    Check(PistolAimBody(pistolAim)&&Length(Sub(pistolAim,Scale(V3{expectedTurn.m[1],expectedTurn.m[5],expectedTurn.m[9]},1/Length(V3{expectedTurn.m[1],expectedTurn.m[5],expectedTurn.m[9]}))))<.0001,"1911 aim rotates with actual gun through reload tail");
    {const auto saved=p;const bool savedTwo=g_twoHand;g_twoHand=true;p.poseHoldUntil=0;p.rootBody=idleBody;
     Check(PistolAimBody(pistolAim)&&Length(Sub(pistolAim,Unit(V3{expectedTurn.m[1],expectedTurn.m[5],expectedTurn.m[9]})))<.0001,"two-hand 1911 aim uses the current controller-owned FRAME instead of a stale native root");
     ++p.parentEpoch;Check(!PistolAimBody(pistolAim),"1911 aim rejects old calibration");p=saved;g_twoHand=savedTwo;
     g_weaponMechanicProfile=39;Check(!PistolAimBody(pistolAim),"1911 aim never replaces accepted Skorpion aim");g_weaponMechanicProfile=20;}
    std::puts("PASS: bare 1911 aim equals actual GPU FRAME barrel, controller rotation, two-hand root and independent Skorpion ownership.");
    // Captured native right wrist and fingers must share the final gun root.
    const bool gripTwo=g_twoHand;g_twoHand=false;p.cycle=false;p.held=true;
    float gripBones[576];std::memcpy(gripBones,kRightGripRaw,sizeof(gripBones));
    Mat4 gripMove={};Check(PistolRightHoldingMove(HandIdentity(),gripBones,gripMove),"1911 right grip attachment available");
    const Mat4 gripWrist=Multiply(gripMove,BoneMatrix(gripBones,2));
    const Mat4 expectedWrist=Multiply(ArmBodyToCamera(),Multiply(PistolParentBody(p.rootBody),kPistolHandFromFrame));
    for(int i=0;i<16;++i)Check(std::fabs(gripWrist.m[i]-expectedWrist.m[i])<.00001,"native wrist stays attached to controller-driven FRAME");
    StoreBone(gripBones,2,gripWrist);Check(PistolRightHoldingGrip(gripBones),"native right holding fingers restored");
    Mat4 invCaptured={};Invert(BoneMatrix(kRightGripRaw,2),invCaptured);
    for(int b=3;b<=19;++b){const Mat4 nativeRelative=Multiply(invCaptured,BoneMatrix(kRightGripRaw,b));const Mat4 expected=Multiply(gripWrist,nativeRelative);for(int i=0;i<12;++i)Check(std::fabs(gripBones[b*12+i]-expected.m[i])<.00001,"right fingers match captured firm gun grip");}
    const bool gpuEye=g_eyeViewsSet;const auto gpuJoints=g_armJoints[0];g_eyeViewsSet=true;
    g_armJoints[0]={{.2,-.15,-.2},{.35,-.3,.05},{.35,-.2,.4},true};
    ID3D11Buffer* armVb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER),*armRaw=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    UINT armStride=40,armOffset=0;context->IASetVertexBuffers(0,1,&armVb,&armStride,&armOffset);
    armRaw->SetPrivateData(kCbCopyGuid,sizeof(kRightGripRaw),kRightGripRaw);context->VSSetConstantBuffers(3,1,&armRaw);
    globals->SetPrivateData(kCbCopyGuid,sizeof(kRightGripGlobals),kRightGripGlobals);
    CameraSummary armCamera={};memcpy(armCamera.right,kRightGripCamera,12);memcpy(armCamera.up,kRightGripCamera+3,12);memcpy(armCamera.forward,kRightGripCamera+6,12);memcpy(armCamera.position,kRightGripCamera+9,12);armCamera.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(armCamera),&armCamera);
    Mat4 gpuToView={},gpuFromView={};float gpuNative[576];Check(DrawSpace(context,gpuNative,gpuToView,gpuFromView),"native arm draw context");
    auto* gpuPalette=BuildArmPalette(context,0);Check(gpuPalette!=nullptr,"actual right arm palette override");float gpuGrip[576];Read(device,context,gpuPalette,gpuGrip);
    const Mat4 gpuWrist=Multiply(gpuToView,BoneMatrix(gpuGrip,2));
    for(int i=0;i<16;++i)Check(std::fabs(gpuWrist.m[i]-expectedWrist.m[i])<.0001,"actual uploaded wrist stays on FRAME through stable pose and IK");
    for(int b=3;b<=19;++b){const Mat4 expected=Multiply(gpuWrist,Multiply(invCaptured,BoneMatrix(kRightGripRaw,b)));const Mat4 actual=Multiply(gpuToView,BoneMatrix(gpuGrip,b));for(int i=0;i<16;++i)Check(std::fabs(actual.m[i]-expected.m[i])<.0001,"actual uploaded fingers preserve native grip");}
    context->VSSetConstantBuffers(3,1,&armRaw);armRaw->Release();armVb->Release();context->IASetVertexBuffers(0,1,&vb,&stride,&offset);camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);g_eyeViewsSet=gpuEye;g_armJoints[0]=gpuJoints;
    std::puts("PASS: actual WARP GPU right arm upload keeps captured wrist and fingers on the controlled gun through stable pose and IK");
    g_twoHand=true;Check(!PistolRightHoldingMove(HandIdentity(),gripBones,gripMove)&&!PistolRightHoldingGrip(gripBones),"accepted two-hand path remains untouched");g_twoHand=gripTwo;p.held=false;
    std::puts("PASS: captured native right wrist attaches to final gun FRAME and all 17 finger bones retain native holding pose; two-hand path preserved");
    std::memcpy(g_handBody[0],oldBody,sizeof(oldBody));fc3pad::Publish(oldControllers);p.poseHoldUntil=0;p.parentValid=false;
    std::puts("PASS: actual GPU FRAME rejects native reload tilt/translation, follows controller translation/rotation, and preserves identical collider root through animation tail");
    const bool savedSupport=g_twoHand;g_twoHand=true;p.poseHoldUntil=0;p.cycle=p.held=p.pending=p.rackMode=p.rackHeld=p.rackReturning=p.needsRack=false;
    Mat4 supportBody=p.rootBody;supportBody.m[3]+=.17;const Mat4 supportPreserved=PistolParentBody(supportBody);
    Mat4 fixedSupport124={};Check(ControllerMagazineRoot124(fixedSupport124),"two-hand controller root available");for(int i=0;i<16;++i)Check(std::fabs(supportPreserved.m[i]-fixedSupport124.m[i])<1e-9,"idle two-hand support rejects native root animation and keeps shared hand/gun alignment");g_twoHand=savedSupport;
    p.looseFlying=true;p.held=false;p.magazinePresent=false;p.looseBody=HandIdentity();p.looseBody.m[3]=.27;p.looseBody.m[7]=-.2;p.looseBody.m[11]=.43;
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicClipGlobals),kMechanicClipGlobals);output=BuildWeaponWorld(context,8664,1338);float flying[88];Read(device,context,output,flying);
    Mat4 bodyInverse={};Invert(ArmBodyToCamera(),bodyInverse);const Mat4 renderedFly=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(flying)));
    for(int i=0;i<16;++i)Check(std::fabs(renderedFly.m[i]-p.looseBody.m[i])<.001,"actual magazine GPU draw follows flying body-space transform instead of hidden native animation");p.looseFlying=false;
    std::puts("PASS: actual loose magazine GPU upload matches independent body-space trajectory");
    p.magazinePresent=false;p.held=true;p.rootFrame=-100000;p.dropActive=p.insertActive=false;
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicClipGlobals),kMechanicClipGlobals);output=BuildWeaponWorld(context,15801,540);float stray[88];Read(device,context,output,stray);Check(Length(Apply(Multiply(HullView(cam),HullWorld(stray)),{0,0,0}))>1000,"1911 original loose bullet is hidden before FRAME while magazine is detached");p.held=false;p.magazinePresent=true;
    std::puts("PASS: 1911 original loose bullet hidden independent of FRAME draw order");
    DrawCall bulletCall={};bulletCall.a=540;bulletCall.b=15801;
    Check(MagazineHideBullet(context,bulletCall),"idle 1911 cosmetic bullet draw suppressed even with seated magazine");
    bulletCall.b=18;bulletCall.a=276;Check(!MagazineHideBullet(context,bulletCall),"frame ammunition material is not suppressed");
    std::puts("PASS: exact rogue 1911 bullet draw suppressed in idle and reload; FRAME material retained");
    // Preview holds the exact real-pickup mesh transform without touching inventory.
    const auto previewSettings=g_pistolGuides;const auto previewMenu=g_pistolMenu;const auto previewControllers=fc3pad::Latest();
    g_pistolMenu.open=true;g_menuGuideFamily=0;g_pistolGuides.ammoPreview=true;
    g_pistolGuides.ammoPosition[0]=.036f;g_pistolGuides.ammoRotation[2]=90;
    c.active=false;fc3pad::Publish(c);g_weaponMechanicFrame=g_frame;
    Check(PistolAmmoPreviewActive(),"mouse mode keeps held-ammo visual preview active");
    const Mat4 previewPose=MagazineHeldFromHand();
    p.hand=previewPose;p.handFrame=g_frame;p.pending=p.cycle=p.insertActive=p.dropActive=p.held=false;
    const auto inventoryBefore=p;
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicClipGlobals),kMechanicClipGlobals);
    output=BuildWeaponWorld(context,8664,1338);float previewOut[88];Read(device,context,output,previewOut);
    const Mat4 previewRendered=Multiply(HullView(cam),HullWorld(previewOut));
    for(int i=0;i<16;++i)Check(std::fabs(previewRendered.m[i]-previewPose.m[i])<.001,"actual preview GPU upload applies calibrated magazine transform");
    Check(!memcmp(&inventoryBefore,&p,sizeof(p)),"ammo preview does not mutate physical magazine transaction");
    float previewBones[576]={};for(int b=0;b<48;++b)StoreBone(previewBones,b,HandIdentity());
    Check(PistolGrip(previewBones),"preview applies native left holding finger pose with inactive controller buttons");
    g_pistolMenu.open=false;Check(!PistolAmmoPreviewActive(),"closing menu removes preview");
    p.held=true;c.active=true;fc3pad::Publish(c);const Mat4 actualPickup=MagazineHeldFromHand();
    for(int i=0;i<16;++i)Check(std::fabs(actualPickup.m[i]-previewPose.m[i])<1e-9,"real pickups share exact saved preview transform");
    g_pistolGuides=previewSettings;g_pistolMenu=previewMenu;fc3pad::Publish(previewControllers);p.held=false;
    std::puts("PASS: ammo-position preview GPU, native holding fingers, mouse persistence, inventory isolation and identical real-pickup offsets");

    g_animation.pending=true;g_animation.draw=AnimationRecord{};g_animation.draw.state=reinterpret_cast<UINT>(context);
    RecordAnimationSolid(context,frame,sizeof(frame));Check(g_animation.draw.finalBytes==352,"final solid CB probe uses exact upload bytes");Compare(g_animation.draw.finalPose,frame,88,"final solid probe preserves actual output");g_animation.pending=false;
    globals->SetPrivateData(kCbCopyGuid,sizeof(kMechanicClipGlobals),kMechanicClipGlobals);
    float source[88]={};ReadCopy(globals,source,sizeof(source));Compare(source,kMechanicClipGlobals,88,"native solid source untouched");
    p=prior;g_1911FrameFinal=savedFinal50;memcpy(g_handBody[1],savedHand50,sizeof(savedHand50));fc3pad::SetMagazineHandling(false);context->ClearState();globals->Release();camera->Release();vb->Release();ib->Release();
    std::puts("PASS: physical slide rear/release/return, short/stale strokes, fresh grip and full stroke after empty reload, early-frame draw order, yawed rail, RT-release guard; actual GPU slide meshes share fresh FRAME despite changing hand movement; visible cosmetic drop, source preservation, final-solid upload probe.");
}
