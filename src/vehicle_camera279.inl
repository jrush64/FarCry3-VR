// Included inside aim_probe.cpp's private namespace, after Sample/NativeSignatures.
// The camera's stack matrix is composed AFTER seat-local angle limits and
// BEFORE matrix->Euler and the native camera publication. No pawn fields persist.
namespace vehicle279 {
SRWLOCK lock=SRWLOCK_INIT;
struct Command {UINT owner[2]={};float head[3][3]={},observed[3][3]={},offset280[3][3]={{1,0,0},{0,1,0},{0,0,1}},seed281[3][3]={};ULONGLONG ms=0;bool active=false,observedValid=false,seedValid281=false,handoff284=false;int lane280=1,action282=0;UINT actionEpoch282=0;} command;
struct Receipt {UINT owner[2]={};float body[3][3]={},target[3][3]={},sourceForward283[3]={};ULONGLONG ms=0;UINT serial=0,camera284=0;int lane280=1,action282=0;UINT actionEpoch282=0;} receipts[32];
UINT serial=0;void* original=nullptr;bool installed=false,attempted=false,acquired=false;
UINT acquiredCamera284=0;volatile LONG matchedSerial284=0;
ULONGLONG logged=0;LONG applied=0,rejected=0;int loggedLane280=0;
bool Action282(const Sample& s,ULONGLONG now){
 const int a=s.cameraAction282;
 return s.valid&&s.vehicle==0&&s.slide280!=1&&s.actionEpoch282&&fc3aim::SpecificNativeAction283(a)&&now>=s.ms&&now-s.ms<=250;
}
int Lane281(const Sample& sample,bool nativeFoot){return Action282(sample,GetTickCount64())?4:sample.vehicle==1?1:sample.vehicle==0&&sample.slide280==1?2:sample.vehicle==0&&nativeFoot?3:0;}
bool Owns281(const Command& c,const Sample& s,ULONGLONG now){
 const bool action=c.lane280==4&&Action282(s,now)&&c.action282==s.cameraAction282&&c.actionEpoch282==s.actionEpoch282;
 const bool handoff=c.lane280==3&&c.handoff284&&s.cameraAction282==0&&c.actionEpoch282==s.actionEpoch282;
 return c.active&&s.valid&&(c.lane280==4?action:c.handoff284?handoff:s.action128<=0)&&now>=s.ms&&now-s.ms<=250&&now>=c.ms&&now-c.ms<=150&&
  c.owner[0]==s.handle[0]&&c.owner[1]==s.handle[1]&&c.lane280==Lane281(s,c.lane280==3);
}
bool FootOwns281(const Sample& sample,ULONGLONG now){
 AcquireSRWLockShared(&lock);const bool owns=command.lane280==3&&Owns281(command,sample,now);ReleaseSRWLockShared(&lock);return owns;
}
bool HandoffOwns284(const Sample& sample,ULONGLONG now){
 AcquireSRWLockShared(&lock);const bool owns=command.handoff284&&Owns281(command,sample,now);ReleaseSRWLockShared(&lock);return owns;
}
// Called when the native input sample publishes the end of an action, before
// the next native camera update/culling. No engine memory is written here.
bool PromoteExit284(const Sample& sample,ULONGLONG now){
 if(!sample.valid||sample.cameraAction282!=0||sample.action128<=0)return false;
 AcquireSRWLockExclusive(&lock);const Command old=command;const UINT matched=UINT(InterlockedCompareExchange(&matchedSerial284,0,0));
 const Receipt& r=receipts[matched%32];UINT nextEpoch=old.actionEpoch282+1;if(!nextEpoch)++nextEpoch;
 const bool valid=old.active&&old.lane280==4&&acquired&&acquiredCamera284&&matched&&r.serial==matched&&r.camera284==acquiredCamera284&&r.lane280==4&&
  r.actionEpoch282==old.actionEpoch282&&r.action282==old.action282&&r.owner[0]==old.owner[0]&&r.owner[1]==old.owner[1]&&
  sample.valid&&sample.vehicle==0&&sample.slide280!=1&&sample.cameraAction282==0&&sample.action128==old.action282&&sample.actionEpoch282==nextEpoch&&
  sample.handle[0]==old.owner[0]&&sample.handle[1]==old.owner[1]&&now>=sample.ms&&now-sample.ms<=250&&
  now>=old.ms&&now-old.ms<=150&&now>=r.ms&&now-r.ms<=250;
 if(valid){
  Command next=old;next.lane280=3;next.action282=0;next.actionEpoch282=sample.actionEpoch282;next.handoff284=true;next.seedValid281=false;
  memset(next.offset280,0,sizeof(next.offset280));for(int i=0;i<3;++i)next.offset280[i][i]=1;
  command=next;memset(receipts,0,sizeof(receipts));InterlockedExchange(&matchedSerial284,0);
 }
 ReleaseSRWLockExclusive(&lock);return valid;
}
bool Basis(const float (&b)[3][3]){
 for(int i=0;i<3;++i){double n=0;for(int j=0;j<3;++j){if(!std::isfinite(b[i][j]))return false;n+=double(b[i][j])*b[i][j];}if(fabs(n-1)>.02)return false;
  for(int k=0;k<i;++k){double d=0;for(int j=0;j<3;++j)d+=double(b[i][j])*b[k][j];if(fabs(d)>.01)return false;}}
 const double det=b[0][0]*(b[1][1]*b[2][2]-b[1][2]*b[2][1])-b[0][1]*(b[1][0]*b[2][2]-b[1][2]*b[2][0])+b[0][2]*(b[1][0]*b[2][1]-b[1][1]*b[2][0]);
 return det>.98&&det<1.02;
}
// Native matrix rows are local X/right, Y/forward, Z/up in world space.
// The mod's body rows are right/up/forward; native X,Y,Z is a proper rotation.
bool Compose(const float* matrix,const Command& c,Receipt& out,float (&target)[3][3]){
 float native[3][3];for(int i=0;i<3;++i)for(int j=0;j<3;++j)native[i][j]=matrix[i*4+j];
 if(!Basis(native)||!Basis(c.head))return false;
 for(int j=0;j<3;++j)out.sourceForward283[j]=native[1][j];
 const int row[3]={0,2,1};
 for(int i=0;i<3;++i)for(int j=0;j<3;++j){
  if(c.lane280==2||c.lane280==3){double v=0;for(int k=0;k<3;++k)v+=c.offset280[i][k]*native[row[k]][j];out.body[i][j]=float(v);}
  else out.body[i][j]=native[row[i]][j];
 }
 for(int i=0;i<3;++i)for(int j=0;j<3;++j){double v=0;for(int k=0;k<3;++k)v+=c.head[i][k]*out.body[k][j];out.target[i][j]=float(v);}
 for(int i=0;i<3;++i)for(int j=0;j<3;++j)target[i][j]=out.target[row[i]][j];
 return Basis(target);
}
void Apply(UINT camera,float* matrix){
 const ULONGLONG now=GetTickCount64();
 if(!camera||!matrix||!TryAcquireSRWLockShared(&g_nativeLock))return;
 const Sample sample=g_sample;ReleaseSRWLockShared(&g_nativeLock);
 if(!TryAcquireSRWLockShared(&lock))return;const Command c=command;const bool bound=acquired;const UINT boundCamera284=acquiredCamera284;ReleaseSRWLockShared(&lock);
 if(!Owns281(c,sample,now))return;
 if(c.handoff284&&camera!=boundCamera284)return;
 Receipt r;float target[3][3];bool valid=false;
 __try {
  // 1416730 resolves precisely this owner handle to the pawn. No foreign camera.
  const UINT* owner=reinterpret_cast<const UINT*>(camera+0x18);
  valid=*reinterpret_cast<const UINT*>(camera)==g_engine+0x1f8c9f0&&owner[0]==c.owner[0]&&owner[1]==c.owner[1]&&Compose(matrix,c,r,target);
 } __except(EXCEPTION_EXECUTE_HANDLER){valid=false;}
 if(valid&&!bound){
  // Ownership alone does not establish that this is the active render view.
  // Before the first write, its untouched axes must match an observed draw.
  const int row[3]={0,2,1};double error=0;for(int i=0;i<3;++i)for(int j=0;j<3;++j){const double d=matrix[row[i]*4+j]-c.observed[i][j];error+=d*d;}
  valid=c.observedValid&&std::isfinite(error)&&error<.02;
 }
 if(!valid){const LONG n=InterlockedIncrement(&rejected);if(!logged||now-logged>=1000){logged=now;fc3::Log("[VEHICLE279] native owner/view gate refused camera=%08X count=%ld; no matrix write",camera,n);}return;}
 if(!TryAcquireSRWLockExclusive(&lock))return;
 // A pause/exit must be able to cancel a command while the hook was validating.
 if(!command.active||command.lane280!=c.lane280||command.ms!=c.ms||command.actionEpoch282!=c.actionEpoch282||command.owner[0]!=c.owner[0]||command.owner[1]!=c.owner[1]){ReleaseSRWLockExclusive(&lock);return;}
 r.owner[0]=c.owner[0];r.owner[1]=c.owner[1];r.ms=now;r.serial=++serial;r.camera284=camera;r.lane280=c.lane280;r.action282=c.action282;r.actionEpoch282=c.actionEpoch282;
 // Only the nine orientation floats change; native translation/padding remain.
 __try {for(int i=0;i<3;++i)for(int j=0;j<3;++j)matrix[i*4+j]=target[i][j];}
 __except(EXCEPTION_EXECUTE_HANDLER){ReleaseSRWLockExclusive(&lock);return;}
 receipts[r.serial%32]=r;acquired=true;acquiredCamera284=camera;ReleaseSRWLockExclusive(&lock);InterlockedIncrement(&applied);
 if(c.lane280!=loggedLane280||!logged||now-logged>=1000){loggedLane280=c.lane280;logged=now;fc3::Log("[VEHICLE279] native camera matrix composed serial=%u lane=%d camera=%08X owner=%08X:%08X baseForward=%.6f/%.6f/%.6f targetForward=%.6f/%.6f/%.6f applied=%ld rejected=%ld",r.serial,c.lane280,camera,r.owner[0],r.owner[1],r.body[2][0],r.body[2][1],r.body[2][2],r.target[2][0],r.target[2][1],r.target[2][2],applied,rejected);}
}
// 14170AB: native ESP is 16-byte aligned, EBX = CCameraPawnComponent,
// [ESP+C0] is the native world rotation from 5400. Keep all CPU/FPU state.
__declspec(naked) void Boundary(){
 __asm {
  pushfd
  pushad
  mov eax,esp
  lea edx,[eax+36]
  sub esp,544
  and esp,0fffffff0h
  mov [esp+512],eax
  fxsave [esp]
  add edx,0c0h
  push edx
  push ebx
  call Apply
  add esp,8
  fxrstor [esp]
  mov esp,[esp+512]
  popad
  popfd
  jmp dword ptr [original]
 }
}
bool Signatures(const BYTE* base){
 const BYTE update[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x81,0xec,0x04,0x01,0,0,0x53,0x8b,0xd9};
 const BYTE site[]={0x8d,0x84,0x24,0xc0,0,0,0,0x8d,0xb4,0x24,0x80,0,0,0};
 const BYTE owner[]={0x8b,0x48,0x18,0x23,0x48,0x1c};
 const BYTE consume[]={0xf3,0x0f,0x10,0x84,0x24,0xc0,0,0,0,0x8b,0x4c,0x24,0x4c,0x8b,0x7c,0x24,0x68};
 auto call=[&](UINT r,UINT dest){int d=0;memcpy(&d,base+r+1,4);return base[r]==0xe8&&r+5+d==dest;};
 __try {
 return !memcmp(base+0x1416bc0,update,sizeof(update))&&!memcmp(base+0x14170ab,site,sizeof(site))&&
  !memcmp(base+0x1416737,owner,sizeof(owner))&&!memcmp(base+0x141751d,consume,sizeof(consume))&&
  call(0x14170a6,0x5400)&&call(0x14170b9,0x5300)&&call(0x1417518,0x89f40)&&
  *reinterpret_cast<const UINT*>(base+0x1f8ca64)==reinterpret_cast<UINT>(base)+0x1416bc0;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool Install(){
 if(installed)return true;if(attempted||!g_engine)return false;attempted=true;
 BYTE* base=reinterpret_cast<BYTE*>(g_engine);
 if(!Signatures(base)){fc3::Log("[VEHICLE279] native camera signatures REFUSED; no hook installed");return false;}
 const MH_STATUS created=MH_CreateHook(base+0x14170ab,reinterpret_cast<void*>(Boundary),&original);
 if(created!=MH_OK){fc3::Log("[VEHICLE279] hook creation refused (%d)",int(created));return false;}
 const MH_STATUS enabled=MH_EnableHook(base+0x14170ab);
 if(enabled!=MH_OK){MH_RemoveHook(base+0x14170ab);original=nullptr;fc3::Log("[VEHICLE279] hook enable refused (%d)",int(enabled));return false;}
 installed=true;fc3::Log("[VEHICLE279] verified native camera 14170AB hooked before camera publication; vehicle HMD uses no synthetic stick");return true;
}
bool Set(bool active,const float* view,const float* right=nullptr,const float* up=nullptr,const float* forward=nullptr,const float* visualBody=nullptr,bool nativeFoot281=false){
 Command next;const auto now=GetTickCount64();
 if(active&&view&&g_engine&&Install()){
  AcquireSRWLockShared(&g_nativeLock);const Sample sample=g_sample;ReleaseSRWLockShared(&g_nativeLock);
  AcquireSRWLockShared(&lock);const bool resume284=nativeFoot281&&command.handoff284&&Owns281(command,sample,now);ReleaseSRWLockShared(&lock);
  const int lane280=Lane281(sample,nativeFoot281);
  if(sample.valid&&lane280&&(sample.action128<=0||lane280==4||resume284)&&now>=sample.ms&&now-sample.ms<=250){
   next.lane280=lane280;
   if(resume284&&lane280==3){next.handoff284=true;next.actionEpoch282=sample.actionEpoch282;}
   if(lane280==4){next.action282=sample.cameraAction282;next.actionEpoch282=sample.actionEpoch282;}
   next.owner[0]=sample.handle[0];next.owner[1]=sample.handle[1];next.ms=now;
   for(int i=0;i<3;++i)for(int j=0;j<3;++j)next.head[i][j]=view[i*4+j];next.active=Basis(next.head);
   if(right&&up&&forward){const float* axes[3]={right,up,forward};float native[3][3];const int row[3]={0,2,1};for(int i=0;i<3;++i)for(int j=0;j<3;++j){next.observed[i][j]=axes[i][j];native[row[i]][j]=axes[i][j];}next.observedValid=Basis(native);}
  }
 }
 AcquireSRWLockExclusive(&lock);
 const bool same280=next.active&&command.active&&command.lane280==next.lane280&&command.actionEpoch282==next.actionEpoch282&&command.owner[0]==next.owner[0]&&command.owner[1]==next.owner[1];
 if(next.active&&next.lane280==2){
  if(same280)memcpy(next.offset280,command.offset280,sizeof(next.offset280));
  else if(visualBody&&next.observedValid){
   for(int i=0;i<3;++i)for(int k=0;k<3;++k){double v=0;for(int j=0;j<3;++j)v+=visualBody[i*3+j]*next.observed[k][j];next.offset280[i][k]=float(v);}
   if(!Basis(next.offset280))next.active=false;
  }
 }
 if(next.active&&next.lane280==3){
  // Preserve the entry reference while waiting for the unmodified camera to
  // match a draw. Refresh its offset against current observed axes until the
  // first native write, rather than caching a glider's last HMD-turned view.
  if(same280&&command.seedValid281){memcpy(next.seed281,command.seed281,sizeof(next.seed281));next.seedValid281=true;}
  else if(visualBody){memcpy(next.seed281,visualBody,sizeof(next.seed281));next.seedValid281=true;}
  else if(next.observedValid){memcpy(next.seed281,next.observed,sizeof(next.seed281));next.seedValid281=true;}
  if(same280&&acquired)memcpy(next.offset280,command.offset280,sizeof(next.offset280));
  else if(next.seedValid281&&next.observedValid){
   for(int i=0;i<3;++i)for(int k=0;k<3;++k){double v=0;for(int j=0;j<3;++j)v+=next.seed281[i][j]*next.observed[k][j];next.offset280[i][k]=float(v);}
   if(!Basis(next.offset280))next.active=false;
  }
 }
 if(!same280||!next.active){memset(receipts,0,sizeof(receipts));acquired=false;acquiredCamera284=0;InterlockedExchange(&matchedSerial284,0);}
 command=next;ReleaseSRWLockExclusive(&lock);return next.active;
}
bool Find(const float* right,const float* up,const float* forward,float* body,UINT& generation,float* sourceForward283=nullptr){
 const auto now=GetTickCount64();generation=0;const float* axes[3]={right,up,forward};
 AcquireSRWLockShared(&g_nativeLock);const Sample sample=g_sample;ReleaseSRWLockShared(&g_nativeLock);
 AcquireSRWLockShared(&lock);bool found=false;float best=.0025f; // all axes within about 2.9 degrees, squared distance
 if(Owns281(command,sample,now)&&(!sourceForward283||command.lane280==3))for(const auto& r:receipts){
  if(!r.serial||r.lane280!=command.lane280||r.actionEpoch282!=command.actionEpoch282||r.action282!=command.action282||now<r.ms||now-r.ms>250||r.owner[0]!=command.owner[0]||r.owner[1]!=command.owner[1])continue;
  float error=0;for(int i=0;i<3;++i){float row=0;for(int j=0;j<3;++j){const float d=axes[i][j]-r.target[i][j];row+=d*d;}error=(std::max)(error,row);}
  if(error<best||(error==best&&r.serial>generation)){best=error;found=true;generation=r.serial;memcpy(body,r.body,sizeof(r.body));if(sourceForward283)memcpy(sourceForward283,r.sourceForward283,sizeof(r.sourceForward283));}
 }
 if(found)InterlockedExchange(&matchedSerial284,LONG(generation));
 ReleaseSRWLockShared(&lock);return found;
}
} // namespace vehicle279
