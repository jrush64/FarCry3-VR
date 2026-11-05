#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <MinHook.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "aer_camera.h"
#include "aer_state.h"

namespace
{
using MapFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT,
    D3D11_MAPPED_SUBRESOURCE*);
using UnmapFn = void (STDMETHODCALLTYPE*)(
    ID3D11DeviceContext*, ID3D11Resource*, UINT);
using UpdateSubresourceFn = void (STDMETHODCALLTYPE*)(
    ID3D11DeviceContext*, ID3D11Resource*, UINT, const D3D11_BOX*,
    const void*, UINT, UINT);
using DrawIndexedFn = void (STDMETHODCALLTYPE*)(
    ID3D11DeviceContext*, UINT, UINT, INT);
using CreateVertexShaderFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D11Device*, const void*, SIZE_T, ID3D11ClassLinkage*,
    ID3D11VertexShader**);

constexpr UINT kCameraBufferSize = 896;
constexpr UINT kViewRotProjectionOffset = 0x000;
constexpr UINT kViewProjectionOffset = 0x040;
constexpr float kHalfEyeSeparation = 0.10f;
constexpr float kWeaponSeparationScale = 0.015f;
constexpr UINT kMaximumOpenMaps = 64;
constexpr UINT kSizeCacheEntries = 2048;
constexpr UINT kSnapshotEntries = 1024;
constexpr UINT kShaderEntries = 32768;
constexpr unsigned __int64 kWeaponVsCloth = 0xD891AD7E48B5BAD9ui64;
constexpr unsigned __int64 kWeaponVsSkin = 0xE8D9D3694E695620ui64;

struct OpenMap
{
    ID3D11Resource* resource;
    void* data;
};

struct SizeCacheEntry
{
    ID3D11Resource* resource;
    UINT byteWidth;
};

struct CameraSnapshot
{
    ID3D11Resource* resource;
    unsigned char bytes[kCameraBufferSize];
    bool valid;
};

struct ShaderRecord
{
    IUnknown* shader;
    unsigned __int64 hash;
};

MapFn g_originalMap = nullptr;
UnmapFn g_originalUnmap = nullptr;
UpdateSubresourceFn g_originalUpdateSubresource = nullptr;
DrawIndexedFn g_originalDrawIndexed = nullptr;
CreateVertexShaderFn g_originalCreateVertexShader = nullptr;
aercamera::LogLineFn g_logLine = nullptr;
ID3D11Buffer* g_weaponCameraBuffer = nullptr;
OpenMap g_openMaps[kMaximumOpenMaps] = {};
SizeCacheEntry g_sizeCache[kSizeCacheEntries] = {};
CameraSnapshot g_snapshots[kSnapshotEntries] = {};
ShaderRecord g_shaders[kShaderEntries] = {};
SRWLOCK g_sizeLock = SRWLOCK_INIT;
SRWLOCK g_snapshotLock = SRWLOCK_INIT;
SRWLOCK g_shaderLock = SRWLOCK_INIT;
volatile LONG g_installed = 0;
volatile LONG g_patchCount = 0;
volatile LONG g_leftPatchCount = 0;
volatile LONG g_rightPatchCount = 0;
volatile LONG g_weaponDrawCount = 0;
volatile LONG g_shaderCount = 0;

void Emit(const char* format, ...)
{
    if (!g_logLine)
        return;
    char line[512] = {};
    va_list args;
    va_start(args, format);
    _vsnprintf_s(line, sizeof(line), _TRUNCATE, format, args);
    va_end(args);
    g_logLine(line);
}

