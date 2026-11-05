#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "buffer_snapshot.h"

namespace
{
using CreateBufferFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D11Device*, const D3D11_BUFFER_DESC*, const D3D11_SUBRESOURCE_DATA*,
    ID3D11Buffer**);

constexpr size_t kCreateBufferSlot = 3;
constexpr UINT kMaximumDeviceVtables = 64;
constexpr UINT kMaximumBuffers = 4096;

struct DeviceHook
{
    void** vtable;
    CreateBufferFn createBuffer;
};

struct BufferRecord
{
    IUnknown* deviceIdentity;
    ID3D11Buffer* buffer;
    UINT byteWidth;
};

SRWLOCK g_lock = SRWLOCK_INIT;
DeviceHook g_deviceHooks[kMaximumDeviceVtables] = {};
UINT g_deviceHookCount = 0;
BufferRecord g_buffers[kMaximumBuffers] = {};
UINT g_bufferCount = 0;
ID3D11Device* g_primaryDevice = nullptr;
IUnknown* g_primaryIdentity = nullptr;
ID3D11DeviceContext* g_primaryContext = nullptr;
buffersnapshot::LogLineFn g_logLine = nullptr;
bool g_dumped120 = false;
bool g_dumped600 = false;

void Emit(const char* format, ...)
{
    if (!g_logLine)
        return;
    char line[640] = {};
    va_list args;
    va_start(args, format);
    _vsnprintf_s(line, sizeof(line), _TRUNCATE, format, args);
    va_end(args);
    g_logLine(line);
}

DeviceHook* FindDeviceHook(ID3D11Device* device)
{
    void** vtable = *reinterpret_cast<void***>(device);
    for (UINT index = 0; index < g_deviceHookCount; ++index)
    {
        if (g_deviceHooks[index].vtable == vtable)
            return &g_deviceHooks[index];
    }
    return nullptr;
}

bool PatchCreateBuffer(ID3D11Device* device)
{
    if (!device)
        return false;
    if (FindDeviceHook(device))
        return true;
    if (g_deviceHookCount >= kMaximumDeviceVtables)
        return false;

    void** vtable = *reinterpret_cast<void***>(device);
    void** slot = &vtable[kCreateBufferSlot];
    DeviceHook* hook = &g_deviceHooks[g_deviceHookCount++];
    hook->vtable = vtable;

    DWORD oldProtection = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection))
        return false;
    hook->createBuffer = reinterpret_cast<CreateBufferFn>(*slot);
    DWORD ignored = 0;
    VirtualProtect(slot, sizeof(void*), oldProtection, &ignored);
    return true;
}

HRESULT STDMETHODCALLTYPE HookCreateBuffer(
    ID3D11Device* device,
    const D3D11_BUFFER_DESC* description,
    const D3D11_SUBRESOURCE_DATA* initialData,
    ID3D11Buffer** buffer)
{
    DeviceHook* hook = FindDeviceHook(device);
    if (!hook || !hook->createBuffer)
        return E_FAIL;

    const HRESULT result = hook->createBuffer(device, description, initialData, buffer);
    if (SUCCEEDED(result) && description && buffer && *buffer &&
        (description->BindFlags & D3D11_BIND_CONSTANT_BUFFER) != 0 &&
        description->ByteWidth >= 64 && description->ByteWidth <= 16384)
    {
        IUnknown* identity = nullptr;
        if (SUCCEEDED(device->QueryInterface(
                __uuidof(IUnknown), reinterpret_cast<void**>(&identity))))
        {
            AcquireSRWLockExclusive(&g_lock);
            if (g_bufferCount < kMaximumBuffers)
            {
                BufferRecord* record = &g_buffers[g_bufferCount++];
                record->deviceIdentity = identity;
                record->buffer = *buffer;
                record->buffer->AddRef();
                record->byteWidth = description->ByteWidth;
                identity = nullptr;
            }
            ReleaseSRWLockExclusive(&g_lock);
            if (identity)
                identity->Release();
        }
    }
    return result;
}

bool InstallCreateBufferHook(ID3D11Device* device)
{
    if (!PatchCreateBuffer(device))
        return false;
    DeviceHook* hook = FindDeviceHook(device);
    void** slot = &hook->vtable[kCreateBufferSlot];
    if (*slot == reinterpret_cast<void*>(&HookCreateBuffer))
        return true;

    DWORD oldProtection = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection))
        return false;
    InterlockedExchangePointer(slot, reinterpret_cast<void*>(&HookCreateBuffer));
    DWORD ignored = 0;
    VirtualProtect(slot, sizeof(void*), oldProtection, &ignored);
    FlushInstructionCache(GetCurrentProcess(), slot, sizeof(void*));
    return true;
}

float Length3(float x, float y, float z)
{
    return std::sqrt(x * x + y * y + z * z);
}

