
#include "../src/di_probe.cpp"
#include <cstdio>
#include <cstdlib>
namespace fc3 {void Log(const char*,...) {}}
namespace fc3pad {bool blocked=false,nativePaused=false,exitWait=false;bool MenuOpen(){return blocked;}bool NativeInputBlocked(){return blocked||(exitWait&&!nativePaused);}bool GameplayInputBlocked(){return blocked||nativePaused;}}
extern "C" MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*){return MH_OK;}
extern "C" MH_STATUS WINAPI MH_EnableHook(LPVOID){return MH_OK;}
void Check(bool ok,const char* s){if(!ok){fprintf(stderr,"FAIL: %s\n",s);exit(1);}}
HRESULT STDMETHODCALLTYPE State(IDirectInputDevice8A*,DWORD bytes,LPVOID out){memset(out,0xab,bytes);return S_OK;}
HRESULT STDMETHODCALLTYPE Data(IDirectInputDevice8A*,DWORD,LPDIDEVICEOBJECTDATA out,LPDWORD count,DWORD){if(out&&*count){out[0].dwData=0x80;}*count=1;return S_OK;}
int main()
{
 HMODULE dll=LoadLibraryW(L"dinput8.dll");using Create=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
 auto create=reinterpret_cast<Create>(GetProcAddress(dll,"DirectInput8Create"));IDirectInput8A* input=nullptr;Check(create&&SUCCEEDED(create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8A,reinterpret_cast<void**>(&input),nullptr)),"native DirectInput creation");
 IDirectInputDevice8A* keyboard=nullptr,*mouse=nullptr;Check(SUCCEEDED(input->CreateDevice(GUID_SysKeyboard,&keyboard,nullptr))&&SUCCEEDED(input->CreateDevice(GUID_SysMouse,&mouse,nullptr)),"native keyboard/mouse interfaces");
 void** kb=*reinterpret_cast<void***>(keyboard);void** ms=*reinterpret_cast<void***>(mouse);
 Check(kb[9]==ms[9]&&kb[10]==ms[10],"keyboard and mouse share covered DirectInput state/buffered hook targets");
 g_state=State;g_data=Data;fc3pad::blocked=true;unsigned char keys[256]={};DIMOUSESTATE2 pointer={};
 Check(MenuDeviceState(keyboard,sizeof(keys),keys)==S_OK&&keys[25]==0,"keyboard blocked without changing HRESULT");
 Check(MenuDeviceState(mouse,sizeof(pointer),&pointer)==S_OK&&!pointer.lX&&!pointer.rgbButtons[0],"mouse movement/buttons blocked without changing HRESULT");
 DIDEVICEOBJECTDATA buffered[2]={};DWORD count=2;Check(MenuDeviceData(mouse,sizeof(buffered[0]),buffered,&count,DIGDD_PEEK)==S_OK&&count==0,"buffered mouse input withheld");
 fc3pad::blocked=false;fc3pad::nativePaused=true;Check(fc3pad::GameplayInputBlocked(),"native menu still blocks gameplay actions");Check(MenuDeviceState(keyboard,sizeof(keys),keys)==S_OK&&keys[25]==0xab,"keyboard state restored after menu");count=2;MenuDeviceData(mouse,sizeof(buffered[0]),buffered,&count,0);Check(count==1&&buffered[0].dwData==0x80,"buffered input restored after menu");
 fc3pad::nativePaused=false;fc3pad::exitWait=true;Check(MenuDeviceState(keyboard,sizeof(keys),keys)==S_OK&&keys[25]==0,"neutral exit guard still blocks keyboard");
 keyboard->Release();mouse->Release();input->Release();puts("PASS: offline29 native menus retain DirectInput mouse/keyboard while Insert suppresses state and buffered input; physical gameplay remains blocked");puts("PASS: native DirectInput keyboard/mouse hook coverage; state and buffered input blocked in menu and restored.");
}
