static UINT g_beginFixtureCalls=0,g_endFixtureCalls=0,g_cycleFixtureWrapper=0,g_cycleFixtureFlag=0;
void __stdcall FixtureReloadBegin(UINT state,UINT wrapper)
{
    ++g_beginFixtureCalls;g_cycleFixtureWrapper=wrapper;
    *reinterpret_cast<UINT*>(state+4)=0;*reinterpret_cast<WORD*>(state+8)=1;
}
void __stdcall FixtureReloadEnd(UINT state,UINT wrapper,UINT flag)
{
    ++g_endFixtureCalls;g_cycleFixtureWrapper=wrapper;g_cycleFixtureFlag=flag;
    *reinterpret_cast<BYTE*>(state+8)=0;
}
#include "manual_right_reload_fixture.inl"
void TestReloadPoseLifecycle(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();
    const LONG savedFrame=g_frame;const bool savedMotion=g_motionGun,savedEye=g_eyeViewsSet,savedLeft=g_handLeftValid,savedRight=g_handRightValid,savedTwo=g_twoHand,savedAim=g_aimActive;
    float savedController[2][16],savedRef[2][16];memcpy(savedController[0],g_handRight,64);memcpy(savedController[1],g_handLeft,64);memcpy(savedRef,g_handRef,sizeof(savedRef));
    double savedQ[3][3];memcpy(savedQ,g_aimQ,sizeof(savedQ));ArmJoints savedJoints[2];memcpy(savedJoints,g_armJoints,sizeof(savedJoints));
    g_frame=50000;g_motionGun=g_eyeViewsSet=g_handLeftValid=g_handRightValid=true;g_twoHand=false;
    g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};g_reload.installed=true;g_reload.shotgunFrame=g_frame;g_physicalReloadEnabled=false;
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,24576,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"reload pose context");
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384;
    *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
    *reinterpret_cast<UINT*>(weapon+0xcc)=5;*reinterpret_cast<UINT*>(hud+0x134)=5;*reinterpret_cast<UINT*>(hud+0x138)=7;*reinterpret_cast<UINT*>(hud+0x140)=997;*reinterpret_cast<UINT*>(hud+0x144)=1002;
    g_nativeReloadBegin=FixtureReloadBegin;g_nativeReloadEnd=FixtureReloadEnd;g_nativeReload=FixtureNativeReload;g_reloadFixtureWeapon=weapon;g_reloadFixtureHud=hud;
    g_beginFixtureCalls=g_endFixtureCalls=0;
    fc3pad::Controllers input={};input.active=true;fc3pad::Publish(input);ObserveAmmoWidget(wrapper,hud);
    // A captured native shell-holding pose is sent through real GPU upload,
    // with independently controlled wrist placement. Grip cannot change it.
    const auto& f=kNativeGripFixture;FixtureBodyTurn(f);
    ID3D11Buffer* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER),*source=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    globals->SetPrivateData(kCbCopyGuid,sizeof(f.globals),f.globals);const CameraSummary summary=FixtureHandCamera(f);camera->SetPrivateData(kCameraSummaryGuid,sizeof(summary),&summary);
    float bones[576]={};memcpy(bones,f.bones,sizeof(f.bones));source->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);
    ID3D11Buffer* buffers[4]={globals,camera,nullptr,source};context->VSSetConstantBuffers(0,4,buffers);
    Check(MeasureArm(context)==1,"real native reload arm joints measured from captured palette");
    Mat4 body=HandIdentity();body.m[3]=-.28;body.m[7]=-.20;body.m[11]=.42;
    const Mat4 q=ArmBodyToCamera();Mat4 controller=Multiply(q,body);
    for(int k=0;k<16;++k){g_handLeft[k]=static_cast<float>(controller.m[k]);g_handRef[1][k]=(k==0||k==5||k==10||k==15)?1.f:0.f;}
    float originalController[16];memcpy(originalController,g_handLeft,sizeof(originalController));
    Check(BuildArmPalette(context,1)!=nullptr,"idle controlled arm before native reload");float freePose[576];Read(device,context,g_armPaletteCb[1],freePose);
    volatile UINT stackCanary=0x1234abcd;
    ReloadBeginBoundary(state,wrapper);
    Check(stackCanary==0x1234abcd&&g_beginFixtureCalls==1&&g_cycleFixtureWrapper==wrapper&&ReloadPoseActive(),"actual typed begin boundary preserves argument/stack contract and activates posing");
    Check(ReloadRead(weapon,0xcc)==5&&ReloadRead(hud,0x140)==997&&!g_reload.mode&&!g_reload.credit,"native start only selects pose; ammo ownership unchanged");
    // A complete reload interval, including zero pending between shells,
    // cannot contaminate the one-handed firing orientation baseline.
    g_shotHand=ShotHandControl{};
    for(int i=0;i<12;++i)ObserveNativeShotBasis(HandIdentity());
    Check(!g_shotHand.valid&&g_shotHand.quiet==0,"reload cycle cannot be learned as an idle firing pose");
    g_shotHand.valid=g_shotHand.active=true;g_shotHand.frame=g_frame;g_shotHand.idle=g_shotHand.current=HandIdentity();g_shotHand.idle.m[0]=g_shotHand.idle.m[10]=0;g_shotHand.idle.m[2]=1;g_shotHand.idle.m[8]=-1;
    Mat4 corrected=HandIdentity();CorrectShotHandMove(g_armJoints[1],corrected);
    Check(HandAngle(corrected,HandIdentity())<.001,"native reload cannot receive firing rotation cancellation after X release");g_shotHand=ShotHandControl{};
    float reference[576];Check(BuildArmPalette(context,1)!=nullptr,"native reload arm without grip");Read(device,context,g_armPaletteCb[1],reference);
    Mat4 inverseNative={},inverseFinal={};Check(Invert(BoneMatrix(bones,2),inverseNative)&&Invert(BoneMatrix(reference,2),inverseFinal),"reload wrist basis inverse");
    for(int bone=3;bone<=19;++bone)
    {
        const Mat4 native=Multiply(inverseNative,BoneMatrix(bones,bone)),final=Multiply(inverseFinal,BoneMatrix(reference,bone));
        Check(HandAngle(native,final)<.01&&Length(Sub(Apply(native,{0,0,0}),Apply(final,{0,0,0})))<.0003,"native shell-gripping finger pose retained without physical curl override");
    }
    for(int grip=0;grip<12;++grip)
    {
        g_twoHand=(grip%2)!=0;input.leftGrip=g_twoHand?1.f:0.f;input.leftTrigger=g_twoHand?1.f:0.f;input.leftIndexTouch=g_twoHand;fc3pad::Publish(input);
        Check(BuildArmPalette(context,1)!=nullptr,"reload arm during repeated grip changes");float actual[576];Read(device,context,g_armPaletteCb[1],actual);
        Compare(actual,reference,300,"grip/index input changes cannot hide or alter native reload pose");Check(ReloadPoseActive(),"grip release cannot finish native cycle");
    }
    *reinterpret_cast<BYTE*>(state+8)=1;ReloadBoundary(state,wrapper,0,-1);
    Check(ReloadRead(weapon,0xcc)==6&&ReloadRead(hud,0x140)==996&&ReloadPoseActive(),"shell commit clears pending but cannot end whole reload posing");
    *reinterpret_cast<BYTE*>(state+8)=1;ReloadBoundary(state,wrapper,0,-1);
    Check(ReloadRead(weapon,0xcc)==7&&ReloadRead(hud,0x140)==995&&ReloadPoseActive(),"reaching capacity alone does not prematurely hide closing animation");
    for(int turn=0;turn<9;++turn)
    {
        const double angle=turn*.12;body=HandIdentity();body.m[0]=body.m[10]=std::cos(angle);body.m[2]=std::sin(angle);body.m[8]=-std::sin(angle);
        body.m[3]=-.28+turn*.01;body.m[7]=-.20-turn*.005;body.m[11]=.42+turn*.008;controller=Multiply(q,body);
        for(int k=0;k<16;++k)g_handLeft[k]=static_cast<float>(controller.m[k]);
        g_twoHand=(turn%2)!=0;Check(BuildArmPalette(context,1)!=nullptr,"moving controller through native reload");float actual[576];Read(device,context,g_armPaletteCb[1],actual);
        const Mat4 view=Multiply(HullView(summary),HullWorld(f.globals));const V3 wrist=Apply(Multiply(view,BoneMatrix(actual,2)),{-.591,.011,1.149});
        Check(Length(Sub(wrist,ControllerWrist(g_handLeft,1)))<.0003,"reload wrist follows moving/turning controller without gun attachment locking");
    }
    ReloadEndBoundary(state,wrapper,1);Check(stackCanary==0x1234abcd&&g_endFixtureCalls==1&&g_cycleFixtureWrapper==wrapper&&g_cycleFixtureFlag==1&&!ReloadPoseActive(),"typed native end event closes posing and forwards flag/arguments unchanged");
    Check(ReloadRead(weapon,0xcc)==7&&ReloadRead(hud,0x140)==995,"pose finish does not mutate ammo");
    g_twoHand=false;input.leftGrip=input.leftTrigger=0;input.leftIndexTouch=false;fc3pad::Publish(input);memcpy(g_handLeft,originalController,sizeof(originalController));
    Check(BuildArmPalette(context,1)!=nullptr,"normal free pose after native end");float after[576];Read(device,context,g_armPaletteCb[1],after);Compare(after,freePose,300,"native end restores prior controller pose");
    float shadow[576];Check(ReadCopy(source,shadow,sizeof(shadow))==sizeof(shadow),"original source palette available");Compare(shadow,bones,300,"reload posing never mutates original source palette");

    // Manual mode removes the native left-arm/finger animation from the
    // actual uploaded palette while retaining the same native ammo lifecycle.
    const bool priorManual=g_manualReloadEnabled;
    g_manualReloadEnabled=true;g_manualReload=ManualReloadState{};
    *reinterpret_cast<UINT*>(weapon+0xcc)=5;*reinterpret_cast<UINT*>(hud+0x134)=5;*reinterpret_cast<UINT*>(hud+0x140)=997;*reinterpret_cast<UINT*>(hud+0x144)=1002;
    input.leftGrip=1;input.leftTrigger=0;input.leftIndexTouch=false;fc3pad::Publish(input);
    g_manualReload.pending=1;g_twoHand=true;UpdateTwoHand();
    Check(!g_twoHand&&ManualReloadOwnsLeftHand(),"accepted contact keeps the left hand free while native start is pending");
    ReloadBeginBoundary(state,wrapper);g_manualReload.pending=0;
    Check(ReloadPoseActive()&&ManualReloadOwnsLeftHand(),"native start retains ammo ownership while manual arm posing stays controlled");
    Mat4 nativeView=Multiply(HullView(summary),HullWorld(f.globals)),inverseView={};
    Check(Invert(nativeView,inverseView),"manual visual draw-space inverse");
    for(int step=0;step<12;++step)
    {
        input.leftGrip=(step%3)*.5f;input.leftTrigger=(step%2)?1.f:0.f;input.leftIndexTouch=(step%2)!=0;fc3pad::Publish(input);
        const double angle=step*.09;body=HandIdentity();body.m[0]=body.m[10]=std::cos(angle);body.m[2]=std::sin(angle);body.m[8]=-std::sin(angle);
        body.m[3]=-.28+step*.005;body.m[7]=-.20;body.m[11]=.42+step*.005;controller=Multiply(q,body);
        for(int k=0;k<16;++k)g_handLeft[k]=static_cast<float>(controller.m[k]);
        source->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);
        Check(MeasureArm(context)==1,"manual reference uses captured left-arm mesh");g_twoHand=false;
        Check(BuildArmPalette(context,1)!=nullptr,"manual reference controller palette");float expected[576];Read(device,context,g_armPaletteCb[1],expected);
        // Rigidly move/rotate the captured animated arm and vary its fingers.
        // The render result must depend on the controller, never these bones.
        Mat4 anim=HandIdentity();const double turn=.16*step;anim.m[0]=anim.m[5]=std::cos(turn);anim.m[1]=-std::sin(turn);anim.m[4]=std::sin(turn);
        anim.m[3]=.025*std::sin(step);anim.m[7]=.025*std::cos(step);
        const Mat4 delta=Multiply(inverseView,Multiply(anim,nativeView));float animated[576];memcpy(animated,bones,sizeof(animated));
        for(int bone=0;bone<25;++bone)StoreBone(animated,bone,Multiply(delta,BoneMatrix(bones,bone)));
        for(int bone=3;bone<=19;++bone)animated[bone*12+3]+=.003f*step;
        source->SetPrivateData(kCbCopyGuid,sizeof(animated),animated);
        Check(MeasureArm(context)==1,"varying native animation remains a valid captured arm");
        g_twoHand=true;UpdateTwoHand();Check(!g_twoHand,"held grip cannot attach the manual reload hand to the gun during the native cycle");
        Check(BuildArmPalette(context,1)!=nullptr,"manual hand during moving native animation");float actual[576];Read(device,context,g_armPaletteCb[1],actual);
        Compare(actual,expected,300,"native reload arm and finger motion cannot change controller-owned manual palette");
        const V3 wrist=Apply(Multiply(nativeView,BoneMatrix(actual,2)),{-.591,.011,1.149});
        Check(Length(Sub(wrist,ControllerWrist(g_handLeft,1)))<.0003,"manual wrist follows moving and turning controller throughout insertion");
        for(int bone=20;bone<25;++bone)
        {
            const Mat4 predicted=Multiply(g_armViewMoves[1][bone],Multiply(nativeView,BoneMatrix(animated,bone)));
            const Mat4 rendered=Multiply(nativeView,BoneMatrix(actual,bone));
            double error=0;for(int k=0;k<16;++k)error=(std::max)(error,std::fabs(predicted.m[k]-rendered.m[k])); Check(error<.0003,"sleeves receive the full animated-to-controller correction including skin scale");
        }
        float unchanged[576];Check(ReadCopy(source,unchanged,sizeof(unchanged))==sizeof(unchanged),"manual animation source retained");
        Compare(unchanged,animated,576,"manual visual override never edits the game's original palette");
        Check(ReloadRead(weapon,0xcc)==5&&ReloadRead(hud,0x140)==997,"manual visual posing cannot award ammo");
    }
    ReloadBoundary(state,wrapper,0,-1);ReloadBoundary(state,wrapper,0,-1);
    Check(ReloadRead(weapon,0xcc)==6&&ReloadRead(hud,0x140)==996&&ReloadPoseActive(),"manual visual override preserves exactly one native insertion and duplicate suppression");
    ReloadEndBoundary(state,wrapper,1);
    Check(!ReloadPoseActive()&&!ManualReloadOwnsLeftHand()&&ReloadRead(weapon,0xcc)==6,"native End releases manual hand ownership with ammo intact");
    source->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);Check(MeasureArm(context)==1,"original mesh restored after manual visual interval");
    g_manualReload=ManualReloadState{};g_manualReloadEnabled=priorManual;
    std::puts("PASS: manual reload left-arm animation suppressed on actual GPU palette; controller wrist/fingers/sleeves stable across native motion; no gun regrab while pending/cycling; native +1/duplicate/End unchanged");

    TestManualRightReloadVisual(device,context,state,wrapper,weapon,hud);
    ReloadBeginBoundary(state,wrapper+256);Check(!ReloadPoseActive(),"unregistered wrapper cannot select local reload pose");
    ReloadBeginBoundary(state,wrapper);input.active=false;fc3pad::Publish(input);Check(!ReloadPoseActive(),"controller loss clears pose ownership");input.active=true;fc3pad::Publish(input);
    ReloadBeginBoundary(state,wrapper);g_reload.shotgunFrame=g_frame-3;Check(!ReloadPoseActive(),"weapon switch clears native pose safely");g_reload.shotgunFrame=g_frame;
    ReloadBeginBoundary(state,wrapper);g_motionGun=false;Check(!ReloadPoseActive(),"motion toggle restores normal animation path");
    g_reloadPose=ReloadPoseCycle{};g_reload=PhysicalReloadState{};g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReload=nullptr;
    memcpy(g_handRight,savedController[0],64);memcpy(g_handLeft,savedController[1],64);memcpy(g_handRef,savedRef,sizeof(savedRef));memcpy(g_aimQ,savedQ,sizeof(savedQ));memcpy(g_armJoints,savedJoints,sizeof(savedJoints));
    g_frame=savedFrame;g_motionGun=savedMotion;g_eyeViewsSet=savedEye;g_handLeftValid=savedLeft;g_handRightValid=savedRight;g_twoHand=savedTwo;g_aimActive=savedAim;
    context->ClearState();vb->Release();globals->Release();camera->Release();source->Release();VirtualFree(page,0,MEM_RELEASE);fc3pad::SetShotgunHandling(false);fc3pad::Publish(fc3pad::Controllers{});
    std::puts("PASS: native reload start/end lifecycle; unchanged ammo transfer; pose survives grip/trigger and per-shell pending changes; real captured fingers and GPU wrist tracking; native end/context recovery");
}
