// Read-only animation timeline. Source palettes and final upload palettes are paired.
#pragma pack(push,4)
struct AnimationRecord
{
    UINT magic=0x414E5031,version=1,bytes=sizeof(AnimationRecord),kind=0;
    ULONGLONG clock=0;
    LONG frame=0;UINT thread=0,event=0;int side=-1;
    UINT drawKind=0,count=0,start=0;int base=0;
    unsigned long long shader=0,vertex=0,index=0;
    UINT vertexBytes=0,stride=0,offset=0,indexFormat=0,rawBytes=0,finalBytes=0;
    float inputs[7]={};UINT buttons=0,flags=0;LONG shotFrame=0,pump=0;
    UINT state=0,wrapper=0,weapon=0,explicitCount=0;int requested=0;
    UINT callerRva=0,loaded=0,stateWords[16]={};
    float globals[88]={},camera[16]={},controllers[64]={};
    float raw[576]={},finalPose[576]={};
};
#pragma pack(pop)
struct AnimationProbe
{
#ifdef FC3_AMMO_TEST
    bool enabled=false;
#else
    bool enabled=true;
#endif
    HANDLE file=INVALID_HANDLE_VALUE;wchar_t path[MAX_PATH]={};
    std::vector<AnimationRecord> ring;
    UINT head=0,segment=0,fileBytes=0,totalBytes=0;
    ULONGLONG until=0,lastFlush=0,lastSample=0,retry=0;
    LONG poses=0,right=0,left=0,shell=0,shots=0,native=0,error=0,lost=0;
    LONG modelFamilies=0;bool savedModels[sizeof(fc3weapon::kProfiles)/sizeof(*fc3weapon::kProfiles)]={};
    LONG priorShot=-100000,priorRequest=0;bool priorX=false,sample=false,pending=false,full=false;
    LONG seenFrame=-1;unsigned long long seen[24]={};UINT seenCount=0;
    AnimationRecord draw;
    AnimationRecord writeBatch169[8];UINT writeCount169=0;
    ULONGLONG writeCalls169=0,writeTicks169=0,flushTicks169=0;
} g_animation;
SRWLOCK g_animationQueueLock=SRWLOCK_INIT;
AnimationRecord g_animationQueue[64];UINT g_animationQueueCount=0;
void AnimationStamp(AnimationRecord& r)
{
    LARGE_INTEGER clock;QueryPerformanceCounter(&clock);r.clock=clock.QuadPart;
    r.frame=g_frame;r.thread=GetCurrentThreadId();r.shotFrame=g_lastShotFrame;r.pump=g_pumpStage;
    const auto c=fc3pad::Latest();r.inputs[0]=c.rightTrigger;r.inputs[1]=c.leftGrip;r.inputs[2]=c.rightGrip;
    r.inputs[3]=c.leftX;r.inputs[4]=c.leftY;r.inputs[5]=c.rightX;r.inputs[6]=c.rightY;
    r.buttons=(c.x?1u:0u)|(c.y?2u:0u)|(c.rightStickClick?4u:0u);
    r.flags=(c.active?1u:0u)|(g_twoHand?2u:0u)|(g_reload.held?4u:0u)|(g_reload.mode?8u:0u)|
        (g_handRightValid?16u:0u)|(g_handLeftValid?32u:0u)|(g_reloadPose.active?64u:0u);
    memcpy(r.controllers,g_handBody,32*sizeof(float));memcpy(r.controllers+32,g_handRight,16*sizeof(float));
    memcpy(r.controllers+48,g_handLeft,16*sizeof(float));
}
bool AnimationDrain169();
bool AnimationFlush169();
void AnimationClose()
{
    if(g_animation.file==INVALID_HANDLE_VALUE)return;
    AnimationDrain169();
    if(g_animation.file==INVALID_HANDLE_VALUE)return;
    AnimationFlush169();CloseHandle(g_animation.file);g_animation.file=INVALID_HANDLE_VALUE;
    LARGE_INTEGER frequency={};QueryPerformanceFrequency(&frequency);
    Log("[ANIMATION IO169] cumulative bytes=%u writes=%llu write_ms=%.3f flush_ms=%.3f lost=%ld error=%ld; CPU file IO only, not GPU timing",g_animation.totalBytes,g_animation.writeCalls169,1000.0*g_animation.writeTicks169/frequency.QuadPart,1000.0*g_animation.flushTicks169/frequency.QuadPart,g_animation.lost,g_animation.error);
}
void AnimationWritten169(const AnimationRecord& r)
{
    if(r.kind==17&&r.state<sizeof(g_animation.savedModels)/sizeof(*g_animation.savedModels)&&!g_animation.savedModels[r.state])
    {
        ++g_animation.modelFamilies;
        for(unsigned i=0;i<sizeof(g_animation.savedModels)/sizeof(*g_animation.savedModels);++i)
            if(std::strcmp(fc3weapon::kProfileNames[r.state],fc3weapon::kProfileNames[i])==0)g_animation.savedModels[i]=true;
    }
    if(r.kind==1&&r.rawBytes==2304)
    {
        ++g_animation.poses;if(r.side==0)++g_animation.right;if(r.side==1)++g_animation.left;
        if(r.shader==0xA38F7CD233BCE12Bull&&r.vertexBytes==257896&&(r.start==18||r.start==126))++g_animation.shell;
    }
}
bool AnimationDrain169()
{
    if(!g_animation.writeCount169)return true;
    if(g_animation.file==INVALID_HANDLE_VALUE)return false;
    const DWORD bytes=g_animation.writeCount169*sizeof(AnimationRecord);DWORD written=0;
    LARGE_INTEGER begin={},end={};QueryPerformanceCounter(&begin);
    const BOOL ok=WriteFile(g_animation.file,g_animation.writeBatch169,bytes,&written,nullptr);
    QueryPerformanceCounter(&end);++g_animation.writeCalls169;g_animation.writeTicks169+=end.QuadPart-begin.QuadPart;
    if(!ok||written!=bytes){g_animation.error=ok?ERROR_WRITE_FAULT:GetLastError();g_animation.lost+=g_animation.writeCount169;g_animation.writeCount169=0;CloseHandle(g_animation.file);g_animation.file=INVALID_HANDLE_VALUE;return false;}
    g_animation.fileBytes+=bytes;g_animation.totalBytes+=bytes;
    for(UINT i=0;i<g_animation.writeCount169;++i)AnimationWritten169(g_animation.writeBatch169[i]);
    g_animation.writeCount169=0;return true;
}
bool AnimationFlush169()
{
    if(!AnimationDrain169()||g_animation.file==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER begin={},end={};QueryPerformanceCounter(&begin);
    const BOOL ok=FlushFileBuffers(g_animation.file);QueryPerformanceCounter(&end);g_animation.flushTicks169+=end.QuadPart-begin.QuadPart;
    if(!ok)g_animation.error=GetLastError();return ok!=FALSE;
}
bool AnimationWrite(const AnimationRecord& r)
{
    if(g_animation.file==INVALID_HANDLE_VALUE)return false;
    g_animation.writeBatch169[g_animation.writeCount169++]=r;
    // Model discovery and bow/tool sampling expose immediate saved counters.
    return (g_animation.writeCount169==8||r.kind==17||r.kind==19)?AnimationDrain169():true;
}
bool AnimationOpen()
{
    if(g_animation.file!=INVALID_HANDLE_VALUE)return true;
    const auto now=GetTickCount64();if(now<g_animation.retry)return false;g_animation.retry=now+500;
    if(g_animation.totalBytes>=1024u*1024*1024){g_animation.full=true;return false;} // 1 GB: 256 MB filled before the FAMAS reload (PID3384)
    wchar_t folder[MAX_PATH]={};
#ifdef FC3_AMMO_TEST
    GetTempPathW(MAX_PATH,folder);
#else
    wcscpy_s(folder,fc3::DataFolder());
#endif
    _snwprintf_s(g_animation.path,MAX_PATH,_TRUNCATE,L"%sfc3_animation_%lu_%llu_%u.bin",folder,GetCurrentProcessId(),now,++g_animation.segment);
    g_animation.file=CreateFileW(g_animation.path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(g_animation.file==INVALID_HANDLE_VALUE){g_animation.error=GetLastError();return false;}
    g_animation.fileBytes=0;AnimationRecord header;header.kind=0;
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);header.clock=frequency.QuadPart;
    HMODULE module=GetModuleHandleW(L"FC3_d3d11.dll");header.state=reinterpret_cast<UINT>(module);
    if(module){auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(module);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(module)+dos->e_lfanew);header.wrapper=nt->OptionalHeader.SizeOfImage;header.weapon=nt->FileHeader.TimeDateStamp;}
    if(!AnimationWrite(header))return false;
    for(UINT i=0;i<g_animation.ring.size();++i)
        if(!AnimationWrite(g_animation.ring[(g_animation.head+i)%g_animation.ring.size()]))return false;
    if(!AnimationFlush169()){AnimationClose();return false;}
    g_animation.error=0;g_animation.lastFlush=now;Log("[ANIMATION PROBE] durable timeline opened: %ls; raw/final poses and native event timing",g_animation.path);return true;
}
void AnimationStore(const AnimationRecord& r)
{
    if(GetTickCount64()<g_animation.until&&AnimationOpen())
    {
        AnimationWrite(r);
        if(g_animation.fileBytes>=32u*1024*1024){AnimationClose();g_animation.retry=0;}
    }
    if(g_animation.ring.size()<192)g_animation.ring.push_back(r);
    else{g_animation.ring[g_animation.head]=r;g_animation.head=(g_animation.head+1)%192;}
}
// Engine callbacks enqueue only: file IO stays on the render/Present thread.
void ProbeNativeAnimation(UINT state,UINT wrapper,UINT flag,int count,UINT caller,UINT phase)
{
    if(!g_animation.enabled)return;
    AnimationRecord r;r.kind=phase;LARGE_INTEGER clock;QueryPerformanceCounter(&clock);r.clock=clock.QuadPart;
    r.frame=g_frame;r.thread=GetCurrentThreadId();r.state=state;r.wrapper=wrapper;r.weapon=ReloadRead(wrapper,0x48);
    r.explicitCount=flag;r.requested=count;r.loaded=ReloadRead(r.weapon,0xcc);
    const UINT module=reinterpret_cast<UINT>(GetModuleHandleW(L"FC3_d3d11.dll"));r.callerRva=caller;
    r.flags=(ReloadContext(state,wrapper,r.weapon)?1u:0u)|(ReloadLocal(wrapper)?2u:0u)|(g_reload.mode?8u:0u)|(g_reloadPose.active?64u:0u);
    if(module)
    {
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(module);
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(module+dos->e_lfanew);
        if(caller>=module&&caller-module<nt->OptionalHeader.SizeOfImage){r.callerRva=caller-module;r.flags|=16;}
    }
    AmmoRead(state,r.stateWords,sizeof(r.stateWords));
    if(!TryAcquireSRWLockExclusive(&g_animationQueueLock)){InterlockedIncrement(&g_animation.lost);return;}
    if(g_animationQueueCount<64)g_animationQueue[g_animationQueueCount++]=r;else InterlockedIncrement(&g_animation.lost);
    ReleaseSRWLockExclusive(&g_animationQueueLock);
}
void TickAnimationProbe(bool gameplay)
{
    if(!g_animation.enabled)return;
    const auto now=GetTickCount64();g_animation.sample=gameplay&&((now<g_animation.until&&g_animation.until-now>8000)||now-g_animation.lastSample>=30);
    if(g_animation.sample)g_animation.lastSample=now;
    const auto c=fc3pad::Latest();UINT events=0;
    if(g_lastShotFrame>=0&&g_lastShotFrame!=g_animation.priorShot){events|=1;++g_animation.shots;}
    if(c.x&&!g_animation.priorX)events|=2;
    if(g_reload.requests!=g_animation.priorRequest)events|=4;
    if(HealProbeHoldEdge215(gameplay,c,now,g_frame)){events|=8;Log("[HEAL PROBE215] native hold-Y request; opening paired animation and first-person draw capture (not proof of healing)");}
    g_animation.priorShot=g_lastShotFrame;g_animation.priorX=c.x;g_animation.priorRequest=g_reload.requests;
    if(events){g_animation.sample=gameplay;g_animation.until=now+10000;AnimationRecord r;r.kind=2;r.event=events;AnimationStamp(r);AnimationStore(r);}
    for(;;)
    {
        AnimationRecord r;AcquireSRWLockExclusive(&g_animationQueueLock);
        if(!g_animationQueueCount){ReleaseSRWLockExclusive(&g_animationQueueLock);break;}
        r=g_animationQueue[0];--g_animationQueueCount;memmove(g_animationQueue,g_animationQueue+1,g_animationQueueCount*sizeof(r));ReleaseSRWLockExclusive(&g_animationQueueLock);
        g_animation.sample=gameplay;g_animation.until=now+10000;if(r.kind==4&&(r.flags&3)==3)++g_animation.native;AnimationStore(r);
    }
    if(g_animation.sample){AnimationRecord r;r.kind=3;AnimationStamp(r);r.state=g_reload.state;r.wrapper=g_reload.wrapper;r.weapon=g_reload.weapon;r.loaded=ReloadRead(r.weapon,0xcc);AmmoRead(r.state,r.stateWords,sizeof(r.stateWords));AnimationStore(r);}
    if(g_animation.file!=INVALID_HANDLE_VALUE&&now-g_animation.lastFlush>=1000)
    {if(!AnimationFlush169())AnimationClose();g_animation.lastFlush=now;}
    if(g_animation.file!=INVALID_HANDLE_VALUE&&now>=g_animation.until)AnimationClose();
}
void RecordAnimationDraw(ID3D11DeviceContext* context,const DrawCall& call)
{
    g_animation.pending=false;if(!g_animation.enabled||!g_animation.sample)return;
    AnimationRecord r;r.kind=1;AnimationStamp(r);r.drawKind=call.kind;r.count=call.a;r.start=call.b;r.base=call.d;
    ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);r.shader=ShaderHash(vs);SafeRelease(vs);
    ID3D11Buffer* vb=nullptr,*ib=nullptr;context->IAGetVertexBuffers(0,1,&vb,&r.stride,&r.offset);
    D3D11_BUFFER_DESC desc={};if(vb)vb->GetDesc(&desc);r.vertexBytes=desc.ByteWidth;r.vertex=reinterpret_cast<uintptr_t>(vb);SafeRelease(vb);
    DXGI_FORMAT format;UINT offset=0;context->IAGetIndexBuffer(&ib,&format,&offset);r.index=reinterpret_cast<uintptr_t>(ib);r.indexFormat=format;SafeRelease(ib);
    const auto key=r.shader^r.vertex^(static_cast<unsigned long long>(r.start)<<32);
    if(g_animation.seenFrame!=g_frame){g_animation.seenFrame=g_frame;g_animation.seenCount=0;}
    for(UINT i=0;i<g_animation.seenCount;++i)if(g_animation.seen[i]==key)return;
    if(g_animation.seenCount>=24)return;g_animation.seen[g_animation.seenCount++]=key;
    ID3D11Buffer* cb[4]={};context->VSGetConstantBuffers(0,4,cb);
    r.state=reinterpret_cast<UINT>(context);
    ReadCopy(cb[0],r.globals,sizeof(r.globals));r.rawBytes=ReadCopy(cb[3],r.raw,sizeof(r.raw));
    CameraSummary camera={};UINT bytes=sizeof(camera);
    if(cb[1]&&SUCCEEDED(cb[1]->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera))
    {memcpy(r.camera,camera.right,12);memcpy(r.camera+3,camera.up,12);memcpy(r.camera+6,camera.forward,12);memcpy(r.camera+9,camera.position,12);r.camera[12]=camera.xScale;r.camera[13]=camera.yScale;r.camera[14]=camera.zScale;r.camera[15]=camera.zOffset;}
    for(auto*& b:cb)SafeRelease(b);g_animation.draw=r;g_animation.pending=true;
}
void RecordAnimationFinal(const float* bones,int side)
{
    if(!g_animation.pending)return;g_animation.draw.side=side;g_animation.draw.finalBytes=2304;
    memcpy(g_animation.draw.finalPose,bones,2304);
}
void RecordAnimationPaletteSource217(ID3D11DeviceContext* context,const float* bones,UINT slot)
{
    if(!g_animation.pending||g_animation.draw.state!=reinterpret_cast<UINT>(context))return;
    memcpy(g_animation.draw.raw,bones,2304);g_animation.draw.rawBytes=2304;
    g_animation.draw.event=0x21700000u|slot;
}
void RecordAnimationSolid(ID3D11DeviceContext* context,const float* globals,UINT bytes)
{
    if(!g_animation.pending||g_animation.draw.state!=reinterpret_cast<UINT>(context)||!globals||bytes>352)return;
    g_animation.draw.finalBytes=bytes;g_animation.draw.side=-1;
    memcpy(g_animation.draw.finalPose,globals,bytes);
}
void FinishAnimationDraw()
{
    if(!g_animation.pending)return;AnimationStore(g_animation.draw);g_animation.pending=false;
}
bool AnimationProbeNote()
{
    if(!g_animation.enabled)return false;
    wchar_t note[256]={};
    if(g_animation.error)_snwprintf_s(note,256,_TRUNCATE,L"ANIMATION FILE ERROR %ld - RETRYING",g_animation.error);
    else if(g_animation.lost)_snwprintf_s(note,256,_TRUNCATE,L"ANIMATION CAPTURE - %ld EVENTS MISSED\nSaved poses remain; mention this when finished.",g_animation.lost);
    else if(g_animation.full)wcscpy_s(note,L"ANIMATION CAPTURE FULL\nExit so the saved timeline can be inspected.");
    else _snwprintf_s(note,256,_TRUNCATE,L"WEAPON WHEEL\nHold Y + left grip. Point the RIGHT stick\ntoward a weapon, then release both to equip.\n%ld model families / %ld poses saved.",g_animation.modelFamilies,g_animation.poses);
    fc3xr::SetTestNote(note);return true;
}