UINT BufferWidth(ID3D11Resource* resource)
{
    if (!resource)
        return 0;
    const size_t start = (reinterpret_cast<size_t>(resource) >> 4) &
                         (kSizeCacheEntries - 1);

    AcquireSRWLockShared(&g_sizeLock);
    for (UINT probe = 0; probe < 8; ++probe)
    {
        const SizeCacheEntry& entry =
            g_sizeCache[(start + probe) & (kSizeCacheEntries - 1)];
        if (entry.resource == resource)
        {
            const UINT width = entry.byteWidth;
            ReleaseSRWLockShared(&g_sizeLock);
            return width;
        }
        if (!entry.resource)
            break;
    }
    ReleaseSRWLockShared(&g_sizeLock);

    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    resource->GetType(&dimension);
    UINT width = 0;
    if (dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
    {
        D3D11_BUFFER_DESC description = {};
        static_cast<ID3D11Buffer*>(resource)->GetDesc(&description);
        width = description.ByteWidth;
    }

    AcquireSRWLockExclusive(&g_sizeLock);
    for (UINT probe = 0; probe < 8; ++probe)
    {
        SizeCacheEntry& entry =
            g_sizeCache[(start + probe) & (kSizeCacheEntries - 1)];
        if (entry.resource == resource || !entry.resource)
        {
            entry.resource = resource;
            entry.byteWidth = width;
            break;
        }
    }
    ReleaseSRWLockExclusive(&g_sizeLock);
    return width;
}

bool IsWritableMap(D3D11_MAP mapType)
{
    return mapType == D3D11_MAP_WRITE ||
           mapType == D3D11_MAP_WRITE_DISCARD ||
           mapType == D3D11_MAP_WRITE_NO_OVERWRITE ||
           mapType == D3D11_MAP_READ_WRITE;
}

void StoreSnapshot(ID3D11Resource* resource, const void* data)
{
    if (!resource || !data)
        return;
    const size_t start = (reinterpret_cast<size_t>(resource) >> 4) &
                         (kSnapshotEntries - 1);
    AcquireSRWLockExclusive(&g_snapshotLock);
    for (UINT probe = 0; probe < 32; ++probe)
    {
        CameraSnapshot& snapshot =
            g_snapshots[(start + probe) & (kSnapshotEntries - 1)];
        if (snapshot.resource == resource || !snapshot.resource)
        {
            snapshot.resource = resource;
            std::memcpy(snapshot.bytes, data, kCameraBufferSize);
            snapshot.valid = true;
            break;
        }
    }
    ReleaseSRWLockExclusive(&g_snapshotLock);
}

bool LoadSnapshot(ID3D11Resource* resource, unsigned char* destination)
{
    if (!resource || !destination)
        return false;
    const size_t start = (reinterpret_cast<size_t>(resource) >> 4) &
                         (kSnapshotEntries - 1);
    bool found = false;
    AcquireSRWLockShared(&g_snapshotLock);
    for (UINT probe = 0; probe < 32; ++probe)
    {
        const CameraSnapshot& snapshot =
            g_snapshots[(start + probe) & (kSnapshotEntries - 1)];
        if (snapshot.resource == resource && snapshot.valid)
        {
            std::memcpy(destination, snapshot.bytes, kCameraBufferSize);
            found = true;
            break;
        }
        if (!snapshot.resource)
            break;
    }
    ReleaseSRWLockShared(&g_snapshotLock);
    return found;
}

bool ApplyEye(void* data, float separationScale, bool countPatch)
{
    if (!data)
        return false;
    auto* bytes = static_cast<unsigned char*>(data);
    float* viewRotProjection = reinterpret_cast<float*>(
        bytes + kViewRotProjectionOffset);
    float* viewProjection = reinterpret_cast<float*>(
        bytes + kViewProjectionOffset);

    const float xScale = std::sqrt(
        viewRotProjection[0] * viewRotProjection[0] +
        viewRotProjection[1] * viewRotProjection[1] +
        viewRotProjection[2] * viewRotProjection[2]);
    if (!std::isfinite(xScale) || xScale < 0.01f || xScale > 100.0f)
        return false;

    const bool left = fc3aer::CurrentEye() == fc3aer::Eye::Left;
    const float eyeOffset = (left ? -kHalfEyeSeparation : kHalfEyeSeparation) *
                            separationScale;
    const float projectedShift = eyeOffset * xScale;

    // FC3 is camera-relative. Ordinary geometry uses ViewRotProjection at 0x00;
    // instanced grass uses ViewProjection at 0x40. Full-frame AER needs no SBS
    // aspect scaling and _CameraPosition must remain untouched.
    viewRotProjection[3] -= projectedShift;
    viewProjection[3] -= projectedShift;

    if (countPatch)
    {
        InterlockedIncrement(&g_patchCount);
        if (left)
            InterlockedIncrement(&g_leftPatchCount);
        else
            InterlockedIncrement(&g_rightPatchCount);
    }
    return true;
}

void PatchCameraUpdate(ID3D11Resource* resource, void* data)
{
    StoreSnapshot(resource, data);
    ApplyEye(data, 1.0f, true);
}

unsigned __int64 HashBytecode(const void* data, SIZE_T byteCount)
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    unsigned __int64 hash = 14695981039346656037ui64;
    for (SIZE_T index = 0; index < byteCount; ++index)
    {
        hash ^= bytes[index];
        hash *= 1099511628211ui64;
    }
    return hash;
}

