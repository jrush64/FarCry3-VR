bool fixtureSurfaceHit177=true,fixtureSurfaceAck177=true;unsigned fixtureSurfaceUses177=0;
bool __stdcall FixtureSurfaceQuery177(UINT strategy){
 auto& s=g_surfaceScope177;++s.rewrites;
 *reinterpret_cast<ULONGLONG*>(strategy+0x98)=~0ULL;
 *reinterpret_cast<BYTE*>(strategy+(s.index?0x140:0xa0))=fixtureSurfaceHit177?1:0;
 float hit[3];for(int i=0;i<3;++i)hit[i]=s.origin[i]+s.delta[i]*.5f;
 memcpy(reinterpret_cast<void*>(strategy+(s.index?0xa0:0xa4)),hit,12);return true;
}
bool __fastcall FixtureSurfaceUse177(void* self,void*,UINT){++fixtureSurfaceUses177;UINT strategy=reinterpret_cast<UINT>(self);
 if(fixtureSurfaceAck177){ULONGLONG id=0;Reference177(Word(strategy,0x88),id);*reinterpret_cast<ULONGLONG*>(Word(strategy,0x7c))=id;*reinterpret_cast<UINT*>(strategy+0x80)=1;}return true;}
void TestSurface177(UINT strategy,UINT list,float* matrix){
 auto query0=g_surfaceQuery177[0],query1=g_surfaceQuery177[1];auto use0=g_surfaceUse177[0],use1=g_surfaceUse177[1];
 *reinterpret_cast<UINT*>(strategy+0x7c)=list;
 for(int index=0;index<2;++index){*reinterpret_cast<UINT*>(strategy)=g_engine+kPlaceVtable177[index];*reinterpret_cast<UINT*>(strategy+0x80)=0;
  g_surfaceQuery177[index]=FixtureSurfaceQuery177;g_surfaceUse177[index]=reinterpret_cast<SurfaceUse177>(FixtureSurfaceUse177);
  const auto kind=index?Mine177:C4177;fixtureSurfaceHit177=false;const auto used=fixtureSurfaceUses177;
  Check(!DispatchSurface177(strategy,kind,matrix)&&fixtureSurfaceUses177==used,"177 no hand surface never calls native use");
  fixtureSurfaceHit177=true;fixtureSurfaceAck177=false;Check(!DispatchSurface177(strategy,kind,matrix)&&fixtureSurfaceUses177==used+1,"177 handler success alone is not placement acknowledgement");
  fixtureSurfaceAck177=true;Check(DispatchSurface177(strategy,kind,matrix)&&fixtureSurfaceUses177==used+2,"177 real placed GUID list acknowledges exactly one use");
  Check(!DispatchSurface177(strategy,kind,matrix)&&fixtureSurfaceUses177==used+2,"177 already placed GUID cannot be committed twice");
 }
 g_surfaceQuery177[0]=query0;g_surfaceQuery177[1]=query1;g_surfaceUse177[0]=use0;g_surfaceUse177[1]=use1;
 auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));Check(code&&MH_Initialize()==MH_OK,"177 surface seam setup");
 for(int index=0;index<2;++index){memset(code,0xcc,4096);code[0]=0xe8;*reinterpret_cast<INT*>(code+1)=95;code[5]=0xc3;code[100]=0xc3;
  auto original=g_surfaceOriginal177[index];void* bridge=index?reinterpret_cast<void*>(MineSurface177):reinterpret_cast<void*>(C4Surface177);
  Check(MH_CreateHook(code,bridge,&g_surfaceOriginal177[index])==MH_OK&&MH_EnableHook(code)==MH_OK,"177 exact CALL surface detour");
  float origin[3]={},delta[3]={};UINT flags=0,ecxOut=0,edxOut=0;UINT input[4]={1,2,3,4},output[4]={};
  g_surfaceScope177={};auto& s=g_surfaceScope177;s.active=true;s.index=index;s.strategy=strategy;for(int j=0;j<3;++j){s.origin[j]=float(10+j);s.delta[j]=float(20+j);}
  __asm { mov ebx,strategy }
  __asm { lea eax,origin }
  __asm { lea esi,delta }
  __asm { mov ecx,17717717h }
  __asm { mov edx,77177177h }
  __asm { movdqu xmm1,input }
  __asm { stc }
  __asm { std }
  __asm { call code }
  __asm { pushfd }
  __asm { pop flags }
  __asm { cld }
  __asm { mov ecxOut,ecx }
  __asm { mov edxOut,edx }
  __asm { movdqu output,xmm1 }
  Check(s.rewrites==1&&!memcmp(origin,s.origin,12)&&!memcmp(delta,s.delta,12),"177 relocated native CALL rewrites only current stack ray");
  Check((flags&0x401)==0x401&&ecxOut==0x17717717&&edxOut==0x77177177&&!memcmp(input,output,16),"177 surface CALL retains flags registers SSE");
  UINT regs[8]={};regs[3]=reinterpret_cast<UINT>(origin)-4;regs[7]=reinterpret_cast<UINT>(origin);regs[1]=reinterpret_cast<UINT>(delta);regs[4]=strategy+1;
  RewriteSurface177(index,regs);Check(s.rewrites==1,"177 wrong strategy bypasses ray override");s={};
  Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"177 surface seam cleanup");g_surfaceOriginal177[index]=original;
 }
 Check(MH_Uninitialize()==MH_OK,"177 surface hooks cleanup");VirtualFree(code,0,MEM_RELEASE);
 std::puts("PASS:177 hand surface no-hit rejection, actual placed-GUID acknowledgement, duplicate refusal, exact relocated CALL and register/flags/SSE preservation.");
}

