#include <cstdio>
#include <cstdlib>
#include "../src/aim_probe.cpp"
#include "../src/selector_policy290.h"
namespace fc3 {void Log(const char*,...){} }
namespace fc3xr {void SetTestNote(const wchar_t*){} }
extern "C" MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*){return MH_ERROR_UNSUPPORTED_FUNCTION;}
extern "C" MH_STATUS WINAPI MH_EnableHook(LPVOID){return MH_ERROR_NOT_CREATED;}
unsigned pawn290=0,slotPointers290[4]={},seenInventory290=0,seenCategory290=0,seenEquip290[4]={},equipCalls290=0;
UINT __cdecl Pawn290(UINT,UINT){return pawn290;}
__declspec(naked) void Seat290(){__asm{xor eax,eax
 ret}}
__declspec(naked) void Slot290(){__asm{mov seenInventory290,eax
 mov edx,[ebp+8]
 mov seenCategory290,edx
 mov eax,[ebp+12]
 mov eax,[slotPointers290+eax*4]
 mov esp,ebp
 pop ebp
 ret 8}}
__declspec(naked) void Equip290(){__asm{inc equipCalls290
 mov seenEquip290,ebx
 mov eax,[ebp+12]
 mov seenEquip290+4,eax
 mov eax,[ebp+16]
 mov seenEquip290+8,eax
 mov eax,[ebp+20]
 mov seenEquip290+12,eax
 pop ebx
 mov esp,ebp
 pop ebp
 ret 16}}
int failures=0;
void Check(bool yes,const char* text){printf("%s: %s\n",yes?"PASS":"FAIL",text);if(!yes)++failures;}
void Jump(BYTE* at,void* target){at[0]=0xe9;*reinterpret_cast<int*>(at+1)=int(reinterpret_cast<BYTE*>(target)-(at+5));}
void Tick(UINT* handle){Sleep(50);fc3select290::KeepAlive(true);SelectorNative290(handle,pawn290);}
int main(){
 using namespace fc3select290;
 for(int i=0;i<4;++i){Gesture g;g.Step(true,true,false,0,-1,100);g.Step(true,true,true,1,-1,110);
  Check(g.open&&!g.ready,"chord opens without grabbing");
  Check(g.Step(true,true,true,1,i,120)<0,"opening grip cannot select");g.Step(true,true,true,0,i,130);
  Check(g.Step(true,true,true,1,i,140)==i&&g.pending,"fresh reached grip selects exactly the touched slot");
  Check(g.Step(true,true,true,1,i,150)<0,"held grip never repeats a request");
  const auto p=Centre(i);Check(Touch(p)==i,"rendered card centre maps to its exact slot");
  g.Step(false,true,true,1,i,160);Check(!g.open,"permission loss cancels");
 }
 Gesture g;g.Step(true,true,false,0,-1,100);g.Step(true,true,true,1,-1,110);g.Step(true,true,false,0,-1,120);Check(!g.open,"A release cancels without selecting");
 g.Step(true,true,true,1,-1,130);g.Step(true,false,true,0,0,140);Check(!g.open,"tracking loss cancels");g.Step(true,true,true,1,0,150);Check(!g.open,"held chord after tracking loss cannot reopen");
 Check(Touch({0,0,0})==-1&&Touch({NAN,0,0})==-1,"out of reach and NaN cannot select");
 BYTE* image=static_cast<BYTE*>(VirtualAlloc(nullptr,48332800,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(!image)return 2;g_engine=UINT(image);g_engineBytes=48332800;
 memcpy(image+0x13169e0,"\x55\x8b\xec\x83\xe4\xf8\x83\xec\x34",9);Jump(image+0x13169e9,Slot290);
 memcpy(image+0x131cb50,"\x55\x8b\xec\x83\xec\x0c\x53\x8b\x5d\x08",10);Jump(image+0x131cb5a,Equip290);
 memcpy(image+0x1989837,"\x8b\x40\x24\x52\x05\xe4\x0b\x00\x00",9);image[0x1989840]=0x68;*reinterpret_cast<UINT*>(image+0x1989841)=g_engine+0x219d1b0;
 memcpy(image+0x2049b2c,"select_weapon_slot0",19);memcpy(image+0x131e4c8,"\x52\x50\x6a\x00\x56\xe8",6);
 Check(SelectorSignatures290(),"exact-image native slot/equip contracts pass");image[0x131cb50]^=1;Check(!SelectorSignatures290(),"changed native equip entry fails closed");image[0x131cb50]^=1;
 UINT pawn[16]={},owner[0xe00/4]={},wrapper[4][40]={},refs[4][4]={},holder[4]={},definitions[4][28]={},properties[4][24]={};char nativeNames[4][32]={};UINT handle[2]={123,456};pawn290=UINT(pawn);pawn[0x24/4]=UINT(owner);const UINT inv=UINT(owner)+0xbe4;
 *reinterpret_cast<UINT*>(inv+0xb4)=UINT(holder);holder[3]=0x100;
 for(int i=0;i<4;++i){slotPointers290[i]=UINT(wrapper[i]);wrapper[i][1]=UINT(refs[i]);refs[i][0]=100+i;refs[i][1]=200+i;refs[i][3]=0x100+i;wrapper[i][0x48/4]=UINT(definitions[i]);definitions[i][0x64/4]=UINT(properties[i]);properties[i][0x4c/4]=UINT(nativeNames[i]);sprintf_s(nativeNames[i]+12,20,"Weapon%d",i);}
 g_pawnFromHandle=Pawn290;g_vehicleSeatOf=Seat290;g_actionTest128=0;
 Tick(handle);auto shown=Read();Check(shown.valid&&shown.weapons[3].entity==0x103&&seenInventory290==inv&&seenCategory290==g_engine+0x219d1b0,"shipping getter uses EAX inventory, native category and all four real slot identities");
 Check(!strcmp(shown.weapons[2].name,"Weapon2"),"shipping unique weapon-name pointer chain and string layout");
 for(int i=0;i<4;++i){holder[3]=0x999;Tick(handle);shown=Read();const unsigned calls=equipCalls290;
  Check(Request(shown,i),"fresh populated slot queues");Tick(handle);
  Check(equipCalls290==calls+1&&seenEquip290[0]==inv&&seenEquip290[1]==0&&seenEquip290[2]==refs[i][0]&&seenEquip290[3]==refs[i][1],"shipping native equip ABI uses exact selected 64-bit identity and native false flag");
  Check(Result()==0,"request dispatch alone is not equip success");Tick(handle);Check(equipCalls290==calls+1,"native request is dispatched once");holder[3]=refs[i][3];Tick(handle);Check(Result()==1,"only exact equipped entity confirms success");
 }
 Tick(handle);shown=Read();Request(shown,0);refs[1][0]++;const unsigned before=equipCalls290;Tick(handle);Check(Result()==-1&&equipCalls290==before,"changed loadout cancels before native mutation");
 shown=Read();Request(shown,0);g_actionTest128=1;Tick(handle);Check(Result()==-1&&equipCalls290==before,"native scripted action refuses selection");g_actionTest128=0;
 Tick(handle);shown=Read();shown.at-=200;Check(!Request(shown,0),"stale snapshot cannot queue");
 KeepAlive(false);VirtualFree(image,0,MEM_RELEASE);return failures?1:0;
}

