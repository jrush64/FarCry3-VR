// 181: next-item preparation and post-throw flight readback for C4/mines.
//
// Next mine. The native mine strategy (update 1231870) prepares its next item
// only for pawns WITHOUT a player (AI) or, for the player, from the throw
// animation's release event (handler 11231E10 -> 11231C10). The mod's direct native
// toss plays no animation, so that event never fires and the hand stayed empty.
// The two engine calls that event handler and the AI path both make are called
// here once per toss, inside the strategy's own update on the game thread, with
// the update's own preconditions: 1230A10 (this in EAX) takes the next item
// reference from the strategy's pool when +88 holds no entity, 1231530 (this in
// EDI) presents it in the hand. Both are no-ops on an empty pool (the engine's
// shared null reference), so an exhausted inventory still follows native rules.
// C4 (update 1236A50) prepares its own next charge and needs nothing here.
//
// Flight readback. Live179/180 proved setter receipts yet the item only
// dropped. This reads the thrown entity's actual position every strategy update
// for 1.5 s after the final velocity commit (entity +50 translation, or the
// physics proxy +68/+B0 exactly as the engine's own getter 1705E70 does) and
// logs the measured speed. If the body is not moving at the requested speed
// within the first 350 ms the committed velocity is re-applied through the
// already verified setter, a bounded number of times, and the result is logged
// either way. Nothing is written to the entity itself.
constexpr UINT kPrepareNext181=0x1230a10,kPresentNext181=0x1231530,kPositionGetter181=0x1705e70;
// 183: the C4 strategy creates its next charge inside its own update but the charge never
// showed in the hand live. 12367B0 is its present-in-hand function (this in EDI), the
// mirror of the mine's 1231530: visible, positioned, attached to the hand.
constexpr UINT kPresentC4183=0x12367b0;
constexpr BYTE kPresentC4Bytes183[15]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x8b,0x47,0x48,0x8b,0x40,0x28,0x83,0xec,0x10};
bool g_presentC4Installed183=false;
// The native backend has no logger of its own; notes queue here and the render
// frontend drains them into the log each tick (PopNativeNote181).
struct NativeNotes181 {char lines[48][320];unsigned head=0,count=0;SRWLOCK lock=SRWLOCK_INIT;}g_notes181;
void NoteNative181(const char* format,...){
 char text[320];va_list args;va_start(args,format);_vsnprintf_s(text,sizeof text,_TRUNCATE,format,args);va_end(args);
 auto& n=g_notes181;AcquireSRWLockExclusive(&n.lock);const unsigned slot=(n.head+n.count)%48;if(n.count==48)n.head=(n.head+1)%48;else ++n.count;memcpy(n.lines[slot],text,sizeof text);ReleaseSRWLockExclusive(&n.lock);
}
constexpr BYTE kPrepareBytes181[15]={0x55,0x8b,0xec,0x51,0x53,0x56,0x57,0x8b,0xf8,0x8b,0x87,0x88,0,0,0};
constexpr BYTE kPresentBytes181[15]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x8b,0x47,0x48,0x8b,0x40,0x28,0x83,0xec,0x18};
constexpr BYTE kPositionBytes181[10]={0x8b,0x4a,0x60,0xc1,0xe9,0x08,0xf6,0xc1,0x01,0x74};
bool g_nextInstalled181=false,g_positionVerified181=false;
struct PrepareRequest181 {ULONGLONG ms=0;unsigned attempts=0;bool active=false;};
PrepareRequest181 g_prepare181[2];
struct Settle181 {ThrowScope178 scope;float origin[3],start[3],last[3];double requested=0,peak=0;LONGLONG startQpc=0,lastQpc=0,lastLogQpc=0;unsigned samples=0,reapplies=0,movingSamples=0,stickChecks=0;bool active=false,moving=false,stuck=false;};
Settle181 g_settle181[2];
LONGLONG Qpc181(){LARGE_INTEGER c;QueryPerformanceCounter(&c);return c.QuadPart;}
double QpcMs181(LONGLONG from,LONGLONG to){static LONGLONG f=0;if(!f){LARGE_INTEGER q;QueryPerformanceFrequency(&q);f=q.QuadPart?q.QuadPart:1;}return double(to-from)*1000.0/double(f);}
float Length181(const float* v){return sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);}
bool ReadItemPosition181(UINT entity,float* out){
 if(!entity)return false;const UINT flags=Word(entity,0x60);UINT source=entity+0x50;
 if((flags>>8)&1){const UINT proxy=Word(entity,0x68);BYTE detached=1;if(proxy&&ReadAt(proxy,0xc4,&detached,1)&&!detached)source=proxy+0xb0;}
 if(!ReadAt(source,0,out,12))return false;
 for(int i=0;i<3;++i)if(!(out[i]==out[i])||out[i]>1e6f||out[i]<-1e6f)return false;return true;
}
void NativePrepareNext181(UINT strategy){
 const UINT prepare=g_engine+kPrepareNext181,present=g_engine+kPresentNext181;
 __asm { mov eax,strategy }
 __asm { call prepare }
 __asm { mov edi,strategy }
 __asm { call present }
}
void NativePresentC4183(UINT strategy){
 const UINT present=g_engine+kPresentC4183;
 __asm { mov edi,strategy }
 __asm { call present }
}
void PumpPresentC4183(UINT strategy){
 if(!g_presentC4Installed183)return;
 AcquireSRWLockShared(&g_manualLock143);PrepareRequest181 r=g_prepare181[0];const bool context=g_placeContext177.active;const ULONGLONG contextOwner=g_placeContext177.owner;ReleaseSRWLockShared(&g_manualLock143);
 if(!r.active)return;
 auto clear=[&](){AcquireSRWLockExclusive(&g_manualLock143);g_prepare181[0].active=false;ReleaseSRWLockExclusive(&g_manualLock143);};
 if(!InterlockedCompareExchange(&g_enabled,0,0)){clear();return;}
 const auto now=GetTickCount64();if(now<r.ms||now-r.ms>2000){clear();NoteNative181("[THROW183] C4 present request expired unused");return;}
 PlaceBinding177 b;if(!BindPlace177(strategy,C4177,b)||!context||contextOwner!=b.owner)return;
 const UINT ref=Word(strategy,0x88),entity=ref?Word(ref,0xc):0;BYTE busy148=0;ReadAt(strategy,0x148,&busy148,1);
 if(!entity){clear();NoteNative181("[THROW183] C4 strategy holds no charge entity; nothing to present");return;}
 clear();bool ok=false;
 __try{NativePresentC4183(strategy);ok=true;}__except(EXCEPTION_EXECUTE_HANDLER){ok=false;}
 NoteNative181("[THROW183] native C4 present-in-hand call %s (ref %08X entity %08X +148=%u)",ok?"made":"FAULTED",ref,entity,busy148);
}
void PumpPrepare181(UINT strategy,PlaceableKind177 kind){
 const int index=PlaceIndex177(kind);if(index==0){PumpPresentC4183(strategy);return;}if(index!=1||!g_nextInstalled181)return;
 AcquireSRWLockShared(&g_manualLock143);PrepareRequest181 r=g_prepare181[index];const bool context=g_placeContext177.active;const ULONGLONG contextOwner=g_placeContext177.owner;ReleaseSRWLockShared(&g_manualLock143);
 if(!r.active)return;
 auto clear=[&](){AcquireSRWLockExclusive(&g_manualLock143);g_prepare181[index].active=false;ReleaseSRWLockExclusive(&g_manualLock143);};
 if(!InterlockedCompareExchange(&g_enabled,0,0)){clear();return;}
 const auto now=GetTickCount64();if(now<r.ms||now-r.ms>2000){clear();NoteNative181("[THROW181] next-mine request expired unused");return;}
 PlaceBinding177 b;if(!BindPlace177(strategy,kind,b)||!context||contextOwner!=b.owner)return;
 const UINT ref=Word(strategy,0x88);if(!ref)return;
 if(Word(ref,0xc)){clear();NoteNative181("[THROW181] native mine strategy already holds an item entity (ref %08X); no prepare call needed",ref);return;}
 UINT stock=0;const bool known=ReadPlaceStock179(b,stock);
 if(known&&!stock){clear();NoteNative181("[THROW181] no mine stock left; native depletion path keeps authority");return;}
 BYTE config138=0;ReadAt(strategy,0x138,&config138,1);
 clear();bool ok=false;UINT after=0,entity=0;
 __try{NativePrepareNext181(strategy);after=Word(strategy,0x88);entity=after?Word(after,0xc):0;ok=entity!=0;}__except(EXCEPTION_EXECUTE_HANDLER){ok=false;}
 AcquireSRWLockExclusive(&g_manualLock143);if(ok)++g_placeStats177[index].prepared181;else ++g_placeStats177[index].prepareFailed181;ReleaseSRWLockExclusive(&g_manualLock143);
 NoteNative181("[THROW181] native next-mine prepare+present %s: ref %08X entity %08X stock %u known%d +138=%u (attempt %u)",ok?"produced an item in hand":"produced NO item",after,entity,stock,known?1:0,config138,r.attempts+1);
}
// 185: landing must never invoke the native held-item preview helper.
// Those helpers detach strategy+88 and move it to the surface; after a toss
// that is the NEXT item. Query against a private strategy snapshot referring
// to the released entity, then transform only that entity.
constexpr UINT kSetTransform182=0x1707a70,kPhysicsEnable182=0x1684af0;
constexpr BYTE kSetTransformBytes182[12]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x83,0xec,0x44,0x53,0x8b,0x5d};
constexpr BYTE kPhysicsEnableBytes182[10]={0x55,0x8b,0xec,0x8b,0x41,0x54,0x85,0xc0,0x74,0x0d};
bool g_stickInstalled182=false;
// 184: the engine's own surface placement (mine 11232A7F / C4 11237CF0) orients the item with
// angles = DirToAngles(surface normal) + item config angle offset (+78 -> +50/+54/+58) and
// sets them with 11707DD0 (keeps position). Normal is strategy +B8 (mine) / +BC (C4).
constexpr UINT kDirToAngles184=0x267860,kSetAngles184=0x1707dd0;
constexpr BYTE kDirToAnglesBytes184[11]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0xf3,0x0f,0x10,0x56,0x04};
constexpr BYTE kSetAnglesBytes184[12]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x83,0xec,0x54,0x53,0x8b,0xd9};
bool g_orientInstalled184=false;
using SetAngles184=void(__thiscall*)(void*,float,float,float);
void NativeDirToAngles184(const float* in,float* out){
 const UINT target=g_engine+kDirToAngles184;
 __asm { mov esi,in }
 __asm { mov edi,out }
 __asm { call target }
}
using PhysicsEnable182=void(__thiscall*)(void*,UINT);
using SetVelocity182=void(__thiscall*)(void*,const float*,UINT);
void NativeSetTransform182(UINT entity,const float* matrix){
 const UINT target=g_engine+kSetTransform182;
 __asm { mov eax,matrix }
 __asm { push 0 }
 __asm { push entity }
 __asm { call target }
}
// Both verified query routines read strategy fields only through +148 and
// write hit outputs in +98..+C7 / mine +140. They also write collision metadata
// on the entity at +88. A scratch strategy isolates BOTH classes of side effects.
// The query only reads/compares its item reference; it does not retain it.
bool ReleasedQuery185(const Settle181& w,int index,BYTE* scratch,UINT* reference){
 if(index<0||index>1)return false;
 PlaceBinding177 current;ULONGLONG id=0;
 if(!BindPlace177(w.scope.binding.strategy,index?Mine177:C4177,current)||
    !SameBinding177(w.scope.binding,current,false)||!w.scope.entity179||
    !ReadAt(w.scope.entity179,8,&id,8)||id!=w.scope.binding.item||
    !SurfacePlaced177(current.strategy,id)||
    !ReadAt(current.strategy,0,scratch,0x150))return false;
 reference[0]=UINT(id);reference[1]=UINT(id>>32);reference[2]=1;reference[3]=w.scope.entity179;
 *reinterpret_cast<UINT*>(scratch+0x88)=reinterpret_cast<UINT>(reference);
 return true;
}
bool SurfaceAhead182(Settle181& w,int index,const float* p,const float* dir,float length,float* point,ULONGLONG& surface,float* normal,bool& terrainHit){
 auto& s=g_surfaceScope177;if(index<0||index>1||s.active||!g_surfaceQuery177[index])return false;
 __declspec(align(16)) BYTE scratch[0x150];UINT reference[4];
 if(!ReleasedQuery185(w,index,scratch,reference))return false;
 const UINT strategy=reinterpret_cast<UINT>(scratch);
 s={};s.strategy=strategy;s.index=index;s.active=true;
 for(int j=0;j<3;++j){s.origin[j]=p[j]+dir[j]*.10f;s.delta[j]=dir[j]*length;}
 bool hit=false;surface=~0ULL;
 __try{g_surfaceQuery177[index](strategy);
  if(s.rewrites==1){BYTE terrain=0;ReadAt(strategy,index?0x140:0xa0,&terrain,1);ReadAt(strategy,0x98,&surface,8);terrainHit=terrain!=0;ReadAt(strategy,index?0xb8:0xbc,normal,12);
   if((terrain||surface!=~0ULL)&&surface!=w.scope.binding.item&&ReadAt(strategy,index?0xa0:0xa4,point,12)){
    const float d[3]={point[0]-p[0],point[1]-p[1],point[2]-p[2]};const float dist=Length181(d);hit=dist==dist&&dist<=length+.25f;}}
 }__except(EXCEPTION_EXECUTE_HANDLER){hit=false;}
 s={};return hit;
}
// On a floor the placeable's broad face is its local XY plane. The native
// AI wall rule maps its forward axis to the normal, tipping that face 90 degrees
// for an upward hit. Construct an orthonormal world basis with local Z = hit
// normal and keep the release heading projected onto the tangent plane.
bool GroundPose186(const float* release,const float* point,const float* normal,float* out){
 const float nz=Length181(normal);if(nz<.9f||nz>1.1f||normal[2]/nz<.75f)return false;
 float z[3]={normal[0]/nz,normal[1]/nz,normal[2]/nz};
 float x[3]={release[0],release[1],release[2]};
 float dot=x[0]*z[0]+x[1]*z[1]+x[2]*z[2];
 for(int j=0;j<3;++j)x[j]-=dot*z[j];
 float length=Length181(x);
 if(length<.1f){x[0]=1;x[1]=0;x[2]=0;dot=z[0];for(int j=0;j<3;++j)x[j]-=dot*z[j];length=Length181(x);}
 if(length<.1f)return false;
 for(int j=0;j<3;++j)x[j]/=length;
 const float y[3]={z[1]*x[2]-z[2]*x[1],z[2]*x[0]-z[0]*x[2],z[0]*x[1]-z[1]*x[0]};
 memcpy(out,release,16*sizeof(float));
 for(int j=0;j<3;++j){out[j]=x[j];out[4+j]=y[j];out[8+j]=z[j];out[12+j]=point[j];}
 out[3]=out[7]=out[11]=0;out[15]=1;
 return MatrixValid177(out);
}
void TryStick182(Settle181& w,int index,const float* p,const float* last,double speed,double t){
 if(!g_stickInstalled182||w.stuck||speed<.3||t<50||w.stickChecks>=400)return;++w.stickChecks;
 float dir[3]={p[0]-last[0],p[1]-last[1],p[2]-last[2]};const float n=Length181(dir);if(n<1e-4f)return;for(int j=0;j<3;++j)dir[j]/=n;
 float length=float(speed)*.05f+.15f;if(length<.2f)length=.2f;if(length>.8f)length=.8f;
 float point[3]={},normal[3]={};ULONGLONG surface=0;bool terrainHit=false;if(!SurfaceAhead182(w,index,p,dir,length,point,surface,normal,terrainHit))return;
 const UINT strategy=w.scope.binding.strategy,entity=w.scope.entity179;
 // Preserve release basis until the existing native surface-angle rule below.
 // Translation comes from the native hit (including its configured offset).
 float matrix[16];memcpy(matrix,w.scope.matrix,sizeof(matrix));memcpy(matrix+12,point,12);
 // Terrain and shallow ground slopes use the actual contact normal. This
 // flattens the visible face without lifting the item above the hit point.
 float groundNormal[3]={normal[0],normal[1],normal[2]};
 if(Length181(groundNormal)<.9f&&(terrainHit||surface==~0ULL)){groundNormal[0]=groundNormal[1]=0;groundNormal[2]=1;}
 const bool ground=GroundPose186(w.scope.matrix,point,groundNormal,matrix);
 if(!MatrixValid177(matrix))return;
 bool frozen=false;
 __try{const UINT physics=g_resolvePhysics179?g_resolvePhysics179(entity):0;
  if(physics&&Word(physics)==g_engine+0x1f566d8&&Word(Word(physics),0xd4)==g_engine+kPhysicsEnable182&&Word(Word(physics),0xec)==g_engine+0x1684a20){
   const float zero[3]={0,0,0};reinterpret_cast<SetVelocity182>(g_engine+0x1684a20)(reinterpret_cast<void*>(physics),zero,0);
   reinterpret_cast<PhysicsEnable182>(g_engine+kPhysicsEnable182)(reinterpret_cast<void*>(physics),0);frozen=true;}
  NativeSetTransform182(entity,matrix);w.stuck=true;
 }__except(EXCEPTION_EXECUTE_HANDLER){}
 // Orientation: the engine's own surface rule. A missing/odd normal on terrain means a floor: up.
 const char* orient="skipped";float angles[3]={};
 if(w.stuck&&g_orientInstalled184&&!ground){
  float n[3]={normal[0],normal[1],normal[2]};const float len=Length181(n);
  bool usable=len>.9f&&len<1.1f;
  if(!usable&&(terrainHit||surface==~0ULL)){n[0]=0;n[1]=0;n[2]=1;usable=true;}
  if(usable){
   if(len>.9f&&len<1.1f)for(int j=0;j<3;++j)n[j]/=len;
   // The normal must face back toward where the item came from (out of the surface).
   if(n[0]*dir[0]+n[1]*dir[1]+n[2]*dir[2]>0)for(int j=0;j<3;++j)n[j]=-n[j];
   const UINT config=Word(strategy,0x78);float offset[3]={};const bool haveOffset=config&&ReadAt(config,0x50,offset,12);
   __try{NativeDirToAngles184(n,angles);for(int j=0;j<3;++j)angles[j]+=haveOffset?offset[j]:0.f;
    reinterpret_cast<SetAngles184>(g_engine+kSetAngles184)(reinterpret_cast<void*>(entity),angles[0],angles[1],angles[2]);orient="native surface angles applied";}
   __except(EXCEPTION_EXECUTE_HANDLER){orient="FAULTED";}
   NoteNative181("[THROW184 ORIENT] kind%d normal %.2f/%.2f/%.2f (%s) -> angles %.3f/%.3f/%.3f (config offset %s): %s",index?4:3,n[0],n[1],n[2],len>.9f&&len<1.1f?"from hit":"floor up",angles[0],angles[1],angles[2],haveOffset?"yes":"no",orient);
  }else NoteNative181("[THROW184 ORIENT] kind%d no usable surface normal (%.2f/%.2f/%.2f); orientation left as placed",index?4:3,normal[0],normal[1],normal[2]);
 }
 if(w.stuck&&ground)NoteNative181("[THROW186 GROUND] kind%d normal %.3f/%.3f/%.3f; face aligned to surface at hit point",index?4:3,groundNormal[0],groundNormal[1],groundNormal[2]);
 NoteNative181("[THROW182 STICK] kind%d %s at %.3f/%.3f/%.3f (surface %016llX, hit point %.2f/%.2f/%.2f, %.0f ms, %.1f m/s, physics %s)",index?4:3,w.stuck?"placed on the surface":"placement FAILED",matrix[12],matrix[13],matrix[14],surface,point[0],point[1],point[2],t,speed,frozen?"frozen":"NOT frozen");
}
void BeginSettle181(int index,const ThrowScope178& scope){
 if(index<0||index>1)return;auto& w=g_settle181[index];w={};w.scope=scope;memcpy(w.origin,scope.matrix+12,12);w.requested=Length181(scope.velocity);
 w.startQpc=w.lastQpc=w.lastLogQpc=Qpc181();
 if(scope.queuedQpc183)NoteNative181("[THROW183 RELEASE] kind%d grip opened -> native use -> velocity commit took %.0f ms",index?4:3,QpcMs181(scope.queuedQpc183,w.startQpc));
 float p[3];if(g_positionVerified181&&ReadItemPosition181(scope.entity179,p)){
  memcpy(w.start,p,12);memcpy(w.last,p,12);w.active=true;const float d[3]={p[0]-w.origin[0],p[1]-w.origin[1],p[2]-w.origin[2]};
  NoteNative181("[THROW181 FLIGHT] kind%d entity%08X body%08X committed %.2f m/s; position now %.3f/%.3f/%.3f, release origin %.3f/%.3f/%.3f (%.2f m apart)",index?4:3,scope.entity179,scope.body179,w.requested,p[0],p[1],p[2],w.origin[0],w.origin[1],w.origin[2],Length181(d));
 }else NoteNative181("[THROW181 FLIGHT] kind%d entity%08X position unreadable; no readback (%s)",index?4:3,scope.entity179,g_positionVerified181?"entity flags":"getter layout unverified");
}
void PumpSettle181(UINT strategy,PlaceableKind177 kind,float dt){
 const int index=PlaceIndex177(kind);if(index<0)return;auto& w=g_settle181[index];if(!w.active||w.scope.binding.strategy!=strategy)return;
 const LONGLONG qpc=Qpc181();const double t=QpcMs181(w.startQpc,qpc);
 auto finish=[&](const char* why){const float d[3]={w.last[0]-w.start[0],w.last[1]-w.start[1],w.last[2]-w.start[2]};
  NoteNative181("[THROW181 FLIGHT] kind%d summary (%s): requested %.2f m/s, peak measured %.2f m/s, travelled %.2f m in %.0f ms over %u samples, velocity re-applied %u time(s), moving %s, stuck %s",index?4:3,why,w.requested,w.peak,Length181(d),t,w.samples,w.reapplies,w.moving?"YES":"NO",w.stuck?"YES":"no");w={};};
 if(!InterlockedCompareExchange(&g_enabled,0,0)){finish("disabled");return;}
 if(t>3000||w.stuck){finish(w.stuck?"stuck":"window complete");return;}
 float p[3];if(!ReadItemPosition181(w.scope.entity179,p)){finish("entity gone");return;}
 const double step=(dt>0&&dt<.25f)?double(dt):QpcMs181(w.lastQpc,qpc)/1000.0;if(step<=0)return;
 const float d[3]={p[0]-w.last[0],p[1]-w.last[1],p[2]-w.last[2]};const double speed=Length181(d)/step;
 float last[3];memcpy(last,w.last,12);
 memcpy(w.last,p,12);w.lastQpc=qpc;++w.samples;if(speed>w.peak)w.peak=speed;
 TryStick182(w,index,p,last,speed,t);if(w.stuck)return;
 const bool fast=w.requested>.5&&speed>=.5*w.requested;w.movingSamples=fast?w.movingSamples+1:0;
 if(!w.moving&&w.movingSamples>=2){w.moving=true;NoteNative181("[THROW181 FLIGHT] kind%d body moving at %.2f m/s (requested %.2f) %.0f ms after commit",index?4:3,speed,w.requested,t);}
 if(w.samples<=8||QpcMs181(w.lastLogQpc,qpc)>=100){w.lastLogQpc=qpc;NoteNative181("[THROW181 FLIGHT] kind%d t%.0fms pos %.3f/%.3f/%.3f speed %.2f m/s (requested %.2f, step %.1f ms)",index?4:3,t,p[0],p[1],p[2],speed,w.requested,step*1000);}
 if(!w.moving&&w.requested>.5&&speed<.4*w.requested&&t<=350&&w.reapplies<4){
  const bool ok=FinalizeThrowVelocity179(w.scope);
  if(ok){++w.reapplies;AcquireSRWLockExclusive(&g_manualLock143);++g_placeStats177[index].reapplied181;ReleaseSRWLockExclusive(&g_manualLock143);}
  NoteNative181("[THROW181] kind%d body only %.2f m/s against %.2f requested at %.0f ms; velocity %s (re-apply %u)",index?4:3,speed,w.requested,t,ok?"re-applied through the verified setter":"re-apply refused by identity/body checks",w.reapplies);
 }
}
void DisableNext181(){g_orientInstalled184=false;g_presentC4Installed183=false;g_stickInstalled182=false;g_nextInstalled181=false;g_positionVerified181=false;g_prepare181[0]={};g_prepare181[1]={};g_settle181[0]={};g_settle181[1]={};}
bool InstallNext181(BYTE* image){
 g_nextInstalled181=!memcmp(image+kPrepareNext181,kPrepareBytes181,sizeof(kPrepareBytes181))&&!memcmp(image+kPresentNext181,kPresentBytes181,sizeof(kPresentBytes181));
 g_positionVerified181=!memcmp(image+kPositionGetter181,kPositionBytes181,sizeof(kPositionBytes181));
 g_stickInstalled182=g_positionVerified181&&
  !memcmp(image+kSetTransform182,kSetTransformBytes182,sizeof(kSetTransformBytes182))&&!memcmp(image+kPhysicsEnable182,kPhysicsEnableBytes182,sizeof(kPhysicsEnableBytes182));
 // The stick needs the 177 surface query ray hooks, which the free-toss build never installed.
 if(g_stickInstalled182&&!g_surfaceInstalled177[0]){g_stickInstalled182=InstallSurface177(image);g_throwDispatch178=DispatchThrow178;}
 g_presentC4Installed183=!memcmp(image+kPresentC4183,kPresentC4Bytes183,sizeof(kPresentC4Bytes183));
 g_orientInstalled184=!memcmp(image+kDirToAngles184,kDirToAnglesBytes184,sizeof(kDirToAnglesBytes184))&&!memcmp(image+kSetAngles184,kSetAnglesBytes184,sizeof(kSetAnglesBytes184));
 NoteNative181("[THROW185] landing isolated from replacement item; held preview helper removed");
 NoteNative181("[THROW184] native surface orientation %s",g_orientInstalled184?"verified":"NOT verified; stuck items keep the preview orientation");
 NoteNative181("[THROW182] surface sticking %s; C4 present-in-hand %s",g_stickInstalled182?"verified (released-item query, transform setter, physics enable)":"NOT verified; thrown items keep free physics",g_presentC4Installed183?"verified":"NOT verified");
 NoteNative181("[THROW181] native next-mine calls %s; entity position layout %s",g_nextInstalled181?"verified":"NOT verified (next mine stays native-event driven)",g_positionVerified181?"verified":"NOT verified (no flight readback)");
 return g_nextInstalled181;
}
