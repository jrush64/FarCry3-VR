#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <MinHook.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <new>
#include <intrin.h>

#include "dxbc_patch.h"
#include "fc3_log.h"
#include "fov_boost.h"
#include "stereo.h"
#include "aim_probe.h"
#include "di_probe.h"
#include "vr_pad.h"
#include "xr_presenter.h"

using fc3::Log;

namespace
{
// ---- private-data keys ------------------------------------------------------
// Everything the mod knows about a D3D object is stored ON that object, so the
// knowledge dies with it. No pointer-keyed side tables that can go stale.

// {7C0E5A10-3F1B-4E0C-9B61-FC3000000001} resource -> twin resource, view -> twin view
const GUID kTwinGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 1 } };
// view: twin epoch at which its resource was last seen to have no twin
const GUID kMonoEpochGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 2 } };
// vertex shader: 1 when the eye shift was patched in
const GUID kVsPatchedGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 3 } };
// pixel shader: bit mask of the texture slots it declares
const GUID kPsSlotsGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 4 } };
// pixel shader: 1 when the depth-unprojection correction was patched in
const GUID kPsPatchedGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 9 } };
// 896-byte camera buffer marker, and its currently mapped pointer
const GUID kCameraGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 5 } };
const GUID kMapPtrGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 6 } };
// 896-byte camera buffer: what was last written to it (CameraSummary)
const GUID kCameraSummaryGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 10 } };
// swapchain backbuffer marker
const GUID kBackbufferGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 7 } };
// on vertex buffers: Jason's first-person body mesh (legs, torso, arms)
const GUID kBodyVbGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 12 } };
// the vertex shader the two first-person arm meshes are drawn with
constexpr unsigned long long kArmVsHash = 0xE8D9D3694E695620ull;
// on constant buffers the game maps: a CPU copy of what it last wrote
// ($Globals 352 bytes: first three rows; bone palettes 2304 bytes: first 23 bones)
const GUID kCbCopyGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 13 } };
constexpr UINT kGlobalsBytes = 352, kBonesBytes = 2304;
constexpr UINT kSmallCbMax = 1024;      // $Globals of any shader: kept whole
// The arm palettes the mod hands the GPU instead of the game's ([0] right, [1] left).
ID3D11Buffer* g_armPaletteCb[2] = {};
ID3D11Buffer* g_weaponPaletteCb = nullptr;     // skinned weapon pieces, moved with the right hand
LONG g_armPaletteDraws = 0, g_weaponPaletteDraws = 0;

// One arm as the game posed it this frame, in its first-person camera's view
// space (x right, y up, z forward, metres): shoulder end of the mesh, elbow, wrist.
struct ArmJoints { double s[3], e[3], w[3]; bool valid; };
ArmJoints g_armJoints[2] = {};     // [0] right, [1] left
LONG g_armIkFrames = 0, g_armIkRejected = 0;

// vertex / pixel shader: FNV-1a 64 of the ORIGINAL bytecode (matches the offline corpus)
const GUID kShaderHashGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 8 } };
// on input layouts: their element list as text, for capture dumps
const GUID kLayoutTextGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 11 } };

constexpr UINT kCameraBufferSize = 896;
const GUID kEjectionBytecodeGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 27 } };
constexpr UINT kPsSlots = 16;

// ---- original functions -----------------------------------------------------
using DrawIndexedFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, INT);
using DrawFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT);
using DrawIndexedInstancedFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, UINT, INT, UINT);
using DrawInstancedFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, UINT, UINT);
using MapFn = HRESULT (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT, D3D11_MAPPED_SUBRESOURCE*);
using UnmapFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT);
using UpdateSubresourceFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, const D3D11_BOX*, const void*, UINT, UINT);
using CopySubresourceRegionFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, UINT, UINT, UINT, ID3D11Resource*, UINT, const D3D11_BOX*);
using CopyResourceFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, ID3D11Resource*);
using ClearRtvFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11RenderTargetView*, const FLOAT[4]);
using ClearDsvFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11DepthStencilView*, UINT, FLOAT, UINT8);
using GenerateMipsFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11ShaderResourceView*);
using ResolveFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, ID3D11Resource*, UINT, DXGI_FORMAT);
using DispatchFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, UINT);
using CreateBufferFn = HRESULT (STDMETHODCALLTYPE*)(ID3D11Device*, const D3D11_BUFFER_DESC*, const D3D11_SUBRESOURCE_DATA*, ID3D11Buffer**);
using CreateVertexShaderFn = HRESULT (STDMETHODCALLTYPE*)(ID3D11Device*, const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11VertexShader**);
using CreatePixelShaderFn = HRESULT (STDMETHODCALLTYPE*)(ID3D11Device*, const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11PixelShader**);

DrawIndexedFn g_oDrawIndexed = nullptr;
DrawFn g_oDraw = nullptr;
DrawIndexedInstancedFn g_oDrawIndexedInstanced = nullptr;
DrawInstancedFn g_oDrawInstanced = nullptr;
MapFn g_oMap = nullptr;
UnmapFn g_oUnmap = nullptr;
UpdateSubresourceFn g_oUpdateSubresource = nullptr;
CopySubresourceRegionFn g_oCopySubresourceRegion = nullptr;
CopyResourceFn g_oCopyResource = nullptr;
ClearRtvFn g_oClearRtv = nullptr;
ClearDsvFn g_oClearDsv = nullptr;
GenerateMipsFn g_oGenerateMips = nullptr;
ResolveFn g_oResolve = nullptr;
DispatchFn g_oDispatch = nullptr;
CreateBufferFn g_oCreateBuffer = nullptr;
using CreateInputLayoutFn = HRESULT (STDMETHODCALLTYPE*)(ID3D11Device*, const D3D11_INPUT_ELEMENT_DESC*, UINT,
                                                         const void*, SIZE_T, ID3D11InputLayout**);
CreateInputLayoutFn g_oCreateInputLayout = nullptr;
int g_weaponDrawIndex = 0;      // weapon draws dumped in the current capture
LONG g_weaponDrawsThisFrame = 0;  // first-person model draws seen this frame
LONG g_visibleWeaponDrawsThisFrame = 0;
LONG g_visibleWeaponDrawsLastFrame = 0;
LONG g_weaponDrawsLastFrame = 0;
CreateVertexShaderFn g_oCreateVertexShader = nullptr;
CreatePixelShaderFn g_oCreatePixelShader = nullptr;

// ---- state ------------------------------------------------------------------
volatile LONG g_hooksInstalled = 0;
ID3D11Device* g_dev = nullptr;              // the game's device (not owned)
ID3D11DeviceContext* g_ctx = nullptr;       // its immediate context (not owned)
bool g_own = false;                         // true while the mod issues its own calls
UINT g_epoch = 1;                           // bumps whenever a twin resource is created
UINT g_bbW = 0, g_bbH = 0;

// settings (fc3_stereo.ini, read once; hotkeys change them live)
bool g_enabled = true;
int g_view = 1;                 // 0 = left eye only on the monitor, 1 = side by side
float g_separation = 0.032f;    // half the eye distance, world units (1 unit ~ 1 m)
float g_uiDepth = 2.0f;         // distance the flat UI is placed at, world units
float g_weaponNear = 0.0f;      // vertices closer than this use the weapon scale; 0 = off
float g_weaponScale = 1.0f;
float g_fpsCap = 60.0f;         // 0 = uncapped
// Physical reload settings are independent of the old transfer gate.
#include "manual_reload_settings.inl"
float g_worldScale = 1.0f;      // headset only: multiplies the headset's eye distance

float g_mainXScale = 1.3f;      // horizontal projection scale of the main camera
float g_mainRight[3] = { 1.0f, 0.0f, 0.0f };  // main camera's right axis in world space
// [0 world, 1 first-person body, 2 ui, 3 left arm, 4 right arm + gun][0 left eye, 1 right eye]
// 5 = Jason's own body pieces (legs, torso): they stay with the player's body,
// not with the camera, while the camera follows the gun
constexpr int kEyeVariants = 6;
bool g_aimActive = false;
double g_aimQ[3][3] = {};            // body axes in camera axes (identity when not aiming)
ID3D11Buffer* g_eyeCb[kEyeVariants][2] = {};
UINT g_armStarts[2] = { 0xffffffffu, 0xffffffffu };   // index starts of the two arm meshes
ID3D11Buffer* g_eyePsCb[2] = {};    // pixel stage: eye clip space back to game clip space

// The game's projection: x' = xs*x, y' = ys*y, z' = a*z + b, w' = z.
struct GameProjection { float xs, ys, a, b; };
GameProjection g_worldProjection = { 1.3f, 2.31f, 1.0f, -0.1f };
GameProjection g_weaponProjection = { 1.57f, 2.79f, 1.0f, -0.1f };
bool g_weaponProjectionValid = false;
fc3stereo::EyeView g_eyeViews[2] = {};
bool g_eyeViewsSet = false;
fc3stereo::EyeView g_eyeViewsBody[2] = {};    // as the headset reported them (body frame)
float g_handBody[2][16] = {};                 // [0] right, [1] left, body frame
// Motion controllers: the first-person gun is drawn at the right controller.
float g_handRight[16] = {};
float g_handLeft[16] = {};
bool g_handRightValid = false, g_handLeftValid = false;
bool g_motionGun = true;       // first-person arms follow the controllers
bool g_weaponMotionGuard = true;
// Render observation stays active while overrides are paused, allowing the
// game to bring the weapon out normally before motion interaction resumes.
bool MacheteMotionLease();
bool MotionInteractionEnabled()
{ return g_motionGun && (!g_weaponMotionGuard || g_visibleWeaponDrawsLastFrame > 0); }
// Where the game holds each hand, as a pose in its first-person camera's space
// ([0] right, [1] left). Defaults until the player calibrates (both stick clicks).
float g_handRef[2][16] = {
    { 1, 0, 0, 0.12f, 0, 1, 0, -0.15f, 0, 0, 1, 0.30f, 0, 0, 0, 1 },
    { 1, 0, 0, -0.10f, 0, 1, 0, -0.15f, 0, 0, 1, 0.45f, 0, 0, 0, 1 } };
// Where the game holds the gun, in its weapon camera's space (metres): this
// point is put on the controller.
float g_gunOffset[3] = { 0.12f, -0.15f, 0.30f };
float g_uiWidthDeg = 60.0f;         // headset: how wide the flat UI panel appears

// What a camera buffer currently holds. Stored on the buffer itself and
// carried along when the game copies one camera buffer into another, so the
// buffer bound at a draw always describes the camera that draw really uses.
struct CameraSummary
{
    float right[3], up[3], forward[3];
    float xScale, yScale, zScale, zOffset;
    float position[3];  // camera position the shaders subtract (with its fractions)
    UINT mainView;      // perspective and shaped like the presented image
};
CameraSummary g_frameCamera = {};
bool g_frameCameraValid = false;
int g_frameCameraTries = 0;
bool g_headTracking = true;
int g_pacePeriods = 0;

// camera histogram for the current frame
struct CameraBin { float xScale; LONG count; float right[3]; GameProjection projection; };
CameraBin g_cameraBins[8] = {};

// SBS monitor view
ID3D11VertexShader* g_blitVs = nullptr;
ID3D11PixelShader* g_blitPs = nullptr;
ID3D11SamplerState* g_blitSampler = nullptr;
ID3D11RasterizerState* g_blitRaster = nullptr;
ID3D11Texture2D* g_leftCopy = nullptr;
ID3D11ShaderResourceView* g_leftCopySrv = nullptr;
UINT g_leftCopyW = 0, g_leftCopyH = 0;
DXGI_FORMAT g_leftCopyFormat = DXGI_FORMAT_UNKNOWN;

// counters
struct Stats
{
    LONG monoDraws, stereoDraws, initiated, failedDraws;
    LONG twins, twinViews, clears, copies, dispatches, stereoDispatches;
    LONG vsPatched, vsUnpatched, psPatched, cameraWrites;
    LONGLONG twinBytes;
    LONGLONG classifyTicks;
};
Stats g_stats = {};
LONG g_frame = 0;
LONG g_firstStereoFrame = -1;
bool g_dumpRequested = false;
bool g_inGameplay = false;
int g_autoDumps = 0;
LONG g_gameplayFrames = 0;      // consecutive frames that look like the 3D world
LONG g_lastStereoDraws = 0;

// ---- small helpers ----------------------------------------------------------
template <class T>
T* GetTwin(ID3D11DeviceChild* object)
{
    IUnknown* twin = nullptr;
    UINT size = sizeof(twin);
    if (SUCCEEDED(object->GetPrivateData(kTwinGuid, &size, &twin)) && twin)
        return static_cast<T*>(twin);   // already AddRef'd by GetPrivateData
    return nullptr;
}

bool HasFlag(ID3D11DeviceChild* object, const GUID& guid)
{
    UINT value = 0;
    UINT size = sizeof(value);
    return SUCCEEDED(object->GetPrivateData(guid, &size, &value)) && size == sizeof(value) && value != 0;
}

void SetFlag(ID3D11DeviceChild* object, const GUID& guid, UINT value)
{
    object->SetPrivateData(guid, sizeof(value), &value);
}

UINT BytesPerPixel(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS: case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return 16;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM: case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G8X24_TYPELESS: case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return 8;
    case DXGI_FORMAT_R8_UNORM: case DXGI_FORMAT_A8_UNORM: case DXGI_FORMAT_R8_TYPELESS:
        return 1;
    case DXGI_FORMAT_R16_FLOAT: case DXGI_FORMAT_R16_UNORM: case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R16_TYPELESS: case DXGI_FORMAT_D16_UNORM:
        return 2;
    default:
        return 4;
    }
}

// Returns the twin of a texture, creating it when asked. Result is AddRef'd.
ID3D11Resource* TwinResource(ID3D11Resource* resource, bool create)
{
    if (!resource)
        return nullptr;
    if (ID3D11Resource* existing = GetTwin<ID3D11Resource>(resource))
        return existing;
    if (!create || !g_dev || !g_ctx)
        return nullptr;

    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    resource->GetType(&dimension);
    if (dimension != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        return nullptr;

    ID3D11Texture2D* texture = static_cast<ID3D11Texture2D*>(resource);
    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);
    if (desc.Usage != D3D11_USAGE_DEFAULT)
        return nullptr;
    if (HasFlag(resource, kBackbufferGuid))
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;   // the mod samples the right eye
    desc.MiscFlags &= ~static_cast<UINT>(D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX);

    ID3D11Texture2D* twin = nullptr;
    const HRESULT result = g_dev->CreateTexture2D(&desc, nullptr, &twin);
    if (FAILED(result) || !twin)
    {
        Log("[TWIN] create failed hr=0x%08lX %ux%u fmt=%u bind=0x%X",
            static_cast<unsigned long>(result), desc.Width, desc.Height,
            static_cast<unsigned>(desc.Format), desc.BindFlags);
        return nullptr;
    }
    // Start the right eye from whatever the shared image already holds.
    g_oCopyResource(g_ctx, twin, resource);
    resource->SetPrivateDataInterface(kTwinGuid, twin);
    ++g_epoch;

    const LONG count = InterlockedIncrement(&g_stats.twins);
    const LONGLONG bytes = static_cast<LONGLONG>(desc.Width) * desc.Height *
        BytesPerPixel(desc.Format) * desc.SampleDesc.Count * desc.ArraySize;
    g_stats.twinBytes += bytes;
    if (count <= 600)
        Log("[TWIN] #%ld %ux%u fmt=%u samples=%u mips=%u bind=0x%X (%.1f MB, total %.1f MB)",
            count, desc.Width, desc.Height, static_cast<unsigned>(desc.Format),
            desc.SampleDesc.Count, desc.MipLevels, desc.BindFlags,
            bytes / 1048576.0, g_stats.twinBytes / 1048576.0);
    return twin;
}

enum class ViewKind { RenderTarget, DepthStencil, ShaderResource, UnorderedAccess };

// Returns the matching view on the twin resource (AddRef'd), or null when the
// resource has no twin. With createResource the twin resource is made on demand.
ID3D11View* TwinView(ID3D11View* view, ViewKind kind, bool createResource)
{
    if (!view)
        return nullptr;
    if (ID3D11View* existing = GetTwin<ID3D11View>(view))
        return existing;

    if (!createResource)
    {
        UINT epoch = 0;
        UINT size = sizeof(epoch);
        if (SUCCEEDED(view->GetPrivateData(kMonoEpochGuid, &size, &epoch)) && epoch == g_epoch)
            return nullptr;     // checked since the last twin was created: still single
    }

    ID3D11Resource* resource = nullptr;
    view->GetResource(&resource);
    if (!resource)
        return nullptr;
    ID3D11Resource* twinResource = TwinResource(resource, createResource);
    resource->Release();
    if (!twinResource)
    {
        if (!createResource)
            view->SetPrivateData(kMonoEpochGuid, sizeof(g_epoch), &g_epoch);
        return nullptr;
    }

    ID3D11View* twinView = nullptr;
    HRESULT result = E_FAIL;
    if (kind == ViewKind::RenderTarget)
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc = {};
        static_cast<ID3D11RenderTargetView*>(view)->GetDesc(&desc);
        ID3D11RenderTargetView* created = nullptr;
        result = g_dev->CreateRenderTargetView(twinResource, &desc, &created);
        twinView = created;
    }
    else if (kind == ViewKind::DepthStencil)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
        static_cast<ID3D11DepthStencilView*>(view)->GetDesc(&desc);
        ID3D11DepthStencilView* created = nullptr;
        result = g_dev->CreateDepthStencilView(twinResource, &desc, &created);
        twinView = created;
    }
    else if (kind == ViewKind::UnorderedAccess)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
        static_cast<ID3D11UnorderedAccessView*>(view)->GetDesc(&desc);
        ID3D11UnorderedAccessView* created = nullptr;
        result = g_dev->CreateUnorderedAccessView(twinResource, &desc, &created);
        twinView = created;
    }
    else
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        static_cast<ID3D11ShaderResourceView*>(view)->GetDesc(&desc);
        ID3D11ShaderResourceView* created = nullptr;
        result = g_dev->CreateShaderResourceView(twinResource, &desc, &created);
        twinView = created;
    }
    twinResource->Release();
    if (FAILED(result) || !twinView)
    {
        Log("[TWIN] view create failed kind=%d hr=0x%08lX", static_cast<int>(kind),
            static_cast<unsigned long>(result));
        return nullptr;
    }
    view->SetPrivateDataInterface(kTwinGuid, twinView);
    InterlockedIncrement(&g_stats.twinViews);
    return twinView;
}

template <class T>
void SafeRelease(T*& pointer)
{
    if (pointer)
    {
        pointer->Release();
        pointer = nullptr;
    }
}

// ---- one-frame census -------------------------------------------------------
// Records every draw, clear and copy of a single frame, in order, with what it
// was bound to and how the mod treated it. Consecutive identical lines collapse.
HANDLE g_census = INVALID_HANDLE_VALUE;
int g_censusArmed = 0;          // >0: start a census at the next Present
char g_censusLast[512] = {};
LONG g_censusRepeat = 0;

void CensusFlush()
{
    if (g_census == INVALID_HANDLE_VALUE || !g_censusLast[0]) return;
    char line[600];
    const int n = _snprintf_s(line, sizeof(line), _TRUNCATE, "%4ldx %s\r\n", g_censusRepeat, g_censusLast);
    DWORD written = 0;
    if (n > 0) WriteFile(g_census, line, static_cast<DWORD>(n), &written, nullptr);
    g_censusLast[0] = 0;
    g_censusRepeat = 0;
}

void CensusLine(const char* text)
{
    if (g_census == INVALID_HANDLE_VALUE) return;
    if (std::strcmp(text, g_censusLast) == 0)
    {
        ++g_censusRepeat;
        return;
    }
    CensusFlush();
    strncpy_s(g_censusLast, text, _TRUNCATE);
    g_censusRepeat = 1;
}

// Appends "3840x2160/f44/ms*" for the texture behind a view ('*' = has a twin).
void DescribeView(ID3D11View* view, char* text, size_t size)
{
    const size_t used = std::strlen(text);
    if (used + 48 >= size) return;
    char* out = text + used;
    const size_t room = size - used;
    if (!view) { _snprintf_s(out, room, _TRUNCATE, "-"); return; }
    ID3D11Resource* resource = nullptr;
    view->GetResource(&resource);
    if (!resource) { _snprintf_s(out, room, _TRUNCATE, "?"); return; }
    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    resource->GetType(&dimension);
    ID3D11Resource* twin = GetTwin<ID3D11Resource>(resource);
    if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        static_cast<ID3D11Texture2D*>(resource)->GetDesc(&desc);
        _snprintf_s(out, room, _TRUNCATE, "%ux%u/f%u%s%s%s%s", desc.Width, desc.Height,
                    static_cast<unsigned>(desc.Format), desc.SampleDesc.Count > 1 ? "/ms" : "",
                    desc.ArraySize > 1 ? "/arr" : "",
                    HasFlag(resource, kBackbufferGuid) ? "/BB" : "", twin ? "*" : "");
    }
    else
    {
        _snprintf_s(out, room, _TRUNCATE, "dim%d%s", static_cast<int>(dimension), twin ? "*" : "");
    }
    SafeRelease(twin);
    resource->Release();
}

void Append(char* text, size_t size, const char* piece)
{
    strncat_s(text, size, piece, _TRUNCATE);
}

unsigned long long ShaderHash(ID3D11DeviceChild* shader)
{
    unsigned long long hash = 0;
    UINT size = sizeof(hash);
    if (shader) shader->GetPrivateData(kShaderHashGuid, &size, &hash);
    return hash;
}

