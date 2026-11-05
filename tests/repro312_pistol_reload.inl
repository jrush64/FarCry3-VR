// First physical magazine baseline: one contact requests one ordinary native
// magazine transaction. No direct ammo writes or render-thread engine calls.
#include "../src/pistol_reload_attachment.inl"
#include "../src/magazine_pull286.h"
struct PistolReloadState
{
#ifdef FC3_AMMO_TEST
    bool enabled=false;
#else
    bool enabled=true;
#endif
    bool held=false,x=false,pending=false,cycle=false,committed=false;
    bool insertionRetryBlocked=false; // Failed request needs a fresh physical contact.
    bool magazinePresent=true,entryAccepted=false;
    ULONGLONG removalBlockedUntil54=0; // confirmed seating protects the charging-handle gesture, never fire
    // Snapshot the verified native count before insertion changes it. The game
    // keeps that count while the physical magazine is detached and fire is gated.
    bool reloadChamberTracked=false,reloadWasEmpty=false,chamberReady=true,awaitingNativeClose=false;
    bool emptyLocked=false,needsRack=false,rackHeld=false,rackRear=false,rackReturning=false,waitTriggerRelease=false;
    bool gripWas=false,rackArmed=false,rackMode=false,dropActive=false,slideGripValid=false;
    float slideGripRelative[204]={};double nativeSlidePull=0;LONG nativeSlideFrame=-100000;
    double slidePull=0,rackStartPull=0; ULONGLONG rackReturnAt=0,dropAt=0;
    Mat4 rootWorld={},rootNative={},rootBody={},clipWorld={},dropWorld={};
    LONG rootFrame=-100000,clipFrame=-100000,palmFrame=-100000,rootNativeFrame=-100000;
    LONG fireResumeFrame=-100000; // AK needs a completed normal FRAME after native End
    UINT rootEpoch=0;ID3D11DeviceContext* rootContext=nullptr;uintptr_t rootVertex=0;
    V3 leftPalm={},rackStart={};
    UINT state=0,wrapper=0,weapon=0,before=0;
    ULONGLONG deadline=0,inputPauseAt=0,requestAt=0;
    LONG handFrame=-100000,portFrame=-100000,contactFrame=-1;
    int contactFrames=0,result=0,commits=0;
    Mat4 hand={},palmBody={},insertStart={},idle={},current={};
    Mat4 rackWrist167={};bool rackWristValid167=false;
    bool d50RackComplete168=false;
    bool d50GripReleased175=false; // release observed in this exact native insertion, including before transfer
    bool insertActive=false;ULONGLONG insertAt=0;
    // Persistent controller-relative FRAME attachment, never learned from a reload.
    bool parentValid=false,slideClosing=false;UINT parentEpoch=0;
    Mat4 controllerParent={};ULONGLONG closeAt=0,poseHoldUntil=0;
    bool catchArmed=false;LONG caughtFrame=-100000;
    unsigned catchProbeCount187=0;ULONGLONG catchProbeAt187=0;
    bool looseFlying=false,looseToss=false;ULONGLONG looseTick=0,looseAt=0,nextAmmoAt=0,sampleAt=0;LONG sampleFrame=-100000;
    unsigned looseId=0,nextLooseId=1,lostObjects=0;
    Mat4 looseBody={},samplePose={};V3 looseVelocity={},looseAngular={},sampleTip={},handVelocity={},handAngular={};double looseYaw=0,sampleYaw=0;
    fc3pull286::State pull286;double pullYaw301=0;bool returning286=false;ULONGLONG returnAt286=0;
    fc3pull286::Key returnKey286,looseKey286;V3 returnVelocity286={};
    Mat4 nativeGunBody={};LONG nativeGunFrame=-100000;
    Mat4 silencerRelative={};bool silencerValid=false;UINT silencerEpoch=0;
    bool removalHeld=false,removalDetached=false,removalGripWait=false;
    UINT ammoWrapper=0;ULONGLONG ejectWaitAt=0; // X eject: the weapon the request seam named, and a pending decision
    Mat4 removalStartPalm={},removalClip={};V3 removalStart={};
    bool akRackComplete=false;
    bool basis=false;
    UINT epoch=0;
    LONG basisFrame=-100000;
    V3 tip={},port={};
};
PistolReloadState g_magazineStates[20];
// Insert owns this interval; a pre-existing tracking pause must exclude it.
void ResumeMagazineInsertPause224(ULONGLONG paused)
{
    for(auto& p:g_magazineStates){
        if(p.pending||p.cycle)p.deadline+=paused;
        if(p.looseFlying){p.looseAt+=paused;p.looseTick+=paused;}
        if(p.inputPauseAt)p.inputPauseAt+=paused;
        p.sampleAt=0;p.sampleFrame=-100000;p.contactFrames=0;
    }
}
#define g_pistolReload (g_magazineStates[MagazineFamilyOfProfile(g_weaponMechanicProfile)])
// "AkModel" = any two-handed magazine rifle on the AK path (AK, A2000, STG-90); see rifle_table.inl.
bool AkModel(){return RifleProfile(g_weaponMechanicProfile)&&g_frame-g_weaponMechanicFrame<=2;}
// Pistol part tables. The 6P9 shares the 1911 animation skeleton: PID56072 measured identical
// clip/slide rests and right-hand-from-FRAME (0 deg, 0.6 cm), so it reuses every accepted 1911
// constant and calibration; only its draw ranges differ (offline 6p9.xbg catalog + live order).
struct PistolMesh{UINT vb,ib,offset;UINT frame[4][2];UINT clip[2],bullet[2],slide[2][2],tinySlide[2],tinyFrame[2];};
const PistolMesh k1911Mesh={177904,32682,240,{{294,4470},{18,276},{4764,3900}},{8664,1338},{15801,540},{{10866,1977},{12843,858}},{0,18},{0,0}};
const PistolMesh k6p9Mesh={106448,20736,240,{{4695,1263},{3741,954},{5958,3468}},{3213,528},{9828,540},{{18,2919},{2937,276}},{0,12},{12,6}};
#include "../src/d50_clips.inl"
bool P6p9Model(){return g_weaponMechanicProfile==1&&g_frame-g_weaponMechanicFrame<=2;}
bool D50Model(){return g_weaponMechanicProfile==12&&g_frame-g_weaponMechanicFrame<=2;}
const PistolMesh& CurrentPistolMesh(){return D50Model()?kD50Mesh:P6p9Model()?k6p9Mesh:k1911Mesh;}
bool PistolMeshPart(const UINT (&part)[2],UINT start,UINT count){return part[1]&&part[0]==start&&part[1]==count;}
bool PistolMeshFrame(UINT start,UINT count){for(const auto& f:CurrentPistolMesh().frame)if(f[0]==start&&f[1]==count)return true;return false;}
bool PistolMeshSlide(UINT start,UINT count){for(const auto& s:CurrentPistolMesh().slide)if(s[0]==start&&s[1]==count)return true;return false;}
template<class G> bool PistolMeshMain(const G& d){const auto& m=CurrentPistolMesh();return d.vertexBytes==m.vb&&d.indexBytes==m.ib&&d.stride==32&&d.offset==m.offset&&d.format==57;}
bool AkWeaponVisualActive(){return AkModel()&&g_pistolReload.enabled&&MotionInteractionEnabled()&&g_handRightValid;}
#include "../src/ak_data.inl"
#include "../src/ak_charge_pose.inl"
#include "../src/rifle_table.inl"
#include "../src/d50_data.inl"
bool SkorpionModel(){return g_weaponMechanicProfile==39&&g_frame-g_weaponMechanicFrame<=2;}
#include "../src/skorpion_data.inl"
#include "../src/skorpion_charge_pose.inl"
const Mat4& PistolHandFromFrameCur();
const Mat4& MagazineHandFromFrame(){return AkModel()?*CurrentRifle().hand:SkorpionModel()?kSkorpionHandFromFrame:PistolHandFromFrameCur();}
void TickPistolMechanics(const fc3pad::Controllers&);
bool TickPistolGuideMenu(const fc3pad::Controllers&,bool);
bool AkFrame(UINT,UINT);
void PistolGuideMarkers();
void BeginPistolInsert();
void PistolLoseLoose(ULONGLONG);
extern double g_bodyYaw;
bool PistolBusy();
bool PistolPoseOwned();
void LoadBowSettings(const wchar_t*);void LoadWeapons7Guides(const wchar_t*);void LoadLever1887Settings(const wchar_t*);void LoadRevolverSettings(const wchar_t*);void LoadSpasSettings(const wchar_t*);void LoadR700Settings(const wchar_t*);
void LoadPistolReloadSettings()
{
    wchar_t path[MAX_PATH];ManualSettingsPath(path);
    g_guideLoadFamily=0;LoadPistolGuides();
    g_guideLoadFamily=1;g_pistolGuides.slide[0]=.026f;g_pistolGuides.slide[1]=.14320966f;g_pistolGuides.slide[2]=.04219692f;g_pistolGuides.port[0]=0;g_pistolGuides.port[1]=.155034f;g_pistolGuides.port[2]=-.055f;LoadPistolGuides();
    g_magazineStates[1].enabled=GetPrivateProfileIntW(L"SkorpionReload",L"Mode",0,path)==0;g_guideLoadFamily=-1;
    g_guideLoadFamily=2;g_pistolGuides.port[0]=g_pistolGuides.slide[0]=0;g_pistolGuides.port[1]=g_pistolGuides.slide[1]=.27f;g_pistolGuides.port[2]=g_pistolGuides.slide[2]=-.06f;LoadPistolGuides();g_magazineStates[2].enabled=GetPrivateProfileIntW(L"AkReload",L"Mode",0,path)==0;g_guideLoadFamily=-1;
    // New rifles inherit the player's AK hand offset/pickup preferences, then take their own
    // FRAME-local boxes (insertion 12 cm below the native clip rest, as on the AK; A2000 knob box).
    // (families 11-14 = Vector / ACE / SVD / R700, weapons 5: they were missing here, so their boxes sat at the gun origin
    //  with no size and their INSERT sections never loaded)
    for(int family=3;family<=18;++family){
        if(family==10)continue; // the D50 is a pistol, below
        g_magazineGuides[family]=g_magazineGuides[2];g_guideLoadFamily=family;
        for(int i=0;i<3;++i){g_pistolGuides.port[i]=g_pistolGuides.slide[i]=PistolGuideDefault(i);g_pistolGuides.charge[i]=PistolGuideDefault(30+i);if(PistolGuideDefault(33+i)>0)g_pistolGuides.size[2][i]=PistolGuideDefault(33+i);}
        for(int i=0;i<3;++i)g_pistolGuides.ammoPosition[i]=g_pistolGuides.ammoRotation[i]=0;
        LoadPistolGuides();g_magazineStates[family].enabled=GetPrivateProfileIntW(MagazineSettingsSection(),L"Mode",0,path)==0;g_guideLoadFamily=-1;
    }
    // Desert Eagle: the player's 1911 hand / pickup preferences, its own captured boxes.
    g_magazineGuides[10]=g_magazineGuides[0];g_guideLoadFamily=10;
    for(int i=0;i<3;++i){g_pistolGuides.port[i]=PistolGuideDefault(i);g_pistolGuides.slide[i]=PistolGuideDefault(3+i);g_pistolGuides.ammoPosition[i]=g_pistolGuides.ammoRotation[i]=0;}
    LoadPistolGuides();g_magazineStates[10].enabled=GetPrivateProfileIntW(L"D50Reload",L"Mode",0,path)==0;g_guideLoadFamily=-1;
    g_hideGameplayGuides=GetPrivateProfileIntW(L"VRGuides",L"HideGameplay",0,path)!=0;
    g_grabGuides=GetPrivateProfileIntW(L"VRGuides",L"GrabGuides",1,path)!=0;
    g_showWeaponReloadGuides=GetPrivateProfileIntW(L"VRGuides",L"ReloadGuides",1,path)!=0;
    g_pistolReload.enabled=GetPrivateProfileIntW(L"PistolReload",L"Mode",0,path)==0;
    Log("[PISTOL RELOAD] 1911 mode %s; Mode=1 restores native X",g_pistolReload.enabled?"physical":"native");
    LoadLever1887Settings(path);
    LoadRevolverSettings(path);
    LoadSpasSettings(path);LoadR700Settings(path);LoadWeapons7Guides(path);LoadBowSettings(path);
}
bool PistolModel()
{return MagazineProfile(g_weaponMechanicProfile)&&g_frame-g_weaponMechanicFrame<=2;}
bool PistolHandling()
{return g_pistolReload.enabled&&PistolModel()&&MotionInteractionEnabled()&&fc3pad::Latest().active&&g_handLeftValid&&g_handRightValid;}
void ObservePistolEmpty(UINT state,UINT wrapper)
{if(ReloadRead(ReloadRead(wrapper,0x48),0xcc)==0)g_pistolReload.emptyLocked=true;}
bool PistolLocal(UINT,UINT);
bool PistolContactReady(UINT state,UINT wrapper)
{
    const auto& p=g_pistolReload;
    if(AkModel()||D50Model())return false; // manual D50/rifle insertion uses the pre-predicate transaction, not a timeline
    return p.pending&&!p.entryAccepted&&GetTickCount64()<=p.deadline&&PistolLocal(state,wrapper);
}
void PistolContactAccepted(UINT state,UINT wrapper)
{
    auto& p=g_pistolReload;p.entryAccepted=true;p.state=state;p.wrapper=wrapper;p.weapon=ReloadRead(wrapper,0x48);
    Log("[PISTOL RELOAD] physical contact authorized one native transition, wrapper %08X",wrapper);
}
bool AkInsertionSeated()
{const auto& p=g_pistolReload;return AkModel()&&p.insertActive&&GetTickCount64()-p.insertAt>=280;}
bool AkReloadSupport()
{return PistolHandling()&&AkInsertionSeated()&&!g_pistolReload.held&&!g_pistolReload.removalHeld&&!g_pistolReload.needsRack&&!g_pistolReload.rackHeld&&!g_pistolReload.rackReturning&&!g_pistolMenu.open;}
bool PistolOwnsLeft()
{return PistolHandling()&&(g_pistolReload.removalHeld||g_pistolReload.held||((g_pistolReload.pending||g_pistolReload.cycle)&&!AkInsertionSeated())||g_pistolReload.rackMode||g_pistolReload.rackHeld||g_pistolReload.rackReturning||g_pistolReload.needsRack||g_pistolMenu.open);}
bool PistolPoseOwned(){return PistolHandling()&&(PistolOwnsLeft()||(AkModel()&&(g_pistolReload.pending||g_pistolReload.cycle))||GetTickCount64()<g_pistolReload.poseHoldUntil);}
bool PistolLocal(UINT state,UINT wrapper)
{
    if(!g_pistolReload.enabled||!ReloadSignalLocal(state,wrapper))return false;
    // HUD is presentation, not ownership: it can lag shooting/reloading or disappear in menus.
    // Exact local actor, state/component relation, wrapper weapon and engine authority are verified above.
    return true;
}
UINT PistolAudioFlags170(){const auto& p=g_pistolReload;return (p.reloadWasEmpty?1u:0u)|(p.committed?2u:0u)|(p.cycle?4u:0u)|(p.rackHeld?8u:0u)|(p.rackReturning?16u:0u)|(p.d50RackComplete168?32u:0u);}
void PistolReset()
{
    if(g_weaponMechanicProfile==12)fc3throw::PublishD50Audio170(0,0,0,0,g_armPoseEpoch,0);
    if(g_pistolReload.looseFlying||g_pistolReload.held)PistolLoseLoose(GetTickCount64());
    const bool owned=g_pistolReload.pending||g_pistolReload.cycle;
    g_pistolReload.held=g_pistolReload.pending=g_pistolReload.cycle=g_pistolReload.insertActive=false;g_pistolReload.entryAccepted=false;
    g_reloadSignal.pistolPermissionUntil=0;
    g_pistolReload.rackHeld=g_pistolReload.rackReturning=g_pistolReload.rackRear=g_pistolReload.dropActive=false;
    g_pistolReload.d50GripReleased175=false;
    g_pistolReload.gripWas=false;g_pistolReload.rackArmed=g_pistolReload.rackMode=false;g_pistolReload.palmFrame=g_pistolReload.rootFrame=-100000;
    g_pistolReload.handFrame=g_pistolReload.portFrame=-100000;
    g_pistolReload.contactFrames=0;
    g_pistolReload.parentValid=g_pistolReload.slideClosing=false;g_pistolReload.poseHoldUntil=0;
    g_pistolReload.reloadChamberTracked=false;g_pistolReload.awaitingNativeClose=false;g_pistolReload.chamberReady=true;
    if(owned)fc3pad::CancelReloadRequest();
}
void PistolRequireManualRack50(){
 auto& p=g_pistolReload;if(g_weaponMechanicProfile!=20||!p.enabled)return;p.chamberReady=false;p.awaitingNativeClose=false;p.needsRack=p.rackMode=true;p.rackArmed=p.rackHeld=p.rackRear=p.rackReturning=false;p.emptyLocked=p.slideClosing=false;p.slidePull=0;
}
ULONGLONG g_pistolRackCaptureUntil=0;
void RevolverNativeBegin(UINT,UINT);void RevolverNativeEnd(UINT,UINT);void SpasNativeBegin(UINT,UINT);void SpasNativeEnd(UINT,UINT);void R700NativeBegin(UINT,UINT);void R700NativeEnd(UINT,UINT);void PkmNativeBegin(UINT,UINT);void PkmNativeEnd(UINT,UINT);
void PistolNativeBegin(UINT state,UINT wrapper)
{
    RevolverNativeBegin(state,wrapper);SpasNativeBegin(state,wrapper);R700NativeBegin(state,wrapper);PkmNativeBegin(state,wrapper);
    if(!g_pistolReload.pending||!g_pistolReload.entryAccepted||g_pistolReload.state!=state||g_pistolReload.wrapper!=wrapper||!PistolLocal(state,wrapper))return;
    auto& p=g_pistolReload;p.pending=false;p.cycle=true;p.committed=false;p.rackMode=p.rackArmed=p.rackHeld=p.rackReturning=false;
    p.state=state;p.wrapper=wrapper;p.weapon=ReloadRead(wrapper,0x48);p.before=ReloadRead(p.weapon,0xcc);
    p.reloadChamberTracked=true;p.reloadWasEmpty=p.before==0;p.akRackComplete=false;
    p.chamberReady=!(p.reloadWasEmpty||(g_weaponMechanicProfile==20&&p.enabled));p.awaitingNativeClose=false;p.d50RackComplete168=false;p.d50GripReleased175=false;
    p.slideGripValid=false;p.nativeSlideFrame=-100000;p.deadline=GetTickCount64()+8000;g_pistolRackCaptureUntil=GetTickCount64()+5000;fc3pad::CancelReloadRequest();
    if(g_weaponMechanicProfile==12)fc3throw::PublishD50Audio170(1,state,wrapper,p.weapon,g_armPoseEpoch,PistolAudioFlags170());
    Log("[PISTOL RELOAD] local native begin, loaded %u; one magazine request",p.before);
    Log("[1911 CHAMBER] %s reload; pre-insertion native count %u, owner %08X",(g_weaponMechanicProfile==20&&p.enabled)?"manual slide pull required":p.reloadWasEmpty?"empty, slide closure required":"partial, chambered round retained",p.before,p.weapon);
}
void PistolNativeTransfer(UINT state,UINT wrapper,UINT before,UINT after)
{
    auto& p=g_pistolReload;
    if(!p.cycle||p.committed||p.state!=state||p.wrapper!=wrapper||!PistolLocal(state,wrapper)||ReloadRead(wrapper,0x48)!=p.weapon)return;
    if(after>before){p.removalBlockedUntil54=GetTickCount64()+2000;p.magazinePresent=true;p.needsRack=false;p.rackArmed=false;p.emptyLocked=false;p.slidePull=0;p.committed=true;
        p.awaitingNativeClose=p.reloadWasEmpty;p.chamberReady=!(p.reloadWasEmpty||(g_weaponMechanicProfile==20&&p.enabled));
        p.slideClosing=p.reloadWasEmpty&&(g_weaponMechanicProfile==20||g_weaponMechanicProfile==1);p.closeAt=GetTickCount64();
        if(p.slideClosing)Log("[1911 SLIDE] native ammo transfer starts independent 160 ms closure on FRAME-local -Y; both slide pieces only");
        if(g_weaponMechanicProfile==12&&p.enabled&&p.reloadWasEmpty){p.d50RackComplete168=false;p.needsRack=p.rackMode=true;p.rackArmed=p.d50GripReleased175;p.rackHeld=p.rackRear=p.rackReturning=false;p.emptyLocked=true;p.awaitingNativeClose=false;Log("[D50 RACK168] verified magazine transfer; manual slide available during native reload tail, firing still gated");}
        if(g_weaponMechanicProfile==12)fc3throw::PublishD50Audio170(2,state,wrapper,p.weapon,g_armPoseEpoch,PistolAudioFlags170());
        ++p.commits;fc3xr::Buzz(1);Log("[1911 CHAMBER] native transfer %u -> %u; %s",before,after,(g_weaponMechanicProfile==20&&p.enabled)?"manual slide rack required after insertion":p.awaitingNativeClose?"empty reload awaits native slide closure":"partial reload retains chambered round, no slide operation");}
}
void PistolNativeEnd(UINT state,UINT wrapper)
{
    RevolverNativeEnd(state,wrapper);SpasNativeEnd(state,wrapper);R700NativeEnd(state,wrapper);PkmNativeEnd(state,wrapper);
    auto& p=g_pistolReload;
    if(!p.cycle||p.state!=state||p.wrapper!=wrapper)return;
    if(!PistolLocal(state,wrapper)||ReloadRead(wrapper,0x48)!=p.weapon){p.result=3;PistolReset();return;}
    p.result=p.committed?1:3;p.cycle=false;p.poseHoldUntil=RifleProfile(g_weaponMechanicProfile)?0:GetTickCount64()+1200;
    if(p.committed){
        // The native end event confirms the game has completed chambering. Ammo
        // transfer alone never clears the empty-reload chamber requirement.
        const bool unfinishedD50168=g_weaponMechanicProfile==12&&p.enabled&&p.reloadWasEmpty&&!p.d50RackComplete168;
        if(!unfinishedD50168){
        p.chamberReady=true;p.awaitingNativeClose=false;
        p.needsRack=p.emptyLocked=p.rackMode=p.rackArmed=p.rackHeld=p.rackReturning=p.rackRear=false;p.slidePull=0;p.waitTriggerRelease=!RifleProfile(g_weaponMechanicProfile)&&fc3pad::Latest().rightTrigger>.3f;
        }else {p.chamberReady=false;p.awaitingNativeClose=false;p.needsRack=p.rackMode=true;}
        if(RifleProfile(g_weaponMechanicProfile)){p.fireResumeFrame=-100000;Log("[AK FIRE] native end observed; fire live immediately, no frame gate");}}
    else if(!p.committed)p.insertActive=false;
    if(g_weaponMechanicProfile==20&&p.committed)PistolRequireManualRack50();
    if(g_weaponMechanicProfile==12&&p.committed&&p.reloadWasEmpty&&!p.d50RackComplete168){
      p.chamberReady=false;p.needsRack=p.rackMode=true;p.emptyLocked=true;
      Log("[D50 SLIDE] empty reload completed; release LEFT grip, grip the slide box, pull it back and release before firing");
    }
    if(g_weaponMechanicProfile==39&&p.committed&&p.reloadWasEmpty){
      p.chamberReady=false;p.needsRack=p.rackMode=true;p.rackArmed=false;p.emptyLocked=false;
      Log("[SKORPION CHARGE] empty reload completed; release left grip, grab right-side handle, pull backward and release before firing");
    }
    fc3pad::CancelReloadRequest();
    if(g_weaponMechanicProfile==12)fc3throw::PublishD50Audio170(3,state,wrapper,p.weapon,g_armPoseEpoch,PistolAudioFlags170());
    Log("[PISTOL RELOAD] native end, loaded %u -> %u, confirmed=%d",p.before,ReloadRead(p.weapon,0xcc),p.committed);
}
void ObservePistolHand(const float* bones,const float* globals,const CameraSummary& camera)
{
    if(!PistolHandling())return;
    auto& p=g_pistolReload;
    if(p.epoch!=g_armPoseEpoch){p.basis=false;p.epoch=g_armPoseEpoch;}
    Mat4 basis={};if(!HandBasis(Multiply(HullView(camera),Multiply(HullWorld(globals),BoneMatrix(bones,2))),basis))return;
    p.current=basis;p.basisFrame=g_frame;
    {Mat4 inverseHand={},bodyInverse={};if(Invert(MagazineHandFromFrame(),inverseHand)&&Invert(ArmBodyToCamera(),bodyInverse)){p.nativeGunBody=Multiply(bodyInverse,Multiply(HullView(camera),Multiply(HullWorld(globals),Multiply(BoneMatrix(bones,2),inverseHand))));p.nativeGunFrame=g_frame;}}
    if(!PistolPoseOwned()&&fc3pad::Latest().rightTrigger<.3f){p.idle=basis;p.basis=true;}
    g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);
}
void CorrectPistolHandMove(const ArmJoints& joints,Mat4& move)
{
    auto& p=g_pistolReload;
    if(!PistolOwnsLeft()||!p.basis||g_frame-p.basisFrame>1)return;
    Mat4 inverse={};if(!Invert(p.current,inverse))return;
    Mat4 correction=Multiply(p.idle,inverse);
    const V3 wrist={joints.w[0],joints.w[1],joints.w[2]},turned=Apply(correction,wrist);
    correction.m[3]=wrist.x-turned.x;correction.m[7]=wrist.y-turned.y;correction.m[11]=wrist.z-turned.z;
    move=Multiply(move,correction);
}
// Visual-only cache: never used as physical contact, reload permission or firing state.
struct AmmoPreviewPose {float bodyBones[576]={};Mat4 palm={};bool valid=false;UINT epoch=0;}g_ammoPreviewPoses[20];
bool PistolAmmoPreviewActive()
{
 const bool equipped=MagazineProfile(g_weaponMechanicProfile)&&MagazineFamilyOfProfile(g_weaponMechanicProfile)==g_menuGuideFamily;
 return g_pistolMenu.open&&equipped&&g_pistolGuides.ammoPreview&&g_motionGun;
}
bool AmmoPreviewPoseAvailable()
{return PistolAmmoPreviewActive()&&g_ammoPreviewPoses[g_menuGuideFamily].valid&&g_ammoPreviewPoses[g_menuGuideFamily].epoch==g_armPoseEpoch;}
void CacheAmmoPreviewPose(ID3D11DeviceContext* context,const float* bones)
{
 if(!MagazineProfile(g_weaponMechanicProfile)||!g_handLeftValid)return;
 Mat4 toView={},fromView={},bodyInverse={};if(!DrawSpace(context,nullptr,toView,fromView)||!Invert(ArmBodyToCamera(),bodyInverse))return;
 auto& cache=g_ammoPreviewPoses[MagazineFamilyOfProfile(g_weaponMechanicProfile)];const Mat4 toBody=Multiply(bodyInverse,toView);
 for(int b=0;b<48;++b)StoreBone(cache.bodyBones,b,Multiply(toBody,BoneMatrix(bones,b)));
 cache.palm=BoneMatrix(cache.bodyBones,2);cache.epoch=g_armPoseEpoch;cache.valid=true;
}
bool RestoreAmmoPreviewArm(ID3D11DeviceContext* context,float* bones,const Mat4& fromView)
{
 if(!AmmoPreviewPoseAvailable())return false;
 const auto& cache=g_ammoPreviewPoses[g_menuGuideFamily];const Mat4 toModel=Multiply(fromView,ArmBodyToCamera());
 for(int b=0;b<48;++b)StoreBone(bones,b,Multiply(toModel,BoneMatrix(cache.bodyBones,b)));
 return true;
}
Mat4 MagazineHeldFromHand()
{
 Mat4 correction=HandIdentity();
 for(int axis=0;axis<3;++axis){const double a=g_pistolGuides.ammoRotation[axis]*3.141592653589793/180;Mat4 r=HandIdentity();const int u=(axis+1)%3,v=(axis+2)%3;r.m[u*4+u]=r.m[v*4+v]=std::cos(a);r.m[u*4+v]=-std::sin(a);r.m[v*4+u]=std::sin(a);correction=Multiply(correction,r);}
 for(int i=0;i<3;++i)correction.m[i*4+3]=g_pistolGuides.ammoPosition[i];
 return Multiply(AkModel()?*CurrentRifle().magazine:PistolMagazineFromHandCur(),correction);
}
bool PlaceAmmoPreviewWorld(ID3D11DeviceContext* context,UINT start,UINT count,float* globals)
{
 if(!AmmoPreviewPoseAvailable())return false;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 const bool clip=RifleFamily(g_menuGuideFamily)?(RifleMain(RifleForFamily(g_menuGuideFamily),d)&&RifleClip(RifleForFamily(g_menuGuideFamily),start,count)):g_menuGuideFamily==1?(d.vertexBytes==157984&&d.indexBytes==35322&&d.stride==32&&d.offset==0&&d.format==57&&start==0&&count==1548):(PistolMeshMain(d)&&PistolMeshPart(CurrentPistolMesh().clip,start,count));
 if(!clip)return false;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);CameraSummary camera={};UINT bytes=sizeof(camera);Mat4 inverse={};
 const bool ok=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera)&&Invert(HullView(camera),inverse);SafeRelease(cb);if(!ok)return false;
 const Mat4 desired=Multiply(inverse,Multiply(ArmBodyToCamera(),Multiply(g_ammoPreviewPoses[g_menuGuideFamily].palm,MagazineHeldFromHand())));
 for(int r=0;r<3;++r)for(int c=0;c<4;++c)globals[r*4+c]=static_cast<float>(desired.m[r*4+c]-(c==3?globals[12+r]:0));
 return true;
}
bool GrabLeftHandBody(Mat4& hand){if(g_frame-g_pistolReload.palmFrame>1)return false;hand=g_pistolReload.palmBody;return true;}
void RecordPistolHand(ID3D11DeviceContext* context,const float* bones)
{
    Mat4 view={},inverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
    g_pistolReload.palmBody=Multiply(inverse,view);
    ObservePistolGuidePalm(g_pistolReload.palmBody);
    g_pistolReload.leftPalm=Apply(g_pistolReload.palmBody,{-.591,.011,1.149});g_pistolReload.palmFrame=g_frame;
    g_pistolReload.hand=Multiply(view,MagazineHeldFromHand());
    g_pistolReload.tip=Apply(g_pistolReload.palmBody,{-.591+g_pistolGuides.hand[0],.011+g_pistolGuides.hand[1],1.149+g_pistolGuides.hand[2]});
    g_pistolReload.handFrame=g_frame;
}
V3 PistolRackGuide();const float* RifleChargeFingers();
void CapturePistolSlideGrip(ID3D11DeviceContext* context,const float* bones)
{
    if(SkorpionModel()||AkModel())return; // verified asset pose; animated recoil cannot replace it
    auto& p=g_pistolReload;
    if(!PistolHandling()||(!p.cycle&&GetTickCount64()>g_pistolRackCaptureUntil)||p.rackHeld||g_frame-p.nativeSlideFrame>1||g_frame-p.rootFrame>1||p.nativeSlidePull<.015)return;
    // Recoil alone moves the slide with the support hand still underneath.
    // Accept a grip only when the raw native palm actually reaches the slide.
    ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);float globals[88]={};UINT bytes=ReadCopy(cb,globals,sizeof(globals));SafeRelease(cb);
    Mat4 frameInverse={};if(bytes<48||!Invert(p.rootNative,frameInverse))return;
    Mat4 nativeHand=Multiply(frameInverse,Multiply(HullWorld(globals),BoneMatrix(bones,2)));
    V3 palm=Apply(nativeHand,{-.591,.011,1.149});
    if(Length(Sub(palm,SkorpionModel()?PistolRackGuide():V3{0,.06,.08}))>.075)return;
    Mat4 inverse={};if(!Invert(BoneMatrix(bones,2),inverse))return;
    for(int b=3;b<=19;++b)StoreBone(p.slideGripRelative,b-3,Multiply(inverse,BoneMatrix(bones,b)));
    if(!p.slideGripValid)Log("[PISTOL SLIDE] native reload palm at slide; rack finger pose captured; tracked wrist retained");
    p.slideGripValid=true;
}
bool PistolGrip(float* bones)
{
    if(!PistolHandling()&&!PistolAmmoPreviewActive())return false;
    const auto& p=g_pistolReload;
    const float* pose=(p.held||p.removalHeld||PistolAmmoPreviewActive())?(AkModel()?CurrentRifle().magazineGrip:PistolMagazineGripCur()):(AkReloadSupport()&&g_twoHand&&PistolPoseOwned()?CurrentRifle().supportGrip:(p.rackHeld?((AkModel()||D50Model()||g_weaponMechanicProfile==20)?RifleChargeFingers():SkorpionModel()?kSkorpionChargingGripRelative:(p.slideGripValid?p.slideGripRelative:kPistolGripRelative)):nullptr));
    if(!pose)return false;
    const Mat4 hand=BoneMatrix(bones,2);
    for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(hand,BoneMatrix(pose,b-3)));
    return true;
}
bool PistolDraw(ID3D11DeviceContext* context,UINT start,UINT count)
{
    if(!PistolHandling()&&!PistolAmmoPreviewActive())return false;
    DrawCall call={};call.a=count;call.b=start;
    const auto d=NativeWeaponGeometry(context,call);
    if(AkModel())return RifleMain(CurrentRifle(),d);
    if(SkorpionModel())return d.vertexBytes==157984&&d.indexBytes==35322&&d.stride==32&&d.offset==0&&d.format==57;
    return PistolMeshMain(d);
}
void AkWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,Mat4&,float*);
void TickAkRemoval(const fc3pad::Controllers&);
void SkorpionWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,Mat4&,float*);
void PistolWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& nativeToView,Mat4& handMove,float* globals)
{
    if(!PistolDraw(context,start,count))return;
    if(AkModel()){AkWorld(context,start,count,nativeToView,handMove,globals);return;}
    if(SkorpionModel()){SkorpionWorld(context,start,count,nativeToView,handMove,globals);return;}
    auto& p=g_pistolReload;
    const auto& mesh=CurrentPistolMesh();const bool clipDraw=PistolMeshPart(mesh.clip,start,count),bulletDraw=PistolMeshPart(mesh.bullet,start,count);
    if(PistolMeshFrame(start,count))
    {
        // Solid first-person pieces have their own animated world transforms.
        // Refresh from the frame draw so gun-before-arm ordering is supported.
        Mat4 basis={};
        if(HandBasis(Multiply(nativeToView,PistolHandFromFrameCur()),basis))
        {
            p.current=basis;p.basisFrame=g_frame;
            if(PistolOwnsLeft())g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);
            RightHandMove(handMove);
        }
    }
    const Mat4 moved=Multiply(handMove,nativeToView);
    if(PistolMeshFrame(start,count))
    {
        Mat4 inverse={};if(Invert(ArmBodyToCamera(),inverse))
        {p.port=Apply(inverse,Apply(moved,{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]}));p.portFrame=g_frame;}
    }
    if(bulletDraw&&(!p.magazinePresent||PistolPoseOwned())){globals[7]=-10000;return;}
    if(clipDraw||bulletDraw)
    {
        Mat4 desired={};bool override=false;
        if(clipDraw&&(p.held||PistolAmmoPreviewActive())&&g_frame-p.handFrame<=1){desired=p.hand;override=true;}
        else if(p.pending||p.cycle||(clipDraw&&(p.held||!p.magazinePresent)))
        {globals[7]=-10000;return;} // original insertion mag/bullet hidden
        if(override)
        {
            Mat4 modelInverse={};if(!Invert(nativeToView,modelInverse))return;
            handMove=Multiply(desired,modelInverse);
        }
    }
}
V3 PistolLooseAnchor()
{
    Mat4 inverse={};if(Invert(ArmBodyToCamera(),inverse)&&g_frame-g_pistolReload.handFrame<=1)
        return Apply(Multiply(inverse,g_pistolReload.hand),{0,0,0});
    return g_pistolReload.tip;
}
bool PistolLooseFresh(){return g_frame-g_pistolReload.palmFrame<=1&&g_frame-g_pistolReload.handFrame<=1;}
#include "../src/magazine_pull286.inl"
void PistolLoseLoose(ULONGLONG now)
{
    CancelMagazinePull286(g_pistolReload);
    auto& p=g_pistolReload;p.looseFlying=p.held=false;p.contactFrames=0;
    p.nextAmmoAt=now+static_cast<ULONGLONG>(g_pistolGuides.replacementDelay*1000);
    ++p.lostObjects;Log("[AMMO PLAY] loose magazine %u lost; replacement delay %.2f s; native reserve unchanged (discard debit unverified)",p.looseId,g_pistolGuides.replacementDelay);
}
bool PistolTakeLoose(ULONGLONG now)
{
    auto& p=g_pistolReload;
    if(p.removalHeld||p.magazinePresent||p.held||p.looseFlying||p.pending||p.cycle||!p.basis||now<p.nextAmmoAt)return false;
    CancelMagazinePull286(p);p.looseId=p.nextLooseId++;p.rackMode=p.rackArmed=false;p.dropActive=false;p.held=true;p.result=0;p.contactFrames=0;p.handFrame=-100000;g_twoHand=false;
    fc3xr::Buzz(1);Log("[PISTOL RELOAD] physical magazine pickup, source %d; no reload input; loose identity %u",g_pistolGuides.ammoPickup,p.looseId);return true;
}
void PistolThrowLoose(ULONGLONG now,bool toss)
{
    auto& p=g_pistolReload;Mat4 inverse={};
    if(!p.held||!PistolLooseFresh()||!Invert(ArmBodyToCamera(),inverse))return;
    CancelMagazinePull286(p);
    p.looseBody=Multiply(inverse,p.hand);p.looseVelocity=p.handVelocity;p.looseAngular=p.handAngular;
    // Only amplify an actual upward hand release. A stationary or downward
    // release remains a drop, and lateral momentum keeps its measured direction.
    if(p.looseVelocity.y>0)p.looseVelocity.y*=g_pistolGuides.tossHeight;
    const double scale=(std::max)(.1f,g_worldScale),speed=Length(p.looseVelocity);
    if(speed>6*scale)p.looseVelocity=Scale(p.looseVelocity,6*scale/speed);
    // Keep the generous catch window for a deliberate upward flick or X toss.
    // An ordinary drop retires after one second, then the saved replacement
    // delay starts. With a 1 s setting, a new clip is ready in 2s.
    p.looseToss=toss||p.looseVelocity.y>.75*scale;
    const double angular=Length(p.looseAngular);if(angular>20)p.looseAngular=Scale(p.looseAngular,20/angular);
    // Release velocity determines direction; no artificial upward launch.
    p.catchArmed=false;p.looseFlying=true;p.held=false;p.contactFrames=0;p.looseAt=p.looseTick=now;p.looseYaw=g_bodyYaw;
    p.looseKey286={};if(g_pistolGuides.pullAssist286)ReadMagazineKey286(p.looseKey286);
    p.catchProbeCount187=0;p.catchProbeAt187=0;
    Log("[AMMO PLAY] %s magazine %u; hold grip near flying clip to catch same identity",toss?"toss":"drop",p.looseId);
}
bool PistolCatchLoose()
{
    auto& p=g_pistolReload;
    if(!p.looseFlying||!PistolLooseFresh()||Length(Sub(PistolLooseAnchor(),Apply(p.looseBody,{0,0,0})))>g_pistolGuides.catchRadius*(std::max)(.1f,g_worldScale))return false;
    CancelMagazinePull286(p);
    p.looseFlying=false;p.catchArmed=false;p.caughtFrame=g_frame;p.held=true;p.contactFrames=0;p.handFrame=-100000;g_twoHand=false;
    fc3xr::Buzz(1);Log("[AMMO PLAY] caught same magazine %u; no new ammo item",p.looseId);return true;
}
bool PistolBodyPickupNear()
{
    if(g_pistolGuides.ammoPickup==2||g_frame-g_pistolReload.palmFrame>1)return false;
    Mat4 view={},inverse={};for(int i=0;i<16;++i)view.m[i]=g_eyeViewsBody[0].view[i];
    if(!Invert(view,inverse))return false;
    const double scale=(std::max)(.1f,g_worldScale);const V3 head=Apply(inverse,{0,0,0});
    const V3 offset=g_pistolGuides.ammoPickup==0?V3{Left1911Reload312()?.18:-.18,-.65,.03}:V3{0,-.38,.12};
    return Length(Sub(g_pistolReload.tip,Add(head,Scale(offset,scale))))<=.18*scale;
}
void TickPistolLoose(const fc3pad::Controllers& c,ULONGLONG now)
{
    auto& p=g_pistolReload;
    if(PistolLooseFresh()&&p.sampleFrame!=g_frame)
    {
        const V3 tip=PistolLooseAnchor();const double dt=static_cast<double>(now-p.sampleAt)/1000;
        Mat4 invBody={},pose={};Invert(ArmBodyToCamera(),invBody);pose=Multiply(invBody,p.hand);
        const double yaw=g_bodyYaw-p.sampleYaw;Mat4 turn=HandIdentity();turn.m[0]=turn.m[10]=std::cos(yaw);turn.m[2]=std::sin(yaw);turn.m[8]=-std::sin(yaw);
        const Mat4 previous=Multiply(turn,p.samplePose);const V3 previousTip=Apply(turn,p.sampleTip);
        const bool sample=p.sampleAt&&dt>.001&&dt<.1;
        p.handVelocity=sample?Scale(Sub(tip,previousTip),1/dt):V3{};p.handAngular={};
        if(sample)for(int col=0;col<3;++col)p.handAngular=Add(p.handAngular,Scale(Cross({previous.m[col],previous.m[4+col],previous.m[8+col]},{pose.m[col],pose.m[4+col],pose.m[8+col]}),.5/dt));
        p.samplePose=pose;p.sampleYaw=g_bodyYaw;
        p.sampleAt=now;p.sampleTip=tip;p.sampleFrame=g_frame;
    }
    if(!p.looseFlying){p.pull286.ObserveGrip(c.leftGrip);if(p.returning286)CancelMagazinePull286(p);}
    if(p.looseFlying)
    {
        // Correct body-yaw changes so a flying clip does not rotate with stick turning.
        const double yaw=g_bodyYaw-p.looseYaw,cosa=std::cos(yaw),sina=std::sin(yaw);
        Mat4 turn={};turn.m[0]=turn.m[10]=cosa;turn.m[2]=sina;turn.m[8]=-sina;turn.m[5]=turn.m[15]=1;
        p.looseBody=Multiply(turn,p.looseBody);p.looseVelocity=Apply(turn,p.looseVelocity);p.looseAngular=Apply(turn,p.looseAngular);p.looseYaw=g_bodyYaw;
        if(p.returning286)p.returnVelocity286=Apply(turn,p.returnVelocity286);
        const bool returning286=TickMagazinePull286(c,now);
        if(!returning286){
        const double dt=static_cast<double>(now-p.looseTick)/1000,gravity=9.81*(std::max)(.1f,g_worldScale);
        const double angular=Length(p.looseAngular);
        if(angular>1e-6){const V3 axis=Scale(p.looseAngular,1/angular);const double a=angular*dt,cs=std::cos(a),sn=std::sin(a),k=1-cs;Mat4 spin=HandIdentity();const double v[3]={axis.x,axis.y,axis.z};for(int r=0;r<3;++r)for(int col=0;col<3;++col)spin.m[r*4+col]=(r==col?cs:0)+k*v[r]*v[col];spin.m[1]-=sn*axis.z;spin.m[2]+=sn*axis.y;spin.m[4]+=sn*axis.z;spin.m[6]-=sn*axis.x;spin.m[8]-=sn*axis.y;spin.m[9]+=sn*axis.x;const V3 origin=Apply(p.looseBody,{0,0,0});p.looseBody=Multiply(spin,p.looseBody);p.looseBody.m[3]=origin.x;p.looseBody.m[7]=origin.y;p.looseBody.m[11]=origin.z;}
        p.looseBody.m[3]+=p.looseVelocity.x*dt;p.looseBody.m[7]+=p.looseVelocity.y*dt-.5*gravity*dt*dt;p.looseBody.m[11]+=p.looseVelocity.z*dt;
        p.looseVelocity.y-=gravity*dt;p.looseTick=now;
        }
        const double distance=Length(Sub(PistolLooseAnchor(),Apply(p.looseBody,{0,0,0}))),radius=g_pistolGuides.catchRadius*(std::max)(.1f,g_worldScale);
        if(distance>radius*1.1||(now-p.looseAt>=150&&p.looseVelocity.y<=0))p.catchArmed=true;
        if(c.leftGrip>.7f&&distance<radius*1.6&&p.catchProbeCount187<5&&now-p.catchProbeAt187>=150){
            ++p.catchProbeCount187;p.catchProbeAt187=now;
            Log("[CATCH187] family %d item %u age %llu ms distance %.3f radius %.3f armed %d poseFresh %d velocityY %.3f; observational only",
                MagazineFamilyOfProfile(g_weaponMechanicProfile),p.looseId,now-p.looseAt,distance,radius,p.catchArmed?1:0,PistolLooseFresh()?1:0,p.looseVelocity.y);
        }
        if(p.catchArmed&&c.leftGrip>.7f)PistolCatchLoose();
        if(p.looseFlying&&!returning286&&now-p.looseAt>=(p.looseToss?3000:1000))PistolLoseLoose(now); // virtual lifetime, no engine floor collision assumed
    }
    if(p.held&&c.leftGrip<.35f)
    {if(PistolLooseFresh())PistolThrowLoose(now,false);else PistolLoseLoose(now);}
    const bool gripEdge=c.leftGrip>.7f&&!p.gripWas;p.gripWas=fc3pad::GripOnlyAmmoEnabled()?(c.leftGrip>.7f||(c.leftGrip>=.35f&&p.gripWas)):c.leftGrip>.7f;
    if(gripEdge&&!p.looseFlying&&!p.held&&!p.magazinePresent&&!p.pending&&!p.cycle&&!p.removalHeld&&(PistolBodyPickupNear()||(fc3pad::GripOnlyAmmoEnabled()&&!g_twoHand&&!p.rackHeld&&!p.rackReturning&&g_frame-p.rootFrame<=1&&p.rootEpoch==g_armPoseEpoch&&!PistolGuideContact(p.rootBody,0,p.tip)&&!PistolGuideContact(p.rootBody,1,p.tip)&&( !AkModel()||!PistolGuideContact(p.rootBody,3,p.tip)))))PistolTakeLoose(now);
}
bool Lever1887Ammo(UINT,UINT&,UINT&,UINT&);
// Design: dropping the clip works like reload - with a full clip X does not drop it; with a few rounds missing X
// drops it. 1 = room and spare ammo (drop), 0 = full or no spare ammo
// (keep), -1 = not known yet. The weapon: the one the reload request seam named, else the one the HUD ammo widget showed last;
// its HUD count must equal the weapon's own loaded count (a stale widget of another gun is not trusted).
int MagazineRoom(UINT& loaded,UINT& capacity,UINT& reserve)
{
    auto& p=g_pistolReload;
    const UINT recent=GetTickCount64()-g_reload.lastWidgetAt<3000?static_cast<UINT>(g_reload.lastWrapper):0;
    const UINT wrappers[2]={p.ammoWrapper,recent};
    for(const UINT wrapper:wrappers)
    {
        if(!wrapper||!Lever1887Ammo(wrapper,loaded,capacity,reserve))continue;
        const UINT weapon=ReloadRead(wrapper,0x48);if(!weapon||ReloadRead(weapon,0xcc)!=loaded)continue;
        return loaded<capacity&&reserve>0?1:0;
    }
    return -1;
}
bool MagazineRemovalAllowed54(ULONGLONG now){return now>=g_pistolReload.removalBlockedUntil54;}
void ProtectSeatedMagazine54(){g_pistolReload.removalBlockedUntil54=GetTickCount64()+2000;}
void PistolEjectSeated()
{
    auto& p=g_pistolReload;
    if(!MagazineRemovalAllowed54(GetTickCount64()))return;
    p.rackMode=p.rackArmed=false;
    p.dropActive=g_frame-p.clipFrame<=2;p.dropWorld=p.clipWorld;p.dropAt=GetTickCount64();
    p.magazinePresent=false;p.result=0;g_twoHand=false;
    fc3xr::Buzz(0);Log("[PISTOL RELOAD] LEFT grip + X ejects magazine; no native reload or ammo write");
}
void PistolXAction(const fc3pad::Controllers& c,bool edge)
{
    auto& p=g_pistolReload;
    p.ejectWaitAt=0; // physical eject must not wait on native HUD/ammo discovery
    if(!edge||p.removalHeld||p.caughtFrame==g_frame||p.pending||p.cycle||p.slideClosing||p.rackHeld||p.rackReturning)return;
    if(p.looseFlying)return;
    if(p.held){
        // The magazine stays gripped: a fresh X press during a measured upward
        // flick releases this exact loose item. A stationary X press, stale
        // motion sample, or contact with the insertion port cannot toss it.
        const double minUp=.35*(std::max)(.1f,g_worldScale);
        if(c.leftGrip>.7f&&p.sampleFrame==g_frame&&PistolLooseFresh()&&
           p.handVelocity.y>minUp&&!PistolGuideContact(p.rootBody,0,p.tip))
            PistolThrowLoose(GetTickCount64(),true);
        return;
    }
    if(fc3pad::AmmoPickupOverride(c)){if(!p.magazinePresent)PistolTakeLoose(GetTickCount64());return;}
    if(fc3pad::AmmoAction(c)==fc3pad::AmmoButtonAction::Release&&p.magazinePresent&&!p.held&&!p.needsRack&&!p.rackMode)
    {
        UINT loaded=0,capacity=0,reserve=0;
        if(MagazineRoom(loaded,capacity,reserve)>=0&&capacity&&loaded>=capacity){
            fc3xr::Buzz(0);Log("[RELOAD26] full magazine %u/%u stays seated; X did not eject",loaded,capacity);return;
        }
        PistolEjectSeated();
    }
    else if(c.leftGrip>.7f&&!p.magazinePresent&&!p.held&&p.basis)
    {
        if(g_pistolGuides.ammoPickup==2||fc3pad::AmmoPickupOverride(c))PistolTakeLoose(GetTickCount64());
    }
    else if(c.leftGrip>.7f&&p.magazinePresent&&!p.held)
    {
        if(SkorpionModel()||AkModel())return;
        p.rackMode=p.rackArmed=true;g_twoHand=false;
        Log("[PISTOL SLIDE] grip+X enables slide mode; release grip exits; magazine remains seated");
    }
}
// Native full-capacity magazines cannot start another reload. Reattach only
// verified existing ammo, without a reload pulse, reserve debit or count write.
bool PistolReseatFull()
{
    auto& p=g_pistolReload;
    const bool akUncharged=AkModel()&&p.needsRack;
    if((!akUncharged&&(!p.chamberReady||p.awaitingNativeClose||p.needsRack))||!p.state||!p.wrapper||
       !ReloadSignalLocal(p.state,p.wrapper)||!PistolLocal(p.state,p.wrapper)||ReloadRead(p.wrapper,0x48)!=p.weapon)return false;
    for(const auto& widget:g_reload.widgets)
    {
        if(static_cast<UINT>(widget.wrapper)!=p.wrapper)continue;
        UINT w[5]={};if(!AmmoRead(static_cast<UINT>(widget.hud)+0x134,w,sizeof(w)))continue;
        if(w[1]<2||w[1]>128||w[0]!=w[1]||w[0]!=ReloadRead(p.weapon,0xcc)||w[3]>100000||w[4]!=w[0]+w[3])continue;
        ProtectSeatedMagazine54();p.magazinePresent=true;p.held=p.pending=p.cycle=p.entryAccepted=false;
        p.rackMode=akUncharged;p.rackArmed=p.rackHeld=p.rackReturning=p.rackRear=false;
        p.slideClosing=false;p.result=1;p.contactFrames=0;p.poseHoldUntil=AkModel()?0:GetTickCount64()+300;
        p.waitTriggerRelease=!AkModel()&&fc3pad::Latest().rightTrigger>.3f;
        if(AkModel())p.fireResumeFrame=g_frame;
        g_reloadSignal.pistolPermissionUntil=0;fc3pad::CancelReloadRequest();
        PistolRequireManualRack50();
        Log("[PISTOL RESEAT] verified native full count %u; physical magazine seated without reload or ammo change",w[0]);return true;
    }
    return false;
}
// AK: insert = ready to fire. The engine's reload is three data callbacks the
// animation normally fires over ~3 s (Begin 0x12e19b0 -> transfer 0x12e1790
// flag0/count-1 -> End 0x12e14b0 flag0; GOSM dispatcher 0x118ed6b0). Run those
// three originals back to back on the game thread at the request seam, then let
// the entry's own predicate/signal gate deny the animation. No native wait, no
// charging, no pose hold, no fire gate. Nothing is written to ammo by hand.
bool R700Model();void R700EmptyInsert(UINT,UINT);bool BoltAfterEveryReload();bool PkmModel();void PkmNativeInsert(UINT,UINT);
bool AkInstantNativeTriple(UINT state,UINT wrapper)
{
    __try{g_nativeReloadBegin(state,wrapper);g_nativeReload(state,wrapper,0,-1);g_nativeReloadEnd(state,wrapper,0);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void InstantAkInsert(UINT state,UINT wrapper)
{
    auto& p=g_pistolReload;
    if(!g_nativeReloadBegin||!g_nativeReload||!g_nativeReloadEnd){Log("[AK INSERT] native reload originals unavailable; insertion left pending");return;}
    const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
    p.state=state;p.wrapper=wrapper;p.weapon=weapon;p.before=before;
    if(PkmModel()){PkmNativeInsert(state,wrapper);return;} // PKM = box in by hand, then the game's own reload
    if(R700Model()&&(before==0||BoltAfterEveryReload())){R700EmptyInsert(state,wrapper);return;} // Z93: the bolt after EVERY reload // EMPTY R700 = the game's own reload animation after the manual insert
    // A verified full magazine may be reseated without a native transfer.
    // Keep special PKM/bolt-action handling above on its existing path.
    if(PistolReseatFull())return;
    p.pending=p.entryAccepted=p.cycle=false; // nothing waits on the engine after this point
    g_reloadSignal.pistolPermissionUntil=0;fc3pad::CancelReloadRequest();
    const bool ok=AkInstantNativeTriple(state,wrapper);
    const UINT after=ReloadRead(weapon,0xcc);
    const bool sameOwner=ReloadSignalLocal(state,wrapper)&&ReloadRead(wrapper,0x48)==weapon;
    // With no spare rounds, the existing native loaded rounds can still be
    // seated again. This is a verified reseat, not a successful refill.
    if(sameOwner&&after==before&&before>0){
        UINT loaded=0,capacity=0,reserve=0;
        if(Lever1887Ammo(wrapper,loaded,capacity,reserve)&&loaded==before&&loaded<=capacity&&!reserve){
            ProtectSeatedMagazine54();p.magazinePresent=true;p.held=p.looseFlying=false;
            p.committed=false;p.result=1;p.contactFrames=0;p.insertionRetryBlocked=false;
            p.fireResumeFrame=g_frame;p.poseHoldUntil=0;
            fc3xr::Buzz(1);Log("[AK INSERT] existing native %u rounds reseated without reserve or refill; chamber requirement preserved",before);return;
        }
    }
    if(!sameOwner||after<=before){
        // No confirmed fill for this owner: return the same replacement item.
        // Do not manufacture seating/chambering or undo native ammo writes.
        p.committed=false;p.pending=p.entryAccepted=p.cycle=p.insertActive=false;
        p.magazinePresent=false;p.held=true;p.looseFlying=false;p.result=2;
        p.insertionRetryBlocked=true;p.contactFrames=0;p.contactFrame=-1;
        p.poseHoldUntil=0;p.fireResumeFrame=-100000;g_twoHand=false;
        g_reloadSignal.pistolPermissionUntil=0;fc3pad::CancelReloadRequest();
        Log("[AK INSERT] native fill unconfirmed; magazine %u retained, loaded %u -> %u, same owner=%d, engine ok=%d",p.looseId,before,after,sameOwner,ok);return;
    }
    p.committed=true;++p.commits;
    ProtectSeatedMagazine54();p.magazinePresent=true;p.held=p.looseFlying=false;
    p.needsRack=p.rackMode=p.rackArmed=p.rackHeld=p.rackReturning=p.rackRear=p.emptyLocked=false;p.slidePull=0;
    p.chamberReady=true;p.awaitingNativeClose=false;p.reloadChamberTracked=false;p.akRackComplete=true;
    p.fireResumeFrame=-100000;p.poseHoldUntil=0;p.waitTriggerRelease=false;p.result=1;p.contactFrames=0;
    if(CurrentRifle().emptyNeedsCharge&&before==0){
        p.needsRack=p.rackMode=true;p.rackArmed=false;p.chamberReady=false;p.akRackComplete=false;
        Log("[%ls CHARGE] empty magazine inserted: fire locked until the charging handle is pulled back and released",CurrentRifle().name);
    }
    fc3xr::Buzz(1);
    Log("[AK INSERT] seated at request seam: native Begin/transfer/End synchronous, loaded %u -> %u, pending word %08X, engine ok=%d; animation denied, fire live",before,after,ReloadRead(state,8),ok);
}
// D50 manual seating uses the same native data callbacks as rifles, but never
// inherits their fire-ready behavior: an empty pistol still needs a real rack.
bool D50NativeSeat226(UINT state,UINT wrapper){
 bool transferred=false,ended=false;
 __try{g_nativeReloadBegin(state,wrapper);g_nativeReload(state,wrapper,0,-1);transferred=true;}
 __except(EXCEPTION_EXECUTE_HANDLER){}
 // Complete native cleanup even if a callback failed; do not leave a timeline
 // or grant animation permission as a fallback.
 __try{g_nativeReloadEnd(state,wrapper,0);ended=true;}
 __except(EXCEPTION_EXECUTE_HANDLER){}
 return transferred&&ended;
}
void InstantD50Insert226(UINT state,UINT wrapper){
 auto& p=g_pistolReload;
 if(!g_nativeReloadBegin||!g_nativeReload||!g_nativeReloadEnd||(ReloadRead(state,8)&255))return;
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 const bool needsRack=before==0||p.needsRack||!p.chamberReady;
 p.state=state;p.wrapper=wrapper;p.weapon=weapon;p.before=before;
 UINT loaded=0,capacity=0,reserve=0;
 const bool known=Lever1887Ammo(wrapper,loaded,capacity,reserve)&&loaded==before&&capacity>=2&&capacity<=128&&loaded<=capacity;
 const bool reseat=known&&before>0&&(before==capacity||!reserve);
 p.pending=p.entryAccepted=p.cycle=false;g_reloadSignal.pistolPermissionUntil=0;fc3pad::CancelReloadRequest();
 const bool ok=reseat||D50NativeSeat226(state,wrapper);
 const UINT after=ReloadRead(weapon,0xcc);
 const bool same=PistolLocal(state,wrapper)&&ReloadRead(wrapper,0x48)==weapon;
 if(!ok||!same||(ReloadRead(state,8)&255)||(!reseat&&after<=before)){
  p.committed=p.insertActive=p.magazinePresent=false;p.held=true;p.looseFlying=false;
  p.result=2;p.insertionRetryBlocked=true;p.contactFrames=0;p.contactFrame=-1;p.poseHoldUntil=0;g_twoHand=false;
  Log("[D50 INSERT226] unconfirmed native seating; same magazine retained, loaded %u -> %u, same=%d ok=%d native=%08X",before,after,same,ok,ReloadRead(state,8));return;
 }
 ProtectSeatedMagazine54();p.magazinePresent=p.committed=true;p.held=p.looseFlying=false;
 if(!reseat)++p.commits;
 p.reloadChamberTracked=true;p.reloadWasEmpty=before==0;p.chamberReady=!needsRack;p.awaitingNativeClose=false;
 p.needsRack=p.rackMode=p.emptyLocked=needsRack;p.rackArmed=false;
 p.rackHeld=p.rackRear=p.rackReturning=p.slideClosing=p.d50RackComplete168=p.d50GripReleased175=false;
 p.slidePull=0;p.poseHoldUntil=0;p.result=1;p.contactFrames=0;p.waitTriggerRelease=fc3pad::Latest().rightTrigger>.3f;
 fc3throw::PublishD50Audio170(1,state,wrapper,weapon,g_armPoseEpoch,PistolAudioFlags170());
 const bool sound=fc3throw::BindD50ManualSound226(state,wrapper,weapon,g_armPoseEpoch,needsRack);
 fc3xr::Buzz(1);
 Log("[D50 INSERT226] native seating complete at request seam %u -> %u; reseat=%d needsRack=%d native=%08X seatSoundDispatch=%d; no reload animation permission",before,after,reseat,needsRack,ReloadRead(state,8),sound);
}
// Loaded Skorpion magazines cannot rely on the native animation predicate:
// a full magazine is rejected, and its HUD wrapper may differ from the equipped
// wrapper. Run the existing data callbacks at the local game-thread seam. A
// no-op may reseat the verified existing rounds; it never claims a refill.
// Empty reloads retain their accepted native timeline and manual charging.
void SkorpionLoadedInsert230(UINT state,UINT wrapper){
 auto& p=g_pistolReload;
 if(!g_nativeReloadBegin||!g_nativeReload||!g_nativeReloadEnd||(ReloadRead(state,8)&255))return;
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 if(!before)return;
 const bool needsRack=p.needsRack||!p.chamberReady;
 p.state=state;p.wrapper=wrapper;p.weapon=weapon;p.before=before;
 p.pending=p.entryAccepted=p.cycle=false;g_reloadSignal.pistolPermissionUntil=0;fc3pad::CancelReloadRequest();
 const bool ok=D50NativeSeat226(state,wrapper);
 const UINT after=ReloadRead(weapon,0xcc);
 const bool same=PistolLocal(state,wrapper)&&ReloadRead(wrapper,0x48)==weapon;
 if(!ok||!same||(ReloadRead(state,8)&255)||after<before){
  p.committed=p.insertActive=p.magazinePresent=false;p.held=true;p.looseFlying=false;
  p.result=2;p.insertionRetryBlocked=true;p.contactFrames=0;p.contactFrame=-1;p.poseHoldUntil=0;g_twoHand=false;
  Log("[SKORPION INSERT230] unconfirmed seating; same magazine retained, loaded %u -> %u owner=%d ok=%d",before,after,same,ok);return;
 }
 ProtectSeatedMagazine54();p.magazinePresent=true;p.held=p.looseFlying=p.removalHeld=p.removalDetached=false;
 p.committed=after>before;if(p.committed)++p.commits;
 p.reloadChamberTracked=true;p.reloadWasEmpty=false;p.chamberReady=!needsRack;p.awaitingNativeClose=false;
 p.needsRack=p.rackMode=needsRack;p.rackArmed=p.rackHeld=p.rackRear=p.rackReturning=p.slideClosing=p.emptyLocked=false;
 p.slidePull=0;p.poseHoldUntil=0;p.result=1;p.contactFrames=0;p.insertionRetryBlocked=false;
 p.waitTriggerRelease=fc3pad::Latest().rightTrigger>.3f;
 fc3xr::Buzz(1);
 Log("[SKORPION INSERT230] seated loaded magazine %u -> %u; refill=%d preserved charge=%d; native idle, no animation permission",before,after,p.committed,needsRack);
}
bool Lever1887Model();void Lever1887Request(UINT,UINT);bool RevolverModel();void RevolverRequest(UINT,UINT);bool FlareModel();void FlareRequest(UINT,UINT);bool RpgModel();void RpgRequest(UINT,UINT);bool Gm94Model();void Gm94Request(UINT,UINT);bool FlamerModel();void FlamerRequest(UINT,UINT);bool SpasModel();void SpasRequest(UINT,UINT);bool R700Model();void R700NativeInsert(UINT,UINT);
#include "../src/shop_unlock.inl"
bool BowModel();void BowRequest(UINT,UINT);
void ObserveMagazineReloadRequest(UINT state,UINT wrapper)
{
    TickShopUnlock(); // game thread: INSERT shop unlock request
    if(g_weaponMechanicProfile==12&&PistolHandling()&&!g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&PistolLocal(state,wrapper))
        if(fc3throw::PumpD50Sound225(state,wrapper,ReloadRead(wrapper,0x48),g_armPoseEpoch))Log("[D50 SOUND225] native slide sound dispatched after physical rear/release");
    if(BowModel()){BowRequest(state,wrapper);return;}
    if(RevolverModel()){RevolverRequest(state,wrapper);return;}
    if(FlareModel()){FlareRequest(state,wrapper);return;}
    if(RpgModel()){RpgRequest(state,wrapper);return;}
    if(Gm94Model()){Gm94Request(state,wrapper);return;}
    if(FlamerModel()){FlamerRequest(state,wrapper);return;}
    if(SpasModel()){SpasRequest(state,wrapper);return;}
    if(Lever1887Model()){Lever1887Request(state,wrapper);return;}
    if(SkorpionModel()&&g_reloadSignal.enabled&&g_reloadSignal.manualPistol&&PistolHandling()&&!g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&PistolLocal(state,wrapper)){
        auto& p=g_pistolReload;p.ammoWrapper=wrapper;
        if(p.pending&&!p.entryAccepted&&!p.cycle&&GetTickCount64()<=p.deadline&&ReloadRead(ReloadRead(wrapper,0x48),0xcc)>0){SkorpionLoadedInsert230(state,wrapper);return;}
    }
    if(D50Model()&&g_reloadSignal.enabled&&g_reloadSignal.manualPistol&&PistolHandling()&&!g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&PistolLocal(state,wrapper)){
        auto& p=g_pistolReload;p.ammoWrapper=wrapper;
        if(p.pending&&!p.entryAccepted&&!p.cycle&&GetTickCount64()<=p.deadline)InstantD50Insert226(state,wrapper);
        return;
    }
    if(MagazineProfile(g_weaponMechanicProfile)&&g_frame-g_weaponMechanicFrame<=2&&ReloadSignalLocal(state,wrapper)){
        auto& p=g_pistolReload;p.ammoWrapper=wrapper;
        // First insertion may encounter a full-mag predicate before a reload signal
        // ever fires. Discover its owner at this pre-predicate seam for pistols too.
        if(!AkModel()&&!D50Model()&&PistolHandling()&&p.pending&&!p.entryAccepted&&!fc3pad::GameplayInputBlocked()&&PistolLocal(state,wrapper)){
            p.state=state;p.wrapper=wrapper;p.weapon=ReloadRead(wrapper,0x48);
            if(PistolReseatFull())return;
            Log("[RELOAD26] pistol insertion reaches native request: loaded %u; awaiting predicate/signal",ReloadRead(p.weapon,0xcc));
        }
    }
    // Native game-thread seam before the reload predicate (request 4 entry 0x12e1bb0).
    if(!AkModel()||!g_reloadSignal.enabled||!g_reloadSignal.manualAk||!PistolHandling()||!PistolLocal(state,wrapper))return;
    auto& p=g_pistolReload;
    if(p.cycle||p.entryAccepted)return;
    const bool newOwner=p.state!=state||p.wrapper!=wrapper||p.weapon!=ReloadRead(wrapper,0x48);
    p.state=state;p.wrapper=wrapper;p.weapon=ReloadRead(wrapper,0x48);
    if(newOwner||p.pending)Log("[AK REQUEST] local pre-predicate owner %08X wrapper %08X loaded %u pending %d virtualX %d",state,wrapper,ReloadRead(p.weapon,0xcc),p.pending,fc3pad::ReloadOutput());
    if(p.pending&&GetTickCount64()<=p.deadline)InstantAkInsert(state,wrapper);
}
void TickPistolReload(bool gameplay)
{
    // Native event telemetry; the separate 225 path defers the exact empty slide event.
    fc3throw::D50AudioRecord170 audio170[16];
    const UINT audioCount170=fc3throw::DrainD50Audio170(audio170,16);
    for(UINT i=0;i<audioCount170;++i){const auto& a=audio170[i];
        Log("[D50 AUDIO170] ms=%llu begin=%llu generation=%u epoch=%u stage=%u flags=%u state=%08X wrapper=%08X weapon=%08X owner=%016llX receiverActor=%016llX weaponEntity=%016llX receiver=%08X reference=%08X weaponReference=%08X sound=%08X type=%u index=%u firstPerson=%u match=%u",a.wallMs,a.beginMs,a.generation,a.epoch,a.stage,a.flags,a.state,a.wrapper,a.weapon,a.owner,a.receiverActor,a.weaponEntity,a.receiver,a.reference,a.weaponReference,a.sound,a.type,a.index,a.firstPerson,a.receiverMatch);
    }
    static ULONGLONG audioStatusAt170=0;static UINT audioStatusLines170=0;
    static fc3throw::D50AudioStats170 priorAudio170;
    const ULONGLONG audioNow170=GetTickCount64();
    if(audioStatusLines170<32&&audioNow170-audioStatusAt170>=1000){
        audioStatusAt170=audioNow170;const auto stats=fc3throw::D50AudioStatus170();
        if(stats.recorded!=priorAudio170.recorded||stats.dropped!=priorAudio170.dropped||stats.rejectedContext!=priorAudio170.rejectedContext||stats.unknownReceiver!=priorAudio170.unknownReceiver||stats.deferred225!=priorAudio170.deferred225||stats.played225!=priorAudio170.played225||stats.cancelled225!=priorAudio170.cancelled225){
            ++audioStatusLines170;priorAudio170=stats;
            Log("[D50 SOUND225 STATUS] deferred=%u played=%u cancelled=%u",stats.deferred225,stats.played225,stats.cancelled225);
            Log("[D50 AUDIO170 STATUS] recorded=%u dropped=%u rejectedContext=%u unknownReceiver=%u",stats.recorded,stats.dropped,stats.rejectedContext,stats.unknownReceiver);
        }
    }
    const auto& audioState170=g_pistolReload;
    if(gameplay&&g_weaponMechanicProfile==12&&PistolHandling()&&!g_pistolMenu.open&&audioState170.reloadChamberTracked&&audioState170.state&&audioState170.wrapper&&PistolLocal(audioState170.state,audioState170.wrapper))
        fc3throw::PublishD50Audio170(4,audioState170.state,audioState170.wrapper,audioState170.weapon,g_armPoseEpoch,PistolAudioFlags170());
    else fc3throw::PublishD50Audio170(0,0,0,0,g_armPoseEpoch,0);
    const int activeFamily=MagazineFamilyOfProfile(g_weaponMechanicProfile);
    static int s_pistolSlotProfile=-1;
    if((g_weaponMechanicProfile==20||g_weaponMechanicProfile==1)&&g_weaponMechanicProfile!=s_pistolSlotProfile){
        auto& slot=g_magazineStates[0];
        if(s_pistolSlotProfile>=0){const bool enabled=slot.enabled;if(slot.pending||slot.cycle)fc3pad::CancelReloadRequest();slot=PistolReloadState{};slot.enabled=enabled;
            Log("[PISTOL RELOAD] pistol model changed %d -> %d; shared magazine slot reset",s_pistolSlotProfile,g_weaponMechanicProfile);}
        s_pistolSlotProfile=g_weaponMechanicProfile;
    }
    for(int i=0;i<20;++i)if(i!=activeFamily){auto& inactive=g_magazineStates[i];if(inactive.removalHeld||inactive.held||inactive.looseFlying||inactive.pending||inactive.cycle){const bool enabled=inactive.enabled;if(inactive.pending||inactive.cycle)fc3pad::CancelReloadRequest();inactive=PistolReloadState{};inactive.enabled=enabled;}}
    auto& p=g_pistolReload;const auto c=fc3pad::AmmoControls();const bool edge=c.x&&!p.x;p.x=c.x;
    const bool handling=gameplay&&PistolHandling();fc3pad::SetMagazineHandling(handling||(gameplay&&(g_weaponMechanicProfile==20||g_weaponMechanicProfile==39)&&p.enabled&&p.needsRack),PistolBusy());
    if(TickPistolGuideMenu(c,handling))return;
    if(!handling)
    {
        CancelMagazinePull286(p);
        if(p.enabled&&MagazineProfile(g_weaponMechanicProfile)){
            if(!p.inputPauseAt)p.inputPauseAt=GetTickCount64();
            // Keep ammo ownership, but an interrupted controller stroke cannot
            // resume against the old grab origin and jump to a complete rack.
            if(p.rackHeld){
                p.rackMode=p.rackArmed=p.rackHeld=p.rackRear=p.rackWristValid167=false;
                p.slidePull=p.emptyLocked?.05:0;
                if(c.rightTrigger>.3f)p.waitTriggerRelease=true;
            }
            p.handFrame=p.palmFrame=p.portFrame=-100000;p.contactFrames=0;p.sampleAt=0;p.sampleFrame=-100000;
            return; // missing tracking is not ejection, a drop, chambering or an owner change
        }
        PistolReset();p.basis=false;return;
    }
    if(g_weaponMechanicProfile==12&&p.committed&&(p.rackReturning||p.d50RackComplete168)&&
       PistolLocal(p.state,p.wrapper)&&fc3throw::RequestD50Sound225(p.state,p.wrapper,p.weapon,g_armPoseEpoch)&&GetTickCount64()-p.requestAt>=250){
        p.requestAt=GetTickCount64();fc3pad::RequestReload(); // read/dispatch seam only; no insertion permission
    }
    if(p.inputPauseAt){
        const ULONGLONG paused=GetTickCount64()-p.inputPauseAt;p.inputPauseAt=0;
        if(p.pending||p.cycle)p.deadline+=paused;
        if(p.looseFlying){p.looseAt+=paused;p.looseTick+=paused;}
        if(p.pending&&!p.entryAccepted)fc3pad::RequestReload();
    }
    if(p.cycle&&(!PistolLocal(p.state,p.wrapper)||ReloadRead(p.wrapper,0x48)!=p.weapon))
    {p.result=3;PistolReset();Log("[PISTOL RELOAD] native owner changed; physical ownership cancelled");}
    if((p.pending||p.cycle)&&GetTickCount64()>p.deadline)
    {
        if(p.pending&&!p.entryAccepted&&!p.cycle)
        {
            // No native transition was authorized: the replacement still belongs
            // to the player. Preserve its identity and return it to the hand.
            // Do not fabricate a seated magazine or change native ammo.
            p.pending=p.insertActive=false;p.held=true;p.result=2;
            p.insertionRetryBlocked=true;p.contactFrames=0;p.contactFrame=-1;
            p.poseHoldUntil=0;p.fireResumeFrame=-100000;g_twoHand=false;
            g_reloadSignal.pistolPermissionUntil=0;fc3pad::CancelReloadRequest();
            Log("[RELOAD26] request never reached native start; magazine %u returned to hand; leave insertion box before another attempt",p.looseId);
        }
        else {p.result=2;PistolReset();Log("[PISTOL RELOAD] native timeout; no retry or ammo write");}
    }
    if(p.pending&&!p.entryAccepted&&!p.cycle&&GetTickCount64()-p.requestAt>=250){
        p.requestAt=GetTickCount64();fc3pad::RequestReload(); // same bounded contact, not another magazine
    }
    TickAkRemoval(c);
    TickPistolLoose(c,GetTickCount64());
    PistolXAction(c,edge);
    TickPistolMechanics(c);

    
    const bool fresh=p.held&&g_frame-p.handFrame<=1&&g_frame-p.portFrame<=1;
    const bool inside=fresh&&PistolGuideContact(p.rootBody,0,p.tip);
    if(!inside){p.contactFrames=0;if(fresh)p.insertionRetryBlocked=false;}
    else if(!p.insertionRetryBlocked&&p.contactFrame!=g_frame)
    {
        p.contactFrame=g_frame;
        if(++p.contactFrames>=2)
        {
            BeginPistolInsert();p.held=false;p.looseFlying=false;
            if(PistolReseatFull()){PistolGuideMarkers();fc3pad::SetMagazineHandling(true,PistolBusy());return;}
            p.pending=true;p.entryAccepted=false;p.requestAt=GetTickCount64();p.deadline=p.requestAt+3000;
            fc3pad::RequestReload();Log("[PISTOL RELOAD] contact accepted; one native magazine request");
        }
    }
    PistolGuideMarkers();
    fc3pad::SetMagazineHandling(true,PistolBusy());
}
void PistolSetNote(const wchar_t* text)
{
    if(!P6p9Model()){fc3xr::SetTestNote(text);return;}
    wchar_t s[512];size_t o=0;
    for(const wchar_t* q=text;*q&&o<511;){if(q[0]==L'1'&&q[1]==L'9'&&q[2]==L'1'&&q[3]==L'1'){if(o+3>511)break;s[o++]=L'6';s[o++]=L'P';s[o++]=L'9';q+=4;}else s[o++]=*q++;}
    s[o]=0;fc3xr::SetTestNote(s);
}
void RifleSetNote(const wchar_t* text)
{
    // Rifle notes are written for the AK; show the equipped rifle's name.
    wchar_t s[512];const wchar_t* name=CurrentRifle().name;size_t o=0;
    if(text[0]==L'A'&&text[1]==L'K'){for(const wchar_t* n=name;*n&&o<500;)s[o++]=*n++;text+=2;}
    for(;*text&&o<511;)s[o++]=*text++;s[o]=0;fc3xr::SetTestNote(s);
}
bool PistolReloadNote()
{
    if(g_pistolMenu.open)return true;
    if(!PistolHandling())return false;
    auto& p=g_pistolReload;
    if(p.returning286){fc3xr::SetTestNote(L"MAGAZINE RETURNING\nKeep LEFT grip held to catch the same magazine.\nRelease grip to cancel; no reload is requested.");return true;}
    if(p.pull286.active){wchar_t note[192];swprintf_s(note,L"MAGAZINE TARGETED\nPull your LEFT hand backward %.0f cm.\nKeep grip held while the magazine returns.",g_pistolGuides.pullDistance286*100.f);fc3xr::SetTestNote(note);return true;}
    if(AkModel()){
      if(p.needsRack){wchar_t note[192];_snwprintf_s(note,192,_TRUNCATE,L"%ls - PULL THE CHARGING KNOB\nEmpty reload: release LEFT grip, grip the knob,\npull back and release. Then fire.",CurrentRifle().name);fc3xr::SetTestNote(note);return true;}
      RifleSetNote(p.removalHeld?L"AK - MAGAZINE GRABBED\nPull down, then release LEFT grip to drop.":p.held?L"AK - REPLACEMENT HELD\nMove it into the magazine insertion guide.\nIt is live the moment it seats.":p.pending||p.cycle?L"AK - SEATING\nFire when ready.":p.magazinePresent?L"AK PHYSICAL RELOAD\nLEFT grip + X without RIGHT grip to eject,\nor grip the magazine and pull down to remove.\nAfter removal, LEFT grip + X takes a replacement.":L"AK - MAGAZINE REMOVED\nLEFT grip + X takes a replacement.\nInsert it and fire. No charging.");return true;
    }
    if(SkorpionModel()){fc3xr::SetTestNote(p.needsRack?L"SKORPION - CHARGE AFTER EMPTY RELOAD\nRelease LEFT grip, then grip the right-side box.\nPull backward and release; magazine stays seated.":p.held?L"SKORPION - MAGAZINE HELD\nMove magazine into the insertion box.\nRelease LEFT grip to drop; grip near it to catch.":p.looseFlying?L"SKORPION - MAGAZINE IN AIR\nKeep LEFT grip; move hand near clip to catch.":p.pending||p.cycle?L"SKORPION - INSERTING\nOne native magazine reload; wait for completion.":p.magazinePresent?L"SKORPION PHYSICAL RELOAD\nLEFT grip + X to eject. LEFT grip + X takes magazine.\nMove it into the box to insert.":L"SKORPION - MAGAZINE OUT\nTake a magazine using your selected pickup.\nDefault: LEFT grip + X.");return true;}
    if(D50Model()&&p.cycle&&p.committed&&p.reloadWasEmpty){PistolSetNote(p.d50RackComplete168?L"D50 - SLIDE CHAMBERED\nReload is finishing. Release the trigger before firing.":L"D50 - MAGAZINE SEATED\nRelease LEFT GRIP, then grip the slide.\nPull fully back and release to chamber.");return true;}
    if(p.looseFlying){PistolSetNote(L"1911 - MAGAZINE IN AIR\nKeep LEFT grip held. The magazine catches\nautomatically when it returns near your hand.");return true;}
    if(GetTickCount64()<p.nextAmmoAt){wchar_t text[192];_snwprintf_s(text,192,_TRUNCATE,L"1911 - REPLACEMENT WAIT\nNew magazine available in %.1f seconds.",static_cast<double>(p.nextAmmoAt-GetTickCount64())/1000);PistolSetNote(text);return true;}
    if(p.held){PistolSetNote(L"1911 - MAGAZINE HELD\nMove into the gun's box to insert.\nGrip near a loose magazine to catch.\nRelease LEFT grip to drop.");return true;}
    if(!p.magazinePresent&&!p.pending&&!p.cycle){PistolSetNote(g_pistolGuides.ammoPickup==0?L"1911 - MAGAZINE OUT\nLEFT grip near your left waist takes ammo.\nPickup source is adjustable in INSERT.":g_pistolGuides.ammoPickup==1?L"1911 - MAGAZINE OUT\nLEFT grip near chest / upper stomach takes ammo.\nPickup source is adjustable in INSERT.":L"1911 - MAGAZINE OUT\nLEFT grip + tap X takes a magazine.\nPickup source is adjustable in INSERT.");return true;}
    PistolSetNote(p.rackMode&&!p.cycle&&!p.pending?(!p.rackArmed&&!p.rackHeld&&!p.rackReturning?L"1911 - RELEASE LEFT GRIP\nInsertion is complete. Release LEFT grip,\nthen grab the top rear guide and pull back.":p.rackHeld?L"1911 - SLIDE GRABBED\nPull backward along the pistol, then release.":L"1911 - RACK SLIDE\nRelease LEFT grip, then grip the top slide guide.\nPull backward and release to chamber."):p.cycle?L"1911 - INSERTING MAGAZINE\nGame handles ammo. Keep moving your hands.":p.pending?L"1911 - MAGAZINE ACCEPTED\nWaiting for the game to accept the reload.":p.held?L"1911 - MAGAZINE HELD\nKeep LEFT grip held. Move magazine into\nthe blue ring underneath the pistol grip.":p.result==1?L"1911 - MAGAZINE CONFIRMED\nSlide chambered. Fire when ready.\nLEFT grip + X to eject before another pickup.":p.result==2?L"1911 - RELOAD DID NOT FINISH\nMagazine returned. Move away from the port,\nthen insert again.":p.result==3?L"1911 - NO AMMO TRANSFER\nCheck reserve/full magazine. Exit and say done.":L"1911 PHYSICAL RELOAD\nLEFT grip + X to eject (no RIGHT grip).\nAfter ejecting, LEFT grip + X takes a magazine;\nmove it into the blue ring under the grip.");
    return true;
}



