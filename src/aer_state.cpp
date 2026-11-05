#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdarg>
#include <cstdio>

#include "aer_state.h"
#include "aer_camera.h"

namespace
{
using CreateSwapChainFn = HRESULT (STDMETHODCALLTYPE*)(
    IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
using PresentFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);

constexpr size_t kFactoryCreateSwapChainSlot = 10;
constexpr size_t kSwapChainPresentSlot = 8;

SRWLOCK g_lock = SRWLOCK_INIT;
HANDLE g_log = INVALID_HANDLE_VALUE;
CreateSwapChainFn g_originalCreateSwapChain = nullptr;
PresentFn g_originalPresent = nullptr;
IDXGISwapChain* g_primarySwapChain = nullptr;
volatile LONG g_primaryPixelCount = 0;
volatile LONG g_frameNumber = 0;
volatile LONG g_deviceHookCount = 0;

void Log(const char* format, ...)
{
    if (g_log == INVALID_HANDLE_VALUE)
        return;

    char line[512] = {};
    va_list args;
    va_start(args, format);
    const int count = _vsnprintf_s(line, sizeof(line), _TRUNCATE, format, args);
    va_end(args);
    if (count <= 0)
        return;

    DWORD written = 0;
    WriteFile(g_log, line, static_cast<DWORD>(count), &written, nullptr);
    FlushFileBuffers(g_log);
}

bool OpenLog(HMODULE proxyModule)
{
    if (g_log != INVALID_HANDLE_VALUE)
        return true;

    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(proxyModule, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return false;

    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash)
        return false;
    *(slash + 1) = L'\0';
    if (lstrlenW(path) + 12 >= MAX_PATH)
        return false;
    lstrcatW(path, L"fc3_aer.log");

    g_log = CreateFileW(
        path,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (g_log == INVALID_HANDLE_VALUE)
        return false;

    Log("FC3 clean-room AER frame core v1\r\n");
    Log("scope=clean-aer-camera; camera_cb=896; openxr=off\r\n");
    return true;
}

bool PatchVtableSlot(void* object, size_t slotIndex, void* hook, void** original)
{
    if (!object || !hook || !original)
        return false;

    void** vtable = *reinterpret_cast<void***>(object);
    void** slot = &vtable[slotIndex];

    AcquireSRWLockExclusive(&g_lock);
    if (*slot == hook)
    {
        const bool valid = (*original != nullptr);
        ReleaseSRWLockExclusive(&g_lock);
        return valid;
    }

    DWORD oldProtection = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection))
    {
        ReleaseSRWLockExclusive(&g_lock);
        return false;
    }

    *original = *slot;
    InterlockedExchangePointer(slot, hook);

    DWORD ignored = 0;
    VirtualProtect(slot, sizeof(void*), oldProtection, &ignored);
    FlushInstructionCache(GetCurrentProcess(), slot, sizeof(void*));
    ReleaseSRWLockExclusive(&g_lock);
    return true;
}

HRESULT STDMETHODCALLTYPE HookPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    IDXGISwapChain* primary = reinterpret_cast<IDXGISwapChain*>(
        InterlockedCompareExchangePointer(
            reinterpret_cast<PVOID volatile*>(&g_primarySwapChain), nullptr, nullptr));
    if (swapChain != primary)
        return g_originalPresent(swapChain, syncInterval, flags);

    const LONG frame = InterlockedCompareExchange(&g_frameNumber, 0, 0);
    const char* eye = ((frame & 1) == 0) ? "LEFT" : "RIGHT";
    if (frame < 16 || (frame % 600) == 0)
        Log("aer_present frame=%ld eye=%s\r\n", frame, eye);
    aercamera::OnPresent(frame);

    const HRESULT result = g_originalPresent(swapChain, syncInterval, flags);
    InterlockedIncrement(&g_frameNumber);
    return result;
}

