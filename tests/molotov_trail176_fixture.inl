unsigned trailCalls176=0;
void __stdcall TrailCreateFixture176(UINT){++trailCalls176;}
void TestMolotovTrail176(const Prepared143& target,UINT component){
 const auto oldSafe=g_safe155;LooseSafe156 oldLoose[64];memcpy(oldLoose,g_looseSafe156,sizeof(oldLoose));
 const auto oldBlocked=g_trailBlocked176;const auto oldLocal=fixtureLocal143;const LONG enabled=g_enabled;
 BYTE active=0;ReadAt(component,0x234,&active,1);BYTE cc=0;ReadAt(component,0xcc,&cc,1);
 *reinterpret_cast<BYTE*>(component+0xcc)=1;*reinterpret_cast<BYTE*>(component+0x234)=0;
 auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(code!=nullptr,"176 trail fixture allocation");
 memcpy(code,kTrailBytes176,sizeof(kTrailBytes176));
 const INT displacement=INT(reinterpret_cast<UINT>(TrailCreateFixture176)-(reinterpret_cast<UINT>(code)+15));memcpy(code+11,&displacement,4);
 FlushInstructionCache(GetCurrentProcess(),code,64);const auto invoke=reinterpret_cast<NativeTrail176>(code);trailCalls176=0;
 Check(invoke(reinterpret_cast<void*>(component),0)&&trailCalls176==1,"176 exact native handler baseline returns true and pops event argument");
 Check(MH_CreateHook(code,reinterpret_cast<void*>(TrailGate176),&g_trailOriginal176)==MH_OK&&MH_EnableHook(code)==MH_OK,"176 actual native cmp-byte prologue MinHook relocation");
 g_safe155={};g_safe155.target=target;g_safe155.component=component;memset(g_looseSafe156,0,sizeof(g_looseSafe156));
 volatile UINT canary=0xabcdef12;
 Check(invoke(reinterpret_cast<void*>(component),0)&&trailCalls176==1&&canary==0xabcdef12,"176 exact held unlit Molotov suppresses trail before release registration with ret4 intact");
 g_safe155.loose=true;g_safe155.launched=true;RememberLoose156();g_safe155={};
 Check(invoke(reinterpret_cast<void*>(component),0)&&trailCalls176==1,"176 dropped unlit Molotov remains trail-free after held identity retires");
 fixtureLocal143^=1;invoke(reinterpret_cast<void*>(component),0);Check(trailCalls176==2,"176 different local owner leaves event native");fixtureLocal143=oldLocal;
 const UINT entity=Word(target.projectileRef,12);const auto id=*reinterpret_cast<ULONGLONG*>(entity+8);*reinterpret_cast<ULONGLONG*>(entity+8)=id+1;
 invoke(reinterpret_cast<void*>(component),0);Check(trailCalls176==3,"176 recycled projectile reference cannot suppress another entity trail");*reinterpret_cast<ULONGLONG*>(entity+8)=id;
 g_safe155=oldSafe;g_safe155.target=target;g_safe155.component=component;g_safe155.armed=true;*reinterpret_cast<BYTE*>(component+0x234)=1;
 invoke(reinterpret_cast<void*>(component),0);Check(trailCalls176==4,"176 subsequent manually lit Molotov keeps native trail creation");
 g_safe155.armed=false;g_safe155.target.archetype=0x03c006e5ac354e19ULL;*reinterpret_cast<BYTE*>(component+0x234)=0;
 invoke(reinterpret_cast<void*>(component),0);Check(trailCalls176==5,"176 unlit grenade trail untouched");
 g_safe155.target=target;InterlockedExchange(&g_enabled,0);invoke(reinterpret_cast<void*>(component),0);Check(trailCalls176==6,"176 disabled feature leaves native event unchanged");InterlockedExchange(&g_enabled,enabled);
 Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"176 trail detour cleanup");g_trailOriginal176=nullptr;VirtualFree(code,0,MEM_RELEASE);
 g_safe155=oldSafe;memcpy(g_looseSafe156,oldLoose,sizeof(oldLoose));g_trailBlocked176=oldBlocked;fixtureLocal143=oldLocal;*reinterpret_cast<BYTE*>(component+0x234)=active;*reinterpret_cast<BYTE*>(component+0xcc)=cc;
 puts("PASS:176 native activate_trail MinHook/thiscall-ret4, exact unlit held/loose suppression, local-owner/recycled-ref/grenade/disabled isolation and subsequent lit trail.");
}
