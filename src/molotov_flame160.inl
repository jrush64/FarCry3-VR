// Exact native Molotov idle-effect matrix upload. This changes the same effect's
// attachment, never entity physics, activation, world fires or particle shaders.
struct MolotovFlame160 {Prepared143 target;ULONGLONG ms=0;float matrix[16]={};bool active=false;}g_flame160;
// Receipt describes the matrix actually consumed by this exact item, not a later publication.
struct MolotovFlameReceipt169 {Prepared143 target;ULONGLONG ms=0,publishedMs=0;float source[16]={},applied[16]={};bool spawn=false;}g_flameReceipt169;
MolotovChildSnapshot178 g_flameChildren178;
void CaptureFlameChildren178(UINT effect,ULONGLONG now){
 g_flameChildren178={};g_flameChildren178.ms=now;
 const UINT count=Word(effect,0x88),children=Word(effect,0x84);
 if(!children||count>64)return;g_flameChildren178.count=count;
 for(UINT n=0;n<(count<4u?count:4u);++n){
  const UINT child=children+n*0x1b0,definition=Word(Word(child,0x174));
  auto& out=g_flameChildren178.child[n];
  out.valid=definition&&Read(child,out.before,64)&&Read(definition,out.local,64)&&ReadAt(child,0xf0,&out.scale,4)&&ReadAt(definition,0x50,&out.flags,4);
  if(out.valid)for(int k=0;k<16;++k)if(!std::isfinite(out.local[k])||!std::isfinite(out.before[k]))out.valid=false;
 }
}
unsigned g_flameUpdates160=0,g_flameSpawns160=0;
void* g_flameUpdateOriginal160=nullptr;void* g_flameSpawnOriginal160=nullptr;bool g_flameInstalled160=false;
constexpr BYTE kFlameUpdate160[]={0x0f,0x28,0x44,0x24,0x40,0x8b,0xf0,0x0f,0x29,0x06};
constexpr BYTE kFlameSpawn160[]={0xe8,0x69,0x2e,0x32,0x00,0x8d,0x8b,0x9c,0x02,0x00,0x00};
void __cdecl AttachFlame160(const UINT* saved,bool spawn){
 const UINT component=saved[4],matrix=spawn?saved[7]:saved[3]+68; // EBX, EAX or native ESP+40
 AcquireSRWLockExclusive(&g_manualLock143);const auto& f=g_flame160;ULONGLONG owner=0;const auto now=GetTickCount64();SafeGrenade155 identity;identity.target=f.target;
 if(InterlockedCompareExchange(&g_enabled,0,0)&&f.active&&now>=f.ms&&now-f.ms<=150&&
    g_context143.active&&g_context143.owner==f.target.owner&&g_context143.epoch==f.target.epoch&&now>=g_context143.ms&&now-g_context143.ms<=250&&
    !g_pending143.active&&!SameItem155(f.target,g_manualLaunched146)&&LocalIdentity143(owner)&&owner==f.target.owner&&SafeIdentity155(identity)&&
    Word(component)==g_engine+0x1fc67c0&&Word(component,4)==f.target.projectileRef&&Writable143(matrix,64)){
    float prior166[16]={};if(Read(matrix,prior166,sizeof(prior166))){g_flameReceipt169.target=f.target;g_flameReceipt169.ms=now;g_flameReceipt169.publishedMs=f.ms;g_flameReceipt169.spawn=spawn;memcpy(g_flameReceipt169.source,prior166,64);memcpy(g_flameReceipt169.applied,f.matrix,64);}else g_flameReceipt169={};
    if(!spawn)CaptureFlameChildren178(saved[7],now);else g_flameChildren178={};
    memcpy(reinterpret_cast<void*>(matrix),f.matrix,64);if(spawn)++g_flameSpawns160;else ++g_flameUpdates160;
 }
 ReleaseSRWLockExclusive(&g_manualLock143);
#ifdef FC3_THROW_PROBE_TEST
 __asm {pxor xmm0,xmm0}
 __asm {fninit}
#endif
}
#define FLAME_BRIDGE160(name,spawn,original) \
__declspec(naked) void name(){ \
 __asm pushfd \
 __asm pushad \
 __asm cld \
 __asm mov eax,esp \
 __asm sub esp,544 \
 __asm and esp,0fffffff0h \
 __asm mov [esp+512],eax \
 __asm fxsave [esp] \
 __asm push spawn \
 __asm push eax \
 __asm call AttachFlame160 \
 __asm add esp,8 \
 __asm fxrstor [esp] \
 __asm mov esp,[esp+512] \
 __asm popad \
 __asm popfd \
 __asm jmp dword ptr [original] \
}
FLAME_BRIDGE160(FlameUpdateBoundary160,0,g_flameUpdateOriginal160)
FLAME_BRIDGE160(FlameSpawnBoundary160,1,g_flameSpawnOriginal160)
#undef FLAME_BRIDGE160