unsigned long long Fnv64(const void* data, size_t size)
{
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    unsigned long long hash = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i)
    {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

// ---- draw duplication -------------------------------------------------------
struct DrawCall
{
    int kind;       // 0 DrawIndexed, 1 Draw, 2 DrawIndexedInstanced, 3 DrawInstanced
    UINT a, b, c, e;
    INT d;
};

void Issue(ID3D11DeviceContext* context, const DrawCall& call)
{
    switch (call.kind)
    {
    case 0: g_oDrawIndexed(context, call.a, call.b, call.d); break;
    case 1: g_oDraw(context, call.a, call.b); break;
    case 2: g_oDrawIndexedInstanced(context, call.a, call.b, call.c, call.d, call.e); break;
    default: g_oDrawInstanced(context, call.a, call.b, call.c, call.e); break;
    }
}

// Reads the CPU copy of what the game last wrote into a constant buffer.
// Returns its size in bytes (0 = none, or larger than the destination).
UINT ReadCopy(ID3D11Buffer* buffer, void* destination, UINT capacity)
{
    if (!buffer) return 0;
    UINT size = capacity;
    return SUCCEEDED(buffer->GetPrivateData(kCbCopyGuid, &size, destination)) ? size : 0;
}

// Where the game posed one arm this frame: the bones' own matrices applied to
// the joint points of the arm mesh (bind space, measured from the captured
// mesh), then the draw's world matrix and camera. Mesh, from the shoulder
// end: upper-arm stub (bones 22-24), elbow (0, with 1), forearm (21, 20), hand
// (2, 13 and the fingers).
void ObserveShotHand(const float*,const float*,const CameraSummary&,const ArmJoints&);
int MeasureArm(ID3D11DeviceContext* context)
{
    ID3D11Buffer* buffers[4] = {};
    context->VSGetConstantBuffers(0, 4, buffers);
    float globals[kSmallCbMax / 4] = {}, bones[48 * 12] = {};
    CameraSummary camera = {};
    bool ok = ReadCopy(buffers[0], globals, sizeof(globals)) >= 48;
    UINT size = sizeof(camera);
    ok = ok && buffers[1] && SUCCEEDED(buffers[1]->GetPrivateData(kCameraSummaryGuid, &size, &camera)) &&
         size == sizeof(camera) && camera.mainView;
    ok = ok && ReadCopy(buffers[3], bones, sizeof(bones)) == sizeof(bones);
    for (ID3D11Buffer*& b : buffers) SafeRelease(b);
    if (!ok)
    {
        if (InterlockedIncrement(&g_armIkRejected) <= 3) Log("[ARMS] could not read an arm's pose (buffers not captured)");
        return -1;
    }
    // a bind-space point through one bone, to first-person view space
    auto toView = [&](const double p[3], int bone, double out[3]) {
        const float* m = bones + 12 * bone;
        double model[3], world[3];
        for (int r = 0; r < 3; ++r)
            model[r] = m[r * 4] * p[0] + m[r * 4 + 1] * p[1] + m[r * 4 + 2] * p[2] + m[r * 4 + 3];
        for (int r = 0; r < 3; ++r)
            world[r] = globals[r * 4] * model[0] + globals[r * 4 + 1] * model[1] + globals[r * 4 + 2] * model[2] +
                       (static_cast<double>(globals[r * 4 + 3]) - camera.position[r]);
        for (int r = 0; r < 3; ++r)
        {
            const float* axis = r == 0 ? camera.right : r == 1 ? camera.up : camera.forward;
            out[r] = axis[0] * world[0] + axis[1] * world[1] + axis[2] * world[2];
        }
    };
    auto gap = [&](const double p[3], int a, int b) {
        double u[3], v[3];
        toView(p, a, u);
        toView(p, b, v);
        return (u[0] - v[0]) * (u[0] - v[0]) + (u[1] - v[1]) * (u[1] - v[1]) + (u[2] - v[2]) * (u[2] - v[2]);
    };
    // Which arm is it? The left arm is the right one mirrored in x; only the
    // right guess makes the bones that meet at the elbow and the wrist agree.
    double score[2] = {};
    for (int side = 0; side < 2; ++side)
    {
        const double mirror = side == 1 ? -1.0 : 1.0;
        const double elbow[3] = { 0.376 * mirror, -0.032, 1.297 };      // bones 0 and 1 meet
        const double wrist[3] = { 0.591 * mirror, 0.011, 1.149 };       // bones 20 and 2 meet
        score[side] = gap(elbow, 0, 1) + gap(wrist, 20, 2);
    }
    const int side = score[0] <= score[1] ? 0 : 1;
    const double mirror = side == 1 ? -1.0 : 1.0;
    ArmJoints joints = {};
    const double top[3] = { 0.219 * mirror, -0.077, 1.368 }, elbow[3] = { 0.385 * mirror, -0.043, 1.293 },
                 wrist[3] = { 0.591 * mirror, 0.011, 1.149 };
    toView(top, 22, joints.s);
    toView(elbow, 0, joints.e);
    toView(wrist, 2, joints.w);
    double reach = 0, forearm = 0;
    for (int r = 0; r < 3; ++r)
    {
        reach += joints.w[r] * joints.w[r];
        forearm += (joints.w[r] - joints.e[r]) * (joints.w[r] - joints.e[r]);
    }
    const double best = score[side] < score[1 - side] ? score[side] : score[1 - side];
    joints.valid = best < 0.05 * 0.05 && reach < 1.5 * 1.5 && forearm > 0.12 * 0.12 && forearm < 0.6 * 0.6;
    if (!joints.valid)
    {
        if (InterlockedIncrement(&g_armIkRejected) <= 5)
            Log("[ARMS] arm pose not readable (joint gap %.3f m, forearm %.3f m) - this arm mesh stays as the game drew it",
                std::sqrt(best), std::sqrt(forearm));
        return -1;
    }
    g_armJoints[side] = joints;
    if(side==0)ObserveShotHand(bones,globals,camera,joints);
    return side;
}

// Copies a constant buffer back to the CPU (capture frames only).
bool ReadBytes(ID3D11DeviceContext* context, ID3D11Buffer* buffer, std::vector<unsigned char>& out)
{
    D3D11_BUFFER_DESC desc = {};
    buffer->GetDesc(&desc);
    if (desc.ByteWidth == 0 || desc.ByteWidth > 96u * 1024u * 1024u) return false;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;
    ID3D11Device* device = nullptr;
    context->GetDevice(&device);
    ID3D11Buffer* staging = nullptr;
    const bool created = device && SUCCEEDED(device->CreateBuffer(&desc, nullptr, &staging));
    SafeRelease(device);
    if (!created) return false;
    const bool own = g_own;
    g_own = true;
    g_oCopyResource(context, staging, buffer);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const bool ok = SUCCEEDED(g_oMap(context, staging, 0, D3D11_MAP_READ, 0, &mapped));
    if (ok)
    {
        out.assign(static_cast<const unsigned char*>(mapped.pData),
                   static_cast<const unsigned char*>(mapped.pData) + desc.ByteWidth);
        g_oUnmap(context, staging, 0);
    }
    g_own = own;
    staging->Release();
    return ok;
}

void WriteBlob(const wchar_t* name, const std::vector<unsigned char>& bytes)
{
    wchar_t path[MAX_PATH] = {};
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%s%s", fc3::DataFolder(), name);
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
    CloseHandle(file);
}

bool ReadBuffer(ID3D11DeviceContext* context, ID3D11Buffer* buffer, std::vector<float>& out)
{
    D3D11_BUFFER_DESC desc = {};
    buffer->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;
    ID3D11Device* device = nullptr;
    context->GetDevice(&device);
    ID3D11Buffer* staging = nullptr;
    const bool created = device && SUCCEEDED(device->CreateBuffer(&desc, nullptr, &staging));
    SafeRelease(device);
    if (!created) return false;
    const bool own = g_own;
    g_own = true;
    g_oCopyResource(context, staging, buffer);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    bool ok = SUCCEEDED(g_oMap(context, staging, 0, D3D11_MAP_READ, 0, &mapped));
    if (ok)
    {
        out.assign(static_cast<const float*>(mapped.pData),
                   static_cast<const float*>(mapped.pData) + desc.ByteWidth / 4);
        g_oUnmap(context, staging, 0);
    }
    g_own = own;
    staging->Release();
    return ok;
}

// Capture frame: what the first-person (weapon camera) draws carry, so the arms
// and hands can be found in the bone palettes.
void DumpWeaponDraw(ID3D11DeviceContext* context, const DrawCall& call)
{
    ID3D11VertexShader* vs = nullptr;
    context->VSGetShader(&vs, nullptr, nullptr);
    char text[512];
    _snprintf_s(text, sizeof(text), _TRUNCATE, "WEAPON draw kind %d count %u instances %u vs %016llX",
                call.kind, call.a, call.kind == 2 || call.kind == 3 ? call.b : 1u, ShaderHash(vs));
    SafeRelease(vs);
    CensusLine(text);

    // The mesh itself: layout, vertex streams and indices (binary files).
    const int index = ++g_weaponDrawIndex;
    _snprintf_s(text, sizeof(text), _TRUNCATE, "  draw %d: a %u b %u c %u e %u base %d", index, call.a, call.b,
                call.c, call.e, call.d);
    CensusLine(text);
    ID3D11InputLayout* layout = nullptr;
    context->IAGetInputLayout(&layout);
    if (layout)
    {
        char layoutText[400] = {};
        UINT size = sizeof(layoutText) - 1;
        if (SUCCEEDED(layout->GetPrivateData(kLayoutTextGuid, &size, layoutText)))
        {
            _snprintf_s(text, sizeof(text), _TRUNCATE, "  layout %s", layoutText);
            CensusLine(text);
        }
        layout->Release();
    }
    ID3D11Buffer* streams[4] = {};
    UINT strides[4] = {}, offsets[4] = {};
    context->IAGetVertexBuffers(0, 4, streams, strides, offsets);
    for (int slot = 0; slot < 4; ++slot)
    {
        if (!streams[slot]) continue;
        std::vector<unsigned char> bytes;
        const bool read = ReadBytes(context, streams[slot], bytes);
        _snprintf_s(text, sizeof(text), _TRUNCATE, "  vb%d stride %u offset %u bytes %zu%s", slot, strides[slot],
                    offsets[slot], bytes.size(), read ? "" : " (not readable)");
        CensusLine(text);
        if (read)
        {
            wchar_t name[64];
            _snwprintf_s(name, 64, _TRUNCATE, L"fc3_weapon_%d_vb%d.bin", index, slot);
            WriteBlob(name, bytes);
        }
        streams[slot]->Release();
    }
    ID3D11Buffer* indices = nullptr;
    DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN;
    UINT indexOffset = 0;
    context->IAGetIndexBuffer(&indices, &indexFormat, &indexOffset);
    if (indices)
    {
        std::vector<unsigned char> bytes;
        const bool read = ReadBytes(context, indices, bytes);
        _snprintf_s(text, sizeof(text), _TRUNCATE, "  ib format %d offset %u bytes %zu%s",
                    static_cast<int>(indexFormat), indexOffset, bytes.size(), read ? "" : " (not readable)");
        CensusLine(text);
        if (read)
        {
            wchar_t name[64];
            _snwprintf_s(name, 64, _TRUNCATE, L"fc3_weapon_%d_ib.bin", index);
            WriteBlob(name, bytes);
        }
        indices->Release();
    }
    ID3D11Buffer* buffers[6] = {};
    context->VSGetConstantBuffers(0, 6, buffers);
    for (int slot = 0; slot < 6; ++slot)
    {
        if (!buffers[slot]) continue;
        D3D11_BUFFER_DESC desc = {};
        buffers[slot]->GetDesc(&desc);
        std::vector<float> data;
        const bool bones = desc.ByteWidth == 48 * 48;
        if (desc.ByteWidth <= 4096 && ReadBuffer(context, buffers[slot], data))
        {
            const size_t rows = bones ? 144 : data.size() / 4;
            for (size_t r = 0; r + (bones ? 2 : 0) < rows && (r + 1) * 4 <= data.size(); r += bones ? 3 : 1)
            {
                if (bones)
                    _snprintf_s(text, sizeof(text), _TRUNCATE,
                                "  cb%d bone %2zu  %9.4f %9.4f %9.4f %9.4f | %9.4f %9.4f %9.4f %9.4f | %9.4f %9.4f %9.4f %9.4f",
                                slot, r / 3, data[r * 4], data[r * 4 + 1], data[r * 4 + 2], data[r * 4 + 3],
                                data[r * 4 + 4], data[r * 4 + 5], data[r * 4 + 6], data[r * 4 + 7],
                                data[r * 4 + 8], data[r * 4 + 9], data[r * 4 + 10], data[r * 4 + 11]);
                else
                    _snprintf_s(text, sizeof(text), _TRUNCATE, "  cb%d row %zu  %9.4f %9.4f %9.4f %9.4f", slot, r,
                                data[r * 4], data[r * 4 + 1], data[r * 4 + 2], data[r * 4 + 3]);
                CensusLine(text);
            }
        }
        else
        {
            _snprintf_s(text, sizeof(text), _TRUNCATE, "  cb%d %u bytes", slot, desc.ByteWidth);
            CensusLine(text);
        }
        buffers[slot]->Release();
    }
}

ID3D11Buffer* BuildArmPalette(ID3D11DeviceContext* context, int side);
void AimActualCamera(const CameraSummary& camera);
void UpdateEyeBuffers();
ID3D11Buffer* BuildWeaponPalette(ID3D11DeviceContext* context, UINT start, UINT count);
ID3D11Buffer* BuildWeaponWorld(ID3D11DeviceContext* context, UINT start, UINT count);
ID3D11Buffer* BuildBodyPalette(ID3D11DeviceContext* context);

HRESULT STDMETHODCALLTYPE HookCreateInputLayout(ID3D11Device* device, const D3D11_INPUT_ELEMENT_DESC* elements,
                                                UINT count, const void* bytecode, SIZE_T length,
                                                ID3D11InputLayout** layout)
{
    const HRESULT result = g_oCreateInputLayout(device, elements, count, bytecode, length, layout);
    if (SUCCEEDED(result) && layout && *layout && elements)
    {
        char text[400] = {};
        for (UINT i = 0; i < count; ++i)
        {
            char piece[64];
            _snprintf_s(piece, sizeof(piece), _TRUNCATE, "%s%u:f%d:s%u:o%u ",
                        elements[i].SemanticName ? elements[i].SemanticName : "?", elements[i].SemanticIndex,
                        static_cast<int>(elements[i].Format), elements[i].InputSlot,
                        elements[i].AlignedByteOffset);
            strncat_s(text, piece, _TRUNCATE);
        }
        (*layout)->SetPrivateData(kLayoutTextGuid, static_cast<UINT>(std::strlen(text) + 1), text);
    }
    return result;
}

void RecordActionDraw(ID3D11DeviceContext* context, const DrawCall& call);
void RecordAnimationDraw(ID3D11DeviceContext*,const DrawCall&);
void RecordAnimationFinal(const float*,int);
void FinishAnimationDraw();
void TickActionProbe(bool gameplay);
void TickEjectionProbe(bool gameplay);
void TickPhysicalHull(bool gameplay);
bool PhysicalHullDraw(ID3D11DeviceContext* context, const DrawCall& call);
bool ReplayPhysicalHull(ID3D11DeviceContext* context);
void RecordEjectionDraw(ID3D11DeviceContext* context, const DrawCall& call, bool firstPerson);

bool MacheteHeld();
void CaptureMachete(ID3D11DeviceContext*,const DrawCall&);
void ReplayMachete(ID3D11DeviceContext*);
void LoadMacheteSettings(const wchar_t*);
#include "weapon_presence.h"
fc3weapon::Geometry NativeWeaponGeometry(ID3D11DeviceContext* context,const DrawCall& call)
{
    fc3weapon::Geometry geometry={};
    if(call.kind!=0||call.d!=0)return geometry;
    ID3D11Buffer* vb=nullptr,*ib=nullptr;UINT indexOffset=0;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
    context->IAGetVertexBuffers(0,1,&vb,&geometry.stride,&geometry.offset);
    context->IAGetIndexBuffer(&ib,&format,&indexOffset);
    D3D11_BUFFER_DESC vd={},id={};if(vb)vb->GetDesc(&vd);if(ib)ib->GetDesc(&id);
    SafeRelease(vb);SafeRelease(ib);
    if(indexOffset)return {};
    geometry.vertexBytes=vd.ByteWidth;geometry.indexBytes=id.ByteWidth;
    geometry.start=call.b;geometry.count=call.a;geometry.format=static_cast<UINT>(format);
    return geometry;
}
void HandleDraw(ID3D11DeviceContext* context, const DrawCall& call)
{
    if (context != g_ctx || !g_enabled || g_own)
    {
        Issue(context, call);
        return;
    }

    ReplayPhysicalHull(context);
    if (PhysicalHullDraw(context, call))
    {
        RecordEjectionDraw(context, call, false);
        return;
    }

    LARGE_INTEGER started;
    QueryPerformanceCounter(&started);

    ID3D11RenderTargetView* rtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11RenderTargetView* twinRtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView* dsv = nullptr;
    ID3D11DepthStencilView* twinDsv = nullptr;
    ID3D11ShaderResourceView* srv[kPsSlots] = {};
    ID3D11ShaderResourceView* twinSrv[kPsSlots] = {};
    UINT rtCount = 0;
    UINT slotMask = 0;
    bool stereo = false;
    bool failed = false;
    bool inputStereo = false;
    bool pixelPatched = false;
    bool weaponHidden = false;
    char reason = '-';

    context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, &dsv);
    for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        if (rtv[i]) rtCount = i + 1;

    if (rtCount != 0 || dsv)
    {
        bool targetStereo = false;
        for (UINT i = 0; i < rtCount; ++i)
        {
            if (!rtv[i]) continue;
            twinRtv[i] = static_cast<ID3D11RenderTargetView*>(
                TwinView(rtv[i], ViewKind::RenderTarget, false));
            if (twinRtv[i]) targetStereo = true;
        }
        if (dsv)
        {
            twinDsv = static_cast<ID3D11DepthStencilView*>(
                TwinView(dsv, ViewKind::DepthStencil, false));
            if (twinDsv) targetStereo = true;
        }

        // Only the texture slots the bound pixel shader declares count as
        // inputs. Stale bindings left over from an earlier pass do not.
        ID3D11PixelShader* pixelShader = nullptr;
        context->PSGetShader(&pixelShader, nullptr, nullptr);
        if (pixelShader)
        {
            UINT size = sizeof(slotMask);
            if (FAILED(pixelShader->GetPrivateData(kPsSlotsGuid, &size, &slotMask)))
                slotMask = 0xffff;
            pixelPatched = HasFlag(pixelShader, kPsPatchedGuid);
            pixelShader->Release();
        }
        if (slotMask)
        {
            context->PSGetShaderResources(0, kPsSlots, srv);
            for (UINT s = 0; s < kPsSlots; ++s)
            {
                if (!srv[s] || !(slotMask & (1u << s))) continue;
                twinSrv[s] = static_cast<ID3D11ShaderResourceView*>(
                    TwinView(srv[s], ViewKind::ShaderResource, false));
                if (twinSrv[s]) inputStereo = true;
            }
        }

        bool initiate = false;
        if (!targetStereo && !inputStereo && dsv && g_bbW != 0)
        {
            // Depth-tested geometry drawn at the size of the presented image
            // is the main view. This is where stereo starts each frame.
            D3D11_VIEWPORT viewport = {};
            UINT viewportCount = 1;
            context->RSGetViewports(&viewportCount, &viewport);
            initiate = viewportCount >= 1 &&
                static_cast<UINT>(viewport.Width + 0.5f) == g_bbW &&
                static_cast<UINT>(viewport.Height + 0.5f) == g_bbH;
            if (initiate) InterlockedIncrement(&g_stats.initiated);
        }

        reason = targetStereo ? 'T' : inputStereo ? 'I' : initiate ? 'S' : '-';
        if (targetStereo || inputStereo || initiate)
        {
            stereo = true;
            for (UINT i = 0; i < rtCount; ++i)
            {
                if (!rtv[i] || twinRtv[i]) continue;
                twinRtv[i] = static_cast<ID3D11RenderTargetView*>(
                    TwinView(rtv[i], ViewKind::RenderTarget, true));
                if (!twinRtv[i]) failed = true;
            }
            if (dsv && !twinDsv)
            {
                twinDsv = static_cast<ID3D11DepthStencilView*>(
                    TwinView(dsv, ViewKind::DepthStencil, true));
                if (!twinDsv) failed = true;
            }
        }
    }

    LARGE_INTEGER classified;
    QueryPerformanceCounter(&classified);
    g_stats.classifyTicks += classified.QuadPart - started.QuadPart;

    if (g_census != INVALID_HANDLE_VALUE)
    {
        char text[512] = {};
        _snprintf_s(text, sizeof(text), _TRUNCATE, "%s %c rt[", failed ? "FAIL  " : stereo ? "DOUBLE" : "single", reason);
        for (UINT i = 0; i < rtCount; ++i)
        {
            if (i) Append(text, sizeof(text), " ");
            DescribeView(rtv[i], text, sizeof(text));
        }
        Append(text, sizeof(text), "] ds[");
        DescribeView(dsv, text, sizeof(text));
        D3D11_VIEWPORT viewport = {};
        UINT viewportCount = 1;
        context->RSGetViewports(&viewportCount, &viewport);
        ID3D11VertexShader* vs = nullptr;
        ID3D11PixelShader* ps = nullptr;
        context->VSGetShader(&vs, nullptr, nullptr);
        context->PSGetShader(&ps, nullptr, nullptr);
        char piece[96];
        _snprintf_s(piece, sizeof(piece), _TRUNCATE, "] vp %.0fx%.0f vs %016llX ps %016llX in[",
                    viewport.Width, viewport.Height, ShaderHash(vs), ShaderHash(ps));
        Append(text, sizeof(text), piece);
        SafeRelease(vs);
        SafeRelease(ps);
        for (UINT slot = 0; slot < kPsSlots; ++slot)
        {
            if (!srv[slot] || !(slotMask & (1u << slot))) continue;
            _snprintf_s(piece, sizeof(piece), _TRUNCATE, "t%u=", slot);
            Append(text, sizeof(text), piece);
            DescribeView(srv[slot], text, sizeof(text));
            Append(text, sizeof(text), " ");
        }
        Append(text, sizeof(text), "]");
        CensusLine(text);
    }

    if (!stereo || failed)
    {
        if (failed) InterlockedIncrement(&g_stats.failedDraws);
        else InterlockedIncrement(&g_stats.monoDraws);
        Issue(context, call);
    }
    else
    {
        InterlockedIncrement(&g_stats.stereoDraws);
        if (g_firstStereoFrame < 0) g_firstStereoFrame = g_frame;

        bool patched = false;
        ID3D11VertexShader* vertexShader = nullptr;
        context->VSGetShader(&vertexShader, nullptr, nullptr);
        if (vertexShader)
        {
            patched = HasFlag(vertexShader, kVsPatchedGuid);
            vertexShader->Release();
        }

        // Which camera is this draw really using? Read it off the camera
        // buffer bound to it: the world camera, or the first-person weapon's
        // own narrower one. Each needs its own conversion to the eye.
        int variant = 0;
        bool nativeModel=false,nativeWeapon=false;
        if (patched)
        {
            ID3D11Buffer* buffers[6] = {};
            context->VSGetConstantBuffers(0, 6, buffers);
            bool found = false;
            for (ID3D11Buffer*& buffer : buffers)
            {
                if (!buffer) continue;
                CameraSummary summary = {};
                UINT size = sizeof(summary);
                if (!found && SUCCEEDED(buffer->GetPrivateData(kCameraSummaryGuid, &size, &summary)) &&
                    size == sizeof(summary) && summary.mainView)
                {
                    found = true;
                    const bool differentProjection=std::fabs(summary.xScale-g_mainXScale)>=g_mainXScale*.01f;
                    if(differentProjection)
                    {
                        const auto geometry=NativeWeaponGeometry(context,call);
                        nativeWeapon=fc3weapon::Present(geometry);
                        ID3D11VertexShader* modelShader=nullptr;context->VSGetShader(&modelShader,nullptr,nullptr);
                        nativeModel=fc3weapon::Model(geometry)||ShaderHash(modelShader)==kArmVsHash;
                        SafeRelease(modelShader);
                    }
                    // Projection changes cannot turn arbitrary scenery into a
                    // first-person weapon. Model identity chooses that camera.
                    if (!differentProjection || !nativeModel)
                    {
                        const bool changed = !g_frameCameraValid ||
                            std::memcmp(&g_frameCamera, &summary, sizeof(summary)) != 0;
                        g_frameCamera = summary;
                        g_frameCameraValid = true;
                        g_worldProjection = { summary.xScale, summary.yScale, summary.zScale, summary.zOffset };
                        if (changed && summary.xScale > 0 && summary.yScale > 0 &&
                            std::isfinite(summary.zOffset) && std::fabs(summary.zOffset) > 1e-6f)
                        {
                            // Refresh from THIS draw, including while aim is paused.
                            // Earlier uploads/predictions cannot own these pixels.
                            if (g_aimActive) AimActualCamera(summary);
                            else UpdateEyeBuffers();
                        }
                    }
                    else
                    {
                        variant = 1;
                        const GameProjection projection = { summary.xScale, summary.yScale, summary.zScale, summary.zOffset };
                        const bool changed = !g_weaponProjectionValid ||
                            std::memcmp(&g_weaponProjection, &projection, sizeof(projection)) != 0;
                        g_weaponProjection = projection;
                        g_weaponProjectionValid = true;
                        if (changed && summary.yScale > 0 && std::fabs(summary.zOffset) > 1e-6f)
                            UpdateEyeBuffers();
                    }
                }
                buffer->Release();
            }
        }

        RecordEjectionDraw(context, call, variant == 1); // original draw, including world-space hull/effects
        ID3D11Buffer* armPalette = nullptr;
        ID3D11Buffer* worldOverride = nullptr;
        if (variant == 1)
        {
            ++g_weaponDrawsThisFrame;
            RecordAnimationDraw(context, call);
            RecordActionDraw(context, call); // original game palettes, before motion overrides
            // Which part of Jason is this? The two arm meshes go to their own
            // controllers; the rest of the body mesh stays with the body; any
            // other first-person draw (the gun) goes to the right hand.
            ID3D11VertexShader* shader = nullptr;
            context->VSGetShader(&shader, nullptr, nullptr);
            const unsigned long long hash = ShaderHash(shader);
            SafeRelease(shader);
            ID3D11Buffer* stream = nullptr;
            UINT stride = 0, offset = 0;
            context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
            if (hash == kArmVsHash)
            {
                if (stream) SetFlag(stream, kBodyVbGuid, 1);
                const UINT start = call.b;
                if (start != g_armStarts[0] && start != g_armStarts[1])
                {
                    if (g_armStarts[0] == 0xffffffffu) g_armStarts[0] = start;
                    else g_armStarts[1] = start;
                    Log("[ARMS] arm mesh found (index start %u)", start);
                }
                const int side = MeasureArm(context);
                variant = MotionInteractionEnabled() ? (side == 1 ? 3 : 4) : 1;
                if (side >= 0)
                {
                    armPalette = BuildArmPalette(context, side);
                    if (armPalette) variant = 1;    // the bones carry the arm; the camera is the plain one
                }
                else
                {
                    variant = 1;                    // unknown arm mesh: leave it as the game posed it
                }
            }
            else
            {
                // Body pieces: the body mesh's own head/torso, legs and feet
                // (index starts seen in captures). Anything else drawn with the
                // first-person camera is the weapon: it goes with the right hand.
                const bool bodyPiece = stream && HasFlag(stream, kBodyVbGuid) &&
                                       (call.b == 0 || call.b == 3687 || call.b == 27747);
                if (bodyPiece)
                {
                    variant = 5;
                    armPalette = BuildBodyPalette(context);
                }
                if (!bodyPiece)
                {
                    if(nativeWeapon)++g_visibleWeaponDrawsThisFrame;
                    variant = MotionInteractionEnabled() ? 4 : 1;
                    // a skinned weapon piece: move its bones exactly like the hand's;
                    // a solid one: move its world matrix the same way
                    CaptureMachete(context,call);
                    weaponHidden=MotionInteractionEnabled()&&MacheteHeld();
                    if(weaponHidden)ReplayMachete(context);
                    else
                    {
                        armPalette = BuildWeaponPalette(context, call.b, call.a);
                        if (armPalette) variant = 1;
                        else
                        {
                            worldOverride = BuildWeaponWorld(context, call.b, call.a);
                            if (worldOverride) variant = 1;
                        }
                    }
                }
                static unsigned long long seen[64] = {};
                static int seenCount = 0;
                D3D11_BUFFER_DESC vbDesc = {};
                if (stream) stream->GetDesc(&vbDesc);
                const unsigned long long key = hash ^ (static_cast<unsigned long long>(call.b) << 32) ^ vbDesc.ByteWidth;
                bool known = false;
                for (int k = 0; k < seenCount; ++k) known = known || seen[k] == key;
                if (!known && seenCount < 64)
                {
                    seen[seenCount++] = key;
                    Log("[ARMS] first-person piece: vs %016llX start %u count %u vertex buffer %u bytes%s -> %s", hash,
                        call.b, call.a, vbDesc.ByteWidth, stream && HasFlag(stream, kBodyVbGuid) ? " (body mesh)" : "",
                        bodyPiece ? "body" : "right hand (weapon)");
                }
            }
            SafeRelease(stream);
            FinishAnimationDraw();
        }
        if ((variant == 1 || variant >= 3) && g_census != INVALID_HANDLE_VALUE)
            DumpWeaponDraw(context, call);

        // Flat UI drawn straight onto the presented image gets its own depth.
        if (variant == 0 && !inputStereo && rtv[0])
        {
            ID3D11Resource* target = nullptr;
            rtv[0]->GetResource(&target);
            if (target)
            {
                if (HasFlag(target, kBackbufferGuid)) variant = 2;
                target->Release();
            }
        }
        const int ui = variant;

        g_own = true;
        ID3D11Buffer* gameBones = nullptr;
        ID3D11Buffer* gameGlobals = nullptr;
        if (armPalette)
        {
            context->VSGetConstantBuffers(3, 1, &gameBones);
            context->VSSetConstantBuffers(3, 1, &armPalette);
        }
        if (worldOverride)
        {
            context->VSGetConstantBuffers(0, 1, &gameGlobals);
            context->VSSetConstantBuffers(0, 1, &worldOverride);
        }
        // left eye: the game's own targets
        if (patched)
            context->VSSetConstantBuffers(dxbc::kEyeCbSlot, 1, &g_eyeCb[ui][0]);
        if (pixelPatched)
            context->PSSetConstantBuffers(dxbc::kEyeCbSlot, 1, &g_eyePsCb[0]);
        if(!weaponHidden)Issue(context, call);

        // right eye: the twins
        for (UINT s = 0; s < kPsSlots; ++s)
            if (twinSrv[s]) context->PSSetShaderResources(s, 1, &twinSrv[s]);
        context->OMSetRenderTargets(rtCount, twinRtv, twinDsv);
        if (patched)
            context->VSSetConstantBuffers(dxbc::kEyeCbSlot, 1, &g_eyeCb[ui][1]);
        if (pixelPatched)
            context->PSSetConstantBuffers(dxbc::kEyeCbSlot, 1, &g_eyePsCb[1]);
        if(!weaponHidden)Issue(context, call);

        // put the game's bindings back
        if (armPalette)
        {
            context->VSSetConstantBuffers(3, 1, &gameBones);
            SafeRelease(gameBones);
        }
        if (worldOverride)
        {
            context->VSSetConstantBuffers(0, 1, &gameGlobals);
            SafeRelease(gameGlobals);
        }
        context->OMSetRenderTargets(rtCount, rtv, dsv);
        for (UINT s = 0; s < kPsSlots; ++s)
            if (twinSrv[s]) context->PSSetShaderResources(s, 1, &srv[s]);
        ID3D11Buffer* none = nullptr;
        if (patched)
            context->VSSetConstantBuffers(dxbc::kEyeCbSlot, 1, &none);
        if (pixelPatched)
            context->PSSetConstantBuffers(dxbc::kEyeCbSlot, 1, &none);
        g_own = false;
    }

    for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        SafeRelease(rtv[i]);
        SafeRelease(twinRtv[i]);
    }
    SafeRelease(dsv);
    SafeRelease(twinDsv);
    for (UINT s = 0; s < kPsSlots; ++s)
    {
        SafeRelease(srv[s]);
        SafeRelease(twinSrv[s]);
    }
}

