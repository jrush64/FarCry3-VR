// EPR1: variable chunks {Header, Marker} or {Header, Draw, VB slice, IB slice}.
// Captures original game state; never changes firing, pumping or effect timing.
#pragma pack(push, 4)
struct EjectionHeader { UINT magic, version, type, bytes, sequence, frame, milliseconds; };
struct EjectionMarker { UINT event, stage, chambered, holding; float pull; LONG shotFrame; };
struct EjectionExtraStream
{
    unsigned long long buffer;
    UINT bytes, stride, offset, usage, snapshotOffset, snapshotBytes;
};
struct EjectionDraw
{
    ActionProbeRecord raw;
    unsigned long long pixelShader;
    UINT phase, vertexUsage, vertexSliceOffset, vertexSliceBytes;
    UINT indexSliceOffset, indexSliceBytes, cbShadowMask;
    EjectionMarker pump;
    EjectionExtraStream extra[3];
};
#pragma pack(pop)
struct EjectionKey
{
    unsigned long long vb, vs;
    UINT start, count, offset, stride;
    bool operator==(const EjectionKey& k) const
    { return vb == k.vb && vs == k.vs && start == k.start && count == k.count && offset == k.offset && stride == k.stride; }
};
struct EjectionState
{
    HANDLE file = INVALID_HANDLE_VALUE;
    UINT sequence = 0, draws = 0, byteTotal = 0, skipped = 0;
    ULONGLONG begun = 0, burstUntil = 0, finishAfter = 0;
    LONG frame = -1, baselineUntil = -1;
    UINT dynamicReads = 0, otherReads = 0, worldDraws = 0;
    bool pulled = false, returned = false, dataLimited = false;
    std::vector<EjectionKey> worldCaptured;
    std::vector<EjectionKey> baseline;
    std::vector<unsigned long long> meshes, shaders;
} g_ejection;
EjectionMarker EjectionPump(UINT event)
{
    return { event, static_cast<UINT>(g_pumpStage), g_chambered ? 1u : 0u,
        g_twoHand ? 1u : 0u, static_cast<float>(g_slideGrab - g_slide), g_lastShotFrame };
}
bool EjectionWrite(const void* data, UINT bytes)
{
    DWORD written = 0;
    const bool ok = g_ejection.file != INVALID_HANDLE_VALUE &&
        WriteFile(g_ejection.file, data, bytes, &written, nullptr) && written == bytes;
    if (ok) g_ejection.byteTotal += bytes;
    return ok;
}
void EndEjectionProbe(const char* reason)
{
    if (g_ejection.file == INVALID_HANDLE_VALUE) return;
    CloseHandle(g_ejection.file); g_ejection.file = INVALID_HANDLE_VALUE;
    Log("[EJECT PROBE] capture %u closed: %s; %u draws, %u trace bytes, %u skipped world draws",
        g_ejection.sequence, reason, g_ejection.draws, g_ejection.byteTotal, g_ejection.skipped);
}
EjectionHeader EjectionChunk(UINT type, UINT bytes)
{
    return {0x45505231, 1, type, bytes, g_ejection.sequence, static_cast<UINT>(g_frame),
        static_cast<UINT>(GetTickCount64() - g_ejection.begun)};
}
void MarkEjectionProbe(UINT event)
{
    const EjectionHeader header = EjectionChunk(0, sizeof(EjectionMarker));
    const EjectionMarker marker = EjectionPump(event);
    if (!EjectionWrite(&header, sizeof(header)) || !EjectionWrite(&marker, sizeof(marker)))
        EndEjectionProbe("marker write failed");
    Log("[EJECT PROBE] capture %u marker %u frame %ld time %u ms pull %.3f m stage %u chambered %u",
        g_ejection.sequence, event, g_frame, header.milliseconds, marker.pull, marker.stage, marker.chambered);
}
// Markers: 0 trigger accepted by pump gate (not proof of an engine shot),
// 1 physical rear threshold, 2 physical forward completion, 3 two-hand release.
void TickEjectionProbe(bool gameplay)
{
    static LONG previousShot = -100000;
    static bool wasHolding = false, announced = false;
    const ULONGLONG now = GetTickCount64();
    const auto controls = fc3pad::Latest();
    if (gameplay && !announced && g_visibleWeaponDrawsLastFrame >= 4)
    {
        announced = true; g_ejection.baselineUntil = g_frame + 30;
        Log("[EJECT PROBE] ready: capture first two shots, automatic effect window and physical pull/return markers");
    }
    const bool shot = g_lastShotFrame >= 0 && g_lastShotFrame != previousShot;
    previousShot = g_lastShotFrame;
    if (shot && gameplay && g_ejection.sequence < 2)
    {
        EndEjectionProbe("next shot request");
        ++g_ejection.sequence;
        g_ejection.draws = g_ejection.byteTotal = g_ejection.skipped = 0;
        g_ejection.meshes.clear(); g_ejection.shaders.clear();
        g_ejection.worldCaptured.clear(); g_ejection.dataLimited = false;
        g_ejection.pulled = g_ejection.returned = false;
        g_ejection.begun = now; g_ejection.burstUntil = now + 2000;
        g_ejection.finishAfter = 0;
        wchar_t path[MAX_PATH] = {};
        _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%sfc3_ejection_%u.bin", fc3::DataFolder(), g_ejection.sequence);
        g_ejection.file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (g_ejection.file == INVALID_HANDLE_VALUE) Log("[EJECT PROBE] OPEN FAILED");
        else MarkEjectionProbe(0);
    }
    if (g_ejection.file != INVALID_HANDLE_VALUE)
    {
        if (!gameplay) EndEjectionProbe("left gameplay");
        else if (controls.x) { MarkEjectionProbe(4); EndEjectionProbe("reload interrupted pump observation"); }
        else
        {
            if (!g_ejection.pulled && !g_chambered && g_pumpStage == 1 && g_twoHand)
            {
                g_ejection.pulled = true; g_ejection.burstUntil = now + 1500;
                MarkEjectionProbe(1);
            }
            if (g_ejection.pulled && !g_ejection.returned && g_chambered)
            {
                g_ejection.returned = true; g_ejection.burstUntil = now + 1500;
                g_ejection.finishAfter = now + 1600; MarkEjectionProbe(2);
            }
            if (wasHolding && !g_twoHand) MarkEjectionProbe(3);
            if (now - g_ejection.begun >= 30000 ||
                (g_ejection.finishAfter && now >= g_ejection.finishAfter))
                EndEjectionProbe("window finished");
        }
    }
    wasHolding = g_twoHand;
}
bool EjectionReadRange(ID3D11DeviceContext* context, ID3D11Buffer* source,
                       UINT offset, UINT length, std::vector<unsigned char>& bytes)
{
    bytes.clear();
    if (!source || !length || length > 128 * 1024) return false;
    D3D11_BUFFER_DESC original = {}; source->GetDesc(&original);
    if (offset > original.ByteWidth || length > original.ByteWidth - offset) return false;
    D3D11_BUFFER_DESC desc = {}; desc.ByteWidth = length;
    desc.Usage = D3D11_USAGE_STAGING; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Device* device = nullptr; context->GetDevice(&device);
    ID3D11Buffer* stage = nullptr;
    const bool created = device && SUCCEEDED(device->CreateBuffer(&desc, nullptr, &stage));
    SafeRelease(device); if (!created) return false;
    const bool own = g_own; g_own = true;
    D3D11_BOX box = {offset, 0, 0, offset + length, 1, 1};
    g_oCopySubresourceRegion(context, stage, 0, 0, 0, 0, source, 0, &box);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const bool ok = SUCCEEDED(g_oMap(context, stage, 0, D3D11_MAP_READ, 0, &mapped));
    if (ok)
    {
        bytes.assign(static_cast<const unsigned char*>(mapped.pData), static_cast<const unsigned char*>(mapped.pData) + length);
        g_oUnmap(context, stage, 0);
    }
    g_own = own; stage->Release(); return ok;
}
bool EjectionSeen(std::vector<unsigned long long>& list, unsigned long long id, size_t cap)
{
    if (std::find(list.begin(), list.end(), id) != list.end() || list.size() >= cap) return true;
    list.push_back(id); return false;
}
void EjectionShader(ID3D11DeviceChild* shader, bool pixel)
{
    if (!shader) return;
    const auto hash = ShaderHash(shader);
    if (EjectionSeen(g_ejection.shaders, hash ^ (pixel ? 1 : 0), 64)) return;
    UINT size = 0;
    shader->GetPrivateData(kEjectionBytecodeGuid, &size, nullptr);
    if (!size || size > 65536) return;
    std::vector<unsigned char> code(size);
    if (FAILED(shader->GetPrivateData(kEjectionBytecodeGuid, &size, code.data()))) return;
    wchar_t name[100] = {};
    _snwprintf_s(name, 100, _TRUNCATE, L"fc3_ejection_%u_%s_%016llX.dxbc",
        g_ejection.sequence, pixel ? L"ps" : L"vs", hash);
    WriteBlob(name, code);
}
void EjectionStaticMesh(ID3D11DeviceContext* context, ID3D11Buffer* buffer, bool index, UINT slot = 0)
{
    if (!buffer) return;
    const auto id = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(buffer));
    if (EjectionSeen(g_ejection.meshes, id, 64)) return;
    D3D11_BUFFER_DESC desc = {}; buffer->GetDesc(&desc);
    if (desc.ByteWidth > 1024 * 1024) return;
    std::vector<unsigned char> data;
    if (!ReadBytes(context, buffer, data)) return;
    wchar_t name[100] = {};
    if (index) _snwprintf_s(name, 100, _TRUNCATE, L"fc3_ejection_%u_ib_%016llX.bin", g_ejection.sequence, id);
    else _snwprintf_s(name, 100, _TRUNCATE, L"fc3_ejection_%u_vb%u_%016llX.bin", g_ejection.sequence, slot, id);
    WriteBlob(name, data);
}
void RecordEjectionDraw(ID3D11DeviceContext* context, const DrawCall& call, bool firstPerson)
{
    const bool active = g_ejection.file != INVALID_HANDLE_VALUE;
    if (!active && (g_ejection.baselineUntil < g_frame || g_ejection.baselineUntil < 0)) return;
    ID3D11Buffer* vertex = nullptr; UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0, 1, &vertex, &stride, &offset);
    if (!vertex) return;
    struct ReleaseVertex { ID3D11Buffer* b; ~ReleaseVertex(){ b->Release(); } } releaseVertex{vertex};
    D3D11_BUFFER_DESC desc = {}; vertex->GetDesc(&desc);
    if (!firstPerson && (desc.ByteWidth > 512 * 1024 || call.a > 2048 || stride > 64 || !stride)) return;
    ID3D11VertexShader* vs = nullptr; context->VSGetShader(&vs, nullptr, nullptr);
    const auto hash = ShaderHash(vs);
    struct ReleaseShader { ID3D11VertexShader* s; ~ReleaseShader(){ SafeRelease(s); } } releaseShader{vs};
    const bool particle = hash == 0x42F3089A89C80C9Cull || hash == 0xDB23497C92F47D21ull ||
                          hash == 0x8D2F62CE1FDB1DC2ull || hash == 0xB9C22C7670E71A08ull;
    const UINT indexStart = call.kind >= 2 ? call.c : call.b;
    const bool knownHull = hash == 0xC386478F2F9495EBull && desc.ByteWidth == 4192 && stride == 32 && offset == 0 &&
        ((indexStart == 0 && call.a == 360) || (indexStart == 360 && call.a == 144));
    // The spent hull is now identified as a rigid world mesh, separate from these pooled effects.
    if (firstPerson && particle) return;
    const bool priority = knownHull || (hash == 0xA38F7CD233BCE12Bull && call.a <= 512);
    const EjectionKey key = {reinterpret_cast<uintptr_t>(vertex), hash,
        call.kind >= 2 ? call.c : call.b, call.a, offset, stride};
    const bool baseline = std::find(g_ejection.baseline.begin(), g_ejection.baseline.end(), key) != g_ejection.baseline.end();
    if (!active)
    {
        if (!firstPerson && !particle && !baseline && g_ejection.baseline.size() < 2048) g_ejection.baseline.push_back(key);
        return;
    }
    if (g_ejection.draws >= 4096 || g_ejection.byteTotal >= 64 * 1024 * 1024)
    {
        if (!g_ejection.dataLimited) Log("[EJECT PROBE] geometry limit reached; retaining pump markers until the window finishes");
        g_ejection.dataLimited = true; return;
    }
    if (g_ejection.frame != g_frame)
    { g_ejection.frame = g_frame; g_ejection.dynamicReads = g_ejection.otherReads = g_ejection.worldDraws = 0; }
    const bool burst = GetTickCount64() <= g_ejection.burstUntil;
    if (firstPerson ? g_frame % (burst ? 2 : 6) != 0 : !burst || (baseline && !priority)) return;
    ID3D11Buffer* cb[6] = {}; context->VSGetConstantBuffers(0, 6, cb);
    struct ReleaseBuffers { ID3D11Buffer** b; ~ReleaseBuffers(){for(int i=0;i<6;++i)SafeRelease(b[i]);} } releaseBuffers{cb};
    bool mainView = firstPerson;
    for (auto* buffer : cb)
    {
        CameraSummary summary = {}; UINT size = sizeof(summary);
        if (buffer && SUCCEEDED(buffer->GetPrivateData(kCameraSummaryGuid, &size, &summary)) &&
            size == sizeof(summary) && summary.mainView) mainView = true;
    }
    if (!mainView) return; // excludes shadow passes and UI
    if (!firstPerson && !priority)
    {
        if (std::find(g_ejection.worldCaptured.begin(), g_ejection.worldCaptured.end(), key) != g_ejection.worldCaptured.end() ||
            g_ejection.worldCaptured.size() >= 64) { ++g_ejection.skipped; return; }
        g_ejection.worldCaptured.push_back(key);
    }
    if (!firstPerson && !priority && ++g_ejection.worldDraws > 48) { ++g_ejection.skipped; return; }
    EjectionDraw draw = {}; auto& raw = draw.raw;
    raw.magic = 0x41505231; raw.version = 1; raw.frame = g_frame; raw.event = 3;
    raw.sequence = g_ejection.sequence; raw.elapsed = static_cast<UINT>(GetTickCount64() - g_ejection.begun);
    raw.kind = call.kind; raw.a = call.a; raw.b = call.b; raw.c = call.c; raw.e = call.e; raw.base = call.d;
    raw.shader = hash; raw.vertexBuffer = reinterpret_cast<uintptr_t>(vertex);
    raw.vertexBytes = desc.ByteWidth; raw.stride = stride; raw.offset = offset;
    draw.vertexUsage = desc.Usage; draw.phase = (firstPerson ? 1 : 0) | (burst ? 2 : 0) | (particle ? 4 : 0) | (knownHull ? 8 : 0);
    draw.pump = EjectionPump(4);
    ID3D11PixelShader* ps = nullptr; context->PSGetShader(&ps, nullptr, nullptr);
    draw.pixelShader = ShaderHash(ps); EjectionShader(vs, false); EjectionShader(ps, true); SafeRelease(ps);
    for (int slot = 0; slot < 6; ++slot)
    {
        if (!cb[slot]) continue;
        D3D11_BUFFER_DESC b = {}; cb[slot]->GetDesc(&b); raw.cbBytes[slot] = b.ByteWidth;
        if (b.ByteWidth > sizeof(raw.cb[slot])) continue;
        const UINT copied = ReadCopy(cb[slot], raw.cb[slot], sizeof(raw.cb[slot]));
        if (copied == b.ByteWidth)
        { raw.cbFloats[slot] = copied / 4; draw.cbShadowMask |= 1u << slot; }
        else if (firstPerson)
        {
            std::vector<float> data;
            if (ReadBuffer(context, cb[slot], data))
            { raw.cbFloats[slot] = static_cast<UINT>(data.size()); memcpy(raw.cb[slot], data.data(), data.size()*4); }
        }
    }
    const auto controls = fc3pad::Latest(); raw.trigger = controls.rightTrigger;
    raw.leftGrip = controls.leftGrip; raw.rightGrip = controls.rightGrip;
    raw.buttons = (controls.x ? 1 : 0) | (controls.rightStickClick ? 2 : 0) | (controls.leftStickClick ? 4 : 0);
    ID3D11Buffer* indices = nullptr; DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    context->IAGetIndexBuffer(&indices, &format, &raw.indexOffset);
    struct ReleaseIndices { ID3D11Buffer* b; ~ReleaseIndices(){SafeRelease(b);} } releaseIndices{indices};
    raw.indexBuffer = reinterpret_cast<uintptr_t>(indices); raw.indexFormat = format;
    std::vector<unsigned char> vertexBytes, indexBytes;
    if (desc.Usage != D3D11_USAGE_DYNAMIC)
    { EjectionStaticMesh(context, vertex, false); EjectionStaticMesh(context, indices, true); }
    else if ((priority || firstPerson) ? g_ejection.dynamicReads < 8 : g_ejection.otherReads < 4)
    {
        if (priority || firstPerson) ++g_ejection.dynamicReads; else ++g_ejection.otherReads;
        const UINT start = call.kind >= 2 ? call.c : call.b;
        unsigned long long first = start, last = first + call.a;
        bool range = call.a != 0;
        if (call.kind == 0 || call.kind == 2)
        {
            const UINT width = format == DXGI_FORMAT_R16_UINT ? 2 : format == DXGI_FORMAT_R32_UINT ? 4 : 0;
            const auto ibOffset = static_cast<unsigned long long>(raw.indexOffset) + static_cast<unsigned long long>(start) * width;
            range = width && ibOffset <= 0xffffffffu && call.a <= 128*1024 / width &&
                EjectionReadRange(context, indices, static_cast<UINT>(ibOffset), call.a*width, indexBytes);
            if (range)
            {
                draw.indexSliceOffset = static_cast<UINT>(ibOffset); first = 0xffffffffu; last = 0;
                for (UINT i = 0; i < call.a; ++i)
                {
                    UINT index = 0; memcpy(&index, indexBytes.data()+i*width, width);
                    const long long adjusted = static_cast<long long>(index) + call.d;
                    if (adjusted < 0) { range = false; break; }
                    first = (std::min)(first, static_cast<unsigned long long>(adjusted));
                    last = (std::max)(last, static_cast<unsigned long long>(adjusted)+1);
                }
            }
        }
        const auto vbOffset = offset + first*stride, vbLength = (last-first)*stride;
        if (range && vbOffset <= 0xffffffffu && vbLength <= 128*1024 &&
            EjectionReadRange(context, vertex, static_cast<UINT>(vbOffset), static_cast<UINT>(vbLength), vertexBytes))
            draw.vertexSliceOffset = static_cast<UINT>(vbOffset);
        draw.vertexSliceBytes = static_cast<UINT>(vertexBytes.size());
        draw.indexSliceBytes = static_cast<UINT>(indexBytes.size());
    }
    std::vector<unsigned char> extraData[3];
    ID3D11Buffer* extraBuffers[3] = {}; UINT extraStrides[3] = {}, extraOffsets[3] = {};
    context->IAGetVertexBuffers(1, 3, extraBuffers, extraStrides, extraOffsets);
    UINT extraBytes = 0;
    for (UINT slot = 0; slot < 3; ++slot)
    {
        auto* buffer = extraBuffers[slot]; if (!buffer) continue;
        auto& stream = draw.extra[slot]; D3D11_BUFFER_DESC extraDesc = {}; buffer->GetDesc(&extraDesc);
        stream.buffer = reinterpret_cast<uintptr_t>(buffer); stream.bytes = extraDesc.ByteWidth;
        stream.stride = extraStrides[slot]; stream.offset = extraOffsets[slot]; stream.usage = extraDesc.Usage;
        if (extraDesc.Usage != D3D11_USAGE_DYNAMIC) EjectionStaticMesh(context, buffer, false, slot+1);
        else if ((priority || firstPerson) ? g_ejection.dynamicReads < 8 : g_ejection.otherReads < 4)
        {
            if (priority || firstPerson) ++g_ejection.dynamicReads; else ++g_ejection.otherReads;
            // Without guessing per-instance input-layout step rates, retain the complete small stream.
            if (EjectionReadRange(context, buffer, 0, extraDesc.ByteWidth, extraData[slot]))
                stream.snapshotBytes = static_cast<UINT>(extraData[slot].size());
        }
        extraBytes += stream.snapshotBytes; buffer->Release();
    }
    const EjectionHeader header = EjectionChunk(1, sizeof(draw)+draw.vertexSliceBytes+draw.indexSliceBytes+extraBytes);
    if (!EjectionWrite(&header,sizeof(header)) || !EjectionWrite(&draw,sizeof(draw)) ||
        (!vertexBytes.empty() && !EjectionWrite(vertexBytes.data(),draw.vertexSliceBytes)) ||
        (!indexBytes.empty() && !EjectionWrite(indexBytes.data(),draw.indexSliceBytes)))
        EndEjectionProbe("draw write failed");
    else
    {
        for (UINT slot = 0; slot < 3; ++slot)
            if (!extraData[slot].empty() && !EjectionWrite(extraData[slot].data(), draw.extra[slot].snapshotBytes))
            { EndEjectionProbe("extra stream write failed"); return; }
        ++g_ejection.draws;
    }
}
