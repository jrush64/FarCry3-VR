#include "machete_measured_data.inl"
LONG g_macheteNativeDraws=0;
void STDMETHODCALLTYPE MacheteNativeDraw(ID3D11DeviceContext*,UINT,UINT,INT){++g_macheteNativeDraws;}
void TestMacheteHolding(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();ClearMacheteVisual();g_dev=device;g_ctx=context;g_frame=41000;
    auto* previousWeaponPalette116=g_weaponPaletteCb;g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    g_enabled=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=true;
    g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};g_manualReload=ManualReloadState{};g_reloadAlignment=false;
    g_macheteEnabled=true;g_chambered=false;g_pumpStage=1;g_pumpBaseSet=true;
    const LONG previousShot=g_lastShotFrame;fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);TickMachete(true);
    c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);
    Check(!(fc3pad::ButtonsOutput()&0x0080),"draw chord cannot reach native R3 before ownership tick");
    TickMachete(true);Check(MacheteHeld()&&(fc3pad::ButtonsOutput()&0x0080),"cold draw requests one native source swipe");
    fc3pad::CancelMacheteSource();TickMachete(true);
    Check(MacheteHeld()&&!(fc3pad::ButtonsOutput()&0x0080),"held click cannot generate another source swipe");
    c.rightStickClick=false;c.rightGrip=0;c.rightTrigger=c.leftTrigger=1;c.leftGrip=1;c.x=c.y=true;
    fc3pad::Publish(c);TickMachete(true);g_twoHand=true;UpdateTwoHand();
    Check(MacheteHeld()&&!g_twoHand&&!fc3pad::FireOutput()&&!fc3pad::ReloadOutput()&&!fc3pad::AimOutput(),"grip release keeps machete; firearm fire/aim/reload and gun regrab blocked");
    Check(!g_chambered&&g_pumpStage==1&&g_pumpBaseSet&&g_lastShotFrame==previousShot,"machete cannot reset spent firearm chamber or pump state");
    c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);
    Check(!MacheteHeld()&&!fc3pad::FireOutput(),"put-away chord restores firearm but requires trigger release");
    g_chambered=true;g_lastTrigger=0;UpdatePump(g_armJoints[0],g_armJoints[1]);
    Check(g_chambered&&g_lastShotFrame==previousShot,"stowing with squeezed trigger cannot record a phantom shot");
    c={};c.active=true;c.rightGrip=1;c.leftStickClick=c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);
    Check(!MacheteHeld()&&!(fc3pad::ButtonsOutput()&0x0080),"both-stick calibration remains separate from machete draw");
    c.leftStickClick=c.rightStickClick=false;fc3pad::Publish(c);TickMachete(true);
    g_reload.held=true;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);
    Check(!MacheteHeld(),"draw rejects an owned shell instead of stealing reload state");g_reload.held=false;
    c.rightStickClick=false;fc3pad::Publish(c);TickMachete(true);c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);
    fc3pad::CancelMacheteSource();g_machete.started=GetTickCount64()-3001;TickMachete(true);
    Check(!MacheteHeld()&&!(fc3pad::ButtonsOutput()&0x0080),"missing mesh timeout returns gun and makes no automatic retry");
    TickMachete(true);Check(!MacheteHeld(),"timeout cannot reopen from an unchanged held draw button");
    c={};c.active=true;fc3pad::Publish(c);TickMachete(true);
    for(const auto& measured:kMacheteMeasuredRoots)
        for(int k=0;k<16;++k)Check(std::fabs(measured[k]-kMacheteFromHand[0].m[k])<.000002,"independent native machete/hand pairs agree within capture precision");

    const Mat4& attachment=kMacheteFromHand[0];Mat4 inverse={};Check(Invert(attachment,inverse),"machete attachment inverse");
    const V3 anchor=Apply(inverse,{0,0,0});char shader[1400]={};
    std::snprintf(shader,sizeof(shader),"cbuffer G:register(b0){float4 g[22];};cbuffer B:register(b3){float4 b[144];};cbuffer E:register(b13){float4 e[4];};float4 main(uint id:SV_VertexID):SV_Position{float3 a=float3(%.9f,%.9f,%.9f);float2 q=float2(dot(b[0],float4(a,1)),dot(b[1],float4(a,1)));float2 p=id%%3==0?float2(-.09,-.09):id%%3==1?float2(0,.09):float2(.09,-.09);return float4(p+q+e[0].xy+g[5].xy,.5,1);}",anchor.x,anchor.y,anchor.z);
    ID3DBlob* code=HullCompile(shader,"vs_5_0");ID3D11VertexShader* vs=nullptr;
    Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs)),"machete VS");
    D3D11_INPUT_ELEMENT_DESC element={"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};ID3D11InputLayout* layout=nullptr;
    Check(SUCCEEDED(device->CreateInputLayout(&element,1,code->GetBufferPointer(),code->GetBufferSize(),&layout)),"machete layout");SafeRelease(code);
    unsigned long long hash=0xA38F7CD233BCE12Bull;vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);SetFlag(vs,kVsPatchedGuid,1);
    ID3D11PixelShader* ps[3]={};const char* pixels[3]={"float4 main():SV_Target{return float4(1,0,0,1);}","float4 main():SV_Target{return float4(0,1,0,1);}","float4 main():SV_Target{return float4(0,0,1,1);}"};
    for(int i=0;i<3;++i){code=HullCompile(pixels[i],"ps_5_0");Check(SUCCEEDED(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&ps[i])),"machete material");SafeRelease(code);}
    ID3D11Buffer* vb=Buffer(device,71720,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,12120,D3D11_BIND_INDEX_BUFFER);
    std::vector<BYTE> vertex(71720,0x4d);context->UpdateSubresource(vb,0,nullptr,vertex.data(),0,0);
    unsigned short indices[6060];for(int i=0;i<6060;++i)indices[i]=i%3;context->UpdateSubresource(ib,0,nullptr,indices,0,0);
    UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
    context->IASetInputLayout(layout);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(vs,nullptr,0);
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*cameraCb=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER),*skin=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    float data[88]={};data[0]=data[5]=data[10]=1;float bones[576]={};for(int i=0;i<48;++i)StoreBone(bones,i,HandIdentity());
    context->UpdateSubresource(skin,0,nullptr,bones,0,0);skin->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);
    CameraSummary camera={};camera.mainView=1;camera.xScale=g_mainXScale*1.5f;camera.right[0]=camera.up[1]=camera.forward[2]=1;
    cameraCb->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);ID3D11Buffer* cb[4]={globals,cameraCb,nullptr,skin};context->VSSetConstantBuffers(0,4,cb);
    D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=128;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* colors[2]={},*depths[2]={};ID3D11RenderTargetView* targets[2]={};ID3D11DepthStencilView* depth[2]={};
    ID3D11Buffer* previousEye[2]={g_eyeCb[1][0],g_eyeCb[1][1]};
    for(int eye=0;eye<2;++eye)
    {
        Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&colors[eye])),"machete eye texture");Check(SUCCEEDED(device->CreateRenderTargetView(colors[eye],nullptr,&targets[eye])),"machete target");
        auto dd=td;dd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;dd.BindFlags=D3D11_BIND_DEPTH_STENCIL;
        Check(SUCCEEDED(device->CreateTexture2D(&dd,nullptr,&depths[eye])),"machete eye depth");Check(SUCCEEDED(device->CreateDepthStencilView(depths[eye],nullptr,&depth[eye])),"machete depth view");
        g_eyeCb[1][eye]=Buffer(device,64,D3D11_BIND_CONSTANT_BUFFER);float e[16]={};e[0]=eye?.06f:-.06f;context->UpdateSubresource(g_eyeCb[1][eye],0,nullptr,e,0,0);
    }
    targets[0]->SetPrivateDataInterface(kTwinGuid,targets[1]);depth[0]->SetPrivateDataInterface(kTwinGuid,depth[1]);context->OMSetRenderTargets(1,&targets[0],depth[0]);
    D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;ID3D11RasterizerState* raster=nullptr;
    Check(SUCCEEDED(device->CreateRasterizerState(&rd,&raster)),"machete raster");context->RSSetState(raster);D3D11_VIEWPORT vp={0,0,128,128,0,1};context->RSSetViewports(1,&vp);
    const UINT starts[3]={0,2910,4704},counts[3]={2910,1794,1356};
    for(int i=0;i<3;++i)
    {
        data[20]=(i-1)*.30f;context->UpdateSubresource(globals,0,nullptr,data,0,0);globals->SetPrivateData(kCbCopyGuid,sizeof(data),data);context->PSSetShader(ps[i],nullptr,0);
        DrawCall draw={};draw.a=counts[i];draw.b=starts[i];Check(MachetePart(context,draw)==i,"exact blade/grip/guard draw profile");CaptureMachete(context,draw);
    }
    Check(MacheteReady(),"all three materials retained independently of native swipe lifetime");
    std::fill(vertex.begin(),vertex.end(),0);context->UpdateSubresource(vb,0,nullptr,vertex.data(),0,0);
    std::vector<unsigned char> retained;Check(ReadBytes(context,g_machete.parts[0].vb,retained)&&retained[0]==0x4d,"source VB reuse cannot corrupt retained machete geometry");
    c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);
    Check(MacheteHeld()&&!(fc3pad::ButtonsOutput()&0x0080),"cached equip sends no attack, even on draw click");
    c.rightGrip=0;c.rightStickClick=false;fc3pad::Publish(c);TickMachete(true);
    {
        DrawCall d={};d.a=2910;
        auto* retainedBlade117=g_machete.parts[0].vb;const auto started117=g_machete.started;
        const int nativePasses117=g_machete.nativeCount116;
        context->OMSetRenderTargets(0,nullptr,depth[0]);CaptureMachete(context,d);
        Check(MacheteHeld()&&MacheteReady()&&g_machete.parts[0].vb==retainedBlade117,"depth-only pass cannot erase acquired machete or its ownership");
        Check(!g_machetePass116[0].parts[0].ready,"depth-only pass cannot occupy a persistent colour snapshot bank");
        // A colour pass with the alternate observed shader but no compatible
        // palette reproduces the actual candidate-116 failure. Give it a new
        // format so it attempts capture independently of the good cache.
        ID3D11Texture2D* badColor117=nullptr;ID3D11RenderTargetView* badTarget117=nullptr;auto badDesc117=td;badDesc117.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
        Check(SUCCEEDED(device->CreateTexture2D(&badDesc117,nullptr,&badColor117)),"unsupported-pass texture");Check(SUCCEEDED(device->CreateRenderTargetView(badColor117,nullptr,&badTarget117)),"unsupported-pass target");
        context->OMSetRenderTargets(1,&badTarget117,depth[0]);ID3D11Buffer* empty117=nullptr;context->VSSetConstantBuffers(3,1,&empty117);
        unsigned long long alternate117=0xCB43EE096F50DBDBull;vs->SetPrivateData(kShaderHashGuid,sizeof(alternate117),&alternate117);
        CaptureMachete(context,d);CaptureMachete(context,d);
        Check(MacheteHeld()&&MacheteReady()&&g_machete.parts[0].vb==retainedBlade117&&g_machete.started==started117,"unsupported alternate colour pass preserves acquired blade, held lifetime and input ownership");
        Check(g_machete.nativeCount116==nativePasses117,"failed fallback capture leaves native pass ownership unchanged");
        vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);context->VSSetConstantBuffers(3,1,&skin);context->OMSetRenderTargets(1,&targets[0],depth[0]);SafeRelease(badTarget117);SafeRelease(badColor117);
        std::puts("PASS: candidate117 acquired machete survives depth-only and unsupported alternate colour passes; draw chord ownership and valid materials preserved");
    }
    auto readPixels=[&](int eye,double& center)
    {
        auto sd=td;sd.BindFlags=0;sd.Usage=D3D11_USAGE_STAGING;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;
        Check(SUCCEEDED(device->CreateTexture2D(&sd,nullptr,&stage)),"machete pixels staging");context->CopyResource(stage,colors[eye]);D3D11_MAPPED_SUBRESOURCE m={};
        Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&m)),"machete pixels map");int hits[3]={};center=0;
        for(int y=0;y<128;++y)for(int x=0;x<128;++x)for(int channel=0;channel<3;++channel)
            if(static_cast<BYTE*>(m.pData)[y*m.RowPitch+x*4+channel]>200){++hits[channel];center+=x;}
        context->Unmap(stage,0);SafeRelease(stage);for(int count:hits)Check(count>25,"blade/grip/guard pixels survive in each eye");center/=(hits[0]+hits[1]+hits[2]);
    };
    for(int step=0;step<6;++step)
    {
        ++g_frame;const float black[4]={};for(int eye=0;eye<2;++eye){context->ClearRenderTargetView(targets[eye],black);context->ClearDepthStencilView(depth[eye],D3D11_CLEAR_DEPTH,1,0);}
        g_machete.hand=HandIdentity();const double angle=step*.08;g_machete.hand.m[0]=g_machete.hand.m[5]=std::cos(angle);g_machete.hand.m[1]=-std::sin(angle);g_machete.hand.m[4]=std::sin(angle);g_machete.hand.m[3]=step*.035;
        g_machete.handFrame=g_frame-1;ReplayMachete(context);Check(g_machete.replayFrame!=g_frame,"stale/weapon-before-hand cannot replay previous frame attachment");
        g_machete.handFrame=g_frame;g_machete.handTarget=MacheteDrawTarget(context)+1;ReplayMachete(context);Check(g_machete.replayFrame!=g_frame,"candidate 115: a hand recorded in another pass is never drawn with this pass's camera");
        g_machete.handTarget=MacheteDrawTarget(context);ReplayMachete(context);Check(g_machete.replayFrame==g_frame,"machete persists after source draw disappears");
        double centers[2];for(int eye=0;eye<2;++eye)readPixels(eye,centers[eye]);Check(centers[1]-centers[0]>5,"machete uses both first-person eye transforms");
        float actual[576];Read(device,context,g_machete.skin[0],actual);const Mat4 expected=Multiply(g_machete.hand,kMacheteFromHand[0]);
        for(int k=0;k<12;++k)Check(std::fabs(actual[k]-expected.m[(k/4)*4+k%4])<1e-5,"actual GPU machete palette follows measured hand through movement/rotation");
        const LONG frame=g_machete.replayFrame;ReplayMachete(context);Check(g_machete.replayFrame==frame,"persistent machete replay is once per frame");
        ID3D11Buffer* restored=nullptr;context->VSGetConstantBuffers(3,1,&restored);Check(restored==skin,"native source bindings restored after both-eye machete replay");SafeRelease(restored);
    }
    // Reproduce the real second pass: different target format, independently
    // retained shader/material pipeline, both eyes populated after native
    // geometry disappears. Candidate 115 refused this pass on format alone.
    ID3D11Texture2D* otherColors116[2]={};ID3D11RenderTargetView* otherTargets116[2]={};
    auto otherDesc116=td;otherDesc116.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    for(int eye=0;eye<2;++eye){Check(SUCCEEDED(device->CreateTexture2D(&otherDesc116,nullptr,&otherColors116[eye])),"second-format texture");Check(SUCCEEDED(device->CreateRenderTargetView(otherColors116[eye],nullptr,&otherTargets116[eye])),"second-format target");}
    otherTargets116[0]->SetPrivateDataInterface(kTwinGuid,otherTargets116[1]);
    context->OMSetRenderTargets(1,&otherTargets116[0],depth[0]);
    for(int i=0;i<3;++i){data[20]=(i-1)*.30f;context->UpdateSubresource(globals,0,nullptr,data,0,0);globals->SetPrivateData(kCbCopyGuid,sizeof(data),data);context->PSSetShader(ps[i],nullptr,0);DrawCall d={};d.a=counts[i];d.b=starts[i];CaptureMachete(context,d);}
    Check(g_machete.parts[0].formats[0]==td.Format&&g_machetePass116[0].parts[0].formats[0]==otherDesc116.Format,"second-pass capture preserves the first-pass pipeline separately");
    ++g_frame;g_machete.hand=HandIdentity();g_machete.handFrame=g_frame;g_machete.handTarget=MacheteDrawTarget(context);
    const float black116[4]={};for(int eye=0;eye<2;++eye){context->ClearRenderTargetView(otherTargets116[eye],black116);context->ClearDepthStencilView(depth[eye],D3D11_CLEAR_DEPTH,1,0);}
    ReplayMachete(context);Check(g_machete.replayFrame==g_frame,"fallback draws in the second-format pass");
    for(int eye=0;eye<2;++eye)
    {
        auto sd=otherDesc116;sd.BindFlags=0;sd.Usage=D3D11_USAGE_STAGING;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;
        Check(SUCCEEDED(device->CreateTexture2D(&sd,nullptr,&stage)),"second-pass staging");context->CopyResource(stage,otherColors116[eye]);D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&mapped)),"second-pass pixels map");
        int hits[3]={};for(int y=0;y<128;++y)for(int x=0;x<128;++x)for(int c=0;c<3;++c)if(static_cast<BYTE*>(mapped.pData)[y*mapped.RowPitch+x*4+c]>200)++hits[c];
        context->Unmap(stage,0);SafeRelease(stage);for(int count:hits)Check(count>25,"blade, grip and guard reach both second-pass eye textures");
    }
    context->OMSetRenderTargets(1,&targets[0],depth[0]);for(int eye=0;eye<2;++eye){SafeRelease(otherTargets116[eye]);SafeRelease(otherColors116[eye]);}
    float unchanged[576];Read(device,context,skin,unchanged);Compare(unchanged,bones,576,"machete replay never edits native skin");

    // Exercise the actual production classifier and per-eye issue calls.
    // Retained replay uses the real GPU above; the original draw is counted
    // here so missing firearm/duplicate hiding cannot pass unnoticed.
    const auto previousDraw=g_oDrawIndexed;g_oDrawIndexed=MacheteNativeDraw;
    DrawCall sourceDraw={};sourceDraw.a=2910;g_macheteNativeDraws=0;
    HandleDraw(context,sourceDraw);Check(g_macheteNativeDraws==0,"owned source machete/weapon draw hidden in actual HandleDraw");
    // Native blade now owns every pass, including the world projection. A
    // completed fallback prevents duplicates in its first frame only.
    ++g_frame;g_machete.handAttachment116=true;g_machete.handEpoch116=g_armPoseEpoch;
    g_machete.controllerHand116=HandIdentity();
    for(int i=0;i<16;++i)g_handBody[0][i]=i%5==0?1.f:0.f;
    HandleDraw(context,sourceDraw);Check(g_macheteNativeDraws==2,"native blade is issued in both eyes after fallback hands ownership over");
    camera.xScale=g_mainXScale;cameraCb->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);g_macheteNativeDraws=0;
    HandleDraw(context,sourceDraw);Check(g_macheteNativeDraws==2,"native blade follows its controller under the world-camera projection too");
    for(int step=0;step<3;++step)
    {
        ++g_frame;g_handBody[0][3]=step*.13f;g_handBody[0][7]=step*-.08f;
        // Poison the animated root: it must not survive absolute attachment.
        bones[3]=30+step;context->UpdateSubresource(skin,0,nullptr,bones,0,0);skin->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);
        Mat4 tv={},fv={},hand={};float raw[576]={};Check(DrawSpace(context,raw,tv,fv)&&MacheteHand116(hand),"current native draw space and tracked hand available");
        auto* palette=NativeMachetePalette116(context,sourceDraw);Check(palette!=nullptr,"native blade obtains a controller palette without current-frame arm draw");
        float actual[576]={};Read(device,context,palette,actual);
        const Mat4 expected=Multiply(fv,Multiply(hand,kMacheteFromHand[0]));
        for(int k=0;k<12;++k)Check(fabs(actual[k]-expected.m[k])<1e-5,"GPU native blade ignores swipe root and follows current controller");
        Read(device,context,skin,raw);Compare(raw,bones,576,"controller attachment leaves native source palette unchanged");
    }
    V3 tipBefore116={},tipAfter116={};float savedBody116[16];memcpy(savedBody116,g_handBody[0],sizeof(savedBody116));
    Check(MacheteStrokePosition116(tipBefore116),"blade tip is available to the physical stroke detector");
    g_handBody[0][0]=g_handBody[0][5]=0;g_handBody[0][1]=-1;g_handBody[0][4]=1;
    Check(MacheteStrokePosition116(tipAfter116)&&Length(Sub(tipAfter116,tipBefore116))>.1,"wrist rotation moves the detected blade tip even with stationary controller translation");
    memcpy(g_handBody[0],savedBody116,sizeof(savedBody116));
    camera.xScale=g_mainXScale*1.5f;cameraCb->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);
    ID3D11Buffer* bodyVB=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);SetFlag(bodyVB,kBodyVbGuid,1);context->IASetVertexBuffers(0,1,&bodyVB,&stride,&offset);
    const LONG previousWeaponCount=g_visibleWeaponDrawsThisFrame;
    const bool previousCameraValid=g_frameCameraValid;const auto previousCamera=g_frameCamera;
    const auto previousProjection=g_worldProjection;
    g_frameCameraValid=false;
    HandleDraw(context,sourceDraw);
    Check(g_visibleWeaponDrawsThisFrame==previousWeaponCount&&g_frameCameraValid&&
          g_worldProjection.xs==camera.xScale&&g_worldProjection.ys==camera.yScale,
          "door projection change keeps unknown geometry on world camera and cannot activate weapon motion");
    g_frameCameraValid=previousCameraValid;g_frameCamera=previousCamera;g_worldProjection=previousProjection;
    g_macheteNativeDraws=0;HandleDraw(context,sourceDraw);Check(g_macheteNativeDraws==2,"classified body geometry remains visible during machete ownership");
    context->IASetVertexBuffers(0,1,&vb,&stride,&offset);SafeRelease(bodyVB);StowMachete("native-draw fixture");
    g_macheteNativeDraws=0;HandleDraw(context,sourceDraw);Check(g_macheteNativeDraws==2,"put-away restores native first-person weapon draws");g_oDrawIndexed=previousDraw;
    c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);Check(MacheteHeld(),"cached redraw after native classifier test");
    c.rightGrip=0;c.rightStickClick=false;fc3pad::Publish(c);TickMachete(true);

    // Real native arm poses through production GPU palette upload. Visible
    // machete attachment must use the final controlled wrist, not raw swipe.
    ID3D11Buffer* armVB=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);context->IASetVertexBuffers(0,1,&armVB,&stride,&offset);
    const bool previousAim=g_aimActive;double previousQ[3][3];memcpy(previousQ,g_aimQ,sizeof(previousQ));
    float previousController[16];memcpy(previousController,g_handRight,sizeof(previousController));const LONG previousVisible=g_visibleWeaponDrawsLastFrame;
    g_visibleWeaponDrawsLastFrame=8;++g_armPoseEpoch;
    for(int pose=0;pose<4;++pose)
    {
        ++g_frame;const auto& f=kShotFixture[pose][pose%2];camera=FixtureHandCamera(f);camera.xScale=g_mainXScale*1.5f;FixtureBodyTurn(f);
        context->UpdateSubresource(globals,0,nullptr,f.globals,0,0);globals->SetPrivateData(kCbCopyGuid,sizeof(f.globals),f.globals);
        memset(bones,0,sizeof(bones));memcpy(bones,f.bones,sizeof(f.bones));context->UpdateSubresource(skin,0,nullptr,bones,0,0);skin->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);
        cameraCb->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);Check(MeasureArm(context)==0,"actual arm draw identifies right hand");
        memset(g_handRight,0,sizeof(g_handRight));g_handRight[0]=g_handRight[5]=g_handRight[10]=g_handRight[15]=1;g_handRight[3]=.25f+pose*.025f;g_handRight[7]=-.15f;g_handRight[11]=.45f;
        Mat4 currentController118={},bodyInverse118={};for(int k=0;k<16;++k)currentController118.m[k]=g_handRight[k];Check(Invert(ArmBodyToCamera(),bodyInverse118),"controller body basis inverse");currentController118=Multiply(bodyInverse118,currentController118);for(int k=0;k<16;++k)g_handBody[0][k]=float(currentController118.m[k]);
        ID3D11Buffer* arm=BuildArmPalette(context,0);Check(arm!=nullptr&&g_machete.handFrame==g_frame,"persistent right arm uses stable IK even while underlying firearm remains drawn");
        float controlled[576];Read(device,context,arm,controlled);const Mat4 view=Multiply(HullView(camera),HullWorld(f.globals));const Mat4 hand=Multiply(view,BoneMatrix(controlled,2));
        for(int k=0;k<16;++k)Check(std::fabs(hand.m[k]-g_machete.hand.m[k])<(k==3||k==7||k==11?.001:1e-5),"GPU wrist stays on pinned controller attachment within 1 mm native float precision");
        for(int b=3;b<=19;++b)
        {
            const Mat4 expected=Multiply(BoneMatrix(controlled,2),BoneMatrix(kMacheteGripRelative,b-3));
            for(int k=0;k<12;++k)Check(std::fabs(controlled[b*12+k]-expected.m[(k/4)*4+k%4])<2e-5,"native machete finger grip follows controlled wrist");
        }
        Read(device,context,skin,unchanged);Compare(unchanged,bones,576,"persistent arm keeps original native animation palette unchanged");
    }
    memcpy(g_handRight,previousController,sizeof(previousController));g_visibleWeaponDrawsLastFrame=previousVisible;g_aimActive=previousAim;memcpy(g_aimQ,previousQ,sizeof(previousQ));
    g_handRightValid=false;TickMachete(true);Check(!MacheteHeld()&&!fc3pad::MacheteFireBlocked(),"tracking loss stows and clears input ownership");g_handRightValid=true;
    c={};c.active=true;fc3pad::Publish(c);TickMachete(true);c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);Check(MacheteHeld(),"cache survives tracking recovery");
    c.rightStickClick=false;fc3pad::Publish(c);TickMachete(true);c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);Check(!MacheteHeld(),"fresh draw chord puts machete away");
    TickMachete(false);c={};c.active=true;fc3pad::Publish(c);c.rightStickClick=true;fc3pad::Publish(c);Check(fc3pad::ButtonsOutput()&0x0080,"fresh stock R3 remains available after releasing the consumed draw/stow click");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(true);TickMachete(true);
    c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);Check(MacheteHeld(),"controls20 draw for switch-away test");
    c.rightStickClick=false;c.rightGrip=0;c.y=true;fc3pad::Publish(c);TickMachete(true);Check(!MacheteHeld()&&(fc3pad::ButtonsOutput()&0x8000),"controls20 Y stows held machete and reaches native weapon switching");
    c.y=false;fc3pad::Publish(c);fc3pad::SetGameplayControls(false);
    ClearMacheteVisual();Check(!MacheteReady()&&!g_machete.deferred,"all three machete material resources released");context->ClearState();
    for(int eye=0;eye<2;++eye){SafeRelease(g_eyeCb[1][eye]);g_eyeCb[1][eye]=previousEye[eye];SafeRelease(targets[eye]);SafeRelease(depth[eye]);SafeRelease(colors[eye]);SafeRelease(depths[eye]);}
    for(auto*& shader:ps)SafeRelease(shader);SafeRelease(vs);SafeRelease(layout);SafeRelease(vb);SafeRelease(ib);SafeRelease(globals);SafeRelease(cameraCb);SafeRelease(skin);SafeRelease(armVB);SafeRelease(raster);
    SafeRelease(g_weaponPaletteCb);g_weaponPaletteCb=previousWeaponPalette116;
    g_dev=nullptr;g_ctx=nullptr;c={};c.active=true;fc3pad::Publish(c);g_reload=PhysicalReloadState{};g_manualReload=ManualReloadState{};
    std::puts("PASS: persistent machete measured native attachment, three live materials retained after source reuse, both-eye GPU pixels/rotation/wrist/finger grip; fresh draw/stow, one cold swipe then cached no-attack draw; ownership/calibration/reload/timeout/tracking recovery; firearm chamber/pump unchanged");
}
