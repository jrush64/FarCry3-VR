__declspec(align(16)) float g_idleOutput160[16]={},g_idleSeed160[16]={};void* g_idleEntry160=nullptr;
__declspec(naked) void CallIdle160(UINT){
 __asm push ebp
 __asm mov ebp,esp
 __asm push ebx
 __asm push esi
 __asm push edi
 __asm sub esp,128
 __asm and esp,0fffffff0h
 __asm sub esp,12
 __asm movups xmm0,[g_idleSeed160]
 __asm movaps [esp+3ch],xmm0
 __asm movups xmm0,[g_idleSeed160+16]
 __asm movaps [esp+4ch],xmm0
 __asm movups xmm0,[g_idleSeed160+32]
 __asm movaps [esp+5ch],xmm0
 __asm movups xmm0,[g_idleSeed160+48]
 __asm movaps [esp+6ch],xmm0
 __asm mov ebx,[ebp+8]
 __asm lea eax,[g_idleOutput160]
 __asm movups xmm1,[g_seed]
 __asm fninit
 __asm fld dword ptr [kX87]
 __asm stc
 __asm std
 __asm call dword ptr [g_idleEntry160]
 __asm movups [g_sse],xmm1
 __asm fstp dword ptr [g_x87]
 __asm pushfd
 __asm pop eax
 __asm mov [g_flags],eax
 __asm cld
 __asm lea esp,[ebp-12]
 __asm pop edi
 __asm pop esi
 __asm pop ebx
 __asm pop ebp
 __asm ret
}
void TestMolotovFlame160(const fc3throw::Prepared143& target,UINT component){
 auto savedPrepared=g_prepared143;auto savedPending=g_pending143;auto savedLaunched=g_manualLaunched146;g_pending143={};g_manualLaunched146={};g_prepared143=target;
 float wanted[16]={1,0,0,0,0,1,0,0,0,0,1,0,11,22,33,1};for(int i=0;i<16;++i)g_idleSeed160[i]=float(i+30);
 Check(PublishMolotovFlame160(target,wanted),"160 publish exact active Molotov's native column-major rag frame");
 auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(code!=nullptr,"idle effect upload fixture allocation");
 const BYTE native[]={0x0f,0x28,0x44,0x24,0x40,0x8b,0xf0,0x0f,0x29,0x06,0x0f,0x28,0x44,0x24,0x50,0x0f,0x29,0x46,0x10,0x0f,0x28,0x44,0x24,0x60,0x0f,0x29,0x46,0x20,0x0f,0x28,0x44,0x24,0x70,0x0f,0x29,0x46,0x30,0xc3};
 memcpy(code,native,sizeof(native));g_idleEntry160=code;FlushInstructionCache(GetCurrentProcess(),code,sizeof(native));CallIdle160(component);Check(!memcmp(g_idleOutput160,g_idleSeed160,64),"original exact idle matrix copy baseline");
 Check(MH_CreateHook(code,reinterpret_cast<void*>(FlameUpdateBoundary160),&g_flameUpdateOriginal160)==MH_OK&&MH_EnableHook(code)==MH_OK,"160 exact native MOVAPS update upload MinHook relocation");
 const auto before=g_flameUpdates160;CallIdle160(component);
 Check(!memcmp(g_idleOutput160,wanted,64)&&g_flameUpdates160==before+1,"native upload receives exact held Molotov rag frame, all four rows");
 const auto snapshot166=InspectMolotov166(target,component);
 Check(snapshot166.entity&&snapshot166.component&&snapshot166.positionValid,"166 exact Molotov entity and component readable");
 Check(snapshot166.effectMs&&!memcmp(snapshot166.nativeFlame,g_idleSeed160+12,12)&&!memcmp(snapshot166.ragTarget,wanted+12,12),"166 original emitter and corrected rag target remain distinct");
 Check(!memcmp(snapshot166.effectSource169,g_idleSeed160,64)&&!memcmp(snapshot166.effectApplied169,wanted,64),"169 receipt retains full original and actually consumed matrix");
 // Exact downstream child layout observed in 115C5F90, sampled read-only.
 BYTE effect178[0x90]={},child178[0x1b0]={},definition178[0x60]={};
 UINT definitionRef178=reinterpret_cast<UINT>(definition178);
 *reinterpret_cast<UINT*>(effect178+0x84)=reinterpret_cast<UINT>(child178);*reinterpret_cast<UINT*>(effect178+0x88)=1;
 *reinterpret_cast<UINT*>(child178+0x174)=reinterpret_cast<UINT>(&definitionRef178);
 memcpy(child178,wanted,64);memcpy(definition178,wanted,64);*reinterpret_cast<float*>(child178+0xf0)=.5f;*reinterpret_cast<UINT*>(definition178+0x50)=0x80;
 CaptureFlameChildren178(reinterpret_cast<UINT>(effect178),GetTickCount64());
 const auto childReceipt178=InspectMolotovChildren178(target);
 Check(childReceipt178.count==1&&childReceipt178.child[0].valid&&childReceipt178.child[0].scale==.5f&&childReceipt178.child[0].flags==0x80&&!memcmp(childReceipt178.child[0].local,wanted,64)&&!memcmp(childReceipt178.child[0].before,wanted,64),"178 scoped receipt records actual authored child transform and prior composed pose without mutation");
 auto wrongChild178=target;wrongChild178.epoch++;Check(!InspectMolotovChildren178(wrongChild178).ms,"178 child receipts cannot cross tracking epochs");
 *reinterpret_cast<UINT*>(effect178+0x88)=65;CaptureFlameChildren178(reinterpret_cast<UINT>(effect178),GetTickCount64());Check(!InspectMolotovChildren178(target).count,"178 implausible native child count rejected before traversal");
 float later169[16];memcpy(later169,wanted,64);later169[12]+=1;
 Check(PublishMolotovFlame160(target,later169),"169 publish subsequent rag pose");
 const auto lag169=InspectMolotov166(target,component);
 Check(!memcmp(lag169.effectApplied169,wanted,64)&&lag169.ragTarget[0]==wanted[12],"169 newer publication cannot masquerade as consumed effect upload");
 auto epoch169=target;++epoch169.epoch;Check(!InspectMolotov166(epoch169,component).effectMs,"169 old tracking epoch has no current upload receipt");
 auto serial169=target;++serial169.serial;Check(!InspectMolotov166(serial169,component).effectMs,"169 different preparation serial has no upload receipt");
 Check(PublishMolotovFlame160(target,wanted),"169 restore published frame");
 auto wrong166=target;wrong166.projectile^=1;
 Check(!InspectMolotov166(wrong166,component).entity,"166 recycled or wrong entity identity rejected");
 wrong166=target;wrong166.archetype=0;
 Check(!InspectMolotov166(wrong166,component).entity,"166 non Molotov rejected");
 Check(!memcmp(g_idleOutput160,wanted,64),"166 inspection does not alter native emitter output");

 Check((g_flags&0x401)==0x401&&!memcmp(g_sse,g_seed,sizeof(g_seed))&&g_x87==kX87,"160 idle bridge retains stack, CF/DF, untouched SSE and x87");
 auto checkOriginal=[&](const char* why){CallIdle160(component);Check(!memcmp(g_idleOutput160,g_idleSeed160,64),why);};
 g_pending143.active=true;checkOriginal("release queue immediately stops held-flame ownership");g_pending143={};
 g_manualLaunched146=target;checkOriginal("launched projectile restores its native world emitter");g_manualLaunched146={};
 g_flame160.ms=GetTickCount64()-1000;checkOriginal("stale held transform cannot move native flame");Check(PublishMolotovFlame160(target,wanted),"refresh flame frame");
 const UINT reference=Word(component,4);*reinterpret_cast<UINT*>(component+4)=0;checkOriginal("another explosive component cannot receive Molotov flame attachment");*reinterpret_cast<UINT*>(component+4)=reference;
 UINT state[9]={};state[4]=component;state[7]=reinterpret_cast<UINT>(g_idleOutput160);AttachFlame160(state,true);Check(!memcmp(g_idleOutput160,wanted,64)&&g_flameSpawns160,"native initial idle spawn uses same bound rag frame");
 wanted[0]=std::numeric_limits<float>::quiet_NaN();Check(!PublishMolotovFlame160(target,wanted),"non-finite emitter frame refused");
 Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"160 upload detour cleanup");g_flameUpdateOriginal160=nullptr;VirtualFree(code,0,MEM_RELEASE);g_flame160={};g_flameReceipt169={};g_flameChildren178={};g_prepared143=savedPrepared;g_pending143=savedPending;g_manualLaunched146=savedLaunched;
 std::puts("PASS:160 exact native idle matrix upload MinHook/ABI, owner/ref/epoch binding, initial spawn, stale/wrong-component/release retirement and finite matrix guard.");
}