void STDMETHODCALLTYPE HookDrawIndexed(ID3D11DeviceContext* context, UINT count, UINT start, INT base)
{
    const DrawCall call = { 0, count, start, 0, 0, base };
    HandleDraw(context, call);
}

void STDMETHODCALLTYPE HookDraw(ID3D11DeviceContext* context, UINT count, UINT start)
{
    const DrawCall call = { 1, count, start, 0, 0, 0 };
    HandleDraw(context, call);
}

void STDMETHODCALLTYPE HookDrawIndexedInstanced(
    ID3D11DeviceContext* context, UINT indexCount, UINT instanceCount, UINT startIndex,
    INT baseVertex, UINT startInstance)
{
    const DrawCall call = { 2, indexCount, instanceCount, startIndex, startInstance, baseVertex };
    HandleDraw(context, call);
}

void STDMETHODCALLTYPE HookDrawInstanced(
    ID3D11DeviceContext* context, UINT vertexCount, UINT instanceCount, UINT startVertex,
    UINT startInstance)
{
    const DrawCall call = { 3, vertexCount, instanceCount, startVertex, startInstance, 0 };
    HandleDraw(context, call);
}

// ---- operations that must reach the right eye too ---------------------------
bool Active(ID3D11DeviceContext* context)
{
    return context == g_ctx && g_enabled && !g_own;
}

void STDMETHODCALLTYPE HookClearRtv(ID3D11DeviceContext* context, ID3D11RenderTargetView* view, const FLOAT color[4])
{
    g_oClearRtv(context, view, color);
    if (!Active(context) || !view) return;
    if (g_census != INVALID_HANDLE_VALUE)
    {
        char text[160] = "clear  color ";
        DescribeView(view, text, sizeof(text));
        CensusLine(text);
    }
    ID3D11RenderTargetView* twin = static_cast<ID3D11RenderTargetView*>(
        TwinView(view, ViewKind::RenderTarget, false));
    if (twin)
    {
        g_oClearRtv(context, twin, color);
        twin->Release();
        InterlockedIncrement(&g_stats.clears);
    }
}

void STDMETHODCALLTYPE HookClearDsv(ID3D11DeviceContext* context, ID3D11DepthStencilView* view, UINT flags, FLOAT depth, UINT8 stencil)
{
    g_oClearDsv(context, view, flags, depth, stencil);
    if (!Active(context) || !view) return;
    if (g_census != INVALID_HANDLE_VALUE)
    {
        char text[160] = "clear  depth ";
        DescribeView(view, text, sizeof(text));
        CensusLine(text);
    }
    ID3D11DepthStencilView* twin = static_cast<ID3D11DepthStencilView*>(
        TwinView(view, ViewKind::DepthStencil, false));
    if (twin)
    {
        g_oClearDsv(context, twin, flags, depth, stencil);
        twin->Release();
        InterlockedIncrement(&g_stats.clears);
    }
}

// For a copy from src to dst: returns the right-eye source and destination.
// A stereo source makes the destination stereo. Returns false when the copy
// needs no right-eye counterpart.
bool TwinCopyPair(ID3D11Resource* dst, ID3D11Resource* src, ID3D11Resource** twinDst, ID3D11Resource** twinSrc)
{
    *twinDst = nullptr;
    *twinSrc = nullptr;
    if (!dst || !src) return false;
    if (g_census != INVALID_HANDLE_VALUE)
    {
        D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        dst->GetType(&dimension);
        if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
            D3D11_TEXTURE2D_DESC a = {}, b = {};
            static_cast<ID3D11Texture2D*>(dst)->GetDesc(&a);
            src->GetType(&dimension);
            if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
                static_cast<ID3D11Texture2D*>(src)->GetDesc(&b);
            ID3D11Resource* st = GetTwin<ID3D11Resource>(src);
            ID3D11Resource* dt = GetTwin<ID3D11Resource>(dst);
            char text[200];
            _snprintf_s(text, sizeof(text), _TRUNCATE, "copy   %ux%u/f%u%s%s -> %ux%u/f%u%s usage%u%s",
                        b.Width, b.Height, static_cast<unsigned>(b.Format), b.SampleDesc.Count > 1 ? "/ms" : "", st ? "*" : "",
                        a.Width, a.Height, static_cast<unsigned>(a.Format), a.SampleDesc.Count > 1 ? "/ms" : "",
                        static_cast<unsigned>(a.Usage), dt ? "*" : "");
            SafeRelease(st);
            SafeRelease(dt);
            CensusLine(text);
        }
    }
    ID3D11Resource* sourceTwin = TwinResource(src, false);
    ID3D11Resource* destinationTwin = TwinResource(dst, sourceTwin != nullptr);
    if (!destinationTwin)
    {
        SafeRelease(sourceTwin);
        return false;       // destination stays single (staging readbacks, buffers)
    }
    if (!sourceTwin)
    {
        sourceTwin = src;   // single source feeds both eyes
        sourceTwin->AddRef();
    }
    *twinDst = destinationTwin;
    *twinSrc = sourceTwin;
    return true;
}

// The game fills the camera buffer its draws bind by copying from another
// camera buffer. The description of the contents travels with the copy.
void CarryCameraSummary(ID3D11Resource* dst, ID3D11Resource* src)
{
    if (!dst || !src || !HasFlag(dst, kCameraGuid)) return;
    CameraSummary summary = {};
    UINT size = sizeof(summary);
    if (SUCCEEDED(src->GetPrivateData(kCameraSummaryGuid, &size, &summary)) && size == sizeof(summary))
        dst->SetPrivateData(kCameraSummaryGuid, sizeof(summary), &summary);
}

void STDMETHODCALLTYPE HookCopyResource(ID3D11DeviceContext* context, ID3D11Resource* dst, ID3D11Resource* src)
{
    g_oCopyResource(context, dst, src);
    if (context == g_ctx && !g_own) CarryCameraSummary(dst, src);
    if (!Active(context)) return;
    ID3D11Resource* twinDst = nullptr;
    ID3D11Resource* twinSrc = nullptr;
    if (TwinCopyPair(dst, src, &twinDst, &twinSrc))
    {
        g_oCopyResource(context, twinDst, twinSrc);
        twinDst->Release();
        twinSrc->Release();
        InterlockedIncrement(&g_stats.copies);
    }
}

void STDMETHODCALLTYPE HookCopySubresourceRegion(
    ID3D11DeviceContext* context, ID3D11Resource* dst, UINT dstSubresource, UINT x, UINT y, UINT z,
    ID3D11Resource* src, UINT srcSubresource, const D3D11_BOX* box)
{
    g_oCopySubresourceRegion(context, dst, dstSubresource, x, y, z, src, srcSubresource, box);
    if (context == g_ctx && !g_own && x == 0 && (!box || (box->left == 0 && box->right >= kCameraBufferSize)))
        CarryCameraSummary(dst, src);
    if (!Active(context)) return;
    ID3D11Resource* twinDst = nullptr;
    ID3D11Resource* twinSrc = nullptr;
    if (TwinCopyPair(dst, src, &twinDst, &twinSrc))
    {
        g_oCopySubresourceRegion(context, twinDst, dstSubresource, x, y, z, twinSrc, srcSubresource, box);
        twinDst->Release();
        twinSrc->Release();
        InterlockedIncrement(&g_stats.copies);
    }
}

void STDMETHODCALLTYPE HookResolve(
    ID3D11DeviceContext* context, ID3D11Resource* dst, UINT dstSubresource, ID3D11Resource* src,
    UINT srcSubresource, DXGI_FORMAT format)
{
    g_oResolve(context, dst, dstSubresource, src, srcSubresource, format);
    if (!Active(context)) return;
    ID3D11Resource* twinDst = nullptr;
    ID3D11Resource* twinSrc = nullptr;
    if (TwinCopyPair(dst, src, &twinDst, &twinSrc))
    {
        g_oResolve(context, twinDst, dstSubresource, twinSrc, srcSubresource, format);
        twinDst->Release();
        twinSrc->Release();
        InterlockedIncrement(&g_stats.copies);
    }
}

void STDMETHODCALLTYPE HookGenerateMips(ID3D11DeviceContext* context, ID3D11ShaderResourceView* view)
{
    g_oGenerateMips(context, view);
    if (!Active(context) || !view) return;
    ID3D11ShaderResourceView* twin = static_cast<ID3D11ShaderResourceView*>(
        TwinView(view, ViewKind::ShaderResource, false));
    if (twin)
    {
        g_oGenerateMips(context, twin);
        twin->Release();
    }
}

// Compute passes follow the same rule as draws: if a pass reads or writes
// anything that differs between the eyes, it runs once per eye.
void STDMETHODCALLTYPE HookDispatch(ID3D11DeviceContext* context, UINT x, UINT y, UINT z)
{
    if (!Active(context))
    {
        g_oDispatch(context, x, y, z);
        return;
    }
    InterlockedIncrement(&g_stats.dispatches);

    constexpr UINT kUavSlots = 8;
    ID3D11ShaderResourceView* srv[kPsSlots] = {};
    ID3D11ShaderResourceView* twinSrv[kPsSlots] = {};
    ID3D11UnorderedAccessView* uav[kUavSlots] = {};
    ID3D11UnorderedAccessView* twinUav[kUavSlots] = {};
    bool stereo = false;
    context->CSGetShaderResources(0, kPsSlots, srv);
    context->CSGetUnorderedAccessViews(0, kUavSlots, uav);
    for (UINT s = 0; s < kPsSlots; ++s)
    {
        if (!srv[s]) continue;
        twinSrv[s] = static_cast<ID3D11ShaderResourceView*>(TwinView(srv[s], ViewKind::ShaderResource, false));
        if (twinSrv[s]) stereo = true;
    }
    for (UINT u = 0; u < kUavSlots; ++u)
    {
        if (!uav[u]) continue;
        twinUav[u] = static_cast<ID3D11UnorderedAccessView*>(TwinView(uav[u], ViewKind::UnorderedAccess, false));
        if (twinUav[u]) stereo = true;
    }

    g_oDispatch(context, x, y, z);
    if (stereo)
    {
        // Outputs that can be duplicated become per-eye. Outputs that cannot
        // (buffers) stay shared and end up holding the right eye's result.
        ID3D11UnorderedAccessView* second[kUavSlots] = {};
        for (UINT u = 0; u < kUavSlots; ++u)
        {
            if (uav[u] && !twinUav[u])
                twinUav[u] = static_cast<ID3D11UnorderedAccessView*>(TwinView(uav[u], ViewKind::UnorderedAccess, true));
            second[u] = twinUav[u] ? twinUav[u] : uav[u];
        }
        const UINT keep[kUavSlots] = { ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u };
        g_own = true;
        for (UINT s = 0; s < kPsSlots; ++s)
            if (twinSrv[s]) context->CSSetShaderResources(s, 1, &twinSrv[s]);
        context->CSSetUnorderedAccessViews(0, kUavSlots, second, keep);
        g_oDispatch(context, x, y, z);
        context->CSSetUnorderedAccessViews(0, kUavSlots, uav, keep);
        for (UINT s = 0; s < kPsSlots; ++s)
            if (twinSrv[s]) context->CSSetShaderResources(s, 1, &srv[s]);
        g_own = false;
        InterlockedIncrement(&g_stats.stereoDispatches);
    }
    for (UINT s = 0; s < kPsSlots; ++s) { SafeRelease(srv[s]); SafeRelease(twinSrv[s]); }
    for (UINT u = 0; u < kUavSlots; ++u) { SafeRelease(uav[u]); SafeRelease(twinUav[u]); }
}

// ---- camera observation -----------------------------------------------------
// Only one number is needed - Hinny that GOAT - from the game's camera: the horizontal projection
// scale of the main view, which converts an eye offset in world units into a
// clip-space shift.
bool IsMainView(const float* m, float& xScale, float& yScale)
{
    xScale = std::sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
    yScale = std::sqrt(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]);
    const float depthRow = std::sqrt(m[12] * m[12] + m[13] * m[13] + m[14] * m[14]);
    if (!std::isfinite(xScale) || !std::isfinite(yScale) || !std::isfinite(depthRow))
        return false;
    if (depthRow < 0.9f || depthRow > 1.1f)     // orthographic (shadow cascades)
        return false;
    if (xScale < 0.2f || xScale > 20.0f || g_bbH == 0)
        return false;
    const float aspect = yScale / xScale;
    const float expected = static_cast<float>(g_bbW) / static_cast<float>(g_bbH);
    return std::fabs(aspect - expected) <= expected * 0.03f;    // not cube faces, spot shadows, probes
}

void ObserveCamera(ID3D11DeviceChild* buffer, const void* data)
{
    float m[16];
    std::memcpy(m, data, sizeof(m));
    float xScale = 0, yScale = 0;
    const bool mainView = IsMainView(m, xScale, yScale);
    if (buffer)
    {
        CameraSummary summary = {};
        summary.mainView = mainView ? 1u : 0u;
        if (mainView)
        {
            summary.xScale = xScale;
            summary.yScale = yScale;
            summary.zScale = m[8] * m[12] + m[9] * m[13] + m[10] * m[14];
            summary.zOffset = m[11] - summary.zScale * m[15];
            const float* rows = static_cast<const float*>(data);
            for (int i = 0; i < 3; ++i)
            {
                summary.right[i] = m[i] / xScale;
                summary.up[i] = m[4 + i] / yScale;
                summary.forward[i] = m[12 + i];
                summary.position[i] = rows[38 * 4 + i] + rows[27 * 4 + i];
            }
        }
        buffer->SetPrivateData(kCameraSummaryGuid, sizeof(summary), &summary);
    }
    if (!mainView)
        return;

    InterlockedIncrement(&g_stats.cameraWrites);
    for (CameraBin& bin : g_cameraBins)
    {
        if (bin.count == 0)
        {
            bin.xScale = xScale;
            bin.projection = { xScale, yScale, m[8]*m[12]+m[9]*m[13]+m[10]*m[14],
                m[11] - (m[8]*m[12]+m[9]*m[13]+m[10]*m[14])*m[15] };
            bin.count = 1;
            bin.right[0] = m[0] / xScale;
            bin.right[1] = m[1] / xScale;
            bin.right[2] = m[2] / xScale;
            return;
        }
        if (std::fabs(bin.xScale - xScale) < 0.002f)
        {
            ++bin.count;
            return;
        }
    }
}

HRESULT STDMETHODCALLTYPE HookMap(
    ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource, D3D11_MAP type,
    UINT flags, D3D11_MAPPED_SUBRESOURCE* mapped)
{
    const HRESULT result = g_oMap(context, resource, subresource, type, flags, mapped);
    if (context == g_ctx && !g_own && SUCCEEDED(result) && resource && mapped && mapped->pData &&
        type != D3D11_MAP_READ)
    {
        bool keep = HasFlag(resource, kCameraGuid);
        if (!keep)
        {
            D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
            resource->GetType(&dimension);
            if (dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
            {
                D3D11_BUFFER_DESC desc = {};
                static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);
                keep = (desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) &&
                       (desc.ByteWidth <= kSmallCbMax || desc.ByteWidth == kBonesBytes);
            }
        }
        if (keep)
        {
            void* pointer = mapped->pData;
            resource->SetPrivateData(kMapPtrGuid, sizeof(pointer), &pointer);
        }
    }
    return result;
}

void STDMETHODCALLTYPE HookUnmap(ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource)
{
    if (context == g_ctx && !g_own && resource)
    {
        void* pointer = nullptr;
        UINT size = sizeof(pointer);
        if (SUCCEEDED(resource->GetPrivateData(kMapPtrGuid, &size, &pointer)) && pointer)
        {
            if (HasFlag(resource, kCameraGuid))
            {
                ObserveCamera(resource, pointer);
            }
            else
            {
                D3D11_BUFFER_DESC desc = {};
                static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);
                resource->SetPrivateData(kCbCopyGuid, desc.ByteWidth, pointer);
            }
            resource->SetPrivateData(kMapPtrGuid, 0, nullptr);
        }
    }
    g_oUnmap(context, resource, subresource);
}

void STDMETHODCALLTYPE HookUpdateSubresource(
    ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource, const D3D11_BOX* box,
    const void* source, UINT rowPitch, UINT depthPitch)
{
    if (context == g_ctx && !g_own && resource && source && !box && HasFlag(resource, kCameraGuid))
        ObserveCamera(resource, source);
    g_oUpdateSubresource(context, resource, subresource, box, source, rowPitch, depthPitch);
}

HRESULT STDMETHODCALLTYPE HookCreateBuffer(
    ID3D11Device* device, const D3D11_BUFFER_DESC* desc, const D3D11_SUBRESOURCE_DATA* initial,
    ID3D11Buffer** buffer)
{
    const HRESULT result = g_oCreateBuffer(device, desc, initial, buffer);
    if (SUCCEEDED(result) && desc && buffer && *buffer && desc->ByteWidth == kCameraBufferSize)
    {
        SetFlag(*buffer, kCameraGuid, 1);
        if (initial && initial->pSysMem && device == g_dev)
            ObserveCamera(*buffer, initial->pSysMem);
    }
    return result;
}

// ---- shader creation --------------------------------------------------------
HRESULT STDMETHODCALLTYPE HookCreateVertexShader(
    ID3D11Device* device, const void* bytecode, SIZE_T size, ID3D11ClassLinkage* linkage,
    ID3D11VertexShader** shader)
{
    if (bytecode && size && shader && !g_own)
    {
        std::vector<uint8_t> patched;
        const bool arm = Fnv64(bytecode, size) == kArmVsHash;
        if (arm) Log("[ARMS] first-person arm shader patched for elbows");
        if ((arm ? dxbc::PatchArmVertexShader(bytecode, size, patched)
                 : dxbc::PatchVertexShader(bytecode, size, patched)) == dxbc::Result::Patched)
        {
            const HRESULT result = g_oCreateVertexShader(
                device, patched.data(), patched.size(), linkage, shader);
            if (SUCCEEDED(result) && *shader)
            {
                const unsigned long long hash = Fnv64(bytecode, size);
                (*shader)->SetPrivateData(kShaderHashGuid, sizeof(hash), &hash);
                if (size <= 65536) (*shader)->SetPrivateData(kEjectionBytecodeGuid, static_cast<UINT>(size), bytecode);
                SetFlag(*shader, kVsPatchedGuid, 1);
                InterlockedIncrement(&g_stats.vsPatched);
                return result;
            }
        }
        const LONG count = InterlockedIncrement(&g_stats.vsUnpatched);
        if (count <= 8)
            Log("[VS] shader left unpatched (size %u)", static_cast<unsigned>(size));
    }
    const HRESULT result = g_oCreateVertexShader(device, bytecode, size, linkage, shader);
    if (SUCCEEDED(result) && shader && *shader && bytecode && size && !g_own)
    {
        const unsigned long long hash = Fnv64(bytecode, size);
        (*shader)->SetPrivateData(kShaderHashGuid, sizeof(hash), &hash);
        if (size <= 65536) (*shader)->SetPrivateData(kEjectionBytecodeGuid, static_cast<UINT>(size), bytecode);
    }
    return result;
}

// Collects the texture slots a pixel shader declares.
UINT PixelShaderSlots(const void* bytecode, SIZE_T size)
{
    const uint8_t* bytes = static_cast<const uint8_t*>(bytecode);
    if (size < 32 || std::memcmp(bytes, "DXBC", 4) != 0)
        return 0xffff;
    uint32_t chunkCount = 0;
    std::memcpy(&chunkCount, bytes + 28, 4);
    if (chunkCount > 64 || 32 + 4 * static_cast<size_t>(chunkCount) > size)
        return 0xffff;
    for (uint32_t c = 0; c < chunkCount; ++c)
    {
        uint32_t offset = 0, tag = 0, chunkSize = 0;
        std::memcpy(&offset, bytes + 32 + 4 * c, 4);
        if (offset + 8 > size) return 0xffff;
        std::memcpy(&tag, bytes + offset, 4);
        std::memcpy(&chunkSize, bytes + offset + 4, 4);
        if (tag != 0x52444853 && tag != 0x58454853) continue;
        if (offset + 8 + static_cast<size_t>(chunkSize) > size) return 0xffff;
        const size_t tokenCount = chunkSize / 4;
        const uint8_t* base = bytes + offset + 8;
        auto token = [&](size_t i) { uint32_t v; std::memcpy(&v, base + 4 * i, 4); return v; };
        UINT mask = 0;
        for (size_t i = 2; i < tokenCount;)
        {
            const uint32_t op = token(i) & 0x7ff;
            const uint32_t length = op == 53 ? (i + 1 < tokenCount ? token(i + 1) : 0)
                                             : (token(i) >> 24) & 0x7f;
            if (length == 0 || i + length > tokenCount) return 0xffff;
            // dcl_resource / dcl_resource_raw / dcl_resource_structured
            if ((op == 88 || op == 161 || op == 162) && length >= 3)
            {
                const uint32_t slot = token(i + 2);
                if (slot >= kPsSlots) return 0xffff;
                mask |= 1u << slot;
            }
            i += length;
        }
        return mask;
    }
    return 0xffff;
}

HRESULT STDMETHODCALLTYPE HookCreatePixelShader(
    ID3D11Device* device, const void* bytecode, SIZE_T size, ID3D11ClassLinkage* linkage,
    ID3D11PixelShader** shader)
{
    HRESULT result = E_FAIL;
    bool patchedPixel = false;
    // test hook: lets the offline test prove it detects the uncorrected case
    static const bool pixelPatchOff = GetEnvironmentVariableW(L"FC3_STEREO_NO_PIXEL_FIX", nullptr, 0) > 0;
    if (bytecode && size && shader && !g_own && !pixelPatchOff)
    {
        std::vector<uint8_t> patched;
        const dxbc::PixelResult outcome = dxbc::PatchPixelShader(bytecode, size, patched);
        if (outcome == dxbc::PixelResult::Patched)
        {
            result = g_oCreatePixelShader(device, patched.data(), patched.size(), linkage, shader);
            patchedPixel = SUCCEEDED(result) && *shader;
        }
        if (outcome != dxbc::PixelResult::NotApplicable && !patchedPixel)
            Log("[PS] depth-unprojecting shader left unpatched (size %u)", static_cast<unsigned>(size));
    }
    if (!patchedPixel)
        result = g_oCreatePixelShader(device, bytecode, size, linkage, shader);
    if (patchedPixel)
    {
        SetFlag(*shader, kPsPatchedGuid, 1);
        InterlockedIncrement(&g_stats.psPatched);
    }
    if (SUCCEEDED(result) && shader && *shader && bytecode)
    {
        const UINT mask = PixelShaderSlots(bytecode, size);
        (*shader)->SetPrivateData(kPsSlotsGuid, sizeof(mask), &mask);
        if (size <= 65536 && !g_own) (*shader)->SetPrivateData(kEjectionBytecodeGuid, static_cast<UINT>(size), bytecode);
        const unsigned long long hash = Fnv64(bytecode, size);
        (*shader)->SetPrivateData(kShaderHashGuid, sizeof(hash), &hash);
    }
    return result;
}

