#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// This is intentionally a signature-agnostic x86 forwarding proxy. A naked
// jump preserves the caller's stack and registers exactly as the real export
// expects them. No render calls are intercepted or modified.

#define D3D11_EXPORTS(X)                         \
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

#define DECLARE_FORWARD(name)                    \
    extern "C" FARPROC p_##name = nullptr;       \
    extern "C" __declspec(naked) void name()     \
    {                                             \
        __asm jmp dword ptr [p_##name]            \
    }

D3D11_EXPORTS(DECLARE_FORWARD)

#undef DECLARE_FORWARD

static HMODULE g_systemD3D11 = nullptr;

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
#define RESOLVE_FORWARD(name)                                      \
    p_##name = GetProcAddress(g_systemD3D11, #name);               \
    complete = complete && (p_##name != nullptr);

    D3D11_EXPORTS(RESOLVE_FORWARD)

#undef RESOLVE_FORWARD
    return complete;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        if (!LoadSystemD3D11())
            return FALSE;
    }
    else if (reason == DLL_PROCESS_DETACH && g_systemD3D11)
    {
        FreeLibrary(g_systemD3D11);
        g_systemD3D11 = nullptr;
    }

    return TRUE;
}
