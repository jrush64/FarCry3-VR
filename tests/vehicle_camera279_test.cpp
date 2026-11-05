#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include "../src/aim_probe.cpp"
void Check279(bool ok,const char* label){if(!ok){fprintf(stderr,"FAIL:279 %s\n",label);exit(1);}}
UINT regs279[5];float sse279[4];const float inputSse279[4]={.125f,-.25f,3.5f,19.f};
__declspec(naked) void Resume279(){__asm ret}
// Runs the REAL shipping mid-function bridge with a synthetic native stack.
__declspec(naked) void RunBridge279(UINT camera,float* matrix){
 __asm {
  push ebp
  mov ebp,esp
  push ebx
  push esi
  push edi
  sub esp,140h
  and esp,0fffffff0h
  sub esp,12
  mov esi,[ebp+12]
  lea edi,[esp+0bch]
  mov ecx,16
  rep movsd
  mov ebx,[ebp+8]
  mov eax,12345678h
  mov ecx,23456789h
  mov edx,3456789ah
  movups xmm0,inputSse279
  stc
  call vehicle279::Boundary
  mov regs279,eax
  mov regs279[4],ecx
  mov regs279[8],edx
  mov regs279[12],ebx
  pushfd
  pop eax
  mov regs279[16],eax
  movups sse279,xmm0
  lea esi,[esp+0bch]
  mov edi,[ebp+12]
  mov ecx,16
  rep movsd
  lea esp,[ebp-12]
  pop edi
  pop esi
  pop ebx
  pop ebp
  ret
 }
}
void Rotation279(double yaw,double pitch,double roll,float (&m)[3][3]){
 const double cy=cos(yaw),sy=sin(yaw),cp=cos(pitch),sp=sin(pitch),cr=cos(roll),sr=sin(roll);
 // Proper 3D rotation: rows are the transposed columns of Rz*Ry*Rx.
 m[0][0]=float(cy*cp);m[0][1]=float(sy*cp);m[0][2]=float(-sp);
 m[1][0]=float(cy*sp*sr-sy*cr);m[1][1]=float(sy*sp*sr+cy*cr);m[1][2]=float(cp*sr);
 m[2][0]=float(cy*sp*cr+sy*sr);m[2][1]=float(sy*sp*cr-cy*sr);m[2][2]=float(cp*cr);
}
int main(){
 // Exact installed engine is read into data-only memory; no DLL entrypoint runs.
 const char* gameDll=std::getenv("FC3VR_GAME_DLL"); // path to the game's bin\\FC3_d3d11.dll
 std::ifstream file(gameDll?gameDll:"FC3_d3d11.dll",std::ios::binary);
 std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});Check279(bytes.size()>0x1000,"engine image readable");
 const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(bytes.data()+dos->e_lfanew);
 BYTE* image=static_cast<BYTE*>(VirtualAlloc(nullptr,nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check279(image!=nullptr,"data image allocation");
 memcpy(image,bytes.data(),nt->OptionalHeader.SizeOfHeaders);const auto* section=IMAGE_FIRST_SECTION(nt);
 for(UINT i=0;i<nt->FileHeader.NumberOfSections;++i)memcpy(image+section[i].VirtualAddress,bytes.data()+section[i].PointerToRawData,section[i].SizeOfRawData);
 // Only relocate the single absolute vtable pointer inspected by this guard.
 *reinterpret_cast<UINT*>(image+0x1f8ca64)=reinterpret_cast<UINT>(image)+0x1416bc0;
 Check279(vehicle279::Signatures(image),"all native boundary/owner/consumer signatures match installed engine");
 const UINT actionSites282[]={0x3b63a1,0x3b69e1,0x3b6861,0x3b68a1,0x254cf1,0x254f01};
 const UINT actionTypes282[]={0x2884e20,0x28855c4,0x2885758,0x2884c24,0x287df64,0x287de68};
 for(int i=0;i<6;++i){Check279(*reinterpret_cast<UINT*>(image+actionSites282[i])==nt->OptionalHeader.ImageBase+actionTypes282[i],"282 exact installed action registration matches its descriptor");*reinterpret_cast<UINT*>(image+actionSites282[i])=reinterpret_cast<UINT>(image)+actionTypes282[i];}
 Check279(ActionSignatures128(image),"282 installed action query/current-owner/registration signatures verified");
 const UINT extraSites283[]={0x3b6661,0x254bf1,0x3b6321,0x3b6361,0x3b68e1};
 for(UINT site:extraSites283)for(UINT operand:{site-7,site}){
  auto* value=reinterpret_cast<UINT*>(image+operand);*value=*value-nt->OptionalHeader.ImageBase+reinterpret_cast<UINT>(image);
 }
 Check279(ExtraActionSignatures283(image),"283 exact pickup/cinematic/carry/door class names and registrations verified in installed engine");
 image[extraSites283[0]]^=1;Check279(!ExtraActionSignatures283(image),"283 changed pickup registration refuses extra action identities");image[extraSites283[0]]^=1;
 image[actionSites282[0]]^=1;Check279(!ActionSignatures128(image),"282 changed native action registration refuses binding");image[actionSites282[0]]^=1;
 *reinterpret_cast<UINT*>(image+0x254eba)=reinterpret_cast<UINT>(image)+0x1f89be0;
 *reinterpret_cast<UINT*>(image+0x254ec1)=*reinterpret_cast<UINT*>(image+0x254ecc)=reinterpret_cast<UINT>(image)+0x287de84;
 Check279(SlideSignatures280(image),"280 exact Slide RTTI name and descriptor registration verified");
 image[0x1f89be0]^=1;Check279(!SlideSignatures280(image),"280 altered Slide identity cannot enable the camera lane");image[0x1f89be0]^=1;
 image[0x14170ab]^=1;Check279(!vehicle279::Signatures(image),"changed boundary refuses hook");image[0x14170ab]^=1;
 alignas(16) UINT camera[16]={};g_engine=reinterpret_cast<UINT>(image);camera[0]=g_engine+0x1f8c9f0;camera[6]=123;camera[7]=456;
 g_sample={};g_sample.valid=true;g_sample.vehicle=1;g_sample.action128=0;g_sample.handle[0]=123;g_sample.handle[1]=456;g_sample.ms=GetTickCount64();
 vehicle279::original=reinterpret_cast<void*>(Resume279);
 float base[3][3];Rotation279(.7,-.25,.18,base);
 const float canary[16]={1,0,0,71,0,1,0,72,0,0,1,73,74,75,76,77};
 for(double nativeYaw:{.7,1.4})for(double yaw:{-3.0,-1.6,0.,1.6,3.0})for(double pitch:{-1.3,0.,1.3}){
  Rotation279(nativeYaw,-.25,.18,base); // native stick/chassis turning remains independent of HMD
  float matrix[16];memcpy(matrix,canary,sizeof(matrix));for(int i=0;i<3;++i)for(int j=0;j<3;++j)matrix[i*4+j]=base[i][j];
  vehicle279::command={};auto& c=vehicle279::command;c.active=true;c.owner[0]=123;c.owner[1]=456;c.ms=GetTickCount64();g_sample.ms=c.ms;Rotation279(yaw,pitch,.32,c.head);
  const int referenceRows[3]={0,2,1};for(int i=0;i<3;++i)for(int j=0;j<3;++j)c.observed[i][j]=base[referenceRows[i]][j];c.observedValid=true;
  RunBridge279(reinterpret_cast<UINT>(camera),matrix);
  Check279(regs279[0]==0x12345678&&regs279[1]==0x23456789&&regs279[2]==0x3456789a&&regs279[3]==reinterpret_cast<UINT>(camera)&&(regs279[4]&1),"bridge preserves registers and flags");
  Check279(!memcmp(sse279,inputSse279,sizeof(sse279)),"bridge preserves SSE state");
  for(int i:{3,7,11,12,13,14,15})Check279(matrix[i]==canary[i],"translation and padding unchanged");
  const int nativeRow[3]={0,2,1};float body[9];UINT serial=0;
  Check279(vehicle279::Find(matrix,matrix+8,matrix+4,body,serial),"actual native target joins matching draw receipt");
  for(int i=0;i<3;++i)for(int j=0;j<3;++j){
   double expected=0;for(int k=0;k<3;++k)expected+=c.head[i][k]*base[nativeRow[k]][j];
   Check279(fabs(matrix[nativeRow[i]*4+j]-expected)<2e-6,"head yaw/pitch/roll composes beyond native seat limits");
   Check279(fabs(body[i*3+j]-base[nativeRow[i]][j])<2e-6,"receipt preserves untouched native body basis");
  }
  // The actual draw transform times Q cancels native H exactly once.
  for(int i=0;i<3;++i)for(int j=0;j<3;++j){double q=0;for(int k=0;k<3;++k)q+=matrix[nativeRow[i]*4+k]*body[j*3+k];Check279(fabs(q-c.head[i][j])<2e-6,"native composition and stereo compensation use the same head rotation");}
 }
 float unmatchedBody[9];UINT unmatchedSerial=0;
 const auto finalVehicleReceipt281=vehicle279::receipts[vehicle279::serial%32];
 g_sample.vehicle=0;
 Check279(!vehicle279::Find(finalVehicleReceipt281.target[0],finalVehicleReceipt281.target[1],finalVehicleReceipt281.target[2],unmatchedBody,unmatchedSerial),"281 fresh vehicle receipt must stop owning draws immediately on native vehicle exit");
 g_sample.vehicle=1;
 Check279(!vehicle279::Find(base[0],base[2],base[1],unmatchedBody,unmatchedSerial),"unrelated draw camera cannot borrow a native receipt");
 // A fresh owner must first agree with an untouched native render camera.
 vehicle279::acquired=false;vehicle279::command.observedValid=false;
 auto refuseUnbound=[&](){float m[16];memcpy(m,canary,sizeof(m));RunBridge279(reinterpret_cast<UINT>(camera),m);Check279(!memcmp(m,canary,sizeof(m)),"unobserved local camera cannot acquire write ownership");};refuseUnbound();
 vehicle279::command.observedValid=true;memset(vehicle279::command.observed,0,sizeof(vehicle279::command.observed));refuseUnbound();vehicle279::acquired=true;
 auto refusal=[&](const char* label){float m[16];memcpy(m,canary,sizeof(m));RunBridge279(reinterpret_cast<UINT>(camera),m);Check279(!memcmp(m,canary,sizeof(m)),label);};
 camera[7]=457;refusal("other owner camera untouched");camera[7]=456;
 g_sample.vehicle=0;refusal("on-foot camera untouched");g_sample.vehicle=1;
 g_sample.action128=1;refusal("native action camera untouched");g_sample.action128=0;
 vehicle279::command.ms=GetTickCount64()-200;refusal("stale HMD command untouched");vehicle279::command.ms=GetTickCount64();
 vehicle279::command.active=false;refusal("disabled/no headset camera untouched");vehicle279::command.active=true;
 vehicle279::command.head[0][0]=NAN;refusal("invalid HMD basis untouched");
 // The new lane must not inherit a still-fresh vehicle command for this player.
 Rotation279(.8,-.5,.2,vehicle279::command.head);vehicle279::command.ms=GetTickCount64();g_sample.ms=GetTickCount64();
 g_sample.vehicle=0;g_sample.slide280=1;refusal("280 confirmed slide refuses a vehicle-lane command");
 vehicle279::command.lane280=2;vehicle279::acquired=true;
 Rotation279(.35,0,0,vehicle279::command.offset280);
 for(double nativeYaw:{.7,1.4}){
  float matrix[16];memcpy(matrix,canary,sizeof(matrix));Rotation279(nativeYaw,-.4,.25,base);for(int i=0;i<3;++i)for(int j=0;j<3;++j)matrix[i*4+j]=base[i][j];
  RunBridge279(reinterpret_cast<UINT>(camera),matrix);float slideBody[9];UINT slideSerial=0;
  Check279(vehicle279::Find(matrix,matrix+8,matrix+4,slideBody,slideSerial),"280 slide draw joins its own native output receipt");
  const int rows[3]={0,2,1};
  for(int i=0;i<3;++i)for(int j=0;j<3;++j){double expected=0;for(int k=0;k<3;++k)expected+=vehicle279::command.offset280[i][k]*base[rows[k]][j];Check279(fabs(slideBody[i*3+j]-expected)<2e-6,"280 pre-slide visual reference survives animated native camera rotation");}
  for(int i=0;i<3;++i)for(int j=0;j<3;++j){double expected=0;for(int k=0;k<3;++k)expected+=vehicle279::command.head[i][k]*slideBody[k*3+j];Check279(fabs(matrix[rows[i]*4+j]-expected)<2e-6,"280 native slide plus HMD stays in agreement with the final stereo basis");}
  for(int i:{3,7,11,12,13,14,15})Check279(matrix[i]==canary[i],"280 slide position/padding unchanged");
 }
 g_sample.slide280=0;refusal("280 slide-end cancels native writes immediately, without waiting for the render tick");
 g_sample.slide280=-1;refusal("280 unknown slide state cannot continue writes");
 g_sample.slide280=1;g_sample.vehicle=1;refusal("280 vehicle cannot inherit old slide command");g_sample.vehicle=0;
 vehicle279::Set(false,nullptr);float body[9];UINT generation=0;Check279(!vehicle279::Find(base[0],base[2],base[1],body,generation),"cancel invalidates all native receipts");
 // Common weapon-free ownership, not a special swimming class or gesture.
 g_sample.vehicle=0;g_sample.slide280=0;g_sample.action128=0;g_sample.ms=GetTickCount64();
 Check279(fc3aim::NativeFootCamera281(),"281 fresh native on-foot view does not require passive aim binding");
 vehicle279::installed=true;
 float headView281[16]={};float head281[3][3];Rotation279(.6,-.3,.2,head281);
 for(int i=0;i<3;++i)for(int j=0;j<3;++j)headView281[i*4+j]=head281[i][j];headView281[15]=1;
 float oldNative281[3][3],newNative281[3][3],seedNative281[3][3];Rotation279(0,0,0,oldNative281);Rotation279(.4,.1,-.15,newNative281);Rotation279(.25,-.05,.08,seedNative281);
 const int nativeRows281[3]={0,2,1};float seedBody281[9];for(int i=0;i<3;++i)for(int j=0;j<3;++j)seedBody281[i*3+j]=seedNative281[nativeRows281[i]][j];
 Check279(vehicle279::Set(true,headView281,oldNative281[0],oldNative281[2],oldNative281[1],seedBody281,true),"281 common native view accepts fresh on-foot owner");
 float retryMatrix281[16];memcpy(retryMatrix281,canary,sizeof(retryMatrix281));for(int i=0;i<3;++i)for(int j=0;j<3;++j)retryMatrix281[i*4+j]=newNative281[i][j];
 float beforeRetry281[16];memcpy(beforeRetry281,retryMatrix281,sizeof(beforeRetry281));const UINT beforeSerial281=vehicle279::serial;
 RunBridge279(reinterpret_cast<UINT>(camera),retryMatrix281);
 Check279(vehicle279::serial==beforeSerial281&&!memcmp(beforeRetry281,retryMatrix281,sizeof(beforeRetry281)),"281 delayed camera acquisition refuses the previous view without writing");
 // Simulate a render tick that now observes the actual new native view. The
 // original seed survives, even if the caller currently lacks compensation.
 Check279(vehicle279::Set(true,headView281,newNative281[0],newNative281[2],newNative281[1],nullptr,true),"281 acquisition can retry against a new observed camera");
 RunBridge279(reinterpret_cast<UINT>(camera),retryMatrix281);
 Check279(vehicle279::Find(retryMatrix281,retryMatrix281+8,retryMatrix281+4,body,generation),"281 acquired common view joins its output");
 for(int i=0;i<9;++i)Check279(fabs(body[i]-seedBody281[i])<2e-6,"281 delayed acquisition preserves the original visual seed without caching an old HMD turn");
 // Repeat observed-target feedback while native camera position/orientation
 // changes. A completed native output cannot be folded back into the offset.
 for(int frame=0;frame<300;++frame){
  g_sample.ms=GetTickCount64();
  float previousTarget281[3][3];for(int i=0;i<3;++i)for(int j=0;j<3;++j)previousTarget281[i][j]=retryMatrix281[nativeRows281[i]*4+j];
  Rotation279(.6+sin(frame*.03),-.3+.25*cos(frame*.02),.2,head281);for(int i=0;i<3;++i)for(int j=0;j<3;++j)headView281[i*4+j]=head281[i][j];
  Check279(vehicle279::Set(true,headView281,previousTarget281[0],previousTarget281[1],previousTarget281[2],nullptr,true),"281 native view command refresh");
  memcpy(retryMatrix281,canary,sizeof(retryMatrix281));for(int i=0;i<3;++i)for(int j=0;j<3;++j)retryMatrix281[i*4+j]=newNative281[i][j];retryMatrix281[14]=77+float(frame)*.01f;
  RunBridge279(reinterpret_cast<UINT>(camera),retryMatrix281);
  Check279(vehicle279::Find(retryMatrix281,retryMatrix281+8,retryMatrix281+4,body,generation),"281 repeated common native output matches its actual draw");
  float source283[3];Check279(vehicle279::Find(retryMatrix281,retryMatrix281+8,retryMatrix281+4,body,generation,source283),"283 matched on-foot draw supplies its original native source");
  for(int j=0;j<3;++j)Check279(fabs(source283[j]-newNative281[1][j])<2e-6,"283 passive bind source is the untouched native forward, not the head-composed output or retained visual offset");
  for(int i=0;i<9;++i)Check279(fabs(body[i]-seedBody281[i])<3e-6,"281 repeated HMD movement cannot accumulate into the native visual base");
  for(int i=0;i<3;++i)for(int j=0;j<3;++j){double expected=0;for(int k=0;k<3;++k)expected+=head281[i][k]*seedBody281[k*3+j];Check279(fabs(retryMatrix281[nativeRows281[i]*4+j]-expected)<3e-6,"281 repeated native output contains exactly one HMD rotation");}
  Check279(retryMatrix281[14]==77+float(frame)*.01f,"281 native view position changes are preserved");
 }
 auto footReceipt281=vehicle279::receipts[vehicle279::serial%32];
 g_sample.vehicle=1;Check279(!vehicle279::Find(footReceipt281.target[0],footReceipt281.target[1],footReceipt281.target[2],body,generation),"281 common native receipt invalidates immediately on vehicle entry");
 g_sample.vehicle=0;g_sample.slide280=1;Check279(!vehicle279::Find(footReceipt281.target[0],footReceipt281.target[1],footReceipt281.target[2],body,generation),"281 common native receipt cannot leak into a slide");g_sample.slide280=0;
 g_sample.handle[1]++;Check279(!vehicle279::Find(footReceipt281.target[0],footReceipt281.target[1],footReceipt281.target[2],body,generation),"281 respawn handle invalidates native draw ownership");g_sample.handle[1]--;
 vehicle279::Set(false,nullptr);Check279(!vehicle279::Find(footReceipt281.target[0],footReceipt281.target[1],footReceipt281.target[2],body,generation),"281 release cancels the last native receipt before armed handoff");
 // Scripted actions use their authored camera orientation/translation, not a
 // held weapon heading. Execute the real shipping bridge for all six owners.
 for(int action:{1,2,4,8,16,32,256,512,1024,2048,4096}){
  vehicle279::Set(false,nullptr);g_sample.valid=true;g_sample.vehicle=0;g_sample.slide280=0;
  g_sample.action128=g_sample.cameraAction282=action;++g_sample.actionEpoch282;g_sample.ms=GetTickCount64();
  Rotation279(.7,-.2,.3,base);
  Check279(fc3aim::NativeActionCamera282()==action,"282 specific native scripted owner selected");
  Check279(vehicle279::Set(true,headView281,base[0],base[2],base[1]),"282 action acquires native camera independently of controller weapon visibility");
  Check279(vehicle279::command.lane280==4,"282 action uses isolated ownership lane");
  for(int frame=0;frame<300;++frame){
   g_sample.ms=GetTickCount64();
   if(frame){Check279(vehicle279::Set(true,headView281,retryMatrix281,retryMatrix281+8,retryMatrix281+4),"282 refresh against actual previous draw");Rotation279(.7+frame*.002,-.2+sin(frame*.03)*.2,.3+frame*.001,base);}
   memcpy(retryMatrix281,canary,sizeof(retryMatrix281));for(int i=0;i<3;++i)for(int j=0;j<3;++j)retryMatrix281[i*4+j]=base[i][j];retryMatrix281[14]=float(frame);
   RunBridge279(reinterpret_cast<UINT>(camera),retryMatrix281);
   Check279(vehicle279::Find(retryMatrix281,retryMatrix281+8,retryMatrix281+4,body,generation),"282 actual scripted draw matches the native receipt");
   float source283[3];Check279(!vehicle279::Find(retryMatrix281,retryMatrix281+8,retryMatrix281+4,body,generation,source283),"283 scripted camera cannot authorize ordinary pawn-look binding");
   for(int i=0;i<3;++i)for(int j=0;j<3;++j){
    Check279(fabs(body[i*3+j]-base[nativeRows281[i]][j])<3e-6,"282 authored script turn/pitch/roll retained without old weapon heading or accumulated HMD");
    double expected=0;for(int k=0;k<3;++k)expected+=head281[i][k]*body[k*3+j];
    Check279(fabs(retryMatrix281[nativeRows281[i]*4+j]-expected)<3e-6,"282 scripted view contains exactly one HMD rotation");
   }
   for(int i:{3,7,11,12,13,15})Check279(retryMatrix281[i]==canary[i],"282 scripted translation and padding remain untouched");
   Check279(retryMatrix281[14]==float(frame),"282 moving scripted camera position retained");
  }
  const auto receipt=vehicle279::receipts[vehicle279::serial%32];
  auto noReceipt=[&](const char* label){Check279(!vehicle279::Find(receipt.target[0],receipt.target[1],receipt.target[2],body,generation),label);};
  g_sample.cameraAction282=0;noReceipt("282 native action end invalidates receipt while arms still retain the150ms grace");refusal("282 native action end stops writes in the same input poll");
  g_sample.cameraAction282=-1;noReceipt("282 unknown current action cannot reuse a receipt");
  g_sample.cameraAction282=action;++g_sample.actionEpoch282;noReceipt("282 immediate same-action reentry cannot reuse the previous episode");refusal("282 immediate reentry cannot reuse the previous HMD command");
  --g_sample.actionEpoch282;g_sample.cameraAction282=action==1?2:1;noReceipt("282 another scripted action cannot inherit the previous receipt");g_sample.cameraAction282=action;
  g_sample.valid=false;noReceipt("282 invalid owner sample refuses scripted compensation");g_sample.valid=true;
  g_sample.ms=GetTickCount64()-251;noReceipt("282 stale action refuses scripted compensation");g_sample.ms=GetTickCount64();
  g_sample.vehicle=1;noReceipt("282 vehicle entry releases script camera ownership");g_sample.vehicle=0;
  ++g_sample.handle[0];noReceipt("282 changed player cannot inherit script camera ownership");--g_sample.handle[0];
  // Real direct action -> armed exit: the action ends at the input seam,
  // while native hands still own their150ms grace and no foot render tick
  // has run. The very next shipping camera update must still contain HMD.
  Sample exit284=g_sample;exit284.cameraAction282=0;exit284.action128=action;++exit284.actionEpoch282;exit284.ms=GetTickCount64();
  const LONG proof284=InterlockedExchange(&vehicle279::matchedSerial284,0);
  Check279(!vehicle279::PromoteExit284(exit284,exit284.ms),"284 no matched native draw cannot promote a scripted camera");
  InterlockedExchange(&vehicle279::matchedSerial284,proof284);
  ++vehicle279::acquiredCamera284;Check279(!vehicle279::PromoteExit284(exit284,exit284.ms),"284 matched output from another camera cannot promote even with the same player/episode");--vehicle279::acquiredCamera284;
  Sample wrong284=exit284;++wrong284.handle[0];Check279(!vehicle279::PromoteExit284(wrong284,wrong284.ms),"284 another player cannot inherit script exit coverage");
  wrong284=exit284;wrong284.cameraAction282=-1;Check279(!vehicle279::PromoteExit284(wrong284,wrong284.ms),"284 unknown action state cannot promote native camera");
  wrong284=exit284;++wrong284.actionEpoch282;Check279(!vehicle279::PromoteExit284(wrong284,wrong284.ms),"284 a missing/reentered episode cannot inherit an old script command");
  Check279(vehicle279::PromoteExit284(exit284,exit284.ms),"284 exact observed action end promotes before render-side cleanup");g_sample=exit284;
  Check279(fc3aim::NativeHandoff284()&&fc3aim::NativeFootCamera281(),"284 exit camera remains eligible while native hands retain arm-release grace");
  Check279(!vehicle279::Find(receipt.target[0],receipt.target[1],receipt.target[2],body,generation),"284 old scripted receipt itself is discarded rather than reused across ownership");
  Rotation279(.2,-.04,.03,base);memcpy(retryMatrix281,canary,sizeof(retryMatrix281));for(int i=0;i<3;++i)for(int j=0;j<3;++j)retryMatrix281[i*4+j]=base[i][j];
  UINT otherCamera284[16];memcpy(otherCamera284,camera,sizeof(otherCamera284));float untouched284[16];memcpy(untouched284,retryMatrix281,sizeof(untouched284));RunBridge279(reinterpret_cast<UINT>(otherCamera284),retryMatrix281);Check279(!memcmp(untouched284,retryMatrix281,sizeof(untouched284)),"284 different camera object with identical owner and vtable stays untouched during exit");
  RunBridge279(reinterpret_cast<UINT>(camera),retryMatrix281);
  Check279(vehicle279::Find(retryMatrix281,retryMatrix281+8,retryMatrix281+4,body,generation),"284 first native frame after script exit has a new matched receipt");
  for(int i=0;i<3;++i)for(int j=0;j<3;++j){double expected=0;for(int k=0;k<3;++k)expected+=head281[i][k]*base[nativeRows281[k]][j];Check279(fabs(retryMatrix281[nativeRows281[i]*4+j]-expected)<3e-6,"284 first exit frame keeps exactly one HMD rotation around the current authored native heading");}
  Check279(vehicle279::Set(true,headView281,retryMatrix281,retryMatrix281+8,retryMatrix281+4,nullptr,true)&&fc3aim::NativeHandoff284(),"284 renderer refresh cannot cancel the exit camera merely because hands still own their grace");
  float exitSource284[3];Check279(vehicle279::Find(retryMatrix281,retryMatrix281+8,retryMatrix281+4,body,generation,exitSource284),"284 exit receipt supplies passive gun-binding source before arms resume");
  for(int frame=0;frame<30;++frame){
   g_sample.ms=GetTickCount64();if(frame==10)g_sample.action128=0;
   Rotation279(.6+frame*.017,-.3+sin(frame*.1)*.15,.2,head281);for(int i=0;i<3;++i)for(int j=0;j<3;++j)headView281[i*4+j]=head281[i][j];
   Check279(vehicle279::Set(true,headView281,retryMatrix281,retryMatrix281+8,retryMatrix281+4,nullptr,true),"284 handoff keeps native coverage through simultaneous stick/head movement and arm release");
   Rotation279(.2+frame*.04,-.04,.03,base);memcpy(retryMatrix281,canary,sizeof(retryMatrix281));for(int i=0;i<3;++i)for(int j=0;j<3;++j)retryMatrix281[i*4+j]=base[i][j];
   RunBridge279(reinterpret_cast<UINT>(camera),retryMatrix281);Check279(vehicle279::Find(retryMatrix281,retryMatrix281+8,retryMatrix281+4,body,generation),"284 every rotating exit frame has its own native camera/draw match");
   for(int i=0;i<3;++i)for(int j=0;j<3;++j){Check279(fabs(body[i*3+j]-base[nativeRows281[i]][j])<3e-6,"284 post-action stick movement remains in the native body frame without retaining an old script angle");double expected=0;for(int k=0;k<3;++k)expected+=head281[i][k]*body[k*3+j];Check279(fabs(retryMatrix281[nativeRows281[i]*4+j]-expected)<3e-6,"284 rotating after an action does not lose or accumulate the HMD transform");}
  }
  g_sample.action128=0;Check279(fc3aim::NativeHandoff284(),"284 confirmed arm release retains camera until ordinary aim is ready");
  ++g_sample.actionEpoch282;g_sample.cameraAction282=action;g_sample.action128=action;refusal("284 new action immediately invalidates the old direct-exit command");
 }
 g_sample.cameraAction282=64;g_sample.action128=64;Check279(!fc3aim::NativeActionCamera282(),"282 generic no-control is never a scripted camera owner");
 g_sample.cameraAction282=3;Check279(!fc3aim::NativeActionCamera282(),"282 combined/ambiguous action flags cannot acquire native camera");
 vehicle279::Set(false,nullptr);
 VirtualFree(image,0,MEM_RELEASE);
 puts("PASS:284 direct script-to-gun input promotion and first native exit frame across11 actions; matched-camera/owner/epoch proof, fresh receipts, native hand grace and passive aim source;279-283 retained");return 0;
}
