#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <cstdio>

static const char* const kExports[] = {
    "CreateDirect3D11DeviceFromDXGIDevice",
    "CreateDirect3D11SurfaceFromDXGISurface",
    "D3D11CoreCreateDevice",
    "D3D11CoreCreateLayeredDevice",
    "D3D11CoreGetLayeredDeviceSize",
    "D3D11CoreRegisterLayers",
    "D3D11CreateDevice",
    "D3D11CreateDeviceAndSwapChain",
    "D3D11CreateDeviceForD3D12",
    "D3D11On12CreateDevice",
    "D3DKMTCloseAdapter",
    "D3DKMTCreateAllocation",
    "D3DKMTCreateContext",
    "D3DKMTCreateDevice",
    "D3DKMTCreateSynchronizationObject",
    "D3DKMTDestroyAllocation",
    "D3DKMTDestroyContext",
    "D3DKMTDestroyDevice",
    "D3DKMTDestroySynchronizationObject",
    "D3DKMTEscape",
    "D3DKMTGetContextSchedulingPriority",
    "D3DKMTGetDeviceState",
    "D3DKMTGetDisplayModeList",
    "D3DKMTGetMultisampleMethodList",
    "D3DKMTGetRuntimeData",
    "D3DKMTGetSharedPrimaryHandle",
    "D3DKMTLock",
    "D3DKMTOpenAdapterFromHdc",
    "D3DKMTOpenResource",
    "D3DKMTPresent",
    "D3DKMTQueryAdapterInfo",
    "D3DKMTQueryAllocationResidency",
    "D3DKMTQueryResourceInfo",
    "D3DKMTRender",
    "D3DKMTSetAllocationPriority",
    "D3DKMTSetContextSchedulingPriority",
    "D3DKMTSetDisplayMode",
    "D3DKMTSetDisplayPrivateDriverFormat",
    "D3DKMTSetGammaRamp",
    "D3DKMTSetVidPnSourceOwner",
    "D3DKMTSignalSynchronizationObject",
    "D3DKMTUnlock",
    "D3DKMTWaitForSynchronizationObject",
    "D3DKMTWaitForVerticalBlankEvent",
    "D3DPerformance_BeginEvent",
    "D3DPerformance_EndEvent",
    "D3DPerformance_GetStatus",
    "D3DPerformance_SetMarker",
    "EnableFeatureLevelUpgrade",
    "OpenAdapter10",
    "OpenAdapter10_2",
};

using CreateDeviceFn = HRESULT (WINAPI*)(
    IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
    const D3D_FEATURE_LEVEL*, UINT, UINT,
    ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2)
    {
        std::fprintf(stderr, "usage: smoke.exe <absolute path to proxy d3d11.dll>\n");
        return 2;
    }

    HMODULE proxy = LoadLibraryW(argv[1]);
    if (!proxy)
    {
        std::fprintf(stderr, "FAIL: LoadLibraryW returned error %lu\n", GetLastError());
        return 3;
    }

    for (const char* name : kExports)
    {
        if (!GetProcAddress(proxy, name))
        {
            std::fprintf(stderr, "FAIL: missing export %s\n", name);
            FreeLibrary(proxy);
            return 4;
        }
    }

    auto createDevice = reinterpret_cast<CreateDeviceFn>(
        GetProcAddress(proxy, "D3D11CreateDevice"));

    const D3D_FEATURE_LEVEL requested[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL obtained = D3D_FEATURE_LEVEL_10_0;
    const HRESULT hr = createDevice(
        nullptr,
        D3D_DRIVER_TYPE_WARP,
        nullptr,
        0,
        requested,
        static_cast<UINT>(sizeof(requested) / sizeof(requested[0])),
        D3D11_SDK_VERSION,
        &device,
        &obtained,
        &context);

    if (FAILED(hr) || !device || !context)
    {
        std::fprintf(stderr, "FAIL: D3D11CreateDevice returned 0x%08lX\n",
                     static_cast<unsigned long>(hr));
        if (context) context->Release();
        if (device) device->Release();
        FreeLibrary(proxy);
        return 5;
    }

    context->Release();
    device->Release();
    FreeLibrary(proxy);

    std::printf("PASS: 51 exports resolved; D3D11 WARP device created through proxy.\n");
    return 0;
}