// ---- settings and eye buffers -----------------------------------------------
float ReadFloat(const wchar_t* path, const wchar_t* key, float fallback)
{
    wchar_t text[64] = {};
    GetPrivateProfileStringW(L"Stereo", key, L"", text, 64, path);
    if (!text[0]) return fallback;
    wchar_t* end = nullptr;
    const float value = static_cast<float>(wcstod(text, &end));
    return end == text ? fallback : value;
}

void LoadSettings()
{
    wchar_t path[MAX_PATH] = {};
    lstrcpynW(path, fc3::DataFolder(), MAX_PATH);
    if (lstrlenW(path) + 16 >= MAX_PATH) return;
    lstrcatW(path, L"fc3_stereo.ini");
    LoadManualReloadSettings();ApplyManualReloadMode();LoadMacheteSettings(path);
    g_enabled = ReadFloat(path, L"Enabled", 1.0f) != 0.0f;
    g_view = static_cast<int>(ReadFloat(path, L"MonitorView", 1.0f));
    g_separation = ReadFloat(path, L"Separation", g_separation);
    g_uiDepth = ReadFloat(path, L"UiDepth", g_uiDepth);
    g_weaponNear = ReadFloat(path, L"WeaponNear", g_weaponNear);
    g_weaponScale = ReadFloat(path, L"WeaponScale", g_weaponScale);
    g_fpsCap = ReadFloat(path, L"FpsCap", g_fpsCap);
    g_worldScale = ReadFloat(path, L"WorldScale", g_worldScale);
    g_headTracking = ReadFloat(path, L"HeadTracking", 1.0f) != 0.0f;
    g_pacePeriods = static_cast<int>(ReadFloat(path, L"PacePeriods", -1.0f));
    g_uiWidthDeg = ReadFloat(path, L"UiWidthDeg", g_uiWidthDeg);
    g_motionGun = ReadFloat(path, L"MotionArms", 1.0f) != 0.0f;
    g_gunOffset[0] = ReadFloat(path, L"GunX", g_gunOffset[0]);
    g_gunOffset[1] = ReadFloat(path, L"GunY", g_gunOffset[1]);
    g_gunOffset[2] = ReadFloat(path, L"GunZ", g_gunOffset[2]);
    if (g_uiWidthDeg < 20.0f) g_uiWidthDeg = 20.0f;
    if (g_uiWidthDeg > 120.0f) g_uiWidthDeg = 120.0f;
    if (g_worldScale < 0.1f) g_worldScale = 0.1f;
    if (g_uiDepth < 0.1f) g_uiDepth = 0.1f;
    Log("[SET] enabled=%d view=%d separation=%.4f uiDepth=%.2f weaponNear=%.3f weaponScale=%.3f fpsCap=%.0f",
        g_enabled ? 1 : 0, g_view, g_separation, g_uiDepth, g_weaponNear, g_weaponScale, g_fpsCap);
}

// ---- small matrix helpers (row-major, column vectors: v' = M v) -------------
struct Mat4 { double m[16]; };

Mat4 Multiply(const Mat4& a, const Mat4& b)
{
    Mat4 r = {};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
        {
            double sum = 0;
            for (int k = 0; k < 4; ++k) sum += a.m[i * 4 + k] * b.m[k * 4 + j];
            r.m[i * 4 + j] = sum;
        }
    return r;
}

bool Invert(const Mat4& source, Mat4& out)
{
    double a[4][8];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
        {
            a[i][j] = source.m[i * 4 + j];
            a[i][4 + j] = i == j ? 1.0 : 0.0;
        }
    for (int column = 0; column < 4; ++column)
    {
        int pivot = column;
        for (int row = column + 1; row < 4; ++row)
            if (std::fabs(a[row][column]) > std::fabs(a[pivot][column])) pivot = row;
        if (std::fabs(a[pivot][column]) < 1e-12) return false;
        if (pivot != column)
            for (int j = 0; j < 8; ++j) { const double t = a[column][j]; a[column][j] = a[pivot][j]; a[pivot][j] = t; }
        const double scale = 1.0 / a[column][column];
        for (int j = 0; j < 8; ++j) a[column][j] *= scale;
        for (int row = 0; row < 4; ++row)
        {
            if (row == column) continue;
            const double factor = a[row][column];
            if (factor == 0.0) continue;
            for (int j = 0; j < 8; ++j) a[row][j] -= factor * a[column][j];
        }
    }
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) out.m[i * 4 + j] = a[i][4 + j];
    return true;
}

Mat4 ProjectionMatrix(const GameProjection& p)
{
    Mat4 r = {};
    r.m[0] = p.xs; r.m[5] = p.ys; r.m[10] = p.a; r.m[11] = p.b; r.m[14] = 1.0;
    return r;
}

Mat4 ProjectionInverse(const GameProjection& p)
{
    Mat4 r = {};
    r.m[0] = 1.0 / p.xs; r.m[5] = 1.0 / p.ys; r.m[11] = 1.0;
    r.m[14] = 1.0 / p.b; r.m[15] = -static_cast<double>(p.a) / p.b;
    return r;
}

// Off-centre frustum from half-angle tangents, same depth mapping as the game.
Mat4 EyeProjectionMatrix(const fc3stereo::EyeView& eye, const GameProjection& depth)
{
    const double width = eye.tanRight - eye.tanLeft, height = eye.tanUp - eye.tanDown;
    Mat4 r = {};
    r.m[0] = 2.0 / width;  r.m[2] = -(eye.tanRight + eye.tanLeft) / width;
    r.m[5] = 2.0 / height; r.m[6] = -(eye.tanUp + eye.tanDown) / height;
    r.m[10] = depth.a; r.m[11] = depth.b; r.m[14] = 1.0;
    return r;
}

// ---- elbow ------------------------------------------------------------------
struct V3 { double x, y, z; };
V3 Sub(V3 a, V3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
V3 Add(V3 a, V3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
V3 Scale(V3 a, double s) { return { a.x * s, a.y * s, a.z * s }; }
double Dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 Cross(V3 a, V3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
double Length(V3 a) { return std::sqrt(Dot(a, a)); }
V3 Unit(V3 a) { const double l = Length(a); return l > 1e-9 ? Scale(a, 1.0 / l) : V3{ 0, 0, 0 }; }
V3 Apply(const Mat4& m, V3 p)
{
    return { m.m[0] * p.x + m.m[1] * p.y + m.m[2] * p.z + m.m[3],
             m.m[4] * p.x + m.m[5] * p.y + m.m[6] * p.z + m.m[7],
             m.m[8] * p.x + m.m[9] * p.y + m.m[10] * p.z + m.m[11] };
}

// Rotation taking direction a onto direction b (shortest arc), about point
// 'from', then moving 'from' to 'to'.
Mat4 TurnAndMove(V3 a, V3 b, V3 from, V3 to)
{
    a = Unit(a);
    b = Unit(b);
    const V3 axis = Cross(a, b);
    const double s = Length(axis), c = Dot(a, b);
    double r[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    if (s > 1e-9)
    {
        const V3 k = Scale(axis, 1.0 / s);
        const double kk[3] = { k.x, k.y, k.z };
        const double K[3][3] = { { 0, -k.z, k.y }, { k.z, 0, -k.x }, { -k.y, k.x, 0 } };
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                r[i][j] = (i == j ? c : 0.0) + s * K[i][j] + (1.0 - c) * kk[i] * kk[j];
    }
    Mat4 m = {};
    const double f[3] = { from.x, from.y, from.z }, t[3] = { to.x, to.y, to.z };
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j) m.m[i * 4 + j] = r[i][j];
        m.m[i * 4 + 3] = t[i] - (r[i][0] * f[0] + r[i][1] * f[1] + r[i][2] * f[2]);
    }
    m.m[15] = 1.0;
    return m;
}

// Where the wrist sits in the controller's own frame (x right, y up, z where
// it points; metres): a little behind the tip, inside the hand. The hand turns
// about this point, so turning the wrist does not move the elbow.
double g_wristOffset[2][3] = { { 0.0, -0.02, -0.10 }, { 0.0, -0.02, -0.10 } };

// The hand: turned by how far the controller has turned from its reference,
// about the game's wrist, and put at the controller's wrist point.
Mat4 HandMove(const Mat4& controller, const Mat4& reference, V3 gameWrist, int side)
{
    double r[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
        {
            r[i][j] = 0;
            for (int k = 0; k < 3; ++k) r[i][j] += controller.m[i * 4 + k] * reference.m[j * 4 + k];
        }
    const double* off = g_wristOffset[side];
    V3 wrist = { controller.m[3], controller.m[7], controller.m[11] };
    wrist = Add(wrist, V3{ controller.m[0] * off[0] + controller.m[1] * off[1] + controller.m[2] * off[2],
                           controller.m[4] * off[0] + controller.m[5] * off[1] + controller.m[6] * off[2],
                           controller.m[8] * off[0] + controller.m[9] * off[1] + controller.m[10] * off[2] });
    Mat4 m = {};
    const double g[3] = { gameWrist.x, gameWrist.y, gameWrist.z }, w[3] = { wrist.x, wrist.y, wrist.z };
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j) m.m[i * 4 + j] = r[i][j];
        m.m[i * 4 + 3] = w[i] - (r[i][0] * g[0] + r[i][1] * g[1] + r[i][2] * g[2]);
    }
    m.m[15] = 1.0;
    return m;
}

// Two-bone arm. The wrist goes where the controller puts it; the upper arm and
// forearm keep their lengths; the shoulder stays put; the elbow bends in the
// plane the game's own elbow is bent in. The forearm points from the elbow to
// the wrist; the hand's own turn happens at the wrist.
bool SolveArm(const ArmJoints& joints, Mat4& handMove, int side, Mat4& forearmMove, Mat4& upperMove,
              bool keepHand = false)
{
    if (!joints.valid) return false;
    const V3 elbow = { joints.e[0], joints.e[1], joints.e[2] };
    const V3 wrist = { joints.w[0], joints.w[1], joints.w[2] };
    double upperLength = 0.28;
    // where a shoulder sits (body frame, metres from the eyes), in camera axes
    const double spotBody[3] = { side == 0 ? 0.20 : -0.20, -0.15, -0.20 };
    V3 shoulderSpot = { spotBody[0], spotBody[1], spotBody[2] };
    if (g_aimActive)
        shoulderSpot = { g_aimQ[0][0] * spotBody[0] + g_aimQ[0][1] * spotBody[1] + g_aimQ[0][2] * spotBody[2],
                         g_aimQ[1][0] * spotBody[0] + g_aimQ[1][1] * spotBody[1] + g_aimQ[1][2] * spotBody[2],
                         g_aimQ[2][0] * spotBody[0] + g_aimQ[2][1] * spotBody[1] + g_aimQ[2][2] * spotBody[2] };
    // the shoulder is fixed to the body; the elbow keeps the game's distance to it
    const V3 shoulder = shoulderSpot;
    upperLength = Length(Sub(elbow, shoulder));
    if (upperLength < 0.12) upperLength = 0.12;
    if (upperLength > 0.45) upperLength = 0.45;
    const double forearmLength = Length(Sub(wrist, elbow));
    const V3 target = Apply(handMove, wrist);

    const V3 toTarget = Sub(target, shoulder);
    double reach = Length(toTarget);
    if (reach < 1e-6) return false;
    const V3 along = Scale(toTarget, 1.0 / reach);
    const double longest = (upperLength + forearmLength) * 0.999;
    const double shortest = std::fabs(upperLength - forearmLength) + 0.01;
    if (reach > longest) reach = longest;
    if (reach < shortest) reach = shortest;
    const double a = (upperLength * upperLength - forearmLength * forearmLength + reach * reach) / (2.0 * reach);
    const double h = std::sqrt((std::max)(upperLength * upperLength - a * a, 0.0));
    const V3 gameLine = Unit(Sub(wrist, shoulder));
    V3 bend = Sub(elbow, Add(shoulder, Scale(gameLine, Dot(Sub(elbow, shoulder), gameLine))));
    bend = Sub(bend, Scale(along, Dot(bend, along)));
    if (Length(bend) < 0.01)
    {
        bend = { side == 0 ? 0.5 : -0.5, -1.0, -0.2 };
        bend = Sub(bend, Scale(along, Dot(bend, along)));
    }
    const V3 newElbow = Add(Add(shoulder, Scale(along, a)), Scale(Unit(bend), h));
    const V3 newWrist = Add(shoulder, Scale(along, reach));
    upperMove = TurnAndMove(Sub(elbow, shoulder), Sub(newElbow, shoulder), shoulder, shoulder);
    if (!keepHand)
    {
        // Out of reach (or too close): the hand stays on the end of the arm
        // instead of pulling away from the wrist; it stops short of the controller.
        handMove.m[3] += newWrist.x - target.x;
        handMove.m[7] += newWrist.y - target.y;
        handMove.m[11] += newWrist.z - target.z;
        forearmMove = TurnAndMove(Sub(wrist, elbow), Sub(newWrist, newElbow), elbow, newElbow);
        return true;
    }
    // The hand must stay put (it holds the gun): the forearm reaches it,
    // stretching along its length if the arm is too short.
    const V3 u = Unit(Sub(wrist, elbow));
    const double stretch = Length(Sub(target, newElbow)) / (forearmLength > 1e-6 ? forearmLength : 1.0);
    Mat4 along2 = {}, toElbow = {};
    const double uu[3] = { u.x, u.y, u.z };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) along2.m[i * 4 + j] = (i == j ? 1.0 : 0.0) + (stretch - 1.0) * uu[i] * uu[j];
    along2.m[15] = 1.0;
    toElbow.m[0] = toElbow.m[5] = toElbow.m[10] = toElbow.m[15] = 1.0;
    toElbow.m[3] = -elbow.x; toElbow.m[7] = -elbow.y; toElbow.m[11] = -elbow.z;
    const Mat4 turn = TurnAndMove(Sub(wrist, elbow), Sub(target, newElbow), V3{ 0, 0, 0 }, newElbow);
    forearmMove = Multiply(turn, Multiply(along2, toElbow));
    return true;
}

// Reads this draw's world matrix, camera and bone palette copies and builds
// its model -> first-person view mapping v = G m + g (as a matrix) and inverse.
bool DrawSpace(ID3D11DeviceContext* context, float bones[48 * 12], Mat4& toView, Mat4& fromView)
{
    ID3D11Buffer* buffers[4] = {};
    context->VSGetConstantBuffers(0, 4, buffers);
    float globals[kSmallCbMax / 4] = {};
    CameraSummary camera = {};
    bool ok = ReadCopy(buffers[0], globals, sizeof(globals)) >= 48;
    UINT size = sizeof(camera);
    ok = ok && buffers[1] && SUCCEEDED(buffers[1]->GetPrivateData(kCameraSummaryGuid, &size, &camera)) &&
         size == sizeof(camera) && camera.mainView;
    ok = ok && (!bones || ReadCopy(buffers[3], bones, 48 * 12 * sizeof(float)) == 48 * 12 * sizeof(float));
    for (ID3D11Buffer*& b : buffers) SafeRelease(b);
    if (!ok) return false;
    toView = {};
    for (int r = 0; r < 3; ++r)
    {
        const float* axis = r == 0 ? camera.right : r == 1 ? camera.up : camera.forward;
        for (int c = 0; c < 3; ++c)
            toView.m[r * 4 + c] = axis[0] * globals[0 * 4 + c] + axis[1] * globals[1 * 4 + c] + axis[2] * globals[2 * 4 + c];
        double t[3];
        for (int k = 0; k < 3; ++k) t[k] = static_cast<double>(globals[k * 4 + 3]) - camera.position[k];
        toView.m[r * 4 + 3] = axis[0] * t[0] + axis[1] * t[1] + axis[2] * t[2];
    }
    toView.m[15] = 1.0;
    return Invert(toView, fromView);
}

// Applies a model-space move to bones [first, last] of a palette.
void MoveBones(float* bones, int first, int last, const Mat4& x)
{
    for (int bone = first; bone <= last; ++bone)
    {
        float* m = bones + bone * 12;
        float out[12];
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 4; ++c)
            {
                double v = x.m[r * 4 + 0] * m[0 * 4 + c] + x.m[r * 4 + 1] * m[1 * 4 + c] + x.m[r * 4 + 2] * m[2 * 4 + c];
                if (c == 3) v += x.m[r * 4 + 3];
                out[r * 4 + c] = static_cast<float>(v);
            }
        std::memcpy(m, out, sizeof(out));
    }
}

// ---- two-handed hold ----------------------------------------------------------
// Squeezing the left grip near the gun's front (where the game puts the left
// hand on it) locks the left hand there: the gun then points along the line
// from the right wrist to the left hand, and the left hand moves with the gun.
bool g_twoHand = false;
Mat4 g_frameGunMove = {};
bool g_frameGunValid = false;

// ---- pump action -------------------------------------------------------------
// After a shot the gun is not chambered: the trigger does not reach the game
// until the left hand, holding the gun's front, slides back and forward again.
bool g_pumpEnabled = true;
bool g_chambered = true;
int g_pumpStage = 0;                // 0: waiting for the pull back, 1: back, waiting for forward
double g_pumpBase = 0;              // slide position when the pump started
bool g_pumpBaseSet = false;
double g_slide = 0, g_slideGrab = 0;  // left hand along the barrel (m), now and when it grabbed
double g_slideAxis[3] = { 0, 0, 1 };  // barrel direction, camera frame
float g_lastTrigger = 0;
LONG g_lastShotFrame = -100000;
double g_pumpAnimView[3] = {};      // the game's own pump slide this frame (camera frame, gun as the game holds it)
double g_pumpRange = 0.12;          // how far the forend travels (learned)
bool g_pumpLearned = false;

#include "action_probe.inl"
#include "ejection_probe.inl"
#include "physical_ejection.inl"
#include "ammo_probe.inl"
#include "ammo_owner_probe.inl"
#include "auto_ammo_capture.inl"
void ProbeNativeAnimation(UINT,UINT,UINT,int,UINT,UINT);
#include "reload_engine.inl"
#include "animation_probe.inl"
void CorrectShotHandMove(const ArmJoints&,Mat4&);
void ObserveShotgunRoot(const float*,const Mat4&,UINT,UINT);
bool ManualReloadNote();
bool ManualReloadOwnsLeftHand();
void ManualMarker();
bool ReloadContactDistance(double&);
void RecordReloadPort(ID3D11DeviceContext*,const float*,UINT,UINT);
void MoveReloadShell(float* bones,const Mat4& fromView);
void CaptureReloadShell(ID3D11DeviceContext*,UINT,UINT);
void ReplayReloadShell(ID3D11DeviceContext*);
void ClearReloadShell();
bool MacheteHeld();
void ClearMacheteVisual();
void LoadMacheteSettings(const wchar_t*);
bool ReloadShellReady();
#include "ammo_test_note.inl"
double g_barrelCam[3] = { 0, 0, 1 };      // gun direction (camera frame) while held with two hands

V3 ControllerWrist(const float* pose, int side)
{
    const double* off = g_wristOffset[side];
    return { pose[3] + pose[0] * off[0] + pose[1] * off[1] + pose[2] * off[2],
             pose[7] + pose[4] * off[0] + pose[5] * off[1] + pose[6] * off[2],
             pose[11] + pose[8] * off[0] + pose[9] * off[1] + pose[10] * off[2] };
}

Mat4 ControllerHandMove(int side, V3 gameWrist)
{
    Mat4 hand = {}, reference = {};
    const float* pose = side == 0 ? g_handRight : g_handLeft;
    for (int i = 0; i < 16; ++i)
    {
        hand.m[i] = pose[i];
        reference.m[i] = g_handRef[side][i];
    }
    return HandMove(hand, reference, gameWrist, side);
}

// The gun's move (= the right hand's): the right controller, turned so the
// game's left-hand spot on the gun points at the real left hand when held
// with two hands; kept on the right arm's reach.
bool GunMove(const ArmJoints& right, const ArmJoints& left, Mat4& move)
{
    if (!MotionInteractionEnabled() || !g_eyeViewsSet || !g_handRightValid || !right.valid) return false;
    const V3 wrist = { right.w[0], right.w[1], right.w[2] };
    move = ControllerHandMove(0, wrist);
    if (g_twoHand && left.valid && g_handLeftValid)
    {
        const V3 newWrist = Apply(move, wrist);
        const V3 support = Apply(move, V3{ left.w[0], left.w[1], left.w[2] });
        const V3 real = ControllerWrist(g_handLeft, 1);
        if (Length(Sub(real, newWrist)) > 0.05 && Length(Sub(support, newWrist)) > 0.05)
            move = Multiply(TurnAndMove(Sub(support, newWrist), Sub(real, newWrist), newWrist, newWrist), move);
    }
    Mat4 forearmMove = {}, upperMove = {};
    if(!SolveArm(right, move, 0, forearmMove, upperMove))return false;
    CorrectShotHandMove(right,move);
    return true;
}

// The left hand's move: its own controller, or the gun's while it holds it.
bool LeftMove(const ArmJoints& left, const ArmJoints& right, Mat4& move)
{
    (void)right;
    if (!MotionInteractionEnabled() || !g_eyeViewsSet || !g_handLeftValid || !left.valid) return false;
    if (g_twoHand && g_frameGunValid)
    {
        // along the barrel with the real hand (back up to 12 cm, forward 2 cm)
        double slide = g_slide - g_slideGrab;
        if (slide < -g_pumpRange) slide = -g_pumpRange;
        if (slide > 0.02) slide = 0.02;
        // the game's own pump animation drags the hand along: take that out
        // (turned like the gun), the real hand drives it instead
        const Mat4& gm = g_frameGunMove;
        const double* a = g_pumpAnimView;
        const double anim[3] = { gm.m[0] * a[0] + gm.m[1] * a[1] + gm.m[2] * a[2],
                                 gm.m[4] * a[0] + gm.m[5] * a[1] + gm.m[6] * a[2],
                                 gm.m[8] * a[0] + gm.m[9] * a[1] + gm.m[10] * a[2] };
        Mat4 shift = {};
        shift.m[0] = shift.m[5] = shift.m[10] = shift.m[15] = 1.0;
        shift.m[3] = g_slideAxis[0] * slide - anim[0];
        shift.m[7] = g_slideAxis[1] * slide - anim[1];
        shift.m[11] = g_slideAxis[2] * slide - anim[2];
        move = Multiply(shift, g_frameGunMove);
        return true;
    }
    move = ControllerHandMove(1, V3{ left.w[0], left.w[1], left.w[2] });
    return true;
}

// The pump: where the left hand is along the barrel, shots and pumps.
struct PumpFireAllowance { LONG shot=-100000; bool passing=false,waitRelease=false; } g_pumpFire;
void UpdatePump(const ArmJoints& right, const ArmJoints& left)
{
    const fc3pad::Controllers c = fc3pad::Latest();
    if (!MotionInteractionEnabled())
    {
        g_lastTrigger = c.rightTrigger;
        g_pumpFire.waitRelease = true;
        fc3pad::RequireFullFirePull(false);
        fc3pad::BlockFire(false);
        return; // hidden-weapon buttons cannot consume a physical chamber
    }
    // where the left hand is along the gun: from the right wrist towards the
    // gun's front grip
    if (g_twoHand && g_frameGunValid && right.valid && left.valid)
    {
        const V3 wrist = Apply(g_frameGunMove, V3{ right.w[0], right.w[1], right.w[2] });
        const V3 front = Apply(g_frameGunMove, V3{ left.w[0], left.w[1], left.w[2] });
        const V3 axis = Unit(Sub(front, wrist));
        g_slideAxis[0] = axis.x; g_slideAxis[1] = axis.y; g_slideAxis[2] = axis.z;
        const V3 real = ControllerWrist(g_handLeft, 1);
        g_slide = Dot(Sub(real, wrist), axis);
    }
    static bool wasHolding = false;
    if (g_twoHand && !wasHolding) g_slideGrab = g_slide;
    wasHolding = g_twoHand;

    // a shot: the trigger pulled through while chambered
    if (g_pumpEnabled && g_chambered && !fc3pad::MacheteFireBlocked() && c.rightTrigger > 0.7f && g_lastTrigger <= 0.7f)
    {
        g_chambered = false;
        g_pumpStage = 0;
        g_pumpBaseSet = false;
        g_lastShotFrame = g_frame;
        static LONG shots = 0;
        if (InterlockedIncrement(&shots) <= 5) Log("[PUMP] shot at frame %ld (watching the weapon for the pump)", static_cast<long>(g_frame));
    }
    g_lastTrigger = c.rightTrigger;
    static bool priorReload=false;
    if (c.x&&!priorReload&&!g_manualReloadEnabled) g_chambered = true; // configured ordinary reload only; manual pickup/save X cannot chamber
    priorReload=c.x;
    if (!g_pumpEnabled) g_chambered = true;
    if (!g_chambered && g_twoHand)
    {
        if (!g_pumpBaseSet) { g_pumpBase = g_slide; g_pumpBaseSet = true; }
        const double needed = g_pumpLearned ? (std::min)(0.06, g_pumpRange * 0.7) : 0.06;
        if (g_pumpStage == 0 && g_slide < g_pumpBase - needed) g_pumpStage = 1;
        if (g_pumpStage == 1 && g_slide > g_pumpBase - 0.02)
        {
            g_chambered = true;
            fc3xr::Buzz(1);
            static LONG pumps = 0;
            if (InterlockedIncrement(&pumps) <= 3) Log("[ARMS] pumped");
        }
    }
    // The initial shot gets one bounded allowance. Release or expiry closes
    // it permanently; no counter wrap may reopen it before a physical pump.
    if(g_pumpFire.shot!=g_lastShotFrame)
    {g_pumpFire.shot=g_lastShotFrame;g_pumpFire.passing=true;g_pumpFire.waitRelease=true;}
    if(c.rightTrigger<.3f||g_frame-g_pumpFire.shot>=6)g_pumpFire.passing=false;
    if(!g_chambered)g_pumpFire.waitRelease=true;
    if(g_chambered&&c.rightTrigger<.3f)g_pumpFire.waitRelease=false;
    if(!g_pumpEnabled)g_pumpFire=PumpFireAllowance{};
    fc3pad::RequireFullFirePull(g_pumpEnabled);
    fc3pad::BlockFire(g_pumpEnabled&&!g_pumpFire.passing&&(!g_chambered||g_pumpFire.waitRelease));
}

