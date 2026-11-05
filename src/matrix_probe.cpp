#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <intrin.h>
#include <cmath>
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

constexpr size_t kVSSetConstantBuffersSlot = 7;
constexpr size_t kMapSlot = 14;
constexpr size_t kUnmapSlot = 15;
constexpr UINT kMaximumCandidates = 512;

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

SRWLOCK g_lock = SRWLOCK_INIT;
ID3D11DeviceContext* g_primaryContext = nullptr;
VSSetConstantBuffersFn g_originalVSSetConstantBuffers = nullptr;
MapFn g_originalMap = nullptr;
UnmapFn g_originalUnmap = nullptr;
matrixprobe::LogLineFn g_logLine = nullptr;
Candidate g_candidates[kMaximumCandidates] = {};
UINT g_candidateCount = 0;
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

bool PatchSlot(void* object, size_t index, void* hook, void** original)
{
    if (!object || !hook || !original)
        return false;
    void** vtable = *reinterpret_cast<void***>(object);
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
    const float epsilon = 0.02f;
    if (std::fabs(matrix[15]) < epsilon)
    {
        score += 2;
        if (std::fabs(std::fabs(matrix[11]) - 1.0f) < 0.15f ||
            std::fabs(std::fabs(matrix[14]) - 1.0f) < 0.15f)
            score += 4;
        if (std::fabs(matrix[0]) > 0.05f && std::fabs(matrix[5]) > 0.05f)
            score += 1;
    }

    if (std::fabs(matrix[15] - 1.0f) < epsilon)
    {
        const float row0 = Length3(matrix[0], matrix[1], matrix[2]);
        const float row1 = Length3(matrix[4], matrix[5], matrix[6]);
        const float row2 = Length3(matrix[8], matrix[9], matrix[10]);
        const float col0 = Length3(matrix[0], matrix[4], matrix[8]);
        const float col1 = Length3(matrix[1], matrix[5], matrix[9]);
        const float col2 = Length3(matrix[2], matrix[6], matrix[10]);
        const float rowError = std::fabs(row0 - 1.0f) + std::fabs(row1 - 1.0f) +
                               std::fabs(row2 - 1.0f);
        const float colError = std::fabs(col0 - 1.0f) + std::fabs(col1 - 1.0f) +
                               std::fabs(col2 - 1.0f);
        if (rowError < 0.35f || colError < 0.35f)
            score += 5;
    }
    return score;
}

void InspectMappedBuffer(Candidate* candidate)
{
    if (!candidate || !candidate->mappedData)
        return;

    const auto* bytes = static_cast<const unsigned char*>(candidate->mappedData);
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
    candidate->writerAddress = _ReturnAddress();
    candidate->mappedData = nullptr;
}

void STDMETHODCALLTYPE HookVSSetConstantBuffers(
    ID3D11DeviceContext* context,
    UINT startSlot,
    UINT bufferCount,
    ID3D11Buffer* const* buffers)
{
    if (context == g_primaryContext && buffers)
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
    g_originalVSSetConstantBuffers(context, startSlot, bufferCount, buffers);
}

HRESULT STDMETHODCALLTYPE HookMap(
    ID3D11DeviceContext* context,
    ID3D11Resource* resource,
    UINT subresource,
    D3D11_MAP mapType,
    UINT mapFlags,
    D3D11_MAPPED_SUBRESOURCE* mapped)
{
    const HRESULT result = g_originalMap(
        context, resource, subresource, mapType, mapFlags, mapped);
    if (context == g_primaryContext && SUCCEEDED(result) && resource && mapped)
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
    if (context == g_primaryContext && resource)
    {
        ID3D11Buffer* buffer = nullptr;
        if (SUCCEEDED(resource->QueryInterface(
                __uuidof(ID3D11Buffer), reinterpret_cast<void**>(&buffer))))
        {
            AcquireSRWLockExclusive(&g_lock);
            InspectMappedBuffer(FindCandidate(buffer, false));
            ReleaseSRWLockExclusive(&g_lock);
            buffer->Release();
        }
    }
    g_originalUnmap(context, resource, subresource);
}

void DumpCandidates(LONG frameNumber)
{
    HMODULE engine = GetModuleHandleW(L"FC3_d3d11.dll");
    const uintptr_t engineBase = reinterpret_cast<uintptr_t>(engine);
    AcquireSRWLockShared(&g_lock);
    Emit("matrix_probe_dump frame=%ld candidates=%u\r\n", frameNumber, g_candidateCount);
    for (UINT index = 0; index < g_candidateCount; ++index)
    {
        const Candidate& candidate = g_candidates[index];
        if (candidate.vertexSlotMask == 0 || candidate.bestScore < 5)
            continue;
        const uintptr_t writer = reinterpret_cast<uintptr_t>(candidate.writerAddress);
        const uintptr_t writerRva = engineBase && writer >= engineBase
            ? writer - engineBase : writer;
        const float* m = candidate.bestMatrix;
        Emit("matrix_candidate id=%u bytes=%u vs_slots=0x%X updates=%u score=%d offset=%u writer=0x%08X m=[%.4g %.4g %.4g %.4g|%.4g %.4g %.4g %.4g|%.4g %.4g %.4g %.4g|%.4g %.4g %.4g %.4g]\r\n",
            index, candidate.byteWidth, candidate.vertexSlotMask,
            candidate.updateCount, candidate.bestScore, candidate.bestOffset,
            static_cast<unsigned>(writerRva),
            m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
    }
    ReleaseSRWLockShared(&g_lock);
}
}

namespace matrixprobe
{
bool Install(ID3D11Device* device, LogLineFn logLine)
{
    if (!device)
        return false;
    g_logLine = logLine;

    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);
    if (!context)
        return false;
    g_primaryContext = context;

    void* originalVS = reinterpret_cast<void*>(g_originalVSSetConstantBuffers);
    void* originalMap = reinterpret_cast<void*>(g_originalMap);
    void* originalUnmap = reinterpret_cast<void*>(g_originalUnmap);
    const bool vs = PatchSlot(context, kVSSetConstantBuffersSlot,
                              reinterpret_cast<void*>(&HookVSSetConstantBuffers), &originalVS);
    const bool map = PatchSlot(context, kMapSlot,
                               reinterpret_cast<void*>(&HookMap), &originalMap);
    const bool unmap = PatchSlot(context, kUnmapSlot,
                                 reinterpret_cast<void*>(&HookUnmap), &originalUnmap);
    g_originalVSSetConstantBuffers = reinterpret_cast<VSSetConstantBuffersFn>(originalVS);
    g_originalMap = reinterpret_cast<MapFn>(originalMap);
    g_originalUnmap = reinterpret_cast<UnmapFn>(originalUnmap);
    context->Release();

    Emit("matrix_probe=context_hooks vs=%s map=%s unmap=%s\r\n",
         vs ? "ok" : "failed", map ? "ok" : "failed", unmap ? "ok" : "failed");
    return vs && map && unmap;
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
    g_primaryContext = nullptr;
    g_logLine = nullptr;
}
}
