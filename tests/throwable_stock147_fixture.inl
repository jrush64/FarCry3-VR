__declspec(naked) void StockSetter147(){__asm {mov [ecx+60h],eax} __asm ret}
__declspec(naked) void CallStock147(UINT){
    __asm {
        push ebp
        mov ebp,esp
        push ebx
        push esi
        push edi
        mov edi,[g_strategy]
        mov esi,12345678h
        mov ebx,87654321h
        mov ecx,[ebp+8]
        mov eax,[ecx+60h]
        mov edx,eax
        cmp eax,1
        jl stock_debit_ready147
        mov edx,1
    stock_debit_ready147:
        fninit
        fld dword ptr [kX87]
        movups xmm0,[g_seed]
        stc
        std
        call dword ptr [g_entry]
        mov [g_registers],eax
        mov [g_registers+4],ecx
        mov [g_registers+8],edx
        mov [g_registers+12],ebx
        mov [g_registers+16],esi
        mov [g_registers+20],edi
        movups [g_sse],xmm0
        fstp dword ptr [g_x87]
        pushfd
        pop eax
        mov [g_flags],eax
        cld
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
void TestStock147(UINT strategy,UINT wrapper,UINT inventory){
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(code&&MH_Initialize()==MH_OK,"stock147 native-site ABI allocation/library");
    memcpy(code,kStockBytes147,7);const UINT site=reinterpret_cast<UINT>(code),setter=reinterpret_cast<UINT>(StockSetter147);
    const UINT relative=setter-(site+7);memcpy(code+3,&relative,4);code[7]=0xc3;FlushInstructionCache(GetCurrentProcess(),code,16);g_entry=code;
    *reinterpret_cast<UINT*>(inventory+0x60)=2;CallStock147(inventory);UINT baseline[6];memcpy(baseline,g_registers,sizeof(baseline));
    Check(Word(inventory,0x60)==1,"original native stock setter deducts one");
    Check(MH_CreateHook(code,reinterpret_cast<void*>(StockBoundary147),&g_stockOriginal147)==MH_OK&&MH_EnableHook(code)==MH_OK,"actual native SUB and relative CALL MinHook relocation");
    SetIdentityReader143(ReadLocal143);g_enabled=1;SetUnlimited147(false);*reinterpret_cast<UINT*>(inventory+0x60)=2;
    volatile UINT canary=0x1234abcd;CallStock147(inventory);
    Check(Word(inventory,0x60)==1&&!memcmp(baseline,g_registers,sizeof(baseline)),"disabled unlimited preserves native count and all registers");
    const UINT ownerRef=Word(wrapper,0x28);const auto archetype=*reinterpret_cast<ULONGLONG*>(strategy+0x40);const auto preserved=PreservedStock147();
    for(auto item:{0x03c006e5ac354e19ULL,0x000006f6a4abae8dULL}){
        *reinterpret_cast<ULONGLONG*>(strategy+0x40)=item;*reinterpret_cast<UINT*>(inventory+0x60)=2;SetUnlimited147(true);
        for(int n=0;n<6;++n){CallStock147(inventory);Check(Word(inventory,0x60)==2,"repeated local grenade/Molotov uses keep two-item stock through actual native setter");}
        Check(canary==0x1234abcd&&g_registers[0]==2&&g_registers[1]==inventory&&g_registers[2]==0&&g_registers[3]==baseline[3]&&g_registers[4]==baseline[4]&&g_registers[5]==strategy&&
              (g_flags&0x400)&&!memcmp(g_seed,g_sse,sizeof(g_seed))&&g_x87==kX87,"only debit EDX is changed; ret/stack/DF/integer/SSE/x87 preserved around native setter");
        *reinterpret_cast<ULONGLONG*>(ownerRef)=fixtureLocal143+1;CallStock147(inventory);Check(Word(inventory,0x60)==1,"NPC stock still deducts");*reinterpret_cast<ULONGLONG*>(ownerRef)=fixtureLocal143;
    }
    Check(PreservedStock147()==preserved+12,"preserved-stock counter names actual suppressed native debits");
    *reinterpret_cast<ULONGLONG*>(strategy+0x40)=0x12345678;*reinterpret_cast<UINT*>(inventory+0x60)=2;CallStock147(inventory);Check(Word(inventory,0x60)==1,"unverified item and firearm stock remain native");
    *reinterpret_cast<ULONGLONG*>(strategy+0x40)=0x03c006e5ac354e19ULL;SetIdentityReader143(nullptr);*reinterpret_cast<UINT*>(inventory+0x60)=2;CallStock147(inventory);Check(Word(inventory,0x60)==1,"unknown/stale local identity never bypasses stock deduction");
    SetIdentityReader143(ReadLocal143);*reinterpret_cast<UINT*>(inventory+0x60)=0;CallStock147(inventory);Check(Word(inventory,0x60)==0,"empty native inventory remains empty, with no underflow or manufactured stock");
    *reinterpret_cast<ULONGLONG*>(strategy+0x40)=archetype;*reinterpret_cast<UINT*>(inventory+0x60)=4;SetIdentityReader143(nullptr);SetUnlimited147(true);
    Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK&&MH_Uninitialize()==MH_OK,"stock147 native-site cleanup");g_stockOriginal147=nullptr;VirtualFree(code,0,MEM_RELEASE);
    std::puts("PASS: actual native debit/setter MinHook relocation; twelve grenade/Molotov uses keep stock2; toggle-off/NPC/unknown-item/unknown-owner paths unchanged; stack/integer/DF/SSE/x87 preserved. Requires existing stock; no item spawning.");
}