HRESULT STDMETHODCALLTYPE HookCreateSwapChain(
    IDXGIFactory* factory,
    IUnknown* device,
    DXGI_SWAP_CHAIN_DESC* description,
    IDXGISwapChain** swapChain)
{
    const HRESULT result = g_originalCreateSwapChain(factory, device, description, swapChain);
    if (SUCCEEDED(result) && swapChain && *swapChain)
    {
        void* original = reinterpret_cast<void*>(g_originalPresent);
        const bool hooked = PatchVtableSlot(
            *swapChain,
            kSwapChainPresentSlot,
            reinterpret_cast<void*>(&HookPresent),
            &original);
        g_originalPresent = reinterpret_cast<PresentFn>(original);

        LONG pixelCount = 0;
        if (description)
        {
            const unsigned long long pixels =
                static_cast<unsigned long long>(description->BufferDesc.Width) *
                static_cast<unsigned long long>(description->BufferDesc.Height);
            if (pixels <= 0x7fffffffULL)
                pixelCount = static_cast<LONG>(pixels);
        }
        const LONG currentLargest = InterlockedCompareExchange(
            &g_primaryPixelCount, 0, 0);
        bool primarySelected = false;
        if (!g_primarySwapChain || pixelCount >= currentLargest)
        {
            InterlockedExchange(&g_primaryPixelCount, pixelCount);
            InterlockedExchangePointer(
                reinterpret_cast<PVOID volatile*>(&g_primarySwapChain), *swapChain);
            InterlockedExchange(&g_frameNumber, 0);
            primarySelected = true;
        }

        if (primarySelected && device)
        {
            ID3D11Device* d3dDevice = nullptr;
            if (SUCCEEDED(device->QueryInterface(
                    __uuidof(ID3D11Device), reinterpret_cast<void**>(&d3dDevice))))
            {
                ID3D11DeviceContext* immediateContext = nullptr;
                d3dDevice->GetImmediateContext(&immediateContext);
                aercamera::Install(
                    d3dDevice, immediateContext,
                    [](const char* line) { Log("%s", line); });
                if (immediateContext)
                    immediateContext->Release();
                d3dDevice->Release();
            }
        }

        if (description)
        {
            Log("swapchain=%ux%u format=%u present_hook=%s\r\n",
                description->BufferDesc.Width,
                description->BufferDesc.Height,
                static_cast<unsigned>(description->BufferDesc.Format),
                hooked ? "ok" : "failed");
        }
        else
        {
            Log("swapchain=unknown present_hook=%s\r\n", hooked ? "ok" : "failed");
        }
    }
    return result;
}

bool HookFactoryFromDevice(ID3D11Device* device)
{
    IDXGIDevice* dxgiDevice = nullptr;
    IDXGIAdapter* adapter = nullptr;
    IDXGIFactory* factory = nullptr;

    HRESULT result = device->QueryInterface(__uuidof(IDXGIDevice),
                                             reinterpret_cast<void**>(&dxgiDevice));
    if (SUCCEEDED(result))
        result = dxgiDevice->GetAdapter(&adapter);
    if (SUCCEEDED(result))
        result = adapter->GetParent(__uuidof(IDXGIFactory),
                                    reinterpret_cast<void**>(&factory));

    bool hooked = false;
    if (SUCCEEDED(result) && factory)
    {
        void* original = reinterpret_cast<void*>(g_originalCreateSwapChain);
        hooked = PatchVtableSlot(
            factory,
            kFactoryCreateSwapChainSlot,
            reinterpret_cast<void*>(&HookCreateSwapChain),
            &original);
        g_originalCreateSwapChain = reinterpret_cast<CreateSwapChainFn>(original);
    }

    if (factory) factory->Release();
    if (adapter) adapter->Release();
    if (dxgiDevice) dxgiDevice->Release();
    return hooked;
}
}

namespace fc3aer
{
bool InstallFrameHooks(ID3D11Device* device, HMODULE proxyModule)
{
    if (!device)
        return false;

    AcquireSRWLockExclusive(&g_lock);
    const bool logReady = OpenLog(proxyModule);
    ReleaseSRWLockExclusive(&g_lock);

    const LONG hookCount = InterlockedIncrement(&g_deviceHookCount);
    const bool factoryHooked = HookFactoryFromDevice(device);
    if (hookCount <= 4 || !factoryHooked)
        Log("d3d11_device=%ld factory_hook=%s\r\n",
            hookCount, factoryHooked ? "ok" : "failed");
    return logReady && factoryHooked;
}

Eye CurrentEye()
{
    const LONG frame = InterlockedCompareExchange(&g_frameNumber, 0, 0);
    return ((frame & 1) == 0) ? Eye::Left : Eye::Right;
}

void Shutdown()
{
    aercamera::Shutdown();
    AcquireSRWLockExclusive(&g_lock);
    if (g_log != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_log);
        g_log = INVALID_HANDLE_VALUE;
    }
    ReleaseSRWLockExclusive(&g_lock);
}
}
