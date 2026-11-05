// Diagnostic only: raw game resources before the motion overrides.
// Version 1 files consist of fixed-size ActionProbeRecord records, packed to 4.
#pragma pack(push, 4)
struct ActionProbeRecord
{
    UINT magic, version, frame, event, sequence, elapsed;
    UINT kind, a, b, c, e; int base;
    unsigned long long shader, vertexBuffer, indexBuffer;
    UINT vertexBytes, stride, offset, indexFormat, indexOffset;
    UINT cbBytes[6], cbFloats[6];
    float cb[6][576]; // complete 48-bone palettes; first 576 floats otherwise
    float trigger, leftGrip, rightGrip;
    UINT buttons;
};
#pragma pack(pop)
struct ActionProbeState
{
    HANDLE file = INVALID_HANDLE_VALUE;
    UINT event = 0, sequence = 0, draws = 0;
    LONG begin = 0;
    ULONGLONG started = 0;
    unsigned long long resources[64] = {};
    UINT resourceCount = 0, meshBytes = 0;
    unsigned long long censusKeys[2][96] = {};
    UINT censusCount[2] = {};
    unsigned long long bladeKeys[2][16] = {};
    UINT bladeCount[2] = {};
    struct HealSample217{unsigned long long key=0;ULONGLONG at=0;bool used=false;}healSamples217[64];
    UINT healBackground217=0,healCandidates217=0;
    unsigned long long healKeys217[2][16]={};UINT healCount217[2]={};
} g_actionProbe;
// Exact installed syringe_01.xbg LOD signatures. These are capture candidates,
// not proof of a health event; complete mesh hashes are saved for identification.
bool HealInjectorGeometry217(const fc3weapon::Geometry& d){
    return d.format==57&&d.stride==32&&d.offset==0&&
        ((d.vertexBytes==27424&&d.indexBytes==6144)||(d.vertexBytes==10848&&d.indexBytes==2112)||
         (d.vertexBytes==4192&&d.indexBytes==672)||(d.vertexBytes==2528&&d.indexBytes==288));
}
bool HealSample217(unsigned long long key,bool candidate,ULONGLONG now){
    auto& p=g_actionProbe;
    if(candidate?p.healCandidates217>=1792:p.healBackground217>=256)return false;
    ActionProbeState::HealSample217* slot=nullptr;
    const UINT first=candidate?48u:0u,last=candidate?64u:48u;
    for(UINT i=first;i<last;++i){auto& s=p.healSamples217[i];if(s.used&&s.key==key){slot=&s;break;}}
    if(!slot)for(UINT i=first;i<last;++i)if(!p.healSamples217[i].used){slot=&p.healSamples217[i];break;}
    if(!slot)return false;
    if(slot->used&&now>=slot->at&&now-slot->at<(candidate?16u:250u))return false;
    slot->used=true;slot->key=key;slot->at=now;
    if(candidate)++p.healCandidates217;else ++p.healBackground217;
    return true;
}
void ActionProbeStem217(wchar_t* name,size_t size){
    if(g_actionProbe.event==3)_snwprintf_s(name,size,_TRUNCATE,L"fc3_action_heal_%u_%llu_%u",GetCurrentProcessId(),g_actionProbe.started,g_actionProbe.sequence);
    else _snwprintf_s(name,size,_TRUNCATE,L"fc3_action_%s_%u",g_actionProbe.event==0?L"shot":g_actionProbe.event==1?L"reload":L"machete",g_actionProbe.sequence);
}
static LONG g_actionBladeSeenFrame=-100000;
void ActionProbeNoteBladeSeen(){g_actionBladeSeenFrame=g_frame;}
// The native hold-Y heal is still game-owned. This only opens a bounded,
// read-only capture after a deliberate hold, excluding the wheel chord.
ULONGLONG g_healHoldAt215=0;bool g_healHoldCaptured215=false;LONG g_healProbeCheckedFrame215=-1,g_healProbeTriggeredFrame215=-1;
bool HealProbeHoldEdge215(bool gameplay,const fc3pad::Controllers& controls,ULONGLONG now,LONG frame){
 if(g_healProbeCheckedFrame215==frame)return g_healProbeTriggeredFrame215==frame;
 g_healProbeCheckedFrame215=frame;
 const bool holding=gameplay&&controls.active&&controls.y&&controls.leftGrip<.5f&&!controls.menu&&!fc3pad::NativeMenuOpen()&&!fc3pad::GameplayInputBlocked();
 if(!holding){g_healHoldAt215=0;g_healHoldCaptured215=false;return false;}
 if(!g_healHoldAt215)g_healHoldAt215=now;
 if(g_healHoldCaptured215||now-g_healHoldAt215<500)return false;
 g_healHoldCaptured215=true;g_healProbeTriggeredFrame215=frame;return true;
}
// The older action recorder only sees draws already classified as first-person.
// A knife variant may use another camera path, so census both classifications
// briefly after an ordinary R3 swipe. This never changes a draw or game input.
void RecordMacheteDrawCensus(ID3D11DeviceContext* context,const DrawCall& call,int variant)
{
    auto& p=g_actionProbe;
    if(p.file==INVALID_HANDLE_VALUE||(p.event!=2&&p.event!=3)||
       call.kind!=0||!call.a||call.a>10000)return;
    const auto geometry=NativeWeaponGeometry(context,call);
    const bool injector=p.event==3&&HealInjectorGeometry217(geometry);
    if(GetTickCount64()-p.started>(injector?10000u:3000u))return;
    ID3D11Buffer* vb=nullptr,*ib=nullptr;UINT stride=0,offset=0,indexOffset=0;
    DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
    context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);
    context->IAGetIndexBuffer(&ib,&format,&indexOffset);
    D3D11_BUFFER_DESC vd={},id={};if(vb)vb->GetDesc(&vd);if(ib)ib->GetDesc(&id);
    if(!vb||!ib||vd.ByteWidth>1024*1024){SafeRelease(vb);SafeRelease(ib);return;}
    ID3D11VertexShader* shader=nullptr;context->VSGetShader(&shader,nullptr,nullptr);
    const auto hash=ShaderHash(shader);SafeRelease(shader);
    const int group=variant==1?0:1;
    const unsigned long long key=hash^(static_cast<unsigned long long>(vd.ByteWidth)<<32)^id.ByteWidth^
        (static_cast<unsigned long long>(call.b)<<16)^call.a;
    // Keep a separate reserved bank for the four known knife mesh sizes. The
    // ordinary world census can fill in one frame, before the blade appears.
    const bool blade=p.event==2&&(vd.ByteWidth==71720||vd.ByteWidth==201920||
        vd.ByteWidth==45320||vd.ByteWidth==41824);
    unsigned long long* keys=injector?p.healKeys217[group]:blade?p.bladeKeys[group]:p.censusKeys[group];
    auto& count=injector?p.healCount217[group]:blade?p.bladeCount[group]:p.censusCount[group];
    const UINT limit=blade||injector?16u:96u;
    bool seen=false;for(UINT i=0;i<count;++i)if(keys[i]==key){seen=true;break;}
    if(!seen&&count<limit){
        keys[count++]=key;
        Log(injector?"[SYRINGE CANDIDATE217] hold%u frame%ld variant%d VS%016llX VB%u stride%u offset%u IB%u format%u indexOffset%u count%u start%u base%d":p.event==3?"[HEAL DRAW CENSUS215] hold%u frame%ld variant%d VS%016llX VB%u stride%u offset%u IB%u format%u indexOffset%u count%u start%u base%d":"[MACHETE DRAW CENSUS] swipe%u frame%ld variant%d VS%016llX VB%u stride%u offset%u IB%u format%u indexOffset%u count%u start%u base%d",
            p.sequence,g_frame,variant,hash,vd.ByteWidth,stride,offset,id.ByteWidth,
            static_cast<UINT>(format),indexOffset,call.a,call.b,call.d);
    }
    SafeRelease(vb);SafeRelease(ib);
}
static const wchar_t* ActionName(UINT event)
{
    return event == 0 ? L"shot" : event == 1 ? L"reload" : event == 2 ? L"machete" : L"heal";
}
void CloseActionProbe(const char* reason)
{
    if (g_actionProbe.file == INVALID_HANDLE_VALUE) return;
    CloseHandle(g_actionProbe.file);
    g_actionProbe.file = INVALID_HANDLE_VALUE;
    Log("[ACTION PROBE] event %u sequence %u closed: %s, %u draws, %u mesh bytes",
        g_actionProbe.event, g_actionProbe.sequence, reason, g_actionProbe.draws, g_actionProbe.meshBytes);
}
// Input edges are requests, not proof that the game accepted an action.
void TickActionProbe(bool gameplay)
{
    static bool previous[4] = {};
    static UINT captures[4] = {};
    static LONG firstMacheteRequest=-100000;
    static LONG previousShot = -100000;
    const auto controls = fc3pad::Latest();
    const bool pressed[4] = {false, controls.x,controls.rightStickClick,false};
    const bool healEdge=HealProbeHoldEdge215(gameplay,controls,GetTickCount64(),g_frame);
    if(healEdge)fc3aim::BeginHealObservation299();
    if (g_actionProbe.file != INVALID_HANDLE_VALUE &&
        (!gameplay || GetTickCount64() - g_actionProbe.started >= 10000))
        CloseActionProbe(gameplay ? "10-second window finished" : "left gameplay");
    // Use the pump's accepted trigger edge, rather than every trigger squeeze.
    const bool shot = g_lastShotFrame != previousShot && g_lastShotFrame >= 0;
    previousShot = g_lastShotFrame;
    for (UINT event = 0; event < 4; ++event)
    {
        const bool edge = event == 0 ? shot : event==3?healEdge:pressed[event] && !previous[event];
        if (event == 0) continue; // richer ejection probe owns shot capture now
        previous[event] = pressed[event];
        if (event == 2 && controls.leftStickClick) continue;
        // The first two R3 windows can identify an unknown equipped variant.
        // Reserve the last two for a later phase or a verified default blade:
        // in run 59172 all four windows were spent before machete.xbg appeared.
        if(event==2&&edge&&captures[event]>=2&&captures[event]<4&&
           g_frame-firstMacheteRequest<1200&&g_frame-g_actionBladeSeenFrame>120)continue;
        // Allow both equipped knife variants, with room for an accidental R3
        // press before the two deliberate swipes in a single diagnostic run.
        if (!edge || !gameplay || !controls.active || captures[event] >= (event == 2||event==3 ? 4u : 2u)) continue;
        // A reload/knife input during the waiting period must not discard the
        // syringe evidence before the native holster/attach phase appears.
        if(event!=3&&g_actionProbe.event==3&&g_actionProbe.file!=INVALID_HANDLE_VALUE)continue;
        if(event==2&&firstMacheteRequest<0)firstMacheteRequest=g_frame;
        // Preserve reload/machete animation if a trigger is touched during it.
        if (event == 0 && g_actionProbe.file != INVALID_HANDLE_VALUE && g_actionProbe.event != 0) continue;
        CloseActionProbe("next action request");
        g_actionProbe = ActionProbeState{};
        g_actionProbe.event = event;
        g_actionProbe.sequence = ++captures[event];
        g_actionProbe.begin = g_frame;
        g_actionProbe.started = GetTickCount64();
        wchar_t path[MAX_PATH] = {};
        wchar_t stem[128]={};ActionProbeStem217(stem,128);
        _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%s%s.bin",fc3::DataFolder(),stem);
        g_actionProbe.file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        Log("[ACTION PROBE] %s event %u sequence %u frame %ld: REQUEST, capturing raw first-person draws for up to 10 seconds",
            g_actionProbe.file == INVALID_HANDLE_VALUE ? "OPEN FAILED" : "started", event,
            g_actionProbe.sequence, g_frame);
    }
}
void ActionMesh(ID3D11DeviceContext* context, ID3D11Buffer* buffer, const wchar_t* role)
{
    if (!buffer || g_actionProbe.resourceCount >= 64) return;
    const auto id = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(buffer));
    // A released VB address can be reused for an IB in the same window. They
    // have different output filenames and must not share a deduplication key.
    const auto resourceKey=(id<<1)|(role[0]==L'i'?1ull:0ull);
    for (UINT i = 0; i < g_actionProbe.resourceCount; ++i)
        if (g_actionProbe.resources[i] == resourceKey) return;
    g_actionProbe.resources[g_actionProbe.resourceCount++] = resourceKey;
    D3D11_BUFFER_DESC desc = {}; buffer->GetDesc(&desc);
    if (desc.ByteWidth > 8 * 1024 * 1024 ||
        g_actionProbe.meshBytes + desc.ByteWidth > 32 * 1024 * 1024) return;
    std::vector<unsigned char> bytes;
    if (!ReadBytes(context, buffer, bytes)) return;
    wchar_t name[128] = {};
    wchar_t stem[96]={};ActionProbeStem217(stem,96);
    _snwprintf_s(name, 128, _TRUNCATE, L"%s_%s_%016llX.bin",stem,role,id);
    wchar_t path[MAX_PATH] = {};
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%s%s", fc3::DataFolder(), name);
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD written = 0;
    const bool ok = file != INVALID_HANDLE_VALUE && WriteFile(file, bytes.data(),
        static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    g_actionProbe.meshBytes += static_cast<UINT>(bytes.size());
    Log("[ACTION PROBE] mesh %016llX bytes %u %s", id, desc.ByteWidth, ok ? "saved" : "SAVE FAILED");
}
void RecordActionDraw(ID3D11DeviceContext* context, const DrawCall& call)
{
    if (g_actionProbe.file == INVALID_HANDLE_VALUE) return;
    if (g_actionProbe.draws >= 2048) { if(g_actionProbe.event!=3)CloseActionProbe("2048-draw limit"); return; }
    ActionProbeRecord record = {};
    record.magic = 0x41505231; record.version = 1; record.frame = g_frame;
    record.event = g_actionProbe.event; record.sequence = g_actionProbe.sequence;
    record.elapsed = g_frame - g_actionProbe.begin;
    record.kind = call.kind; record.a = call.a; record.b = call.b;
    record.c = call.c; record.e = call.e; record.base = call.d;
    ID3D11VertexShader* shader = nullptr; context->VSGetShader(&shader, nullptr, nullptr);
    record.shader = ShaderHash(shader); SafeRelease(shader);
    const UINT priorResources = g_actionProbe.resourceCount;
    ID3D11Buffer* vertex = nullptr;
    context->IAGetVertexBuffers(0, 1, &vertex, &record.stride, &record.offset);
    if (vertex)
    {
        D3D11_BUFFER_DESC desc = {}; vertex->GetDesc(&desc);
        record.vertexBytes = desc.ByteWidth;
        record.vertexBuffer = reinterpret_cast<uintptr_t>(vertex);
    }
    ID3D11Buffer* indices = nullptr; DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    context->IAGetIndexBuffer(&indices, &format, &record.indexOffset);
    record.indexFormat = static_cast<UINT>(format);
    record.indexBuffer = reinterpret_cast<uintptr_t>(indices);
    if(g_actionProbe.event==3){
        D3D11_BUFFER_DESC id={};if(indices)indices->GetDesc(&id);
        const fc3weapon::Geometry d={record.vertexBytes,id.ByteWidth,record.stride,record.offset,call.b,call.a,record.indexFormat};
        const auto key=record.shader^record.vertexBuffer^(record.indexBuffer<<1)^
            (static_cast<unsigned long long>(call.b)<<32)^record.offset;
        if(!HealSample217(key,HealInjectorGeometry217(d),GetTickCount64())){SafeRelease(vertex);SafeRelease(indices);return;}
    }
    ActionMesh(context, vertex, L"vb0");SafeRelease(vertex);
    ActionMesh(context, indices, L"ib"); SafeRelease(indices);
    // Always capture a newly appearing shell/blade; sample recurring draws every third frame.
    if (g_actionProbe.event!=3&&priorResources == g_actionProbe.resourceCount && record.elapsed % 3 != 0) return;
    ID3D11Buffer* buffers[6] = {}; context->VSGetConstantBuffers(0, 6, buffers);
    for (UINT slot = 0; slot < 6; ++slot)
    {
        if (!buffers[slot]) continue;
        D3D11_BUFFER_DESC desc = {}; buffers[slot]->GetDesc(&desc);
        record.cbBytes[slot] = desc.ByteWidth;
        std::vector<float> data;
        if (desc.ByteWidth <= 4096 && ReadBuffer(context, buffers[slot], data))
        {
            record.cbFloats[slot] = static_cast<UINT>((std::min)(data.size(), size_t(576)));
            memcpy(record.cb[slot], data.data(), record.cbFloats[slot] * sizeof(float));
        }
        buffers[slot]->Release();
    }
    const auto controls = fc3pad::Latest();
    record.trigger = controls.rightTrigger; record.leftGrip = controls.leftGrip;
    record.rightGrip = controls.rightGrip;
    record.buttons = (controls.x ? 1 : 0) | (controls.rightStickClick ? 2 : 0) |
                     (controls.leftStickClick ? 4 : 0) | (controls.y ? 8 : 0);
    DWORD written = 0;
    if (!WriteFile(g_actionProbe.file, &record, sizeof(record), &written, nullptr) || written != sizeof(record))
        CloseActionProbe("trace write failed");
    else ++g_actionProbe.draws;
}
void RecordHealAlternate217(ID3D11DeviceContext* context,const DrawCall& call,int variant){
    auto& p=g_actionProbe;
    if(variant==1||p.event!=3||p.file==INVALID_HANDLE_VALUE||call.kind!=0||call.d!=0||
       GetTickCount64()-p.started>10000||!HealInjectorGeometry217(NativeWeaponGeometry(context,call)))return;
    // The syringe is absent from the weapon model table. Record it on its
    // actual camera path without changing projection, input, health or poses.
    ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);CameraSummary camera={};UINT bytes=sizeof(camera);
    const bool main=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera)&&camera.mainView;
    SafeRelease(cb);if(main)RecordActionDraw(context,call);
}
