static int ShadowReadbackTest()
{
    ID3D11Device* dev=nullptr; ID3D11DeviceContext* ctx=nullptr;
    if (FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,nullptr,&ctx))) return Fail("shadow WARP");
    int result=0;
    // Real game's shadow and depth formats: typeless storage + typed MSAA views.
    for (int kind=0;kind<2;++kind) {
        D3D11_TEXTURE2D_DESC td={19,13,1,1,kind?DXGI_FORMAT_R32_TYPELESS:DXGI_FORMAT_R8_TYPELESS,{4,0},D3D11_USAGE_DEFAULT,
            static_cast<UINT>(D3D11_BIND_SHADER_RESOURCE | (kind?D3D11_BIND_DEPTH_STENCIL:D3D11_BIND_RENDER_TARGET)),0,0};
        ID3D11Texture2D* tex=nullptr;
        if (FAILED(dev->CreateTexture2D(&td,nullptr,&tex))) return Fail("shadow texture");
        D3D11_SHADER_RESOURCE_VIEW_DESC sd={}; sd.Format=kind?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R8_UNORM;
        sd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DMS;
        ID3D11ShaderResourceView* srv=nullptr; if(FAILED(dev->CreateShaderResourceView(tex,&sd,&srv)))return Fail("shadow SRV");
        ID3D11RenderTargetView* rt=nullptr; ID3D11DepthStencilView* ds=nullptr;
        if(kind) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dd={};dd.Format=DXGI_FORMAT_D32_FLOAT;dd.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2DMS;
            if(FAILED(dev->CreateDepthStencilView(tex,&dd,&ds)))return Fail("shadow DSV");
            ctx->ClearDepthStencilView(ds,D3D11_CLEAR_DEPTH,0.375f,0);ctx->OMSetRenderTargets(0,nullptr,ds);
        } else {
            D3D11_RENDER_TARGET_VIEW_DESC rd={};rd.Format=DXGI_FORMAT_R8_UNORM;rd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2DMS;
            if(FAILED(dev->CreateRenderTargetView(tex,&rd,&rt)))return Fail("shadow RTV");
            float color[4]={0.6f,0,0,0};ctx->ClearRenderTargetView(rt,color);ctx->OMSetRenderTargets(1,&rt,nullptr);
        }
        std::vector<float> pixels; UINT w=0,h=0;
        if(FAILED(fc3shadow::ReadSample0(ctx,srv,pixels,w,h))||w!=19||h!=13)return Fail("shadow readback");
        for(float f:pixels)if(std::fabs(f-(kind?0.375f:0.6f))>0.00001f)result=Fail("shadow sample values");
        ID3D11RenderTargetView* checkRt=nullptr;ID3D11DepthStencilView* checkDs=nullptr;
        ctx->OMGetRenderTargets(1,&checkRt,&checkDs);
        if(checkRt!=rt||checkDs!=ds)result=Fail("shadow readback changed game targets");
        fc3shadow::Release(checkRt);fc3shadow::Release(checkDs);
        ID3D11ComputeShader* cs=nullptr;ctx->CSGetShader(&cs,nullptr,nullptr);
        ID3D11ShaderResourceView* csSrv=nullptr;ctx->CSGetShaderResources(0,1,&csSrv);
        ID3D11UnorderedAccessView* csUav=nullptr;ctx->CSGetUnorderedAccessViews(0,1,&csUav);
        if(cs||csSrv||csUav)result=Fail("shadow readback leaked CS bindings");
        fc3shadow::Release(cs);fc3shadow::Release(csSrv);fc3shadow::Release(csUav);
        ctx->OMSetRenderTargets(0,nullptr,nullptr);
        fc3shadow::Release(rt);fc3shadow::Release(ds);fc3shadow::Release(srv);fc3shadow::Release(tex);
    }
    fc3shadow::Release(ctx);fc3shadow::Release(dev);
    if(!result)std::puts("PASS: shared shadow readback, R8 and R32 typeless 4xMSAA sample0, odd dimensions, OM/CS restoration");
    return result;
}
