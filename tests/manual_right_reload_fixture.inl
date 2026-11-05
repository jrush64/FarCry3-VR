#include "manual_right_reload_data.inl"
void TestManualRightReloadVisual(ID3D11Device* device,ID3D11DeviceContext* context,UINT state,UINT wrapper,UINT weapon,UINT hud)
{
    context->ClearState();
    const bool priorManual=g_manualReloadEnabled,priorChamber=g_chambered;
    const LONG priorShot=g_lastShotFrame;const auto priorShotHand=g_shotHand;
    const LONG priorVisibleWeapon=g_visibleWeaponDrawsLastFrame;g_visibleWeaponDrawsLastFrame=1;
    ID3D11Buffer* priorWeaponPalette=g_weaponPaletteCb;
    float priorBody124[16];memcpy(priorBody124,g_handBody[0],64);
    float priorController[16],priorReference[16];memcpy(priorController,g_handRight,64);memcpy(priorReference,g_handRef[0],64);
    g_manualReloadEnabled=true;g_twoHand=false;g_chambered=false;g_lastShotFrame=-100000;g_shotHand=ShotHandControl{};
    g_reloadPose=ReloadPoseCycle{};g_manualReload=ManualReloadState{};
    g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER),*skin=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    ID3D11Buffer* handVb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER),*gunVb=Buffer(device,257896,D3D11_BIND_VERTEX_BUFFER);
    ID3DBlob* code=HullCompile("float4 main(uint id:SV_VertexID):SV_Position{return float4(0,0,0,1);}","vs_5_0");ID3D11VertexShader* gunVs=nullptr;
    Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&gunVs)),"reload receiver recognition shader");code->Release();
    const unsigned long long hash=0xA38F7CD233BCE12Bull;gunVs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);
    ID3D11Buffer* cb[4]={globals,camera,nullptr,skin};context->VSSetConstantBuffers(0,4,cb);
    fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
    memset(g_handRef[0],0,64);g_handRef[0][0]=g_handRef[0][5]=g_handRef[0][10]=g_handRef[0][15]=1;
    auto bind=[&](const CapturedHandFixture& f,const float* allBones,ID3D11Buffer* vb,UINT offset)
    {
        FixtureBodyTurn(f);UINT stride=40;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
        globals->SetPrivateData(kCbCopyGuid,sizeof(f.globals),f.globals);
        const auto summary=FixtureHandCamera(f);camera->SetPrivateData(kCameraSummaryGuid,sizeof(summary),&summary);
        float bones[576]={};if(allBones)memcpy(bones,allBones,sizeof(bones));else memcpy(bones,f.bones,sizeof(f.bones));
        skin->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);
    };
    // The firing correction's neutral basis must survive a reload even when
    // its current pose is observed after the gun draw rather than the arm.
    bind(kManualRightIdle,nullptr,handVb,0);
    float idleBones[576]={};memcpy(idleBones,kManualRightIdle.bones,sizeof(kManualRightIdle.bones));
    const auto idleCamera=FixtureHandCamera(kManualRightIdle);
    g_chambered=true;
    for(int i=0;i<8;++i){++g_frame;g_reload.shotgunFrame=g_frame;ObserveShotHand(idleBones,kManualRightIdle.globals,idleCamera,ArmJoints{});}
    Check(g_shotHand.valid,"actual pre-reload right wrist provides a neutral basis");
    const Mat4 idleBasis=g_shotHand.idle;g_chambered=false;
    *reinterpret_cast<UINT*>(weapon+0xcc)=5;*reinterpret_cast<UINT*>(hud+0x134)=5;*reinterpret_cast<UINT*>(hud+0x140)=997;*reinterpret_cast<UINT*>(hud+0x144)=1002;
    ReloadBeginBoundary(state,wrapper);Check(ReloadPoseActive(),"right visual correction uses the real native reload lifetime");
    float lastGun[576]={};
    for(size_t step=0;step<sizeof(kManualRightReload)/sizeof(kManualRightReload[0]);++step)
    {
        const auto& f=kManualRightReload[step];++g_frame;g_reload.shotgunFrame=g_frame;
        controls.x=(step%2)!=0;fc3pad::Publish(controls);
        bind(f.hand,nullptr,handVb,0);
        const double turn=step*.14;Mat4 hand=HandIdentity();hand.m[0]=hand.m[10]=std::cos(turn);hand.m[2]=std::sin(turn);hand.m[8]=-std::sin(turn);
        hand.m[3]=.20+step*.006;hand.m[7]=-.18;hand.m[11]=.17+step*.004;
        Mat4 q=ArmBodyToCamera();const Mat4 controller=Multiply(q,hand);
        for(int k=0;k<16;++k){g_handRight[k]=static_cast<float>(controller.m[k]);g_handBody[0][k]=static_cast<float>(hand.m[k]);}
        auto arm=[&]
        {
            bind(f.hand,nullptr,handVb,0);Check(MeasureArm(context)==0,"captured reload right arm measured");
            Check(BuildArmPalette(context,0)!=nullptr,"actual GPU right-arm override during native reload");
            float actual[576];Read(device,context,g_armPaletteCb[0],actual);
            const Mat4 view=Multiply(HullView(FixtureHandCamera(f.hand)),HullWorld(f.hand.globals));
            Mat4 qi={},rendered={},expected={};Check(Invert(ArmBodyToCamera(),qi),"right reload body frame inverse");
            HandBasis(Multiply(qi,Multiply(view,BoneMatrix(actual,2))),rendered);HandBasis(Multiply(hand,g_freeRightFromController),expected);
            Check(HandAngle(rendered,expected)<.03,"native upward reload tilt cancelled while independently turning controller remains authoritative");
            // Pivot cancellation preserves the existing wrist/arm reach policy.
            const ArmJoints& joints=g_armJoints[0];const V3 wrist={joints.w[0],joints.w[1],joints.w[2]};
            Mat4 plain=ControllerHandMove(0,wrist),forearm={},upper={};Check(SolveArm(joints,plain,0,forearm,upper),"uncorrected wrist placement");
            const V3 finalWrist=Apply(Multiply(view,BoneMatrix(actual,2)),{.591,.011,1.149});
            Check(Length(Sub(finalWrist,Apply(plain,wrist)))<.0004,"reload rotation correction retains moving wrist placement and existing reach limit");
        };
        auto gun=[&]
        {
            bind(f.gun,f.gunBones,gunVb,336);context->VSSetShader(gunVs,nullptr,0);
            Check(BuildWeaponPalette(context,16428,7311)!=nullptr,"actual receiver GPU palette during right reload correction");Read(device,context,g_weaponPaletteCb,lastGun);
            const Mat4 view=Multiply(HullView(FixtureHandCamera(f.gun)),HullWorld(f.gun.globals));Mat4 qi={},rendered={},expected={};
            Check(Invert(ArmBodyToCamera(),qi),"receiver body frame inverse");
            HandBasis(Multiply(qi,Multiply(view,Multiply(BoneMatrix(lastGun,0),kShotgunHandFromGun))),rendered);HandBasis(Multiply(hand,g_freeRightFromController),expected);
            Check(HandAngle(rendered,expected)<.1,"gun-before-arm and arm-before-gun receiver poses both follow controller without native reload tilt");
            Mat4 beforeInverse={},afterInverse={};Check(Invert(BoneMatrix(f.gunBones,0),beforeInverse)&&Invert(BoneMatrix(lastGun,0),afterInverse),"receiver root inverses");
            for(int bone=1;bone<48;++bone)
            {
                if(!BoneUsable(f.gunBones+bone*12))continue; // unused palette slots contain unrelated GPU data
                const Mat4 before=Multiply(beforeInverse,BoneMatrix(f.gunBones,bone)),after=Multiply(afterInverse,BoneMatrix(lastGun,bone));
                double error=0;for(int k=0;k<16;++k)error=(std::max)(error,std::fabs(before.m[k]-after.m[k]));
                Check(error<.001,"every native child transform relative to receiver stays animated; only hand/root orientation corrected");
            }
            float untouched[576];Check(ReadCopy(skin,untouched,sizeof(untouched))==sizeof(untouched),"original receiver source available");Compare(untouched,f.gunBones,576,"right reload correction never edits native receiver source");
        };
        if(step%2){arm();gun();}else{gun();arm();}
        // This separate draw has six real moving mechanism bones, rather than
        // the receiver draw's mostly unused palette slots. Preserve every one.
        bind(kManualMechanism[step],kManualMechanismBones[step],gunVb,336);context->VSSetShader(gunVs,nullptr,0);
        Check(BuildWeaponPalette(context,486,4071)!=nullptr,"actual moving loading mechanism GPU palette");
        float mechanism[576];Read(device,context,g_weaponPaletteCb,mechanism);
        Mat4 nativeInverse={},finalInverse={};
        Check(Invert(BoneMatrix(kManualMechanismBones[step],0),nativeInverse)&&Invert(BoneMatrix(mechanism,0),finalInverse),"moving mechanism receiver-root inverses");
        int checked=0;
        for(int bone=1;bone<48;++bone)
        {
            if(!BoneUsable(kManualMechanismBones[step]+bone*12))continue;
            const Mat4 native=Multiply(nativeInverse,BoneMatrix(kManualMechanismBones[step],bone)),final=Multiply(finalInverse,BoneMatrix(mechanism,bone));
            double error=0;for(int k=0;k<16;++k)error=(std::max)(error,std::fabs(native.m[k]-final.m[k]));
            Check(error<.001,"native moving loading mechanism preserved relative to the controller-owned receiver");++checked;
        }
        Check(checked==6,"all six captured moving loading-part bones verified");
        Check(!g_chambered&&ReloadRead(weapon,0xcc)==5&&ReloadRead(hud,0x140)==997,"rendered right-hand correction neither chambers nor awards a shell");
        Check(HandAngle(g_shotHand.idle,idleBasis)<.001,"native reload cannot contaminate the neutral firing baseline");
    }
    ReloadBoundary(state,wrapper,0,-1);ReloadBoundary(state,wrapper,0,-1);ReloadEndBoundary(state,wrapper,1);
    Check(!ReloadPoseActive()&&ReloadRead(weapon,0xcc)==6&&ReloadRead(hud,0x140)==996&&!g_chambered,"native one-shell transfer/End intact and spent chamber still requires pump");
    // Configured ordinary X reload and tracking/context loss keep their fallback.
    g_manualReloadEnabled=false;ReloadBeginBoundary(state,wrapper);g_shotHand.active=true;
    Mat4 untouched=HandIdentity();CorrectShotHandMove(g_armJoints[0],untouched);Check(HandAngle(untouched,HandIdentity())<.001,"ordinary X fallback reload retains native right-hand posing");ReloadEndBoundary(state,wrapper,1);
    g_manualReloadEnabled=priorManual;g_chambered=priorChamber;g_lastShotFrame=priorShot;g_shotHand=priorShotHand;
    g_visibleWeaponDrawsLastFrame=priorVisibleWeapon;
    memcpy(g_handBody[0],priorBody124,64);memcpy(g_handRight,priorController,64);memcpy(g_handRef[0],priorReference,64);g_manualReload=ManualReloadState{};
    context->ClearState();SafeRelease(g_weaponPaletteCb);g_weaponPaletteCb=priorWeaponPalette;
    SafeRelease(globals);SafeRelease(camera);SafeRelease(skin);SafeRelease(handVb);SafeRelease(gunVb);SafeRelease(gunVs);
    std::puts("PASS: actual 69-degree native right reload tilt cancelled on GPU arm/receiver palettes; moving/turning controller and both draw orders; all child mechanism transforms/source retained; native +1/End and required pump unchanged; ordinary X fallback");
}
