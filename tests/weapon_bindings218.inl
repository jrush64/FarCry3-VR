void TestRigidWeaponBindings218(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;g_oCreateBuffer=PistolFixtureCreate;
    g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=true;
    g_weaponMotionGuard=g_flatViewActive=g_twoHand=false;g_machete.held=false;g_pistolMenu.open=false;
    g_reload={};g_r700={};g_lever={};g_spas={};g_flare={};g_rpg={};g_gm94={};g_reloadAlignment=false;
    fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
    const Mat4 id=HandIdentity();++g_armPoseEpoch;g_freeRightEpoch=g_armPoseEpoch;g_freeRightFromController=id;
    for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_handBody[side][i]=g_handRef[side][i]=float(id.m[i]);
    g_handBody[0][3]=.2f;g_handBody[0][7]=-.2f;g_handBody[0][11]=.4f;memcpy(g_handRight,g_handBody[0],64);memcpy(g_handLeft,g_handBody[1],64);
    g_frameGunValid=true;g_frameGunMove=TurnAndMove({0,0,1},Unit({.3,.2,1}),{},{.1,.04,-.07});
    auto* oldOutput=g_weaponPaletteCb;g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    auto* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);auto* cameraBuffer=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
    auto* stale=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);auto* clip=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);
    float bogus[576]={};for(int b=0;b<48;++b){Mat4 m=id;m.m[3]=100+b;StoreBone(bogus,b,m);}stale->SetPrivateData(kCbCopyGuid,sizeof(bogus),bogus);context->UpdateSubresource(stale,0,nullptr,bogus,0,0);
    const float plane[4]={.1f,.2f,.3f,.4f};clip->SetPrivateData(kCbCopyGuid,sizeof(plane),plane);context->UpdateSubresource(clip,0,nullptr,plane,0,0);
    CameraSummary camera={};camera.mainView=1;camera.right[0]=camera.up[1]=camera.forward[2]=1;cameraBuffer->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);
    ID3D11VertexShader* shaders[2]={};
    for(int prefix=0;prefix<2;++prefix){
        const std::string text=std::string(prefix?"float4 AlphaValues;":"")+
            "row_major float3x4 _WorldMatrix;float3 _WorldPositionFractions;float4 main(uint id:SV_VertexID):SV_Position{"+
            "float3 p=mul(_WorldMatrix,float4(0,0,0,1))+_WorldPositionFractions;"+(prefix?"p+=AlphaValues.xyz*.00001;":"")+"return float4(p,1);}";
        auto* code=HullCompile(text.c_str(),"vs_5_0");Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shaders[prefix])),"bindings218 original rigid shader");
        shaders[prefix]->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();
    }
    auto close=[](const Mat4& a,const Mat4& b){for(int i=0;i<12;++i)if(fabs(a.m[i]-b.m[i])>4e-4)return false;return true;};
    UINT families=0;
    for(int profile:{0,1,2,7,8,9,12,14,16,17,18,20,22,23,26,31,32,34,35,36,38,39,41}){
        const auto& geometry=fc3weapon::kProfiles[profile];g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;
        g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=true;
        auto* vb=Buffer(device,geometry.vertexBytes,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,geometry.indexBytes,D3D11_BIND_INDEX_BUFFER);
        UINT stride=geometry.stride,offset=geometry.offset;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
        Mat4 reference={};bool haveReference=false;
        for(int prefix=0;prefix<2;++prefix)for(bool staleBound:{false,true})for(bool split:{false,true}){
            float rows[88]={};const int off=prefix*4;if(prefix)memcpy(rows,plane,16);
            rows[off]=rows[off+5]=rows[off+10]=1;rows[off+3]=.13f;rows[off+7]=-.07f;rows[off+11]=.2f;
            if(split){rows[off+12]=.25f;rows[off+13]=-.375f;rows[off+14]=.125f;for(int axis=0;axis<3;++axis)rows[off+axis*4+3]-=rows[off+12+axis];}
            globals->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);context->UpdateSubresource(globals,0,nullptr,rows,0,0);
            ID3D11Buffer* cb[4]={globals,cameraBuffer,nullptr,staleBound?stale:clip};context->VSSetConstantBuffers(0,4,cb);context->VSSetShader(shaders[prefix],nullptr,0);
            Check(MuzzlePaletteSlot58(context)==-1&&WeaponWorldOffset(context)==off,"bindings218 reflects shader resource and world offset independently of weapon family");
            UINT slot=99;Check(!BuildWeaponPalette(context,geometry.start,geometry.count,&slot)&&slot==99,"bindings218 rigid shader rejects unrelated 2304-byte arm buffer");
            auto* output=BuildWeaponWorld(context,geometry.start,geometry.count);Check(output!=nullptr,"bindings218 actual rigid weapon world upload");
            float result[88]={};Read(device,context,output,result);if(prefix)Compare(result,rows,4,"bindings218 AlphaValues survive every weapon's shifted world upload");
            const Mat4 world=HullWorld(result+off);
            if(!haveReference){reference=world;haveReference=true;}else Check(close(world,reference),"bindings220 rigid weapon pose agrees across color/depth layouts, high/low position encodings and stale/no-palette bindings");
        }
        ++families;vb->Release();ib->Release();
    }
    float intact[576]={};Read(device,context,stale,intact);Compare(intact,bogus,576,"bindings218 original arm palette untouched by all rigid weapons");
    context->ClearState();for(auto*& s:shaders)SafeRelease(s);globals->Release();cameraBuffer->Release();stale->Release();clip->Release();g_weaponPaletteCb->Release();g_weaponPaletteCb=oldOutput;MuzzleDiscardStage58();
    printf("PASS: bindings218 %u rigid weapon families reject stale skin buffers, share pose across actual shader world layouts, preserve AlphaValues and source palettes\n",families);
}
void TestSpecialWeaponBindings218(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;g_oCreateBuffer=PistolFixtureCreate;
    g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=true;
    g_weaponMotionGuard=g_flatViewActive=g_twoHand=false;g_machete.held=false;g_pistolMenu.open=false;
    fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
    const Mat4 id=HandIdentity();++g_armPoseEpoch;g_freeRightEpoch=g_armPoseEpoch;g_freeRightFromController=id;
    for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_handBody[side][i]=g_handRef[side][i]=float(id.m[i]);
    g_handBody[0][3]=.2f;g_handBody[0][7]=-.2f;g_handBody[0][11]=.4f;memcpy(g_handRight,g_handBody[0],64);memcpy(g_handLeft,g_handBody[1],64);
    g_frameGunValid=true;g_frameGunMove=TurnAndMove({0,0,1},Unit({.3,.2,1}),{},{.1,.04,-.07});
    auto* oldOutput=g_weaponPaletteCb;g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    auto* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);auto* cameraBuffer=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
    auto* palette=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);auto* clip=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);
    const float plane[4]={.1f,.2f,.3f,.4f};clip->SetPrivateData(kCbCopyGuid,sizeof(plane),plane);context->UpdateSubresource(clip,0,nullptr,plane,0,0);
    CameraSummary camera={};camera.mainView=1;camera.right[0]=camera.up[1]=camera.forward[2]=1;cameraBuffer->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);
    ID3D11VertexShader* shaders[2][2]={};
    for(int prefix=0;prefix<2;++prefix)for(int s=0;s<2;++s){
        const std::string text=std::string(prefix?"float4 AlphaValues;":"")+
            "row_major float3x4 _WorldMatrix;float3 _WorldPositionFractions;cbuffer BlendMatricesParameterProvider:register(b"+std::to_string(3+s)+
            "){float4 p[144];};float4 main(uint id:SV_VertexID):SV_Position{float3 q=mul(_WorldMatrix,float4(p[id].xyz,1))+_WorldPositionFractions;"+
            (prefix?"q+=AlphaValues.xyz*.00001;":"")+"return float4(q,1);}";
        auto* code=HullCompile(text.c_str(),"vs_5_0");Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shaders[prefix][s])),"bindings218 special weapon shader");
        shaders[prefix][s]->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();
    }
    UINT draws=0;
    for(int profile:{3,10,15}){
        const auto& geometry=fc3weapon::kProfiles[profile];g_weaponMechanicProfile=profile;
        auto* vb=Buffer(device,geometry.vertexBytes,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,geometry.indexBytes,D3D11_BIND_INDEX_BUFFER);
        UINT stride=geometry.stride,offset=geometry.offset;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
        std::vector<std::pair<UINT,UINT>> parts=profile==3?std::vector<std::pair<UINT,UINT>>{{486,4071},{4557,5076},{9633,6795},{16428,7311}}:
            profile==10?std::vector<std::pair<UINT,UINT>>{{0,9852},{9852,5130},{14982,7140}}:
            std::vector<std::pair<UINT,UINT>>{{0,1548},{1548,2082},{3630,8004},{13338,252},{20838,13380}};
        for(int pose=0;pose<3;++pose)for(const auto& part:parts){
            float reference[576]={};bool haveReference=false;
            const Mat4 native=TurnAndMove({0,0,1},Unit({.15+pose*.2,.2-pose*.1,1}),{},{.03*pose,-.02*pose,.1});
            float raw[576]={};for(int b=0;b<48;++b)StoreBone(raw,b,native);
            for(int prefix=0;prefix<2;++prefix)for(int s=0;s<2;++s){
                g_weaponMechanicFrame=++g_frame;g_reload={};g_r700={};g_lever={};g_reloadAlignment=false;
                g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=true;
                g_bow={};g_bow.epoch=g_armPoseEpoch;g_bow.handFrame[0]=g_bow.handFrame[1]=g_frame;
                g_bow.left=Multiply(native,BowMat(kBowLeftHandFromRoot));g_bow.right=id;g_bow.held=g_bow.nocked=true;g_bow.draw=pose*.4;
                g_bowVisualRoot58={};g_flamer={};g_flamerClip={};g_flamer.seated=true;g_flamer.leverDown=pose==1;
                if(profile==3)g_reload.shotgunFrame=g_frame;
                float rows[88]={};const int off=prefix*4;if(prefix)memcpy(rows,plane,16);rows[off]=rows[off+5]=rows[off+10]=1;
                globals->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);palette->SetPrivateData(kCbCopyGuid,sizeof(raw),raw);context->UpdateSubresource(palette,0,nullptr,raw,0,0);
                ID3D11Buffer* cb[5]={globals,cameraBuffer,nullptr,s==0?palette:clip,s==1?palette:clip};context->VSSetConstantBuffers(0,5,cb);context->VSSetShader(shaders[prefix][s],nullptr,0);
                UINT outputSlot=99;auto* output=BuildWeaponPalette(context,part.first,part.second,&outputSlot);
                Check(output&&outputSlot==UINT(3+s),"bindings218 shotgun/bow/flamethrower upload consumes actual b3/b4 palette");
                float result[576]={};Read(device,context,output,result);
                int anchor=0;if(MuzzleCore58(context,part.first,part.second,anchor)>=0){
                    Check(g_muzzleRender58.valid,"bindings218 actual core draw stages muzzle transport on b3 and b4");
                    const Mat4 expectedNative=BoneMatrix(raw,anchor),expectedFinal=BoneMatrix(result,anchor);
                    for(int i=0;i<16;++i)Check(fabs(g_muzzleRender58.native.m[i]-expectedNative.m[i])<2e-5&&fabs(g_muzzleRender58.finalWorld.m[i]-expectedFinal.m[i])<2e-5,"bindings218 staged muzzle follows the actual source/upload including shifted shader world layout");
                }
                if(!haveReference){memcpy(reference,result,sizeof(result));haveReference=true;}else Compare(result,reference,576,"bindings218 special mechanism pose agrees across b3/b4 and both world layouts");
                float original[576]={};Read(device,context,palette,original);Compare(original,raw,576,"bindings218 special mechanism preserves native palette");
                ID3D11Buffer* bound=nullptr;context->VSGetConstantBuffers(s==0?4:3,1,&bound);Check(bound==clip,"bindings218 special mechanism preserves companion clip-plane binding");SafeRelease(bound);
            }
            ++draws;
        }
        vb->Release();ib->Release();
    }
    float intact[4]={};Read(device,context,clip,intact);Compare(intact,plane,4,"bindings218 clip-plane GPU bytes remain untouched");
    context->ClearState();for(auto& pair:shaders)for(auto*& shader:pair)SafeRelease(shader);
    for(auto* b:{globals,cameraBuffer,palette,clip})b->Release();g_weaponPaletteCb->Release();g_weaponPaletteCb=oldOutput;
    g_bow={};g_flamer={};g_flamerClip={};g_reload={};MuzzleDiscardStage58();
    printf("PASS: bindings218 %u shotgun/bow/flamethrower part poses preserve mechanics and source buffers across actual b3/b4 and world layouts\n",draws);
}
void TestWeaponPaletteBindings218(ID3D11Device* device,ID3D11DeviceContext* context){
    TestRigidWeaponBindings218(device,context);
    for(int slot:{3,4})for(int profile:{28,21,25,43})TestMp5Root142(device,context,profile,slot);
    TestSpecialWeaponBindings218(device,context);
}
