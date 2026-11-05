// Native strategy constructor13C734C stores root at strategy+24; root's
// first word is that strategy (29AF35). The13110A0 tick receives a subobject
// whose +48 is the same root. Native12D86CE obtains melee config with
// EAX=root+BE4 ->1316990. Re-resolve here instead of using a destroyed
// strategy from a prior weapon. Every candidate must belong to the current
// local player's64-bit entity handle. Never synthesize a source swipe.
bool MacheteRecoverySignatures123(const BYTE* base)
{
    const BYTE local[]={0x55,0x8b,0xec,0x51,0xa1};
    const BYTE config[]={0xe8,0xcb,0xe7,0xff,0xff,0x85,0xc0};
    return !memcmp(base+0xc80190,local,sizeof(local))&&!memcmp(base+0x1316990,config,sizeof(config));
}
UINT MacheteNativeConfig123(UINT root)
{
    const UINT fn=g_macheteBladeContact.engine+0x1316990;UINT result=0;
    __asm mov eax,root
    __asm add eax,0be4h
    __asm call fn
    __asm mov result,eax
    return result;
}
bool MacheteLocalStrategy123(UINT strategy)
{
    const UINT base=g_macheteBladeContact.engine;
    const UINT entity=MacheteNativeRead(MacheteNativeRead(strategy,4),12);
    const UINT manager=MacheteNativeRead(base+0x2829474,0),count=MacheteNativeRead(manager,12);
    if(!entity||!manager||!count||count>4)return false;
    UINT handle[2]={~0u,~0u};
    if(reinterpret_cast<UINT*(__cdecl*)(UINT*)>(base+0xc80190)(handle)!=handle||handle[0]==~0u||handle[1]==~0u)return false;
    return MacheteNativeRead(entity,8)==handle[0]&&MacheteNativeRead(entity,12)==handle[1];
}
void MacheteRefreshController123(UINT receiver)
{
    auto& b=g_macheteBladeContact;
    if(!b.engine||!b.enabled||!MacheteHeld()||!g_inGameplay||fc3pad::GameplayInputBlocked())return;
    __try {
        const UINT root=MacheteNativeRead(receiver,0x48),strategy=MacheteNativeRead(root,0);
        if(!root||!strategy||MacheteNativeRead(strategy,0)!=b.engine+0x1f99568||MacheteNativeRead(strategy,0x24)!=root)return;
        if(!MacheteLocalStrategy123(strategy))return;
        if(strategy==b.strategy&&b.config)return;
        const UINT config=MacheteNativeConfig123(root);
        const UINT before=b.strategy;MacheteBladeLatch(strategy,config);
        if(b.strategy==strategy&&b.config==config&&config){
            b.havePrev=false;MacheteCutPause121();++b.recovered123;
            Log("[MACHETE RECOVERY123] local controller %08x -> %08x config %08x; native tick receiver %08x",before,strategy,config,receiver);
        }else ++b.recoveryReject123;
    }__except(EXCEPTION_EXECUTE_HANDLER){++b.recoveryReject123;}
}
