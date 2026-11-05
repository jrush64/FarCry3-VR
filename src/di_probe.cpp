// Logs how the game looks for game controllers through DirectInput.
//
// Touch input is only accepted while a real Xbox pad is plugged in, even
// though the game reads it through XInputGetState. So the game decides that a
// pad exists some other way; this records what it asks DirectInput for and
// what it is told, so a controller can be presented there.
#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <MinHook.h>

#include "di_probe.h"
#include "vr_pad.h"
#include <cstring>
#include "fc3_log.h"

using fc3::Log;

namespace
{
using EnumDevicesFn = HRESULT(STDMETHODCALLTYPE*)(IDirectInput8A*, DWORD, LPDIENUMDEVICESCALLBACKA, LPVOID, DWORD);
using CreateDeviceFn = HRESULT(STDMETHODCALLTYPE*)(IDirectInput8A*, REFGUID, LPDIRECTINPUTDEVICE8A*, LPUNKNOWN);
EnumDevicesFn g_oEnumDevices = nullptr;
CreateDeviceFn g_oCreateDevice = nullptr;
LONG g_enumCalls = 0, g_createCalls = 0;
using DeviceStateFn=HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8A*,DWORD,LPVOID);
using DeviceDataFn=HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8A*,DWORD,LPDIDEVICEOBJECTDATA,LPDWORD,DWORD);
DeviceStateFn g_state=nullptr;DeviceDataFn g_data=nullptr;
HRESULT STDMETHODCALLTYPE MenuDeviceState(IDirectInputDevice8A* d,DWORD bytes,LPVOID out)
{const HRESULT hr=g_state(d,bytes,out);if(fc3pad::NativeInputBlocked()&&SUCCEEDED(hr)&&out)std::memset(out,0,bytes);return hr;}
HRESULT STDMETHODCALLTYPE MenuDeviceData(IDirectInputDevice8A* d,DWORD size,LPDIDEVICEOBJECTDATA out,LPDWORD count,DWORD flags)
{const HRESULT hr=g_data(d,size,out,count,flags);if(fc3pad::NativeInputBlocked()&&count&&SUCCEEDED(hr))*count=0;return hr;}
void InstallDeviceMenuHooks(IDirectInputDevice8A* d)
{if(!d)return;void** table=*reinterpret_cast<void***>(d);
 if(!g_state){const auto hr=MH_CreateHook(table[9],reinterpret_cast<void*>(&MenuDeviceState),reinterpret_cast<void**>(&g_state));if(hr==MH_OK)MH_EnableHook(table[9]);Log("[MENU INPUT] DirectInput state hook %d",hr);}
 if(!g_data){const auto hr=MH_CreateHook(table[10],reinterpret_cast<void*>(&MenuDeviceData),reinterpret_cast<void**>(&g_data));if(hr==MH_OK)MH_EnableHook(table[10]);Log("[MENU INPUT] DirectInput buffered hook %d",hr);}}


struct Forward
{
    LPDIENUMDEVICESCALLBACKA callback;
    LPVOID context;
    int count;
};

BOOL CALLBACK LogDevice(LPCDIDEVICEINSTANCEA device, LPVOID context)
{
    Forward* forward = static_cast<Forward*>(context);
    ++forward->count;
    if (g_enumCalls <= 20)
        Log("[DI]   device: type 0x%08lX \"%s\" product %08lX", device->dwDevType, device->tszProductName,
            device->guidProduct.Data1);
    return forward->callback ? forward->callback(device, forward->context) : DIENUM_STOP;
}

HRESULT STDMETHODCALLTYPE HookEnumDevices(IDirectInput8A* self, DWORD type, LPDIENUMDEVICESCALLBACKA callback,
                                          LPVOID context, DWORD flags)
{
    const LONG call = InterlockedIncrement(&g_enumCalls);
    Forward forward = { callback, context, 0 };
    const HRESULT result = g_oEnumDevices(self, type, LogDevice, &forward, flags);
    if (call <= 20 || call % 500 == 0)
        Log("[DI] EnumDevices #%ld (class %lu, flags 0x%lX): %d device(s), hr 0x%08lX", call, type, flags,
            forward.count, static_cast<unsigned long>(result));
    return result;
}

HRESULT STDMETHODCALLTYPE HookCreateDevice(IDirectInput8A* self, REFGUID guid, LPDIRECTINPUTDEVICE8A* device,
                                           LPUNKNOWN outer)
{
    const HRESULT result = g_oCreateDevice(self, guid, device, outer);
    if(SUCCEEDED(result)&&device)InstallDeviceMenuHooks(*device);
    if (InterlockedIncrement(&g_createCalls) <= 20)
        Log("[DI] CreateDevice %08lX-%04X: hr 0x%08lX", guid.Data1, guid.Data2, static_cast<unsigned long>(result));
    return result;
}
}

namespace fc3di
{
void Install()
{
    HMODULE module = LoadLibraryW(L"dinput8.dll");
    using CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
    const auto create = module ? reinterpret_cast<CreateFn>(GetProcAddress(module, "DirectInput8Create")) : nullptr;
    IDirectInput8A* input = nullptr;
    if (!create || FAILED(create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8A,
                                 reinterpret_cast<void**>(&input), nullptr)) || !input)
    {
        Log("[DI] could not create a DirectInput object to find its functions");
        return;
    }
    void** table = *reinterpret_cast<void***>(input);
    const bool enumOk = MH_CreateHook(table[4], reinterpret_cast<void*>(&HookEnumDevices),
                                      reinterpret_cast<void**>(&g_oEnumDevices)) == MH_OK && MH_EnableHook(table[4]) == MH_OK;
    const bool createOk = MH_CreateHook(table[3], reinterpret_cast<void*>(&HookCreateDevice),
                                        reinterpret_cast<void**>(&g_oCreateDevice)) == MH_OK && MH_EnableHook(table[3]) == MH_OK;
    IDirectInputDevice8A* keyboard=nullptr;input->CreateDevice(GUID_SysKeyboard,&keyboard,nullptr);InstallDeviceMenuHooks(keyboard);if(keyboard)keyboard->Release();
    input->Release();
    Log("[DI] DirectInput probe: EnumDevices %s, CreateDevice %s", enumOk ? "hooked" : "FAILED",
        createOk ? "hooked" : "FAILED");
}
}
