void TestControllerGrip124(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;
    g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=true;g_weaponMotionGuard=false;g_aimActive=false;g_twoHand=false;g_neutralQuietGate=true;g_machete.held=false;
    g_manualReloadEnabled=true;g_reload={};g_r700={};g_lever={};g_reloadAlignment=false;
    fc3pad::SetPistolCalibration(false);fc3pad::SetGameplayControls(true);fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
    const auto id=HandIdentity();++g_armPoseEpoch;
    for(int side=0;side<2;++side){for(int i=0;i<16;++i)g_handBody[side][i]=g_handRef[side][i]=float(id.m[i]);g_handBody[side][3]=side?-.25f:.25f;g_handBody[side][7]=-.2f;g_handBody[side][11]=.4f;for(int i=0;i<16;++i)(side?g_handLeft:g_handRight)[i]=g_handBody[side][i];}
    auto* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER),*palette=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    ID3D11Buffer* cb[4]={globals,camera,nullptr,palette};context->VSSetConstantBuffers(0,4,cb);
    float rows[88]={};rows[0]=rows[5]=rows[10]=1;globals->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);
    CameraSummary cam={};cam.right[0]=cam.up[1]=cam.forward[2]=1;cam.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
    auto* oldArm=g_armPaletteCb[0];g_armPaletteCb[0]=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    float native[576]={};for(int b=0;b<48;++b)StoreBone(native,b,id);memcpy(native,kNeutralArmView[0],sizeof(kNeutralArmView[0]));
    auto close=[](const Mat4& a,const Mat4& b){for(int i=0;i<12;++i)if(fabs(a.m[i]-b.m[i])>3e-4)return false;return true;};
    // Pistols, multiple rifle families, shotguns, revolver and special weapons.
    for(int profile:{12,20,8,7,41,28,14,9,31,25,21,17,35,2,36,22,23,43,34,42,16,32,18,15,3,39})for(bool two:{false,true}){
        g_weaponMechanicProfile=profile;g_twoHand=two;g_pistolReload={};g_pistolReload.enabled=true;
        g_frame=90000+profile*200;g_weaponMechanicFrame=g_frame;g_reload.shotgunFrame=profile==3?g_frame:-100000;
        g_armJoints[0]={{.2,-.15,-.2},{.35,-.3,.05},{.35,-.2,.4},true};g_frameGunValid=true;g_frameGunMove=id;
        SeedControllerHand124();Mat4 initialRoot={},grip={};Check(ControllerCurrentRoot124(initialRoot,grip),"all supported firearm groups have a controller-owned root before any arm draw");
        for(int frame=0;frame<45;++frame){
            ++g_frame;g_weaponMechanicFrame=g_frame;if(profile==3)g_reload.shotgunFrame=g_frame;
            Mat4 animation=id;const double t=frame*.31;animation.m[0]=animation.m[5]=cos(t);animation.m[1]=-sin(t);animation.m[4]=sin(t);animation.m[3]=.12*sin(t);animation.m[7]=.18*cos(t);animation.m[11]=.07*sin(t*.3);
            float moving[576];memcpy(moving,native,sizeof(moving));for(int b=0;b<25;++b)StoreBone(moving,b,Multiply(animation,BoneMatrix(native,b)));palette->SetPrivateData(kCbCopyGuid,sizeof(moving),moving);
            g_armJoints[0].w[2]+=.01*sin(t);Check(BuildArmPalette(context,0)!=nullptr,"animated native input still builds actual right-arm GPU palette");float result[576];Read(device,context,g_armPaletteCb[0],result);
            Mat4 root={},currentGrip={},desired={};Check(ControllerCurrentRoot124(root,currentGrip)&&ControllerRightHand124(desired),"fixed firearm root and right-hand target remain available during sprint animation");
            Check(close(initialRoot,root),"native walk/sprint/equip changes cannot move the fixed gun root");
            Check(close(BoneMatrix(result,2),desired),"actual uploaded right wrist follows the shared grip target, never native animation");
            const float* smgGrip=profile==7?kA2000RightGripRelative:profile==28?kMp5RightIdleFingers:profile==9?kBz19RightIdleFingers:profile==21?kVectorRightIdleFingers:profile==39?kSkorpionRightGripRelative:nullptr;
            if(smgGrip)for(int b=3;b<=19;++b)Check(close(BoneMatrix(result,b),Multiply(desired,BoneMatrix(smgGrip,b-3))),"every SMG normal hold keeps its own captured trigger-hand fingers relative to the fixed wrist");
            Check(close(Multiply(root,currentGrip),desired),"uploaded hand and displayed weapon root retain exactly the same grip one- and two-handed");
            if(PistolModel()){const Mat4 posed=PistolParentBody(animation);Check(close(posed,root),"magazine firearm placement ignores the animated rendered root");}
        }
    }
    for(int profile:{7,28,9,21,39}){g_weaponMechanicProfile=profile;g_weaponMechanicFrame=g_frame-3;Check(SmgRightHoldingGrip141()==nullptr,"stale SMG identity cannot choose a captured hand pose");g_weaponMechanicFrame=g_frame;g_machete.held=true;Check(SmgRightHoldingGrip141()==nullptr,"held machete retains hand ownership over SMG fingers");g_machete.held=false;}
    g_weaponMechanicProfile=8;g_weaponMechanicFrame=g_frame;Check(SmgRightHoldingGrip141()==nullptr,"SMG fix cannot change unrelated rifle pose selection");
    g_twoHand=false;Mat4 before={},after={};Check(ControllerRightHand124(before),"baseline controller wrist");g_handBody[0][3]+=.1f;g_handRight[3]+=.1f;Check(ControllerRightHand124(after)&&fabs(after.m[3]-before.m[3]-.1)<1e-5,"real controller translation still moves the wrist immediately");
    g_handRightValid=false;Mat4 unavailable={},grip={};Check(!ControllerCurrentRoot124(unavailable,grip),"lost right tracking cannot fabricate a fixed weapon pose");g_handRightValid=true;
    context->ClearState();g_armPaletteCb[0]->Release();g_armPaletteCb[0]=oldArm;vb->Release();globals->Release();camera->Release();palette->Release();
    puts("PASS: controller124 actual right-wrist GPU palettes reject all native walk/sprint rotations and bob across26 firearm groups including all5 SMGs, one/two hands; shared gun grip, first-draw seed, real motion and tracking loss");
}
