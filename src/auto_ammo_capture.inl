// Autonomous, read-only recorder. No input sequence arms or cancels this worker.
struct AutoAmmoCandidate { UINT address=0,initial=0,words[5]={}; };
struct AutoAmmoState
{
    volatile LONG worker=0,stop=0,frame=0,gameplay=0,inputs=0,shot=-100000;
    volatile LONG packets=0,changes=0,transfers=0,candidates=0,error=0,scans=0;
    std::vector<AutoAmmoCandidate> tracked;
    uintptr_t ownerLo=0,ownerHi=0;
    ULONGLONG begun=0;
    UINT bytes=0,segment=0;
    wchar_t path[MAX_PATH]={};
} g_autoAmmo;
bool AutoAmmoLayout(const UINT* w)
{
    // Deliberately omit the old exact capacity, float, trailing flags and vtable.
    // These are hypotheses until actual transfers are observed.
    return w[1]>=2&&w[1]<=32&&w[0]<=w[1]&&w[3]>0&&w[3]<=100000&&w[4]==w[0]+w[3];
}
bool AutoAmmoWrite(HANDLE file,AmmoPacket& p)
{
    p.frame=static_cast<UINT>(InterlockedCompareExchange(&g_autoAmmo.frame,0,0));
    p.milliseconds=static_cast<UINT>(GetTickCount64()-g_autoAmmo.begun);
    DWORD n=0;
    if(!WriteFile(file,&p,sizeof(p),&n,nullptr)||n!=sizeof(p))
    {InterlockedExchange(&g_autoAmmo.error,GetLastError()?GetLastError():ERROR_WRITE_FAULT);return false;}
    g_autoAmmo.bytes+=n;InterlockedIncrement(&g_autoAmmo.packets);return true;
}
HANDLE AutoAmmoOpen()
{
    _snwprintf_s(g_autoAmmo.path,MAX_PATH,_TRUNCATE,L"%sfc3_ammo_auto_%lu_%llu_%u.bin",AmmoFolder(),GetCurrentProcessId(),g_autoAmmo.begun,g_autoAmmo.segment++);
    HANDLE file=CreateFileW(g_autoAmmo.path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){InterlockedExchange(&g_autoAmmo.error,GetLastError());return file;}
    g_autoAmmo.bytes=0;
    AmmoPacket start;start.type=5;start.initial=0x4155544f;start.value=1;
    if(!AutoAmmoWrite(file,start)||!FlushFileBuffers(file)){InterlockedExchange(&g_autoAmmo.error,GetLastError()?GetLastError():ERROR_WRITE_FAULT);CloseHandle(file);return INVALID_HANDLE_VALUE;}
    InterlockedExchange(&g_autoAmmo.error,0);
    const wchar_t* names[]={nullptr,L"FC3_d3d11.dll",L"FC3.dll"};
    for(auto* name:names)
    {
        HMODULE module=GetModuleHandleW(name);if(!module)continue;
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(module);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>65536)continue;
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(module)+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE)continue;
        AmmoPacket p;p.type=4;p.address=reinterpret_cast<UINT>(module);p.initial=nt->OptionalHeader.SizeOfImage;p.value=nt->FileHeader.TimeDateStamp;
        const wchar_t* label=name?name:L"main.exe";memcpy(p.nearby,label,(wcslen(label)+1)*sizeof(wchar_t));AutoAmmoWrite(file,p);
    }
    Log("[AMMO AUTO] recording file opened and flushed: %ls",g_autoAmmo.path);return file;
}
void AutoAmmoSnapshot(HANDLE file,const AutoAmmoCandidate& c,UINT type)
{
    AmmoPacket p;p.type=type;p.address=c.address;p.initial=c.initial;p.value=c.words[0];
    MEMORY_BASIC_INFORMATION m={};
    if(VirtualQuery(reinterpret_cast<void*>(c.address),&m,sizeof(m))&&AmmoReadable(m))
    {
        const uintptr_t lo=reinterpret_cast<uintptr_t>(m.BaseAddress),hi=lo+m.RegionSize;
        const uintptr_t first=(std::max)(lo,static_cast<uintptr_t>(c.address>128?c.address-128:0));
        const SIZE_T size=(std::min)(static_cast<SIZE_T>(sizeof(p.nearby)),static_cast<SIZE_T>(hi-first));
        if(AmmoRead(static_cast<UINT>(first),p.nearby,size)){p.nearBase=static_cast<UINT>(first);p.nearBytes=static_cast<UINT>(size);}
    }
    AutoAmmoWrite(file,p);
}
bool AutoAmmoOutside(uintptr_t address,uintptr_t stackLo,uintptr_t stackHi)
{
    const uintptr_t lo=reinterpret_cast<uintptr_t>(g_autoAmmo.tracked.data()),hi=lo+g_autoAmmo.tracked.capacity()*sizeof(AutoAmmoCandidate);
    return !(address>=g_autoAmmo.ownerLo&&address<g_autoAmmo.ownerHi)&&!(address>=stackLo&&address<stackHi)&&!(address>=lo&&address<hi);
}
void AutoAmmoScan(HANDLE file)
{
    constexpr size_t limit=8192;
    g_autoAmmo.tracked.clear();g_autoAmmo.tracked.reserve(limit);
    UINT words[16388]={};MEMORY_BASIC_INFORMATION stack={};VirtualQuery(words,&stack,sizeof(stack));
    uintptr_t stackLo=reinterpret_cast<uintptr_t>(stack.AllocationBase),stackHi=stackLo;
    MEMORY_BASIC_INFORMATION range={};
    while(VirtualQuery(reinterpret_cast<void*>(stackHi),&range,sizeof(range))&&range.AllocationBase==stack.AllocationBase)
    {const uintptr_t next=reinterpret_cast<uintptr_t>(range.BaseAddress)+range.RegionSize;if(next<=stackHi)break;stackHi=next;}
    SYSTEM_INFO si={};GetSystemInfo(&si);
    uintptr_t address=reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress),end=reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);
    const auto start=GetTickCount64();ULONGLONG bytes=0;bool capped=false;
    while(address<end&&!InterlockedCompareExchange(&g_autoAmmo.stop,0,0))
    {
        MEMORY_BASIC_INFORMATION m={};if(!VirtualQuery(reinterpret_cast<void*>(address),&m,sizeof(m)))break;
        const uintptr_t base=reinterpret_cast<uintptr_t>(m.BaseAddress),last=base+m.RegionSize;if(last<=address)break;
        if(AmmoReadable(m)&&(m.Type==MEM_PRIVATE||m.Type==MEM_IMAGE))
        for(uintptr_t chunk=base;chunk<last;chunk+=65536)
        {
            if(InterlockedCompareExchange(&g_autoAmmo.stop,0,0))break;
            if(GetTickCount64()-start>8000){capped=true;break;}
            if(!AutoAmmoOutside(chunk,stackLo,stackHi))continue;
            const SIZE_T size=(std::min)(static_cast<SIZE_T>(sizeof(words)),static_cast<SIZE_T>(last-chunk));
            if(!AmmoRead(static_cast<UINT>(chunk),words,size))continue;bytes+=size;
            // 16-byte overlap retains layouts crossing a 64 KiB read boundary.
            for(SIZE_T at=0;at<65536&&at+20<=size;at+=4)
            {
                const uintptr_t location=chunk+at;
                if(!AutoAmmoLayout(words+at/4)||!AutoAmmoOutside(location,stackLo,stackHi))continue;
                if(g_autoAmmo.tracked.size()==limit){capped=true;break;}
                AutoAmmoCandidate c;c.address=static_cast<UINT>(location);c.initial=words[at/4];memcpy(c.words,words+at/4,sizeof(c.words));
                g_autoAmmo.tracked.push_back(c);AutoAmmoSnapshot(file,c,1);
            }
            if(capped)break;
        }
        if(capped)break;address=last;
    }
    InterlockedExchange(&g_autoAmmo.candidates,static_cast<LONG>(g_autoAmmo.tracked.size()));InterlockedIncrement(&g_autoAmmo.scans);
    AmmoPacket summary;summary.type=6;summary.initial=static_cast<UINT>(g_autoAmmo.tracked.size());summary.value=capped?1:0;AutoAmmoWrite(file,summary);
    Log("[AMMO AUTO] scan %ld: %u live layout hypotheses, %.0f MiB, %.2f seconds%s",g_autoAmmo.scans,summary.initial,bytes/1048576.0,(GetTickCount64()-start)/1000.0,capped?" (coverage capped)":"");
}
void AutoAmmoSample(HANDLE file,bool full)
{
    UINT matches=0,address=0,loaded=0,reserve=0;
    for(auto& c:g_autoAmmo.tracked)
    {
        UINT w[5]={};if(!AmmoRead(c.address,w,sizeof(w)))continue;
        const bool changed=memcmp(w,c.words,sizeof(w))!=0;
        if(AutoAmmoLayout(w)&&w[0]==c.words[0]+1&&c.words[3]>0&&w[3]==c.words[3]-1)
        {++matches;address=c.address;loaded=w[0];reserve=w[3];InterlockedIncrement(&g_autoAmmo.transfers);}
        memcpy(c.words,w,sizeof(w));
        if(changed){InterlockedIncrement(&g_autoAmmo.changes);AutoAmmoSnapshot(file,c,3);}
        else if(full)AutoAmmoSnapshot(file,c,2);
    }
    if(matches)AmmoOwnerObserved(address,loaded,reserve,matches);
}
DWORD WINAPI AutoAmmoWorker(LPVOID)
{
    g_autoAmmo.begun=GetTickCount64();HANDLE file=INVALID_HANDLE_VALUE;
    ULONGLONG nextScan=0,nextFlush=0,nextFull=0;
    LONG lastShot=-100000,lastInputs=-1;
    while(!InterlockedCompareExchange(&g_autoAmmo.stop,0,0))
    {
        if(file==INVALID_HANDLE_VALUE)
        {file=AutoAmmoOpen();if(file==INVALID_HANDLE_VALUE){Sleep(500);continue;}nextFlush=0;}
        const auto now=GetTickCount64();
        const LONG shot=InterlockedCompareExchange(&g_autoAmmo.shot,0,0),inputs=InterlockedCompareExchange(&g_autoAmmo.inputs,0,0);
        if(shot!=lastShot||inputs!=lastInputs||now>=nextFlush)
        {
            AmmoPacket event;event.type=5;event.initial=static_cast<UINT>(shot);event.value=static_cast<UINT>(inputs);
            event.nearby[0]=static_cast<UINT>(g_autoAmmo.gameplay);event.nearby[1]=static_cast<UINT>(g_autoAmmo.changes);event.nearby[2]=static_cast<UINT>(g_autoAmmo.transfers);
            AutoAmmoWrite(file,event);lastShot=shot;lastInputs=inputs;
        }
        // No controller, quiet-time, weapon-draw or READY gate. Retry discovery
        // automatically as the live game creates/replaces weapon objects.
        if((g_autoAmmo.gameplay||shot>=0)&&now>=nextScan)
        {AutoAmmoScan(file);nextScan=GetTickCount64()+10000;nextFull=0;}
        AutoAmmoSample(file,now>=nextFull);if(now>=nextFull)nextFull=now+10000;
        if(now>=nextFlush)
        {
            if(!FlushFileBuffers(file))InterlockedExchange(&g_autoAmmo.error,GetLastError());
            nextFlush=GetTickCount64()+1000;
        }
        if(g_autoAmmo.error||g_autoAmmo.bytes>=16*1024*1024)
        {FlushFileBuffers(file);CloseHandle(file);file=INVALID_HANDLE_VALUE;}
        Sleep(100);
    }
    if(file!=INVALID_HANDLE_VALUE){FlushFileBuffers(file);CloseHandle(file);}
    InterlockedExchange(&g_autoAmmo.worker,2);return 0;
}
bool AutoAmmoDiagnostics314(){wchar_t value[4]={};return GetEnvironmentVariableW(L"FC3_AMMO_DIAGNOSTICS",value,4)==1&&value[0]==L'1';}
void TickAutoAmmoCapture(bool gameplay)
{
    // Obsolete full-process discovery is research-only, never normal gameplay.
    if(!AutoAmmoDiagnostics314())return;
    const auto c=fc3pad::Latest();
    InterlockedExchange(&g_autoAmmo.frame,g_frame);InterlockedExchange(&g_autoAmmo.gameplay,gameplay?1:0);
    InterlockedExchange(&g_autoAmmo.shot,g_lastShotFrame);
    const LONG input=(c.active?1:0)|(c.x?2:0)|(c.rightTrigger>.3f?4:0)|(c.rightStickClick?8:0)|(g_twoHand?16:0)|(g_chambered?32:0);
    InterlockedExchange(&g_autoAmmo.inputs,input);
    if(InterlockedCompareExchange(&g_autoAmmo.worker,1,0)!=0)return;
    HMODULE owner=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&g_autoAmmo),&owner);
    if(owner){auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(owner);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(owner)+dos->e_lfanew);g_autoAmmo.ownerLo=reinterpret_cast<uintptr_t>(owner);g_autoAmmo.ownerHi=g_autoAmmo.ownerLo+nt->OptionalHeader.SizeOfImage;}
    HANDLE thread=CreateThread(nullptr,0,AutoAmmoWorker,nullptr,0,nullptr);
    if(thread){SetThreadPriority(thread,THREAD_PRIORITY_BELOW_NORMAL);CloseHandle(thread);Log("[AMMO AUTO] autonomous recorder started; no setup sequence");}
    else {InterlockedExchange(&g_autoAmmo.error,GetLastError());InterlockedExchange(&g_autoAmmo.worker,0);}
}
