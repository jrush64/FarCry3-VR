// Cosmetic retiming of the identified rigid shotgun hull. The engine's ammo,
// animation and casing physics are untouched; only this one visual is replayed.
struct HullPart
{
    DrawCall draw = {};
    ID3D11InputLayout* layout = nullptr;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11Buffer* vb = nullptr;
    ID3D11Buffer* ib = nullptr;
    ID3D11Buffer* globals = nullptr;
    ID3D11Buffer* clip = nullptr;
    ID3D11Buffer* psCb[8] = {};
    ID3D11ShaderResourceView* textures[16] = {};
    ID3D11SamplerState* samplers[16] = {};
    ID3D11RasterizerState* raster = nullptr;
    ID3D11DepthStencilState* depth = nullptr;
    ID3D11BlendState* blend = nullptr;
    UINT stencil = 0, mask = 0, indexOffset = 0;
    float blendFactor[4] = {};
    DXGI_FORMAT formats[8] = {}, depthFormat = DXGI_FORMAT_UNKNOWN;
    UINT targets = 0;
    std::vector<float> data;
    bool ready = false;
};
struct HullPose { ULONGLONG time; Mat4 world; };
struct PhysicalHull
{
    HullPart part[2];
    ID3D11DeviceContext* deferred = nullptr;
    std::vector<HullPose> poses;
    Mat4 sourceToView = {}, releaseMove = {}, lastWorld = {};
    LONG shot = -100000, sampledFrame = -1, replayFrame = -1;
    ULONGLONG shotTime = 0, firstTime = 0, released = 0;
    bool active = false, rear = false, playing = false, failed = false, replayLogged = false, ended = false;
} g_hull;

