void TestPlaceStock179(UINT strategy,UINT ammo){
 const auto oldUnlimited=g_unlimited147;const auto oldScope=g_throwScope178;const auto oldAmmo=Word(Word(strategy,0x48),0x48);
 *reinterpret_cast<UINT*>(Word(strategy,0x48)+0x48)=ammo;
 auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));Check(code&&MH_Initialize()==MH_OK,"179 stock native seam setup");
 for(int site=0;site<2;++site){const auto kind=site?Mine177:C4177;*reinterpret_cast<UINT*>(strategy)=g_engine+kPlaceVtable177[site];*reinterpret_cast<UINT*>(ammo+0xcc)=2;
  PlaceBinding177 b;Check(BindPlace177(strategy,kind,b),"179 stock exact local fixture");g_throwScope178={};g_throwScope178.active=true;g_throwScope178.index=site;g_throwScope178.binding=b;
  UINT regs[8]={};regs[site?4:1]=strategy;regs[6]=ammo;regs[site?1:4]=2;g_unlimited147=1;
  PreservePlaceStock179(site,regs);Check(regs[site?1:4]==3&&Word(ammo,0xcc)==2,"179 unlimited alters only debit register before native DEC");
  regs[site?1:4]=2;g_unlimited147=0;PreservePlaceStock179(site,regs);Check(regs[site?1:4]==2,"179 finite native debit unchanged");g_unlimited147=1;
  regs[6]=ammo+4;PreservePlaceStock179(site,regs);Check(regs[site?1:4]==2,"179 wrong inventory rejected");regs[6]=ammo;
  fixtureLocal143^=1;PreservePlaceStock179(site,regs);Check(regs[site?1:4]==2,"179 nonlocal stock untouched");fixtureLocal143^=1;
  g_throwScope178.active=false;PreservePlaceStock179(site,regs);Check(regs[site?1:4]==2,"179 native nonmanual action untouched");g_throwScope178.active=true;
  *reinterpret_cast<UINT*>(ammo+0xcc)=0;regs[site?1:4]=0;PreservePlaceStock179(site,regs);Check(regs[site?1:4]==0,"179 unlimited does not invent zero stock");
  *reinterpret_cast<UINT*>(ammo+0xcc)=2;
  // Execute the exact DEC/test/short-jump block through MinHook, including its
  // actual branch displacement; a fixture RET replaces only the target body.
  memset(code,0xcc,4096);memcpy(code,kStockBytes179[site],8);code[5]=0xc3;code[5+(site?0x34:0x66)]=0xc3;
  auto original=g_stockOriginal179[site];void* bridge=site?reinterpret_cast<void*>(MineStockBoundary179):reinterpret_cast<void*>(C4StockBoundary179);
  Check(MH_CreateHook(code,bridge,&g_stockOriginal179[site])==MH_OK&&MH_EnableHook(code)==MH_OK,"179 stock seam actual native branch relocated");
  for(int unlimited=0;unlimited<2;++unlimited){g_unlimited147=unlimited;UINT result=0,flags=0;UINT vector[4]={1,2,3,4},vectorOut[4]={};
   if(site){
    __asm { mov ebx,strategy }
    __asm { mov esi,2 }
   }else{
    __asm { mov esi,strategy }
    __asm { mov ebx,2 }
   }
   __asm { mov ecx,ammo }
   __asm { movdqu xmm0,vector }
   __asm { stc }
   __asm { std }
   __asm { call code }
   __asm { pushfd }
   __asm { pop flags }
   __asm { cld }
   __asm { movdqu vectorOut,xmm0 }
   if(site){__asm { mov result,esi }}else{__asm { mov result,ebx }}
   // TEST in the original block clears CF; DF must survive the helper.
   Check(result==UINT(unlimited?2:1)&&(flags&0x400)&&!memcmp(vector,vectorOut,16),"179 relocated native debit preserves stock only when enabled and preserves SSE/DF");
  }
  Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"179 stock seam cleanup");g_stockOriginal179[site]=original;
  SamplePlaceStock179(strategy,kind);Check(g_placeStats177[site].stockKnown179&&g_placeStats177[site].stock179==2,"179 exact remaining stock published");
  *reinterpret_cast<UINT*>(ammo+0xcc)=0;SamplePlaceStock179(strategy,kind);Check(g_placeStats177[site].stockKnown179&&g_placeStats177[site].stock179==0,"179 depleted stock published independently of held pose");
 }
 Check(MH_Uninitialize()==MH_OK,"179 stock hooks cleanup");VirtualFree(code,0,MEM_RELEASE);
 *reinterpret_cast<UINT*>(Word(strategy,0x48)+0x48)=oldAmmo;g_unlimited147=oldUnlimited;g_throwScope178=oldScope;
 std::puts("PASS:179 exact C4/mine native stock DEC seams: finite debit, unlimited preservation, zero/NPC/wrong inventory/stock-action isolation, remaining stock, original branch relocation and SSE/DF preservation.");
}
