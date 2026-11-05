// Separate C4/mine strategy service. Native updates retain all original behavior.
// Queue dispatch is admitted only with an exact local pawn/equipped/prepared identity.
namespace {
constexpr UINT kPlaceUpdate177[2]={0x1236a50,0x1231870};
constexpr UINT kPlaceVtable177[2]={0x1fd52a8,0x1fd54a0};
constexpr BYTE kPlaceUpdateBytes177[2][11]={
 {0x55,0x8b,0xec,0x83,0xe4,0xf8,0xf3,0x0f,0x10,0x45,0x08},
 {0x55,0x8b,0xec,0x83,0xe4,0xf8,0xf3,0x0f,0x10,0x45,0x08}};
constexpr BYTE kRemoteBytes177[11]={0x51,0xe8,0x6a,0xfd,0xff,0xff,0xb0,0x01,0xc2,0x04,0x00};
void* g_placeOriginal177[2]={}; bool g_placeInstalled177[2]={};
using PlaceUpdate177=void(__thiscall*)(void*,float);
using PlaceDispatch177=bool(*)(UINT,PlaceableKind177,const float*);
// Set only after placement/native inventory ABI is verified, never from an INI.
PlaceDispatch177 g_placeDispatch177=nullptr;
using ThrowDispatch178=bool(*)(UINT,PlaceableKind177,const float*,const float*);
ThrowDispatch178 g_throwDispatch178=nullptr;
using RemoteDispatch177=bool(__thiscall*)(void*,UINT); RemoteDispatch177 g_remoteDispatch177=nullptr;
struct PlaceBinding177 {
 UINT strategy=0,wrapper=0,pawnReference=0,equippedReference=0,reference=0;
 ULONGLONG owner=0,equipped=0,item=0,ms=0;
};
PlaceBinding177 g_placeBinding177[2]; PlaceableStatus177 g_placeStats177[2];
Context143 g_placeContext177;
struct PlaceTicket177 {PlaceBinding177 binding;float matrix[16]={},velocity178[3]={};bool throwing178=false;unsigned epoch=0;ULONGLONG ms=0;PlaceableKind177 kind=None177;bool detonate=false,active=false;LONGLONG qpc183=0;}g_placeTicket177;
LONGLONG QpcNow183(){LARGE_INTEGER c;QueryPerformanceCounter(&c);return c.QuadPart;}
__declspec(thread) LONGLONG g_dispatchQpc183=0;
int PlaceIndex177(PlaceableKind177 kind){return kind==C4177?0:kind==Mine177?1:-1;}
bool Reference177(UINT ref,ULONGLONG& id){ULONGLONG entityId=0;const UINT entity=Word(ref,12);
 return ref&&entity&&ReadAt(ref,0,&id,8)&&id&&id!=~0ULL&&ReadAt(entity,8,&entityId,8)&&entityId==id;}
bool BindPlace177(UINT strategy,PlaceableKind177 kind,PlaceBinding177& out){
 const int index=PlaceIndex177(kind);if(index<0||!g_engine||Word(strategy)!=g_engine+kPlaceVtable177[index])return false;
 PlaceBinding177 b;b.strategy=strategy;b.wrapper=Word(strategy,0x48);b.pawnReference=Word(b.wrapper,0x28);b.equippedReference=Word(b.wrapper,0x24);
 ULONGLONG local=0;if(!Reference177(b.pawnReference,b.owner)||!Reference177(b.equippedReference,b.equipped)||!LocalIdentity143(local)||local!=b.owner)return false;
 b.reference=Word(strategy,0x88);if(!Reference177(b.reference,b.item))b.item=0;b.ms=GetTickCount64();out=b;return true;
}
bool SameBinding177(const PlaceBinding177& a,const PlaceBinding177& b,bool item){return a.strategy==b.strategy&&a.wrapper==b.wrapper&&
 a.pawnReference==b.pawnReference&&a.owner==b.owner&&a.equippedReference==b.equippedReference&&a.equipped==b.equipped&&
 (!item||(a.reference==b.reference&&a.item&&a.item==b.item));}
bool MatrixValid177(const float* m){if(!m)return false;for(int i=0;i<16;++i)if(!(m[i]==m[i])||m[i]>10000000.f||m[i]<-10000000.f)return false;
 if(m[3]!=0.f||m[7]!=0.f||m[11]!=0.f||m[15]!=1.f)return false;
 for(int c=0;c<3;++c){const float q=m[c*4]*m[c*4]+m[c*4+1]*m[c*4+1]+m[c*4+2]*m[c*4+2];if(q<.5f||q>1.5f)return false;}return true;}
bool PlaceContextValid177(const PlaceBinding177& b,unsigned epoch,ULONGLONG now){return g_placeContext177.active&&
 g_placeContext177.owner==b.owner&&g_placeContext177.epoch==epoch&&now>=g_placeContext177.ms&&now-g_placeContext177.ms<=250;}
void PumpPlace177(UINT strategy,PlaceableKind177 kind){
 if(!InterlockedCompareExchange(&g_enabled,0,0))return;
 PlaceBinding177 binding;if(!BindPlace177(strategy,kind,binding))return;const int index=PlaceIndex177(kind);PlaceTicket177 dispatch;
 AcquireSRWLockExclusive(&g_manualLock143);g_placeBinding177[index]=binding;
 auto& stats=g_placeStats177[index];if(stats.observed!=~0u)++stats.observed;
 if(g_placeTicket177.active&&g_placeTicket177.kind==kind){const auto& t=g_placeTicket177;const auto now=GetTickCount64();
  const bool valid=SameBinding177(t.binding,binding,!t.detonate)&&PlaceContextValid177(binding,t.epoch,now)&&now>=t.ms&&now-t.ms<=250;
  if(valid)dispatch=t;else ++stats.rejected;g_placeTicket177.active=false;
 }ReleaseSRWLockExclusive(&g_manualLock143);
 // No lock held during native code: event callbacks may re-enter the service.
 if(dispatch.active){bool ok=false;
  if(dispatch.detonate){
   const UINT count=Word(strategy,0x80),list=Word(strategy,0x7c);
   if(count&&count<=64&&list){__try{if(g_remoteDispatch177){g_remoteDispatch177(reinterpret_cast<void*>(strategy),0);ok=Word(strategy,0x80)<count;}}__except(EXCEPTION_EXECUTE_HANDLER){ok=false;}}
  }else if(dispatch.throwing178&&g_throwDispatch178){g_dispatchQpc183=dispatch.qpc183;ok=g_throwDispatch178(strategy,kind,dispatch.matrix,dispatch.velocity178);g_dispatchQpc183=0;}
  else if(g_placeDispatch177)ok=g_placeDispatch177(strategy,kind,dispatch.matrix);
  AcquireSRWLockExclusive(&g_manualLock143);if(ok){++stats.completed;if(dispatch.detonate)++stats.detonated;}else ++stats.rejected;ReleaseSRWLockExclusive(&g_manualLock143);
 }
}
void SamplePlaceStock179(UINT strategy,PlaceableKind177 kind);
void PumpFinalVelocity179(UINT strategy,PlaceableKind177 kind);
void PumpSettle181(UINT strategy,PlaceableKind177 kind,float dt);
void PumpPrepare181(UINT strategy,PlaceableKind177 kind);
void __fastcall C4Update177(void* self,void*,float dt){PumpPlace177(reinterpret_cast<UINT>(self),C4177);auto original=reinterpret_cast<PlaceUpdate177>(g_placeOriginal177[0]);if(original)original(self,dt);PumpFinalVelocity179(reinterpret_cast<UINT>(self),C4177);PumpSettle181(reinterpret_cast<UINT>(self),C4177,dt);PumpPrepare181(reinterpret_cast<UINT>(self),C4177);SamplePlaceStock179(reinterpret_cast<UINT>(self),C4177);}
void __fastcall MineUpdate177(void* self,void*,float dt){PumpPlace177(reinterpret_cast<UINT>(self),Mine177);auto original=reinterpret_cast<PlaceUpdate177>(g_placeOriginal177[1]);if(original)original(self,dt);PumpFinalVelocity179(reinterpret_cast<UINT>(self),Mine177);PumpSettle181(reinterpret_cast<UINT>(self),Mine177,dt);PumpPrepare181(reinterpret_cast<UINT>(self),Mine177);SamplePlaceStock179(reinterpret_cast<UINT>(self),Mine177);}
#include "placeable_surface177.inl"
#include "placeable_throw178.inl"
#include "placeable_stock179.inl"
#include "placeable_next181.inl"
bool InstallPlaceables177(BYTE* image){
 if(memcmp(image+0x1234190,kRemoteBytes177,sizeof(kRemoteBytes177)))return false;
 for(int i=0;i<2;++i)if(memcmp(image+kPlaceUpdate177[i],kPlaceUpdateBytes177[i],11)||Word(reinterpret_cast<UINT>(image)+kPlaceVtable177[i],0x10)!=reinterpret_cast<UINT>(image)+kPlaceUpdate177[i])return false;
 void* bridges[2]={reinterpret_cast<void*>(C4Update177),reinterpret_cast<void*>(MineUpdate177)};
 for(int i=0;i<2;++i){if(MH_CreateHook(image+kPlaceUpdate177[i],bridges[i],&g_placeOriginal177[i])!=MH_OK||MH_EnableHook(image+kPlaceUpdate177[i])!=MH_OK){
  for(int j=0;j<=i;++j){MH_DisableHook(image+kPlaceUpdate177[j]);g_placeInstalled177[j]=false;}return false;}g_placeInstalled177[i]=true;}
 g_remoteDispatch177=reinterpret_cast<RemoteDispatch177>(reinterpret_cast<UINT>(image)+0x1234190);const bool ok=InstallThrow178(image)&&InstallStock179(image);
 if(ok)InstallNext181(image);return ok;
}
void DisablePlaceables177(){DisableNext181();DisableStock179();DisableThrow178();DisableSurface177();for(int i=0;i<2;++i){if(g_placeInstalled177[i])MH_DisableHook(reinterpret_cast<void*>(g_engine+kPlaceUpdate177[i]));g_placeInstalled177[i]=false;}CancelPlaceable177();}
}
void PublishPlaceableContext177(ULONGLONG owner,unsigned epoch,bool active){AcquireSRWLockExclusive(&g_manualLock143);
 if(!active||g_placeContext177.owner!=owner||g_placeContext177.epoch!=epoch)g_placeTicket177.active=false;
 g_placeContext177.owner=owner;g_placeContext177.epoch=epoch;g_placeContext177.ms=GetTickCount64();g_placeContext177.active=active;ReleaseSRWLockExclusive(&g_manualLock143);}