// Once per frame (camera frame, after the aim turn): grab / release, and the
// gun direction the aim uses while held with both hands.
void UpdateTwoHand()
{
    if(MacheteHeld())
    {
        g_twoHand=false;g_frameGunValid=false;g_lastTrigger=fc3pad::Latest().rightTrigger;
        fc3pad::SuppressLeftGrip(false);return; // preserve firearm chamber/pump state
    }
    // the gun's move for this frame, before any grab change (from the arms
    // measured last frame); recomputed below once the hold state is known
    g_frameGunValid = GunMove(g_armJoints[0], g_armJoints[1], g_frameGunMove);
    struct Finish { ~Finish() { g_frameGunValid = GunMove(g_armJoints[0], g_armJoints[1], g_frameGunMove); } } finish;
    float leftGrip = 0, rightGrip = 0;
    fc3pad::Grips(leftGrip, rightGrip);
    const bool manualLeft=ManualReloadOwnsLeftHand();
    if(g_reload.held||manualLeft)leftGrip=0;
    const ArmJoints& right = g_armJoints[0];
    const ArmJoints& left = g_armJoints[1];
    const bool possible = MotionInteractionEnabled() && g_eyeViewsSet && g_handRightValid && g_handLeftValid && right.valid && left.valid;
    if (!possible)
    {
        if (g_twoHand) Log("[ARMS] two-handed hold released (hands not tracked)");
        g_twoHand = false;
        fc3pad::SuppressLeftGrip(manualLeft);
        UpdatePump(right,left); // keep fire locked even when the other hand loses tracking
        return;
    }
    if (!g_twoHand)
    {
        Mat4 move = {};
        const bool wasTwo = g_twoHand;
        g_twoHand = false;
        if (GunMove(right, left, move) && leftGrip > 0.6f)
        {
            const V3 support = Apply(move, V3{ left.w[0], left.w[1], left.w[2] });
            const V3 real = ControllerWrist(g_handLeft, 1);
            if (Length(Sub(real, support)) < 0.15)
            {
                g_twoHand = true;
                fc3xr::Buzz(1);
                Log("[ARMS] left hand grabbed the gun (%.2f m from its front grip)", Length(Sub(real, support)));
            }
        }
        (void)wasTwo;
    }
    else if (leftGrip < 0.4f)
    {
        g_twoHand = false;
        Log("[ARMS] left hand let go of the gun");
    }
    fc3pad::SuppressLeftGrip(g_twoHand||g_reload.held||manualLeft);
    UpdatePump(right, left);
    if (g_twoHand)
    {
        // gun direction for the aim: the right controller's forward, turned
        // like the gun was, back in the body frame
        Mat4 base = ControllerHandMove(0, V3{ right.w[0], right.w[1], right.w[2] }), move = {};
        if (GunMove(right, left, move))
        {
            Mat4 baseInverse = {};
            if (Invert(base, baseInverse))
            {
                const Mat4 turn = Multiply(move, baseInverse);
                const V3 forward = { g_handRight[2], g_handRight[6], g_handRight[10] };
                const V3 turned = { turn.m[0] * forward.x + turn.m[1] * forward.y + turn.m[2] * forward.z,
                                    turn.m[4] * forward.x + turn.m[5] * forward.y + turn.m[6] * forward.z,
                                    turn.m[8] * forward.x + turn.m[9] * forward.y + turn.m[10] * forward.z };
                g_barrelCam[0] = turned.x; g_barrelCam[1] = turned.y; g_barrelCam[2] = turned.z;
            }
        }
    }
}

// The right hand's move this frame (wrist pivot, kept on the arm), from the
// latest measured arms. False when there is nothing to follow.
bool RightHandMove(Mat4& handMove)
{
    if (!MotionInteractionEnabled() || !g_frameGunValid) return false;
    handMove = g_frameGunMove;
    return true;
}

// ---- the shotgun's forend --------------------------------------------------------
// The weapon's bones relative to its first bone: after a shot the game's pump
// animation slides one of them back and forward along the barrel. That bone is
// found by watching (the largest travel within 100 frames of a shot), then
// driven by the real left hand instead.
Mat4 BoneMatrix(const float* bones, int bone);
void StoreBone(float* bones, int bone, const Mat4& m);

struct ForendState
{
    UINT vbBytes;
    bool restSet, learned, evaluated;
    float rest[48][12];             // bone matrices while the gun is idle
    bool restUsable[48];
    bool pairSeen[48][48];          // zero travel is meaningful only for an observed pair
    float maxDist[48][48];          // [reference][bone]: travel relative to the reference after a shot
    float maxVec[48][48][3];
    int reference, bone;
    bool rides[48];                 // bones that move with the forend
    double back[3];                 // slide direction in the reference bone's frame
    double range;
    Mat4 restRelative;              // forend relative to the reference, idle
};
// Let the weapon buffer own its learner. A fixed pointer table filled up with
// earlier meshes and silently excluded the current weapon; released buffers
// could also leave stale identities in that table.
const GUID kForendStateGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 14 } };
struct ForendRecord : IUnknown
{
    LONG refs = 1;
    ForendState state = {};
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override
    {
        if (!result) return E_POINTER;
        *result = nullptr;
        if (!IsEqualIID(iid, __uuidof(IUnknown))) return E_NOINTERFACE;
        *result = static_cast<IUnknown*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refs); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const LONG remaining = InterlockedDecrement(&refs);
        if (!remaining) delete this;
        return remaining;
    }
};
struct ForendRecordScope
{
    ForendRecord* record;
    ~ForendRecordScope() { if (record) record->Release(); }
};

bool BoneUsable(const float* m)
{
    for (int r = 0; r < 3; ++r)
    {
        const double n = m[r * 4] * m[r * 4] + m[r * 4 + 1] * m[r * 4 + 1] + m[r * 4 + 2] * m[r * 4 + 2];
        if (!(n > 0.8 && n < 1.2)) return false;
    }
    return true;
}

Mat4 BoneMatrix(const float* bones, int bone);
void StoreBone(float* bones, int bone, const Mat4& m);

// translation of bone b relative to bone r
void RelativeTranslation(const float* br, const float* bb, double out[3])
{
    Mat4 r = {}, m = {}, ri = {};
    for (int i = 0; i < 12; ++i) { r.m[i] = br[i]; m.m[i] = bb[i]; }
    r.m[15] = m.m[15] = 1.0;
    if (!Invert(r, ri)) { out[0] = out[1] = out[2] = 0; return; }
    const Mat4 rel = Multiply(ri, m);
    out[0] = rel.m[3]; out[1] = rel.m[7]; out[2] = rel.m[11];
}

// The pump: the game's animation slides one bone of the gun against the gun's
// body. The body is the bone most others stay rigid to during the animation;
// the forend the bone that slides 3-25 cm against it.
void DriveForend(ID3D11DeviceContext* context, float* bones, const Mat4& toView, UINT start, UINT count)
{
    (void)start;
    (void)count;
    ID3D11Buffer* stream = nullptr;
    UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
    if (!stream) return;
    D3D11_BUFFER_DESC vbDesc = {};
    stream->GetDesc(&vbDesc);
    ForendRecord* record = nullptr;
    UINT recordBytes = sizeof(record);
    if (FAILED(stream->GetPrivateData(kForendStateGuid, &recordBytes, &record)))
    {
        record = new (std::nothrow) ForendRecord;
        if (record)
        {
            record->state.vbBytes = vbDesc.ByteWidth;
            if (FAILED(stream->SetPrivateDataInterface(kForendStateGuid, record)))
            {
                record->Release();
                record = nullptr;
            }
            else Log("[PUMP] tracking weapon buffer %u bytes (resource-owned learner)", vbDesc.ByteWidth);
        }
    }
    stream->Release();
    ForendRecordScope recordScope = { record };
    if (!record) return;
    ForendState* st = &record->state;

    bool usable[48] = {};
    for (int b = 0; b < 48; ++b) usable[b] = BoneUsable(bones + b * 12);
    const LONG since = g_frame - g_lastShotFrame;
    if (!st->learned)
    {
        if (since > 90)
        {
            for (int b = 0; b < 48; ++b)
            {
                // Each weapon piece only fills the bones it uses. Preserve
                // rest bones captured from other pieces sharing this mesh.
                if (!usable[b]) continue;
                std::memcpy(st->rest[b], bones + b * 12, sizeof(st->rest[b]));
                st->restUsable[b] = true;
            }
            std::memset(st->maxDist, 0, sizeof(st->maxDist));
            std::memset(st->pairSeen, 0, sizeof(st->pairSeen));
            st->restSet = true;
            st->evaluated = false;
        }
        else if (st->restSet && since < 45)
        {
            for (int r = 0; r < 48; ++r)
            {
                if (!usable[r] || !st->restUsable[r]) continue;
                for (int b = 0; b < 48; ++b)
                {
                    if (b == r || !usable[b] || !st->restUsable[b]) continue;
                    st->pairSeen[r][b] = true;
                    double now[3], was[3];
                    RelativeTranslation(bones + r * 12, bones + b * 12, now);
                    RelativeTranslation(st->rest[r], st->rest[b], was);
                    const double d[3] = { now[0] - was[0], now[1] - was[1], now[2] - was[2] };
                    const float len = static_cast<float>(std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
                    if (len > st->maxDist[r][b])
                    {
                        st->maxDist[r][b] = len;
                        for (int k = 0; k < 3; ++k) st->maxVec[r][b][k] = static_cast<float>(d[k]);
                    }
                }
            }
        }
        else if (st->restSet && !st->evaluated && since >= 45)
        {
            st->evaluated = true;
            // the body: the bone the most others stayed rigid to
            int reference = -1, rigidBest = -1;
            for (int r = 0; r < 48; ++r)
            {
                if (!st->restUsable[r]) continue;
                int rigid = 0;
                for (int b = 0; b < 48; ++b)
                    if (b != r && st->restUsable[b] && st->pairSeen[r][b] && st->maxDist[r][b] < 0.02f) ++rigid;
                if (rigid > rigidBest) { rigidBest = rigid; reference = r; }
            }
            int forend = -1;
            if (reference >= 0)
                for (int b = 0; b < 48; ++b)
                {
                    const float d = st->maxDist[reference][b];
                    if (b != reference && st->restUsable[b] && st->pairSeen[reference][b] && d >= 0.03f && d <= 0.25f &&
                        (forend < 0 || d > st->maxDist[reference][forend]))
                        forend = b;
                }
            for (int r = 0; r < 6; ++r)
                Log("[PUMP] bone %d against bones 0-5: %.3f %.3f %.3f %.3f %.3f %.3f m", r, st->maxDist[r][0], st->maxDist[r][1],
                    st->maxDist[r][2], st->maxDist[r][3], st->maxDist[r][4], st->maxDist[r][5]);
            Log("[PUMP] weapon mesh %u bytes: body bone %d (%d bones rigid to it), sliding bone %d (%.3f m)", st->vbBytes,
                reference, rigidBest, forend, forend >= 0 ? st->maxDist[reference][forend] : 0.0);
            if (forend >= 0)
            {
                st->learned = true;
                st->reference = reference;
                st->bone = forend;
                st->range = st->maxDist[reference][forend];
                for (int k = 0; k < 3; ++k) st->back[k] = st->maxVec[reference][forend][k] / st->range;
                // bones riding with the forend: rigid to it and travelling like it
                for (int c = 0; c < 48; ++c)
                    st->rides[c] = c != forend && st->restUsable[c] && st->pairSeen[forend][c] && st->pairSeen[reference][c] && st->maxDist[forend][c] < 0.02f &&
                                   std::fabs(st->maxDist[reference][c] - st->range) < st->range * 0.25;
                Mat4 r = BoneMatrix(st->rest[0], 0), ri = {};
                for (int i = 0; i < 12; ++i) r.m[i] = st->rest[reference][i];
                Mat4 m = {};
                for (int i = 0; i < 12; ++i) m.m[i] = st->rest[forend][i];
                m.m[15] = 1.0;
                if (Invert(r, ri)) st->restRelative = Multiply(ri, m);
                Log("[ARMS] shotgun forend found: bone %d slides %.3f m against body bone %d", forend, st->range, reference);
            }
        }
    }
    if (!st->learned) return;
    const int rb = st->reference, fb = st->bone;
    if (!usable[rb] || !usable[fb]) return;
    const Mat4 body = BoneMatrix(bones, rb), forendNow = BoneMatrix(bones, fb);
    Mat4 bodyInverse = {};
    if (!Invert(body, bodyInverse)) return;
    const Mat4 relNow = Multiply(bodyInverse, forendNow);
    const double* bk = st->back;
    const double* rest = &st->restRelative.m[0];
    const double animDist = (relNow.m[3] - rest[3]) * bk[0] + (relNow.m[7] - rest[7]) * bk[1] + (relNow.m[11] - rest[11]) * bk[2];
    double backModel[3];
    for (int r = 0; r < 3; ++r) backModel[r] = body.m[r * 4] * bk[0] + body.m[r * 4 + 1] * bk[1] + body.m[r * 4 + 2] * bk[2];
    for (int r = 0; r < 3; ++r)
        g_pumpAnimView[r] = (toView.m[r * 4] * backModel[0] + toView.m[r * 4 + 1] * backModel[1] +
                             toView.m[r * 4 + 2] * backModel[2]) * animDist;
    g_pumpRange = st->range;
    g_pumpLearned = true;
    // the forend (and what rides on it) where the real hand has it
    double pull = g_twoHand ? g_slideGrab - g_slide : 0.0;
    if (pull < 0) pull = 0;
    if (pull > st->range) pull = st->range;
    Mat4 placedRel = st->restRelative;
    placedRel.m[3] += bk[0] * pull;
    placedRel.m[7] += bk[1] * pull;
    placedRel.m[11] += bk[2] * pull;
    const Mat4 forendNew = Multiply(body, placedRel);
    Mat4 forendNowInverse = {};
    const bool canCarry = Invert(forendNow, forendNowInverse);
    for (int c = 0; c < 48; ++c)
        if (st->rides[c] && usable[c] && canCarry)
            StoreBone(bones, c, Multiply(forendNew, Multiply(forendNowInverse, BoneMatrix(bones, c))));
    StoreBone(bones, fb, forendNew);
}

// Capture raw per-draw palettes through one firing animation. Bone slots are
// local to each draw, so a per-mesh bone-number heuristic is not authoritative.
#pragma pack(push, 4)
struct PumpTraceRecord
{
    UINT magic, frame, kind, vbBytes, stride, offset, start, count;
    unsigned long long bufferId, shaderHash;
    float world[12];
    float bones[48 * 12];
};
#pragma pack(pop)
static_assert(sizeof(PumpTraceRecord) == 2400, "pump trace layout");

void RecordPumpDraw(ID3D11DeviceContext* context, const float* bones,
                    const Mat4& world, UINT start, UINT count)
{
    static std::vector<PumpTraceRecord> idle, trace;
    static LONG idleFrame = -1, shotFrame = -1;
    static bool done = false;
    if (done) return;
    if (shotFrame < 0 && g_lastShotFrame >= 0)
    {
        shotFrame = g_lastShotFrame;
        trace = idle;
        Log("[PUMP TRACE] recording raw weapon draws for 100 frames after the shot");
    }
    if (shotFrame >= 0 && g_frame - shotFrame >= 100)
    {
        done = true;
        wchar_t path[MAX_PATH] = {};
        _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%sfc3_pump_trace.bin", fc3::DataFolder());
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        DWORD written = 0;
        const DWORD bytes = static_cast<DWORD>(trace.size() * sizeof(PumpTraceRecord));
        const bool ok = file != INVALID_HANDLE_VALUE && WriteFile(file, trace.data(), bytes, &written, nullptr) && written == bytes;
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        Log("[PUMP TRACE] %s: %zu raw draws, %lu bytes (fc3_pump_trace.bin)", ok ? "saved" : "SAVE FAILED", trace.size(), written);
        if (ok) fc3xr::Buzz(1);
        idle.clear(); trace.clear();
        return;
    }
    if (shotFrame < 0 && idleFrame != g_frame) { idle.clear(); idleFrame = g_frame; }
    auto& records = shotFrame < 0 ? idle : trace;
    if (records.size() >= 4096) return; // bounded even with unexpected draw traffic
    ID3D11Buffer* stream = nullptr;
    UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
    if (!stream) return;
    D3D11_BUFFER_DESC desc = {}; stream->GetDesc(&desc);
    PumpTraceRecord record = {};
    record.magic = 0x50545231;
    record.frame = g_frame; record.kind = bones ? 0 : 1;
    record.vbBytes = desc.ByteWidth; record.stride = stride; record.offset = offset;
    record.start = start; record.count = count;
    record.bufferId = reinterpret_cast<uintptr_t>(stream);
    stream->Release();
    ID3D11VertexShader* shader = nullptr;
    context->VSGetShader(&shader, nullptr, nullptr);
    record.shaderHash = ShaderHash(shader); SafeRelease(shader);
    for (int i = 0; i < 12; ++i) record.world[i] = static_cast<float>(world.m[i]);
    if (bones) std::memcpy(record.bones, bones, sizeof(record.bones));
    records.push_back(record);
}


// Measured from the shotgun's raw firing trace, not a travel heuristic:
// start 9633/count 6795 uses bone 3; start 4557/count 5076 uses bone 1.
// Both slide 0.09003 m along local -Y relative to bone 0, and return to it.
// Other moving bones belong to the receiver/strap and must stay untouched.
void DriveMeasuredForend(ID3D11DeviceContext* context, float* bones,
                         const Mat4& toView, UINT start, UINT count)
{
    if (!g_pumpEnabled) return;
    const int pumpBone = start == 9633 && count == 6795 ? 3 :
                         start == 4557 && count == 5076 ? 1 : -1;
    if (pumpBone < 0) return;
    ID3D11VertexShader* shader = nullptr;
    context->VSGetShader(&shader, nullptr, nullptr);
    const bool shaderMatches = ShaderHash(shader) == 0xA38F7CD233BCE12Bull;
    SafeRelease(shader);
    if (!shaderMatches) return;
    ID3D11Buffer* stream = nullptr;
    UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
    if (!stream) return;
    D3D11_BUFFER_DESC desc = {}; stream->GetDesc(&desc); stream->Release();
    if (desc.ByteWidth != 257896 || stride != 40 || offset != 336 ||
        !BoneUsable(bones) || !BoneUsable(bones + pumpBone * 12)) return;

    const Mat4 body = BoneMatrix(bones, 0);
    const Mat4 gamePump = BoneMatrix(bones, pumpBone);
    // Remove the game's own slide from the supporting hand as well. This is
    // measured before overriding the palette, in the same view frame as IK.
    const double animation[3] = { gamePump.m[3] - body.m[3],
                                 gamePump.m[7] - body.m[7],
                                 gamePump.m[11] - body.m[11] };
    for (int r = 0; r < 3; ++r)
        g_pumpAnimView[r] = toView.m[r * 4] * animation[0] +
                           toView.m[r * 4 + 1] * animation[1] +
                           toView.m[r * 4 + 2] * animation[2];
    g_pumpRange = 0.09;
    g_pumpLearned = true;
    double pull = g_twoHand ? g_slideGrab - g_slide : 0.0;
    pull = (std::max)(0.0, (std::min)(0.09, pull));
    // The forward/rest transform agrees with the body within 0.1 mm in the
    // capture. Replace the entire animated transform, including rotation.
    Mat4 controlled = body;
    for (int r = 0; r < 3; ++r) controlled.m[r * 4 + 3] -= body.m[r * 4 + 1] * pull;
    StoreBone(bones, pumpBone, controlled);
    static LONG seen = 0;
    if (InterlockedIncrement(&seen) <= 2)
        Log("[PUMP] measured foregrip override: start %u count %u bone %d; 9 cm local -Y; one-handed held forward", start, count, pumpBone);
}


// A skinned weapon piece: every bone gets the right hand's move, so the weapon
// stays in the hand exactly as the game posed it there.
ID3D11Buffer* BuildWeaponPalette(ID3D11DeviceContext* context, UINT start, UINT count)
{
    MarkReloadShotgun(context,start,count);
    CaptureReloadShell(context,start,count);
    ReplayReloadShell(context);
    if (!g_weaponPaletteCb) return nullptr;
    Mat4 handMove = {};
    ID3D11Buffer* bonesBuffer = nullptr;
    context->VSGetConstantBuffers(3, 1, &bonesBuffer);
    D3D11_BUFFER_DESC desc = {};
    if (bonesBuffer) bonesBuffer->GetDesc(&desc);
    SafeRelease(bonesBuffer);
    if (desc.ByteWidth != kBonesBytes) return nullptr;      // not skinned
    float bones[48 * 12] = {};
    Mat4 toView = {}, fromView = {};
    if (!DrawSpace(context, bones, toView, fromView)) return nullptr;
    ObserveShotgunRoot(bones,toView,start,count);
    if (!RightHandMove(handMove)) return nullptr;
    RecordPumpDraw(context, bones, toView, start, count); // raw, before hand movement
    DriveMeasuredForend(context, bones, toView, start, count);
    // The held-shell replay owns the physical shell. Hide the game's separate
    // loading-shell draws for all of manual mode, including after consumption
    // when mode/held have cleared but the native animation is still finishing.
    if((g_reload.mode||g_manualReloadEnabled)&&ReloadShellDraw(context,start,count))
    {
        MoveReloadShell(bones,fromView);for(int b=0;b<48;++b)bones[b*12+7]=-100;
        static LONG hidden=0;
        if(g_manualReloadEnabled&&InterlockedIncrement(&hidden)<=2)
            Log("[PHYSICAL RELOAD] original animated loading shell hidden (start %u count %u); held shell replay independent",start,count);
    }
    else MoveBones(bones, 0, 47, Multiply(fromView, Multiply(handMove, toView)));
    const bool own = g_own;
    g_own = true;
    RecordReloadPort(context,bones,start,count);
    RecordAnimationFinal(bones,2);
    g_oUpdateSubresource(context, g_weaponPaletteCb, 0, nullptr, bones, 0, 0);
    g_own = own;
    if (InterlockedIncrement(&g_weaponPaletteDraws) == 1)
        Log("[ARMS] weapon bones moved with the right hand for the first time");
    return g_weaponPaletteCb;
}

// A solid weapon piece: its $Globals with the world matrix (first three rows)
// moved by the right hand's move, uploaded into a buffer of the same size.
ID3D11Buffer* g_worldCbs[8] = {};
UINT g_worldCbSizes[8] = {};
LONG g_weaponWorldDraws = 0;

// ---- solid weapon pieces: the moving forend ----------------------------------
// Each solid piece of the held weapon is watched relative to the weapon's
// biggest piece (its body). After a shot the game's pump animation slides the
// forend piece(s) back and forward; those are found by their travel and from
// then on placed by the real left hand.
struct PieceState
{
    ID3D11Buffer* vb;
    UINT start, count;
    Mat4 world;
    LONG frame;
    bool restSet, forend;
    double rest[3], maxDist, maxVec[3], back[3];
    Mat4 restRelative;
};
PieceState g_pieces[96] = {};
int g_pieceCount = 0;
LONG g_piecesEvaluatedFor = -1;
bool g_solidForendFound = false;

void EvaluatePieces()
{
    double best = 0;
    for (int i = 0; i < g_pieceCount; ++i)
        if (g_pieces[i].restSet && g_pieces[i].maxDist < 0.35 && g_pieces[i].maxDist > best) best = g_pieces[i].maxDist;
    Log("[PUMP] %d solid weapon pieces watched; largest travel after the shot %.3f m", g_pieceCount, best);
    if (best < 0.02)
    {
        Log("[ARMS] no solid weapon piece slid after the shot (largest travel %.3f m)", best);
        return;
    }
    int found = 0;
    for (int i = 0; i < g_pieceCount; ++i)
    {
        PieceState& p = g_pieces[i];
        if (!p.restSet || p.maxDist < best * 0.6 || p.maxDist >= 0.35) continue;
        p.forend = true;
        for (int k = 0; k < 3; ++k) p.back[k] = p.maxVec[k] / p.maxDist;
        ++found;
        Log("[ARMS] forend piece: start %u count %u travels %.3f m", p.start, p.count, p.maxDist);
    }
    g_solidForendFound = found > 0;
    if (g_solidForendFound) g_pumpRange = best;
}

// Changes 'world' (the game's model -> world for this piece) when it is the forend.
void DriveForendPiece(ID3D11DeviceContext* context, UINT start, UINT count, Mat4& world, const Mat4& toView)
{
    ID3D11Buffer* stream = nullptr;
    UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
    if (!stream) return;
    PieceState* e = nullptr;
    for (int i = 0; i < g_pieceCount && !e; ++i)
        if (g_pieces[i].vb == stream && g_pieces[i].start == start && g_pieces[i].count == count) e = &g_pieces[i];
    if (!e && g_pieceCount < 96)
    {
        e = &g_pieces[g_pieceCount++];
        *e = {};
        e->vb = stream;
        e->start = start;
        e->count = count;
    }
    stream->Release();
    if (!e) return;
    e->world = world;
    e->frame = g_frame;
    // the weapon's body: the biggest piece seen this frame or the last
    PieceState* body = nullptr;
    for (int i = 0; i < g_pieceCount; ++i)
        if (g_frame - g_pieces[i].frame <= 1 && (!body || g_pieces[i].count > body->count)) body = &g_pieces[i];
    if (!body || body == e) return;
    Mat4 bodyInverse = {};
    if (!Invert(body->world, bodyInverse)) return;
    const Mat4 relative = Multiply(bodyInverse, world);
    const double t[3] = { relative.m[3], relative.m[7], relative.m[11] };
    const LONG since = g_frame - g_lastShotFrame;
    if (since > 90)
    {
        for (int k = 0; k < 3; ++k) e->rest[k] = t[k];
        e->restRelative = relative;
        e->restSet = true;
        if (!e->forend) e->maxDist = 0;
    }
    else if (e->restSet && since < 45 && !e->forend)
    {
        const double d[3] = { t[0] - e->rest[0], t[1] - e->rest[1], t[2] - e->rest[2] };
        const double len = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        if (len > e->maxDist) { e->maxDist = len; for (int k = 0; k < 3; ++k) e->maxVec[k] = d[k]; }
    }
    else if (since >= 45 && g_piecesEvaluatedFor != g_lastShotFrame && !g_solidForendFound)
    {
        g_piecesEvaluatedFor = g_lastShotFrame;
        EvaluatePieces();
    }
    if (!e->forend || !e->restSet) return;
    // the game's own slide (for the left hand), as a camera-frame vector
    const double animDist = (t[0] - e->rest[0]) * e->back[0] + (t[1] - e->rest[1]) * e->back[1] + (t[2] - e->rest[2]) * e->back[2];
    double backWorld[3];
    for (int r = 0; r < 3; ++r)
        backWorld[r] = body->world.m[r * 4] * e->back[0] + body->world.m[r * 4 + 1] * e->back[1] + body->world.m[r * 4 + 2] * e->back[2];
    // camera axes = toView rotation * world rotation^T
    for (int r = 0; r < 3; ++r)
    {
        double v = 0;
        for (int k = 0; k < 3; ++k)
        {
            double axis = 0;     // row r of (toView * world^T)
            for (int j = 0; j < 3; ++j) axis += toView.m[r * 4 + j] * world.m[k * 4 + j];
            v += axis * backWorld[k];
        }
        g_pumpAnimView[r] = v * animDist;
    }
    g_pumpLearned = true;
    // the forend where the real hand has it (forward when held in one hand)
    double pull = g_twoHand ? g_slideGrab - g_slide : 0.0;
    if (pull < 0) pull = 0;
    if (pull > e->maxDist) pull = e->maxDist;
    Mat4 placed = e->restRelative;
    placed.m[3] = e->rest[0] + e->back[0] * pull;
    placed.m[7] = e->rest[1] + e->back[1] * pull;
    placed.m[11] = e->rest[2] + e->back[2] * pull;
    world = Multiply(body->world, placed);
}

ID3D11Buffer* BuildWeaponWorld(ID3D11DeviceContext* context, UINT start, UINT count)
{
    Mat4 handMove = {};
    if (!RightHandMove(handMove)) return nullptr;
    ID3D11Buffer* globalsBuffer = nullptr;
    context->VSGetConstantBuffers(0, 1, &globalsBuffer);
    float globals[kSmallCbMax / 4] = {};
    const UINT bytes = ReadCopy(globalsBuffer, globals, sizeof(globals));
    D3D11_BUFFER_DESC desc = {};
    if (globalsBuffer) globalsBuffer->GetDesc(&desc);
    SafeRelease(globalsBuffer);
    if (bytes < 48 || bytes != desc.ByteWidth) return nullptr;
    Mat4 toView = {}, fromView = {};
    if (!DrawSpace(context, nullptr, toView, fromView)) return nullptr;
    // model -> world is the first three rows; the move in view space becomes
    // world' = W * (G^-1 D G) applied before: W' = W X with X in model space
    Mat4 world = {};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c) world.m[r * 4 + c] = globals[r * 4 + c];
    world.m[15] = 1.0;
    // the forend goes where the left hand has it (toView/fromView are for the
    // game's own world matrix, so the hand move is applied around that)
    Mat4 gameWorld = world;
    RecordPumpDraw(context, nullptr, world, start, count); // no guessed pump override
    Mat4 gameWorldInverse = {};
    Mat4 forendFix = {};
    forendFix.m[0] = forendFix.m[5] = forendFix.m[10] = forendFix.m[15] = 1.0;
    if (Invert(gameWorld, gameWorldInverse)) forendFix = Multiply(gameWorldInverse, world);
    const Mat4 moved = Multiply(gameWorld, Multiply(fromView, Multiply(handMove, Multiply(toView, forendFix))));
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c) globals[r * 4 + c] = static_cast<float>(moved.m[r * 4 + c]);
    int slot = -1;
    for (int i = 0; i < 8 && slot < 0; ++i)
        if (g_worldCbSizes[i] == bytes) slot = i;
    for (int i = 0; i < 8 && slot < 0; ++i)
        if (g_worldCbSizes[i] == 0)
        {
            D3D11_BUFFER_DESC cb = {};
            cb.ByteWidth = bytes;
            cb.Usage = D3D11_USAGE_DEFAULT;
            cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            if (SUCCEEDED(g_oCreateBuffer(g_dev, &cb, nullptr, &g_worldCbs[i]))) { g_worldCbSizes[i] = bytes; slot = i; }
            else break;
        }
    if (slot < 0) return nullptr;
    const bool own = g_own;
    g_own = true;
    g_oUpdateSubresource(context, g_worldCbs[slot], 0, nullptr, globals, 0, 0);
    g_own = own;
    if (InterlockedIncrement(&g_weaponWorldDraws) == 1)
        Log("[ARMS] solid weapon pieces moved with the right hand for the first time");
    return g_worldCbs[slot];
}

