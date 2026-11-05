#define WIN32_LEAN_AND_MEAN
#include "vr_pad.h"
#include <windows.h>

#define D3D11CreateDevice D3D11CreateDevice_SystemDeclaration
#define D3D11CreateDeviceAndSwapChain D3D11CreateDeviceAndSwapChain_SystemDeclaration
#include <d3d11.h>
#undef D3D11CreateDevice
#undef D3D11CreateDeviceAndSwapChain

#include <dxgi.h>

#include "main_state.h"

#define D3D11_ALL_EXPORTS(X)                     \
    X(CreateDirect3D11DeviceFromDXGIDevice)      \
    X(CreateDirect3D11SurfaceFromDXGISurface)    \
    X(D3D11CoreCreateDevice)                     \
    X(D3D11CoreCreateLayeredDevice)              \
    X(D3D11CoreGetLayeredDeviceSize)             \
    X(D3D11CoreRegisterLayers)                   \
    X(D3D11CreateDevice)                         \
    X(D3D11CreateDeviceAndSwapChain)             \
    X(D3D11CreateDeviceForD3D12)                 \
    X(D3D11On12CreateDevice)                     \
    X(D3DKMTCloseAdapter)                        \
    X(D3DKMTCreateAllocation)                    \
    X(D3DKMTCreateContext)                       \
    X(D3DKMTCreateDevice)                        \
    X(D3DKMTCreateSynchronizationObject)         \
    X(D3DKMTDestroyAllocation)                   \
    X(D3DKMTDestroyContext)                      \
    X(D3DKMTDestroyDevice)                       \
    X(D3DKMTDestroySynchronizationObject)        \
    X(D3DKMTEscape)                              \
    X(D3DKMTGetContextSchedulingPriority)        \
    X(D3DKMTGetDeviceState)                      \
    X(D3DKMTGetDisplayModeList)                  \
    X(D3DKMTGetMultisampleMethodList)            \
    X(D3DKMTGetRuntimeData)                      \
    X(D3DKMTGetSharedPrimaryHandle)              \
    X(D3DKMTLock)                                \
    X(D3DKMTOpenAdapterFromHdc)                  \
    X(D3DKMTOpenResource)                        \
    X(D3DKMTPresent)                             \
    X(D3DKMTQueryAdapterInfo)                    \
    X(D3DKMTQueryAllocationResidency)            \
    X(D3DKMTQueryResourceInfo)                   \
    X(D3DKMTRender)                              \
    X(D3DKMTSetAllocationPriority)               \
    X(D3DKMTSetContextSchedulingPriority)        \
    X(D3DKMTSetDisplayMode)                      \
    X(D3DKMTSetDisplayPrivateDriverFormat)       \
    X(D3DKMTSetGammaRamp)                        \
    X(D3DKMTSetVidPnSourceOwner)                 \
    X(D3DKMTSignalSynchronizationObject)         \
    X(D3DKMTUnlock)                              \
    X(D3DKMTWaitForSynchronizationObject)        \
    X(D3DKMTWaitForVerticalBlankEvent)           \
    X(D3DPerformance_BeginEvent)                 \
    X(D3DPerformance_EndEvent)                   \
    X(D3DPerformance_GetStatus)                  \
    X(D3DPerformance_SetMarker)                  \
    X(EnableFeatureLevelUpgrade)                 \
    X(OpenAdapter10)                             \
    X(OpenAdapter10_2)

#define D3D11_FORWARDED_EXPORTS(X)               \
    X(CreateDirect3D11DeviceFromDXGIDevice)      \
    X(CreateDirect3D11SurfaceFromDXGISurface)    \
    X(D3D11CoreCreateDevice)                     \
    X(D3D11CoreCreateLayeredDevice)              \
    X(D3D11CoreGetLayeredDeviceSize)             \
    X(D3D11CoreRegisterLayers)                   \
    X(D3D11CreateDeviceForD3D12)                 \
    X(D3D11On12CreateDevice)                     \
    X(D3DKMTCloseAdapter)                        \
    X(D3DKMTCreateAllocation)                    \
    X(D3DKMTCreateContext)                       \
    X(D3DKMTCreateDevice)                        \
    X(D3DKMTCreateSynchronizationObject)         \
    X(D3DKMTDestroyAllocation)                   \
    X(D3DKMTDestroyContext)                      \
    X(D3DKMTDestroyDevice)                       \
    X(D3DKMTDestroySynchronizationObject)        \
    X(D3DKMTEscape)                              \
    X(D3DKMTGetContextSchedulingPriority)        \
    X(D3DKMTGetDeviceState)                      \
    X(D3DKMTGetDisplayModeList)                  \
    X(D3DKMTGetMultisampleMethodList)            \
    X(D3DKMTGetRuntimeData)                      \
    X(D3DKMTGetSharedPrimaryHandle)              \
    X(D3DKMTLock)                                \
    X(D3DKMTOpenAdapterFromHdc)                  \
    X(D3DKMTOpenResource)                        \
    X(D3DKMTPresent)                             \
    X(D3DKMTQueryAdapterInfo)                    \
    X(D3DKMTQueryAllocationResidency)            \
    X(D3DKMTQueryResourceInfo)                   \
    X(D3DKMTRender)                              \
    X(D3DKMTSetAllocationPriority)               \
    X(D3DKMTSetContextSchedulingPriority)        \
    X(D3DKMTSetDisplayMode)                      \
    X(D3DKMTSetDisplayPrivateDriverFormat)       \
    X(D3DKMTSetGammaRamp)                        \
    X(D3DKMTSetVidPnSourceOwner)                 \
    X(D3DKMTSignalSynchronizationObject)         \
    X(D3DKMTUnlock)                              \
    X(D3DKMTWaitForSynchronizationObject)        \
    X(D3DKMTWaitForVerticalBlankEvent)           \
    X(D3DPerformance_BeginEvent)                 \
    X(D3DPerformance_EndEvent)                   \
    X(D3DPerformance_GetStatus)                  \
    X(D3DPerformance_SetMarker)                  \
    X(EnableFeatureLevelUpgrade)                 \
    X(OpenAdapter10)                             \
    X(OpenAdapter10_2)