PlaceableStatus177 PlaceableNativeStatus177(PlaceableKind177 kind){PlaceableStatus177 out;const int index=PlaceIndex177(kind);if(index<0)return out;
 AcquireSRWLockShared(&g_manualLock143);out=g_placeStats177[index];const auto b=g_placeBinding177[index];out.strategy=b.strategy;out.reference=b.reference;out.owner=b.owner;out.item=b.item;out.observedMs=b.ms;
 out.supported=InterlockedCompareExchange(&g_enabled,0,0)&&g_placeInstalled177[index]&&g_placeDispatch177;
 const auto now=GetTickCount64();PlaceBinding177 current;
 out.prepared=out.supported&&now>=b.ms&&now-b.ms<=250&&BindPlace177(b.strategy,kind,current)&&SameBinding177(b,current,false)&&(current.item||(kind==C4177&&Word(current.strategy,0x80)>0));
 out.pending=g_placeTicket177.active&&g_placeTicket177.kind==kind;ReleaseSRWLockShared(&g_manualLock143);return out;}
bool QueuePlaceableInternal178(PlaceableKind177 kind,ULONGLONG owner,unsigned epoch,const float* matrix,const float* velocity){const int index=PlaceIndex177(kind);if(index<0||!MatrixValid177(matrix))return false;
 AcquireSRWLockExclusive(&g_manualLock143);const auto b=g_placeBinding177[index];PlaceBinding177 current;const auto now=GetTickCount64();
 const bool valid=InterlockedCompareExchange(&g_enabled,0,0)&&g_placeInstalled177[index]&&g_placeDispatch177&&!g_placeTicket177.active&&owner==b.owner&&b.item&&
 now>=b.ms&&now-b.ms<=250&&PlaceContextValid177(b,epoch,now)&&BindPlace177(b.strategy,kind,current)&&SameBinding177(b,current,true);
 if(valid){g_placeTicket177={};auto& t=g_placeTicket177;t.binding=b;t.epoch=epoch;t.ms=now;t.kind=kind;t.active=true;t.qpc183=QpcNow183();memcpy(t.matrix,matrix,64);if(velocity){memcpy(t.velocity178,velocity,12);t.throwing178=true;}}else ++g_placeStats177[index].rejected;
 ReleaseSRWLockExclusive(&g_manualLock143);return valid;}
