void TestRevolverAssembly217(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;g_oCreateBuffer=PistolFixtureCreate;
    const auto oldRevolver=g_revolver;const LONG oldProfile=g_weaponMechanicProfile;
    const auto oldFree=g_freeRightFromController;const UINT oldEpoch=g_freeRightEpoch;
    auto* oldOutput=g_weaponPaletteCb;g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=true;g_weaponMotionGuard=false;g_twoHand=false;
    g_machete.held=false;g_weaponMechanicProfile=42;g_weaponMechanicFrame=++g_frame;g_revolver=Revolver{};
    fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
    ++g_armPoseEpoch;g_freeRightEpoch=g_armPoseEpoch;g_freeRightFromController=HandIdentity();
    Mat4 controller=HandIdentity();controller.m[0]=controller.m[5]=cos(.7);controller.m[1]=-sin(.7);controller.m[4]=sin(.7);
    controller.m[3]=.25;controller.m[7]=-.3;controller.m[11]=.45;
    for(int i=0;i<16;++i)g_handBody[0][i]=g_handRight[i]=float(controller.m[i]);
    auto close=[](const Mat4& a,const Mat4& b){for(int i=0;i<12;++i)if(fabs(a.m[i]-b.m[i])>3e-4)return false;return true;};
    Mat4 root={},grip={};Check(ControllerCurrentRoot124(root,grip),"revolver217 tracked controller root");
    const Mat4 target=Multiply(ArmBodyToCamera(),root);
    float rows[88]={};rows[0]=rows[5]=rows[10]=1;rows[3]=100;rows[7]=-80;rows[11]=3;
    auto* globals=Buffer(device,sizeof(rows),D3D11_BIND_CONSTANT_BUFFER);
    globals->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);context->UpdateSubresource(globals,0,nullptr,rows,0,0);
    auto* cameraBuffer=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
    CameraSummary camera={};camera.mainView=1;camera.right[0]=camera.up[1]=camera.forward[2]=1;camera.position[0]=100;camera.position[1]=-80;camera.position[2]=3;
    cameraBuffer->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);
    auto* source=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    auto* clip=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);const float clipRows[4]={.2f,.4f,.6f,.8f};
    clip->SetPrivateData(kCbCopyGuid,sizeof(clipRows),clipRows);context->UpdateSubresource(clip,0,nullptr,clipRows,0,0);
    ID3D11VertexShader* shaders[3]={};
    for(int i=0;i<3;++i){
        char hlsl[512]={};
        if(i<2)sprintf_s(hlsl,"cbuffer BlendMatricesParameterProvider:register(b%d){float4 p[144];};float4 main(uint id:SV_VertexID):SV_Position{return float4(p[id].w,p[id+1].w,0,1);}",i+3);
        else strcpy_s(hlsl,"float4 main(uint id:SV_VertexID):SV_Position{return float4(0,0,0,1);}");
        auto* code=HullCompile(hlsl,"vs_5_0");Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shaders[i])),"revolver217 actual shader fixture");
        shaders[i]->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();
    }
    const UINT meshes[][4]={{186104,31002,40,224},{48520,7032,40,1680},{14768,2172,32,80},{18032,2688,32,80}};
    for(int mesh=0;mesh<4;++mesh){
        auto* vb=Buffer(device,meshes[mesh][0],D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,meshes[mesh][1],D3D11_BIND_INDEX_BUFFER);
        UINT stride=meshes[mesh][2],offset=meshes[mesh][3];context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
        const UINT start=mesh==0?12:mesh==1?168:6,count=mesh==0?9432:mesh==1?3030:mesh==2?1080:1338;
        if(mesh>=2){DrawCall loaderCall={};loaderCall.a=count;loaderCall.b=start;
            const auto geometry=NativeWeaponGeometry(context,loaderCall);
            Check(RevolverBarrelMaterial217(geometry,start,count)&&!RevolverBarrelMaterial217(geometry,start,count+3),"revolver217 both actual barrel variants qualify for the loader material, unrelated draw ranges do not");}
        Mat4 relative=HandIdentity();if(mesh==1){relative.m[3]=-.00000227;relative.m[7]=.14557447;relative.m[11]=.09465650;}else if(mesh>=2){relative.m[3]=-.00000160;relative.m[7]=.12262147;relative.m[11]=.08685834;}
        for(int pass=0;pass<2;++pass)for(int turn=0;turn<8;++turn){
            float native[576]={};Mat4 animated=HandIdentity();animated.m[0]=animated.m[5]=cos(turn*.8);animated.m[1]=-sin(turn*.8);animated.m[4]=sin(turn*.8);animated.m[3]=turn*.07;animated.m[7]=-.12*turn;
            for(int b=0;b<48;++b)StoreBone(native,b,animated);
            source->SetPrivateData(kCbCopyGuid,sizeof(native),native);context->UpdateSubresource(source,0,nullptr,native,0,0);
            ID3D11Buffer* cb[5]={globals,cameraBuffer,nullptr,pass?clip:source,pass?source:clip};context->VSSetConstantBuffers(0,5,cb);context->VSSetShader(shaders[pass],nullptr,0);
            Check(MuzzlePaletteSlot58(context)==pass+3,"revolver217 reflects real b3/b4 BlendMatrices binding");
            g_animation.pending=true;g_animation.draw.state=reinterpret_cast<UINT>(context);g_animation.draw.event=0;
            UINT slot=99;g_revolver.angle=mesh==0?100:0;
            auto* output=BuildRevolverPalette217(context,start,count,slot);
            Check(output&&slot==UINT(pass+3),"revolver217 selected output binding agrees with shader");
            Check(g_animation.draw.event==(0x21700000u|slot)&&g_animation.draw.rawBytes==2304&&g_animation.draw.finalBytes==2304,"revolver217 timeline tags the actual paired palette slot");
            Compare(g_animation.draw.raw,native,576,"revolver217 timeline source comes from the consumed palette, not a clip plane or stale b3");g_animation.pending=false;
            float uploaded[576]={};Read(device,context,output,uploaded);
            Check(close(BoneMatrix(uploaded,0),Multiply(target,relative)),"receiver/sight/barrels keep the measured relative attachment through every native pose and palette slot");
            if(mesh==0)Check(close(BoneMatrix(uploaded,2),Multiply(target,RevolverCrane(100))),"revolver217 crane articulation reaches color and depth palettes");
            float unchanged[576]={};Read(device,context,source,unchanged);Compare(unchanged,native,576,"revolver217 source palette preserved");
            float plane[4]={};Read(device,context,clip,plane);Compare(plane,clipRows,4,"revolver217 user clip plane preserved");
        }
        context->VSSetShader(shaders[2],nullptr,0);g_revolver.angle=0;
        ID3D11Buffer* cb[5]={globals,cameraBuffer,nullptr,clip,source};context->VSSetConstantBuffers(0,5,cb);
        float finalRows[88];memcpy(finalRows,rows,sizeof(rows));
        Check(RevolverAssemblyWorld217(context,start,count,finalRows),"revolver217 rigid shader uses shared pinned root too");
        Mat4 expectedWorld=Multiply(HullWorld(rows),Multiply(target,relative));
        Check(close(HullWorld(finalRows),expectedWorld),"revolver217 rigid world attachment agrees with skinned pass");
        g_handRightValid=false;UINT slot=99;context->VSSetShader(shaders[0],nullptr,0);
        Check(!BuildRevolverPalette217(context,start,count,slot)&&slot==99,"revolver217 lost tracking cannot fabricate an upload or output slot");g_handRightValid=true;
        g_weaponMechanicProfile=20;Check(!RevolverAssemblyDraw217(context,start,count),"revolver217 cannot take another handgun's shared sight");
        DrawCall early247={};early247.b=start;early247.a=count;ObserveRevolverAssembly247(context,early247);
        Check(g_weaponMechanicProfile==(mesh==0||mesh>=2?42:20),"247 unique revolver receiver/barrel acquires first-draw assembly; shared optic cannot claim another gun");g_weaponMechanicProfile=42;
        vb->Release();ib->Release();
    }
    // The small alpha stream uses cb0[4:16], unlike the regular rigid pass.
    // Exercise the real world builder so its offset selection and upload are
    // covered, including preservation of the preceding AlphaValues vector.
    float alpha[88]={};alpha[0]=.13f;alpha[1]=.27f;alpha[2]=.41f;alpha[3]=.55f;memcpy(alpha+4,rows,16*sizeof(float));
    globals->SetPrivateData(kCbCopyGuid,sizeof(alpha),alpha);context->UpdateSubresource(globals,0,nullptr,alpha,0,0);
    const unsigned long long alphaHash=0x4C0DBE621F6449A7ull;shaders[2]->SetPrivateData(kShaderHashGuid,sizeof(alphaHash),&alphaHash);context->VSSetShader(shaders[2],nullptr,0);
    g_frameGunValid=true;g_frameGunMove=HandIdentity();Check(WeaponWorldOffset(context)==4,"revolver217 alpha shader selects its actual cb0 world offset");
    auto* alphaOutput=BuildWeaponWorld(context,6,1338);Check(alphaOutput!=nullptr,"revolver217 alpha world upload");float alphaFinal[88]={};Read(device,context,alphaOutput,alphaFinal);
    Compare(alphaFinal,alpha,4,"revolver217 preserves AlphaValues before the shifted matrix");
    Mat4 barrel=HandIdentity();barrel.m[3]=-.00000160;barrel.m[7]=.12262147;barrel.m[11]=.08685834;
    Check(close(HullWorld(alphaFinal+4),Multiply(HullWorld(rows),Multiply(target,barrel))),"revolver217 shifted alpha pass keeps the same barrel assembly pose");
    context->ClearState();for(auto*& s:shaders)SafeRelease(s);globals->Release();cameraBuffer->Release();source->Release();clip->Release();
    g_weaponPaletteCb->Release();g_weaponPaletteCb=oldOutput;g_revolver=oldRevolver;g_weaponMechanicProfile=oldProfile;g_freeRightFromController=oldFree;g_freeRightEpoch=oldEpoch;MuzzleDiscardStage58();
    puts("PASS: revolver217 receiver, short/long barrels and reflex retain assembly offsets in actual b3/b4 GPU uploads, crane reaches both passes, original palettes/clip planes intact, tracking and weapon guards");
}
