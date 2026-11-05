// 210: God mode at Jason's native damage entry (FC3_d3d11 E541594E, RVAs).
// vtable 2008E38 is CFCXCountersComponentPlayerSP: slot+4 -> 44E8B0 jmp 3B2960,
// the descriptor named at 1FE16BC whose parent is the Player descriptor 3ACC50.
// Slot+158 = 10D0F40, bool __thiscall ApplyDamage(float), ret4. Its first act is
// IsInvulnerable() (slot+80 = 10CE420); when true it returns false at 10D150A
// without touching health. 10CE420 is true for cheat GodMode==1 (10CE4D7), never
// for 2: mode 2 lets the hit land and refills only on a later health change
// (10CE2B6, 1988C6C), which is why 123's mode 2 still lost health.
// This bridge repeats the native early-out for the PlayerSP component only.
static void* g_applyDamageOriginal210=nullptr;
static volatile LONG g_godOn210=0,g_godBlocked210=0,g_godLogged210=0;
static UINT g_godVtable210=0;static ULONGLONG g_godLogMs210=0;
static bool g_godInstalled210=false,g_godAttempted210=false;
bool GodBlock210(UINT self)
{
    if(!g_godOn210||!g_godVtable210||!self)return false;
    UINT vtable=0;
    __try{vtable=*reinterpret_cast<const UINT*>(self);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    if(vtable!=g_godVtable210)return false;
    const LONG blocked=InterlockedIncrement(&g_godBlocked210);const ULONGLONG now=GetTickCount64();
    if(!g_godLogMs210||now-g_godLogMs210>=60000){
        g_godLogMs210=now;const LONG last=InterlockedExchange(&g_godLogged210,blocked);
        fc3::Log("[GOD210] blocked %ld hits (%ld since last line); Jason's ApplyDamage took the native invulnerable exit",blocked,blocked-last);
    }
    return true;
}
// Native ABI: ECX=component, one float on the stack, ret4, result in AL.
// Every register, EFLAGS, x87 and SSE state is preserved around the decision;
// a refused hit returns false exactly like the native early-out at 10D150A.
__declspec(naked) void ApplyDamageBoundary210()
{
    __asm {
        pushfd
        pushad
        mov eax,esp
        sub esp,544
        and esp,0fffffff0h
        mov [esp+512],eax
        fxsave [esp]
        push ecx
        call GodBlock210
        add esp,4
        fxrstor [esp]
        mov esp,[esp+512]
        test al,al
        jnz blocked
        popad
        popfd
        jmp dword ptr [g_applyDamageOriginal210]
    blocked:
        popad
        popfd
        xor eax,eax
        ret 4
    }
}
bool ValidateGodMode210(const BYTE* base)
{
    const UINT b=reinterpret_cast<UINT>(base);
    const BYTE apply[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x5c,0x53,0x8b,0xd9,0x8b,0x03,0x8b,0x90,0x80,0x00,0x00,0x00,0x56,0x57,0xff,0xd2,0x84,0xc0,0x0f,0x85,0xaa,0x05,0x00,0x00};
    const BYTE nativeExit[]={0x5f,0x5e,0x32,0xc0,0x5b,0x8b,0xe5,0x5d,0xc2,0x04,0x00};
    const BYTE invulnerable[]={0x55,0x8b,0xec,0x83,0xec,0x0c,0x53,0x56,0x57,0x8b,0xf9};
    const BYTE modeOne[]={0x83,0xf8,0x01,0x74,0x76};
    const BYTE type[]={0xe9,0xab,0x40,0xf6,0xff};
    __try {
        const UINT* vtable=reinterpret_cast<const UINT*>(base+0x2008e38);
        return !memcmp(base+0x10d0f40,apply,sizeof(apply))&&!memcmp(base+0x10d150a,nativeExit,sizeof(nativeExit))&&
            !memcmp(base+0x10ce420,invulnerable,sizeof(invulnerable))&&!memcmp(base+0x10ce4d7,modeOne,sizeof(modeOne))&&
            vtable[0x158/4]==b+0x10d0f40&&vtable[0x80/4]==b+0x10ce420&&vtable[1]==b+0x44e8b0&&
            !memcmp(base+0x44e8b0,type,sizeof(type))&&base[0x3b2979]==0x68&&
            *reinterpret_cast<const UINT*>(base+0x3b297a)==b+0x1fe16bc&&!memcmp(base+0x1fe16bc,"CFCXCountersComponentPlayerSP",30);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool InstallGodMode210()
{
    if(g_godInstalled210)return true;if(!g_engine||g_godAttempted210)return false;g_godAttempted210=true;
    BYTE* base=reinterpret_cast<BYTE*>(g_engine);
    if(!ValidateGodMode210(base)){fc3::Log("[GOD210] engine signatures differ; damage hook NOT installed (native cheat_GodMode 1 still applies)");return false;}
    g_godVtable210=g_engine+0x2008e38;
    if(MH_CreateHook(base+0x10d0f40,reinterpret_cast<void*>(ApplyDamageBoundary210),&g_applyDamageOriginal210)!=MH_OK||MH_EnableHook(base+0x10d0f40)!=MH_OK){
        g_godVtable210=0;fc3::Log("[GOD210] MinHook refused 10D0F40; damage hook NOT installed (native cheat_GodMode 1 still applies)");return false;
    }
    g_godInstalled210=true;fc3::Log("[GOD210] verified PlayerSP ApplyDamage 10D0F40 hooked; Jason's hits refused while God mode is on, everyone else native");return true;
}
namespace fc3aim {
void SetGodMode210(bool on){InterlockedExchange(&g_godOn210,on?1:0);}
}
