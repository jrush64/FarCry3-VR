ID3DBlob* HullCompile(const char* source, const char* target)
{
    ID3DBlob* blob=nullptr,*errors=nullptr;
    HRESULT hr=D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"main",target,0,0,&blob,&errors);
    if(errors){std::fprintf(stderr,"%s",static_cast<const char*>(errors->GetBufferPointer()));errors->Release();}
    Check(SUCCEEDED(hr),"hull fixture shader compilation");return blob;
}
void TestPhysicalHull(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();ClearPhysicalHull();g_dev=device;g_ctx=context;
    g_enabled=g_motionGun=g_pumpEnabled=g_inGameplay=true;
    g_mainXScale=1.3f;g_frame=1000;g_lastShotFrame=1000;g_pumpStage=0;g_chambered=false;
    TickPhysicalHull(true);Check(g_hull.active&&!g_hull.playing,"shot arms ejection without playing it");
    CameraSummary camera={};camera.mainView=1;camera.xScale=1.3f;camera.right[0]=camera.up[1]=camera.forward[2]=1;
    camera.position[0]=2;camera.position[1]=3;camera.position[2]=4;
    g_frameCamera=camera;g_frameCameraValid=true;g_frameGunValid=true;
    g_frameGunMove={};g_frameGunMove.m[0]=g_frameGunMove.m[5]=g_frameGunMove.m[10]=g_frameGunMove.m[15]=1;
    g_frameGunMove.m[3]=.2;
    float data[88]={};data[0]=data[5]=data[10]=1;data[3]=2;data[7]=3;data[11]=4;data[12]=.05f;
    data[17]=.001f;data[19]=.1f;
    const Mat4 folded=HullWorld(data);Check(std::fabs(folded.m[3]-2.05)<1e-6,"world fraction folded into shell transform");
    const Mat4 view=HullView(camera),local=Multiply(view,folded);
    Check(std::fabs(local.m[3]-.05)<1e-6&&std::fabs(local.m[7])<1e-6,"source casing mapped to camera space");
    auto* shader=HullCompile("cbuffer G:register(b0){float4 world[22];};cbuffer Eye:register(b13){float4 eye[4];};float4 main(uint id:SV_VertexID):SV_Position{float2 p=id%3==0?float2(-.5,-.5):id%3==1?float2(0,.5):float2(.5,-.5);return float4(p+float2(world[0].w-2,world[1].w-3)+eye[0].xy,.5,1);}","vs_5_0");
    ID3D11VertexShader* vs=nullptr;Check(SUCCEEDED(device->CreateVertexShader(shader->GetBufferPointer(),shader->GetBufferSize(),nullptr,&vs)),"hull VS");
    D3D11_INPUT_ELEMENT_DESC element={"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};
    ID3D11InputLayout* layout=nullptr;Check(SUCCEEDED(device->CreateInputLayout(&element,1,shader->GetBufferPointer(),shader->GetBufferSize(),&layout)),"hull layout");shader->Release();
    unsigned long long hash=0xC386478F2F9495EBull;vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);SetFlag(vs,kVsPatchedGuid,1);
    ID3D11PixelShader* ps[2]={};
    for(int i=0;i<2;++i)
    {
        auto* code=HullCompile(i?"float4 main():SV_Target{return float4(0,1,0,1);}":"float4 main():SV_Target{return float4(1,0,0,1);}","ps_5_0");
        Check(SUCCEEDED(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&ps[i])),"hull PS");code->Release();
        hash=i?0x0334CEB991DCA831ull:0x23E84ABA164A3A78ull;ps[i]->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);
    }
    ID3D11Buffer* vertex=Buffer(device,4192,D3D11_BIND_VERTEX_BUFFER),*indices=Buffer(device,1008,D3D11_BIND_INDEX_BUFFER);
    unsigned short indexData[504];for(int i=0;i<504;++i)indexData[i]=i%3;
    context->UpdateSubresource(indices,0,nullptr,indexData,0,0);
    UINT stride=32,offset=0;context->IASetVertexBuffers(0,1,&vertex,&stride,&offset);context->IASetIndexBuffer(indices,DXGI_FORMAT_R16_UINT,0);
    context->IASetInputLayout(layout);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*viewport=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
    context->UpdateSubresource(globals,0,nullptr,data,0,0);globals->SetPrivateData(kCbCopyGuid,sizeof(data),data);
    viewport->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);ID3D11Buffer* cbs[2]={globals,viewport};context->VSSetConstantBuffers(0,2,cbs);
    context->VSSetShader(vs,nullptr,0);
    D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=64;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* color[2]={},*z[2]={};ID3D11RenderTargetView* rtv[2]={};ID3D11DepthStencilView* dsv[2]={};
    for(int i=0;i<2;++i)
    {
        Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&color[i])),"hull render texture");Check(SUCCEEDED(device->CreateRenderTargetView(color[i],nullptr,&rtv[i])),"hull RTV");
        auto dd=td;dd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;dd.BindFlags=D3D11_BIND_DEPTH_STENCIL;
        Check(SUCCEEDED(device->CreateTexture2D(&dd,nullptr,&z[i])),"hull depth texture");Check(SUCCEEDED(device->CreateDepthStencilView(z[i],nullptr,&dsv[i])),"hull DSV");
    }
    rtv[0]->SetPrivateDataInterface(kTwinGuid,rtv[1]);dsv[0]->SetPrivateDataInterface(kTwinGuid,dsv[1]);
    context->OMSetRenderTargets(1,&rtv[0],dsv[0]);
    D3D11_DEPTH_STENCIL_DESC depthDesc={};depthDesc.DepthEnable=TRUE;depthDesc.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;depthDesc.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
    ID3D11DepthStencilState* depthState=nullptr;Check(SUCCEEDED(device->CreateDepthStencilState(&depthDesc,&depthState)),"hull depth state");context->OMSetDepthStencilState(depthState,0);
    D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;
    ID3D11RasterizerState* raster=nullptr;Check(SUCCEEDED(device->CreateRasterizerState(&rd,&raster)),"hull raster");context->RSSetState(raster);
    D3D11_VIEWPORT vp={0,0,64,64,0,1};context->RSSetViewports(1,&vp);
    for(int i=0;i<2;++i)
    {
        context->PSSetShader(ps[i],nullptr,0);DrawCall call={};call.kind=0;call.a=i?144:360;call.b=i?360:0;
        Check(PhysicalHullDraw(context,call),"native shell suppressed while rear stroke is pending");
    }
    Check(g_hull.part[0].ready&&g_hull.part[1].ready&&!g_hull.playing,"both material snapshots retained before pump");
    // GPU source can be reused without changing retained geometry/material globals.
    float corrupted[88]={};context->UpdateSubresource(globals,0,nullptr,corrupted,0,0);
    float retained[88];Read(device,context,g_hull.part[0].globals,retained);Compare(retained,data,88,"retained globals independent of overwritten source");
    // A long hold must not expire just because the native shell has disappeared.
    g_hull.shotTime=GetTickCount64()-5000;g_hull.rear=true;StartPhysicalHull();
    Check(g_hull.playing,"rear stroke releases retained hull after five second hold");
    Mat4 launch=Multiply(g_hull.releaseMove,g_hull.poses[0].world);
    Check(std::fabs(launch.m[3]-2.25)<1e-5&&std::fabs(launch.m[7]-3)<1e-5,"launch follows motion gun and camera with fraction applied once");
    ID3D11Buffer* previousEye[2]={g_eyeCb[0][0],g_eyeCb[0][1]};
    for(int i=0;i<2;++i)
    {
        g_eyeCb[0][i]=Buffer(device,64,D3D11_BIND_CONSTANT_BUFFER);float eye[16]={};eye[0]=i?.08f:-.08f;context->UpdateSubresource(g_eyeCb[0][i],0,nullptr,eye,0,0);
        const float black[4]={};context->ClearRenderTargetView(rtv[i],black);context->ClearDepthStencilView(dsv[i],D3D11_CLEAR_DEPTH,1,0);
    }
    Check(ReplayPhysicalHull(context),"deferred hull replay submitted to both eyes");
    Check(!ReplayPhysicalHull(context),"casing replay only once per frame");
    ID3D11Buffer* restored=nullptr;context->VSGetConstantBuffers(0,1,&restored);Check(restored==globals,"immediate source CB binding restored");restored->Release();
    ID3D11RenderTargetView* restoredRtv=nullptr;context->OMGetRenderTargets(1,&restoredRtv,nullptr);Check(restoredRtv==rtv[0],"immediate render target restored");restoredRtv->Release();
    ID3D11PixelShader* restoredPs=nullptr;context->PSGetShader(&restoredPs,nullptr,nullptr);Check(restoredPs==ps[1],"immediate shader restored");restoredPs->Release();
    // Read actual rendered pixels: both eye targets contain the shell, at distinct offsets.
    double centers[2]={};
    for(int eye=0;eye<2;++eye)
    {
        auto stageDesc=td;stageDesc.Usage=D3D11_USAGE_STAGING;stageDesc.BindFlags=0;stageDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        ID3D11Texture2D* stage=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&stageDesc,nullptr,&stage)),"pixel readback texture");context->CopyResource(stage,color[eye]);
        D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&mapped)),"pixel map");int count=0;
        for(UINT y=0;y<64;++y)for(UINT x=0;x<64;++x){auto* pixel=static_cast<unsigned char*>(mapped.pData)+y*mapped.RowPitch+x*4;if(pixel[1]>200){++count;centers[eye]+=x;}}
        context->Unmap(stage,0);stage->Release();Check(count>100,"spent hull pixels in each eye");centers[eye]/=count;
    }
    Check(centers[1]-centers[0]>3,"per-eye CB affects actual replay pixels");
    g_hull.released=GetTickCount64()-1100;++g_frame;Check(!ReplayPhysicalHull(context),"visual expires without duplicate replay");
    g_pumpEnabled=false;TickPhysicalHull(true);Check(!g_hull.active&&!g_hull.deferred,"pump toggle clears retained shell and permits native path");g_pumpEnabled=true;
    ClearPhysicalHull();context->ClearState();
    for(int i=0;i<2;++i){SafeRelease(g_eyeCb[0][i]);g_eyeCb[0][i]=previousEye[i];rtv[i]->Release();dsv[i]->Release();color[i]->Release();z[i]->Release();ps[i]->Release();}
    depthState->Release();raster->Release();globals->Release();viewport->Release();vertex->Release();indices->Release();layout->Release();vs->Release();g_dev=nullptr;g_ctx=nullptr;
    std::puts("PASS: physical hull deferred release, source reuse, five-second hold, world fractions, actual stereo pixels, state restoration, one replay per frame and toggle cleanup");
}