void ClearPhysicalHull()
{
    for (auto& p : g_hull.part)
    {
        SafeRelease(p.layout); SafeRelease(p.vs); SafeRelease(p.ps);
        SafeRelease(p.vb); SafeRelease(p.ib); SafeRelease(p.globals); SafeRelease(p.clip);
        for (auto*& b : p.psCb) SafeRelease(b);
        for (auto*& t : p.textures) SafeRelease(t);
        for (auto*& s : p.samplers) SafeRelease(s);
        SafeRelease(p.raster); SafeRelease(p.depth); SafeRelease(p.blend);
        p = HullPart{};
    }
    SafeRelease(g_hull.deferred);
    g_hull = PhysicalHull{};
}
Mat4 HullView(const CameraSummary& camera)
{
    Mat4 m = {};
    for (int r = 0; r < 3; ++r)
    {
        const float* axis = r == 0 ? camera.right : r == 1 ? camera.up : camera.forward;
        for (int c = 0; c < 3; ++c) m.m[r*4+c] = axis[c];
        m.m[r*4+3] = -axis[0]*camera.position[0]-axis[1]*camera.position[1]-axis[2]*camera.position[2];
    }
    m.m[15] = 1;
    return m;
}
Mat4 HullWorld(const float* data)
{
    Mat4 m = {}; m.m[15] = 1;
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 4; ++c) m.m[r*4+c] = data[r*4+c];
        m.m[r*4+3] += data[12+r]; // fold world-position fractions into translation
    }
    return m;
}
bool HullCamera(ID3D11Buffer* b, CameraSummary& camera)
{
    UINT size = sizeof(camera);
    return b && SUCCEEDED(b->GetPrivateData(kCameraSummaryGuid, &size, &camera)) &&
        size == sizeof(camera) && camera.mainView &&
        std::fabs(camera.xScale-g_mainXScale) < g_mainXScale*.01f;
}
DXGI_FORMAT HullFormat(ID3D11View* view)
{
    if (!view) return DXGI_FORMAT_UNKNOWN;
    ID3D11Resource* resource = nullptr; view->GetResource(&resource);
    ID3D11Texture2D* texture = nullptr;
    if (resource) resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture));
    D3D11_TEXTURE2D_DESC desc = {}; if (texture) texture->GetDesc(&desc);
    SafeRelease(texture); SafeRelease(resource); return desc.Format;
}
bool HullClone(ID3D11DeviceContext* context, ID3D11Buffer* source, UINT bind, ID3D11Buffer** target)
{
    if (!source) return true;
    D3D11_BUFFER_DESC desc = {}; source->GetDesc(&desc);
    if (!desc.ByteWidth || desc.ByteWidth > 1048576) return false;
    std::vector<unsigned char> bytes(desc.ByteWidth);
    if (ReadCopy(source, bytes.data(), desc.ByteWidth) != desc.ByteWidth)
    {
        // Keep GPU readbacks bounded while retaining the shared 258 KB mesh VB.
        for(UINT offset=0;offset<desc.ByteWidth;offset+=65536)
        {
            const UINT count=(std::min)(65536u,desc.ByteWidth-offset);
            std::vector<unsigned char> chunk;
            if(!EjectionReadRange(context,source,offset,count,chunk))return false;
            memcpy(bytes.data()+offset,chunk.data(),count);
        }
    }
    desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = bind;
    desc.CPUAccessFlags = desc.MiscFlags = desc.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA initial = {}; initial.pSysMem = bytes.data();
    return SUCCEEDED(g_dev->CreateBuffer(&desc, &initial, target));
}
bool SnapshotVisualPart(ID3D11DeviceContext* context, const DrawCall& call, HullPart& p,
                     const std::vector<float>& globals)
{
    if (p.ready) return true;
    p.draw = call; p.data = globals;
    context->IAGetInputLayout(&p.layout);
    context->VSGetShader(&p.vs, nullptr, nullptr); context->PSGetShader(&p.ps, nullptr, nullptr);
    ID3D11Buffer* vb = nullptr; UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);
    ID3D11Buffer* ib = nullptr; DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    context->IAGetIndexBuffer(&ib,&format,&p.indexOffset);
    bool ok = format == DXGI_FORMAT_R16_UINT && HullClone(context,vb,D3D11_BIND_VERTEX_BUFFER,&p.vb) &&
        HullClone(context,ib,D3D11_BIND_INDEX_BUFFER,&p.ib);
    SafeRelease(vb); SafeRelease(ib);
    ID3D11Buffer* cb[3] = {}; context->VSGetConstantBuffers(0,3,cb);
    ok = ok && HullClone(context,cb[0],D3D11_BIND_CONSTANT_BUFFER,&p.globals) &&
        HullClone(context,cb[2],D3D11_BIND_CONSTANT_BUFFER,&p.clip);
    for (auto*& b : cb) SafeRelease(b);
    // Both identified original pixel shaders declare only Metal at CB0.
    // Do not snapshot unrelated stale bindings from the previous material.
    ID3D11Buffer* pixel = nullptr; context->PSGetConstantBuffers(0,1,&pixel);
    ok = HullClone(context,pixel,D3D11_BIND_CONSTANT_BUFFER,&p.psCb[0]) && ok;
    SafeRelease(pixel);
    context->PSGetShaderResources(0,16,p.textures); context->PSGetSamplers(0,16,p.samplers);
    context->RSGetState(&p.raster); context->OMGetDepthStencilState(&p.depth,&p.stencil);
    context->OMGetBlendState(&p.blend,p.blendFactor,&p.mask);
    ID3D11RenderTargetView* targets[8] = {}; ID3D11DepthStencilView* depth = nullptr;
    context->OMGetRenderTargets(8,targets,&depth);
    for (UINT i = 0; i < 8; ++i)
    { p.formats[i] = HullFormat(targets[i]); if (targets[i]) p.targets = i+1; SafeRelease(targets[i]); }
    p.depthFormat = HullFormat(depth); SafeRelease(depth);
    p.ready = ok && p.layout && p.vs && p.ps && p.vb && p.ib && p.globals;
    return p.ready;
}
bool CaptureHullPart(ID3D11DeviceContext* context, const DrawCall& call, int side,
                     const std::vector<float>& globals)
{ return SnapshotVisualPart(context,call,g_hull.part[side],globals); }
void StartPhysicalHull()
{
    if (g_hull.playing || !g_hull.rear || !g_hull.part[0].ready || !g_hull.part[1].ready ||
        g_hull.poses.empty() || !g_frameCameraValid || !g_frameGunValid) return;
    Mat4 fromView = {};
    if (!Invert(HullView(g_frameCamera),fromView)) return;
    g_hull.releaseMove = Multiply(Multiply(fromView,g_frameGunMove),g_hull.sourceToView);
    g_hull.released = GetTickCount64(); g_hull.playing = true;
    Log("[PHYSICAL EJECT] rear stroke releases retained hull; shot %ld, delay %llu ms",
        g_hull.shot,g_hull.released-g_hull.shotTime);
}
void TickPhysicalHull(bool gameplay)
{
    if (!gameplay || !g_enabled || !MotionInteractionEnabled() || !g_pumpEnabled || KnownNonPumpModel() || fc3pad::AmmoControls().x)
    { if (g_hull.active) ClearPhysicalHull(); return; }
    if (g_lastShotFrame != g_hull.shot)
    {
        if (g_lastShotFrame < 0 || g_frame-g_lastShotFrame > 2) return;
        ClearPhysicalHull(); g_hull.shot = g_lastShotFrame;
        g_hull.shotTime = GetTickCount64(); g_hull.active = true;
    }
    if (!g_hull.active) return;
    if (GetTickCount64()-g_hull.shotTime > 30000) { ClearPhysicalHull(); return; }
    if (g_hull.playing && !g_hull.ended && GetTickCount64()-g_hull.released > 1000)
    {
        g_hull.ended = true;
        Log("[PHYSICAL EJECT] visual window ended; stereo replay %s, retained poses %u",
            g_hull.replayLogged ? "submitted" : "NO COMPATIBLE PASS",static_cast<UINT>(g_hull.poses.size()));
    }
    if (g_pumpStage == 1) g_hull.rear = true;
    StartPhysicalHull();
}
bool PhysicalHullDraw(ID3D11DeviceContext* context, const DrawCall& call)
{
    if (!g_hull.active || g_hull.failed || !MotionInteractionEnabled() || !g_pumpEnabled || KnownNonPumpModel() || !g_inGameplay) return false;
    if (call.kind != 0 || call.d != 0 || !((call.b == 0 && call.a == 360) || (call.b == 360 && call.a == 144))) return false;
    ID3D11VertexShader* vs = nullptr; ID3D11PixelShader* ps = nullptr;
    context->VSGetShader(&vs,nullptr,nullptr); context->PSGetShader(&ps,nullptr,nullptr);
    const auto vh = ShaderHash(vs), ph = ShaderHash(ps); SafeRelease(vs); SafeRelease(ps);
    const int part = call.b == 0 ? 0 : 1;
    if (vh != 0xC386478F2F9495EBull || ph != (part == 0 ? 0x23E84ABA164A3A78ull : 0x0334CEB991DCA831ull)) return false;
    ID3D11Buffer* vb = nullptr; UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);
    D3D11_BUFFER_DESC desc = {}; if (vb) vb->GetDesc(&desc); SafeRelease(vb);
    if (desc.ByteWidth != 4192 || stride != 32 || offset != 0) return false;
    ID3D11Buffer* cb[2] = {}; context->VSGetConstantBuffers(0,2,cb);
    CameraSummary camera = {}; bool ok = HullCamera(cb[1],camera);
    std::vector<float> data(kSmallCbMax/4);
    UINT copied = ReadCopy(cb[0],data.data(),kSmallCbMax);
    for (auto*& b : cb) SafeRelease(b);
    if (!ok || copied < 80 || copied > kSmallCbMax) return false;
    data.resize(copied/4); const Mat4 world = HullWorld(data.data());
    for (double x : world.m) if (!std::isfinite(x)) return false;
    const V3 position = {world.m[3],world.m[7],world.m[11]};
    if (g_hull.poses.empty())
    {
        const V3 eye = {camera.position[0],camera.position[1],camera.position[2]};
        if (Length(Sub(position,eye)) > 1.25 || GetTickCount64()-g_hull.shotTime > 2000) return false;
        // No tessellation/geometry shader on the identified source pipeline.
        ID3D11GeometryShader* gs = nullptr; ID3D11HullShader* hs = nullptr; ID3D11DomainShader* ds = nullptr;
        context->GSGetShader(&gs,nullptr,nullptr); context->HSGetShader(&hs,nullptr,nullptr); context->DSGetShader(&ds,nullptr,nullptr);
        bool plain = !gs && !hs && !ds; SafeRelease(gs); SafeRelease(hs); SafeRelease(ds);
        if (!plain || FAILED(g_dev->CreateDeferredContext(0,&g_hull.deferred)))
        { g_hull.failed = true; Log("[PHYSICAL EJECT] unsupported source pipeline; leaving native shell visible"); return false; }
        g_hull.sourceToView = HullView(camera); g_hull.firstTime = GetTickCount64();
        g_hull.lastWorld = world;
    }
    else
    {
        const V3 last = {g_hull.lastWorld.m[3],g_hull.lastWorld.m[7],g_hull.lastWorld.m[11]};
        if (Length(Sub(position,last)) > .35) return false; // another casing instance
    }
    if (!CaptureHullPart(context,call,part,data))
    { g_hull.failed = true; Log("[PHYSICAL EJECT] snapshot failed; leaving native shell visible"); return false; }
    g_hull.lastWorld = world;
    if (g_hull.sampledFrame != g_frame && g_hull.poses.size() < 180 && GetTickCount64()-g_hull.firstTime <= 2000)
    {
        g_hull.poses.push_back({GetTickCount64()-g_hull.firstTime,world});
        g_hull.sampledFrame = g_frame;
        if (g_hull.poses.size() == 1) Log("[PHYSICAL EJECT] retained player's rigid hull; native visual suppressed");
    }
    StartPhysicalHull();
    return true;
}
// Executes only in a matching opaque world pass. TRUE restores the immediate
// context, including the render targets, eye CBs and every binding it touches.
bool ReplayPhysicalHull(ID3D11DeviceContext* context)
{
    if (!g_hull.playing || g_hull.failed || g_hull.replayFrame == g_frame || !g_hull.deferred) return false;
    const ULONGLONG elapsed = GetTickCount64()-g_hull.released;
    if (elapsed > 1000) return false;
    ID3D11VertexShader* vs = nullptr; context->VSGetShader(&vs,nullptr,nullptr);
    const auto hash = ShaderHash(vs); SafeRelease(vs);
    if (hash != 0xC386478F2F9495EBull) return false;
    ID3D11Buffer* cameraBuffer = nullptr; context->VSGetConstantBuffers(1,1,&cameraBuffer);
    CameraSummary camera = {}; bool compatible = HullCamera(cameraBuffer,camera);
    ID3D11RenderTargetView* targets[8] = {}, *twins[8] = {}; ID3D11DepthStencilView* depth = nullptr, *twinDepth = nullptr;
    context->OMGetRenderTargets(8,targets,&depth);
    const auto& first = g_hull.part[0];
    for (UINT i = 0; i < 8; ++i) compatible = compatible && HullFormat(targets[i]) == first.formats[i];
    compatible = compatible && depth && HullFormat(depth) == first.depthFormat;
    if (compatible)
    {
        for (UINT i = 0; i < first.targets; ++i)
        {
            if (!targets[i]) continue;
            twins[i] = static_cast<ID3D11RenderTargetView*>(TwinView(targets[i],ViewKind::RenderTarget,false));
            if (!twins[i]) compatible = false;
        }
        twinDepth = static_cast<ID3D11DepthStencilView*>(TwinView(depth,ViewKind::DepthStencil,false));
        compatible = compatible && twinDepth && g_eyeCb[0][0] && g_eyeCb[0][1];
    }
    bool submitted = false;
    if (compatible)
    {
        size_t sample = 0;
        while (sample+1 < g_hull.poses.size() && g_hull.poses[sample+1].time <= elapsed) ++sample;
        Mat4 world = g_hull.poses[sample].world;
        // If rear stroke happens before enough native samples have arrived,
        // continue the measured velocity briefly rather than freeze the hull.
        if (elapsed > g_hull.poses.back().time && g_hull.poses.size() >= 2)
        {
            const auto& a = g_hull.poses[g_hull.poses.size()-2]; const auto& b = g_hull.poses.back();
            const double dt = static_cast<double>(b.time-a.time)/1000;
            const double extra = static_cast<double>(elapsed-b.time)/1000;
            if (dt > .001) for (int r = 0; r < 3; ++r)
                world.m[r*4+3] += (b.world.m[r*4+3]-a.world.m[r*4+3])/dt*extra;
            world.m[11] -= 4.905*extra*extra; // Dunia world Z is up
        }
        world = Multiply(g_hull.releaseMove,world);
        D3D11_VIEWPORT viewport[16] = {}; UINT nvp = 16; context->RSGetViewports(&nvp,viewport);
        D3D11_RECT scissor[16] = {}; UINT nsc = 16; context->RSGetScissorRects(&nsc,scissor);
        const bool own = g_own; g_own = true;
        auto* dc = g_hull.deferred; dc->ClearState();
        dc->RSSetViewports(nvp,viewport); dc->RSSetScissorRects(nsc,scissor);
        for (auto& p : g_hull.part)
        {
            auto data = p.data;
            for (int r = 0; r < 3; ++r)
            {
                for (int c = 0; c < 4; ++c) data[r*4+c] = static_cast<float>(world.m[r*4+c]);
                data[12+r] = 0;
            }
            context->UpdateSubresource(p.globals,0,nullptr,data.data(),0,0);
            UINT stride = 32, offset = 0; dc->IASetVertexBuffers(0,1,&p.vb,&stride,&offset);
            dc->IASetIndexBuffer(p.ib,DXGI_FORMAT_R16_UINT,p.indexOffset); dc->IASetInputLayout(p.layout);
            dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            dc->VSSetShader(p.vs,nullptr,0); dc->PSSetShader(p.ps,nullptr,0);
            ID3D11Buffer* vertexCb[3] = {p.globals,cameraBuffer,p.clip}; dc->VSSetConstantBuffers(0,3,vertexCb);
            dc->PSSetConstantBuffers(0,8,p.psCb); dc->PSSetShaderResources(0,16,p.textures); dc->PSSetSamplers(0,16,p.samplers);
            dc->RSSetState(p.raster); dc->OMSetDepthStencilState(p.depth,p.stencil); dc->OMSetBlendState(p.blend,p.blendFactor,p.mask);
            for (int eye = 0; eye < 2; ++eye)
            {
                dc->OMSetRenderTargets(p.targets,eye ? twins : targets,eye ? twinDepth : depth);
                dc->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyeCb[0][eye]);
                if (HasFlag(p.ps,kPsPatchedGuid)) dc->PSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyePsCb[eye]);
                dc->DrawIndexed(p.draw.a,p.draw.b,p.draw.d);
            }
        }
        ID3D11CommandList* list = nullptr;
        if (SUCCEEDED(dc->FinishCommandList(FALSE,&list)))
        {
            context->ExecuteCommandList(list,TRUE); list->Release(); submitted = true; g_hull.replayFrame = g_frame;
            if (!g_hull.replayLogged) Log("[PHYSICAL EJECT] shell replay submitted to both eyes at frame %ld",g_frame);
            g_hull.replayLogged = true;
        }
        g_own = own;
    }
    for (UINT i = 0; i < 8; ++i) { SafeRelease(targets[i]); SafeRelease(twins[i]); }
    SafeRelease(depth); SafeRelease(twinDepth); SafeRelease(cameraBuffer);
    return submitted;
}