// ---- fingers --------------------------------------------------------------------
// Each finger joint is blended between the game's pose (curl 1: the hand as the
// game animates it, gripping) and the mesh's own rest pose (curl 0: straight
// fingers). A bone's skin matrix relative to its parent's is a rotation about
// the joint between them; scaling that rotation's angle bends the finger at
// that joint. No axis is guessed: each comes from the game's own pose.
struct FingerChain { int parent; int bones[3]; double joints[3][3]; };
const FingerChain kFingers[5] = {   // right arm bind space (metres); left arm = x mirrored
    { 2, { 3, 4, 5 }, { { 0.610, 0.065, 1.110 }, { 0.611, 0.091, 1.095 }, { 0.621, 0.103, 1.069 } } },     // thumb
    { 2, { 7, 8, 9 }, { { 0.658, 0.076, 1.079 }, { 0.682, 0.078, 1.043 }, { 0.687, 0.085, 1.018 } } },     // index
    { 2, { 10, 11, 12 }, { { 0.671, 0.050, 1.080 }, { 0.690, 0.053, 1.034 }, { 0.699, 0.057, 1.007 } } },  // middle
    { 13, { 14, 15, 16 }, { { 0.673, 0.020, 1.078 }, { 0.684, 0.024, 1.038 }, { 0.695, 0.024, 1.012 } } }, // ring
    { 13, { 17, 18, 19 }, { { 0.658, -0.003, 1.073 }, { 0.675, 0.001, 1.049 }, { 0.681, -0.003, 1.029 } } } };  // pinky

Mat4 BoneMatrix(const float* bones, int bone)
{
    Mat4 m = {};
    for (int i = 0; i < 12; ++i) m.m[i] = bones[bone * 12 + i];
    m.m[15] = 1.0;
    return m;
}

void StoreBone(float* bones, int bone, const Mat4& m)
{
    for (int i = 0; i < 12; ++i) bones[bone * 12 + i] = static_cast<float>(m.m[i]);
}

// rotation part of r scaled to 'amount' of its angle, about point j
Mat4 PartialTurn(const Mat4& r, const double j[3], double amount)
{
    const double trace = r.m[0] + r.m[5] + r.m[10];
    double c = (trace - 1.0) * 0.5;
    if (c > 1) c = 1;
    if (c < -1) c = -1;
    const double angle = std::acos(c);
    double a[3] = { r.m[9] - r.m[6], r.m[2] - r.m[8], r.m[4] - r.m[1] };
    const double len = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    Mat4 out = {};
    out.m[0] = out.m[5] = out.m[10] = out.m[15] = 1.0;
    if (angle < 1e-4 || len < 1e-9) return out;
    for (double& v : a) v /= len;
    const double t = angle * amount, s = std::sin(t), k = 1.0 - std::cos(t);
    const double rr[3][3] = {
        { 1 - k * (a[1] * a[1] + a[2] * a[2]), k * a[0] * a[1] - s * a[2], k * a[0] * a[2] + s * a[1] },
        { k * a[0] * a[1] + s * a[2], 1 - k * (a[0] * a[0] + a[2] * a[2]), k * a[1] * a[2] - s * a[0] },
        { k * a[0] * a[2] - s * a[1], k * a[1] * a[2] + s * a[0], 1 - k * (a[0] * a[0] + a[1] * a[1]) } };
    for (int i = 0; i < 3; ++i)
    {
        for (int k2 = 0; k2 < 3; ++k2) out.m[i * 4 + k2] = rr[i][k2];
        out.m[i * 4 + 3] = j[i] - (rr[i][0] * j[0] + rr[i][1] * j[1] + rr[i][2] * j[2]);
    }
    return out;
}

void CurlFingers(float* bones, int side)
{
    // the hand holding the gun keeps the game's grip
    if (side == 0 || g_twoHand) return;
    const fc3pad::Controllers c = fc3pad::Latest();
    if (!c.active) return;
    const double grip = c.leftGrip, trigger = c.leftTrigger;
    const double curl[5] = {
        c.leftThumbTouch ? 1.0 : 0.25,                                 // thumb
        c.leftIndexTouch ? 0.35 + 0.65 * trigger : 0.0,               // index
        grip, grip, grip };                                           // middle, ring, pinky
    const double mirror = side == 1 ? -1.0 : 1.0;
    for (int f = 0; f < 5; ++f)
    {
        const FingerChain& chain = kFingers[f];
        Mat4 parentOld = BoneMatrix(bones, chain.parent), parentNew = parentOld;
        for (int k = 0; k < 3; ++k)
        {
            const Mat4 childOld = BoneMatrix(bones, chain.bones[k]);
            Mat4 parentInverse = {};
            if (!Invert(parentOld, parentInverse)) break;
            const Mat4 relative = Multiply(parentInverse, childOld);
            const double joint[3] = { chain.joints[k][0] * mirror, chain.joints[k][1], chain.joints[k][2] };
            const Mat4 childNew = Multiply(parentNew, PartialTurn(relative, joint, curl[f]));
            StoreBone(bones, chain.bones[k], childNew);
            parentOld = childOld;
            parentNew = childNew;
        }
    }
}

float g_armOriginal[2][25 * 12] = {};
Mat4 g_armViewMoves[2][25] = {};
LONG g_armMovesFrame[2] = { -10, -10 };
ID3D11Buffer* g_bodyPaletteCb = nullptr;
LONG g_sleeveDraws = 0;

int ArmGroup(int bone)
{
    return (bone == 0 || bone >= 22) ? 0 : (bone == 1 || bone == 20 || bone == 21) ? 1 : 2;
}

// Jason's body mesh (shirt, sleeves) shares bones with the arm meshes. A body
// bone whose matrix equals one of an arm's bones this frame is that bone: it
// gets the same move, so the sleeve follows the arm. The body is drawn in the
// player's frame, the arms in the camera's: the move is turned by Q^T to match.
ID3D11Buffer* BuildBodyPalette(ID3D11DeviceContext* context)
{
    if (!g_bodyPaletteCb || !MotionInteractionEnabled() || !g_eyeViewsSet) return nullptr;
    ID3D11Buffer* bonesBuffer = nullptr;
    context->VSGetConstantBuffers(3, 1, &bonesBuffer);
    D3D11_BUFFER_DESC desc = {};
    if (bonesBuffer) bonesBuffer->GetDesc(&desc);
    SafeRelease(bonesBuffer);
    if (desc.ByteWidth != kBonesBytes) return nullptr;
    float bones[48 * 12] = {};
    Mat4 toView = {}, fromView = {};
    if (!DrawSpace(context, bones, toView, fromView)) return nullptr;
    Mat4 toBody = {};       // Q^T
    toBody.m[15] = 1.0;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) toBody.m[i * 4 + j] = g_aimActive ? g_aimQ[j][i] : (i == j ? 1.0 : 0.0);
    bool moved = false;
    for (int b = 0; b < 48; ++b)
    {
        const float* m = bones + b * 12;
        for (int side = 0; side < 2; ++side)
        {
            if (g_armMovesFrame[side] != g_frame) continue;
            int match = -1;
            for (int a = 0; a < 25 && match < 0; ++a)
            {
                const float* o = g_armOriginal[side] + a * 12;
                bool same = true;
                for (int k = 0; k < 12 && same; ++k) same = std::fabs(o[k] - m[k]) < 1e-5f;
                if (same) match = a;
            }
            if (match < 0) continue;
            const Mat4 x = Multiply(fromView, Multiply(toBody, Multiply(g_armViewMoves[side][match], toView)));
            MoveBones(bones, b, b, x);
            moved = true;
            break;
        }
    }
    if (!moved) return nullptr;
    const bool own = g_own;
    g_own = true;
    g_oUpdateSubresource(context, g_bodyPaletteCb, 0, nullptr, bones, 0, 0);
    g_own = own;
    if (InterlockedIncrement(&g_sleeveDraws) == 1)
        Log("[ARMS] body mesh bones (sleeves) moved with the arms for the first time");
    return g_bodyPaletteCb;
}

// The arm's bones, moved. Each bone of the game's palette takes a mesh point to
// where the game posed it (model space); the moves are rigid transforms in the
// first-person camera's view space. A view-space move D becomes the model-space
// move X = G^-1 (D (G m + g) - g), with v = G m + g the draw's model -> view
// mapping (world matrix, camera position, camera axes). Upper-arm bones take the
// upper-arm move, forearm bones the forearm move, hand and finger bones the
// hand move, so the game's own skinning draws a whole, unstretched arm and the
// fingers keep their grip.
// Neutral arm skin matrices and joints in body axes, owned by the mesh.
// Game animation is sampled once, then controller IK replaces it per draw.
UINT g_armPoseEpoch = 1;
struct StableArmPose
{
    UINT epoch;
    float bones[25 * 12];
    ArmJoints joints;
};
const GUID kStableArmGuid[2] = {
    { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 15 } },
    { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 16 } }
};
Mat4 ArmBodyToCamera()
{
    Mat4 q = {}; q.m[15] = 1;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) q.m[r * 4 + c] = g_aimActive ? g_aimQ[r][c] : (r == c ? 1.0 : 0.0);
    return q;
}
ArmJoints TransformArmJoints(const ArmJoints& source, const Mat4& transform)
{
    ArmJoints out = source;
    const double* points[3] = { source.s, source.e, source.w };
    double* targets[3] = { out.s, out.e, out.w };
    for (int i = 0; i < 3; ++i)
    {
        const V3 v = Apply(transform, { points[i][0], points[i][1], points[i][2] });
        targets[i][0] = v.x; targets[i][1] = v.y; targets[i][2] = v.z;
    }
    return out;
}
bool RestoreStableArmPose(ID3D11DeviceContext* context, int side, const Mat4& toView,
                          const Mat4& fromView, float* bones, ArmJoints& joints)
{
    ID3D11Buffer* stream = nullptr; UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
    if (!stream) return false;
    const Mat4 q = ArmBodyToCamera(); Mat4 qi = {};
    if (!Invert(q, qi)) { stream->Release(); return false; }
    StableArmPose pose = {}; UINT bytes = sizeof(pose);
    if (FAILED(stream->GetPrivateData(kStableArmGuid[side], &bytes, &pose)) || bytes != sizeof(pose) || pose.epoch != g_armPoseEpoch)
    {
        pose.epoch = g_armPoseEpoch;
        pose.joints = TransformArmJoints(joints, qi);
        const Mat4 toBody = Multiply(qi, toView);
        for (int b = 0; b < 25; ++b) StoreBone(pose.bones, b, Multiply(toBody, BoneMatrix(bones, b)));
        if (FAILED(stream->SetPrivateData(kStableArmGuid[side], sizeof(pose), &pose))) { stream->Release(); return false; }
        Log("[ARMS] %s neutral pose captured; game arm animation replaced by controller IK", side == 0 ? "right" : "left");
    }
    stream->Release();
    const Mat4 toModel = Multiply(fromView, q);
    for (int b = 0; b < 25; ++b) StoreBone(bones, b, Multiply(toModel, BoneMatrix(pose.bones, b)));
    joints = TransformArmJoints(pose.joints, q);
    return true;
}
Mat4 StableControllerHandMove(int side, V3 wrist)
{
    Mat4 hand = {}, reference = {};
    const float* controller = side == 0 ? g_handRight : g_handLeft;
    for (int i = 0; i < 16; ++i) { hand.m[i] = controller[i]; reference.m[i] = g_handRef[side][i]; }
    reference = Multiply(ArmBodyToCamera(), reference);
    return HandMove(hand, reference, wrist, side);
}

#include "captured_hand_control.inl"
#include "manual_reload.inl"
#include "machete_hold.inl"

ID3D11Buffer* BuildArmPalette(ID3D11DeviceContext* context, int side)
{
    const bool tracked = side == 0 ? g_handRightValid : g_handLeftValid;
    if (!MotionInteractionEnabled() || !g_eyeViewsSet || !tracked || !g_armJoints[side].valid || !g_armPaletteCb[side]) return nullptr;
    float bones[48 * 12] = {}; Mat4 toView = {}, fromView = {};
    if (!DrawSpace(context, bones, toView, fromView)) return nullptr;
    float original[25 * 12]; std::memcpy(original, bones, sizeof(original));
    ArmJoints joints = g_armJoints[side];
    // Preserve the confirmed right-hand weapon attachment. The free/right arm
    // in weapon-restricted areas, and the free left arm, use neutral IK.
    // While gripping, retain the game grip pose and the confirmed pump-slide correction.
    const bool manualLeft=side==1&&ManualReloadOwnsLeftHand();
    // Manual insertion retains the game's ammo events, but its left-arm
    // palette must stay on the same neutral/controller path as shell pickup.
    // Configured X reload fallbacks retain their original animated hand pose.
    const bool nativeReload=side==1&&!g_manualReloadEnabled&&ReloadPoseActive();
    const bool stable = side == 1 ? ((!g_twoHand||manualLeft)&&!nativeReload) : (MacheteHeld()||g_visibleWeaponDrawsLastFrame == 0);
    if (stable && !RestoreStableArmPose(context, side, toView, fromView, bones, joints)) return nullptr;
    Mat4 handMove = {};
    if (stable||nativeReload)
        handMove = StableControllerHandMove(side, { joints.w[0], joints.w[1], joints.w[2] });
    else if (!(side == 0 ? RightHandMove(handMove) : LeftMove(joints, g_armJoints[0], handMove))) return nullptr;
    Mat4 forearmMove = {}, upperMove = {};
    // Keep the tracked wrist exact during insertion, as the native reload path did.
    const bool manualInsert=manualLeft&&!g_reload.held&&!g_reloadAlignment;
    const bool holding = side == 1 && ((g_twoHand&&!manualLeft)||nativeReload||manualInsert);
    if (!SolveArm(joints, handMove, side, forearmMove, upperMove, holding || side == 0)) return nullptr;
    const Mat4 viewMoves[3] = { upperMove, forearmMove, handMove };
    for (int b = 0; b < 25; ++b)
    {
        const Mat4 desired = Multiply(viewMoves[ArmGroup(b)], Multiply(toView, BoneMatrix(bones, b)));
        StoreBone(bones, b, Multiply(fromView, desired));
        // Sleeves need the full animated -> controlled transform for each
        // matching bone, including the animation removed by the neutral pose.
        Mat4 inverseOriginal = {};
        const Mat4 game = Multiply(toView, BoneMatrix(original, b));
        Mat4 move = {}; move.m[0] = move.m[5] = move.m[10] = move.m[15] = 1;
        if (Invert(game, inverseOriginal)) move = Multiply(desired, inverseOriginal);
        g_armViewMoves[side][b] = move;
    }
    std::memcpy(g_armOriginal[side], original, sizeof(original));
    g_armMovesFrame[side] = g_frame;
    const bool macheteGrip=side==0&&MacheteGrip(context,bones);
    const bool capturedGrip=side==1&&CapturedReloadGrip(context,bones);
    if(!capturedGrip&&!macheteGrip&&!nativeReload)CurlFingers(bones, side);
    if(side==0&&macheteGrip)RecordMacheteHand(context,bones);
    else if(side==0&&MacheteHeld())g_machete.handFrame=-100000;
    if(side==1){if(capturedGrip)RecordReloadHand(context,bones);else g_reloadContact.hand=false;}
    const bool own = g_own; g_own = true;
    RecordAnimationFinal(bones,side);
    g_oUpdateSubresource(context, g_armPaletteCb[side], 0, nullptr, bones, 0, 0);
    g_own = own;
    if(side==0&&MacheteHeld())ReplayMachete(context);
    if(side==1&&capturedGrip)ReplayReloadShell(context); // shell uses this exact completed hand palette
    if (InterlockedIncrement(&g_armPaletteDraws) == 1)
        Log("[ARMS] arm bones rewritten for the first time (%s arm)", side == 0 ? "right" : "left");
    return g_armPaletteCb[side];
}


// Fills the constant buffers the patched shaders read, for the frame the game
// renders next. For each eye and each game camera (world, weapon):
//   eyeMatrix = eyeProjection * eyeView * inverse(gameProjection)
// takes whatever the game's shader produced to where that eye sees it.
// ---- aiming follows the gun ----------------------------------------------------
// The game aims where its camera points. With the look direction found in
// memory, the camera is pointed along the right controller every frame; the
// player's body is a separate frame (yaw only, turned with the right stick),
// and everything the headset reports relative to the body is re-expressed
// relative to the camera with Q (body axes in camera axes), so the view stays
// on the head while the camera follows the gun.
bool g_aimEnabled = true;
bool g_bodyValid = false;
double g_bodyYaw = 0, g_rightSign = 1;
double g_bodyAxes[3][3] = {};        // body right, up, forward (world)
bool g_aimActualThisFrame = false;
LARGE_INTEGER g_aimLast = {};

