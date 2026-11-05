// Verified native use debit sites: C4 DEC EBX, mine DEC ESI. Preserve the native
// setter, stock events and refill path; change only the one-item debit operand.
constexpr UINT kStockSites179[2]={0x1238205,0x1233150};
constexpr BYTE kStockBytes179[2][8]={{0x4b,0x85,0xdb,0x7f,0x66,0x8b,0x57,0x24},{0x4e,0x85,0xf6,0x7f,0x34,0x8b,0x57,0x24}};
void* g_stockOriginal179[2]={};bool g_stockInstalled179[2]={};
bool ReadPlaceStock179(const PlaceBinding177& b,UINT& amount){
 const UINT ammo=Word(b.wrapper,0x48);return ammo&&ReadAt(ammo,0xcc,&amount,4)&&amount<=10000;
}
void SamplePlaceStock179(UINT strategy,PlaceableKind177 kind){
 const int index=PlaceIndex177(kind);PlaceBinding177 b;if(index<0||!BindPlace177(strategy,kind,b))return;
 UINT stock=0;const bool known=ReadPlaceStock179(b,stock);const UINT count=Word(strategy,0x80);
 AcquireSRWLockExclusive(&g_manualLock143);auto& s=g_placeStats177[index];
 s.stockKnown179=known;s.stock179=known?stock:0;s.placedCount179=count<=64?count:0;
 ReleaseSRWLockExclusive(&g_manualLock143);
}
void __cdecl PreservePlaceStock179(unsigned site,UINT* regs){
 const auto& scope=g_throwScope178;
 if(!regs||site>1||!scope.active||scope.index!=int(site)||!InterlockedCompareExchange(&g_enabled,0,0)||!Unlimited147())return;
 const UINT strategy=regs[site?4:1];PlaceBinding177 current;
 if(strategy!=scope.binding.strategy||!BindPlace177(strategy,site?Mine177:C4177,current)||!SameBinding177(current,scope.binding,false))return;
 const UINT ammo=Word(current.wrapper,0x48),slot=site?1:4;UINT stock=0;
 if(!ReadPlaceStock179(current,stock)||!stock||regs[6]!=ammo||regs[slot]!=stock)return;
 // Original DEC now restores the original value, avoiding a refill or empty transition.
 ++regs[slot];InterlockedIncrement(&g_preserved147);
 AcquireSRWLockExclusive(&g_manualLock143);++g_placeStats177[site].preservedStock179;ReleaseSRWLockExclusive(&g_manualLock143);
#ifdef FC3_THROW_PROBE_TEST
 __asm { pxor xmm0,xmm0 }
 __asm { fninit }
#endif
}
#define STOCK_BOUNDARY179(name,index,slot) \
__declspec(naked) void name(){ \
 __asm pushfd \
 __asm pushad \
 __asm cld \
 __asm mov eax,esp \
 __asm sub esp,544 \
 __asm and esp,0fffffff0h \
 __asm mov [esp+512],eax \
 __asm fxsave [esp] \
 __asm push eax \
 __asm push index \
 __asm call PreservePlaceStock179 \
 __asm add esp,8 \
 __asm fxrstor [esp] \
 __asm mov esp,[esp+512] \
 __asm popad \
 __asm popfd \
 __asm jmp dword ptr [g_stockOriginal179+slot] \
}
STOCK_BOUNDARY179(C4StockBoundary179,0,0)
STOCK_BOUNDARY179(MineStockBoundary179,1,4)
#undef STOCK_BOUNDARY179
void DisableStock179(){for(int i=0;i<2;++i){if(g_stockInstalled179[i])MH_DisableHook(reinterpret_cast<void*>(g_engine+kStockSites179[i]));g_stockInstalled179[i]=false;}}
bool InstallStock179(BYTE* image){
 for(int i=0;i<2;++i)if(memcmp(image+kStockSites179[i],kStockBytes179[i],8))return false;
 void* bridges[2]={reinterpret_cast<void*>(C4StockBoundary179),reinterpret_cast<void*>(MineStockBoundary179)};
 for(int i=0;i<2;++i){if(MH_CreateHook(image+kStockSites179[i],bridges[i],&g_stockOriginal179[i])!=MH_OK||MH_EnableHook(image+kStockSites179[i])!=MH_OK){DisableStock179();return false;}g_stockInstalled179[i]=true;}
 return true;
}
