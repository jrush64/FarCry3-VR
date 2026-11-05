#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <intrin.h>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "matrix_probe.h"

namespace
{
using VSSetConstantBuffersFn = void (STDMETHODCALLTYPE*)(
    ID3D11DeviceContext*, UINT, UINT, ID3D11Buffer* const*);
using MapFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT,
    D3D11_MAPPED_SUBRESOURCE*);
using UnmapFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT);
using UpdateSubresourceFn = void (STDMETHODCALLTYPE*)(
    ID3D11DeviceContext*, ID3D11Resource*, UINT, const D3D11_BOX*,
    const void*, UINT, UINT);
using CreateDeferredContextFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D11Device*, UINT, ID3D11DeviceContext**);

constexpr size_t kVSSetConstantBuffersSlot = 7;
constexpr size_t kMapSlot = 14;
constexpr size_t kUnmapSlot = 15;
constexpr size_t kUpdateSubresourceSlot = 48;
constexpr size_t kCreateDeferredContextSlot = 27;
constexpr UINT kMaximumCandidates = 512;
constexpr UINT kMaximumContextVtables = 512;
constexpr UINT kMaximumContextCache = 128;

struct Candidate
{
    ID3D11Buffer* buffer;
    UINT byteWidth;
    UINT vertexSlotMask;
    UINT updateCount;
    UINT bestOffset;
    int bestScore;
    float bestMatrix[16];
    void* mappedData;
    void* writerAddress;
};

struct ContextVtableHook
{
    void** vtable;
    VSSetConstantBuffersFn vsSetConstantBuffers;
    MapFn map;
    UnmapFn unmap;
    UpdateSubresourceFn updateSubresource;
};

struct ContextCacheEntry
{
    ID3D11DeviceContext* context;
    bool belongsToPrimaryDevice;
};

SRWLOCK g_lock = SRWLOCK_INIT;
ID3D11Device* g_primaryDevice = nullptr;
CreateDeferredContextFn g_originalCreateDeferredContext = nullptr;
matrixprobe::LogLineFn g_logLine = nullptr;
Candidate g_candidates[kMaximumCandidates] = {};
UINT g_candidateCount = 0;
ContextVtableHook g_contextHooks[kMaximumContextVtables] = {};
UINT g_contextHookCount = 0;
ContextCacheEntry g_contextCache[kMaximumContextCache] = {};
UINT g_contextCacheCount = 0;
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

bool ReplaceVtableSlot(void** vtable, size_t index, void* hook, void** original)
{
    void** slot = &vtable[index];
    if (*slot == hook)
        return *original != nullptr;
    DWORD oldProtection = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection))
        return false;
    *original = *slot;
    InterlockedExchangePointer(slot, hook);
    DWORD ignored = 0;
    VirtualProtect(slot, sizeof(void*), oldProtection, &ignored);
    FlushInstructionCache(GetCurrentProcess(), slot, sizeof(void*));
    return true;
}

ContextVtableHook* FindContextHook(ID3D11DeviceContext* context)
{
    void** vtable = *reinterpret_cast<void***>(context);
    for (UINT index = 0; index < g_contextHookCount; ++index)
    {
        if (g_contextHooks[index].vtable == vtable)
            return &g_contextHooks[index];
    }
    return nullptr;
}

bool ContextBelongsToPrimaryDevice(ID3D11DeviceContext*)
{
    // Deferred contexts can execute before swapchain selection. Once the
    // primary device is selected, capture every already-patched context;
    // candidate frequency and vertex binding identify the main view.
    return g_primaryDevice != nullptr;
}