void AimFrame(bool headset)
{
    // Compensation outlives a temporary controller/weapon loss. Otherwise the
    // gun-driven native camera becomes the headset view during every dropout.
    g_aimActive = g_bodyValid && headset && g_eyeViewsSet && g_inGameplay;
    const bool want = g_aimEnabled && headset && MotionInteractionEnabled() && g_eyeViewsSet && g_handRightValid && g_inGameplay &&
                      g_frameCameraValid && fc3aim::Ready();
    if (!want)
    {
        fc3aim::Suspend();
        fc3pad::SetLookOverride(false, 0, 0);
        if (!headset || !g_inGameplay || !g_eyeViewsSet)
        {
            g_bodyValid = false;
            g_aimActive = false;
        }
        return;
    }
    const float* f = g_frameCamera.forward;
    const float* r = g_frameCamera.right;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (!g_bodyValid)
    {
        g_bodyYaw = std::atan2(static_cast<double>(f[1]), static_cast<double>(f[0]));
        // which way is "right" in this world (right = sign * forward x up)
        const double cx = f[1], cy = -f[0];
        g_rightSign = cx * r[0] + cy * r[1] >= 0 ? 1.0 : -1.0;
        g_bodyValid = true;
        g_aimLast = now;
        Log("[AIM] aiming follows the gun; turn with the right stick");
    }
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double dt = static_cast<double>(now.QuadPart - g_aimLast.QuadPart) / static_cast<double>(frequency.QuadPart);
    g_aimLast = now;
    if (dt > 0.1) dt = 0.1;
    // the stick turns the body; the game gets no look input of its own
    float sx = 0, sy = 0;
    fc3pad::LookStick(sx, sy);
    fc3pad::SetLookOverride(true, 0, 0);
    if (std::fabs(sx) > 0.2f)
    {
        const double amount = (std::fabs(sx) - 0.2) / 0.8 * (sx > 0 ? 1.0 : -1.0);
        g_bodyYaw -= g_rightSign * amount * 2.2 * dt;      // up to ~125 degrees a second
    }
    const double by = g_bodyYaw;
    const double body[3][3] = {     // rows: right, up, forward (world)
        { g_rightSign * std::sin(by), -g_rightSign * std::cos(by), 0 },
        { 0, 0, 1 },
        { std::cos(by), std::sin(by), 0 } };
    // gun direction: the controller's forward axis, body frame -> world
    double h[3] = { g_handBody[0][2], g_handBody[0][6], g_handBody[0][10] };
    if (g_twoHand)      // the two-handed gun direction, camera frame -> body frame (Q^T, last frame's)
        for (int k = 0; k < 3; ++k)
            h[k] = g_aimQ[0][k] * g_barrelCam[0] + g_aimQ[1][k] * g_barrelCam[1] + g_aimQ[2][k] * g_barrelCam[2];
    for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k) g_bodyAxes[i][k] = body[i][k];
    double d[3];
    for (int k = 0; k < 3; ++k) d[k] = body[0][k] * h[0] + body[1][k] * h[1] + body[2][k] * h[2];
    // Aim cone: the camera follows the gun only within 35 degrees of where the
    // head looks. The game draws only what its camera sees; pointing far away
    // from the view would cut the picture.
    {
        double hb[3] = { 0, 0, 0 };
        for (int eye = 0; eye < 2; ++eye)
            for (int k = 0; k < 3; ++k) hb[k] += g_eyeViewsBody[eye].view[8 + k];
        double hw[3];
        for (int k = 0; k < 3; ++k) hw[k] = body[0][k] * hb[0] + body[1][k] * hb[1] + body[2][k] * hb[2];
        const double hl = std::sqrt(hw[0] * hw[0] + hw[1] * hw[1] + hw[2] * hw[2]);
        const double dl = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        if (hl > 1e-6 && dl > 1e-6)
        {
            for (double& v : hw) v /= hl;
            for (double& v : d) v /= dl;
            const double cosAngle = (std::max)(-1.0, (std::min)(1.0, hw[0] * d[0] + hw[1] * d[1] + hw[2] * d[2]));
            const double angle = std::acos(cosAngle), cone = 35.0 * 3.14159265358979 / 180.0;
            if (angle > cone && std::sin(angle) > 1e-6)
            {
                const double t = cone / angle;
                const double a = std::sin((1 - t) * angle) / std::sin(angle), b = std::sin(t * angle) / std::sin(angle);
                for (int k = 0; k < 3; ++k) d[k] = a * hw[k] + b * d[k];
            }
        }
    }
    const double yaw = std::atan2(d[1], d[0]);
    double pitch = std::asin((std::max)(-1.0, (std::min)(1.0, d[2])));
    const double limit = 80.0 * 3.14159265358979 / 180.0;
    if (pitch > limit) pitch = limit;
    if (pitch < -limit) pitch = -limit;
    fc3aim::Drive(yaw, pitch);
    // the camera the game will render with
    const double cf[3] = { std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch) };
    // Queueing an aim request does not establish the rendered camera.
    // Use the observed camera until HandleDraw supplies this draw's actual axes.
    const float* observed[3] = { g_frameCamera.right, g_frameCamera.up, g_frameCamera.forward };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            g_aimQ[i][j] = observed[i][0]*body[j][0] + observed[i][1]*body[j][1] + observed[i][2]*body[j][2];
    g_aimActive = true;
    // how well the last prediction matched the camera the game really used
    static double lastPredicted[3] = {};
    static LONG checked = 0;
    static double worst = 0;
    if (checked > 0)
    {
        const double dot = lastPredicted[0] * f[0] + lastPredicted[1] * f[1] + lastPredicted[2] * f[2];
        const double err = std::acos((std::min)(1.0, dot)) * 57.29578;
        if (err > worst) worst = err;
    }
    std::memcpy(lastPredicted, cf, sizeof(cf));
    if (++checked % 1800 == 0)
    {
        Log("[AIM] camera follows the gun; worst mismatch to the predicted camera %.2f degrees", worst);
        worst = 0;
    }
}

#include "physical_reload_gesture.inl"
#include "reload_shell_visual.inl"

void UpdateEyeBuffers();

// The camera this frame is really drawn with (it may lag or smooth what was
// written): Q from its own axes, and the eye buffers rebuilt before the first
// world draw uses them. The view then matches the frame exactly.
void AimActualCamera(const CameraSummary& camera)
{
    if (!g_aimActive) return;
    const float* cam[3] = { camera.right, camera.up, camera.forward };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            g_aimQ[i][j] = cam[i][0] * g_bodyAxes[j][0] + cam[i][1] * g_bodyAxes[j][1] + cam[i][2] * g_bodyAxes[j][2];
    UpdateEyeBuffers();
    g_aimActualThisFrame = true;
}

// Re-expresses this frame's eye views and hands relative to the camera.
void ApplyAimToPoses()
{
    if (!g_aimActive || !g_eyeViewsSet) return;
    std::memcpy(g_eyeViews, g_eyeViewsBody, sizeof(g_eyeViews));
    if (g_handRightValid) std::memcpy(g_handRight, g_handBody[0], sizeof(g_handRight));
    if (g_handLeftValid) std::memcpy(g_handLeft, g_handBody[1], sizeof(g_handLeft));
    for (int eye = 0; eye < 2; ++eye)
    {
        // V maps body-space points to the eye; camera points are body points turned by Q
        float* v = g_eyeViews[eye].view;
        float out[16];
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                double sum = 0;
                for (int k = 0; k < 3; ++k) sum += v[i * 4 + k] * g_aimQ[j][k];     // V * Q^T
                out[i * 4 + j] = static_cast<float>(sum);
            }
            out[i * 4 + 3] = v[i * 4 + 3];
        }
        std::memcpy(v, out, sizeof(out));
    }
    float* hands[2] = { g_handRightValid ? g_handRight : nullptr, g_handLeftValid ? g_handLeft : nullptr };
    for (float* hm : hands)
    {
        if (!hm) continue;
        float out[16];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 4; ++j)
            {
                double sum = 0;
                for (int k = 0; k < 3; ++k) sum += g_aimQ[i][k] * hm[k * 4 + j];       // Q * H
                out[i * 4 + j] = static_cast<float>(sum);
            }
        for (int j = 0; j < 4; ++j) out[12 + j] = hm[12 + j];
        std::memcpy(hm, out, sizeof(out));
    }
}

void UpdateEyeBuffers()
{
    if (!g_ctx || !g_eyeCb[0][0]) return;
    ApplyAimToPoses();
    UpdateTwoHand();
    // Both projection axes come from the same observed camera sample.

    // Offline test: a fixed head pose (turned 10 degrees left, 5 up, leaning)
    // and an off-centre field of view, the same numbers tests/stereo_smoke.cpp uses.
    static const bool testView = GetEnvironmentVariableW(L"FC3_STEREO_TEST_VIEW", nullptr, 0) > 0;
    if (testView)
    {
        const double yaw = 10.0 * 0.0174532925, pitch = 5.0 * 0.0174532925;
        const double cy = std::cos(yaw), sy = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch);
        // eye-to-body rotation in game view axes (x right, y up, z forward)
        const double r[3][3] = {
            { cy, -sy * -sp, -sy * cp },
            { 0, cp, sp },
            { sy, cy * -sp, cy * cp } };
        const double lean[3] = { 0.10, 0.05, 0.20 };
        for (int eye = 0; eye < 2; ++eye)
        {
            const double side = eye == 0 ? -g_separation : g_separation;
            const double t[3] = { lean[0] + r[0][0] * side, lean[1] + r[1][0] * side, lean[2] + r[2][0] * side };
            float* m = g_eyeViews[eye].view;
            std::memset(m, 0, sizeof(float) * 16);
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j) m[i * 4 + j] = static_cast<float>(r[j][i]);
                m[i * 4 + 3] = static_cast<float>(-(r[0][i] * t[0] + r[1][i] * t[1] + r[2][i] * t[2]));
            }
            m[15] = 1.0f;
            g_eyeViews[eye].tanLeft = -1.1f;
            g_eyeViews[eye].tanRight = 0.9f;
            g_eyeViews[eye].tanUp = 0.8f;
            g_eyeViews[eye].tanDown = -1.0f;
        }
        g_eyeViewsSet = true;
    }

    for (int eye = 0; eye < 2; ++eye)
    {
        Mat4 view = {};
        view.m[0] = view.m[5] = view.m[10] = view.m[15] = 1.0;
        if (g_eyeViewsSet)
            for (int i = 0; i < 16; ++i) view.m[i] = g_eyeViews[eye].view[i];
        else
            view.m[3] = eye == 0 ? g_separation : -g_separation;    // left eye: the world sits to its right

        Mat4 worldMatrix = {};
        const Mat4 cameraView = view;
        Mat4 bodyView = cameraView;
        if (g_aimActive && g_eyeViewsSet)
            for (int i = 0; i < 16; ++i) bodyView.m[i] = g_eyeViewsBody[eye].view[i];
        for (int variant = 0; variant < kEyeVariants; ++variant)
        {
            view = variant == 5 ? bodyView : cameraView;
            const bool weapon = (variant == 1 || variant >= 3) && g_weaponProjectionValid;
            GameProjection game = weapon ? g_weaponProjection : g_worldProjection;
            if (std::fabs(game.b) < 1e-6f) game = g_worldProjection;
            const Mat4 projection = g_eyeViewsSet ? EyeProjectionMatrix(g_eyeViews[eye], g_worldProjection)
                                                  : ProjectionMatrix(game);
            Mat4 placed = view;
            Mat4 forearmPlaced = view, upperPlaced = view;
            const int handSide = variant == 4 ? 0 : variant == 3 ? 1 : -1;     // 0 right hand, 1 left hand
            const bool tracked = handSide == 0 ? g_handRightValid : handSide == 1 ? g_handLeftValid : false;
            if (weapon && handSide >= 0 && tracked && g_eyeViewsSet && MotionInteractionEnabled())
            {
                // the game's hand pose -> the controller -> the eye
                Mat4 hand = {}, reference = {}, toReference = {};
                const float* pose = handSide == 0 ? g_handRight : g_handLeft;
                for (int i = 0; i < 16; ++i)
                {
                    hand.m[i] = pose[i];
                    reference.m[i] = g_handRef[handSide][i];
                }
                (void)toReference;
                {
                    const ArmJoints& joints = g_armJoints[handSide];
                    // the game's wrist: measured this frame, else where the
                    // reference pose puts it
                    V3 gameWrist = { reference.m[3], reference.m[7], reference.m[11] };
                    if (joints.valid) gameWrist = { joints.w[0], joints.w[1], joints.w[2] };
                    else gameWrist = Apply(reference, V3{ g_wristOffset[handSide][0], g_wristOffset[handSide][1],
                                                          g_wristOffset[handSide][2] });
                    Mat4 handMove = HandMove(hand, reference, gameWrist, handSide);
                    if (handSide == 0) RightHandMove(handMove);
                    else LeftMove(g_armJoints[1], g_armJoints[0], handMove);
                    Mat4 forearmMove = {}, upperMove = {};
                    const bool solved = SolveArm(g_armJoints[handSide], handMove, handSide, forearmMove, upperMove);
                    // the gun follows the hand as the arm solve left it
                    placed = Multiply(view, handMove);
                    forearmPlaced = placed;
                    upperPlaced = placed;
                    if (solved)
                    {
                        forearmPlaced = Multiply(view, forearmMove);
                        upperPlaced = Multiply(view, upperMove);
                        if (eye == 0 && variant == 3) InterlockedIncrement(&g_armIkFrames);
                    }
                }
            }
            const Mat4 matrix = Multiply(projection, Multiply(placed, ProjectionInverse(game)));
            if (variant == 0) worldMatrix = matrix;

            float values[dxbc::kEyeRows * 4] = {};
            for (int i = 0; i < 16; ++i)
                values[i] = static_cast<float>(matrix.m[i] - ((i % 5) == 0 ? 1.0 : 0.0));
            // arm shader rows: [6..9] forearm, [10..13] upper arm (equal to the
            // hand when there is nothing to bend)
            const Mat4 forearmMatrix = Multiply(projection, Multiply(forearmPlaced, ProjectionInverse(game)));
            const Mat4 upperMatrix = Multiply(projection, Multiply(upperPlaced, ProjectionInverse(game)));
            for (int i = 0; i < 16; ++i)
            {
                values[24 + i] = static_cast<float>(forearmMatrix.m[i] - ((i % 5) == 0 ? 1.0 : 0.0));
                values[40 + i] = static_cast<float>(upperMatrix.m[i] - ((i % 5) == 0 ? 1.0 : 0.0));
            }
            if (variant == 2)
            {
                // Flat UI. On the monitor: pushed back to UiDepth by a sideways
                // offset. In the headset: a 16:9 panel UiWidthDeg wide, centred
                // straight ahead of the eye, at UiDepth.
                const float side = eye == 0 ? 1.0f : -1.0f;
                if (g_eyeViewsSet)
                {
                    const fc3stereo::EyeView& e = g_eyeViews[eye];
                    const float width = e.tanRight - e.tanLeft, height = e.tanUp - e.tanDown;
                    const float panel = std::tan(g_uiWidthDeg * 0.5f * 0.0174532925f);
                    const float aspect = g_bbW ? static_cast<float>(g_bbH) / static_cast<float>(g_bbW) : 0.5625f;
                    values[20] = panel / (width * 0.5f) - 1.0f;
                    values[21] = panel * aspect / (height * 0.5f) - 1.0f;
                    values[16] = -(e.tanRight + e.tanLeft) / width +
                                 (2.0f / width) * side * static_cast<float>(std::fabs(view.m[3])) / g_uiDepth;
                    values[17] = -(e.tanUp + e.tanDown) / height;
                }
                else
                {
                    values[16] = side * g_separation * g_worldProjection.xs / g_uiDepth;
                }
                // The crosshair: by default it sits with the rest of the UI ...
                values[56] = values[16]; values[57] = values[17];
                values[60] = values[20]; values[61] = values[21];
                // ... but in the headset it goes where the game camera points
                // (along the gun when aiming follows it), far out, at the angular
                // size the game draws it.
                if (g_eyeViewsSet)
                {
                    const fc3stereo::EyeView& e = g_eyeViews[eye];
                    const double distance = 15.0;
                    const double p[4] = { 0, 0, distance, 1 };
                    double v[3];
                    for (int i = 0; i < 3; ++i)
                        v[i] = view.m[i * 4 + 0] * p[0] + view.m[i * 4 + 1] * p[1] + view.m[i * 4 + 2] * p[2] + view.m[i * 4 + 3];
                    if (v[2] > 0.1)
                    {
                        const double width = e.tanRight - e.tanLeft, height = e.tanUp - e.tanDown;
                        const double tx = v[0] / v[2], ty = v[1] / v[2];
                        values[56] = static_cast<float>((2.0 * tx - (e.tanRight + e.tanLeft)) / width);
                        values[57] = static_cast<float>((2.0 * ty - (e.tanUp + e.tanDown)) / height);
                        // the crosshair's size as the game shows it at its normal
                        // 75-degree view (the widened view would blow it up)
                        const double normalXs = 1.303;
                        const double normalYs = normalXs * g_worldProjection.ys / g_worldProjection.xs;
                        values[60] = static_cast<float>((2.0 / width) / normalXs - 1.0);
                        values[61] = static_cast<float>((2.0 / height) / normalYs - 1.0);
                    }
                }
            }
            g_own = true;
            g_oUpdateSubresource(g_ctx, g_eyeCb[variant][eye], 0, nullptr, values, 0, 0);
            g_own = false;
        }

        // Pixel stage: columns of (inverse(eyeMatrix) - identity). Positions the
        // game's shaders rebuild from depth come out relative to ITS camera.
        Mat4 inverse = {};
        float columns[16] = {};
        if (Invert(worldMatrix, inverse))
            for (int column = 0; column < 4; ++column)
                for (int row = 0; row < 4; ++row)
                    columns[column * 4 + row] = static_cast<float>(inverse.m[row * 4 + column] - (row == column ? 1.0 : 0.0));
        g_own = true;
        g_oUpdateSubresource(g_ctx, g_eyePsCb[eye], 0, nullptr, columns, 0, 0);
        g_own = false;
    }
}

bool CreateDeviceObjects()
{
    ClearPhysicalHull();
    ClearReloadShell();
    ClearMacheteVisual();
    g_shotHand=ShotHandControl{};g_reloadContact=ReloadHandContact{};g_reloadPose=ReloadPoseCycle{};
    AnimationClose();
    for (int ui = 0; ui < kEyeVariants; ++ui)
        for (int eye = 0; eye < 2; ++eye)
            SafeRelease(g_eyeCb[ui][eye]);
    SafeRelease(g_blitVs);
    SafeRelease(g_blitPs);
    SafeRelease(g_blitSampler);
    SafeRelease(g_blitRaster);
    SafeRelease(g_leftCopySrv);
    SafeRelease(g_leftCopy);
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = dxbc::kEyeRows * 16;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    for (int ui = 0; ui < kEyeVariants; ++ui)
        for (int eye = 0; eye < 2; ++eye)
            if (FAILED(g_oCreateBuffer(g_dev, &desc, nullptr, &g_eyeCb[ui][eye])))
                return false;
    desc.ByteWidth = kBonesBytes;
    SafeRelease(g_weaponPaletteCb);
    if (FAILED(g_oCreateBuffer(g_dev, &desc, nullptr, &g_weaponPaletteCb)))
        return false;
    SafeRelease(g_bodyPaletteCb);
    if (FAILED(g_oCreateBuffer(g_dev, &desc, nullptr, &g_bodyPaletteCb)))
        return false;
    for (int side = 0; side < 2; ++side)
    {
        SafeRelease(g_armPaletteCb[side]);
        if (FAILED(g_oCreateBuffer(g_dev, &desc, nullptr, &g_armPaletteCb[side])))
            return false;
    }
    desc.ByteWidth = 64;
    for (int eye = 0; eye < 2; ++eye)
    {
        SafeRelease(g_eyePsCb[eye]);
        if (FAILED(g_oCreateBuffer(g_dev, &desc, nullptr, &g_eyePsCb[eye])))
            return false;
    }

    // Monitor side-by-side view. Compiled at run time; optional.
    using CompileFn = HRESULT (WINAPI*)(LPCVOID, SIZE_T, LPCSTR, const void*, void*, LPCSTR,
                                        LPCSTR, UINT, UINT, ID3D10Blob**, ID3D10Blob**);
    HMODULE compiler = LoadLibraryW(L"d3dcompiler_47.dll");
    CompileFn compile = compiler
        ? reinterpret_cast<CompileFn>(GetProcAddress(compiler, "D3DCompile")) : nullptr;
    if (compile)
    {
        static const char source[] =
            "struct V{float4 p:SV_Position;float2 uv:TEXCOORD0;};"
            "V vs(uint id:SV_VertexID){V o;o.uv=float2((id<<1)&2,id&2);"
            "o.p=float4(o.uv*float2(2,-2)+float2(-1,1),0,1);return o;}"
            "Texture2D t:register(t0);SamplerState s:register(s0);"
            "float4 ps(V i):SV_Target{return t.Sample(s,i.uv);}";
        ID3D10Blob* vs = nullptr;
        ID3D10Blob* ps = nullptr;
        compile(source, sizeof(source) - 1, nullptr, nullptr, nullptr, "vs", "vs_4_0", 0, 0, &vs, nullptr);
        compile(source, sizeof(source) - 1, nullptr, nullptr, nullptr, "ps", "ps_4_0", 0, 0, &ps, nullptr);
        if (vs && ps)
        {
            g_oCreateVertexShader(g_dev, vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &g_blitVs);
            g_oCreatePixelShader(g_dev, ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &g_blitPs);
        }
        if (vs) vs->Release();
        if (ps) ps->Release();
        D3D11_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler.MaxLOD = D3D11_FLOAT32_MAX;
        g_dev->CreateSamplerState(&sampler, &g_blitSampler);
        D3D11_RASTERIZER_DESC raster = {};
        raster.FillMode = D3D11_FILL_SOLID;
        raster.CullMode = D3D11_CULL_NONE;
        raster.DepthClipEnable = TRUE;
        g_dev->CreateRasterizerState(&raster, &g_blitRaster);
    }
    Log("[INIT] eye buffers ready, monitor side-by-side view %s",
        (g_blitVs && g_blitPs && g_blitSampler && g_blitRaster) ? "available" : "UNAVAILABLE");
    return true;
}

// ---- monitor side-by-side view ----------------------------------------------
void DrawSideBySide(ID3D11Texture2D* backbuffer, ID3D11Texture2D* rightEye)
{
    if (!g_blitVs || !g_blitPs || !g_blitSampler || !g_blitRaster) return;
    D3D11_TEXTURE2D_DESC desc = {};
    backbuffer->GetDesc(&desc);
    if (desc.SampleDesc.Count != 1) return;

    if (!g_leftCopy || g_leftCopyW != desc.Width || g_leftCopyH != desc.Height ||
        g_leftCopyFormat != desc.Format)
    {
        SafeRelease(g_leftCopySrv);
        SafeRelease(g_leftCopy);
        D3D11_TEXTURE2D_DESC copy = desc;
        copy.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        copy.MiscFlags = 0;
        copy.Usage = D3D11_USAGE_DEFAULT;
        copy.CPUAccessFlags = 0;
        if (FAILED(g_dev->CreateTexture2D(&copy, nullptr, &g_leftCopy))) return;
        if (FAILED(g_dev->CreateShaderResourceView(g_leftCopy, nullptr, &g_leftCopySrv))) return;
        g_leftCopyW = desc.Width;
        g_leftCopyH = desc.Height;
        g_leftCopyFormat = desc.Format;
    }

    ID3D11ShaderResourceView* rightSrv = nullptr;
    ID3D11RenderTargetView* target = nullptr;
    if (FAILED(g_dev->CreateShaderResourceView(rightEye, nullptr, &rightSrv)) ||
        FAILED(g_dev->CreateRenderTargetView(backbuffer, nullptr, &target)))
    {
        SafeRelease(rightSrv);
        SafeRelease(target);
        return;
    }

    ID3D11DeviceContext* c = g_ctx;
    g_own = true;
    g_oCopyResource(c, g_leftCopy, backbuffer);

    // save the state this draw touches
    ID3D11RenderTargetView* oldRtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView* oldDsv = nullptr;
    c->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, oldRtv, &oldDsv);
    ID3D11BlendState* oldBlend = nullptr; FLOAT oldFactor[4]; UINT oldMask = 0;
    c->OMGetBlendState(&oldBlend, oldFactor, &oldMask);
    ID3D11DepthStencilState* oldDepth = nullptr; UINT oldStencil = 0;
    c->OMGetDepthStencilState(&oldDepth, &oldStencil);
    ID3D11RasterizerState* oldRaster = nullptr;
    c->RSGetState(&oldRaster);
    D3D11_VIEWPORT oldViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    UINT oldViewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    c->RSGetViewports(&oldViewportCount, oldViewports);
    ID3D11InputLayout* oldLayout = nullptr;
    c->IAGetInputLayout(&oldLayout);
    D3D11_PRIMITIVE_TOPOLOGY oldTopology;
    c->IAGetPrimitiveTopology(&oldTopology);
    ID3D11VertexShader* oldVs = nullptr; ID3D11PixelShader* oldPs = nullptr;
    ID3D11GeometryShader* oldGs = nullptr; ID3D11HullShader* oldHs = nullptr;
    ID3D11DomainShader* oldDs = nullptr;
    c->VSGetShader(&oldVs, nullptr, nullptr);
    c->PSGetShader(&oldPs, nullptr, nullptr);
    c->GSGetShader(&oldGs, nullptr, nullptr);
    c->HSGetShader(&oldHs, nullptr, nullptr);
    c->DSGetShader(&oldDs, nullptr, nullptr);
    ID3D11ShaderResourceView* oldSrv = nullptr;
    c->PSGetShaderResources(0, 1, &oldSrv);
    ID3D11SamplerState* oldSampler = nullptr;
    c->PSGetSamplers(0, 1, &oldSampler);

    c->OMSetRenderTargets(1, &target, nullptr);
    c->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    c->OMSetDepthStencilState(nullptr, 0);
    c->RSSetState(g_blitRaster);
    c->IASetInputLayout(nullptr);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->VSSetShader(g_blitVs, nullptr, 0);
    c->PSSetShader(g_blitPs, nullptr, 0);
    c->GSSetShader(nullptr, nullptr, 0);
    c->HSSetShader(nullptr, nullptr, 0);
    c->DSSetShader(nullptr, nullptr, 0);
    c->PSSetSamplers(0, 1, &g_blitSampler);
    for (int eye = 0; eye < 2; ++eye)
    {
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = eye ? desc.Width * 0.5f : 0.0f;
        viewport.Width = desc.Width * 0.5f;
        viewport.Height = static_cast<float>(desc.Height);
        viewport.MaxDepth = 1.0f;
        c->RSSetViewports(1, &viewport);
        c->PSSetShaderResources(0, 1, eye ? &rightSrv : &g_leftCopySrv);
        g_oDraw(c, 3, 0);
    }

    c->PSSetShaderResources(0, 1, &oldSrv);
    c->PSSetSamplers(0, 1, &oldSampler);
    c->VSSetShader(oldVs, nullptr, 0);
    c->PSSetShader(oldPs, nullptr, 0);
    c->GSSetShader(oldGs, nullptr, 0);
    c->HSSetShader(oldHs, nullptr, 0);
    c->DSSetShader(oldDs, nullptr, 0);
    c->IASetPrimitiveTopology(oldTopology);
    c->IASetInputLayout(oldLayout);
    c->RSSetViewports(oldViewportCount, oldViewports);
    c->RSSetState(oldRaster);
    c->OMSetDepthStencilState(oldDepth, oldStencil);
    c->OMSetBlendState(oldBlend, oldFactor, oldMask);
    c->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, oldRtv, oldDsv);
    g_own = false;

    for (ID3D11RenderTargetView*& view : oldRtv) SafeRelease(view);
    SafeRelease(oldDsv); SafeRelease(oldBlend); SafeRelease(oldDepth); SafeRelease(oldRaster);
    SafeRelease(oldLayout); SafeRelease(oldVs); SafeRelease(oldPs); SafeRelease(oldGs);
    SafeRelease(oldHs); SafeRelease(oldDs); SafeRelease(oldSrv); SafeRelease(oldSampler);
    SafeRelease(rightSrv);
    SafeRelease(target);
}

