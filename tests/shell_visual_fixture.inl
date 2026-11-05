void TestReloadShellVisual(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();ClearReloadShell();g_dev=device;g_ctx=context;g_frame=1600;
    const char* source="cbuffer B:register(b3){float4 b[144];};cbuffer E:register(b13){float4 e[4];};float4 main(uint id:SV_VertexID):SV_Position{float3 anchor=float3(-.0079,.20485,.07385);float2 q=float2(dot(b[0],float4(anchor,1)),dot(b[1],float4(anchor,1)));float2 p=id%3==0?float2(-.3,-.3):id%3==1?float2(0,.3):float2(.3,-.3);return float4(p+q+e[0].xy,.5,1);}";
    ID3DBlob* code=HullCompile(source,"vs_5_0");ID3D11VertexShader* vs=nullptr;
    Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs)),"reload shell test VS");
    D3D11_INPUT_ELEMENT_DESC element={"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};
    ID3D11InputLayout* layout=nullptr;Check(SUCCEEDED(device->CreateInputLayout(&element,1,code->GetBufferPointer(),code->GetBufferSize(),&layout)),"shell test layout");code->Release();
    unsigned long long hash=0xA38F7CD233BCE12Bull;vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);SetFlag(vs,kVsPatchedGuid,1);
    code=HullCompile("float4 main():SV_Target{return float4(0,1,0,1);}","ps_5_0");ID3D11PixelShader* ps=nullptr;
    Check(SUCCEEDED(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&ps)),"shell PS");code->Release();
    ID3D11Buffer* vb=Buffer(device,257896,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,1008,D3D11_BIND_INDEX_BUFFER);
    unsigned short indices[504];for(int i=0;i<504;++i)indices[i]=i%3;context->UpdateSubresource(ib,0,nullptr,indices,0,0);
    UINT stride=40,offset=336;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
    context->IASetInputLayout(layout);context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*cameraCb=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER),*skin=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    float data[88]={};data[0]=data[5]=data[10]=1;context->UpdateSubresource(globals,0,nullptr,data,0,0);globals->SetPrivateData(kCbCopyGuid,sizeof(data),data);
    float bones[576]={};for(int b=0;b<48;++b)bones[b*12]=bones[b*12+5]=bones[b*12+10]=1;
    context->UpdateSubresource(skin,0,nullptr,bones,0,0);skin->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);
    CameraSummary camera={};camera.mainView=1;camera.xScale=g_mainXScale*1.5f;camera.right[0]=camera.up[1]=camera.forward[2]=1;
    cameraCb->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);ID3D11Buffer* cb[4]={globals,cameraCb,nullptr,skin};context->VSSetConstantBuffers(0,4,cb);
    D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=64;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* colors[2]={},*depths[2]={};ID3D11RenderTargetView* rtv[2]={};ID3D11DepthStencilView* dsv[2]={};
    ID3D11Buffer* priorEye[2]={g_eyeCb[1][0],g_eyeCb[1][1]};
    for(int i=0;i<2;++i)
    {
        Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&colors[i])),"shell color");Check(SUCCEEDED(device->CreateRenderTargetView(colors[i],nullptr,&rtv[i])),"shell target");
        auto dd=td;dd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;dd.BindFlags=D3D11_BIND_DEPTH_STENCIL;Check(SUCCEEDED(device->CreateTexture2D(&dd,nullptr,&depths[i])),"shell depth");Check(SUCCEEDED(device->CreateDepthStencilView(depths[i],nullptr,&dsv[i])),"shell depth view");
        g_eyeCb[1][i]=Buffer(device,64,D3D11_BIND_CONSTANT_BUFFER);float eye[16]={};eye[0]=i?.08f:-.08f;context->UpdateSubresource(g_eyeCb[1][i],0,nullptr,eye,0,0);
        const float black[4]={};context->ClearRenderTargetView(rtv[i],black);context->ClearDepthStencilView(dsv[i],D3D11_CLEAR_DEPTH,1,0);
    }
    rtv[0]->SetPrivateDataInterface(kTwinGuid,rtv[1]);dsv[0]->SetPrivateDataInterface(kTwinGuid,dsv[1]);context->OMSetRenderTargets(1,&rtv[0],dsv[0]);
    D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;ID3D11RasterizerState* raster=nullptr;Check(SUCCEEDED(device->CreateRasterizerState(&rd,&raster)),"shell raster");context->RSSetState(raster);
    D3D11_VIEWPORT vp={0,0,64,64,0,1};context->RSSetViewports(1,&vp);
    CaptureReloadShell(context,18,108);CaptureReloadShell(context,126,360);Check(ReloadShellReady(),"both shell materials snapshot without any native reload input");
    float priorHand[16];memcpy(priorHand,g_handLeft,sizeof(priorHand));memset(g_handLeft,0,sizeof(priorHand));g_handLeft[0]=g_handLeft[5]=g_handLeft[10]=g_handLeft[15]=1;
    double offsets[2][3];memcpy(offsets,g_wristOffset,sizeof(offsets));memset(g_wristOffset,0,sizeof(offsets));
    g_reload.held=true;g_reload.mode=1;
    g_reloadContact.hand=true;g_reloadContact.handFrame=g_frame;g_reloadContact.view=HandIdentity();
    const V3 nativeCentre=Apply(kCapturedShellToHand,kCapturedShellCentre);
    g_reloadContact.view.m[3]=-nativeCentre.x;g_reloadContact.view.m[7]=-nativeCentre.y;
    float corrupt[576]={};context->UpdateSubresource(skin,0,nullptr,corrupt,0,0);context->UpdateSubresource(globals,0,nullptr,corrupt,0,0);
    g_reloadContact.handFrame=g_frame-1;ReplayReloadShell(context);Check(g_reloadShell.frame!=g_frame,"weapon-before-hand cannot replay a previous-frame shell pose");
    g_reloadContact.handFrame=g_frame;ReplayReloadShell(context);Check(g_reloadShell.frame==g_frame,"shell replay while held without native animation draws");
    float retained[576];Read(device,context,g_reloadShell.skin[0],retained);Check(Length(Sub(Apply(BoneMatrix(retained,0),{-.0079,.20485,.07385}),Apply(Multiply(g_reloadContact.view,kCapturedShellToHand),{-.0079,.20485,.07385})))<1e-6,"GPU palette preserves captured shell attachment");
    ID3D11Buffer* restored=nullptr;context->VSGetConstantBuffers(3,1,&restored);Check(restored==skin,"native bone binding restored after standalone replay");SafeRelease(restored);
    double centers[2]={};for(int i=0;i<2;++i)
    {
        auto sd=td;sd.BindFlags=0;sd.Usage=D3D11_USAGE_STAGING;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&sd,nullptr,&stage)),"shell pixel staging");context->CopyResource(stage,colors[i]);D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&m)),"shell pixels");int n=0;
        for(int y=0;y<64;++y)for(int x=0;x<64;++x)if(static_cast<BYTE*>(m.pData)[y*m.RowPitch+x*4+1]>200){++n;centers[i]+=x;}
        context->Unmap(stage,0);SafeRelease(stage);Check(n>100,"held shell actual pixels in both eye targets");centers[i]/=n;
    }
    Check(centers[1]-centers[0]>3,"held shell uses first-person stereo eye variant");
    g_reload.held=false;++g_frame;ReplayReloadShell(context);Check(g_reloadShell.frame!=g_frame,"released shell is not replayed");

    // Exercise the real weapon-palette upload and actual native draw pixels.
    // The animated duplicate must stay hidden after held/mode have cleared;
    // the independently retained physical shell must still render in both eyes.
    const bool priorManual=g_manualReloadEnabled,priorGunValid=g_frameGunValid;
    const Mat4 priorGunMove=g_frameGunMove;
    ID3D11Buffer* priorWeaponPalette=g_weaponPaletteCb;
    g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    g_manualReloadEnabled=true;g_frameGunValid=true;g_frameGunMove=HandIdentity();g_reload.mode=0;
    context->UpdateSubresource(globals,0,nullptr,data,0,0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    auto pixels=[&](int eye)
    {
        auto sd=td;sd.BindFlags=0;sd.Usage=D3D11_USAGE_STAGING;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        ID3D11Texture2D* stage=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&sd,nullptr,&stage)),"native-shell pixel staging");
        context->CopyResource(stage,colors[eye]);D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&m)),"native-shell pixel read");
        int count=0;for(int y=0;y<64;++y)for(int x=0;x<64;++x)if(static_cast<BYTE*>(m.pData)[y*m.RowPitch+x*4+1]>200)++count;
        context->Unmap(stage,0);SafeRelease(stage);return count;
    };
    auto drawNative=[&](UINT start,UINT count)
    {
        ID3D11Buffer* palette=BuildWeaponPalette(context,start,count);Check(palette!=nullptr,"native shell weapon palette upload");
        for(int eye=0;eye<2;++eye)
        {
            context->OMSetRenderTargets(1,&rtv[eye],dsv[eye]);
            context->VSSetConstantBuffers(3,1,&palette);context->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyeCb[1][eye]);
            context->DrawIndexed(count,start,0);
        }
        context->OMSetRenderTargets(1,&rtv[0],dsv[0]);context->VSSetConstantBuffers(3,1,&skin);
    };
    for(int phase=0;phase<3;++phase)
    {
        ++g_frame;float animated[576];memcpy(animated,bones,sizeof(animated));
        for(int b=0;b<48;++b){animated[b*12+3]=phase*.05f;animated[b*12+7]=phase*.025f;}
        context->UpdateSubresource(skin,0,nullptr,animated,0,0);skin->SetPrivateData(kCbCopyGuid,sizeof(animated),animated);
        const float black[4]={};for(int eye=0;eye<2;++eye){context->ClearRenderTargetView(rtv[eye],black);context->ClearDepthStencilView(dsv[eye],D3D11_CLEAR_DEPTH,1,0);}
        drawNative(18,108);drawNative(126,360);
        Check(pixels(0)==0&&pixels(1)==0,"both native animated shell materials remain invisible after contact and during closing animation");
        float unchanged[576];Read(device,context,skin,unchanged);Compare(unchanged,animated,576,"native shell original GPU skin is never changed");
        Read(device,context,g_reloadShell.skin[0],retained);
        Check(Length(Sub(Apply(BoneMatrix(retained,0),{-.0079,.20485,.07385}),Apply(Multiply(g_reloadContact.view,kCapturedShellToHand),{-.0079,.20485,.07385})))<1e-6,"native hiding cannot corrupt the retained physical-shell palette");
    }
    // The same retained shell is visible while original loading-shell draws
    // stay hidden. No new engine input/ammo event is involved in either draw.
    ++g_frame;g_reload.held=true;g_reload.mode=1;g_reloadContact.handFrame=g_frame;
    ReplayReloadShell(context);drawNative(18,108);drawNative(126,360);
    Check(pixels(0)>100&&pixels(1)>100,"physical shell remains visible in both eyes while native duplicates are hidden");
    g_reload.held=false;g_reload.mode=0;
    g_manualReloadEnabled=false;
    const float black[4]={};for(int eye=0;eye<2;++eye){context->ClearRenderTargetView(rtv[eye],black);context->ClearDepthStencilView(dsv[eye],D3D11_CLEAR_DEPTH,1,0);}
    drawNative(18,108);drawNative(126,360);
    Check(pixels(0)>100&&pixels(1)>100,"configured X reload fallback restores the original animated shell draw");
    Check(!ReloadShellDraw(context,16428,7311)&&!ReloadShellDraw(context,486,4071)&&!ReloadShellDraw(context,0,18),"receiver, pump and other indexed geometry are excluded from shell hiding");
    g_manualReloadEnabled=priorManual;g_frameGunValid=priorGunValid;g_frameGunMove=priorGunMove;
    SafeRelease(g_weaponPaletteCb);g_weaponPaletteCb=priorWeaponPalette;
    std::puts("PASS: native animated loading-shell GPU pixels hidden in both eyes after shell consumption; physical held-shell replay stays visible, source/cache unchanged, normal X fallback and receiver/pump exclusion");

    ClearReloadShell();Check(!ReloadShellReady()&&!g_reloadShell.deferred,"shell cache cleanup");context->ClearState();g_reloadContact=ReloadHandContact{};g_reload=PhysicalReloadState{};
    memcpy(g_handLeft,priorHand,sizeof(priorHand));memcpy(g_wristOffset,offsets,sizeof(offsets));
    for(int i=0;i<2;++i){SafeRelease(g_eyeCb[1][i]);g_eyeCb[1][i]=priorEye[i];SafeRelease(rtv[i]);SafeRelease(dsv[i]);SafeRelease(colors[i]);SafeRelease(depths[i]);}
    SafeRelease(raster);SafeRelease(vb);SafeRelease(ib);SafeRelease(globals);SafeRelease(cameraCb);SafeRelease(skin);SafeRelease(layout);SafeRelease(vs);SafeRelease(ps);g_dev=nullptr;g_ctx=nullptr;
    std::puts("PASS: independent held-shell capture, reused source buffers, palm geometry anchor, actual stereo pixels, native binding restoration, release and cache cleanup");
}
