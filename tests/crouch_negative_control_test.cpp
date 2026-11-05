#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "../src/crouch_negative_control.cpp"
#include "../src/native_ui_pointer.h"
namespace fc3 { void Log(const char*,...) {} }
namespace fc3aim {int aimCalls=0,suspends=0;void InputTick(const void*) {++aimCalls;}void Suspend(){++suspends;} }
extern "C" MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*) {return MH_OK;}
extern "C" MH_STATUS WINAPI MH_EnableHook(LPVOID) {return MH_OK;}
void Check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
DWORD WINAPI RealPad(DWORD,PadReading* reading){reading->packet=17;reading->pad={};reading->pad.buttons=kA|kX;reading->pad.rightTrigger=255;reading->pad.thumbLX=20000;return ERROR_SUCCESS;}

int main(){
 fc3pad::SetGameplayControls(true);fc3pad::SetDedicatedInteractions(true);
 fc3pad::Controllers c={};c.active=true;
 auto neutral=[&]{c.rightY=0;fc3pad::Publish(c);};
 auto gesture=[&](float y){c.rightY=y;fc3pad::Publish(c);return (FromControllers(g_state).buttons&kB)!=0;};
 ObserveNativeStance(0);neutral();Check(gesture(-1),"standing: down issues native crouch toggle");
 ObserveNativeStance(1);neutral();Check(!gesture(-1),"actual crouch: repeated down cannot stand");
 neutral();Check(gesture(1),"actual crouch: up issues native stand toggle");
 ObserveNativeStance(0);neutral();Check(!gesture(1),"actual stand: repeated up cannot crouch");
 // Landing changed stance without this mapper issuing any input.
 g_stanceCrouched=false;ObserveNativeStance(1);neutral();Check(!gesture(-1),"landing auto crouch: down cannot invert to standing");
 neutral();Check(gesture(1),"landing auto crouch: up stands regardless of old requested state");
 ObserveNativeStance(0);neutral();Check(!gesture(1),"native auto stand: up cannot invert back to crouching");
 neutral();Check(gesture(-1),"native auto stand: down crouches");
 for(int i=0;i<100;++i)Check(gesture(-1),"held gesture keeps single native button press without repeated rising edges");
 ObserveNativeStance(-1);neutral();Check(!gesture(-1),"unknown or unsupported stance cannot emit guessed toggle");
 ObserveNativeStance(1);g_nativeStanceAt=GetTickCount()-251;neutral();Check(!gesture(1),"stale native sample cannot emit guessed toggle");
 puts("PASS: native actual stance controls down/up after out of band landing and standing; unknown and stale snapshots fail closed");
}