Candidate* FindCandidate(ID3D11Buffer* buffer, bool create)
{
    for (UINT index = 0; index < g_candidateCount; ++index)
    {
        if (g_candidates[index].buffer == buffer)
            return &g_candidates[index];
    }
    if (!create || g_candidateCount >= kMaximumCandidates)
        return nullptr;

    D3D11_BUFFER_DESC description = {};
    buffer->GetDesc(&description);
    if ((description.BindFlags & D3D11_BIND_CONSTANT_BUFFER) == 0 ||
        description.ByteWidth < 64 || description.ByteWidth > 16384)
        return nullptr;

    Candidate* candidate = &g_candidates[g_candidateCount++];
    candidate->buffer = buffer;
    candidate->byteWidth = description.ByteWidth;
    return candidate;
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

void InspectData(Candidate* candidate, const void* source, void* writer)
{
    if (!candidate || !source)
        return;
    const auto* bytes = static_cast<const unsigned char*>(source);
    for (UINT offset = 0; offset + 64 <= candidate->byteWidth; offset += 16)
    {
        float matrix[16] = {};
        std::memcpy(matrix, bytes + offset, sizeof(matrix));
        const int score = ScoreMatrix(matrix);
        if (score > candidate->bestScore)
        {
            candidate->bestScore = score;
            candidate->bestOffset = offset;
            std::memcpy(candidate->bestMatrix, matrix, sizeof(matrix));
        }
    }
    ++candidate->updateCount;
    candidate->writerAddress = writer;
}

void STDMETHODCALLTYPE HookVSSetConstantBuffers(
    ID3D11DeviceContext* context, UINT startSlot, UINT bufferCount,
    ID3D11Buffer* const* buffers)
{
    ContextVtableHook* hook = FindContextHook(context);
    if (ContextBelongsToPrimaryDevice(context) && buffers)
    {
        AcquireSRWLockExclusive(&g_lock);
        for (UINT index = 0; index < bufferCount; ++index)
        {
            if (!buffers[index])
                continue;
            Candidate* candidate = FindCandidate(buffers[index], true);
            const UINT slot = startSlot + index;
            if (candidate && slot < 32)
                candidate->vertexSlotMask |= (1u << slot);
        }
        ReleaseSRWLockExclusive(&g_lock);
    }
    hook->vsSetConstantBuffers(context, startSlot, bufferCount, buffers);
}

HRESULT STDMETHODCALLTYPE HookMap(
    ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource,
    D3D11_MAP mapType, UINT mapFlags, D3D11_MAPPED_SUBRESOURCE* mapped)
{
    ContextVtableHook* hook = FindContextHook(context);
    const HRESULT result = hook->map(
        context, resource, subresource, mapType, mapFlags, mapped);
    if (ContextBelongsToPrimaryDevice(context) && SUCCEEDED(result) && resource && mapped)
    {
        ID3D11Buffer* buffer = nullptr;
        if (SUCCEEDED(resource->QueryInterface(
                __uuidof(ID3D11Buffer), reinterpret_cast<void**>(&buffer))))
        {
            AcquireSRWLockExclusive(&g_lock);
            Candidate* candidate = FindCandidate(buffer, true);
            if (candidate)
                candidate->mappedData = mapped->pData;
            ReleaseSRWLockExclusive(&g_lock);
            buffer->Release();
        }
    }
    return result;
}

void STDMETHODCALLTYPE HookUnmap(
    ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource)
{
    ContextVtableHook* hook = FindContextHook(context);
    if (ContextBelongsToPrimaryDevice(context) && resource)
    {
        ID3D11Buffer* buffer = nullptr;
        if (SUCCEEDED(resource->QueryInterface(
                __uuidof(ID3D11Buffer), reinterpret_cast<void**>(&buffer))))
        {
            AcquireSRWLockExclusive(&g_lock);
            Candidate* candidate = FindCandidate(buffer, false);
            if (candidate && candidate->mappedData)
            {
                InspectData(candidate, candidate->mappedData, _ReturnAddress());
                candidate->mappedData = nullptr;
            }
            ReleaseSRWLockExclusive(&g_lock);
            buffer->Release();
        }
    }
    hook->unmap(context, resource, subresource);
}

void STDMETHODCALLTYPE HookUpdateSubresource(
    ID3D11DeviceContext* context, ID3D11Resource* destination, UINT subresource,
    const D3D11_BOX* box, const void* source, UINT rowPitch, UINT depthPitch)
{
    ContextVtableHook* hook = FindContextHook(context);
    if (ContextBelongsToPrimaryDevice(context) && destination && source)
    {
        ID3D11Buffer* buffer = nullptr;
        if (SUCCEEDED(destination->QueryInterface(
                __uuidof(ID3D11Buffer), reinterpret_cast<void**>(&buffer))))
        {
            AcquireSRWLockExclusive(&g_lock);
            InspectData(FindCandidate(buffer, true), source, _ReturnAddress());
            ReleaseSRWLockExclusive(&g_lock);
            buffer->Release();
        }
    }
    hook->updateSubresource(
        context, destination, subresource, box, source, rowPitch, depthPitch);
}

bool PatchContext(ID3D11DeviceContext* context)
{
    if (!context)
        return false;
    void** vtable = *reinterpret_cast<void***>(context);
    if (FindContextHook(context))
        return true;
    if (g_contextHookCount >= kMaximumContextVtables)
        return false;

    ContextVtableHook* hook = &g_contextHooks[g_contextHookCount++];
    hook->vtable = vtable;
    void* vs = nullptr;
    void* map = nullptr;
    void* unmap = nullptr;
    void* update = nullptr;
    const bool vsOk = ReplaceVtableSlot(
        vtable, kVSSetConstantBuffersSlot,
        reinterpret_cast<void*>(&HookVSSetConstantBuffers), &vs);
    const bool mapOk = ReplaceVtableSlot(
        vtable, kMapSlot, reinterpret_cast<void*>(&HookMap), &map);
    const bool unmapOk = ReplaceVtableSlot(
        vtable, kUnmapSlot, reinterpret_cast<void*>(&HookUnmap), &unmap);
    const bool updateOk = ReplaceVtableSlot(
        vtable, kUpdateSubresourceSlot,
        reinterpret_cast<void*>(&HookUpdateSubresource), &update);
    hook->vsSetConstantBuffers = reinterpret_cast<VSSetConstantBuffersFn>(vs);
    hook->map = reinterpret_cast<MapFn>(map);
    hook->unmap = reinterpret_cast<UnmapFn>(unmap);
    hook->updateSubresource = reinterpret_cast<UpdateSubresourceFn>(update);
    Emit("matrix_probe=context_vtable=%p vs=%s map=%s unmap=%s update=%s\r\n",
         vtable, vsOk ? "ok" : "failed", mapOk ? "ok" : "failed",
         unmapOk ? "ok" : "failed", updateOk ? "ok" : "failed");
    return vsOk && mapOk && unmapOk && updateOk;
}

HRESULT STDMETHODCALLTYPE HookCreateDeferredContext(
    ID3D11Device* device, UINT flags, ID3D11DeviceContext** deferredContext)
{
    const HRESULT result = g_originalCreateDeferredContext(device, flags, deferredContext);
    if (SUCCEEDED(result) && deferredContext && *deferredContext)
        PatchContext(*deferredContext);
    return result;
}

void DumpCandidates(LONG frameNumber)
{
    const uintptr_t engineBase = reinterpret_cast<uintptr_t>(
        GetModuleHandleW(L"FC3_d3d11.dll"));
    AcquireSRWLockShared(&g_lock);
    Emit("matrix_probe_dump frame=%ld candidates=%u contexts=%u\r\n",
         frameNumber, g_candidateCount, g_contextHookCount);
    for (UINT index = 0; index < g_candidateCount; ++index)
    {
        const Candidate& candidate = g_candidates[index];
        if (candidate.vertexSlotMask == 0 || candidate.bestScore < 5)
            continue;
        const uintptr_t writer = reinterpret_cast<uintptr_t>(candidate.writerAddress);
        const uintptr_t relative = engineBase && writer >= engineBase &&
            writer - engineBase < 0x03000000u ? writer - engineBase : writer;
        const float* m = candidate.bestMatrix;
        Emit("matrix_candidate id=%u bytes=%u vs_slots=0x%X updates=%u score=%d offset=%u writer=0x%08X m=[%.4g %.4g %.4g %.4g|%.4g %.4g %.4g %.4g|%.4g %.4g %.4g %.4g|%.4g %.4g %.4g %.4g]\r\n",
            index, candidate.byteWidth, candidate.vertexSlotMask,
            candidate.updateCount, candidate.bestScore, candidate.bestOffset,
            static_cast<unsigned>(relative),
            m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
    }
    ReleaseSRWLockShared(&g_lock);
}
}

namespace matrixprobe
{
bool ArmDevice(ID3D11Device* device)
{
    if (!device)
        return false;
    void** deviceVtable = *reinterpret_cast<void***>(device);
    void* originalDeferred = reinterpret_cast<void*>(g_originalCreateDeferredContext);
    const bool armed = ReplaceVtableSlot(
        deviceVtable, kCreateDeferredContextSlot,
        reinterpret_cast<void*>(&HookCreateDeferredContext), &originalDeferred);
    g_originalCreateDeferredContext =
        reinterpret_cast<CreateDeferredContextFn>(originalDeferred);
    return armed;
}

bool Install(ID3D11Device* device, LogLineFn logLine)
{
    if (!device)
        return false;
    g_logLine = logLine;
    g_primaryDevice = device;

    ID3D11DeviceContext* immediate = nullptr;
    device->GetImmediateContext(&immediate);
    const bool immediateOk = PatchContext(immediate);
    if (immediate)
        immediate->Release();

    void** deviceVtable = *reinterpret_cast<void***>(device);
    void* originalDeferred = reinterpret_cast<void*>(g_originalCreateDeferredContext);
    const bool deferredOk = ReplaceVtableSlot(
        deviceVtable, kCreateDeferredContextSlot,
        reinterpret_cast<void*>(&HookCreateDeferredContext), &originalDeferred);
    g_originalCreateDeferredContext =
        reinterpret_cast<CreateDeferredContextFn>(originalDeferred);
    Emit("matrix_probe=installed immediate=%s deferred_factory=%s\r\n",
         immediateOk ? "ok" : "failed", deferredOk ? "ok" : "failed");
    void** activeVtable = *reinterpret_cast<void***>(immediate);
    Emit("matrix_probe_slots current=[%p %p %p %p] hooks=[%p %p %p %p]\r\n",
         activeVtable[kVSSetConstantBuffersSlot], activeVtable[kMapSlot],
         activeVtable[kUnmapSlot], activeVtable[kUpdateSubresourceSlot],
         &HookVSSetConstantBuffers, &HookMap, &HookUnmap, &HookUpdateSubresource);
    return immediateOk && deferredOk;
}

void OnPresent(LONG frameNumber)
{
    if (frameNumber >= 120 && !g_dumped120)
    {
        g_dumped120 = true;
        DumpCandidates(frameNumber);
    }
    if (frameNumber >= 600 && !g_dumped600)
    {
        g_dumped600 = true;
        DumpCandidates(frameNumber);
    }
}

void Shutdown()
{
    g_primaryDevice = nullptr;
    g_logLine = nullptr;
}
}