bool QueuePlaceable177(PlaceableKind177 kind,ULONGLONG owner,unsigned epoch,const float* matrix){return QueuePlaceableInternal178(kind,owner,epoch,matrix,nullptr);}
bool QueueThrowPlaceable178(PlaceableKind177 kind,ULONGLONG owner,unsigned epoch,const float* matrix,const float* velocity){
 if(!g_throwDispatch178||!velocity)return false;float q=0;for(int i=0;i<3;++i){if(!(velocity[i]==velocity[i])||velocity[i]>35||velocity[i]<-35)return false;q+=velocity[i]*velocity[i];}
 return q<=1225.f&&QueuePlaceableInternal178(kind,owner,epoch,matrix,velocity);
}
bool QueueDetonate177(ULONGLONG owner,unsigned epoch){AcquireSRWLockExclusive(&g_manualLock143);const auto b=g_placeBinding177[0];PlaceBinding177 current;const auto now=GetTickCount64();
 const UINT count=Word(b.strategy,0x80);const bool valid=InterlockedCompareExchange(&g_enabled,0,0)&&g_placeInstalled177[0]&&g_placeDispatch177&&!g_placeTicket177.active&&owner==b.owner&&
 now>=b.ms&&now-b.ms<=250&&PlaceContextValid177(b,epoch,now)&&count&&count<=64&&Word(b.strategy,0x7c)&&BindPlace177(b.strategy,C4177,current)&&SameBinding177(b,current,false);
 if(valid){g_placeTicket177={};auto& t=g_placeTicket177;t.binding=b;t.epoch=epoch;t.ms=now;t.kind=C4177;t.detonate=true;t.active=true;}else ++g_placeStats177[0].rejected;
 ReleaseSRWLockExclusive(&g_manualLock143);return valid;}
void CancelPlaceable177(){AcquireSRWLockExclusive(&g_manualLock143);g_placeTicket177.active=false;g_placeContext177.active=false;ReleaseSRWLockExclusive(&g_manualLock143);}
bool PopNativeNote181(char* out,unsigned bytes){auto& n=g_notes181;if(!out||!bytes)return false;AcquireSRWLockExclusive(&n.lock);const bool any=n.count>0;if(any){strncpy_s(out,bytes,n.lines[n.head],_TRUNCATE);n.head=(n.head+1)%48;--n.count;}ReleaseSRWLockExclusive(&n.lock);return any;}
void RequestPreparePlaceable181(PlaceableKind177 kind){const int index=PlaceIndex177(kind);if(index<0)return;AcquireSRWLockExclusive(&g_manualLock143);g_prepare181[index].ms=GetTickCount64();g_prepare181[index].active=true;ReleaseSRWLockExclusive(&g_manualLock143);}