#define DECLARE_POINTER(name) extern "C" FARPROC p_##name = nullptr;
D3D11_ALL_EXPORTS(DECLARE_POINTER)
#undef DECLARE_POINTER

#define DECLARE_FORWARD(name)                    \
    extern "C" __declspec(naked) void name()     \
    {                                             \
        __asm jmp dword ptr [p_##name]            \
    }
D3D11_FORWARDED_EXPORTS(DECLARE_FORWARD)
#undef DECLARE_FORWARD

static HMODULE g_systemD3D11 = nullptr;
static HMODULE g_proxyModule = nullptr;

using CreateDeviceFn = HRESULT (WINAPI*)(
    IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
    const D3D_FEATURE_LEVEL*, UINT, UINT,
    ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

extern "C" HRESULT WINAPI D3D11CreateDevice(
    IDXGIAdapter* adapter,
    D3D_DRIVER_TYPE driverType,
    HMODULE software,
    UINT flags,
    const D3D_FEATURE_LEVEL* featureLevels,
    UINT featureLevelCount,
    UINT sdkVersion,
    ID3D11Device** device,
    D3D_FEATURE_LEVEL* selectedFeatureLevel,
    ID3D11DeviceContext** immediateContext)
{
    const auto realCreateDevice = reinterpret_cast<CreateDeviceFn>(p_D3D11CreateDevice);
    if (!realCreateDevice)
        return E_FAIL;

    const HRESULT result = realCreateDevice(
        adapter, driverType, software, flags, featureLevels, featureLevelCount,
        sdkVersion, device, selectedFeatureLevel, immediateContext);
    if (SUCCEEDED(result) && device && *device)
    {
        fc3::OnDeviceCreated(*device, g_proxyModule);
    }
    return result;
}

using CreateDeviceAndSwapChainFn = HRESULT (WINAPI*)(
    IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
    const D3D_FEATURE_LEVEL*, UINT, UINT, const DXGI_SWAP_CHAIN_DESC*,
    IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

extern "C" HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
    IDXGIAdapter* adapter,
    D3D_DRIVER_TYPE driverType,
    HMODULE software,
    UINT flags,
    const D3D_FEATURE_LEVEL* featureLevels,
    UINT featureLevelCount,
    UINT sdkVersion,
    const DXGI_SWAP_CHAIN_DESC* swapChainDescription,
    IDXGISwapChain** swapChain,
    ID3D11Device** device,
    D3D_FEATURE_LEVEL* selectedFeatureLevel,
    ID3D11DeviceContext** immediateContext)
{
    const auto real = reinterpret_cast<CreateDeviceAndSwapChainFn>(p_D3D11CreateDeviceAndSwapChain);
    if (!real)
        return E_FAIL;

    const HRESULT result = real(
        adapter, driverType, software, flags, featureLevels, featureLevelCount,
        sdkVersion, swapChainDescription, swapChain, device, selectedFeatureLevel,
        immediateContext);
    if (SUCCEEDED(result) && device && *device)
    {
        fc3::OnDeviceCreated(*device, g_proxyModule);
        if (swapChain && *swapChain)
            fc3::OnSwapChainCreated(*swapChain, *device, swapChainDescription);
    }
    return result;
}

static bool LoadSystemD3D11()
{
    wchar_t path[MAX_PATH] = {};
    const UINT length = GetSystemDirectoryW(path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH - 11)
        return false;

    lstrcatW(path, L"\\d3d11.dll");
    g_systemD3D11 = LoadLibraryW(path);
    if (!g_systemD3D11)
        return false;

    bool complete = true;
#define RESOLVE_EXPORT(name)                                       \
    p_##name = GetProcAddress(g_systemD3D11, #name);               \
    complete = complete && (p_##name != nullptr);
    D3D11_ALL_EXPORTS(RESOLVE_EXPORT)
#undef RESOLVE_EXPORT
    return complete;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_proxyModule = instance;
        DisableThreadLibraryCalls(instance);
        if (!LoadSystemD3D11())
            return FALSE;
        fc3pad::StartVirtualPad();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        fc3::Shutdown();
        if (g_systemD3D11)
        {
            FreeLibrary(g_systemD3D11);
            g_systemD3D11 = nullptr;
        }
    }
    return TRUE;
}
