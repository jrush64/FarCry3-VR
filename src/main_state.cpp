#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <shlobj.h>
#include <cstdarg>
#include <cstdio>

#include "fc3_log.h"
#include "main_state.h"
#include "stereo.h"

namespace
{
using CreateSwapChainFn = HRESULT (STDMETHODCALLTYPE*)(
    IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
using PresentFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);

constexpr size_t kFactoryCreateSwapChainSlot = 10;
constexpr size_t kSwapChainPresentSlot = 8;

SRWLOCK g_lock = SRWLOCK_INIT;
SRWLOCK g_logLock = SRWLOCK_INIT;
HANDLE g_log = INVALID_HANDLE_VALUE;
wchar_t g_dataFolder[MAX_PATH] = {};
CreateSwapChainFn g_originalCreateSwapChain = nullptr;
PresentFn g_originalPresent = nullptr;
IDXGISwapChain* g_primarySwapChain = nullptr;
volatile LONG g_primaryPixelCount = 0;
volatile LONG g_deviceCount = 0;

void OpenLog()
{
    if (g_log != INVALID_HANDLE_VALUE)
        return;

    wchar_t base[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, base)))
        return;
    if (lstrlenW(base) + 64 >= MAX_PATH)
        return;
    lstrcatW(base, L"\\FarCry3VR");
    CreateDirectoryW(base, nullptr);
    lstrcpyW(g_dataFolder, base);
    lstrcatW(g_dataFolder, L"\\");

    // One log per host process, and the previous run is kept, never deleted.
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    const wchar_t* name = wcsrchr(exe, L'\\');
    name = name ? name + 1 : exe;
    wchar_t stem[64] = {};
    lstrcpynW(stem, name, 60);
    if (wchar_t* dot = wcsrchr(stem, L'.')) *dot = L'\0';

    wchar_t path[MAX_PATH] = {};
    wchar_t previous[MAX_PATH] = {};
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%sfc3_stereo_%s.log", g_dataFolder, stem);
    _snwprintf_s(previous, MAX_PATH, _TRUNCATE, L"%sfc3_stereo_%s.previous.log", g_dataFolder, stem);
    MoveFileExW(path, previous, MOVEFILE_REPLACE_EXISTING);

    g_log = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
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
    const bool primary = swapChain == g_primarySwapChain && !(flags & DXGI_PRESENT_TEST);
    if (primary)
        fc3stereo::OnPresent(swapChain);
    const HRESULT result = g_originalPresent(swapChain, syncInterval, flags);
    if (primary)
        fc3stereo::AfterPresent(swapChain);
    return result;
}

void AdoptSwapChain(IDXGISwapChain* swapChain, IUnknown* deviceUnknown, const DXGI_SWAP_CHAIN_DESC* description)
{
    void* original = reinterpret_cast<void*>(g_originalPresent);
    const bool hooked = PatchVtableSlot(
        swapChain, kSwapChainPresentSlot, reinterpret_cast<void*>(&HookPresent), &original);
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

    // The game's main window is the largest swapchain. Overlays and helper
    // devices create small ones.
    bool primary = false;
    if (!g_primarySwapChain || pixelCount >= g_primaryPixelCount)
    {
        g_primaryPixelCount = pixelCount;
        g_primarySwapChain = swapChain;
        primary = true;
    }

    if (description)
        fc3::Log("[SWAP] %ux%u format=%u buffers=%u samples=%u effect=%u windowed=%d present_hook=%s primary=%d",
                 description->BufferDesc.Width, description->BufferDesc.Height,
                 static_cast<unsigned>(description->BufferDesc.Format), description->BufferCount,
                 description->SampleDesc.Count, static_cast<unsigned>(description->SwapEffect),
                 description->Windowed ? 1 : 0, hooked ? "ok" : "FAILED", primary ? 1 : 0);

    if (primary && deviceUnknown)
    {
        ID3D11Device* device = nullptr;
        if (SUCCEEDED(deviceUnknown->QueryInterface(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device))) && device)
        {
            fc3stereo::OnPrimarySwapChain(swapChain, device);
            device->Release();
        }
    }
}

HRESULT STDMETHODCALLTYPE HookCreateSwapChain(
    IDXGIFactory* factory, IUnknown* device, DXGI_SWAP_CHAIN_DESC* description,
    IDXGISwapChain** swapChain)
{
    const HRESULT result = g_originalCreateSwapChain(factory, device, description, swapChain);
    if (SUCCEEDED(result) && swapChain && *swapChain)
        AdoptSwapChain(*swapChain, device, description);
    return result;
}

bool HookFactoryFromDevice(ID3D11Device* device)
{
    IDXGIDevice* dxgiDevice = nullptr;
    IDXGIAdapter* adapter = nullptr;
    IDXGIFactory* factory = nullptr;

    HRESULT result = device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    if (SUCCEEDED(result))
        result = dxgiDevice->GetAdapter(&adapter);
    if (SUCCEEDED(result))
        result = adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));

    bool hooked = false;
    if (SUCCEEDED(result) && factory)
    {
        void* original = reinterpret_cast<void*>(g_originalCreateSwapChain);
        hooked = PatchVtableSlot(factory, kFactoryCreateSwapChainSlot,
                                 reinterpret_cast<void*>(&HookCreateSwapChain), &original);
        g_originalCreateSwapChain = reinterpret_cast<CreateSwapChainFn>(original);
    }

    if (factory) factory->Release();
    if (adapter) adapter->Release();
    if (dxgiDevice) dxgiDevice->Release();
    return hooked;
}
}

namespace fc3
{
void Log(const char* format, ...)
{
    if (g_log == INVALID_HANDLE_VALUE)
        return;
    char line[640] = {};
    va_list args;
    va_start(args, format);
    int count = _vsnprintf_s(line, sizeof(line) - 2, _TRUNCATE, format, args);
    va_end(args);
    if (count < 0)
        count = static_cast<int>(strlen(line));
    line[count++] = '\r';
    line[count++] = '\n';

    AcquireSRWLockExclusive(&g_logLock);
    DWORD written = 0;
    WriteFile(g_log, line, static_cast<DWORD>(count), &written, nullptr);
    ReleaseSRWLockExclusive(&g_logLock);
}

const wchar_t* DataFolder()
{
    return g_dataFolder;
}

bool OnDeviceCreated(ID3D11Device* device, HMODULE)
{
    if (!device)
        return false;

    AcquireSRWLockExclusive(&g_lock);
    const bool first = g_log == INVALID_HANDLE_VALUE;
    OpenLog();
    ReleaseSRWLockExclusive(&g_lock);
    if (first)
        Log("Far Cry 3 same-frame stereo, build " __DATE__ " " __TIME__);

    const LONG count = InterlockedIncrement(&g_deviceCount);
    const bool factoryHooked = HookFactoryFromDevice(device);
    if (count <= 4 || !factoryHooked)
        Log("[DEV] d3d11 device #%ld factory_hook=%s", count, factoryHooked ? "ok" : "FAILED");
    fc3stereo::OnDeviceCreated(device);
    return factoryHooked;
}

void OnSwapChainCreated(IDXGISwapChain* swapChain, ID3D11Device* device, const DXGI_SWAP_CHAIN_DESC* description)
{
    if (swapChain && device)
        AdoptSwapChain(swapChain, device, description);
}

void Shutdown()
{
    // Hooks stay in place until the process ends; removing detours from
    // DllMain can race the render thread.
}
}
