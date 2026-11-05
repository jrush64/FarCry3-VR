// Exact native use-call seams: replace only the released item's transform and
// initial linear velocity. Native inventory, activation and collision stay native.
constexpr UINT kThrowSites178[4]={0x123767d,0x123231d,0x1237a59,0x12327e7};
constexpr BYTE kThrowBytes178[4][6]={{0xe8,0xee,0x03,0x4d,0,0x5f},{0xe8,0x4e,0x57,0x4d,0,0x5f},{0xff,0xd2,0x8b,0x47,0x78,0xf3},{0x8b,0x90,0xec,0,0,0}};
void* g_throwOriginal178[4]={};bool g_throwInstalled178[4]={};
struct ThrowScope178 {PlaceBinding177 binding;int index;float matrix[16],velocity[3];unsigned transforms,velocities,epoch179;UINT physics179,physicsVtable179,setter179,entity179,body179;bool bodyMissing179,active;LONGLONG queuedQpc183;};
__declspec(thread) ThrowScope178 g_throwScope178={};
void RewriteThrow178(unsigned site,UINT* regs){
 auto& s=g_throwScope178;if(!regs||!s.active||site>3||int(site&1)!=s.index||!InterlockedCompareExchange(&g_enabled,0,0))return;
 const UINT stack=regs[3]+4;PlaceBinding177 current;
 if(!BindPlace177(s.binding.strategy,s.index?Mine177:C4177,current)||!SameBinding177(s.binding,current,true))return;
 const UINT pointer=site<2?regs[7]:Word(stack);const UINT bytes=site<2?64:12;
 if(pointer<stack||pointer-stack>4096||!Writable143(pointer,bytes))return;
 if(site<2){if(regs[0]!=s.binding.strategy||Word(stack)!=Word(s.binding.reference,12))return;}
 else if(Word(regs[2],8)!=s.binding.strategy)return;
 __try{memcpy(reinterpret_cast<void*>(pointer),site<2?s.matrix:s.velocity,bytes);if(site<2)++s.transforms;else {++s.velocities;s.entity179=Word(s.binding.reference,12);s.physics179=regs[6];s.physicsVtable179=Word(regs[6]);s.setter179=Word(s.physicsVtable179,0xec);}}__except(EXCEPTION_EXECUTE_HANDLER){}
}
#define THROW_BOUNDARY178(name,index,slot) \
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
 __asm call RewriteThrow178 \
 __asm add esp,8 \
 __asm fxrstor [esp] \
 __asm mov esp,[esp+512] \
 __asm popad \
 __asm popfd \
 __asm jmp dword ptr [g_throwOriginal178+slot] \
}
THROW_BOUNDARY178(C4Transform178,0,0)
THROW_BOUNDARY178(MineTransform178,1,4)
THROW_BOUNDARY178(C4Velocity178,2,8)
THROW_BOUNDARY178(MineVelocity178,3,12)
#undef THROW_BOUNDARY178
// Same lookup ABI as native C4 use at 1123786F..1123787A; no guessed
// component member offsets. Runtime metadata +0x10 identifies the physics iface.
UINT NativeResolvePhysics179(UINT entity){
 const UINT typeGetter=g_engine+0x57280,lookup=g_engine+0x1706640;UINT component=0;
 __asm { call typeGetter }
 __asm { add eax,10h }
 __asm { push eax }
 __asm { mov eax,entity }
 __asm { call lookup }
 __asm { mov component,eax }
 return component;
}
using ResolvePhysics179=UINT(*)(UINT);ResolvePhysics179 g_resolvePhysics179=NativeResolvePhysics179;
// Verified CRigidPhysComponent vtable and setter. Other component layouts
// are never probed at +0x54. The native setter silently returns on a null body.
int BodyReady179(UINT table,UINT setter,UINT component,UINT& body){
 body=0;if(table!=g_engine+0x1f566d8||setter!=g_engine+0x1684a20)return -1;
 body=Word(component,0x54);return body?1:0;
}
struct DeferredVelocity179 {ThrowScope178 scope;ULONGLONG deadline=0;bool active=false;};
__declspec(thread) DeferredVelocity179 g_deferredVelocity179[2]={};
// A native use dispatch sends activation and attachment events AFTER its first
// velocity write. Commit measured launch velocity once after that initialization,
// to the same released physics component; never keep accelerating a flying item.
bool FinalizeThrowVelocity179(ThrowScope178& s){
 s.bodyMissing179=false;
 AcquireSRWLockShared(&g_manualLock143);const bool context=PlaceContextValid177(s.binding,s.epoch179,GetTickCount64());ReleaseSRWLockShared(&g_manualLock143);
 if(!context||!InterlockedCompareExchange(&g_enabled,0,0))return false;
 ULONGLONG physicsItem=0,local=0;PlaceBinding177 current;
 if(s.transforms!=1||s.velocities!=1||!s.physics179||!s.setter179||
    Word(s.physics179)!=s.physicsVtable179||Word(s.physicsVtable179,0xec)!=s.setter179||
    !ReadAt(s.entity179,8,&physicsItem,8)||physicsItem!=s.binding.item||
    !LocalIdentity143(local)||local!=s.binding.owner||
    !BindPlace177(s.binding.strategy,s.index?Mine177:C4177,current)||!SameBinding177(s.binding,current,false))return false;
 using SetVelocity179=void(__thiscall*)(void*,const float*,UINT);
 __try{if(!g_resolvePhysics179||g_resolvePhysics179(s.entity179)!=s.physics179)return false;
  if(BodyReady179(s.physicsVtable179,s.setter179,s.physics179,s.body179)==0){s.bodyMissing179=true;return false;}
  reinterpret_cast<SetVelocity179>(s.setter179)(reinterpret_cast<void*>(s.physics179),s.velocity,0);return true;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void BeginSettle181(int index,const ThrowScope178& scope);
void PumpFinalVelocity179(UINT strategy,PlaceableKind177 kind){
 const int index=PlaceIndex177(kind);if(index<0)return;auto& p=g_deferredVelocity179[index];if(!p.active||p.scope.binding.strategy!=strategy)return;
 if(!InterlockedCompareExchange(&g_enabled,0,0)||GetTickCount64()>p.deadline){p={};AcquireSRWLockExclusive(&g_manualLock143);++g_placeStats177[index].velocityExpired179;++g_placeStats177[index].correctionFailed178;ReleaseSRWLockExclusive(&g_manualLock143);return;}
 if(FinalizeThrowVelocity179(p.scope)){
  AcquireSRWLockExclusive(&g_manualLock143);auto& stats=g_placeStats177[index];++stats.finalVelocity179;stats.physicsBody179=p.scope.body179;ReleaseSRWLockExclusive(&g_manualLock143);BeginSettle181(index,p.scope);p={};
 }else if(!p.scope.bodyMissing179){p={};AcquireSRWLockExclusive(&g_manualLock143);++g_placeStats177[index].velocityExpired179;++g_placeStats177[index].correctionFailed178;ReleaseSRWLockExclusive(&g_manualLock143);}
}
bool DispatchThrow178(UINT strategy,PlaceableKind177 kind,const float* matrix,const float* velocity){
 const int index=PlaceIndex177(kind);PlaceBinding177 before;
 if(index<0||g_throwScope178.active||!MatrixValid177(matrix)||!BindPlace177(strategy,kind,before)||!before.item||SurfacePlaced177(strategy,before.item)||!g_surfaceUse177[index])return false;
 const UINT flagOffset=index?0x140:0xa0;ULONGLONG oldSurface=0;BYTE oldFlag=0;
 if(!ReadAt(strategy,0x98,&oldSurface,8)||!ReadAt(strategy,flagOffset,&oldFlag,1)||!Writable143(strategy+0x98,8)||!Writable143(strategy+flagOffset,1))return false;
 auto& s=g_throwScope178;s={};s.binding=before;s.index=index;s.active=true;s.queuedQpc183=g_dispatchQpc183;AcquireSRWLockShared(&g_manualLock143);s.epoch179=g_placeContext177.epoch;ReleaseSRWLockShared(&g_manualLock143);memcpy(s.matrix,matrix,64);memcpy(s.velocity,velocity,12);
 bool consumed=false;
 __try{
  // Explicit native no-surface branch; held camera preview must not snap release.
  *reinterpret_cast<ULONGLONG*>(strategy+0x98)=~0ULL;*reinterpret_cast<BYTE*>(strategy+flagOffset)=0;
  g_surfaceUse177[index](reinterpret_cast<void*>(strategy),0);
  consumed=SurfacePlaced177(strategy,before.item);
 }__except(EXCEPTION_EXECUTE_HANDLER){consumed=SurfacePlaced177(strategy,before.item);}
 // The preview is recomputed next update. Restore only if native did not consume.
 if(!consumed){__try{*reinterpret_cast<ULONGLONG*>(strategy+0x98)=oldSurface;*reinterpret_cast<BYTE*>(strategy+flagOffset)=oldFlag;}__except(EXCEPTION_EXECUTE_HANDLER){}}
  if(consumed){AcquireSRWLockExclusive(&g_manualLock143);++g_placeStats177[index].consumed178;
  auto& receipt=g_placeStats177[index];receipt.physics179=s.physics179;receipt.physicsVtable179=s.physicsVtable179;receipt.velocitySetter179=s.setter179;receipt.physicsBody179=s.body179;
   // The use callback can return before the native strategy's enclosing update
   // finishes. That update may still rewrite velocity, so commit after its next
   // original update through PumpFinalVelocity179, even when the body is ready.
   auto& pending=g_deferredVelocity179[index];pending.scope=s;pending.deadline=GetTickCount64()+250;pending.active=true;++receipt.velocityDeferred179;
   memcpy(receipt.requestedVelocity179,s.velocity,12);ReleaseSRWLockExclusive(&g_manualLock143);}
  s={};return consumed;
}
void DisableThrow178(){g_throwDispatch178=nullptr;g_deferredVelocity179[0]={};g_deferredVelocity179[1]={};for(int i=0;i<4;++i){if(g_throwInstalled178[i])MH_DisableHook(reinterpret_cast<void*>(g_engine+kThrowSites178[i]));g_throwInstalled178[i]=false;}}
bool InstallThrow178(BYTE* image){
 const BYTE rigidSetter179[]={0x55,0x8b,0xec,0x83,0x79,0x54,0,0x74,0x2b};
 if(memcmp(image+0x1684a20,rigidSetter179,sizeof(rigidSetter179))||Word(reinterpret_cast<UINT>(image)+0x1f566d8,0xec)!=reinterpret_cast<UINT>(image)+0x1684a20)return false;
 const BYTE typeBytes179[]={0xb8,1,0,0,0};const BYTE lookupBytes179[]={0x55,0x8b,0xec,0x51,0x56,0x57,0x8b,0xf8,0xf7,0x47,0x60,0};
 if(memcmp(image+0x57280,typeBytes179,sizeof(typeBytes179))||memcmp(image+0x1706640,lookupBytes179,sizeof(lookupBytes179)))return false;
 for(int i=0;i<4;++i)if(memcmp(image+kThrowSites178[i],kThrowBytes178[i],6))return false;
 for(int i=0;i<2;++i)if(memcmp(image+kSurfaceUse177[i],kSurfaceUseBytes177[i],11))return false;
 void* bridges[4]={reinterpret_cast<void*>(C4Transform178),reinterpret_cast<void*>(MineTransform178),reinterpret_cast<void*>(C4Velocity178),reinterpret_cast<void*>(MineVelocity178)};
 for(int i=0;i<4;++i){if(MH_CreateHook(image+kThrowSites178[i],bridges[i],&g_throwOriginal178[i])!=MH_OK||MH_EnableHook(image+kThrowSites178[i])!=MH_OK){DisableThrow178();return false;}g_throwInstalled178[i]=true;}
 for(int i=0;i<2;++i)g_surfaceUse177[i]=reinterpret_cast<SurfaceUse177>(image+kSurfaceUse177[i]);
 g_placeDispatch177=DispatchSurface177;g_throwDispatch178=DispatchThrow178;return true;
}
