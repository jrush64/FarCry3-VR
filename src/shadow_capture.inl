// Called immediately AFTER the real draw, while that eye's resources remain bound.
int g_shadowCaptureDraws = 0;
LONG g_shadowCaptureFrame = -1;
void CaptureShadowDraw(ID3D11DeviceContext* context, int eye)
{
    if (g_census == INVALID_HANDLE_VALUE) return;
    ID3D11PixelShader* ps = nullptr; context->PSGetShader(&ps,nullptr,nullptr);
    const auto hash = ShaderHash(ps); SafeRelease(ps);
    if (hash != 0xA4222A0CCB4CE14FULL) return;
    if (g_shadowCaptureFrame != g_frame) { g_shadowCaptureFrame=g_frame; g_shadowCaptureDraws=0; }
    if (g_shadowCaptureDraws >= 8) return;
    const int draw = g_shadowCaptureDraws++;
    ID3D11RenderTargetView* rt = nullptr; context->OMGetRenderTargets(1,&rt,nullptr);
    ID3D11ShaderResourceView* depth = nullptr; context->PSGetShaderResources(1,1,&depth);
    ID3D11ShaderResourceView* mask = nullptr;
    if (rt) {
        D3D11_RENDER_TARGET_VIEW_DESC rd = {}; rt->GetDesc(&rd);
        ID3D11Resource* res = nullptr; rt->GetResource(&res);
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {}; sd.Format=rd.Format;
        sd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DMS;
        if (rd.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
            g_dev->CreateShaderResourceView(res,&sd,&mask);
        SafeRelease(res);
    }
    wchar_t name[160];
    // Actual PS constants, including the eye transform bound at cb13.
    const UINT slots[] = {0,1,2,13};
    for (UINT slot:slots) {
        ID3D11Buffer* cb=nullptr; context->PSGetConstantBuffers(slot,1,&cb);
        std::vector<unsigned char> bytes;
        if (cb && ReadBytes(context,cb,bytes)) {
            _snwprintf_s(name,160,_TRUNCATE,L"fc3_shadow_f%ld_d%d_e%d_cb%u.bin",g_frame,draw,eye,slot);
            WriteBlob(name,bytes);
        }
        SafeRelease(cb);
    }
    ID3D11ShaderResourceView* views[] = {mask,depth};
    for (int kind=0;kind<2;++kind) {
        std::vector<float> pixels; UINT w=0,h=0;
        const HRESULT hr=fc3shadow::ReadSample0(context,views[kind],pixels,w,h);
        if (FAILED(hr)) { Log("[SHADOWCAP] frame %ld draw %d eye %d %s FAILED %08X",g_frame,draw,eye,kind?"depth":"mask",hr); continue; }
        // Header: magic SHC1, width, height, frame, draw, eye, sample=0, format=R32_FLOAT.
        const UINT header[8]={0x31434853u,w,h,static_cast<UINT>(g_frame),static_cast<UINT>(draw),static_cast<UINT>(eye),0,41};
        std::vector<unsigned char> bytes(sizeof(header)+pixels.size()*sizeof(float));
        std::memcpy(bytes.data(),header,sizeof(header));
        std::memcpy(bytes.data()+sizeof(header),pixels.data(),pixels.size()*sizeof(float));
        _snwprintf_s(name,160,_TRUNCATE,L"fc3_shadow_f%ld_d%d_e%d_%s.bin",g_frame,draw,eye,kind?L"depth":L"mask");
        WriteBlob(name,bytes);
        Log("[SHADOWCAP] frame %ld draw %d eye %d %s %ux%u sample0",g_frame,draw,eye,kind?"depth":"mask",w,h);
    }
    SafeRelease(mask); SafeRelease(depth); SafeRelease(rt);
}