int ScoreMatrix(const float* matrix)
{
    for (int index = 0; index < 16; ++index)
    {
        if (!std::isfinite(matrix[index]) || std::fabs(matrix[index]) > 10000000.0f)
            return 0;
    }
    int score = 0;
    if (std::fabs(matrix[15]) < 0.02f)
    {
        score += 2;
        if (std::fabs(std::fabs(matrix[11]) - 1.0f) < 0.15f ||
            std::fabs(std::fabs(matrix[14]) - 1.0f) < 0.15f)
            score += 4;
        if (std::fabs(matrix[0]) > 0.05f && std::fabs(matrix[5]) > 0.05f)
            score += 1;
    }
    if (std::fabs(matrix[15] - 1.0f) < 0.02f)
    {
        const float rowError =
            std::fabs(Length3(matrix[0], matrix[1], matrix[2]) - 1.0f) +
            std::fabs(Length3(matrix[4], matrix[5], matrix[6]) - 1.0f) +
            std::fabs(Length3(matrix[8], matrix[9], matrix[10]) - 1.0f);
        const float colError =
            std::fabs(Length3(matrix[0], matrix[4], matrix[8]) - 1.0f) +
            std::fabs(Length3(matrix[1], matrix[5], matrix[9]) - 1.0f) +
            std::fabs(Length3(matrix[2], matrix[6], matrix[10]) - 1.0f);
        if (rowError < 0.35f || colError < 0.35f)
            score += 5;
    }
    return score;
}

void Snapshot(LONG frameNumber)
{
    if (!g_primaryDevice || !g_primaryContext || !g_primaryIdentity)
        return;

    UINT primaryBuffers = 0;
    UINT readSuccesses = 0;
    UINT matrixCandidates = 0;
    AcquireSRWLockShared(&g_lock);
    const UINT count = g_bufferCount;
    for (UINT index = 0; index < count; ++index)
    {
        const BufferRecord& record = g_buffers[index];
        if (record.deviceIdentity != g_primaryIdentity)
            continue;
        ++primaryBuffers;

        D3D11_BUFFER_DESC stagingDescription = {};
        stagingDescription.ByteWidth = record.byteWidth;
        stagingDescription.Usage = D3D11_USAGE_STAGING;
        stagingDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ID3D11Buffer* staging = nullptr;
        if (FAILED(g_primaryDevice->CreateBuffer(
                &stagingDescription, nullptr, &staging)) || !staging)
            continue;

        g_primaryContext->CopyResource(staging, record.buffer);
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(g_primaryContext->Map(
                staging, 0, D3D11_MAP_READ, 0, &mapped)))
        {
            ++readSuccesses;
            int bestScore = 0;
            UINT bestOffset = 0;
            float best[16] = {};
            const auto* bytes = static_cast<const unsigned char*>(mapped.pData);
            for (UINT offset = 0; offset + 64 <= record.byteWidth; offset += 16)
            {
                float matrix[16] = {};
                std::memcpy(matrix, bytes + offset, sizeof(matrix));
                const int score = ScoreMatrix(matrix);
                if (score > bestScore)
                {
                    bestScore = score;
                    bestOffset = offset;
                    std::memcpy(best, matrix, sizeof(matrix));
                }
            }
            g_primaryContext->Unmap(staging, 0);
            if (bestScore >= 5)
            {
                ++matrixCandidates;
                Emit("snapshot_candidate frame=%ld id=%u buffer=%p bytes=%u score=%d offset=%u m=[%.4g %.4g %.4g %.4g|%.4g %.4g %.4g %.4g|%.4g %.4g %.4g %.4g|%.4g %.4g %.4g %.4g]\r\n",
                    frameNumber, index, record.buffer, record.byteWidth,
                    bestScore, bestOffset,
                    best[0], best[1], best[2], best[3],
                    best[4], best[5], best[6], best[7],
                    best[8], best[9], best[10], best[11],
                    best[12], best[13], best[14], best[15]);
            }
        }
        staging->Release();
    }
    ReleaseSRWLockShared(&g_lock);
    Emit("snapshot_dump frame=%ld tracked=%u primary=%u read_ok=%u candidates=%u\r\n",
         frameNumber, count, primaryBuffers, readSuccesses, matrixCandidates);
}
}

namespace buffersnapshot
{
bool ArmDevice(ID3D11Device* device)
{
    return InstallCreateBufferHook(device);
}

bool SelectPrimary(ID3D11Device* device, LogLineFn logLine)
{
    if (!device)
        return false;
    g_logLine = logLine;
    g_primaryDevice = device;
    if (g_primaryIdentity)
        g_primaryIdentity->Release();
    g_primaryIdentity = nullptr;
    device->QueryInterface(__uuidof(IUnknown),
                           reinterpret_cast<void**>(&g_primaryIdentity));
    if (g_primaryContext)
        g_primaryContext->Release();
    g_primaryContext = nullptr;
    device->GetImmediateContext(&g_primaryContext);
    Emit("buffer_snapshot=selected tracked=%u device_hooks=%u identity=%p\r\n",
         g_bufferCount, g_deviceHookCount, g_primaryIdentity);
    return g_primaryIdentity && g_primaryContext;
}

void OnPresent(LONG frameNumber)
{
    if (frameNumber >= 120 && !g_dumped120)
    {
        g_dumped120 = true;
        Snapshot(frameNumber);
    }
    if (frameNumber >= 600 && !g_dumped600)
    {
        g_dumped600 = true;
        Snapshot(frameNumber);
    }
}

void Shutdown()
{
    g_logLine = nullptr;
    // The retained probe objects intentionally live until process teardown;
    // releasing COM objects from DllMain is unsafe.
}
}