void RecordShader(IUnknown* shader, const void* bytecode, SIZE_T byteCount)
{
    if (!shader || !bytecode || byteCount == 0)
        return;
    AcquireSRWLockExclusive(&g_shaderLock);
    const LONG index = g_shaderCount;
    if (index >= 0 && index < static_cast<LONG>(kShaderEntries))
    {
        g_shaders[index].shader = shader;
        g_shaders[index].hash = HashBytecode(bytecode, byteCount);
        InterlockedIncrement(&g_shaderCount);
    }
    ReleaseSRWLockExclusive(&g_shaderLock);
}

bool BoundVertexShaderHash(
    ID3D11DeviceContext* context, unsigned __int64* hash)
{
    if (!context || !hash)
        return false;
    *hash = 0;
    ID3D11VertexShader* shader = nullptr;
    context->VSGetShader(&shader, nullptr, nullptr);
    if (!shader)
        return false;

    bool found = false;
    const LONG count = InterlockedCompareExchange(&g_shaderCount, 0, 0);
    AcquireSRWLockShared(&g_shaderLock);
    for (LONG index = count - 1;
         index >= 0 && index < static_cast<LONG>(kShaderEntries); --index)
    {
        if (g_shaders[index].shader == shader)
        {
            *hash = g_shaders[index].hash;
            found = true;
            break;
        }
    }
    ReleaseSRWLockShared(&g_shaderLock);
    shader->Release();
    return found;
}

bool IsWeaponDraw(
    ID3D11DeviceContext* context, UINT count, UINT startIndex, INT baseVertex)
{
    if (baseVertex != 0)
        return false;
    unsigned __int64 shaderHash = 0;
    if (!BoundVertexShaderHash(context, &shaderHash))
        return false;
    if (shaderHash == kWeaponVsCloth)
    {
        return (count == 11418 && startIndex == 27747) ||
               (count == 3687 && startIndex == 0) ||
               (count == 3174 && startIndex == 3687);
    }
    if (shaderHash == kWeaponVsSkin)
        return count == 10443 && (startIndex == 6861 || startIndex == 17304);
    return false;
}

int FindCameraSlot(
    ID3D11DeviceContext* context, unsigned char* originalCamera)
{
    ID3D11Buffer* buffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    context->VSGetConstantBuffers(
        0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, buffers);
    int slot = -1;
    for (UINT index = 0;
         index < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++index)
    {
        if (slot < 0 && buffers[index] &&
            BufferWidth(buffers[index]) == kCameraBufferSize &&
            LoadSnapshot(buffers[index], originalCamera))
        {
            slot = static_cast<int>(index);
        }
        if (buffers[index])
            buffers[index]->Release();
    }
    return slot;
}

bool UploadWeaponCamera(
    ID3D11DeviceContext* context, unsigned char* originalCamera)
{
    if (!context || !g_weaponCameraBuffer || !originalCamera)
        return false;
    ApplyEye(originalCamera, kWeaponSeparationScale, false);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const HRESULT result = g_originalMap(
        context, g_weaponCameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result) || !mapped.pData)
        return false;
    std::memcpy(mapped.pData, originalCamera, kCameraBufferSize);
    g_originalUnmap(context, g_weaponCameraBuffer, 0);
    return true;
}

