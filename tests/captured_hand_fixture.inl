#include "captured_hand_data.inl"
CameraSummary FixtureHandCamera(const CapturedHandFixture& f)
{
    CameraSummary c={};memcpy(c.right,f.camera,12);memcpy(c.up,f.camera+3,12);memcpy(c.forward,f.camera+6,12);
    memcpy(c.position,f.camera+9,12);c.xScale=f.camera[12];c.yScale=f.camera[13];c.zScale=f.camera[14];c.zOffset=f.camera[15];c.mainView=1;return c;
}
void FixtureBodyTurn(const CapturedHandFixture& f)
{g_aimActive=true;for(int r=0;r<3;++r)for(int c=0;c<3;++c)g_aimQ[r][c]=f.q[r*3+c];}
void TestCapturedHands(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
    g_motionGun=g_eyeViewsSet=g_handRightValid=true;g_twoHand=false;g_reload=PhysicalReloadState{};
    for(int shot=0;shot<11;++shot)
    {
        g_shotHand=ShotHandControl{};g_chambered=true;g_lastShotFrame=-100000;
        const auto& before=kShotFixture[shot][0];const auto& peak=kShotFixture[shot][1];
        const CameraSummary camera=FixtureHandCamera(before);FixtureBodyTurn(before);
        float raw[576]={};memcpy(raw,before.bones,sizeof(before.bones));ArmJoints joints={};
        for(int i=0;i<9;++i){g_frame=20000+shot*100+i;g_reload.shotgunFrame=g_frame;ObserveShotHand(raw,before.globals,camera,joints);}
        Check(g_shotHand.valid&&!g_shotHand.active,"real pre-shot pose learns a quiet native baseline");
        g_lastShotFrame=g_frame;g_chambered=false;++g_frame;g_reload.shotgunFrame=g_frame;
        const CameraSummary afterCamera=FixtureHandCamera(peak);FixtureBodyTurn(peak);memcpy(raw,peak.bones,sizeof(peak.bones));
        ObserveShotHand(raw,peak.globals,afterCamera,joints);Check(g_shotHand.active&&g_shotHand.departed,"real shot arms correction only after baseline is known");
        const Mat4 native=Multiply(HullView(afterCamera),Multiply(HullWorld(peak.globals),BoneMatrix(raw,2)));
        const V3 wrist=Apply(native,{.591,.011,1.149});joints.w[0]=wrist.x;joints.w[1]=wrist.y;joints.w[2]=wrist.z;
        Mat4 controllerMove=HandIdentity();const double turn=.35;controllerMove.m[0]=controllerMove.m[10]=std::cos(turn);controllerMove.m[2]=std::sin(turn);controllerMove.m[8]=-std::sin(turn);
        controllerMove.m[3]=.1;controllerMove.m[7]=-.07;controllerMove.m[11]=.12;
        const V3 expectedWrist=Apply(controllerMove,wrist);Mat4 corrected=controllerMove;CorrectShotHandMove(joints,corrected);
        Check(Length(Sub(Apply(corrected,wrist),expectedWrist))<1e-8,"all real shot corrections retain wrist placement and translation");
        Mat4 finalBasis={},expectedBasis={};HandBasis(Multiply(corrected,native),finalBasis);
        HandBasis(Multiply(controllerMove,g_shotHand.idle),expectedBasis);
        Check(HandAngle(finalBasis,expectedBasis)<.002,"all eleven captured firing turns cancel while controller rotation remains");
        // Production controller path with a moving game camera. Body-frame
        // output must follow H once, even when aiming rotates Q the other way.
        float savedRef[16],savedController[16];memcpy(savedRef,g_handRef[0],sizeof(savedRef));memcpy(savedController,g_handRight,sizeof(savedController));
        memset(g_handRef[0],0,sizeof(savedRef));g_handRef[0][0]=g_handRef[0][5]=g_handRef[0][10]=g_handRef[0][15]=1;
        joints.valid=true;joints.e[0]=wrist.x-.15;joints.e[1]=wrist.y+.15;joints.e[2]=wrist.z-.15;
        for(int step=0;step<9;++step)
        {
            const double angle=step*.12,cameraTurn=-angle*.8;
            Mat4 h=HandIdentity(),q=HandIdentity();h.m[0]=h.m[10]=std::cos(angle);h.m[2]=std::sin(angle);h.m[8]=-std::sin(angle);
            q.m[0]=q.m[10]=std::cos(cameraTurn);q.m[2]=std::sin(cameraTurn);q.m[8]=-std::sin(cameraTurn);
            g_aimActive=true;for(int r=0;r<3;++r)for(int c=0;c<3;++c)g_aimQ[r][c]=q.m[r*4+c];
            Mat4 controller=Multiply(q,h);controller.m[3]=.25;controller.m[7]=-.2;controller.m[11]=.4;
            for(int k=0;k<16;++k)g_handRight[k]=static_cast<float>(controller.m[k]);
            Mat4 production={},qInverse={},rendered={},expected={};
            Check(GunMove(joints,ArmJoints{},production),"real GunMove with independently moving controller and game camera");Check(Invert(q,qInverse),"moving camera inverse");
            HandBasis(Multiply(qInverse,Multiply(production,native)),rendered);HandBasis(Multiply(h,g_shotHand.idle),expected);
            Check(HandAngle(rendered,expected)<.002,"camera aiming cannot cancel, lock or double controller rotation after firing");
        }
        memcpy(g_handRef[0],savedRef,sizeof(savedRef));memcpy(g_handRight,savedController,sizeof(savedController));FixtureBodyTurn(peak);
        Mat4 single=controllerMove;CorrectShotHandMove(joints,single);g_twoHand=true;Mat4 held=controllerMove;CorrectShotHandMove(joints,held);Check(HandAngle(held,single)<.001&&HandAngle(held,controllerMove)>1,"two-hand grip removes the same native after-shot rotation as one-handed (reported: shotgun animated on its own two-handed)");g_twoHand=false;
        controls.x=true;fc3pad::Publish(controls);held=controllerMove;CorrectShotHandMove(joints,held);Check(HandAngle(held,controllerMove)>1,"ordinary interaction X cannot disable physical shot correction");controls.x=false;fc3pad::Publish(controls);
        g_chambered=true;++g_frame;g_reload.shotgunFrame=g_frame;FixtureBodyTurn(before);memcpy(raw,before.bones,sizeof(before.bones));ObserveShotHand(raw,before.globals,camera,joints);
        Check(!g_shotHand.active,"return of native orientation releases correction without a timer");
    }
    g_handRightValid=false;float empty[576]={},globals[88]={};CameraSummary c={};ArmJoints j={};ObserveShotHand(empty,globals,c,j);Check(!g_shotHand.valid&&!g_shotHand.active,"tracking loss clears shot pose state");g_handRightValid=true;
    // Reconstruct the exact source pose with its native relative finger palette.
    const auto& f=kNativeGripFixture;float bones[576]={};memcpy(bones,f.bones,sizeof(f.bones));
    ID3D11Buffer* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    g_reload.held=true;g_reload.mode=1;g_reload.shotgunFrame=g_frame;Check(CapturedReloadGrip(context,bones),"known native hand profile accepts captured grip");Compare(bones,f.bones,300,"captured finger control reproduces actual native shell-holding frame");
    const Mat4 hand=BoneMatrix(bones,2);Mat4 movement=HandIdentity();movement.m[3]=.22;movement.m[7]=-.31;movement.m[11]=.14;
    StoreBone(bones,2,Multiply(movement,hand));Check(CapturedReloadGrip(context,bones),"captured grip follows a moved wrist");
    for(int b=3;b<=19;++b){const Mat4 expected=Multiply(movement,BoneMatrix(f.bones,b));float actual[12],want[12];memcpy(actual,bones+b*12,sizeof(actual));for(int k=0;k<12;++k)want[k]=static_cast<float>(expected.m[k]);Compare(actual,want,12,"finger bones move with the wrist, preserving native grip");}
    Compare(bones,f.bones,24,"upper arm and forearm remain untouched by finger control");
    ID3D11Buffer* cb0=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*cb1=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);cb0->SetPrivateData(kCbCopyGuid,sizeof(f.globals),f.globals);c=FixtureHandCamera(f);cb1->SetPrivateData(kCameraSummaryGuid,sizeof(c),&c);ID3D11Buffer* cb[2]={cb0,cb1};context->VSSetConstantBuffers(0,2,cb);
    memcpy(bones,f.bones,sizeof(f.bones));FixtureBodyTurn(f);RecordReloadHand(context,bones);
    Mat4 target={};Check(CapturedShellTarget(target),"native grip records final rendered hand attachment");
    const Mat4 recorded=BoneMatrix(kNativeShellFixture,0);Check(HandAngle(target,recorded)<.002,"attached shell preserves native captured orientation");
    std::printf("Native shell reconstruction error %.9f m\n",Length(Sub(Apply(target,kCapturedShellCentre),Apply(recorded,kCapturedShellCentre))));
    Check(Length(Sub(Apply(target,kCapturedShellCentre),Apply(recorded,kCapturedShellCentre)))<1e-4,"attached shell reconstructs actual native shell position");
    // Submillimetre tolerance includes float camera/world cancellation (0.038 mm grip, 0.124 mm port).
    const auto& port=kNativePortFixture;cb0->SetPrivateData(kCbCopyGuid,sizeof(port.globals),port.globals);c=FixtureHandCamera(port);cb1->SetPrivateData(kCameraSummaryGuid,sizeof(c),&c);FixtureBodyTurn(port);
    float gunBones[576]={};memcpy(gunBones,port.bones,sizeof(port.bones));g_reload.shotgunFrame=g_frame;RecordReloadPort(context,gunBones,16428,7311);
    Mat4 bodyInverse={};Check(Invert(ArmBodyToCamera(),bodyInverse),"port body transform");
    std::printf("Receiver flag %d reconstruction error %.9f m\n",g_reloadContact.port,Length(Sub(g_reloadContact.portBody,Apply(bodyInverse,kNativePortShellView))));
    Check(g_reloadContact.port&&Length(Sub(g_reloadContact.portBody,Apply(bodyInverse,kNativePortShellView)))<2.5e-4,"measured receiver matches actual native insertion centre at commit");
    // Native weapon draw arrives before this frame's arm. Use its current
    // attachment, not the old arm sample, for both orientation and wrist pivot.
    g_reload.held=false;g_reload.mode=0;g_shotHand.valid=true;g_shotHand.active=true;g_shotHand.shot=g_lastShotFrame;g_shotHand.epoch=g_armPoseEpoch;
    g_shotHand.current=HandIdentity();g_shotHand.idle=HandIdentity();g_chambered=false;
    const Mat4 rootToView=Multiply(HullView(c),HullWorld(port.globals));
    g_armJoints[0].valid=true;g_armJoints[0].w[0]=.25;g_armJoints[0].w[1]=-.1;g_armJoints[0].w[2]=.4;
    g_armJoints[0].e[0]=.2;g_armJoints[0].e[1]=-.2;g_armJoints[0].e[2]=.2;
    ObserveShotgunRoot(gunBones,rootToView,16428,7311);
    Mat4 recordedHand={},predictedHand={};HandBasis(BoneMatrix(kNativePortHand,0),recordedHand);predictedHand=g_shotHand.current;
    Check(g_shotHand.frame==g_frame&&g_frameGunValid&&HandAngle(predictedHand,recordedHand)<.003,"weapon-before-arm refresh matches real same-frame native hand rotation");
    const V3 predictedWrist=Apply(Multiply(rootToView,Multiply(BoneMatrix(gunBones,0),kShotgunHandFromGun)),{.591,.011,1.149});
    Check(Length(Sub(predictedWrist,Apply(BoneMatrix(kNativePortHand,0),{.591,.011,1.149})))<.001,"weapon-root wrist agrees with real native hand within float world precision");
    // Contact accepts visual alignment, rejects stale geometry and distant releases.
    g_reloadContact.port=true;g_reloadContact.portFrame=g_frame;g_reloadContact.portBody=g_reloadContact.shellBody;double distance=1;
    Check(ReloadContactDistance(distance)&&distance<1e-8,"visual shell at actual receiver gives zero insertion distance");
    g_reloadContact.portBody.x+=.2;Check(ReloadContactDistance(distance)&&distance>.19,"shell outside receiver rejects insertion distance");g_reloadContact.portFrame=g_frame-3;Check(!ReloadContactDistance(distance),"stale receiver cannot grant physical ammo credit");
    g_reloadContact.handFrame=g_frame-3;Check(!CapturedShellTarget(target),"stale hand attachment cannot replay misplaced shell");
    context->ClearState();vb->Release();cb0->Release();cb1->Release();g_reload=PhysicalReloadState{};g_reloadContact=ReloadHandContact{};g_shotHand=ShotHandControl{};g_aimActive=false;g_motionGun=false;
    std::puts("PASS: eleven real captured shot turns corrected with wrist/controller motion retained; two-hand/X bypass and native return; exact native finger pose and shell attachment; stale receiver/hand rejected");
}
