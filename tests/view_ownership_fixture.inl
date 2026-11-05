void STDMETHODCALLTYPE ViewDraw(ID3D11DeviceContext* c,UINT n,UINT start){c->Draw(n,start);}
void TestViewOwnership(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();g_dev=device;g_ctx=context;g_enabled=true;g_bbW=g_bbH=128;
    g_mainXScale=1.3f;g_frameCameraValid=false;g_motionGun=false;g_weaponProjectionValid=false;
    g_aimActive=g_bodyValid=g_inGameplay=true;g_handRightValid=g_handLeftValid=false;
    for(int i=0;i<3;++i)for(int j=0;j<3;++j)g_bodyAxes[i][j]=i==j?1:0;
    fc3stereo::EyeView eyes[2]={};
    for(int eye=0;eye<2;++eye){auto& e=eyes[eye];e.view[0]=e.view[5]=e.view[10]=e.view[15]=1;e.view[3]=eye?-.032f:.032f;e.tanLeft=e.tanDown=-1;e.tanRight=e.tanUp=1;}
    fc3stereo::SetEyeViews(eyes);
    for(int v=0;v<kEyeVariants;++v)for(int e=0;e<2;++e)g_eyeCb[v][e]=Buffer(device,dxbc::kEyeRows*16,D3D11_BIND_CONSTANT_BUFFER);
    for(int e=0;e<2;++e)g_eyePsCb[e]=Buffer(device,64,D3D11_BIND_CONSTANT_BUFFER);
    ID3DBlob* code=HullCompile("cbuffer C:register(b1){float4 c[56];};cbuffer E:register(b13){float4 e[16];};float4 main(uint id:SV_VertexID):SV_Position{float2 q=id==0?float2(-.15,-.12):id==1?float2(0,.15):float2(.15,-.12);float4 p=float4(q,3,1);float4 clip=float4(dot(c[0],p),dot(c[1],p),dot(c[2],p),dot(c[3],p));return clip+float4(dot(e[0],clip),dot(e[1],clip),dot(e[2],clip),dot(e[3],clip));}","vs_5_0");
    ID3D11VertexShader* vs=nullptr;Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs)),"view VS");code->Release();SetFlag(vs,kVsPatchedGuid,1);
    code=HullCompile("float4 main():SV_Target{return float4(1,1,1,1);}","ps_5_0");ID3D11PixelShader* ps=nullptr;device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&ps);code->Release();
    context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11Buffer* camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);context->VSSetConstantBuffers(1,1,&camera);
    D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=128;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* images[2]={};ID3D11RenderTargetView* targets[2]={};for(int e=0;e<2;++e){device->CreateTexture2D(&td,nullptr,&images[e]);device->CreateRenderTargetView(images[e],nullptr,&targets[e]);}
    targets[0]->SetPrivateDataInterface(kTwinGuid,targets[1]);context->OMSetRenderTargets(1,&targets[0],nullptr);
    D3D11_VIEWPORT vp={0,0,128,128,0,1};context->RSSetViewports(1,&vp);D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;ID3D11RasterizerState* rs=nullptr;device->CreateRasterizerState(&rd,&rs);context->RSSetState(rs);
    const auto oldDraw=g_oDraw;g_oDraw=ViewDraw;double baseline[2]={};
    for(int run=0;run<6;++run)
    {
        const double angle=run==0?0:run%2?.35:-.35;const float xs=run<2?1.3f:run<4?.37f:2.3f;
        float raw[224]={};const float co=static_cast<float>(cos(angle)),si=static_cast<float>(sin(angle));
        raw[0]=xs*co;raw[2]=-xs*si;raw[5]=xs;raw[8]=si;raw[10]=co;raw[11]=-.1f;raw[12]=si;raw[14]=co;
        ObserveCamera(camera,raw);context->UpdateSubresource(camera,0,nullptr,raw,0,0);
        // Do not reset frameCameraValid: changes later in the same frame count.
        for(int e=0;e<2;++e){const float clear[4]={};context->ClearRenderTargetView(targets[e],clear);}
        DrawCall draw={};draw.kind=1;draw.a=3;HandleDraw(context,draw);
        for(int e=0;e<2;++e){auto stageDesc=td;stageDesc.BindFlags=0;stageDesc.Usage=D3D11_USAGE_STAGING;stageDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;device->CreateTexture2D(&stageDesc,nullptr,&stage);context->CopyResource(stage,images[e]);D3D11_MAPPED_SUBRESOURCE m={};context->Map(stage,0,D3D11_MAP_READ,0,&m);double sum=0;int count=0;for(int y=0;y<128;++y){auto* row=static_cast<unsigned char*>(m.pData)+y*m.RowPitch;for(int x=0;x<128;++x)if(row[x*4]){sum+=x;++count;}}context->Unmap(stage,0);stage->Release();Check(count>8,"world triangle remains visible in both eyes across door/aim changes");const double centre=sum/count;if(run==0)baseline[e]=centre;else Check(fabs(centre-baseline[e])<.3,"actual drawn pixels stay at head direction as gun camera/FOV changes");}
    }
    Check(baseline[0]>baseline[1],"world retains stereo disparity");
    g_bodyYaw=.73;g_motionGun=true;g_weaponMotionGuard=true;g_visibleWeaponDrawsLastFrame=0;AimFrame(true);
    Check(g_bodyValid&&g_aimActive&&fabs(g_bodyYaw-.73)<1e-10,"controller/weapon loss cannot discard world compensation or anchor to gun");
    // Projection depth must not include the view's translation.
    float raw[224]={};raw[0]=raw[5]=raw[10]=raw[14]=1;raw[15]=5;raw[11]=4.9f;ObserveCamera(camera,raw);CameraSummary sample={};UINT bytes=sizeof(sample);camera->GetPrivateData(kCameraSummaryGuid,&bytes,&sample);Check(fabs(sample.zOffset+.1f)<1e-5,"camera translation excluded from projection depth");
    fc3stereo::SetEyeViews(nullptr);AimFrame(true);Check(!g_bodyValid&&!g_aimActive,"lost headset reference clears matching body reference");
    g_oDraw=oldDraw;context->ClearState();for(int v=0;v<kEyeVariants;++v)for(int e=0;e<2;++e)SafeRelease(g_eyeCb[v][e]);for(int e=0;e<2;++e){SafeRelease(g_eyePsCb[e]);targets[e]->Release();images[e]->Release();}camera->Release();vs->Release();ps->Release();rs->Release();g_dev=nullptr;g_ctx=nullptr;g_bodyValid=g_aimActive=false;
    puts("PASS: actual HandleDraw WARP pixels retain head direction/stereo across native aim and doorway FOV; dropout preserves body reference; depth excludes camera translation");
}