HRESULT STDMETHODCALLTYPE HookMap(
    ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource,
    D3D11_MAP mapType, UINT mapFlags, D3D11_MAPPED_SUBRESOURCE* mapped)
{
    const HRESULT result = g_originalMap(
        context, resource, subresource, mapType, mapFlags, mapped);
    if (SUCCEEDED(result) && mapped && mapped->pData &&
        IsWritableMap(mapType) && BufferWidth(resource) == kCameraBufferSize)
    {
        for (UINT index = 0; index < kMaximumOpenMaps; ++index)
        {
            if (InterlockedCompareExchangePointer(
                    reinterpret_cast<PVOID volatile*>(&g_openMaps[index].resource),
                    resource, nullptr) == nullptr)
            {
                g_openMaps[index].data = mapped->pData;
                break;
            }
        }
    }
    return result;
}

void STDMETHODCALLTYPE HookUnmap(
    ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource)
{
    for (UINT index = 0; index < kMaximumOpenMaps; ++index)
    {
        if (g_openMaps[index].resource == resource)
        {
            void* data = g_openMaps[index].data;
            g_openMaps[index].data = nullptr;
            InterlockedExchangePointer(
                reinterpret_cast<PVOID volatile*>(&g_openMaps[index].resource),
                nullptr);
            PatchCameraUpdate(resource, data);
            break;
        }
    }
    g_originalUnmap(context, resource, subresource);
}

void STDMETHODCALLTYPE HookUpdateSubresource(
    ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource,
    const D3D11_BOX* destinationBox, const void* source, UINT sourceRowPitch,
    UINT sourceDepthPitch)
{
    const UINT width = source ? BufferWidth(resource) : 0;
    const bool fullUpdate = !destinationBox ||
        (destinationBox->left == 0 && destinationBox->right >= kCameraBufferSize);
    if (source && width == kCameraBufferSize && fullUpdate)
    {
        unsigned char copy[kCameraBufferSize] = {};
        std::memcpy(copy, source, kCameraBufferSize);
        PatchCameraUpdate(resource, copy);
        g_originalUpdateSubresource(
            context, resource, subresource, destinationBox, copy,
            sourceRowPitch, sourceDepthPitch);
        return;
    }
    g_originalUpdateSubresource(
        context, resource, subresource, destinationBox, source,
        sourceRowPitch, sourceDepthPitch);
}

HRESULT STDMETHODCALLTYPE HookCreateVertexShader(
    ID3D11Device* device, const void* bytecode, SIZE_T byteCount,
    ID3D11ClassLinkage* linkage, ID3D11VertexShader** shader)
{
    const HRESULT result = g_originalCreateVertexShader(
        device, bytecode, byteCount, linkage, shader);
    if (SUCCEEDED(result) && shader && *shader)
        RecordShader(*shader, bytecode, byteCount);
    return result;
}

void STDMETHODCALLTYPE HookDrawIndexed(
    ID3D11DeviceContext* context, UINT count, UINT startIndex, INT baseVertex)
{
    if (!IsWeaponDraw(context, count, startIndex, baseVertex))
    {
        g_originalDrawIndexed(context, count, startIndex, baseVertex);
        return;
    }

    unsigned char originalCamera[kCameraBufferSize] = {};
    const int slot = FindCameraSlot(context, originalCamera);
    if (slot < 0 || !UploadWeaponCamera(context, originalCamera))
    {
        g_originalDrawIndexed(context, count, startIndex, baseVertex);
        return;
    }

    ID3D11Buffer* originalBuffer = nullptr;
    context->VSGetConstantBuffers(static_cast<UINT>(slot), 1, &originalBuffer);
    context->VSSetConstantBuffers(
        static_cast<UINT>(slot), 1, &g_weaponCameraBuffer);
    g_originalDrawIndexed(context, count, startIndex, baseVertex);
    context->VSSetConstantBuffers(
        static_cast<UINT>(slot), 1, &originalBuffer);
    if (originalBuffer)
        originalBuffer->Release();

    const LONG weaponDraw = InterlockedIncrement(&g_weaponDrawCount);
    if (weaponDraw == 1)
        Emit("aer_weapon=active separation_scale=%.3f slot=b%d\r\n",
             kWeaponSeparationScale, slot);
}
}