// ---- eye dump and measurement -----------------------------------------------
// Reads an eye image back as half-resolution luminance and writes a BMP.
bool ReadEye(ID3D11Texture2D* texture, const wchar_t* fileName, std::vector<uint8_t>& luma,
             UINT& outW, UINT& outH)
{
    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);
    if (desc.SampleDesc.Count != 1) return false;
    const bool bgra = desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM ||
        desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB || desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM ||
        desc.Format == DXGI_FORMAT_B8G8R8A8_TYPELESS;
    const bool rgba = desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM ||
        desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || desc.Format == DXGI_FORMAT_R8G8B8A8_TYPELESS;
    const bool rgb10 = desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM;
    if (!bgra && !rgba && !rgb10) return false;

    D3D11_TEXTURE2D_DESC staging = desc;
    staging.Usage = D3D11_USAGE_STAGING;
    staging.BindFlags = 0;
    staging.MiscFlags = 0;
    staging.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* copy = nullptr;
    if (FAILED(g_dev->CreateTexture2D(&staging, nullptr, &copy))) return false;
    g_oCopyResource(g_ctx, copy, texture);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(g_oMap(g_ctx, copy, 0, D3D11_MAP_READ, 0, &mapped)))
    {
        copy->Release();
        return false;
    }

    outW = desc.Width / 2;
    outH = desc.Height / 2;
    const UINT stride = (outW * 3 + 3) & ~3u;
    std::vector<uint8_t> pixels(static_cast<size_t>(stride) * outH);
    luma.assign(static_cast<size_t>(outW) * outH, 0);
    for (UINT y = 0; y < outH; ++y)
    {
        const uint8_t* row = static_cast<const uint8_t*>(mapped.pData) + static_cast<size_t>(mapped.RowPitch) * (y * 2);
        uint8_t* out = pixels.data() + static_cast<size_t>(stride) * (outH - 1 - y);
        for (UINT x = 0; x < outW; ++x)
        {
            uint8_t r, g, b;
            if (rgb10)
            {
                uint32_t v;
                std::memcpy(&v, row + 8 * x, 4);
                r = static_cast<uint8_t>((v & 0x3ff) >> 2);
                g = static_cast<uint8_t>(((v >> 10) & 0x3ff) >> 2);
                b = static_cast<uint8_t>(((v >> 20) & 0x3ff) >> 2);
            }
            else
            {
                const uint8_t* p = row + 8 * x;
                r = bgra ? p[2] : p[0];
                g = p[1];
                b = bgra ? p[0] : p[2];
            }
            out[3 * x] = b; out[3 * x + 1] = g; out[3 * x + 2] = r;
            luma[static_cast<size_t>(y) * outW + x] = static_cast<uint8_t>((r * 54 + g * 183 + b * 19) >> 8);
        }
    }
    g_oUnmap(g_ctx, copy, 0);
    copy->Release();

    wchar_t path[MAX_PATH] = {};
    lstrcpynW(path, fc3::DataFolder(), MAX_PATH);
    if (lstrlenW(path) + lstrlenW(fileName) + 1 < MAX_PATH)
    {
        lstrcatW(path, fileName);
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE)
        {
            BITMAPFILEHEADER header = {};
            BITMAPINFOHEADER info = {};
            header.bfType = 0x4D42;
            header.bfOffBits = sizeof(header) + sizeof(info);
            header.bfSize = header.bfOffBits + static_cast<DWORD>(pixels.size());
            info.biSize = sizeof(info);
            info.biWidth = static_cast<LONG>(outW);
            info.biHeight = static_cast<LONG>(outH);
            info.biPlanes = 1;
            info.biBitCount = 24;
            DWORD written = 0;
            WriteFile(file, &header, sizeof(header), &written, nullptr);
            WriteFile(file, &info, sizeof(info), &written, nullptr);
            WriteFile(file, pixels.data(), static_cast<DWORD>(pixels.size()), &written, nullptr);
            CloseHandle(file);
        }
    }
    return true;
}

// Mean absolute luminance difference between the eyes over a horizontal band,
// with the right eye slid by dx pixels.
double BandDifference(const std::vector<uint8_t>& left, const std::vector<uint8_t>& right,
                      UINT w, UINT y0, UINT y1, int dx)
{
    unsigned long long sum = 0;
    unsigned long long count = 0;
    const int margin = 80;
    for (UINT y = y0; y < y1; y += 2)
    {
        const uint8_t* l = left.data() + static_cast<size_t>(y) * w;
        const uint8_t* r = right.data() + static_cast<size_t>(y) * w;
        for (int x = margin; x < static_cast<int>(w) - margin; x += 2)
        {
            const int d = static_cast<int>(l[x]) - static_cast<int>(r[x + dx]);
            sum += static_cast<unsigned>(d < 0 ? -d : d);
            ++count;
        }
    }
    return count ? static_cast<double>(sum) / static_cast<double>(count) : 0.0;
}

void DumpEyes(ID3D11Texture2D* leftEye, ID3D11Texture2D* rightEye)
{
    static int dumpIndex = 0;
    ++dumpIndex;
    wchar_t leftName[48] = {}, rightName[48] = {};
    _snwprintf_s(leftName, 48, _TRUNCATE, L"fc3_eye_left_%d.bmp", dumpIndex);
    _snwprintf_s(rightName, 48, _TRUNCATE, L"fc3_eye_right_%d.bmp", dumpIndex);
    std::vector<uint8_t> left, right;
    UINT w = 0, h = 0, w2 = 0, h2 = 0;
    g_own = true;
    const bool ok = ReadEye(leftEye, leftName, left, w, h) &&
                    ReadEye(rightEye, rightName, right, w2, h2);
    g_own = false;
    if (!ok || w != w2 || h != h2 || w < 400)
    {
        Log("[DUMP] could not read the eye images");
        return;
    }
    // Per band: difference with the eyes overlaid, and the horizontal slide that
    // matches them best. Near things (bottom of the screen) should need a larger
    // slide than far things (top). Units: half-resolution pixels.
    static const char* names[3] = { "top", "middle", "bottom" };
    for (UINT band = 0; band < 3; ++band)
    {
        const UINT y0 = h * band / 3;
        const UINT y1 = h * (band + 1) / 3;
        const double overlaid = BandDifference(left, right, w, y0, y1, 0);
        double best = overlaid;
        int bestDx = 0;
        for (int dx = -72; dx <= 72; ++dx)
        {
            const double difference = BandDifference(left, right, w, y0, y1, dx);
            if (difference < best)
            {
                best = difference;
                bestDx = dx;
            }
        }
        Log("[DUMP] %-6s band: eyes overlaid differ by %.2f, best match at slide %+d px (differ by %.2f)",
            names[band], overlaid, bestDx, best);
    }
    Log("[DUMP] wrote eye pair #%d (%ux%u) at frame %ld", dumpIndex, w, h, g_frame);
}

// The game advances motion by the previous frame's duration. Doubled draw work
// makes frame times uneven, which shows as judder while walking. Holding an
// even cadence removes it.
void PaceFrame()
{
    static LARGE_INTEGER frequency = {};
    static LONGLONG next = 0;
    if (g_fpsCap < 1.0f || !g_enabled) { next = 0; return; }
    if (!frequency.QuadPart) QueryPerformanceFrequency(&frequency);
    const LONGLONG interval = static_cast<LONGLONG>(frequency.QuadPart / g_fpsCap);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (next == 0 || now.QuadPart > next + interval * 4)
        next = now.QuadPart;        // first frame, or fell far behind: restart the cadence
    while (now.QuadPart < next)
    {
        const LONGLONG remaining = next - now.QuadPart;
        if (remaining * 1000 / frequency.QuadPart > 2) Sleep(1);
        else YieldProcessor();
        QueryPerformanceCounter(&now);
    }
    next += interval;
}

bool KeyPressed(int key)
{
    static SHORT previous[256] = {};
    const SHORT state = GetAsyncKeyState(key);
    const bool pressed = (state & 0x8000) && !(previous[key & 0xff] & 0x8000);
    previous[key & 0xff] = state;
    return pressed;
}

void InstallOne(void* target, void* detour, void** original, const char* name, int& installed, int& total)
{
    ++total;
    const MH_STATUS create = MH_CreateHook(target, detour, original);
    const MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : create;
    if (create == MH_OK && enable == MH_OK)
        ++installed;
    else
        Log("[HOOK] FAILED %s create=%d enable=%d", name, static_cast<int>(create), static_cast<int>(enable));
}
}

namespace fc3stereo
{
void OnDeviceCreated(ID3D11Device* device)
{
    if (!device || InterlockedCompareExchange(&g_hooksInstalled, 1, 0) != 0)
        return;

    const MH_STATUS init = MH_Initialize();
    if (init != MH_OK && init != MH_ERROR_ALREADY_INITIALIZED)
    {
        Log("[HOOK] MinHook init failed %d", static_cast<int>(init));
        return;
    }
    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);
    if (!context)
        return;

    void** cv = *reinterpret_cast<void***>(context);
    void** dv = *reinterpret_cast<void***>(device);
    int installed = 0, total = 0;
#define HOOK(table, slot, detour, original) \
    InstallOne(table[slot], reinterpret_cast<void*>(&detour), reinterpret_cast<void**>(&original), #detour, installed, total)
    HOOK(cv, 12, HookDrawIndexed, g_oDrawIndexed);
    HOOK(cv, 13, HookDraw, g_oDraw);
    HOOK(cv, 20, HookDrawIndexedInstanced, g_oDrawIndexedInstanced);
    HOOK(cv, 21, HookDrawInstanced, g_oDrawInstanced);
    HOOK(cv, 14, HookMap, g_oMap);
    HOOK(cv, 15, HookUnmap, g_oUnmap);
    HOOK(cv, 48, HookUpdateSubresource, g_oUpdateSubresource);
    HOOK(cv, 46, HookCopySubresourceRegion, g_oCopySubresourceRegion);
    HOOK(cv, 47, HookCopyResource, g_oCopyResource);
    HOOK(cv, 50, HookClearRtv, g_oClearRtv);
    HOOK(cv, 53, HookClearDsv, g_oClearDsv);
    HOOK(cv, 54, HookGenerateMips, g_oGenerateMips);
    HOOK(cv, 57, HookResolve, g_oResolve);
    HOOK(cv, 41, HookDispatch, g_oDispatch);
    HOOK(dv, 3, HookCreateBuffer, g_oCreateBuffer);
    // (no CreateInputLayout hook: the build that added one hard-froze the PC at startup;
    //  the mesh layout is worked out offline from the shader signature instead)
    HOOK(dv, 12, HookCreateVertexShader, g_oCreateVertexShader);
    HOOK(dv, 15, HookCreatePixelShader, g_oCreatePixelShader);
#undef HOOK
    context->Release();
    Log("[HOOK] stereo hooks installed=%d/%d", installed, total);
    fc3pad::Install();
    fc3di::Install();
    LoadSettings();
}

void OnPrimarySwapChain(IDXGISwapChain* swapChain, ID3D11Device* device)
{
    if (!swapChain || !device || !g_oCreateBuffer)
        return;
    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);
    if (!context)
        return;
    context->Release();     // the device keeps its immediate context alive
    const bool changed = device != g_dev;
    g_dev = device;
    g_ctx = context;
    if (changed && !CreateDeviceObjects())
        Log("[INIT] FAILED to create the eye buffers; stereo will not shift anything");
    Log("[INIT] game device=%p context=%p", static_cast<void*>(device), static_cast<void*>(context));

    // test hook: record the first few frames without needing gameplay
    if (GetEnvironmentVariableW(L"FC3_STEREO_CENSUS", nullptr, 0) > 0)
        g_censusArmed = 3;
}

void CopyTexture(ID3D11Texture2D* destination, ID3D11Texture2D* source)
{
    if (!g_ctx || !g_oCopyResource || !destination || !source) return;
    g_own = true;
    g_oCopyResource(g_ctx, destination, source);
    g_own = false;
}

float MainXScale() { return g_mainXScale; }

void SetHandPoses(const float* right, const float* left)
{
    if (fc3pad::TakeCalibration() && right && left)
    {
        ++g_armPoseEpoch; // recalibration also refreshes neutral mesh poses
        std::memcpy(g_handRef[0], right, sizeof(g_handRef[0]));
        std::memcpy(g_handRef[1], left, sizeof(g_handRef[1]));
        for (int side = 0; side < 2; ++side)
        {
            const float* c = side == 0 ? right : left;
            const ArmJoints& j = g_armJoints[side];
            if (!j.valid) continue;
            const double d[3] = { j.w[0] - c[3], j.w[1] - c[7], j.w[2] - c[11] };
            for (int k = 0; k < 3; ++k)
                g_wristOffset[side][k] = c[0 + k] * d[0] + c[4 + k] * d[1] + c[8 + k] * d[2];
            Log("[ARMS] %s wrist sits %.2f %.2f %.2f m from the controller (its own axes)", side == 0 ? "right" : "left",
                g_wristOffset[side][0], g_wristOffset[side][1], g_wristOffset[side][2]);
        }
        Log("[ARMS] calibrated: right hand %.2f %.2f %.2f, left hand %.2f %.2f %.2f (game axes, m)",
            right[3], right[7], right[11], left[3], left[7], left[11]);
    }
    g_handRightValid = right != nullptr;
    g_handLeftValid = left != nullptr;
    if (right) { std::memcpy(g_handRight, right, sizeof(g_handRight)); std::memcpy(g_handBody[0], right, sizeof(g_handBody[0])); }
    if (left) { std::memcpy(g_handLeft, left, sizeof(g_handLeft)); std::memcpy(g_handBody[1], left, sizeof(g_handBody[1])); }
}

void SetEyeViews(const EyeView* eyes)
{
    g_eyeViewsSet = eyes != nullptr;
    if (eyes)
    {
        std::memcpy(g_eyeViews, eyes, sizeof(g_eyeViews));
        std::memcpy(g_eyeViewsBody, eyes, sizeof(g_eyeViewsBody));
    }
}
bool InGameplay() { return g_inGameplay; }
bool HeadTrackingEnabled() { return g_headTracking; }
float WorldScale() { return g_worldScale; }
int PacePeriods() { return g_pacePeriods; }

bool FrameCamera(float right[3], float up[3], float forward[3])
{
    if (!g_frameCameraValid) return false;
    std::memcpy(right, g_frameCamera.right, sizeof(float) * 3);
    std::memcpy(up, g_frameCamera.up, sizeof(float) * 3);
    std::memcpy(forward, g_frameCamera.forward, sizeof(float) * 3);
    return true;
}
float EyeSeparation() { return g_separation; }
void SetEyeSeparation(float halfDistanceMetres) { g_separation = halfDistanceMetres * g_worldScale; }

void OnPresent(IDXGISwapChain* swapChain)
{
    if (!swapChain || !g_dev || !g_ctx)
        return;

    ID3D11Texture2D* backbuffer = nullptr;
    if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backbuffer))) || !backbuffer)
        return;
    D3D11_TEXTURE2D_DESC desc = {};
    backbuffer->GetDesc(&desc);
    g_bbW = desc.Width;
    g_bbH = desc.Height;
    if (!HasFlag(backbuffer, kBackbufferGuid))
        SetFlag(backbuffer, kBackbufferGuid, 1);

    // hotkeys, only while the game window has focus
    DXGI_SWAP_CHAIN_DESC chain = {};
    swapChain->GetDesc(&chain);
    if (GetForegroundWindow() == chain.OutputWindow)
    {
        // Numpad only. The game uses the F keys.
        if (KeyPressed(VK_NUMPAD7)) { g_enabled = !g_enabled; Log("[KEY] stereo %s", g_enabled ? "ON" : "OFF"); }
        if (KeyPressed(VK_NUMPAD9)) { g_view = g_view ? 0 : 1; Log("[KEY] monitor view = %s", g_view ? "side by side" : "left eye"); }
        if (fc3xr::Running())
        {
            // In the headset the eye distance comes from the headset; the keys scale it.
            if (KeyPressed(VK_NUMPAD4)) { g_worldScale = g_worldScale > 0.15f ? g_worldScale - 0.05f : 0.1f; Log("[KEY] world scale = %.2f", g_worldScale); }
            if (KeyPressed(VK_NUMPAD6)) { g_worldScale += 0.05f; Log("[KEY] world scale = %.2f", g_worldScale); }
        }
        else
        {
            if (KeyPressed(VK_NUMPAD4)) { g_separation = g_separation > 0.004f ? g_separation - 0.004f : 0.0f; Log("[KEY] separation = %.4f", g_separation); }
            if (KeyPressed(VK_NUMPAD6)) { g_separation += 0.004f; Log("[KEY] separation = %.4f", g_separation); }
        }
        if (KeyPressed(VK_NUMPAD1)) g_dumpRequested = true;
        if (KeyPressed(VK_NUMPAD5)) fc3xr::Recenter();
        if (KeyPressed(VK_NUMPAD2)) { g_headTracking = !g_headTracking; Log("[KEY] head tracking %s", g_headTracking ? "ON" : "OFF"); }
        if (KeyPressed(VK_NUMPAD0)) { g_pumpEnabled = !g_pumpEnabled; Log("[KEY] pump action %s", g_pumpEnabled ? "ON" : "OFF"); }
        if (KeyPressed(VK_NUMPAD3)) { g_aimEnabled = !g_aimEnabled; Log("[KEY] aim with the gun %s", g_aimEnabled ? "ON" : "OFF"); }
        if (KeyPressed(VK_NUMPAD8)) { g_motionGun = !g_motionGun; Log("[KEY] arms follow the controllers %s", g_motionGun ? "ON" : "OFF"); }
    }

    // choose the main camera's projection scale from this frame's writes
    LONG bestCount = 0;
    for (const CameraBin& bin : g_cameraBins)
    {
        if (bin.count > bestCount)
        {
            bestCount = bin.count;
            g_mainXScale = bin.xScale;
            g_worldProjection = bin.projection;
            std::memcpy(g_mainRight, bin.right, sizeof(g_mainRight));
        }
    }
    const LONG frame = g_frame;
    const bool report = frame == 120 || (frame > 0 && frame % 600 == 0);
    if (report)
    {
        char bins[160] = {};
        int used = 0;
        for (const CameraBin& bin : g_cameraBins)
            if (bin.count && used < 130)
                used += _snprintf_s(bins + used, sizeof(bins) - used, _TRUNCATE, " %.3fx%ld", bin.xScale, bin.count);
        Log("[CAM] frame %ld main xScale=%.4f, this frame's cameras:%s", frame, g_mainXScale, bins[0] ? bins : " none");
    }
    std::memset(g_cameraBins, 0, sizeof(g_cameraBins));

    ID3D11Texture2D* rightEye = g_enabled ? static_cast<ID3D11Texture2D*>(GetTwin<ID3D11Resource>(backbuffer)) : nullptr;
    const bool headset = fc3xr::Frame(g_dev, backbuffer, rightEye);
    static int previousMotion = -1;
    const bool motionAllowed = MotionInteractionEnabled();
    if (previousMotion != static_cast<int>(motionAllowed))
    {
        Log("[MOTION AREA] %s: native weapon draws %ld; movement/buttons stay active",
            motionAllowed ? "weapon interaction active" : "arm/weapon overrides paused",
            g_visibleWeaponDrawsLastFrame);
        previousMotion = static_cast<int>(motionAllowed);
    }
    AimFrame(headset);
    TickMacheteNativeProbe();
    TickMachete(headset && g_inGameplay && MotionInteractionEnabled());
    TickMacheteRoutingProbe();
    TickMacheteQuickProbe();
    if(!MacheteHeld())TickPhysicalReload(headset && g_inGameplay && MotionInteractionEnabled());
    TickAutoAmmoCapture(headset && g_inGameplay && MotionInteractionEnabled());
    TickAmmoOwnerProbe(headset && g_inGameplay);
    TickAnimationProbe(headset && g_inGameplay);
    UpdateAmmoTestNote();
    MacheteNote();
    TickPhysicalHull(headset && g_inGameplay);
    TickEjectionProbe(headset && g_inGameplay);
    TickActionProbe(headset && g_inGameplay);
    UpdateEyeBuffers();     // for the frame the game renders next
    if (rightEye)
    {
        // Automatic eye captures only once the 3D world has been on screen for
        // a while (menus and loading screens draw a few dozen doubled draws,
        // the world draws hundreds), then twice more a minute apart.
        static int zeroTestFrames = 0;
        static float zeroTestSeparation = 0.0f;
        const LONG doubledThisFrame = g_stats.stereoDraws - g_lastStereoDraws;
        g_gameplayFrames = doubledThisFrame >= 300 ? g_gameplayFrames + 1 : 0;
        // (not while the headset is running: the readback stalls a frame)
        const bool autoDump = !headset && g_autoDumps < 3 &&
            g_gameplayFrames == 600 + 3600 * g_autoDumps;
        if (zeroTestFrames > 0 && --zeroTestFrames == 0)
        {
            Log("[ZERO] capture with separation 0 follows; eyes should match exactly");
            DumpEyes(backbuffer, rightEye);
            g_separation = zeroTestSeparation;
        }
        static int handFrames = 0;
        if (g_eyeViewsSet && g_inGameplay && g_weaponDrawsLastFrame >= 4 && fc3pad::FireHeld() && handFrames++ == 0)
        {
            Log("[WEAPON] automatic capture of the first-person model while firing (bones, mesh, camera)");
            g_dumpRequested = true;
        }
        if (g_dumpRequested || autoDump)
        {
            g_dumpRequested = false;
            if (autoDump) ++g_autoDumps;
            DumpEyes(backbuffer, rightEye);
            ++g_censusArmed;
            // After the first automatic capture, take one more with the eye
            // separation at zero. With no separation the two eyes must be
            // identical; any difference left is a plumbing fault, not parallax.
            if (autoDump && g_autoDumps == 1 && zeroTestFrames == 0)
            {
                zeroTestSeparation = g_separation;
                g_separation = 0.0f;
                zeroTestFrames = 45;
                Log("[ZERO] separation forced to 0 for 45 frames");
            }
        }
        if (g_view == 1)
            DrawSideBySide(backbuffer, rightEye);
        rightEye->Release();
    }

    if (report)
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        static Stats previous = {};
        static LONG previousFrame = 0;
        const LONG frames = frame - previousFrame > 0 ? frame - previousFrame : 1;
        const double classifyMs = (g_stats.classifyTicks - previous.classifyTicks) * 1000.0 /
            static_cast<double>(frequency.QuadPart) / frames;
        Log("[STAT] frame %ld %ux%u stereo=%s rightEyeImage=%s | per frame: single draws %ld, doubled draws %ld, "
            "failed %ld, started %ld, clears %ld, copies %ld, compute %ld (doubled %ld) | bookkeeping %.2f ms",
            frame, g_bbW, g_bbH, g_enabled ? "on" : "off", rightEye ? "yes" : "NO",
            (g_stats.monoDraws - previous.monoDraws) / frames,
            (g_stats.stereoDraws - previous.stereoDraws) / frames,
            (g_stats.failedDraws - previous.failedDraws) / frames,
            (g_stats.initiated - previous.initiated) / frames,
            (g_stats.clears - previous.clears) / frames,
            (g_stats.copies - previous.copies) / frames,
            (g_stats.dispatches - previous.dispatches) / frames,
            (g_stats.stereoDispatches - previous.stereoDispatches) / frames, classifyMs);
        Log("[STAT] duplicate targets created so far %ld (%.0f MB, includes freed ones), duplicate views %ld, vertex shaders patched %ld / left alone %ld, "
            "pixel shaders patched %ld, camera writes %ld, separation %.4f",
            g_stats.twins, g_stats.twinBytes / 1048576.0, g_stats.twinViews,
            g_stats.vsPatched, g_stats.vsUnpatched, g_stats.psPatched, g_stats.cameraWrites, g_separation);
        previous = g_stats;
        previousFrame = frame;
    }

    if (g_frame % 1800 == 0 && g_frame > 0)
        Log("[ARMS] arm draws with rewritten bones %ld, weapon draws: skinned %ld / solid %ld, arm pose unreadable %ld times",
            g_armPaletteDraws, g_weaponPaletteDraws, g_weaponWorldDraws, g_armIkRejected);

    // A census covers exactly the frame after an eye capture.
    if (g_census != INVALID_HANDLE_VALUE)
    {
        CensusFlush();
        CloseHandle(g_census);
        g_census = INVALID_HANDLE_VALUE;
        Log("[CENSUS] frame recorded");
    }
    else if (g_censusArmed > 0)
    {
        static int censusIndex = 0;
        --g_censusArmed;
        wchar_t path[MAX_PATH] = {};
        _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%sfc3_census_%d.txt", fc3::DataFolder(), ++censusIndex);
        g_census = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    }
    fc3fov::Update(headset && g_headTracking, g_frameCameraValid, g_worldProjection.xs, g_worldProjection.ys);
    fc3aim::Probe(g_frameCamera.forward, g_frameCameraValid, g_inGameplay,
                  g_visibleWeaponDrawsThisFrame > 0);
    g_inGameplay = g_stats.stereoDraws - g_lastStereoDraws >= 300;
    g_frameCameraValid = false;
    g_frameCameraTries = 0;
    g_visibleWeaponDrawsLastFrame = g_visibleWeaponDrawsThisFrame;
    g_visibleWeaponDrawsThisFrame = 0;
    g_weaponDrawsLastFrame = g_weaponDrawsThisFrame;
    g_weaponDrawsThisFrame = 0;
    g_lastStereoDraws = g_stats.stereoDraws;
    backbuffer->Release();
    ++g_frame;
    if (!headset)
        PaceFrame();    // the headset paces frames itself
}
}
