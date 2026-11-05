void TestAutoAmmoCapture()
{
    GetTempPathW(MAX_PATH,g_ammoTestFolder);g_autoAmmo=AutoAmmoState{};g_ammoOwner=AmmoOwnerState{};
    fc3pad::Controllers c={};fc3pad::Publish(c);g_lastShotFrame=-100000;g_visibleWeaponDrawsLastFrame=0;
    auto wait=[](auto predicate,const char* label){const auto start=GetTickCount64();while(!predicate()){Check(GetTickCount64()-start<15000,label);Sleep(20);}};
    SetEnvironmentVariableW(L"FC3_AMMO_DIAGNOSTICS",nullptr);TickAutoAmmoCapture(true);
    Check(g_autoAmmo.worker==0&&g_autoAmmo.packets==0&&g_autoAmmo.scans==0,"314 shipping default starts no broad ammo scanner or recording worker");
    SetEnvironmentVariableW(L"FC3_AMMO_DIAGNOSTICS",L"1");
    TickAutoAmmoCapture(false);
    wait([](){return g_autoAmmo.packets>=1&&!g_autoAmmo.error;},"autonomous file opens with no gameplay, tracking, controls or READY");
    HANDLE live=CreateFileW(g_autoAmmo.path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
    Check(live!=INVALID_HANDLE_VALUE,"capture file readable while recorder still runs");
    AmmoPacket header;DWORD n=0;Check(ReadFile(live,&header,sizeof(header),&n,nullptr)&&n==sizeof(header)&&header.magic==0x414d5031&&header.type==5,"durable header exists before first gameplay frame");CloseHandle(live);
    auto* page=static_cast<UINT*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"autonomous fixture allocation");
    const UINT shape[5]={1,7,0x12345678,997,998};memcpy(page+32,shape,sizeof(shape));
    Check(AutoAmmoLayout(page+32),"loose discovery accepts changed float/trailing layout and one shell");
    c.x=true;c.rightTrigger=1;fc3pad::Publish(c);TickAutoAmmoCapture(true);
    wait([](){return g_autoAmmo.scans>=1;},"real full-process autonomous scan completes while reload and trigger held");
    const UINT address=reinterpret_cast<UINT>(page+32);bool found=false;for(const auto& candidate:g_autoAmmo.tracked)found|=candidate.address==address;
    Check(found,"real autonomous scanner finds relocated one-shell layout without setup sequence");
    AwaitAmmoFixture338(g_autoAmmo.path,reinterpret_cast<UINT>(page+32),1,997,"autonomous initial sample recorded");volatile UINT* observed=page;
    observed[32]=0;observed[36]=997;g_lastShotFrame=100;c.active=false;fc3pad::Publish(c);TickAutoAmmoCapture(false);AwaitAmmoFixture338(g_autoAmmo.path,reinterpret_cast<UINT>(page+32),0,997,"autonomous shot-to-empty recorded");
    observed[32]=1;observed[35]=996;AwaitAmmoFixture338(g_autoAmmo.path,reinterpret_cast<UINT>(page+32),1,996,"autonomous first insertion recorded");wait([](){return InterlockedCompareExchange(&g_autoAmmo.transfers,0,0)>=1;},"autonomous first conserved transfer observed");
    observed[32]=2;observed[35]=995;AwaitAmmoFixture338(g_autoAmmo.path,reinterpret_cast<UINT>(page+32),2,995,"autonomous second insertion recorded");wait([](){return InterlockedCompareExchange(&g_autoAmmo.transfers,0,0)>=2;},"autonomous second conserved transfer observed");
    Check(g_autoAmmo.worker==1&&g_autoAmmo.transfers>=2,"empty gun and tracking loss preserve subsequent reload transfers");
    wait([](){return g_autoAmmo.packets>10;},"autonomous observations saved");
    InterlockedExchange(&g_autoAmmo.stop,1);wait([](){return g_autoAmmo.worker==2;},"autonomous worker stops cooperatively");
    live=CreateFileW(g_autoAmmo.path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);Check(live!=INVALID_HANDLE_VALUE,"complete autonomous trace readable");
    bool values[3]={};UINT targetChanges=0,events=0;
    while(ReadFile(live,&header,sizeof(header),&n,nullptr)&&n==sizeof(header))
    {
        Check(header.magic==0x414d5031&&header.version==1,"every autonomous packet has expected schema");
        if(header.type==5)++events;
        if(header.address==address&&header.type==3)
        {++targetChanges;if(header.value<3)values[header.value]=true;Check(header.nearBytes==260,"autonomous reload saves nearby fields");}
    }
    CloseHandle(live);Check(values[0]&&values[1]&&values[2]&&targetChanges>=3&&events>=2,"actual file contains shot to zero and both reload commits despite held buttons/tracking loss");
    Check(page[32]==2&&page[33]==7&&page[35]==995&&page[36]==997,"autonomous recorder does not alter game counters");
    SetEnvironmentVariableW(L"FC3_AMMO_DIAGNOSTICS",nullptr);
    VirtualFree(page,0,MEM_RELEASE);g_autoAmmo=AutoAmmoState{};g_ammoOwner=AmmoOwnerState{};
    std::puts("PASS: autonomous durable capture before gameplay/READY, real relocated-counter discovery, held inputs, shot to empty, tracking loss, both reload commits and readable saved file");
}
