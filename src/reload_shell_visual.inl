// Retain the loading shell independently of the game's reload animation.
struct ReloadShellVisual
{
    HullPart parts[2];
    ID3D11Buffer* skin[2]={};
    float bones[2][576]={};
    ID3D11DeviceContext* deferred=nullptr;
    LONG frame=-1;
    bool logged=false;
} g_reloadShell;
bool ReloadShellReady(){return g_reloadShell.parts[0].ready&&g_reloadShell.parts[1].ready;}
void ClearReloadShell()
{
    for(int i=0;i<2;++i)
    {
        auto& p=g_reloadShell.parts[i];
        SafeRelease(p.layout);SafeRelease(p.vs);SafeRelease(p.ps);SafeRelease(p.vb);SafeRelease(p.ib);
        SafeRelease(p.globals);SafeRelease(p.clip);SafeRelease(g_reloadShell.skin[i]);
        for(auto*& b:p.psCb)SafeRelease(b);
        for(auto*& t:p.textures)SafeRelease(t);
        for(auto*& s:p.samplers)SafeRelease(s);
        SafeRelease(p.raster);SafeRelease(p.depth);SafeRelease(p.blend);
    }
    SafeRelease(g_reloadShell.deferred);g_reloadShell=ReloadShellVisual{};
}
void CaptureReloadShell(ID3D11DeviceContext* context,UINT start,UINT count)
{
    if(!ReloadShellDraw(context,start,count))return;
    const int i=start==18?0:1;auto& p=g_reloadShell.parts[i];if(p.ready)return;
    ID3D11Buffer* globals=nullptr,*skin=nullptr;
    context->VSGetConstantBuffers(0,1,&globals);context->VSGetConstantBuffers(3,1,&skin);
    std::vector<float> data(kSmallCbMax/4);UINT bytes=ReadCopy(globals,data.data(),kSmallCbMax);
    bool ok=bytes>=80&&ReadCopy(skin,g_reloadShell.bones[i],2304)==2304;
    if(ok)
    {
        data.resize(bytes/4);DrawCall draw={};draw.kind=0;draw.a=count;draw.b=start;
        ok=SnapshotVisualPart(context,draw,p,data)&&HullClone(context,skin,D3D11_BIND_CONSTANT_BUFFER,&g_reloadShell.skin[i]);
        for(UINT slot=1;slot<8&&ok;++slot)
        {
            ID3D11Buffer* b=nullptr;context->PSGetConstantBuffers(slot,1,&b);
            ok=HullClone(context,b,D3D11_BIND_CONSTANT_BUFFER,&p.psCb[slot]);SafeRelease(b);
        }
        if(ok&&!g_reloadShell.deferred)ok=SUCCEEDED(g_dev->CreateDeferredContext(0,&g_reloadShell.deferred));
    }
    SafeRelease(globals);SafeRelease(skin);
    if(!ok){ClearReloadShell();Log("[PHYSICAL RELOAD] shell snapshot unavailable; pickup waits for valid visual");}
    else if(ReloadShellReady())Log("[PHYSICAL RELOAD] both loading shell materials retained independently of animation");
}
void ReplayReloadShell(ID3D11DeviceContext* context)
{
    if(!g_reload.held||!g_reload.mode||!ReloadShellReady()||!g_reloadShell.deferred||g_reloadShell.frame==g_frame||!g_reloadContact.hand||g_reloadContact.handFrame!=g_frame)return;
    ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);
    CameraSummary camera={};UINT cameraBytes=sizeof(camera);
    bool ok=cameraBuffer&&SUCCEEDED(cameraBuffer->GetPrivateData(kCameraSummaryGuid,&cameraBytes,&camera))&&
        cameraBytes==sizeof(camera)&&camera.mainView&&std::fabs(camera.xScale-g_mainXScale)>g_mainXScale*.01f;
    ID3D11RenderTargetView* targets[8]={},*twins[8]={};ID3D11DepthStencilView* depth=nullptr,*twinDepth=nullptr;
    context->OMGetRenderTargets(8,targets,&depth);
    const auto& first=g_reloadShell.parts[0];
    for(UINT i=0;i<8;++i)ok=ok&&HullFormat(targets[i])==first.formats[i];
    ok=ok&&depth&&HullFormat(depth)==first.depthFormat&&g_eyeCb[1][0]&&g_eyeCb[1][1];
    if(ok)
    {
        for(UINT i=0;i<first.targets;++i)if(targets[i])
        {twins[i]=static_cast<ID3D11RenderTargetView*>(TwinView(targets[i],ViewKind::RenderTarget,false));ok=ok&&twins[i];}
        twinDepth=static_cast<ID3D11DepthStencilView*>(TwinView(depth,ViewKind::DepthStencil,false));ok=ok&&twinDepth;
    }
    if(ok)
    {
        auto* dc=g_reloadShell.deferred;const bool own=g_own;g_own=true;
        D3D11_VIEWPORT viewports[16]={};UINT n=16;context->RSGetViewports(&n,viewports);dc->RSSetViewports(n,viewports);
        D3D11_RECT rects[16]={};n=16;context->RSGetScissorRects(&n,rects);dc->RSSetScissorRects(n,rects);
        dc->GSSetShader(nullptr,nullptr,0);dc->HSSetShader(nullptr,nullptr,0);dc->DSSetShader(nullptr,nullptr,0);
        for(int i=0;i<2;++i)
        {
            auto& p=g_reloadShell.parts[i];const Mat4 world=HullWorld(p.data.data());
            Mat4 fromView={};if(!Invert(Multiply(HullView(camera),world),fromView))continue;
            float bones[576];memcpy(bones,g_reloadShell.bones[i],sizeof(bones));MoveReloadShell(bones,fromView);
            g_oUpdateSubresource(context,g_reloadShell.skin[i],0,nullptr,bones,0,0);
            UINT stride=40,offset=336;dc->IASetVertexBuffers(0,1,&p.vb,&stride,&offset);
            dc->IASetIndexBuffer(p.ib,DXGI_FORMAT_R16_UINT,p.indexOffset);dc->IASetInputLayout(p.layout);
            dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            dc->VSSetShader(p.vs,nullptr,0);dc->PSSetShader(p.ps,nullptr,0);
            ID3D11Buffer* cb[4]={p.globals,cameraBuffer,p.clip,g_reloadShell.skin[i]};dc->VSSetConstantBuffers(0,4,cb);
            dc->PSSetConstantBuffers(0,8,p.psCb);dc->PSSetShaderResources(0,16,p.textures);dc->PSSetSamplers(0,16,p.samplers);
            dc->RSSetState(p.raster);dc->OMSetDepthStencilState(p.depth,p.stencil);dc->OMSetBlendState(p.blend,p.blendFactor,p.mask);
            for(int eye=0;eye<2;++eye)
            {
                dc->OMSetRenderTargets(p.targets,eye?twins:targets,eye?twinDepth:depth);
                dc->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyeCb[1][eye]);
                if(HasFlag(p.ps,kPsPatchedGuid))dc->PSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyePsCb[eye]);
                dc->DrawIndexed(p.draw.a,p.draw.b,p.draw.d);
            }
        }
        ID3D11CommandList* list=nullptr;if(SUCCEEDED(dc->FinishCommandList(FALSE,&list)))
        {context->ExecuteCommandList(list,TRUE);SafeRelease(list);g_reloadShell.frame=g_frame;
            if(!g_reloadShell.logged){Log("[PHYSICAL RELOAD] held shell replay submitted to both first-person eye targets; captured native hand attachment");g_reloadShell.logged=true;}}
        g_own=own;
    }
    for(UINT i=0;i<8;++i){SafeRelease(targets[i]);SafeRelease(twins[i]);}
    SafeRelease(depth);SafeRelease(twinDepth);SafeRelease(cameraBuffer);
}
