// Native hand-local surface placement. Only stack-local ray inputs change;
// the engine retains collision filtering, attachment identity and inventory commit.
constexpr UINT kSurfaceRay177[2]={0x1234db2,0x122fd79};
constexpr UINT kSurfaceQuery177[2]={0x1234bc0,0x122fb90};
constexpr UINT kSurfaceUse177[2]={0x12383a0,0x1233340};
constexpr BYTE kSurfaceRayBytes177[2][5]={{0xe8,0xe9,0xb7,0x21,0},{0xe8,0x22,0x08,0x22,0}};
constexpr BYTE kSurfaceQueryBytes177[2][12]={{0x55,0x8b,0xec,0x83,0xe4,0xf0,0x81,0xec,0x04,0x01,0,0},{0x55,0x8b,0xec,0x83,0xe4,0xf0,0x81,0xec,0x44,0x01,0,0}};
constexpr BYTE kSurfaceUseBytes177[2][11]={{0x51,0xe8,0xea,0xf2,0xff,0xff,0xb0,0x01,0xc2,4,0},{0x51,0xe8,0xea,0xef,0xff,0xff,0xb0,0x01,0xc2,4,0}};
void* g_surfaceOriginal177[2]={};bool g_surfaceInstalled177[2]={};
struct SurfaceScope177 {UINT strategy;int index;float origin[3],delta[3];unsigned rewrites;bool active;};
__declspec(thread) SurfaceScope177 g_surfaceScope177={};
using SurfaceQuery177=bool(__stdcall*)(UINT);
using SurfaceUse177=bool(__thiscall*)(void*,UINT);
SurfaceQuery177 g_surfaceQuery177[2]={};SurfaceUse177 g_surfaceUse177[2]={};
void RewriteSurface177(unsigned index,UINT* regs){
 auto& s=g_surfaceScope177;if(!s.active||s.index!=static_cast<int>(index)||!regs||regs[4]!=s.strategy||!InterlockedCompareExchange(&g_enabled,0,0))return;
 const UINT stack=regs[3]+4,origin=regs[7],delta=regs[1];
 // Both pointers are local arrays in this exact native query stack frame.
 if(origin<stack||origin-stack>2048||delta<stack||delta-stack>2048||origin==delta)return;
 __try {memcpy(reinterpret_cast<void*>(origin),s.origin,12);memcpy(reinterpret_cast<void*>(delta),s.delta,12);++s.rewrites;}__except(EXCEPTION_EXECUTE_HANDLER){}
}
#define SURFACE_BOUNDARY177(name,index,slot) \
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
 __asm call RewriteSurface177 \
 __asm add esp,8 \
 __asm fxrstor [esp] \
 __asm mov esp,[esp+512] \
 __asm popad \
 __asm popfd \
 __asm jmp dword ptr [g_surfaceOriginal177+slot] \
}
SURFACE_BOUNDARY177(C4Surface177,0,0)
SURFACE_BOUNDARY177(MineSurface177,1,4)
#undef SURFACE_BOUNDARY177
bool SurfaceHit177(UINT strategy,int index,const float* hand,float& distance){
 BYTE terrain=0;ULONGLONG surface=~0ULL;float hit[3]={};
 if(!ReadAt(strategy,index?0x140:0xa0,&terrain,1)||!ReadAt(strategy,0x98,&surface,8)||(!terrain&&surface==~0ULL)||
    !ReadAt(strategy,index?0xa0:0xa4,hit,12))return false;
 distance=0;for(int i=0;i<3;++i){float d=hit[i]-hand[i];if(!(d==d)||d>1.f||d< -1.f)return false;distance+=d*d;}
 return distance<=.09f;
}
bool SurfacePlaced177(UINT strategy,ULONGLONG item){
 const UINT count=Word(strategy,0x80),list=Word(strategy,0x7c);if(!list||!count||count>64)return false;
 for(UINT i=0;i<count;++i){ULONGLONG id=0;if(ReadAt(list,i*8,&id,8)&&id==item)return true;}return false;
}
bool DispatchSurface177(UINT strategy,PlaceableKind177 kind,const float* matrix){
 const int index=PlaceIndex177(kind);PlaceBinding177 before,after;
 if(index<0||g_surfaceScope177.active||!MatrixValid177(matrix)||!BindPlace177(strategy,kind,before)||!before.item||
    !g_surfaceQuery177[index]||!g_surfaceUse177[index]||SurfacePlaced177(strategy,before.item))return false;
 auto& s=g_surfaceScope177;s={};s.strategy=strategy;s.index=index;s.active=true;
 bool committed=false;int best=-1;float bestDistance=1.f;
 __try{
  // Six short rays allow contact on walls/floors without native camera aiming.
  for(int ray=0;ray<6;++ray){const int axis=ray/2;const float sign=(ray&1)?-1.f:1.f;
   for(int j=0;j<3;++j){float dir=matrix[axis*4+j]*sign;s.origin[j]=matrix[12+j]-dir*.025f;s.delta[j]=dir*.275f;}
   s.rewrites=0;g_surfaceQuery177[index](strategy);float distance=0;
   if(s.rewrites==1&&SurfaceHit177(strategy,index,matrix+12,distance)&&distance<bestDistance){best=ray;bestDistance=distance;}
  }
  if(best>=0){for(int j=0;j<3;++j){float dir=matrix[(best/2)*4+j]*((best&1)?-1.f:1.f);s.origin[j]=matrix[12+j]-dir*.025f;s.delta[j]=dir*.275f;}
   s.rewrites=0;g_surfaceQuery177[index](strategy);float distance=0;
   if(s.rewrites==1&&SurfaceHit177(strategy,index,matrix+12,distance)&&BindPlace177(strategy,kind,after)&&SameBinding177(before,after,true)){
    // Native use consumes the query's genuine surface GUID, normal and point.
    g_surfaceUse177[index](reinterpret_cast<void*>(strategy),0);committed=SurfacePlaced177(strategy,before.item);
   }
  }
 }__except(EXCEPTION_EXECUTE_HANDLER){committed=false;}
 s={};return committed;
}
void DisableSurface177(){g_placeDispatch177=nullptr;for(int i=0;i<2;++i){if(g_surfaceInstalled177[i])MH_DisableHook(reinterpret_cast<void*>(g_engine+kSurfaceRay177[i]));g_surfaceInstalled177[i]=false;g_surfaceQuery177[i]=nullptr;g_surfaceUse177[i]=nullptr;}}
bool InstallSurface177(BYTE* image){
 for(int i=0;i<2;++i)if(memcmp(image+kSurfaceRay177[i],kSurfaceRayBytes177[i],5)||memcmp(image+kSurfaceQuery177[i],kSurfaceQueryBytes177[i],12)||memcmp(image+kSurfaceUse177[i],kSurfaceUseBytes177[i],11))return false;
 void* bridge[2]={reinterpret_cast<void*>(C4Surface177),reinterpret_cast<void*>(MineSurface177)};
 for(int i=0;i<2;++i){if(MH_CreateHook(image+kSurfaceRay177[i],bridge[i],&g_surfaceOriginal177[i])!=MH_OK||MH_EnableHook(image+kSurfaceRay177[i])!=MH_OK){for(int j=0;j<=i;++j){MH_DisableHook(image+kSurfaceRay177[j]);g_surfaceInstalled177[j]=false;}return false;}
  g_surfaceInstalled177[i]=true;g_surfaceQuery177[i]=reinterpret_cast<SurfaceQuery177>(image+kSurfaceQuery177[i]);g_surfaceUse177[i]=reinterpret_cast<SurfaceUse177>(image+kSurfaceUse177[i]);}
 g_placeDispatch177=DispatchSurface177;return true;
}
