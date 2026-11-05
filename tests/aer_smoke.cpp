#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <cstdio>
#include <cstring>
#include <cmath>

using CreateDeviceFn = HRESULT (WINAPI*)(
    IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
    const D3D_FEATURE_LEVEL*, UINT, UINT,
    ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2)
        return 2;

    wchar_t logPath[MAX_PATH] = {};
    lstrcpynW(logPath, argv[1], MAX_PATH);
    wchar_t* slash = wcsrchr(logPath, L'\\');
    if (!slash)
        return 3;
    *(slash + 1) = L'\0';
    lstrcatW(logPath, L"fc3_aer.log");
    DeleteFileW(logPath);

    HMODULE proxy = LoadLibraryW(argv[1]);
    if (!proxy)
    {
        std::fprintf(stderr, "FAIL: proxy load error %lu\n", GetLastError());
        return 4;
    }

    const char* required[] = {
        "CreateDirect3D11DeviceFromDXGIDevice", "CreateDirect3D11SurfaceFromDXGISurface",
        "D3D11CoreCreateDevice", "D3D11CoreCreateLayeredDevice",
        "D3D11CoreGetLayeredDeviceSize", "D3D11CoreRegisterLayers",
        "D3D11CreateDevice", "D3D11CreateDeviceAndSwapChain",
        "D3D11CreateDeviceForD3D12", "D3D11On12CreateDevice",
        "D3DKMTCloseAdapter", "D3DKMTCreateAllocation", "D3DKMTCreateContext",
        "D3DKMTCreateDevice", "D3DKMTCreateSynchronizationObject",
        "D3DKMTDestroyAllocation", "D3DKMTDestroyContext", "D3DKMTDestroyDevice",
        "D3DKMTDestroySynchronizationObject", "D3DKMTEscape",
        "D3DKMTGetContextSchedulingPriority", "D3DKMTGetDeviceState",
        "D3DKMTGetDisplayModeList", "D3DKMTGetMultisampleMethodList",
        "D3DKMTGetRuntimeData", "D3DKMTGetSharedPrimaryHandle", "D3DKMTLock",
        "D3DKMTOpenAdapterFromHdc", "D3DKMTOpenResource", "D3DKMTPresent",
        "D3DKMTQueryAdapterInfo", "D3DKMTQueryAllocationResidency",
        "D3DKMTQueryResourceInfo", "D3DKMTRender", "D3DKMTSetAllocationPriority",
        "D3DKMTSetContextSchedulingPriority", "D3DKMTSetDisplayMode",
        "D3DKMTSetDisplayPrivateDriverFormat", "D3DKMTSetGammaRamp",
        "D3DKMTSetVidPnSourceOwner", "D3DKMTSignalSynchronizationObject",
        "D3DKMTUnlock", "D3DKMTWaitForSynchronizationObject",
        "D3DKMTWaitForVerticalBlankEvent", "D3DPerformance_BeginEvent",
        "D3DPerformance_EndEvent", "D3DPerformance_GetStatus",
        "D3DPerformance_SetMarker", "EnableFeatureLevelUpgrade",
        "OpenAdapter10", "OpenAdapter10_2",
    };
    for (const char* name : required)
    {
        if (!GetProcAddress(proxy, name))
        {
            std::fprintf(stderr, "FAIL: missing export %s\n", name);
            return 5;
        }
    }

    const auto createDevice = reinterpret_cast<CreateDeviceFn>(
        GetProcAddress(proxy, "D3D11CreateDevice"));
    const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
    };
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL selected = D3D_FEATURE_LEVEL_10_0;
    HRESULT result = createDevice(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 3,
        D3D11_SDK_VERSION, &device, &selected, &context);
    if (FAILED(result) || !device || !context)
    {
        std::fprintf(stderr, "FAIL: device creation 0x%08lX\n",
                     static_cast<unsigned long>(result));
        return 6;
    }

    ID3D11DeviceContext* earlyDeferred = nullptr;
    HRESULT earlyDeferredResult = device->CreateDeferredContext(0, &earlyDeferred);
    if (FAILED(earlyDeferredResult) || !earlyDeferred)
        return 12;
    earlyDeferred->Release();

    HWND window = CreateWindowExW(
        0, L"STATIC", L"FC3 AER smoke", WS_OVERLAPPED,
        0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    IDXGIDevice* dxgiDevice = nullptr;
    IDXGIAdapter* adapter = nullptr;
    IDXGIFactory* factory = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    result = device->QueryInterface(__uuidof(IDXGIDevice),
                                    reinterpret_cast<void**>(&dxgiDevice));
    if (SUCCEEDED(result)) result = dxgiDevice->GetAdapter(&adapter);
    if (SUCCEEDED(result)) result = adapter->GetParent(
        __uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));

    DXGI_SWAP_CHAIN_DESC description = {};
    description.BufferDesc.Width = 64;
    description.BufferDesc.Height = 64;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 1;
    description.OutputWindow = window;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    if (SUCCEEDED(result))
        result = factory->CreateSwapChain(device, &description, &swapChain);
    if (FAILED(result) || !swapChain)
    {
        std::fprintf(stderr, "FAIL: swapchain creation 0x%08lX\n",
                     static_cast<unsigned long>(result));
        return 7;
    }

    D3D11_BUFFER_DESC cameraDescription = {};
    cameraDescription.ByteWidth = 896;
    cameraDescription.Usage = D3D11_USAGE_DYNAMIC;
    cameraDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer* cameraBuffer = nullptr;
    result = device->CreateBuffer(&cameraDescription, nullptr, &cameraBuffer);

    D3D11_BUFFER_DESC readbackDescription = cameraDescription;
    readbackDescription.Usage = D3D11_USAGE_STAGING;
    readbackDescription.BindFlags = 0;
    readbackDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* readbackBuffer = nullptr;
    if (SUCCEEDED(result))
        result = device->CreateBuffer(&readbackDescription, nullptr, &readbackBuffer);

    const float expectedShifts[] = { 0.10f, -0.10f };
    for (int eye = 0; SUCCEEDED(result) && eye < 2; ++eye)
    {
        D3D11_MAPPED_SUBRESOURCE mapping = {};
        result = context->Map(
            cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapping);
        if (FAILED(result))
            break;
        std::memset(mapping.pData, 0, 896);
        float* values = static_cast<float*>(mapping.pData);
        values[0] = 1.0f;
        values[16] = 1.0f;
        context->Unmap(cameraBuffer, 0);

        context->CopyResource(readbackBuffer, cameraBuffer);
        result = context->Map(readbackBuffer, 0, D3D11_MAP_READ, 0, &mapping);
        if (FAILED(result))
            break;
        values = static_cast<float*>(mapping.pData);
        const float viewRotShift = values[3];
        const float viewShift = values[19];
        context->Unmap(readbackBuffer, 0);
        if (std::fabs(viewRotShift - expectedShifts[eye]) > 0.0001f ||
            std::fabs(viewShift - expectedShifts[eye]) > 0.0001f)
        {
            std::fprintf(stderr,
                "FAIL: eye %d shifts were %.4f/%.4f, expected %.4f\n",
                eye, viewRotShift, viewShift, expectedShifts[eye]);
            return 13;
        }
        result = swapChain->Present(0, 0);
    }
    if (cameraBuffer) cameraBuffer->Release();
    if (readbackBuffer) readbackBuffer->Release();
    if (FAILED(result))
        return 14;

    ID3D11DeviceContext* deferred = nullptr;
    ID3D11Buffer* probeBuffer = nullptr;
    ID3D11CommandList* commandList = nullptr;
    result = device->CreateDeferredContext(0, &deferred);
    void** immediateVtable = *reinterpret_cast<void***>(context);
    void** deferredVtable = *reinterpret_cast<void***>(deferred);
    std::printf("CONTEXT VT: immediate=%p deferred=%p map_equal=%s map=[%p %p]\n",
                immediateVtable, deferredVtable,
                immediateVtable[14] == deferredVtable[14] ? "yes" : "no",
                immediateVtable[14], deferredVtable[14]);
    D3D11_BUFFER_DESC probeDescription = {};
    probeDescription.ByteWidth = 64;
    probeDescription.Usage = D3D11_USAGE_DYNAMIC;
    probeDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    probeDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (SUCCEEDED(result))
        result = device->CreateBuffer(&probeDescription, nullptr, &probeBuffer);
    D3D11_MAPPED_SUBRESOURCE probeMapping = {};
    if (SUCCEEDED(result))
        result = deferred->Map(probeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &probeMapping);
    if (SUCCEEDED(result))
    {
        const float projection[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, -0.1f, 0.0f,
        };
        std::memcpy(probeMapping.pData, projection, sizeof(projection));
        deferred->Unmap(probeBuffer, 0);
        deferred->VSSetConstantBuffers(0, 1, &probeBuffer);
        result = deferred->FinishCommandList(FALSE, &commandList);
    }
    if (FAILED(result))
    {
        std::fprintf(stderr, "FAIL: deferred constant-buffer probe 0x%08lX\n",
                     static_cast<unsigned long>(result));
        return 11;
    }
    context->ExecuteCommandList(commandList, FALSE);
    commandList->Release();
    probeBuffer->Release();
    deferred->Release();

    for (int index = 0; index < 119; ++index)
    {
        result = swapChain->Present(0, 0);
        if (FAILED(result))
            return 8;
    }

    swapChain->Release();
    factory->Release();
    adapter->Release();
    dxgiDevice->Release();
    DestroyWindow(window);
    context->Release();
    device->Release();
    FreeLibrary(proxy);

    HANDLE log = CreateFileW(logPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (log == INVALID_HANDLE_VALUE)
        return 9;
    char contents[4096] = {};
    DWORD bytesRead = 0;
    ReadFile(log, contents, sizeof(contents) - 1, &bytesRead, nullptr);
    CloseHandle(log);

    const char* expected[] = {
        "aer_present frame=0 eye=LEFT", "aer_present frame=1 eye=RIGHT",
        "aer_present frame=2 eye=LEFT", "aer_present frame=3 eye=RIGHT",
    };
    for (const char* line : expected)
    {
        if (!std::strstr(contents, line))
        {
            std::fprintf(stderr, "FAIL: missing log sequence: %s\n", line);
            return 10;
        }
    }

    std::printf("PASS: 51 exports; D3D11; AER L/R; 896-byte camera shifts +0.10/-0.10.\n");
    return 0;
}
