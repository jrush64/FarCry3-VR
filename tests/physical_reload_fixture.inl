static UINT g_reloadFixtureCalls=0,g_reloadFixtureFlag=0;static int g_reloadFixtureCount=0;
static UINT g_reloadFixtureHud=0,g_reloadFixtureWeapon=0;
static UINT g_widgetFixtureRegisters[4]={},g_widgetFixtureFlags=0;
static float g_widgetFixtureScalar=3.25f,g_widgetFixtureScalarSaved=0;
__declspec(naked) void FixtureWidgetOriginal()
{
    __asm {
        mov g_widgetFixtureRegisters[0],edi
        mov g_widgetFixtureRegisters[4],esi
        mov g_widgetFixtureRegisters[8],eax
        mov g_widgetFixtureRegisters[12],ecx
        movss g_widgetFixtureScalarSaved,xmm0
        pushfd
        pop eax
        mov g_widgetFixtureFlags,eax
        ret
    }
}
__declspec(naked) void InvokeFixtureWidget(UINT,UINT)
{
    __asm {
        pushad
        mov edi,[esp+36]
        mov esi,[esp+40]
        mov eax,12345678h
        mov ecx,23456789h
        movss xmm0,g_widgetFixtureScalar
        stc
        call ReloadWidgetBoundary
        popad
        ret
    }
}
DWORD WINAPI FixtureOtherWidgetThread(LPVOID)
{ObserveAmmoWidget(g_reload.wrapper,g_reloadFixtureHud);return 0;}
void __stdcall FixtureNativeReload(UINT state,UINT,UINT flag,int count)
{
    ++g_reloadFixtureCalls;g_reloadFixtureFlag=flag;g_reloadFixtureCount=count;
    auto* p=reinterpret_cast<UINT*>(g_reloadFixtureWeapon);auto* h=reinterpret_cast<UINT*>(g_reloadFixtureHud);
    if(!*reinterpret_cast<BYTE*>(state+8)||p[0xcc/4]>=7)return;
    const UINT n=flag?static_cast<UINT>(count):1;
    p[0xcc/4]+=n;h[0x134/4]=p[0xcc/4];h[0x140/4]-=n;h[0x144/4]=h[0x134/4]+h[0x140/4];
    *reinterpret_cast<BYTE*>(state+8)=0;
}
void TestPhysicalReload()
{
    g_physicalReloadEnabled=true;
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,24576,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"reload context fixture");
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384;
    *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
    *reinterpret_cast<UINT*>(weapon+0xcc)=3;*reinterpret_cast<UINT*>(hud+0x134)=3;*reinterpret_cast<UINT*>(hud+0x138)=7;*reinterpret_cast<UINT*>(hud+0x140)=10;*reinterpret_cast<UINT*>(hud+0x144)=13;
    g_reload=PhysicalReloadState{};g_reload.installed=true;g_reload.shotgunFrame=g_frame;g_nativeReload=FixtureNativeReload;g_reloadFixtureCalls=0;g_reloadFixtureWeapon=weapon;g_reloadFixtureHud=hud;
    ObserveAmmoWidget(wrapper,hud);*reinterpret_cast<BYTE*>(state+8)=1;ReloadBoundary(state,wrapper,0,-1);
    Check(g_reload.ready&&g_reload.weapon==weapon&&ReloadRead(weapon,0xcc)==4,"actual typed boundary forwards ordinary reload and validates one-shell context");
    g_reload.mode=1;g_reload.deadline=GetTickCount64()+10000;*reinterpret_cast<BYTE*>(state+8)=1;ReloadBoundary(state,wrapper,0,-1);
    Check(g_reload.pending&&g_reloadFixtureCalls==1&&ReloadRead(weapon,0xcc)==4,"physical mode defers auto-fill without changing native loaded/reserve");
    g_reload.credit=1;
    HANDLE wrongThread=CreateThread(nullptr,0,FixtureOtherWidgetThread,nullptr,0,nullptr);Check(wrongThread!=nullptr,"other widget thread");WaitForSingleObject(wrongThread,5000);CloseHandle(wrongThread);
    Check(g_reload.credit==1&&g_reloadFixtureCalls==1,"deferred native transfer cannot execute on a different thread");
    ObserveAmmoWidget(wrapper,hud);
    Check(g_reloadFixtureCalls==2&&g_reloadFixtureFlag==1&&g_reloadFixtureCount==1&&ReloadRead(weapon,0xcc)==5&&ReloadRead(hud,0x140)==8&&g_reload.commits==1,"one physical credit consumes one native shell transfer on original callback thread");
    ObserveAmmoWidget(wrapper,hud);Check(g_reloadFixtureCalls==2,"same insertion cannot replay twice");
    // Receiver gesture is exercised through the real production tick.
    g_reload.mode=0;g_reload.held=false;g_reload.grip=false;g_reload.pending=0;g_reload.credit=0;g_twoHand=false;
    const bool handR=g_handRightValid,handL=g_handLeftValid;g_handRightValid=g_handLeftValid=true;
    float priorBody[2][16];memcpy(priorBody,g_handBody,sizeof(priorBody));double oldOffsets[2][3];memcpy(oldOffsets,g_wristOffset,sizeof(oldOffsets));memset(g_wristOffset,0,sizeof(g_wristOffset));
    for(auto& pose:g_handBody){memset(pose,0,sizeof(pose));pose[0]=pose[5]=pose[10]=pose[15]=1;}
    g_handBody[0][3]=.15f;g_handBody[0][7]=-.10f;g_handBody[0][11]=.4f;g_handBody[1][3]=-.3f;g_handBody[1][7]=-.55f;g_handBody[1][11]=.2f;
    g_reloadShell.parts[0].ready=g_reloadShell.parts[1].ready=true;
    fc3pad::Controllers c={};c.active=true;c.leftGrip=1;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(g_reload.held&&g_reload.mode&&!fc3pad::Latest().x&&g_reload.requests==0,"waist pickup holds a shell without requesting native reload");
    TickPhysicalReload(true);Check(!g_reload.credit&&g_reload.requests==0,"stationary held grip sends no reload input and grants no ammo");
    g_reloadContact.hand=g_reloadContact.port=true;g_reloadContact.handFrame=g_reloadContact.portFrame=g_frame;g_reloadContact.shellBody={.15,-.16,.5};g_reloadContact.portBody={.15,-.16,.5};
    g_handBody[1][3]=.15f;g_handBody[1][7]=-.16f;g_handBody[1][11]=.5f;c.leftGrip=0;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(!g_reload.held&&g_reload.credit==1&&g_reload.requests==1,"receiver release grants exactly one pending insertion");
    *reinterpret_cast<BYTE*>(state+8)=1;ReloadBoundary(state,wrapper,0,-1);Check(g_reload.commits==2&&ReloadRead(weapon,0xcc)==6,"gesture reaches real boundary and consumes one credit");
    g_reload.held=true;g_reloadContact.portFrame=g_frame-3;TickPhysicalReload(true);
    Check(!g_reload.held&&!g_reload.credit&&g_reload.requests==1,"release without fresh receiver geometry cannot grant a shell");
    const LONG keepMode=g_reload.mode,keepCredit=g_reload.credit,keepPending=g_reload.pending;c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);Check(g_reload.mode==keepMode&&g_reload.credit==keepCredit&&g_reload.pending==keepPending,"ordinary interaction X cannot cancel physical reload ownership");
    g_reload.mode=1;g_reload.credit=1;c.active=false;fc3pad::Publish(c);TickPhysicalReload(false);Check(!g_reload.mode&&!g_reload.credit,"tracking loss restores ordinary reload without retained credit");
    // Rejected native transfer cannot keep the game trapped in physical mode.
    g_reload.mode=1;g_reload.credit=1;g_reload.deadline=GetTickCount64()+10000;*reinterpret_cast<BYTE*>(state+8)=0;ReloadBoundary(state,wrapper,0,-1);
    Check(g_reload.failed&&!g_reload.mode&&ReloadRead(weapon,0xcc)==6,"native no-op restores vanilla path and reports failure");
    g_nativeAmmoWidget=FixtureWidgetOriginal;InvokeFixtureWidget(wrapper,hud);
    Check(g_widgetFixtureRegisters[0]==wrapper&&g_widgetFixtureRegisters[1]==hud&&g_widgetFixtureRegisters[2]==0x12345678&&g_widgetFixtureRegisters[3]==0x23456789&&(g_widgetFixtureFlags&1)&&g_widgetFixtureScalarSaved==g_widgetFixtureScalar,"real naked widget bridge restores integer/SSE registers, flags and stack before trampoline");
    float shellBones[576]={};for(int b=0;b<48;++b){shellBones[b*12]=shellBones[b*12+5]=shellBones[b*12+10]=1;shellBones[b*12+3]=1;shellBones[b*12+7]=2;shellBones[b*12+11]=3;}
    float priorHand[16];memcpy(priorHand,g_handLeft,sizeof(priorHand));memcpy(g_handLeft,g_handBody[1],sizeof(priorHand));
    Mat4 identity={};identity.m[0]=identity.m[5]=identity.m[10]=identity.m[15]=1;g_reload.held=true;g_reloadContact.hand=true;g_reloadContact.handFrame=g_frame;g_reloadContact.view=identity;MoveReloadShell(shellBones,identity);
    const V3 shellCentre=Apply(BoneMatrix(shellBones,0),kCapturedShellCentre),palm=Apply(kCapturedShellToHand,kCapturedShellCentre);
    Check(Length(Sub(shellCentre,palm))<1e-6,"shell preserves captured native hand-relative placement despite offset mesh origin");
    g_reload.held=false;MoveReloadShell(shellBones,identity);Check(shellBones[7]<-99,"unheld native loading shell hidden only during physical mode");memcpy(g_handLeft,priorHand,sizeof(priorHand));
    memcpy(g_handBody,priorBody,sizeof(priorBody));memcpy(g_wristOffset,oldOffsets,sizeof(oldOffsets));g_handRightValid=handR;g_handLeftValid=handL;
    g_reloadShell.parts[0].ready=g_reloadShell.parts[1].ready=false;
    g_physicalReloadEnabled=false;fc3pad::SetShotgunHandling(false);g_reloadContact=ReloadHandContact{};g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};g_nativeReload=nullptr;g_nativeAmmoWidget=nullptr;fc3pad::SuppressLeftGrip(false);VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: native reload ABI/context, normal passthrough, deferred auto-fill, one credit/one shell, real waist/receiver gesture, no duplicate commit, X/tracking recovery and native no-op fallback");
}