namespace aercamera
{
bool Install(ID3D11Device* device, ID3D11DeviceContext* context, LogLineFn logLine)
{
    if (!device || !context)
        return false;
    g_logLine = logLine;
    if (InterlockedCompareExchange(&g_installed, 1, 0) != 0)
        return true;

    D3D11_BUFFER_DESC weaponDescription = {};
    weaponDescription.ByteWidth = kCameraBufferSize;
    weaponDescription.Usage = D3D11_USAGE_DYNAMIC;
    weaponDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    weaponDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    const HRESULT weaponResult = device->CreateBuffer(
        &weaponDescription, nullptr, &g_weaponCameraBuffer);
    if (FAILED(weaponResult))
    {
        Emit("aer_camera=weapon_buffer_failed hr=0x%08lX\r\n",
             static_cast<unsigned long>(weaponResult));
        return false;
    }

    const MH_STATUS initializeStatus = MH_Initialize();
    if (initializeStatus != MH_OK &&
        initializeStatus != MH_ERROR_ALREADY_INITIALIZED)
    {
        Emit("aer_camera=minhook_init_failed status=%d\r\n",
             static_cast<int>(initializeStatus));
        return false;
    }

    void** contextVtable = *reinterpret_cast<void***>(context);
    void** deviceVtable = *reinterpret_cast<void***>(device);
    struct HookRequest
    {
        void* target;
        void* detour;
        void** original;
        const char* name;
    };
    HookRequest requests[] = {
        { contextVtable[14], reinterpret_cast<void*>(&HookMap),
          reinterpret_cast<void**>(&g_originalMap), "Map" },
        { contextVtable[15], reinterpret_cast<void*>(&HookUnmap),
          reinterpret_cast<void**>(&g_originalUnmap), "Unmap" },
        { contextVtable[48], reinterpret_cast<void*>(&HookUpdateSubresource),
          reinterpret_cast<void**>(&g_originalUpdateSubresource),
          "UpdateSubresource" },
        { contextVtable[12], reinterpret_cast<void*>(&HookDrawIndexed),
          reinterpret_cast<void**>(&g_originalDrawIndexed), "DrawIndexed" },
        { deviceVtable[12], reinterpret_cast<void*>(&HookCreateVertexShader),
          reinterpret_cast<void**>(&g_originalCreateVertexShader),
          "CreateVertexShader" },
    };

    int installed = 0;
    for (const HookRequest& request : requests)
    {
        const MH_STATUS createStatus = MH_CreateHook(
            request.target, request.detour, request.original);
        const MH_STATUS enableStatus = createStatus == MH_OK
            ? MH_EnableHook(request.target) : createStatus;
        if (createStatus == MH_OK && enableStatus == MH_OK)
            ++installed;
        else
            Emit("aer_camera=hook_failed name=%s create=%d enable=%d\r\n",
                 request.name, static_cast<int>(createStatus),
                 static_cast<int>(enableStatus));
    }

    Emit("aer_camera=inline_hooks installed=%d/5 separation=%.3f weapon_scale=%.3f offsets=0x000,0x040\r\n",
         installed, kHalfEyeSeparation, kWeaponSeparationScale);
    return installed == 5;
}

void OnPresent(LONG frameNumber)
{
    if (frameNumber < 16 || (frameNumber % 600) == 0)
    {
        Emit("aer_camera_frame=%ld eye=%s patches=%ld left=%ld right=%ld weapon_draws=%ld\r\n",
             frameNumber,
             fc3aer::CurrentEye() == fc3aer::Eye::Left ? "LEFT" : "RIGHT",
             InterlockedCompareExchange(&g_patchCount, 0, 0),
             InterlockedCompareExchange(&g_leftPatchCount, 0, 0),
             InterlockedCompareExchange(&g_rightPatchCount, 0, 0),
             InterlockedCompareExchange(&g_weaponDrawCount, 0, 0));
    }
}

void Shutdown()
{
    g_logLine = nullptr;
    // Hooks and the private CB intentionally remain until process teardown.
    // Disabling executable detours from DllMain can race the render thread.
}
}