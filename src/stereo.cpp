#include "selector_scene292.h"
#include "car_wheel352.h"
#include "handlebar355.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdarg>
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <MinHook.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <utility>
#include <new>
#include <intrin.h>

#include "dxbc_patch.h"
#include "fc3_log.h"
#include "fov_boost.h"
#include "stereo.h"
#include "tile_shader.h"
#include "shadow_readback.h"
#include "scene_grid258.h"
#include "aim_probe.h"
#include "throwable_launch_probe.h"
#include "muzzle58.h"
#include "di_probe.h"
#include "vr_pad.h"
#include "xr_presenter.h"
#include "hand_ui_settings.h"
#include "night_tone_probe.inl"
#include "health_ui_probe.inl"
#ifndef FC3_AMMO_TEST
#include "throwable_capture_host.h" // throwable launch capture (observation only)
#endif
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_dx11.h"


using fc3::Log;

namespace
{
#include "scope60_settings.inl"
#include "calibration136_settings.inl"
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
ArmJoints g_armJoints[2] = {};
bool g_nativeUiRayEnabled298=true;
bool g_nativeUiMouseEnabled=true;     // [0] right, [1] left
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
using ClearUavUintFn=void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11UnorderedAccessView*,const UINT[4]);
using ClearUavFloatFn=void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11UnorderedAccessView*,const FLOAT[4]);
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
ClearUavUintFn g_oClearUavUint61=nullptr;ClearUavFloatFn g_oClearUavFloat61=nullptr;
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
volatile LONG g_minHookReady = 0; // set once MH_Initialize succeeded (throwable capture host)
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
#include "pistol_settings.inl"
float g_worldScale = 1.0f;      // headset only: multiplies the headset's eye distance

float g_mainXScale = 1.3f;      // horizontal projection scale of the main camera
float g_mainRight[3] = { 1.0f, 0.0f, 0.0f };  // main camera's right axis in world space
// [0 world, 1 first-person body, 2 ui, 3 left arm, 4 right arm + gun][0 left eye, 1 right eye]
// 5 = Jason's own body pieces (legs, torso): they stay with the player's body,
// not with the camera, while the camera follows the gun
constexpr int kEyeVariants = 6;
bool g_aimActive = false;
bool g_autoPistolSupport=true,g_autoLongGunSupport=false;
float g_twoHandRecoilAssist=1.0f;
float g_rifleRecoilAssist=0.5f; // fraction of animated rifle kick removed from native aim
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
float g_physicalBody303[2][16]={};bool g_physicalValid303[2]={},g_renderAnatomical303=false;
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
struct DrawCall;
bool ThrowableLease144();bool ThrowableOwns144();bool ThrowableArmLease150();
void ObserveThrowable144(ID3D11DeviceContext*,const DrawCall&);
bool g_doors357Enabled=false; // 365: physical door entry removed (never entered the car live); forced off
// 365: where the drawn wrist sits relative to the rim centreline while attached.
// 363 put the wrist ON the centreline = hand inside the wheel tube. Tunable in
// the Insert menu: pull toward the driver (−Y in wheel space) and outward along the rim radius.
// 366: per hand (index 0 = right, 1 = left). The 365 eye capture: wrist placed on the rim,
// right fist extends toward the hub, left fist over the top. Defaults read off that capture.
float g_handRimPullCm366[2]={4.f,4.f},g_handRimOutCm366[2]={6.f,1.f};
bool g_autoHeadsetCapture366=false; // 366: the automatic in-headset eye capture (400 ms stall) is opt-in
void ObserveCarDoor357(ID3D11DeviceContext*,const DrawCall&,int);
ID3D11Buffer* BuildCarWheelWorld352(ID3D11DeviceContext*,const DrawCall&,int);
ID3D11Buffer* BuildCarArmDraw353(ID3D11DeviceContext*,const DrawCall&,bool&);
ID3D11Buffer* BuildHandlebarWorld355(ID3D11DeviceContext*,const DrawCall&,int);
struct CameraSummary;
void TickHandlebars355(const CameraSummary&);
void TickCarWheel363(const CameraSummary&);
bool ThrowableKeepVertexShadow145(const D3D11_BUFFER_DESC&);
bool BeginMolotovParticles145(ID3D11DeviceContext*,const DrawCall&,int,ID3D11Buffer*&,UINT&,UINT&);
ID3D11Buffer* ThrowablePalette144(ID3D11DeviceContext*,UINT,UINT);
ID3D11Buffer* ThrowableWorld144(ID3D11DeviceContext*,UINT,UINT);
bool ThrowableVisual144(ID3D11DeviceContext*);bool ThrowableHidden144();
bool PlaceableDrawHidden177(ID3D11DeviceContext*);
void CaptureLighterDepth161(ID3D11DeviceContext*,ID3D11DepthStencilView*,ID3D11DepthStencilView*);
void CaptureMolotovVisual158(ID3D11DeviceContext*);void RenderMolotov158(ID3D11Texture2D*,ID3D11Texture2D*);
bool ThrowablePinHidden154(ID3D11DeviceContext*,const DrawCall&);
void ReplayGrenadePin154(ID3D11DeviceContext*,const DrawCall&);
void LoadThrowables144(const wchar_t*);
bool MotionInteractionEnabled()
{ bool seated=false;int seat=-1;return !fc3aim::NativeActionOwnsArms128() && !(fc3aim::VehicleState(seated,seat)&&seated) && !fc3pad::NativeMenuOpen() && g_motionGun && (!g_weaponMotionGuard || g_visibleWeaponDrawsLastFrame > 0 || MacheteMotionLease() || ThrowableLease144()); }
// Render-only recovery: do not revive aim/camera or gameplay interactions
// just because the native throw animation still has an arm to draw.
bool ThrowableArmMotion150();bool ThrowableArmSide150(int side);
bool ControllerBodyDraw151(int variant){return variant==0&&(MotionInteractionEnabled()||ThrowableArmMotion150());}
// Where the game holds each hand, as a pose in its first-person camera's space
// ([0] right, [1] left). Defaults until the player calibrates (both stick clicks).
// Where the top of the upper arm sits (right side; body frame, metres from the eyes; the left is
// x-mirrored). Default = where the game's own rig holds the right arm top in the SPAS idle, so the
// arms sit exactly where the game put them. 10-08 first fix used the old IK pivot (0.20,-0.15,-0.20),
// 15 cm nearer the eyes: "the arms are too close to the head". Ini: ShoulderX/Y/Z.
double g_shoulderSpot[3] = { 0.269, -0.240, -0.304 };
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
    UINT uploadCallerRva, uploadMethod; // read-only D3D upload provenance, not CPU cull ownership
};
struct RenderDrawSample {LONG frame=-1;uintptr_t context=0,buffer=0,target=0,depth=0;CameraSummary camera={};D3D11_VIEWPORT viewport={};UINT width=0,height=0,depthWidth=0,depthHeight=0,depthSamples=0;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN,depthFormat=DXGI_FORMAT_UNKNOWN,depthViewFormat=DXGI_FORMAT_UNKNOWN;};
RenderDrawSample g_renderDrawSamples[4]={};
void ObserveRenderDraw(ID3D11DeviceContext*,ID3D11Buffer*,const CameraSummary&);
void ObserveNativeUiPanel(int eye,float sx,float sy,float ox,float oy);
CameraSummary g_frameCamera = {};
bool g_nativeVehicleCamera279=false;
bool g_slideCamera280=false,g_slideBodyValid280=false;
bool g_frameCameraValid = false;
LONG g_mainBinCount = 0; // draws of the chosen main camera this frame (world: hundreds; the weapon camera alone: ~6-15)
int g_frameCameraTries = 0;
bool g_headTracking = true;
int g_pacePeriods = 0;

// camera histogram for the current frame
struct CameraBin { float xScale; LONG count; float right[3]; GameProjection projection; };
CameraBin g_cameraBins[8] = {};
// Perspective camera writes that IsMainView rejected (aspect / size), for the [CAM] report (LT aim / scope left the
// mod without a world camera for the rest of the session: "this frame's cameras: 1.554x15" = weapon only).
struct RejectedCamera { float xScale, yScale, depthRow; LONG count; };
RejectedCamera g_rejectedCameras[6] = {};
UINT g_cameraBufferSizes[8] = {}; LONG g_cameraBufferSizeCounts[8] = {}; // sizes of buffers that carried a main-view camera this frame

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
bool g_worldViewActive=false;
#include "gameplay_assist.inl"
int g_autoDumps = 0;
LONG g_gameplayFrames = 0;      // consecutive frames that look like the 3D world
LONG g_lastStereoDraws = 0;
// [FLATVIEW] Loading screens draw perspective geometry without any camera the
// mod recognises. Converting it with a default projection fans its depth
// layers apart. Frames with no 3D camera are shown as the head-locked UI panel.
int g_cameraLessFrames = 0;     // consecutive frames without a main-view camera write
bool g_flatViewEnabled = true;
bool g_flatViewActive = false;
int g_zeroEyeFrames = 0;        // >0: both eyes use the left eye's view (headset zero-separation capture)
// [SBSRESTORE] The game's swap chain has one buffer (blt model) that keeps its
// contents across Present. Menus and loading screens draw over it without a
// full redraw, so the monitor side-by-side image (left eye squeezed into the
// left half) fed back into the next frame: copies at half, quarter, eighth
// width, in the headset too. After Present the left eye image goes back.
bool g_sideBySideDrawn = false;
// [TILES] Deferred lighting is drawn as flat screen tiles (vs DAF35B64: pos =
// v0 * QuadParams.xy + QuadParams.zw, w = 1); the CPU picks each tile's lamps
// for the GAME camera's screen. With head tracking the eye looks elsewhere, so
// a lamp's light was cut off at tile edges and vanished when looked at. The
// tiles are redrawn through the eye's homography (game clip -> eye clip at a
// far depth), keeping the game's position-to-uv relation.
constexpr unsigned long long kTileVsHash = 0xDAF35B64BFCE130Dull;
constexpr UINT kTileCbSlot = 12;
bool g_tileFixEnabled = true;
ID3D11VertexShader* g_tileVs = nullptr;
ID3D11GeometryShader* g_tileGs = nullptr;   // clips each tile at the eye plane, then projects
// renderfix5 turned the scene black: 105 shaders took the tile test, but every
// non-tile draw bound the 64-byte pixel eye buffer, so cb13[4] (the test's
// on-switch) was read past the end of the buffer and was not zero on this GPU.
// Both pixel eye buffers are now 80 bytes; the normal one has cb13[4] = 0.
// [TILEDIAG] 10-10: tiles = runs of cells on a 30x17 screen grid, two triangles
// per run, QuadParams (1,-1,0,0), no scissor: what the tile shaders assume.
// renderfix7 still dark: the game's own shaders no longer carry the test at all
// (renderfix8). Tile-capable lighting shaders get a SEPARATE tile-test copy,
// bound only for the mod's tile draws. A self-check compares the samples the
// tiles keep with and without the test every 120 frames and turns it off if
// the test drops pixels.
bool g_tilePixelTest = true;
const GUID kPsTileVariantGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 61 } };
ID3D11Query* g_tileQuery[2][160] = {};      // [frame parity][draw]: samples kept, left eye
int g_tileQueryUsed[2] = {};
bool g_tileQueryMeasure[2] = {};            // that frame drew the tiles WITHOUT the test (renderfix4 shaders)
int g_tileSelfFail = 0;
bool g_tileMeasureFrame = false;            // this frame: tiles without the test
ID3D11VertexShader* g_tileVsSimple = nullptr;
ID3D11GeometryShader* g_tileGsSimple = nullptr;
int g_tileDiag = 0;                          // [TILEDIAG] tile draws described so far
ID3D11Buffer* g_tileCb[2] = {};
ID3D11Buffer* g_eyePsCbTile[2] = {};   // pixel eye columns + row 4 = (1,0,0,0): tile test on
LONG g_psTileTest = 0;                   // pixel shaders that took the tile test
LONG g_tileDraws = 0;

// ---- small helpers ----------------------------------------------------------
template <class T>
T* GetTwin(ID3D11DeviceChild* object,const GUID& key=kTwinGuid)
{
    IUnknown* twin = nullptr;
    UINT size = sizeof(twin);
    if (SUCCEEDED(object->GetPrivateData(key, &size, &twin)) && twin)
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
ID3D11Resource* TwinResource(ID3D11Resource* resource, bool create,const GUID& key=kTwinGuid)
{
    if (!resource)
        return nullptr;
    if (ID3D11Resource* existing = GetTwin<ID3D11Resource>(resource,key))
        return existing;
    if (!create || !g_dev || !g_ctx)
        return nullptr;

    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    resource->GetType(&dimension);
    if(dimension==D3D11_RESOURCE_DIMENSION_BUFFER&&!IsEqualGUID(key,kTwinGuid)){
        D3D11_BUFFER_DESC desc={};static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);if(desc.Usage!=D3D11_USAGE_DEFAULT||desc.CPUAccessFlags||desc.ByteWidth>16*1024*1024)return nullptr;
        ID3D11Buffer* copy=nullptr;const bool own=g_own;g_own=true;const HRESULT result=g_dev->CreateBuffer(&desc,nullptr,&copy);if(SUCCEEDED(result)&&copy){g_oCopyResource(g_ctx,copy,resource);resource->SetPrivateDataInterface(key,copy);ScopeCleanTrack61(resource);}g_own=own;return copy;
    }
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
    resource->SetPrivateDataInterface(key, twin);
    if(!IsEqualGUID(key,kTwinGuid)){ScopeCleanTrack61(resource);return twin;}
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
ID3D11View* TwinView(ID3D11View* view, ViewKind kind, bool createResource,const GUID& key=kTwinGuid)
{
    if (!view)
        return nullptr;
    if (ID3D11View* existing = GetTwin<ID3D11View>(view,key))
        return existing;

    if (!createResource&&IsEqualGUID(key,kTwinGuid))
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
    ID3D11Resource* twinResource = TwinResource(resource, createResource,key);
    resource->Release();
    if (!twinResource)
    {
        if (!createResource&&IsEqualGUID(key,kTwinGuid))
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
        if(!IsEqualGUID(key,kTwinGuid)&&desc.ViewDimension==D3D11_UAV_DIMENSION_BUFFER&&(desc.Buffer.Flags&(D3D11_BUFFER_UAV_FLAG_APPEND|D3D11_BUFFER_UAV_FLAG_COUNTER))){twinResource->Release();return nullptr;}
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
    view->SetPrivateDataInterface(key, twinView);
    if(!IsEqualGUID(key,kTwinGuid))ScopeCleanTrack61(view);
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

#include "hands_only287.inl"

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

void RawIssue(ID3D11DeviceContext* context, const DrawCall& call)
{
    switch (call.kind)
    {
    case 0: g_oDrawIndexed(context, call.a, call.b, call.d); break;
    case 1: g_oDraw(context, call.a, call.b); break;
    case 2: g_oDrawIndexedInstanced(context, call.a, call.b, call.c, call.d, call.e); break;
    default: g_oDrawInstanced(context, call.a, call.b, call.c, call.e); break;
    }
}

#include "hand_ui_capture.inl"
#include "night_shadow_floor.inl"
SRWLOCK g_nightProbeLock=SRWLOCK_INIT;
__declspec(align(8)) LONG64 g_nightDrawSerial=0;
LONG g_nightActiveDraws=0;
#include "night_capture_windows.inl"
bool IssueMp5Hint229(ID3D11DeviceContext*,const DrawCall&);
void Issue(ID3D11DeviceContext* context,const DrawCall& call,UINT eye=0)
{
    if(CutHandUi(context,call))return;
    bool armed=false;unsigned long long hash=0,serial=0;
    const LONG frame=g_frame;
    if(g_enabled&&g_inGameplay&&!g_flatViewActive&&g_visibleWeaponDrawsLastFrame>0&&!fc3pad::NativeMenuOpen()&&frame>=600){
        AcquireSRWLockExclusive(&g_nightProbeLock);
        const bool needed=!(fc3night::claimed[0][0]&&fc3night::claimed[0][1]&&fc3night::claimed[1][0]&&fc3night::claimed[1][1]);
        if(needed){ID3D11PixelShader* ps=nullptr;context->PSGetShader(&ps,nullptr,nullptr);hash=ShaderHash(ps);SafeRelease(ps);
            serial=static_cast<unsigned long long>(InterlockedIncrement64(&g_nightDrawSerial));
            armed=fc3night::BeginActualDraw(context,hash,frame,serial,eye,true);}
        if(armed)++g_nightActiveDraws;
        ReleaseSRWLockExclusive(&g_nightProbeLock);
    }
    if(g_enabled&&g_inGameplay&&!g_flatViewActive&&!fc3pad::NativeMenuOpen()&&call.kind<=1&&call.a<=384){
        if(!hash){ID3D11PixelShader* ps=nullptr;context->PSGetShader(&ps,nullptr,nullptr);hash=ShaderHash(ps);SafeRelease(ps);}
        NightShadowFloor floor(context,hash,fc3xr::Running());if(!IssueMp5Hint229(context,call))RawIssue(context,call);
    }else if(!IssueMp5Hint229(context,call))RawIssue(context,call);
    if(armed){AcquireSRWLockExclusive(&g_nightProbeLock);fc3night::EndActualDraw(context,hash,frame,serial,eye,true);--g_nightActiveDraws;ReleaseSRWLockExclusive(&g_nightProbeLock);}
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
void ObservePistolHand(const float*,const float*,const CameraSummary&);
bool MagazineHideBullet(ID3D11DeviceContext*,const DrawCall&);
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
    if(side==0){ObserveShotHand(bones,globals,camera,joints);ObservePistolHand(bones,globals,camera);}
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

#include "shadow_capture.inl"
#include "water_capture.inl"

// Census frames: the original bytecode of every shader drawn, once per hash,
// as shaders\fc3_<kind>_<hash>.bin (disassemble with fxc /dumpbin).
void DumpShaderBytecode(ID3D11DeviceChild* shader, const char* kind)
{
    if (!shader) return;
    const unsigned long long hash = ShaderHash(shader);
    static std::vector<unsigned long long> done;
    for (unsigned long long h : done) if (h == hash) return;
    done.push_back(hash);
    UINT size = 0;
    if (FAILED(shader->GetPrivateData(kEjectionBytecodeGuid, &size, nullptr)) || size == 0) return;
    std::vector<unsigned char> bytes(size);
    if (FAILED(shader->GetPrivateData(kEjectionBytecodeGuid, &size, bytes.data()))) return;
    static bool folder = false;
    if (!folder)
    {
        wchar_t path[MAX_PATH] = {};
        _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%sshaders", fc3::DataFolder());
        CreateDirectoryW(path, nullptr);
        folder = true;
    }
    wchar_t name[64] = {};
    _snwprintf_s(name, 64, _TRUNCATE, L"shaders\\fc3_%hs_%016llX.bin", kind, hash);
    WriteBlob(name, bytes);
}

#include "environment_capture252.inl"

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
bool NativeVehicleActual279(const CameraSummary& camera);
void UpdateEyeBuffers();
ID3D11Buffer* BuildWeaponPalette(ID3D11DeviceContext* context, UINT start, UINT count,UINT* outputSlot=nullptr);
ID3D11Buffer* BuildWeaponWorld(ID3D11DeviceContext* context, UINT start, UINT count);
static const GUID kA2000Mirror337={0x337a2000,0x7c12,0x4b78,{0xb1,0x3d,0x21,0xc7,0x91,0x5e,0x44,0x37}};
bool A2000MirrorDrawModel337(ID3D11DeviceContext*,UINT,UINT);
bool SideAttachment338(ID3D11DeviceContext*,UINT,UINT);
bool A2000MirrorWorld337(ID3D11DeviceContext*,UINT,UINT,float*);
struct Mat4;
bool SideSkinnedAttachmentMove338(ID3D11DeviceContext*,UINT,UINT,Mat4&);
Mat4 A2000MirrorOutput337(const Mat4&,const Mat4&,bool);
Mat4 CanonicalPhysical338(const Mat4&,const Mat4&);
bool MirrorSkinned338(float*,const Mat4&);
void ClearMirrorPalette338();
ID3D11Buffer* MirrorRasterSource349(ID3D11Buffer*,ID3D11Buffer*);
bool BeginA2000Raster337(ID3D11DeviceContext*,ID3D11Buffer*,ID3D11RasterizerState*&,bool&);
void EndA2000Raster337(ID3D11DeviceContext*,ID3D11RasterizerState*&,bool);

bool RevolverAssemblyDraw217(ID3D11DeviceContext*,UINT,UINT);
void ObserveRevolverAssembly247(ID3D11DeviceContext*,const DrawCall&);
ID3D11Buffer* BuildRevolverPalette217(ID3D11DeviceContext*,UINT,UINT,UINT&);
bool RevolverAssemblyWorld217(ID3D11DeviceContext*,UINT,UINT,float*);
void Stage1911Frame(ID3D11DeviceContext*,UINT,UINT,const float*,ID3D11Buffer*);
void Confirm1911Frame(ID3D11DeviceContext*,UINT,UINT,ID3D11Buffer*,int);
void Capture1911Guide48(ID3D11DeviceContext*,UINT,UINT,ID3D11Buffer*,int,int);
void MuzzleConfirm58(ID3D11DeviceContext*,const DrawCall&,ID3D11Buffer*,int);
void CaptureLaserEye205(ID3D11DeviceContext*,const DrawCall&,ID3D11Buffer*,int,int);
void LaserDrawObserve206(ID3D11DeviceContext*);void LaserClearObserve206(ID3D11DeviceContext*,ID3D11DepthStencilView*);
void RenderLaserDepth205(ID3D11Texture2D*,ID3D11Texture2D*);
bool NativeGuideScope50();bool NativeGuideShadowScope51();void CaptureWeaponGuides50(ID3D11DeviceContext*,const DrawCall&,int,int);void CaptureBowGuides51(ID3D11DeviceContext*,const DrawCall&,int,int);void CaptureBodyGuides51(ID3D11DeviceContext*,const DrawCall&,int,int);
ID3D11Buffer* Begin1911Tint49(ID3D11DeviceContext*,const DrawCall&,int);
void End1911Tint49(ID3D11DeviceContext*,ID3D11Buffer*&);
void Begin1911Trace46(ID3D11DeviceContext*,const DrawCall&);void End1911Trace46(ID3D11DeviceContext*,const DrawCall&,int);
void Trace1911World46(ID3D11DeviceContext*,const float*,UINT,ID3D11Buffer*);void Trace1911Palette46(ID3D11DeviceContext*,const float*,UINT,ID3D11Buffer*);

ID3D11Buffer* BuildBodyPalette(ID3D11DeviceContext* context);
UINT g_bodyDrawStart = 0, g_bodyDrawCount = 0;   // the body section BuildBodyPalette is called for

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
void RecordMacheteDrawCensus(ID3D11DeviceContext* context,const DrawCall& call,int variant);
void RecordHealAlternate217(ID3D11DeviceContext*,const DrawCall&,int);
void RecordAnimationDraw(ID3D11DeviceContext*,const DrawCall&);
void RecordAnimationFinal(const float*,int);
void FinishAnimationDraw();
void RecordVrBaselineDraw(ID3D11DeviceContext*,const DrawCall&,int,bool);
void TickActionProbe(bool gameplay);
void TickMacheteBladeContact();void LoadMacheteBladeContactSettings(const wchar_t*); // blade-driven melee contact (machete_blade_contact.inl)
bool Weapons8CaptureNote();
void TickEjectionProbe(bool gameplay);
void TickPhysicalHull(bool gameplay);
bool PhysicalHullDraw(ID3D11DeviceContext* context, const DrawCall& call);
bool ReplayPhysicalHull(ID3D11DeviceContext* context);
void ObserveBowArrowDraw(ID3D11DeviceContext*,const DrawCall&);void ReplayBowArrow(ID3D11DeviceContext*,const DrawCall&);void ReleaseBowArrowResources();
void ReplayRevolverLoader(ID3D11DeviceContext*,const DrawCall&);void ObserveRevolverLoaderDraw(ID3D11DeviceContext*,const DrawCall&);void ReplayRpgRocket(ID3D11DeviceContext*,const DrawCall&);void ObserveRpgRocketDraw(ID3D11DeviceContext*,const DrawCall&);
void RecordEjectionDraw(ID3D11DeviceContext* context, const DrawCall& call, bool firstPerson);

bool MacheteHeld();
void CaptureMachete(ID3D11DeviceContext*,const DrawCall&);
void ReplayMachete(ID3D11DeviceContext*);
bool NativeMacheteDraw116(ID3D11DeviceContext*,const DrawCall&);
ID3D11Buffer* NativeMachetePalette116(ID3D11DeviceContext*,const DrawCall&);
void LoadMacheteSettings(const wchar_t*);
void LoadReloadSignalSettings(const wchar_t*);
void LoadPistolReloadSettings();
void LoadSupportGrab214(const wchar_t*);
void ObserveRepairToolDraw(ID3D11DeviceContext*,const DrawCall&);ID3D11Buffer* BuildRepairToolEffectWorld(ID3D11DeviceContext*,const DrawCall&,int);void ReleaseRepairToolResources();void ReleaseMuzzleFlashResources111();void ReleaseThrowableResources145();
ID3D11Buffer* BuildVectorCasingWorld231(ID3D11DeviceContext*,const DrawCall&,int);
void ReleaseVectorCasing231();
ID3D11Buffer* BuildRifleCasingWorld235(ID3D11DeviceContext*,const DrawCall&,int);
void ReleaseRifleCasings235();
ID3D11Buffer* BuildSvdLens236(ID3D11DeviceContext*,const DrawCall&);
void ReleaseSvdLens236();
void CaptureFlamerStream239(ID3D11DeviceContext*,UINT,UINT,const float*);
bool FlamerProjectedDraw241(ID3D11DeviceContext*,const DrawCall&);
bool BeginFlamerProjection241(ID3D11DeviceContext*,ID3D11VertexShader*&);
bool RepairKeepVertexShadow(const D3D11_BUFFER_DESC&);void RepairRememberVertexCopy(ID3D11Resource*);
bool BeginRepairToolParticleDraw(ID3D11DeviceContext*,const DrawCall&,int,ID3D11Buffer*&,UINT&,UINT&);void EndRepairToolParticleDraw(ID3D11DeviceContext*,ID3D11Buffer*&,UINT,UINT);
bool MuzzleFlashKeepVertexShadow111(const D3D11_BUFFER_DESC&);bool BeginMuzzleFlashDraw111(ID3D11DeviceContext*,const DrawCall&,int,ID3D11Buffer*&,UINT&,UINT&);
bool BeginFlamerDraw242(ID3D11DeviceContext*,const DrawCall&,ID3D11Buffer*&,UINT&,UINT&);
bool ParticlePoolDraw243(ID3D11DeviceContext*,const DrawCall&);void ParticlePoolProbe243(ID3D11DeviceContext*,const DrawCall&,int,bool,bool);
void RepairEffectCensus(ID3D11DeviceContext*,const DrawCall&,int);void RepairIndexUploaded(ID3D11Resource*,const void*,bool);
#include "weapon_presence.h"
// An observed non-M133 model must never inherit the M133's cycling rules.
// Unknown geometry retains the legacy behavior until a known player model is
// observed; profile metadata does not pretend to identify native variants.
volatile LONG g_weaponMechanicProfile=-1;
bool NativeGuideShadowScope51(){return g_weaponMechanicProfile>=0;}
volatile LONG g_weaponMechanicFrame=-100000;
bool KnownNonPumpModel()
{
    return g_weaponMechanicProfile>=0&&std::strcmp(fc3weapon::kProfileNames[g_weaponMechanicProfile],"870shotgun.xbg")!=0;
}

// Cache only the immutable vertex-buffer size prefilter on the COM object.
// Equal-FOV scenery draws need no index-buffer/format/part inspection. A
// positive prefilter still requires the complete native geometry identity.
bool PossibleWeaponGeometry345(ID3D11DeviceContext* context,const DrawCall& call){
 if(call.kind!=0||call.d!=0)return false;
 static const GUID tag={0x5f350345,0xa604,0x4f82,{0x81,0x45,0x38,0x12,0x45,0x71,0x34,0x51}};
 ID3D11Buffer* stream=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&stream,&stride,&offset);if(!stream)return false;
 UINT value=0,bytes=sizeof(value);if(FAILED(stream->GetPrivateData(tag,&bytes,&value))||bytes!=sizeof(value)){
  D3D11_BUFFER_DESC desc={};stream->GetDesc(&desc);value=1;for(const auto& profile:fc3weapon::kProfiles)if(desc.ByteWidth==profile.vertexBytes){value=2;break;}stream->SetPrivateData(tag,sizeof(value),&value);
 }
 stream->Release();return value==2;
}
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
void ObservePistolGuideDraw(ID3D11DeviceContext*,const DrawCall&,int);
// [SHOT DRAWS] probe (all LMGs appeared to fire from the left of the barrel, not from it). The mod does not
// touch muzzle flashes or tracers, so they come from the game's own gun. While the trigger is held, every 15th frame (8 frames per
// weapon per session), log every BLENDED draw (flash / tracer / smoke candidates) with its shaders, geometry and the first 16 floats
// of cb0 and cb1, plus the native -> displayed gun move of that frame. Read-only.
bool ShotProbeGunMove(float* out12);
void ShotDrawProbe(ID3D11DeviceContext* context, const DrawCall& call)
{
    static LONG probeFrame = -1, lines = 0, perProfile[64] = {};
    const LONG profile = g_weaponMechanicProfile;
    if (fc3pad::Latest().rightTrigger < 0.7f || profile < 0 || profile >= 64) return;
    if (probeFrame != g_frame)
    {
        if (g_frame % 10 || perProfile[profile] >= 6) return;
        probeFrame = g_frame; lines = 0; ++perProfile[profile];
        float m[12] = {}; const bool moved = ShotProbeGunMove(m);
        Log("[SHOT DRAWS] frame %ld profile %ld: gun move native->displayed (view) %s x %.3f %.3f %.3f y %.3f %.3f %.3f z %.3f %.3f %.3f t %.3f %.3f %.3f",
            static_cast<long>(g_frame), static_cast<long>(profile), moved ? "fresh" : "none", m[0], m[4], m[8], m[1], m[5], m[9], m[2], m[6], m[10], m[3], m[7], m[11]);
    }
    // (the first version logged only HUD quads - Draw(4), dynamic 115200-byte vb, stride 40 - which filled the cap before the flash drew.)
    // Now: the HUD skipped; each distinct draw signature once per probe frame (its first draw), any blend state.
    static unsigned long long seen[160]; static UINT seenCount = 0; static LONG seenFrame = -1;
    if (seenFrame != g_frame) { seenFrame = g_frame; seenCount = 0; }
    ID3D11BlendState* blend = nullptr; float factor[4]; UINT mask = 0; context->OMGetBlendState(&blend, factor, &mask);
    D3D11_BLEND_DESC bd = {}; if (blend) blend->GetDesc(&bd); SafeRelease(blend);
    ID3D11VertexShader* vs = nullptr; ID3D11PixelShader* ps = nullptr; context->VSGetShader(&vs, nullptr, nullptr); context->PSGetShader(&ps, nullptr, nullptr);
    const unsigned long long vh = ShaderHash(vs), ph = ShaderHash(ps); SafeRelease(vs); SafeRelease(ps);
    ID3D11Buffer* vb = nullptr; UINT stride = 0, offset = 0; context->IAGetVertexBuffers(0, 1, &vb, &stride, &offset);
    D3D11_BUFFER_DESC vd = {}; if (vb) vb->GetDesc(&vd); SafeRelease(vb);
    if (call.kind == 1 && call.a == 4 && stride == 40 && vd.ByteWidth == 115200) return; // HUD
    const unsigned long long signature = vh * 31 + ph * 17 + vd.ByteWidth * 7 + stride + (bd.RenderTarget[0].BlendEnable ? 1ull << 63 : 0);
    for (UINT i = 0; i < seenCount; ++i) if (seen[i] == signature) return;
    if (seenCount >= 160 || ++lines > 160) return;
    seen[seenCount++] = signature;
    D3D11_PRIMITIVE_TOPOLOGY topology = {}; context->IAGetPrimitiveTopology(&topology);
    float c0[16] = {}, c1[16] = {}; ID3D11Buffer* cb = nullptr;
    context->VSGetConstantBuffers(0, 1, &cb); const UINT b0 = ReadCopy(cb, c0, sizeof(c0)); SafeRelease(cb);
    context->VSGetConstantBuffers(1, 1, &cb); const UINT b1 = ReadCopy(cb, c1, sizeof(c1)); SafeRelease(cb);
    Log("[SHOT DRAWS]  kind %u n %u start %u base %d | vs %016llX ps %016llX | vb %u bytes %s stride %u topo %d | blend %d src %d dst %d | cb0(%u) %.3f %.3f %.3f %.3f / %.3f %.3f %.3f %.3f / %.3f %.3f %.3f %.3f / %.3f %.3f %.3f %.3f | cb1(%u) %.3f %.3f %.3f %.3f / %.3f %.3f %.3f %.3f / %.3f %.3f %.3f %.3f / %.3f %.3f %.3f %.3f",
        call.kind, call.a, call.kind == 2 ? call.c : call.b, call.d, vh, ph, vd.ByteWidth, vd.Usage == D3D11_USAGE_DYNAMIC ? "dynamic" : "static", stride, static_cast<int>(topology),
        bd.RenderTarget[0].BlendEnable ? 1 : 0, static_cast<int>(bd.RenderTarget[0].SrcBlend), static_cast<int>(bd.RenderTarget[0].DestBlend),
        b0, c0[0], c0[1], c0[2], c0[3], c0[4], c0[5], c0[6], c0[7], c0[8], c0[9], c0[10], c0[11], c0[12], c0[13], c0[14], c0[15],
        b1, c1[0], c1[1], c1[2], c1[3], c1[4], c1[5], c1[6], c1[7], c1[8], c1[9], c1[10], c1[11], c1[12], c1[13], c1[14], c1[15]);
}

// [NEAR GUN] probe (screenshot: a GM-94 grenade floating beside the gun in both eyes; every GM-94 draw the mod places is on the
// gun or hidden in the recordings, so it is something else). While the GM-94 is in hand, 6 sampled frames, 2 s apart: every draw
// (any camera) whose $Globals world sits within 1.2 m of the camera, other than the GM-94 body (vb 243072) and Jason's mesh
// (vb 316240) - its geometry, shaders and position in camera axes. Read-only.
double Gm94PumpDistance54();bool Gm94Model();bool Gm94SkipDraw(ID3D11DeviceContext*,const DrawCall&);bool FlareSkipDraw(ID3D11DeviceContext*,const DrawCall&);
void NearGunProbe(ID3D11DeviceContext* context, const DrawCall& call)
{
    static LONG frame = -1, samples = 0, lines = 0;static ULONGLONG nextAt = 0;
    if (!Gm94Model() || samples > 6 || !g_frameCameraValid) return;
    const ULONGLONG now = GetTickCount64();
    if (frame != g_frame) { if (now < nextAt) return; frame = g_frame; nextAt = now + 2000; ++samples; lines = 0;
        Log("[NEAR GUN] sample %ld (frame %ld): draws within 1.2 m of the camera", samples, static_cast<long>(g_frame)); }
    if (lines >= 40 || call.kind != 0) return;
    ID3D11Buffer* vb = nullptr; UINT stride = 0, offset = 0; context->IAGetVertexBuffers(0, 1, &vb, &stride, &offset);
    D3D11_BUFFER_DESC vd = {}; if (vb) vb->GetDesc(&vd); SafeRelease(vb);
    if (vd.ByteWidth == 243072 || vd.ByteWidth == 316240 || vd.Usage == D3D11_USAGE_DYNAMIC) return;
    float g[16] = {}; ID3D11Buffer* cb = nullptr; context->VSGetConstantBuffers(0, 1, &cb); const UINT bytes = ReadCopy(cb, g, sizeof(g)); SafeRelease(cb);
    if (bytes < 60) return;
    const float* c = g_frameCamera.position;
    const double dx = g[3] + g[12] - c[0], dy = g[7] + g[13] - c[1], dz = g[11] + g[14] - c[2], d = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (!(d < 1.2)) return;
    ID3D11Buffer* ib = nullptr; DXGI_FORMAT f = DXGI_FORMAT_UNKNOWN; UINT io = 0; context->IAGetIndexBuffer(&ib, &f, &io); D3D11_BUFFER_DESC idd = {}; if (ib) ib->GetDesc(&idd); SafeRelease(ib);
    ID3D11VertexShader* vs = nullptr; context->VSGetShader(&vs, nullptr, nullptr); const auto vh = ShaderHash(vs); SafeRelease(vs);
    const float* r = g_frameCamera.right; const float* u = g_frameCamera.up; const float* fw = g_frameCamera.forward;
    ++lines;
    Log("[NEAR GUN]  vb %u ib %u start %u count %u stride %u offset %u vs %016llX | %.2f m: right %.2f up %.2f forward %.2f",
        vd.ByteWidth, idd.ByteWidth, call.b, call.a, stride, offset, vh, d, dx * r[0] + dy * r[1] + dz * r[2], dx * u[0] + dy * u[1] + dz * u[2], dx * fw[0] + dy * fw[1] + dz * fw[2]);
}

// [ROGUE] probe (screenshot: a grenade floats beside the GM-94; [GM94 PASSES] proved every GM-94 shell draw was skipped,
// so it is another object). While the GM-94 is in hand, 2 frames 3 s apart: every draw that uses the rigid weapon shader
// (C386478F2F9495EB) outside the GM-94 / Jason buffers, and every small static object (index buffer < 24 KB), any camera, with
// its $Globals position from the camera in camera axes. Read-only.
void RogueProbe(ID3D11DeviceContext* context, const DrawCall& call)
{
    static LONG frame=-1,samples=0,lines=0;static ULONGLONG nextAt=0;
    if(!Gm94Model()||samples>2||call.kind!=0||!g_frameCameraValid)return;
    const ULONGLONG now=GetTickCount64();
    if(frame!=g_frame){if(now<nextAt)return;frame=g_frame;nextAt=now+3000;++samples;lines=0;Log("[ROGUE] frame %ld",static_cast<long>(g_frame));}
    if(lines>=80)return;
    ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC vd={};if(vb)vb->GetDesc(&vd);SafeRelease(vb);
    if(vd.ByteWidth==243072||vd.ByteWidth==316240)return;
    ID3D11Buffer* ib=nullptr;DXGI_FORMAT f=DXGI_FORMAT_UNKNOWN;UINT io=0;context->IAGetIndexBuffer(&ib,&f,&io);D3D11_BUFFER_DESC idd={};if(ib)ib->GetDesc(&idd);SafeRelease(ib);
    ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto vh=ShaderHash(vs);SafeRelease(vs);
    const bool weaponShader=vh==0xC386478F2F9495EBull,smallObject=idd.ByteWidth&&idd.ByteWidth<24*1024&&vd.Usage!=D3D11_USAGE_DYNAMIC;
    if(!weaponShader&&!smallObject)return;
    float g[16]={};ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);const UINT bytes=ReadCopy(cb,g,sizeof(g));SafeRelease(cb);
    const float* c=g_frameCamera.position;const float* r=g_frameCamera.right;const float* u=g_frameCamera.up;const float* fw=g_frameCamera.forward;
    const double dx=g[3]+g[12]-c[0],dy=g[7]+g[13]-c[1],dz=g[11]+g[14]-c[2],d=std::sqrt(dx*dx+dy*dy+dz*dz);
    if(!weaponShader&&!(d<3))return;
    ID3D11RenderTargetView* rt[8]={};ID3D11DepthStencilView* ds=nullptr;context->OMGetRenderTargets(8,rt,&ds);UINT n=0;for(auto*& v:rt){if(v)++n;SafeRelease(v);}SafeRelease(ds);
    ++lines;
    Log("[ROGUE]  vb %u%s ib %u start %u count %u stride %u offset %u vs %016llX targets %u cb0 %u | %.2f m: right %.2f up %.2f forward %.2f",vd.ByteWidth,vd.Usage==D3D11_USAGE_DYNAMIC?" dyn":"",idd.ByteWidth,call.b,call.a,stride,offset,vh,n,bytes,
        d,dx*r[0]+dy*r[1]+dz*r[2],dx*u[0]+dy*u[1]+dz*u[2],dx*fw[0]+dy*fw[1]+dz*fw[2]);
}

// [TILEDIAG] One line per lighting tile draw (first 24 in gameplay): draw
// layout, topology, QuadParams, scissor, and the tile's vertex positions.
void TileDiag(ID3D11DeviceContext* context, const DrawCall& call)
{
    ++g_tileDiag;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    context->IAGetPrimitiveTopology(&topology);
    float q[4] = {};
    ID3D11Buffer* cb0 = nullptr;
    context->VSGetConstantBuffers(0, 1, &cb0);
    float globals[64] = {};
    if (cb0 && ReadCopy(cb0, globals, sizeof(globals)) >= 128) std::memcpy(q, globals + 28, sizeof(q));
    else if (cb0) { std::vector<unsigned char> raw; if (ReadBytes(context, cb0, raw) && raw.size() >= 128) std::memcpy(q, raw.data() + 112, sizeof(q)); }
    SafeRelease(cb0);
    ID3D11RasterizerState* rs = nullptr;
    context->RSGetState(&rs);
    D3D11_RASTERIZER_DESC rd = {};
    if (rs) rs->GetDesc(&rd);
    SafeRelease(rs);
    D3D11_RECT scissor = {};
    UINT scissorCount = 1;
    context->RSGetScissorRects(&scissorCount, &scissor);
    ID3D11Buffer* vb = nullptr; UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0, 1, &vb, &stride, &offset);
    ID3D11Buffer* ib = nullptr; DXGI_FORMAT ibFormat = DXGI_FORMAT_UNKNOWN; UINT ibOffset = 0;
    context->IAGetIndexBuffer(&ib, &ibFormat, &ibOffset);
    char verts[400] = {};
    int used = 0;
    std::vector<unsigned char> vbData, ibData;
    if (vb && stride >= 8 && ReadBytes(context, vb, vbData))
    {
        const bool indexed = call.kind == 0 || call.kind == 2;
        if (indexed && ib) ReadBytes(context, ib, ibData);
        const UINT count = call.a;
        const UINT first = call.kind == 0 ? call.b : call.kind == 1 ? call.b : call.kind == 2 ? call.c : call.e;
        for (UINT k = 0; k < count && k < 8 && used < 360; ++k)
        {
            UINT index = first + k;
            if (indexed && !ibData.empty())
            {
                const size_t at = ibOffset + static_cast<size_t>(index) * (ibFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
                if (at + 4 > ibData.size()) break;
                index = ibFormat == DXGI_FORMAT_R16_UINT ? *reinterpret_cast<const uint16_t*>(&ibData[at]) : *reinterpret_cast<const uint32_t*>(&ibData[at]);
                index += static_cast<UINT>(call.d);
            }
            const size_t at = offset + static_cast<size_t>(index) * stride;
            if (at + 8 > vbData.size()) break;
            float xy[2]; std::memcpy(xy, &vbData[at], 8);
            used += _snprintf_s(verts + used, sizeof(verts) - used, _TRUNCATE, " (%.4f,%.4f)", xy[0], xy[1]);
        }
    }
    D3D11_BUFFER_DESC vbDesc = {};
    if (vb) vb->GetDesc(&vbDesc);
    SafeRelease(vb); SafeRelease(ib);
    Log("[TILEDIAG] #%d kind %d a %u b %u c %u e %u d %d topology %d | QuadParams %.4f %.4f %.4f %.4f | scissor %s %ld,%ld-%ld,%ld (count %u) | vb %u bytes stride %u offset %u | verts%s",
        g_tileDiag, call.kind, call.a, call.b, call.c, call.e, call.d, static_cast<int>(topology), q[0], q[1], q[2], q[3],
        rd.ScissorEnable ? "ON" : "off", scissor.left, scissor.top, scissor.right, scissor.bottom, scissorCount,
        vbDesc.ByteWidth, stride, offset, verts);
}

#include "scope_clean61.inl"

void ObserveSelectorDraw292(ID3D11DeviceContext*,const DrawCall&,int);
void ObserveSelectorHand292(ID3D11DeviceContext*,const DrawCall&,int);

void HandleDraw(ID3D11DeviceContext* context, const DrawCall& call)
{
    if (context != g_ctx || !g_enabled || g_own)
    {
        Issue(context, call);
        return;
    }

    LaserDrawObserve206(context); // 207: an armed weapon depth is flushed once the game moves to another depth target
    // waterdiag14 captures retained locally; automatic readback disabled in waterfix15.
    ShotDrawProbe(context, call);
    RogueProbe(context, call);
    { // [GM94 PASSES] probe: every GM-94 draw (all passes) in 3 frames, 3 s apart - which draws, shaders, targets, and which are skipped
        static LONG frame=-1,samples=0;static ULONGLONG nextAt=0;
        if(Gm94Model()&&samples<3&&call.kind==0)
        {
            const auto d=NativeWeaponGeometry(context,call);
            if(d.vertexBytes==243072)
            {
                const ULONGLONG now=GetTickCount64();
                if(frame!=g_frame&&now>=nextAt){frame=g_frame;nextAt=now+3000;++samples;Log("[GM94 PASSES] frame %ld",static_cast<long>(g_frame));}
                if(frame==g_frame)
                {
                    ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto vh=ShaderHash(vs);SafeRelease(vs);
                    ID3D11RenderTargetView* rt[8]={};ID3D11DepthStencilView* ds=nullptr;context->OMGetRenderTargets(8,rt,&ds);UINT n=0;for(auto*& v:rt){if(v)++n;SafeRelease(v);}SafeRelease(ds);
                    Log("[GM94 PASSES]  start %u count %u stride %u offset %u vs %016llX targets %u%s",call.b,call.a,d.stride,d.offset,vh,n,Gm94SkipDraw(context,call)?" SKIPPED":"");
                }
            }
        }
    }
    if (Gm94SkipDraw(context, call) || FlareSkipDraw(context, call)) return; // parked shells / flare round: not drawn at all
    NearGunProbe(context, call);
    ReplayPhysicalHull(context);
    ReplayRevolverLoader(context,call); // speedloader body in the left hand (Manual revolver), on the barrel's own draw
    ObserveRevolverLoaderDraw(context,call); // snapshot the speedloader body the first time the game draws it
    ObserveBowArrowDraw(context,call);
    ReplayBowArrow(context,call);
    ObserveRpgRocketDraw(context,call); // RPG rocket: copied once; marks the frames the game draws it itself
    ReplayRpgRocket(context,call); // RPG rocket drawn by the mod while the game keeps it off (hand, insert, seated)
    if (PhysicalHullDraw(context, call))
    {
        RecordEjectionDraw(context, call, false);
        return;
    }

    if(g_inGameplay&&g_visibleWeaponDrawsLastFrame>0&&!fc3pad::NativeMenuOpen()&&fc3healthprobe::used<fc3healthprobe::Budget&&(fc3healthprobe::armedFrame<0||g_frame<=fc3healthprobe::armedFrame+2)&&call.kind==1&&call.a<=384){
        ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto vh=ShaderHash(vs);SafeRelease(vs);
        if(fc3healthprobe::Shader(vh)){ID3D11PixelShader* ps=nullptr;context->PSGetShader(&ps,nullptr,nullptr);const auto ph=ShaderHash(ps);SafeRelease(ps);const bool own=g_own;g_own=true;fc3healthprobe::BeforeDraw(context,call.kind,call.a,call.b,vh,ph,g_frame,true,false,kCbCopyGuid,kLayoutTextGuid);g_own=own;}
    }
    CaptureHandUi(context,call);
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
    bool posedNativeArm162 = false;
    bool handsCapable287=false,handsMotion287=false;
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

    CaptureEnvironmentDraw252(context,-1,-1);
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
        _snprintf_s(piece, sizeof(piece), _TRUNCATE, "] vp %.0fx%.0f vs %016llX ps %016llX%s in[",
                    viewport.Width, viewport.Height, ShaderHash(vs), ShaderHash(ps), pixelPatched ? "/PSFIX" : "");
        Append(text, sizeof(text), piece);
        DumpShaderBytecode(vs, "vs");
        DumpShaderBytecode(ps, "ps");
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
                    const auto geometry345=(differentProjection||PossibleWeaponGeometry345(context,call))?NativeWeaponGeometry(context,call):fc3weapon::Geometry{};
                    nativeWeapon=fc3weapon::Present(geometry345);
                    if(differentProjection||nativeWeapon)
                    {
                        const auto& geometry=geometry345;
                        ID3D11VertexShader* modelShader=nullptr;context->VSGetShader(&modelShader,nullptr,nullptr);
                        nativeModel=fc3weapon::Model(geometry)||ShaderHash(modelShader)==kArmVsHash;
                        SafeRelease(modelShader);
                    }
                    // Projection changes cannot turn arbitrary scenery into a
                    // first-person weapon. Model identity chooses that camera.
                    if (!fc3hands303::FirstPersonCamera345(differentProjection,nativeModel,nativeWeapon))
                    {
                        const bool changed = !g_frameCameraValid ||
                            std::memcmp(&g_frameCamera, &summary, sizeof(summary)) != 0;
                        ObserveRenderDraw(context,buffer,summary);
                        g_frameCamera = summary;
                        g_frameCameraValid = true;
                        TickCarWheel363(summary);
                        TickHandlebars355(summary);
                        g_worldProjection = { summary.xScale, summary.yScale, summary.zScale, summary.zOffset };
                        if (changed && summary.xScale > 0 && summary.yScale > 0 &&
                            std::isfinite(summary.zOffset) && std::fabs(summary.zOffset) > 1e-6f)
                        {
                            // Refresh from THIS draw, including while aim is paused.
                            // Earlier uploads/predictions cannot own these pixels.
                            if(g_nativeVehicleCamera279){NativeVehicleActual279(summary);UpdateEyeBuffers();}
                            else if (g_aimActive) AimActualCamera(summary);
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

        ObserveSelectorDraw292(context,call,variant);
        RecordVrBaselineDraw(context, call, variant, nativeWeapon); // observation only; never chooses a camera
        RecordMacheteDrawCensus(context,call,variant); // read-only identity of alternate knife draws after R3
        RecordHealAlternate217(context,call,variant);
        RecordEjectionDraw(context, call, variant == 1); // original draw, including world-space hull/effects
        ID3D11Buffer* armPalette = nullptr;
        UINT armPaletteSlot=3;
        ID3D11Buffer* worldOverride = nullptr;
        const bool nativeMachete116=MotionInteractionEnabled()&&MacheteHeld()&&NativeMacheteDraw116(context,call);
        bool carArm353=false;
        armPalette=BuildCarArmDraw353(context,call,carArm353);
        if(carArm353){
            // Car arms are drawn through the WORLD camera. Keep that camera;
            // do not send them through the on-foot weapon projection/route.
            weaponHidden=armPalette==nullptr;posedNativeArm162=armPalette!=nullptr;
        }
        else if(nativeMachete116)
        {
            CaptureMachete(context,call);
            armPalette=NativeMachetePalette116(context,call);
            // Never leave a frozen native blade beside the controller-owned tool.
            weaponHidden=armPalette==nullptr;
        }
        else if (variant == 1)
        {
            ++g_weaponDrawsThisFrame;
            probe66::Draw(context,call,0,0,false); // original constants before motion overrides
            RecordAnimationDraw(context, call);
            ObserveRevolverAssembly247(context,call);
            ObserveThrowable144(context,call);
            CaptureMolotovVisual158(context);
            Begin1911Trace46(context,call);
            RecordActionDraw(context, call); // original game palettes, before motion overrides
            // Which part of Jason is this? The two arm meshes go to their own
            // controllers; the rest of the body mesh stays with the body; any
            // other first-person draw (the gun) goes to the right hand.
            ID3D11VertexShader* shader = nullptr;
            context->VSGetShader(&shader, nullptr, nullptr);
            const unsigned long long hash = ShaderHash(shader);
            handsCapable287=hash==kArmVsHash&&shader&&HasFlag(shader,kHandsClipShader287);
            SafeRelease(shader);
            ID3D11Buffer* stream = nullptr;
            UINT stride = 0, offset = 0;
            context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
            if (hash == kArmVsHash)
            {
                const UINT start = call.b;
                if (start != g_armStarts[0] && start != g_armStarts[1])
                {
                    if (g_armStarts[0] == 0xffffffffu) g_armStarts[0] = start;
                    else g_armStarts[1] = start;
                    Log("[ARMS] arm mesh found (index start %u)", start);
                }
                const int side = MeasureArm(context);
                ObserveSelectorHand292(context,call,side);
                if (side >= 0 && stream) SetFlag(stream, kBodyVbGuid, 1); // only Jason's own mesh is the body mesh
                variant = (MotionInteractionEnabled()||ThrowableArmSide150(side)) ? (side == 1 ? 3 : 4) : 1;
                if (side >= 0)
                {
                    armPalette = BuildArmPalette(context, side);
                    posedNativeArm162 = armPalette != nullptr;
                    handsMotion287=posedNativeArm162&&side>=0&&HandsOnlyActive287();
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
                    g_bodyDrawStart = call.b; g_bodyDrawCount = call.a;
                    armPalette = BuildBodyPalette(context);
                    if(HandsOnlyActive287())weaponHidden=true;
                }
                if (!bodyPiece)
                {
                    if(nativeWeapon)++g_visibleWeaponDrawsThisFrame;
                    variant = MotionInteractionEnabled() ? 4 : 1;
                    // a skinned weapon piece: move its bones exactly like the hand's;
                    // a solid one: move its world matrix the same way
                    CaptureMachete(context,call);
                    const bool macheteHidden=MotionInteractionEnabled()&&MacheteHeld();
                    weaponHidden=macheteHidden||MagazineHideBullet(context,call)||fc3visual292::Hidden()||(MotionInteractionEnabled()&&fc3hands303::SuppressWeapon(fc3hands303::Read()));
                    if(macheteHidden)ReplayMachete(context);
                    if(!weaponHidden)
                    {
                        if(ThrowableVisual144(context)){
                            ReplayGrenadePin154(context,call);
                            weaponHidden=ThrowableHidden144()||PlaceableDrawHidden177(context)||ThrowablePinHidden154(context,call);
                            if(!weaponHidden){armPalette=ThrowablePalette144(context,call.b,call.a);if(!armPalette)worldOverride=ThrowableWorld144(context,call.b,call.a);}
                            variant=1;
                        }else{
                            worldOverride=BuildSvdLens236(context,call);
                            if(worldOverride)variant=1;
                            else {armPalette = RevolverAssemblyDraw217(context,call.b,call.a)?
                                BuildRevolverPalette217(context,call.b,call.a,armPaletteSlot):
                                BuildWeaponPalette(context, call.b, call.a,&armPaletteSlot);
                            if (armPalette) variant = 1;
                            else {worldOverride = BuildWeaponWorld(context, call.b, call.a);if (worldOverride) variant = 1;}}
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
        else if (ControllerBodyDraw151(variant))
        {
            // Jason's body mesh (shirt, sleeves) is drawn with the WORLD camera (10-08 records:
            // every body draw variant 0; on 10-02 it used the weapon camera). The sleeves still
            // share bones with the arms, so they take the arms' final bones from here as well.
            ID3D11Buffer* stream = nullptr;
            UINT stride = 0, offset = 0;
            context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
            if (stream && HasFlag(stream, kBodyVbGuid))
            {
                g_bodyDrawStart = call.b; g_bodyDrawCount = call.a;
                armPalette = BuildBodyPalette(context);
                if(HandsOnlyActive287()&&(call.b==0||call.b==3687||call.b==27747)){
                    ID3D11VertexShader* bodyShader287=nullptr;context->VSGetShader(&bodyShader287,nullptr,nullptr);
                    if(ShaderHash(bodyShader287)!=kArmVsHash)weaponHidden=true;SafeRelease(bodyShader287);
                }
            }
            SafeRelease(stream);
        }
        if ((variant == 1 || variant >= 3) && g_census != INVALID_HANDLE_VALUE)
            DumpWeaponDraw(context, call);

        ID3D11VertexShader* flameOriginal241=nullptr;
        const bool flameProjected241=patched&&ParticlePoolDraw243(context,call)&&BeginFlamerProjection241(context,flameOriginal241); // 243: every draw of the NDC particle pool, any weapon
        if(flameProjected241)variant=0;
        // Flat UI drawn straight onto the presented image gets its own depth.
        if (!flameProjected241 && variant == 0 && !inputStereo && rtv[0])
        {
            ID3D11Resource* target = nullptr;
            rtv[0]->GetResource(&target);
            if (target)
            {
                if (HasFlag(target, kBackbufferGuid)) variant = 2;
                target->Release();
            }
        }
        if(!worldOverride&&!armPalette)ObserveCarDoor357(context,call,variant);
        if(!worldOverride&&!armPalette)worldOverride=BuildCarWheelWorld352(context,call,variant);
        if(!worldOverride&&!armPalette)worldOverride=BuildHandlebarWorld355(context,call,variant);
        RepairEffectCensus(context,call,variant); // read-only census of blended draws while the repair tool is equipped
        if(!worldOverride&&!armPalette)worldOverride=BuildRepairToolEffectWorld(context,call,variant);
        if(!worldOverride&&!armPalette)worldOverride=BuildVectorCasingWorld231(context,call,variant);
        if(!worldOverride&&!armPalette)worldOverride=BuildRifleCasingWorld235(context,call,variant);
        const int ui = variant;
        ID3D11Buffer* repairOriginalVb=nullptr;UINT repairStride=0,repairOffset=0;
        if(!weaponHidden)BeginRepairToolParticleDraw(context,call,variant,repairOriginalVb,repairStride,repairOffset);
        if(!weaponHidden&&!repairOriginalVb)BeginMolotovParticles145(context,call,variant,repairOriginalVb,repairStride,repairOffset);
        if(!weaponHidden&&!repairOriginalVb)BeginFlamerDraw242(context,call,repairOriginalVb,repairStride,repairOffset); // 242: the flamethrower jet moved to the drawn nozzle
        if(!weaponHidden&&!repairOriginalVb)BeginMuzzleFlashDraw111(context,call,variant,repairOriginalVb,repairStride,repairOffset); // restored by the same End call
        ParticlePoolProbe243(context,call,variant,repairOriginalVb!=nullptr,flameProjected241); // 243: read-only census of blended dynamic draws while a jet/launch/shot is live
        if(!weaponHidden&&!worldOverride)ObservePistolGuideDraw(context,call,ui);

        g_own = true;
        ID3D11Buffer* gameBones = nullptr;
        ID3D11Buffer* gameGlobals = nullptr;
        if (armPalette)
        {
            context->VSGetConstantBuffers(armPaletteSlot, 1, &gameBones);
            context->VSSetConstantBuffers(armPaletteSlot, 1, &armPalette);
        }
        if (worldOverride)
        {
            context->VSGetConstantBuffers(0, 1, &gameGlobals);
            context->VSSetConstantBuffers(0, 1, &worldOverride);
        }
        // lighting tiles: the mod's vertex shader re-places them for each eye
        ID3D11VertexShader* tileGameVs = nullptr;
        ID3D11GeometryShader* tileGameGs = nullptr;
        ID3D11Buffer* tileGameCb = nullptr;
        ID3D11Buffer* tileGameGsCb = nullptr;
        bool tile = false;
        // the tile-test copy of the bound pixel shader, if it has one
        ID3D11PixelShader* tileGamePs = nullptr;
        ID3D11PixelShader* tileVariantPs = nullptr;
        bool tileTestDraw = false;
        if (g_tilePixelTest && !g_tileMeasureFrame && pixelPatched)
        {
            context->PSGetShader(&tileGamePs, nullptr, nullptr);
            UINT bytes = sizeof(tileVariantPs);
            if (tileGamePs && SUCCEEDED(tileGamePs->GetPrivateData(kPsTileVariantGuid, &bytes, &tileVariantPs)) && tileVariantPs)
                tileTestDraw = true;
        }
        ID3D11VertexShader* useTileVs = tileTestDraw ? g_tileVs : g_tileVsSimple;
        ID3D11GeometryShader* useTileGs = tileTestDraw ? g_tileGs : g_tileGsSimple;
        if (g_tileFixEnabled && useTileVs && useTileGs && g_tileCb[0] && (ui == 0 || ui == 2))
        {
            context->VSGetShader(&tileGameVs, nullptr, nullptr);
            tile = tileGameVs && ShaderHash(tileGameVs) == kTileVsHash;
            if (tile)
            {
                context->VSGetConstantBuffers(kTileCbSlot, 1, &tileGameCb);
                context->GSGetConstantBuffers(kTileCbSlot, 1, &tileGameGsCb);
                context->GSGetShader(&tileGameGs, nullptr, nullptr);
                if (g_tileDiag < 24 && g_gameplayFrames > 600 && !g_flatViewActive) TileDiag(context, call);
                context->VSSetShader(useTileVs, nullptr, 0);
                context->GSSetShader(useTileGs, nullptr, 0);
                if (tileTestDraw) context->PSSetShader(tileVariantPs, nullptr, 0);
                context->VSSetConstantBuffers(kTileCbSlot, 1, &g_tileCb[0]);
                // renderfix5-8 black: the tile-test geometry shader reads this
                // buffer and it was bound to the vertex stage only
                context->GSSetConstantBuffers(kTileCbSlot, 1, &g_tileCb[0]);
                InterlockedIncrement(&g_tileDraws);
            }
            else SafeRelease(tileGameVs);
        }
        if(nativeWeapon&&MotionInteractionEnabled()&&fc3hands303::SuppressWeapon(fc3hands303::Read()))weaponHidden=true;
        if(nativeWeapon&&fc3visual292::Preparing330())weaponHidden=true;
        if(nativeWeapon){const auto owner326=fc3hands303::Read();if(owner326.ownerContext326&&owner326.hand!=owner326.nativeHoldingSide){
            bool seated326=false;int seat326=-1;
            const bool nativeAction326=fc3aim::NativeActionOwnsArms128()||fc3pad::NativeMenuOpen()||MacheteHeld()||ThrowableOwns144()||(fc3aim::VehicleState(seated326,seat326)&&seated326);
            if(!nativeAction326&&fc3hands303::NativeFallback326(owner326,fc3weapon::ProfileIndex(NativeWeaponGeometry(context,call)),armPalette||worldOverride,MotionInteractionEnabled())){
                weaponHidden=true;static LONG reports326=0;if(InterlockedIncrement(&reports326)<=12)Log("[HANDS335] blocked native fallback frame=%ld profile=%d entity=%08X owner=%d native=%d active=%d visual=%d pose=%d motion=%d",g_frame,fc3weapon::ProfileIndex(NativeWeaponGeometry(context,call)),owner326.entity,owner326.hand,owner326.nativeHoldingSide,owner326.active,owner326.presentation314,armPalette||worldOverride,MotionInteractionEnabled());
            }
        }}
        HandsClipBinding287 handsClip287(context,handsCapable287,handsMotion287);
        // left eye: the game's own targets
        if (patched)
            context->VSSetConstantBuffers(dxbc::kEyeCbSlot, 1, &g_eyeCb[ui][0]);
        if (pixelPatched)
            context->PSSetConstantBuffers(dxbc::kEyeCbSlot, 1, (tile && tileTestDraw) ? &g_eyePsCbTile[0] : &g_eyePsCb[0]);
        ID3D11Query* tileQuery = nullptr;
        if (tile)
        {
            const int parity = g_frame & 1;
            if (g_tileQueryUsed[parity] < 160)
            {
                ID3D11Query*& q = g_tileQuery[parity][g_tileQueryUsed[parity]];
                if (!q) { D3D11_QUERY_DESC qd = { D3D11_QUERY_OCCLUSION, 0 }; g_dev->CreateQuery(&qd, &q); }
                if (q) { tileQuery = q; ++g_tileQueryUsed[parity]; g_tileQueryMeasure[parity] = g_tileMeasureFrame; }
            }
        }
        ScopeCleanDraw61(context,call,ui,nativeModel||nativeWeapon||armPalette!=nullptr,patched,pixelPatched,tile,tileTestDraw,rtCount,rtv,twinRtv,dsv,twinDsv,srv,twinSrv,slotMask);
        const bool scopeGlass60=ScopeNativeGlass60(context,call,ui);
        { // [SVD PASSES] probe (SVD lens and markings floated apart from the body): 3 frames, 3 s apart, every SVD-VB draw with its placement route
            static LONG svdFrame=-1,svdSamples=0;static ULONGLONG svdNext=0;
            if(g_weaponMechanicProfile==35&&svdSamples<3&&call.kind==0){const auto d=NativeWeaponGeometry(context,call);if(d.vertexBytes==268464){const ULONGLONG now=GetTickCount64();if(svdFrame!=g_frame&&now>=svdNext){svdFrame=g_frame;svdNext=now+3000;++svdSamples;}
                if(svdFrame==g_frame){ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto vh=ShaderHash(vs);SafeRelease(vs);ID3D11Buffer* cb0=nullptr;context->VSGetConstantBuffers(0,1,&cb0);D3D11_BUFFER_DESC bd={};if(cb0)cb0->GetDesc(&bd);SafeRelease(cb0);
                    Log("[SVD PASSES] frame %ld start %u count %u stride %u offset %u ib %u vs %016llX cb0 %u B variant %d patched %d worldOverride %d armPalette %d scopeGlass %d hidden %d nativeWeapon %d nativeModel %d",long(g_frame),call.b,call.a,d.stride,d.offset,d.indexBytes,vh,bd.ByteWidth,ui,patched?1:0,worldOverride?1:0,armPalette?1:0,scopeGlass60?1:0,weaponHidden?1:0,nativeWeapon?1:0,nativeModel?1:0);}}}
        }
        ID3D11RasterizerState* oldA2000Raster337=nullptr;bool a2000RasterChanged337=false;
        if(!BeginA2000Raster337(context,MirrorRasterSource349(worldOverride,armPalette),oldA2000Raster337,a2000RasterChanged337))weaponHidden=true;
        ID3D11Buffer* slideMaterial49=weaponHidden?nullptr:Begin1911Tint49(context,call,ui);
        if(!weaponHidden&&!scopeGlass60)CaptureSceneGrid258(context,0,true);
        if (tileQuery) context->Begin(tileQuery);
        if(!weaponHidden){if(!scopeGlass60)Issue(context, call);if(patched){probe66::Draw(context,call,1,0,scopeGlass60);CaptureScope60(context,call,worldOverride,ui,0,armPalette);}if(patched)MuzzleConfirm58(context,call,worldOverride?worldOverride:armPalette,ui);if(patched)CaptureLaserEye205(context,call,worldOverride?worldOverride:armPalette,ui,0);if(patched&&worldOverride)Confirm1911Frame(context,call.b,call.a,worldOverride,ui);if(patched&&worldOverride)Capture1911Guide48(context,call.b,call.a,worldOverride,ui,0);if(patched){CaptureWeaponGuides50(context,call,ui,0);CaptureBowGuides51(context,call,ui,0);CaptureBodyGuides51(context,call,ui,0);}End1911Trace46(context,call,ui);}
        if (tileQuery) context->End(tileQuery);
        if (!weaponHidden) CaptureShadowDraw(context, 0);
        if (!weaponHidden) CaptureWaterDraw(context, 0, ui, worldOverride != nullptr, armPalette != nullptr);
        if (!weaponHidden) CaptureEnvironmentDraw252(context,0,ui);

        // right eye: the twins
        for (UINT s = 0; s < kPsSlots; ++s)
            if (twinSrv[s]) context->PSSetShaderResources(s, 1, &twinSrv[s]);
        context->OMSetRenderTargets(rtCount, twinRtv, twinDsv);
        if (patched)
            context->VSSetConstantBuffers(dxbc::kEyeCbSlot, 1, &g_eyeCb[ui][1]);
        if (tile)
        {
            context->VSSetConstantBuffers(kTileCbSlot, 1, &g_tileCb[1]);
            context->GSSetConstantBuffers(kTileCbSlot, 1, &g_tileCb[1]);
        }
        if (pixelPatched)
            context->PSSetConstantBuffers(dxbc::kEyeCbSlot, 1, (tile && tileTestDraw) ? &g_eyePsCbTile[1] : &g_eyePsCb[1]);
        if(!weaponHidden){if(!scopeGlass60)Issue(context, call,1);if(patched){probe66::Draw(context,call,1,1,scopeGlass60);CaptureScope60(context,call,worldOverride,ui,1,armPalette);CaptureLaserEye205(context,call,worldOverride?worldOverride:armPalette,ui,1);}if(patched&&worldOverride)Capture1911Guide48(context,call.b,call.a,worldOverride,ui,1);if(patched){CaptureWeaponGuides50(context,call,ui,1);CaptureBowGuides51(context,call,ui,1);CaptureBodyGuides51(context,call,ui,1);}}
        if (!weaponHidden) CaptureShadowDraw(context, 1);
        if (!weaponHidden) CaptureWaterDraw(context, 1, ui, worldOverride != nullptr, armPalette != nullptr);
        if (!weaponHidden) CaptureEnvironmentDraw252(context,1,ui);

        if(!weaponHidden && patched && posedNativeArm162) CaptureLighterDepth161(context,dsv,twinDsv);
        End1911Tint49(context,slideMaterial49);
        EndA2000Raster337(context,oldA2000Raster337,a2000RasterChanged337);
        // put the game's bindings back
        if (tile)
        {
            context->VSSetShader(tileGameVs, nullptr, 0);
            context->GSSetShader(tileGameGs, nullptr, 0);
            if (tileTestDraw) context->PSSetShader(tileGamePs, nullptr, 0);
            context->VSSetConstantBuffers(kTileCbSlot, 1, &tileGameCb);
            context->GSSetConstantBuffers(kTileCbSlot, 1, &tileGameGsCb);
            SafeRelease(tileGameGsCb);
            SafeRelease(tileGameVs);
            SafeRelease(tileGameGs);
            SafeRelease(tileGameCb);
        }
        SafeRelease(tileGamePs);
        SafeRelease(tileVariantPs);
        EndRepairToolParticleDraw(context,repairOriginalVb,repairStride,repairOffset);
        if(flameOriginal241){context->VSSetShader(flameOriginal241,nullptr,0);SafeRelease(flameOriginal241);}
        if (armPalette)
        {
            context->VSSetConstantBuffers(armPaletteSlot, 1, &gameBones);
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
    ScopeCleanClear61(context,view,false,color);
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
    if (context == g_ctx && !g_own) LaserClearObserve206(context, view); // 207: flush an armed weapon depth before it is cleared
    g_oClearDsv(context, view, flags, depth, stencil);
    if (!Active(context) || !view) return;
    ScopeCleanClear61(context,view,true,nullptr,flags,depth,stencil);
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

void STDMETHODCALLTYPE HookClearUavUint61(ID3D11DeviceContext* c,ID3D11UnorderedAccessView* view,const UINT value[4]){g_oClearUavUint61(c,view,value);if(!Active(c)||g_scopeCleanOwners61.empty()||!view)return;ID3D11UnorderedAccessView* sources[2]={view,GetTwin<ID3D11UnorderedAccessView>(view)};for(int eye=0;eye<2;++eye){auto* source=sources[eye]?sources[eye]:view;auto* clean=static_cast<ID3D11UnorderedAccessView*>(TwinView(source,ViewKind::UnorderedAccess,false,ScopeCleanKey61(source,eye)));if(clean){g_oClearUavUint61(c,clean,value);SafeRelease(clean);}}SafeRelease(sources[1]);}
void STDMETHODCALLTYPE HookClearUavFloat61(ID3D11DeviceContext* c,ID3D11UnorderedAccessView* view,const FLOAT value[4]){g_oClearUavFloat61(c,view,value);if(!Active(c)||g_scopeCleanOwners61.empty()||!view)return;ID3D11UnorderedAccessView* sources[2]={view,GetTwin<ID3D11UnorderedAccessView>(view)};for(int eye=0;eye<2;++eye){auto* source=sources[eye]?sources[eye]:view;auto* clean=static_cast<ID3D11UnorderedAccessView*>(TwinView(source,ViewKind::UnorderedAccess,false,ScopeCleanKey61(source,eye)));if(clean){g_oClearUavFloat61(c,clean,value);SafeRelease(clean);}}SafeRelease(sources[1]);}

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
void NoteCameraBufferSize(UINT bytes);
void CarryCameraSummary(ID3D11Resource* dst, ID3D11Resource* src)
{
    if (!dst || !src) return;
    CameraSummary summary = {};
    UINT size = sizeof(summary);
    if (!(SUCCEEDED(src->GetPrivateData(kCameraSummaryGuid, &size, &summary)) && size == sizeof(summary))) return;
    if (!HasFlag(dst, kCameraGuid))
    {   // a camera copied into a buffer of another size (seen after LT / scope zoom): accept camera-sized constant buffers
        D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN; dst->GetType(&dimension);
        if (dimension != D3D11_RESOURCE_DIMENSION_BUFFER) return;
        D3D11_BUFFER_DESC desc = {}; static_cast<ID3D11Buffer*>(dst)->GetDesc(&desc);
        if (!(desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) || desc.ByteWidth < kCameraBufferSize || desc.ByteWidth > kSmallCbMax) return;
        if (summary.mainView) { static LONG logged = 0; if (InterlockedIncrement(&logged) <= 3) Log("[CAM] main-view camera copied into a %u-byte constant buffer (not the usual %u): now tracked", desc.ByteWidth, kCameraBufferSize); }
    }
    dst->SetPrivateData(kCameraSummaryGuid, sizeof(summary), &summary);
    if(NativeGuideShadowScope51()){unsigned char copy[kSmallCbMax]={};UINT copied=sizeof(copy);if(SUCCEEDED(src->GetPrivateData(kCbCopyGuid,&copied,copy))&&copied>=kCameraBufferSize&&copied<=sizeof(copy))dst->SetPrivateData(kCbCopyGuid,copied,copy);else dst->SetPrivateData(kCbCopyGuid,0,nullptr);}else dst->SetPrivateData(kCbCopyGuid,0,nullptr);
    if (summary.mainView) NoteCameraBufferSize(kCameraBufferSize + 1); // "via copy" marker in the report
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
    ScopeCleanCopy61(context,dst,src,0);
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
    ScopeCleanCopy61(context,dst,src,1,dstSubresource,x,y,z,srcSubresource,box);
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
    ScopeCleanCopy61(context,dst,src,2,dstSubresource,0,0,0,srcSubresource,nullptr,format);
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

    ScopeCleanDispatch61(context,x,y,z,srv,twinSrv,uav,twinUav);
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
// Only one number is needed from the game's camera: the horizontal projection
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
    if (xScale < 0.1f || xScale > 20.0f || g_bbH == 0)   // 0.1 = 168 degrees: a saved widened FOV factor (residue) must still be a camera
        return false;
    const float aspect = yScale / xScale;
    const float expected = static_cast<float>(g_bbW) / static_cast<float>(g_bbH);
    return std::fabs(aspect - expected) <= expected * 0.03f;    // not cube faces, spot shadows, probes
}

void NoteCameraBufferSize(UINT bytes)
{
    for (int i = 0; i < 8; ++i)
    {
        if (g_cameraBufferSizeCounts[i] && g_cameraBufferSizes[i] == bytes) { ++g_cameraBufferSizeCounts[i]; return; }
        if (!g_cameraBufferSizeCounts[i]) { g_cameraBufferSizes[i] = bytes; g_cameraBufferSizeCounts[i] = 1; return; }
    }
}
void ObserveCamera(ID3D11DeviceChild* buffer, const void* data, UINT bytes = kCameraBufferSize,
                   const void* caller = nullptr, UINT method = 0)
{
    float m[16];
    std::memcpy(m, data, sizeof(m));
    float xScale = 0, yScale = 0;
    const bool mainView = IsMainView(m, xScale, yScale);
    if (!mainView)
    {   // remember what a perspective-looking write was rejected for
        const float depthRow = std::sqrt(m[12] * m[12] + m[13] * m[13] + m[14] * m[14]);
        if (std::isfinite(xScale) && std::isfinite(yScale) && std::isfinite(depthRow) && depthRow > 0.9f && depthRow < 1.1f && xScale > 0.05f && xScale < 50.0f)
            for (RejectedCamera& r : g_rejectedCameras)
            {
                if (r.count && std::fabs(r.xScale - xScale) < 0.002f) { ++r.count; break; }
                if (!r.count) { r = { xScale, yScale, depthRow, 1 }; break; }
            }
    }
    else NoteCameraBufferSize(bytes);
    if (buffer)
    {
        CameraSummary summary = {};
        summary.mainView = mainView ? 1u : 0u;
        static const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"FC3_d3d11.dll"));
        const auto address=reinterpret_cast<uintptr_t>(caller);
        if(base&&address>=base&&address-base<48332800u)summary.uploadCallerRva=UINT(address-base);
        summary.uploadMethod=method;
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
                keep = ((desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) &&
                        (desc.ByteWidth <= kSmallCbMax || desc.ByteWidth == kBonesBytes)) ||
                       (desc.ByteWidth >= kCameraBufferSize && desc.ByteWidth <= kSmallCbMax); // camera-sized staging buffers too
                keep=keep||ThrowableKeepVertexShadow145(desc);
                keep=keep||RepairKeepVertexShadow(desc); // exact torch: small dynamic particle VB, CPU upload only
                keep=keep||MuzzleFlashKeepVertexShadow111(desc); // 1 s after a local shot: pooled particle VBs (flash/tracer)
                keep=keep||HandUiKeepVertexShadow(desc); // exact native HUD pool; CPU writes only, no GPU/world capture
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
                ObserveCamera(resource, pointer, kCameraBufferSize, _ReturnAddress(), 1);
                if(NativeGuideShadowScope51()){D3D11_BUFFER_DESC desc={};static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);if(desc.ByteWidth<=kSmallCbMax)resource->SetPrivateData(kCbCopyGuid,desc.ByteWidth,pointer);}else resource->SetPrivateData(kCbCopyGuid,0,nullptr);
            }
            else
            {
                D3D11_BUFFER_DESC desc = {};
                static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);
                resource->SetPrivateData(kCbCopyGuid, desc.ByteWidth, pointer);
                RepairRememberVertexCopy(resource);
                // The camera is recognised by CONTENT too: after an LT / scope zoom the world camera stopped arriving in the
                // 896-byte buffer and the mod was left with the weapon camera only (no motion controls, no recording, wrong FOV).
                if (!(desc.BindFlags&(D3D11_BIND_VERTEX_BUFFER|D3D11_BIND_INDEX_BUFFER))&&desc.ByteWidth >= kCameraBufferSize && desc.ByteWidth <= kSmallCbMax) ObserveCamera(resource, pointer, desc.ByteWidth, _ReturnAddress(), 1);
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
    if (context == g_ctx && !g_own && resource && source && !box)
    {
        if (HasFlag(resource, kCameraGuid)){ObserveCamera(resource, source, kCameraBufferSize, _ReturnAddress(), 2);if(NativeGuideShadowScope51()){D3D11_BUFFER_DESC desc={};static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);if(desc.ByteWidth<=kSmallCbMax)resource->SetPrivateData(kCbCopyGuid,desc.ByteWidth,source);}else resource->SetPrivateData(kCbCopyGuid,0,nullptr);}
        else
        {
            D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN; resource->GetType(&dimension);
            if (dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
            {
                D3D11_BUFFER_DESC desc = {}; static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);
                if ((desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) && desc.ByteWidth >= kCameraBufferSize && desc.ByteWidth <= kSmallCbMax) ObserveCamera(resource, source, desc.ByteWidth, _ReturnAddress(), 2);
                // The CPU copy every weapon path reads (kCbCopyGuid) was only kept on the Map/Unmap route. The R700 scope lens
                // (VS F9AF) gets its 544-byte globals through UpdateSubresource, so the mod saw zeros and could not move it:
                // the lens stayed on the game's hidden hand animation. Keep the copy here too (same rule as HookUnmap).
                if ((desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) && (desc.ByteWidth <= kSmallCbMax || desc.ByteWidth == kBonesBytes))
                {
                    resource->SetPrivateData(kCbCopyGuid, desc.ByteWidth, source);
                    static LONG logged = 0; if (InterlockedIncrement(&logged) == 1) Log("[CB] constant buffers filled by UpdateSubresource are mirrored now (first: %u bytes)", desc.ByteWidth);
                }
                RepairIndexUploaded(resource, source, false); // particle index buffers (repair flame): mirror whole uploads
            }
        }
    }
    else if (context == g_ctx && !g_own && resource && source && box)
    { // a partial update: patch the existing copy in place
        D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN; resource->GetType(&dimension);
        if (dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
        {
            D3D11_BUFFER_DESC desc = {}; static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);
            if ((desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) && desc.ByteWidth <= kSmallCbMax && box->right > box->left && box->right <= desc.ByteWidth)
            {
                unsigned char copy[kSmallCbMax] = {}; UINT bytes = sizeof(copy);
                if (SUCCEEDED(resource->GetPrivateData(kCbCopyGuid, &bytes, copy)) && bytes == desc.ByteWidth)
                { std::memcpy(copy + box->left, source, box->right - box->left); resource->SetPrivateData(kCbCopyGuid, desc.ByteWidth, copy); }
            }
            RepairIndexUploaded(resource, source, true); // partial index upload: drop the cached copy
        }
    }
    g_oUpdateSubresource(context, resource, subresource, box, source, rowPitch, depthPitch);
}

HRESULT STDMETHODCALLTYPE HookCreateBuffer(
    ID3D11Device* device, const D3D11_BUFFER_DESC* desc, const D3D11_SUBRESOURCE_DATA* initial,
    ID3D11Buffer** buffer)
{
    const HRESULT result = g_oCreateBuffer(device, desc, initial, buffer);
    // Particle draws use a pooled VB plus an index range. Retain small initial
    // index uploads so repair attachment can select ACTUALLY DRAWN vertices.
    // CPU data already supplied by the game; never GPU readback.
    if(SUCCEEDED(result)&&!g_own&&desc&&buffer&&*buffer&&initial&&initial->pSysMem&&
       (desc->BindFlags&D3D11_BIND_INDEX_BUFFER)&&desc->ByteWidth<=1024*1024)
        (*buffer)->SetPrivateData(kCbCopyGuid,desc->ByteWidth,initial->pSysMem);
    if (SUCCEEDED(result) && desc && buffer && *buffer && desc->ByteWidth == kCameraBufferSize &&
        !(desc->BindFlags&(D3D11_BIND_VERTEX_BUFFER|D3D11_BIND_INDEX_BUFFER)))
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
            bool handsPatched287=false;std::vector<uint8_t> originalArm287;
            if(arm)originalArm287=patched;
            if(arm){std::vector<uint8_t> clipped;if(dxbc::PatchHandsOnly287(patched.data(),patched.size(),clipped)==dxbc::Result::Patched){patched.swap(clipped);handsPatched287=true;}}
            HRESULT result = g_oCreateVertexShader(
                device, patched.data(), patched.size(), linkage, shader);
            if(FAILED(result)&&handsPatched287){SafeRelease(*shader);handsPatched287=false;result=g_oCreateVertexShader(device,originalArm287.data(),originalArm287.size(),linkage,shader);Log("[HANDS287] optional clip shader rejected; existing arm stereo shader retained");}
            if (SUCCEEDED(result) && *shader)
            {
                if(handsPatched287){SetFlag(*shader,kHandsClipShader287,1);g_handsClipAvailable287=true;g_handsShaderDevice287=device;}
                const unsigned long long hash = Fnv64(bytecode, size);
                if (hash == 0x2287E89CC64747EDull || hash == 0xE01EBD2966163B82ull || hash == 0xF23C874C31A8D0A0ull || hash == 0x97ADE3799CCB9D8Dull || hash == 0x19F9BF97F64AD98Dull)
                    Log("[SCREENFIX] %016llX depth/refraction outputs follow the eye clip position", hash);
                for(auto effect:{0x69AD5DA00175E7FCull,0xF753F9846F8728BAull,0x279EA22F666BB930ull,0xAE5D8C51FA18F38Cull,0xA67FF6778C62D6ACull,0x25367B004D183632ull,0xE75BBB615C9C2AEEull,0xA985EF48D466B435ull})if(hash==effect)Log("[ENV254] vertex depth-coordinate correction created %016llX",hash);
                if(hash==0x4ABB679B228FE247ull)Log("[ENV256] cave water vertex depth-coordinate correction created %016llX",hash);
                if(hash==0x1981348762D3E19Cull)Log("[ENV257] atmospheric particle vertex depth-coordinate correction created %016llX",hash);
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
        bool tileTest = false;
        const dxbc::PixelResult outcome = dxbc::PatchPixelShader(bytecode, size, patched, &tileTest);
        if (outcome == dxbc::PixelResult::Patched)
        {
            result = g_oCreatePixelShader(device, patched.data(), patched.size(), linkage, shader);
            patchedPixel = SUCCEEDED(result) && *shader;
        }
        if (patchedPixel)
        {
            std::vector<uint8_t> tilePatched;
            bool tileAdded = false;
            if (dxbc::PatchPixelShader(bytecode, size, tilePatched, &tileAdded, true) == dxbc::PixelResult::Patched && tileAdded)
            {
                ID3D11PixelShader* variant = nullptr;
                if (SUCCEEDED(g_oCreatePixelShader(device, tilePatched.data(), tilePatched.size(), linkage, &variant)) && variant)
                {
                    (*shader)->SetPrivateDataInterface(kPsTileVariantGuid, variant);
                    variant->Release();
                    InterlockedIncrement(&g_psTileTest);
                }
            }
        }
        if (outcome != dxbc::PixelResult::NotApplicable && !patchedPixel)
            Log("[PS] depth-unprojecting shader left unpatched (size %u)", static_cast<unsigned>(size));
    }
    if (!patchedPixel)
        result = g_oCreatePixelShader(device, bytecode, size, linkage, shader);
    if (patchedPixel)
    {
        const auto effectHash254=Fnv64(bytecode,size);
        for(auto effect:{0xC19BFBC792F0F843ull,0xBA81B5F7771569B2ull,0x057EE039AB2B7A3Eull,0xE20ABD31F16921EAull,0x83A6FCAF6854B4EDull,0xB42B55CCCF14B3CEull,0x919B2333D357EBB2ull,0xFFD1D36205DC7757ull,0x22ECA6952F53DBD7ull,0x3E11D1C4732A23E3ull,0x6CEEF5F8E48054A5ull})if(effectHash254==effect)Log("[ENV254] pixel native-depth correction created %016llX",effectHash254);
        if(effectHash254==0xF8F31518956C98DBull)Log("[ENV256] cave water pixel depth/reflection correction created %016llX",effectHash254);
        if(effectHash254==0x6771A60187C7067Full)Log("[ENV257] atmospheric particle pixel native-depth correction created %016llX",effectHash254);
        if (Fnv64(bytecode, size) == 0x5E139F4BB30305E5ull || Fnv64(bytecode, size) == 0xA1F37111CC1926D5ull)
            Log("[WATERFIX13] river depth and mono-reflection screen coordinates reconstructed in game space");
        if (Fnv64(bytecode, size) == 0xB5850815B1E93395ull || Fnv64(bytecode, size) == 0xF9BD64B91671E5FEull)
            Log("[WATERFIX15] diffuse river depth mapped to eye and reconstructed using its CB0 viewport");
        if (Fnv64(bytecode, size) == 0xA577E6A5E9ECCB71ull)
            Log("[MOTIONFIX16] native camera motion-blur sampling velocity disabled");
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
extern bool g_vehicleLookEnabled;extern float g_vehicleLookGain; // vehicle_look102.inl
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
    LoadManualReloadSettings();ApplyManualReloadMode();LoadPistolReloadSettings();LoadSupportGrab214(path);LoadMacheteSettings(path);LoadMacheteBladeContactSettings(path);LoadReloadSignalSettings(path);LoadHandsOnly287(path);
    g_twoHandRecoilAssist=(std::max)(0.0f,(std::min)(1.0f,GetPrivateProfileIntW(L"MotionControls",L"TwoHandRecoilAssist",100,path)/100.0f));
    g_autoPistolSupport=GetPrivateProfileIntW(L"MotionControls",L"AutoPistolSupport",1,path)!=0;
    g_autoLongGunSupport=GetPrivateProfileIntW(L"MotionControls",L"AutoLongGunSupport",0,path)!=0;
    fc3pad::SetGripOnlyAmmo(GetPrivateProfileIntW(L"MotionControls",L"GripOnlyAmmo",0,path)!=0);
    g_nativeUiMouseEnabled=GetPrivateProfileIntW(L"MotionControls",L"NativeMenuMouse",1,path)!=0;
    g_nativeUiRayEnabled298=GetPrivateProfileIntW(L"MotionControls",L"NativeMenuRay",1,path)!=0;
    fc3pad::SetSwapBX(GetPrivateProfileIntW(L"MotionControls",L"SwapBX",0,path)!=0);
    g_rifleRecoilAssist=(std::max)(0.0f,(std::min)(1.0f,GetPrivateProfileIntW(L"MotionControls",L"RifleRecoilAssist",50,path)/100.0f));
    g_enabled = ReadFloat(path, L"Enabled", 1.0f) != 0.0f;
    LoadHandUiSettings(path);LoadReticleColour52(path);LoadScopeSettings60(path);LoadGodMode123(path);LoadCalibration136(path);LoadThrowables144(path);
    g_vehicleLookEnabled=GetPrivateProfileIntW(L"Vehicle",L"HeadLook",1,path)!=0;
    g_vehicleLookGain=(std::max)(.2f,(std::min)(10.f,GetPrivateProfileIntW(L"Vehicle",L"LookGainPercent",200,path)/100.f));
    fc3pad::SetVehicleSmoothing363(false,float(GetPrivateProfileIntW(L"Vehicle",L"CarSmoothing363",45,path)));
    fc3pad::SetVehicleSmoothing363(true,float(GetPrivateProfileIntW(L"Vehicle",L"BarSmoothing363",70,path)));
    g_doors357Enabled=false; // 365: removed; the saved Doors357 value is ignored
    for(int h=0;h<2;++h){const wchar_t* sideKey[2]={L"HandRimPullCm366R",L"HandRimPullCm366L"},*outKey[2]={L"HandRimOutCm366R",L"HandRimOutCm366L"};
        g_handRimPullCm366[h]=(std::max)(-5.f,(std::min)(15.f,GetPrivateProfileIntW(L"Vehicle",sideKey[h],h?4:4,path)*1.f));
        g_handRimOutCm366[h]=(std::max)(-8.f,(std::min)(12.f,GetPrivateProfileIntW(L"Vehicle",outKey[h],h?1:6,path)*1.f));}
    fc3pad::SetStickThrottle365(GetPrivateProfileIntW(L"Vehicle",L"StickThrottle365",1,path)!=0);
    g_autoHeadsetCapture366=GetPrivateProfileIntW(L"Capture",L"HeadsetGameplayAuto366",0,path)!=0;
    fc3pad::SetCarWheelFullLockDegrees(static_cast<float>(GetPrivateProfileIntW(L"Vehicle",L"WheelFullLockDegrees",90,path)));
    fc3pad::SetCarWheelSteeringEnabled(GetPrivateProfileIntW(L"Vehicle",L"PhysicalWheel353",1,path)!=0);
    fc3pad::SetHandlebarEnabled355(GetPrivateProfileIntW(L"Vehicle",L"Handlebars355",1,path)!=0);
    fc3pad::SetHandlebarLock355(static_cast<float>(GetPrivateProfileIntW(L"Vehicle",L"HandlebarFullLock355",35,path)));
    Log("[BARS355] ATV/jetski steering loaded; enabled=%d lock=%.1f; car354 retained",fc3pad::HandlebarEnabled355(),fc3pad::HandlebarLock355());
    Log("[CAR353] automatic tracked driver hands, physical wheel grips loaded; enabled=%d fullLock=%.1f; weapons/reloads/holsters retained from351",fc3pad::CarWheelSteeringEnabled(),fc3pad::CarWheelFullLockDegrees());
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
    g_shoulderSpot[0] = ReadFloat(path, L"ShoulderX", static_cast<float>(g_shoulderSpot[0]));
    g_shoulderSpot[1] = ReadFloat(path, L"ShoulderY", static_cast<float>(g_shoulderSpot[1]));
    g_shoulderSpot[2] = ReadFloat(path, L"ShoulderZ", static_cast<float>(g_shoulderSpot[2]));
    Log("[ARMS] shoulder spot (right, metres from the eyes: x right, y up, z forward) %.3f %.3f %.3f; left mirrored",
        g_shoulderSpot[0], g_shoulderSpot[1], g_shoulderSpot[2]);
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
bool g_carArmScope352=false;Mat4 g_carBodyView352={};
ULONGLONG g_physicalAt352=0;
void SelectorContactMove292(Mat4&,const Mat4&);
void SelectorFingers292(float*);

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

// Candidate84: reticle73 hands and guns, with ONLY the candidate77/78 arm solver
// (elbow bend from a body-relative pole, fixed forward visual shoulder, bounded
// extension). The hand's source and the gun root are the accepted reticle73 ones.
#include "calibration136_core.inl"
bool g_armIk84 = true; // offline negative-control switch
// The wrist stays on the controller. The elbow is inferred from a body-relative
// pole, never from Jason's weapon animation. A bounded longitudinal extension
// reduces the old forward shoulder slide without enlarging the arm's width.
V3 g_armSlide[2] = {};
double g_armStretch77[2] = { 1.0, 1.0 };
V3 g_armElbow77[2] = {};
Mat4 g_armForearmRigid77[2] = {};
Mat4 StretchArmSegment77(V3 start,V3 end,double factor)
{
    const V3 axis=Unit(Sub(end,start));
    const double a[3]={axis.x,axis.y,axis.z},p[3]={start.x,start.y,start.z};
    Mat4 m={};m.m[15]=1.0;
    for(int row=0;row<3;++row){
        for(int col=0;col<3;++col)m.m[row*4+col]=(row==col?1.0:0.0)+(factor-1.0)*a[row]*a[col];
        m.m[row*4+3]=p[row]-(m.m[row*4+0]*p[0]+m.m[row*4+1]*p[1]+m.m[row*4+2]*p[2]);
    }
    return m;
}
bool SolveArm(const ArmJoints& joints, Mat4& handMove, int side, Mat4& forearmMove, Mat4& upperMove,
              bool keepHand = false,bool newIk77 = false)
{
    (void)keepHand; // the hand ALWAYS stays on the controller
    if (!joints.valid) return false;
    const V3 elbow = { joints.e[0], joints.e[1], joints.e[2] };
    const V3 wrist = { joints.w[0], joints.w[1], joints.w[2] };
    const V3 stubTop = { joints.s[0], joints.s[1], joints.s[2] };   // where the mesh's upper arm starts
    // The arm mesh is short. Its visible start is in front of Jason's shirt;
    // use a fixed forward visual shoulder so a closer hand can actually bend
    // the elbow, then let the blended shirt sleeve reach that shoulder.
    V3 calibratedShoulder136={};const bool calibrated136=newIk77&&CalibrationShoulder136(side,calibratedShoulder136);
    const double spotBody[3] = {calibrated136?calibratedShoulder136.x:(side==0?g_shoulderSpot[0]:-g_shoulderSpot[0]),calibrated136?calibratedShoulder136.y:g_shoulderSpot[1],calibrated136?calibratedShoulder136.z:g_shoulderSpot[2]+(newIk77?.30:0.)};
    const double anatomicalBody[3] = {spotBody[0],spotBody[1],calibrated136?spotBody[2]:g_shoulderSpot[2]};
    V3 shoulderSpot = { spotBody[0], spotBody[1], spotBody[2] };
    V3 anatomicalSpot = { anatomicalBody[0],anatomicalBody[1],anatomicalBody[2] };
    if (g_aimActive)
    {
        shoulderSpot = { g_aimQ[0][0] * spotBody[0] + g_aimQ[0][1] * spotBody[1] + g_aimQ[0][2] * spotBody[2],
                         g_aimQ[1][0] * spotBody[0] + g_aimQ[1][1] * spotBody[1] + g_aimQ[1][2] * spotBody[2],
                         g_aimQ[2][0] * spotBody[0] + g_aimQ[2][1] * spotBody[1] + g_aimQ[2][2] * spotBody[2] };
        anatomicalSpot = { g_aimQ[0][0]*anatomicalBody[0]+g_aimQ[0][1]*anatomicalBody[1]+g_aimQ[0][2]*anatomicalBody[2],
                           g_aimQ[1][0]*anatomicalBody[0]+g_aimQ[1][1]*anatomicalBody[1]+g_aimQ[1][2]*anatomicalBody[2],
                           g_aimQ[2][0]*anatomicalBody[0]+g_aimQ[2][1]*anatomicalBody[1]+g_aimQ[2][2]*anatomicalBody[2] };
    }
    // Keep the mesh's measured segment lengths as the rest lengths. The current
    // design allows controlled extension to reduce the bent-elbow look.
    const double upperLength = (std::max)(0.12, (std::min)(0.45, Length(Sub(elbow, stubTop))));
    const double forearmLength = (std::max)(0.05, Length(Sub(wrist, elbow)));
    const double natural = upperLength + forearmLength;
    const V3 target = Apply(handMove, wrist);           // the controller's wrist point: the hand goes HERE, always
    V3 shoulder = shoulderSpot;
    V3 toTarget = Sub(target, shoulder);
    double reach = Length(toTarget);
    if (reach < 1e-6) return false;
    V3 along = Scale(toTarget, 1.0 / reach);
    // Fixed segment lengths are essential: adapting stretch to every wrist
    // position held the elbow nearly straight at all times in candidate77.
    // The remaining slide is used only past the available reach.
    const double stretch=calibrated136?CalibrationUpperScale136(upperLength,forearmLength,side):(newIk77?1.35:1.0);
    const double forearmStretch136=calibrated136?1.0:stretch;
    const double extendedUpper=upperLength*stretch,extendedForearm=forearmLength*forearmStretch136;
    const double reachLimit139=calibrated136&&CalibrationCurrent136().forwardFit?.9995:(newIk77?.985:.92);
    const double furthest=(extendedUpper+extendedForearm)*reachLimit139;
    if (reach > furthest)
    {
        shoulder = Sub(target, Scale(along, furthest));
        toTarget = Sub(target, shoulder);
        reach = furthest;
    }
    const double shortest = std::fabs(extendedUpper - extendedForearm) + 0.01;
    if (reach < shortest) {if(calibrated136)shoulder=Sub(target,Scale(along,shortest));reach=shortest;}
    const double a = (extendedUpper * extendedUpper - extendedForearm * extendedForearm + reach * reach) / (2.0 * reach);
    const double h = std::sqrt((std::max)(extendedUpper * extendedUpper - a * a, 0.0));
    const V3 gameLine=Unit(Sub(wrist,stubTop));
    V3 bend=newIk77?V3{side == 0 ? 0.8 : -0.8,-0.6,-0.15}:Sub(elbow,Add(stubTop,Scale(gameLine,Dot(Sub(elbow,stubTop),gameLine))));
    if(newIk77&&g_aimActive)bend={g_aimQ[0][0]*bend.x+g_aimQ[0][1]*bend.y+g_aimQ[0][2]*bend.z,
                         g_aimQ[1][0]*bend.x+g_aimQ[1][1]*bend.y+g_aimQ[1][2]*bend.z,
                         g_aimQ[2][0]*bend.x+g_aimQ[2][1]*bend.y+g_aimQ[2][2]*bend.z};
    bend = Sub(bend, Scale(along, Dot(bend, along)));
    if (Length(bend) < 0.01)
    {
        bend = newIk77?V3{0.0,-1.0,0.0}:V3{side == 0 ? 0.5:-0.5,-1.0,-0.2};
        bend = Sub(bend, Scale(along, Dot(bend, along)));
    }
    const V3 newElbow = Add(Add(shoulder, Scale(along, a)), Scale(Unit(bend), h));
    const V3 newWrist = Add(shoulder, Scale(along, reach));
    // Align and extend each segment along its own length; its width stays fixed.
    const Mat4 upperRigid=TurnAndMove(Sub(elbow, stubTop), Sub(newElbow, shoulder), stubTop, shoulder);
    upperMove=newIk77?Multiply(upperRigid,StretchArmSegment77(stubTop,elbow,stretch)):upperRigid;
    // The forearm's rigid component is kept separately for wrist-twist math.
    g_armForearmRigid77[side]=TurnAndMove(Sub(wrist, elbow), Sub(newWrist, newElbow), elbow, newElbow);
    forearmMove=newIk77?Multiply(g_armForearmRigid77[side],StretchArmSegment77(elbow,wrist,forearmStretch136)):g_armForearmRigid77[side];
    g_armSlide[side] = Sub(shoulder, anatomicalSpot);
    g_armStretch77[side]=stretch;
    g_armElbow77[side]=newElbow;
    return true;
}

// Reads this draw's world matrix, camera and bone palette copies and builds
// its model -> first-person view mapping v = G m + g (as a matrix) and inverse.
// SPAS-12 (profile 34): its unskinned VS 31524177ACA0314B / 4C0DBE621F6449A7 draws keep a vector in c0 and the
// world rows at cb0 floats [4:16] (fractions at [16:19]); only the rigid VS C386478F2F9495EB has them at [0:12].
// Writing the moved world at [0:12] for those draws flung the body, slide and loading gate hundreds of metres away
// (SPAS parts went missing under motion controls).
int WeaponWorldOffset(ID3D11DeviceContext* context,bool* hasPositionFractions=nullptr)
{
    if(hasPositionFractions)*hasPositionFractions=false;
    // Layout belongs to the shader, not the equipped weapon. The AlphaValues
    // prefix also occurs on pistol/SMG streams, so the former SPAS/.44-only
    // exception left those passes writing into the preceding vector.
    static const GUID layoutGuid={0x92df2180,0x76e4,0x41ca,{0x9e,0x42,0xc7,0x45,0x63,0x18,0x00,0x01}};
    ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);
    UINT encoded=0,size=sizeof(encoded);bool cached=vs&&SUCCEEDED(vs->GetPrivateData(layoutGuid,&size,&encoded));
    if(vs&&!cached){
        UINT bytes=0;std::vector<BYTE> code;
        if(SUCCEEDED(vs->GetPrivateData(kEjectionBytecodeGuid,&bytes,nullptr))&&bytes&&bytes<=65536){
            code.resize(bytes);
            if(SUCCEEDED(vs->GetPrivateData(kEjectionBytecodeGuid,&bytes,code.data()))){
                ID3D11ShaderReflection* reflection=nullptr;
                if(SUCCEEDED(D3DReflect(code.data(),bytes,IID_ID3D11ShaderReflection,reinterpret_cast<void**>(&reflection)))){
                    D3D11_SHADER_INPUT_BIND_DESC binding={};D3D11_SHADER_VARIABLE_DESC world={},fractions={};
                    auto* globals=reflection->GetConstantBufferByName("$Globals");
                    if(SUCCEEDED(reflection->GetResourceBindingDescByName("$Globals",&binding))&&binding.BindPoint==0&&
                       globals&&SUCCEEDED(globals->GetVariableByName("_WorldMatrix")->GetDesc(&world))&&
                       SUCCEEDED(globals->GetVariableByName("_WorldPositionFractions")->GetDesc(&fractions))&&
                       world.Size==48&&world.StartOffset%16==0&&world.StartOffset<=64&&
                       fractions.Size==12&&fractions.StartOffset==world.StartOffset+48)
                        encoded=world.StartOffset/4+1;
                    SafeRelease(reflection);
                }
            }
        }
        vs->SetPrivateData(layoutGuid,sizeof(encoded),&encoded);
    }
    const auto hash=ShaderHash(vs);SafeRelease(vs);
    if(encoded){if(hasPositionFractions)*hasPositionFractions=true;return int(encoded)-1;}
    // Preserve the known fallback for old/incomplete shader metadata.
    if((g_weaponMechanicProfile==34||g_weaponMechanicProfile==42)&&g_frame-g_weaponMechanicFrame<=2&&
       (hash==0x31524177ACA0314Bull||hash==0x4C0DBE621F6449A7ull)){if(hasPositionFractions)*hasPositionFractions=true;return 4;}
    return 0;
}
bool DrawSpace(ID3D11DeviceContext* context, float bones[48 * 12], Mat4& toView, Mat4& fromView,UINT paletteSlot=3,bool fullWeaponWorld=false)
{
    bool hasPositionFractions=false;
    const int off = WeaponWorldOffset(context,&hasPositionFractions);
    fullWeaponWorld=fullWeaponWorld&&hasPositionFractions;
    ID3D11Buffer* buffers[4] = {};
    context->VSGetConstantBuffers(0, 4, buffers);
    if(bones&&paletteSlot!=3){SafeRelease(buffers[3]);context->VSGetConstantBuffers(paletteSlot,1,&buffers[3]);}
    float globals[kSmallCbMax / 4] = {};
    CameraSummary camera = {};
    bool ok = ReadCopy(buffers[0], globals, sizeof(globals)) >= static_cast<UINT>((off + (fullWeaponWorld?15:12)) * 4);
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
            toView.m[r * 4 + c] = axis[0] * globals[off + 0 * 4 + c] + axis[1] * globals[off + 1 * 4 + c] + axis[2] * globals[off + 2 * 4 + c];
        double t[3];
        for (int k = 0; k < 3; ++k) t[k] = static_cast<double>(globals[off + k * 4 + 3]) + (fullWeaponWorld?globals[off+12+k]:0) - camera.position[k];
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
Mat4 g_guideShaderPlacement[kEyeVariants]={};
LONG g_guideShaderPlacementFrame[kEyeVariants]={};
bool g_twoHand = false;
bool g_neutralQuietGate = true; // tests switch the quiet-arm capture gate off
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
void M133SightUploaded57(ID3D11DeviceContext*,UINT,UINT,UINT);bool M133SightDraw57(ID3D11DeviceContext*,UINT,UINT);bool M133SightWorld57(ID3D11DeviceContext*,UINT,UINT,float*);void M133SightObserveFinal57(ID3D11DeviceContext*,const Mat4&,const float*,UINT,UINT);
bool M133AimSupport56(const ArmJoints&,const Mat4&,V3&);void CorrectShotHandMove(const ArmJoints&,Mat4&);bool ShotgunSupportNative(const ArmJoints&,V3&);
void CorrectPistolHandMove(const ArmJoints&,Mat4&);
bool PistolPoseOwned();
bool PistolOwnsLeft();
bool M133ControllerSupport356();
bool AkSupportPosition(V3&);
bool AkReloadSupport();
void TickPistolReload(bool);
void PistolGuideMarkers();
bool PistolReloadNote();
bool Lever1887Note();bool RevolverNote();bool SpasNote();bool FlareNote();
bool SpasModel();bool SpasActive();bool SpasManual();void UpdateSpasFire(const fc3pad::Controllers&);void TickSpas(bool);void SpasGuideMarkers();bool SpasNativeLeft();void SpasLeftNativeMove(Mat4&);bool SpasClipPlaying();bool SpasLeftClipMove(const Mat4&,const float*,Mat4&);bool SpasLeftGrip(float*);void RecordSpasLeftHand(ID3D11DeviceContext*,const float*);bool SpasLeftSupportMove(const Mat4&,const float*,Mat4&);bool SpasForendHeld();bool SpasTwoHandAim56();void SpasTwoHandAimDirection56(const ArmJoints&,const ArmJoints&);bool RifleChargeHandMove(const Mat4&,const float*,Mat4&);bool RifleChargeHandHeld();
void PistolWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,Mat4&,float*);
void PistolFinalWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,float*);
void WritePistolWorld(float*,const Mat4&);
void SpasFinalWorld(ID3D11DeviceContext*,UINT,UINT,float*);
void SpasNativeFrame231(ID3D11DeviceContext*,UINT,UINT,const Mat4&);
bool SpasAttachmentMove231(ID3D11DeviceContext*,UINT,UINT,const float*,const Mat4*,Mat4&);
bool RifleAttachmentMove(ID3D11DeviceContext*,UINT,UINT,Mat4&);bool RifleSkinnedAttachmentMove(ID3D11DeviceContext*,UINT,UINT,const float*,const Mat4&,Mat4&);void RifleAttachmentProbe(ID3D11DeviceContext*,UINT,UINT,const Mat4&,const float*,const Mat4&);
bool Lever1887Model();bool Lever1887LeftHold();void UpdateLever1887Fire(const fc3pad::Controllers&);
bool PistolModel();
extern const Mat4 kShotgunHandFromGun,kRevolverRightHandFromFrame;
bool ControllerNeutralArm124(const Mat4&,float*,ArmJoints&);
void SeedControllerHand124();
bool ControllerGripRoot124(const Mat4&,const Mat4*,Mat4&);
bool ControllerMagazineRoot124(Mat4&);
bool ControllerRightHand124(Mat4&);
bool ControllerCurrentRoot124(Mat4&,Mat4&);
bool ControllerGripRoot124(const Mat4&,const Mat4*,Mat4&);
bool RepairPalette247(ID3D11DeviceContext*,float*,const Mat4&,UINT,UINT);
bool RepairSupportContact247(V3&,V3&);
void RepairSupportReset248();
bool RepairLeftMove249(const Mat4&,const float*,Mat4&);bool RepairLeftGrip249(float*);
bool ControllerGripWorld124(ID3D11DeviceContext*,const Mat4&,const Mat4*,Mat4&);
bool PinWeaponPalette124(float*,const Mat4&,const Mat4&,int=0);
bool R700Model();bool R700Hold();void UpdateR700Fire(const fc3pad::Controllers&);void TickR700(bool);bool R700NativeLeft();void R700LeftNativeMove(Mat4&);bool R700RightClipMove(const Mat4&,const float*,Mat4&);bool R700RightGrip(float*);bool R700LeftHold();bool R700LeftSupportActive();bool R700LeftSupportMove(const Mat4&,const float*,Mat4&);bool R700LeftGrip(float*);void RecordR700RightHand(ID3D11DeviceContext*,const float*);bool R700GunMove(ID3D11DeviceContext* context,Mat4&);bool R700SupportPosition(V3&);bool PkmArmsNative();bool PkmGunMove(Mat4&);void RecordR700FreeHand(const Mat4& freeWristView);
bool FlamerLeverMirror349(ID3D11DeviceContext*,float*,const Mat4&,UINT,UINT);
bool FlamerModel();bool FlamerActive();void UpdateFlamerFire(const fc3pad::Controllers&);void TickFlamer(bool);void FlamerGuideMarkers();bool FlamerLeftGrip(float*);bool FlamerLeftLeverMove(const Mat4&,const float*,Mat4&);bool FlamerBusyLeft();void RecordFlamerLeftHand(ID3D11DeviceContext*,const float*);bool FlamerNote();void FlamerPalette(ID3D11DeviceContext*,float*,const Mat4&,UINT,UINT);
bool Gm94Model();bool Gm94Active();void UpdateGm94Fire(const fc3pad::Controllers&);void TickGm94(bool);void Gm94GuideMarkers();bool Gm94LeftGrip(float*);bool Gm94LeftSupportMove(const Mat4&,const float*,Mat4&);bool Gm94SupportPosition(V3&);bool Gm94BusyLeft();void RecordGm94LeftHand(ID3D11DeviceContext*,const float*);bool Gm94Note();
bool BowModel();bool BowActive();bool BowAimBody(V3&);bool BowGunMove(Mat4&);bool BowPalette(ID3D11DeviceContext*,float*,const Mat4&,const Mat4&,UINT,UINT);bool BowWorld(ID3D11DeviceContext*,UINT,UINT,const Mat4&,Mat4&);bool BowNote();bool RepairToolNote();
bool RpgModel();bool RpgActive();void UpdateRpgFire(const fc3pad::Controllers&);void TickRpg(bool);void RpgGuideMarkers();bool RpgLeftGrip(float*);bool RpgRightGrip(float*);bool RpgClipMove(int,const Mat4&,const float*,Mat4&);bool RpgClipPlaying();bool RpgCarrying();void RecordRpgLeftHand(ID3D11DeviceContext*,const float*);bool RpgNote();
bool FlareModel();bool FlareActive();void UpdateFlareFire(const fc3pad::Controllers&);void TickFlare(bool);void FlareGuideMarkers();bool FlareLeftGrip(float*);void RecordFlareLeftHand(ID3D11DeviceContext*,const float*);
bool RevolverModel();bool RevolverActive();void UpdateRevolverFire(const fc3pad::Controllers&);void TickRevolver(bool);void RevolverGuideMarkers();bool RevolverNativeLeft();void RevolverLeftNativeMove(Mat4&);bool RevolverClipPlaying();bool RevolverLeftClipMove(const Mat4&,const float*,Mat4&);bool RevolverLeftGrip(float*);void RecordRevolverLeftHand(ID3D11DeviceContext*,const float*);void RevolverPalette(ID3D11DeviceContext*,float*,const Mat4&,UINT,UINT);
bool Lever1887RightGrip(float*);void Record1887RightHand(ID3D11DeviceContext*,const float*);bool Lever1887LeftGrip(float*);void Record1887LeftHand(ID3D11DeviceContext*,const float*);bool Lever1887RightStable();bool Lever1887RightHoldingMove(const Mat4&,const float*,Mat4&);bool Lever1887LeftSupportMove(const Mat4&,const float*,Mat4&);bool Lever1887SupportPosition(V3&);bool Lever1887AimBody(V3&);void TickLever1887(bool);
bool PlaceAmmoPreviewWorld(ID3D11DeviceContext*,UINT,UINT,float*);
void ObservePistolGuideRoot(ID3D11DeviceContext*,UINT,UINT,const float*,int);
bool Resolve1911Insertion(const Mat4&,Mat4&,bool);
void ObservePistolGuidePalm(const Mat4&);
void CapturePistolSlideGrip(ID3D11DeviceContext*,const float*);
bool SkorpionRigidDraw(ID3D11DeviceContext*,UINT,UINT);
bool AkRigidDraw(ID3D11DeviceContext*,UINT,UINT);
bool Mp5Palette(ID3D11DeviceContext*,float*,const Mat4&,const Mat4&,const Mat4&,UINT,UINT);
void LoadPistolReloadSettings();
void ObserveShotgunRoot(const float*,const Mat4&,UINT,UINT);
bool ManualReloadNote();
bool ManualReloadOwnsLeftHand();
bool ShotgunSupportAccepted342(const fc3pad::Controllers&);
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

V3 ShotgunCameraWrist318(const V3& fallback);
V3 OtherCameraWrist319(const V3& fallback);
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
    // Two-handed: take the game's after-shot wrist/gun rotation out FIRST (pivot at
    // the wrist), so the support turn below aligns the corrected gun with the left
    // hand. Previously the correction was one-handed only and the native shot
    // animation played on the right arm and shotgun whenever both hands held it.
    if (g_twoHand) CorrectShotHandMove(right, move);
    if (g_twoHand && left.valid && g_handLeftValid)
    {
        const V3 newWrist = Apply(move, wrist);
        V3 supportNative = { left.w[0], left.w[1], left.w[2] };
        ShotgunSupportNative(right, supportNative); // idle grip point, not the animated native left wrist
        V3 support = Apply(move, supportNative);M133AimSupport56(right,move,support);
        const V3 real = OtherCameraWrist319(ShotgunCameraWrist318(ControllerWrist(g_handLeft, 1)));
        if (Length(Sub(real, newWrist)) > 0.05 && Length(Sub(support, newWrist)) > 0.05)
            move = Multiply(TurnAndMove(Sub(support, newWrist), Sub(real, newWrist), newWrist, newWrist), move);
    }
    Mat4 forearmMove = {}, upperMove = {};
    if(!SolveArm(right, move, 0, forearmMove, upperMove, true))return false; // hand stays on the controller
    if(!g_twoHand)CorrectShotHandMove(right,move);
    CorrectPistolHandMove(right,move);
    return true;
}

// The left hand's move: its own controller, or the gun's while it holds it.
bool LeftMove(const ArmJoints& left, const ArmJoints& right, Mat4& move)
{
    if (!MotionInteractionEnabled() || !g_eyeViewsSet || !g_handLeftValid || !left.valid) return false;
    if (g_twoHand && g_frameGunValid)
    {
        if(KnownNonPumpModel()){move=g_frameGunMove;return true;}
        // along the barrel with the real hand (back up to 12 cm, forward 2 cm)
        double slide = g_slide - g_slideGrab;
        if (slide < -g_pumpRange) slide = -g_pumpRange;
        if (slide > 0.02) slide = 0.02;
        // the game's own pump animation drags the hand along: take that out
        // (turned like the gun), the real hand drives it instead
        const Mat4& gm = g_frameGunMove;
        const double* a = g_pumpAnimView;
        double anim[3] = { gm.m[0] * a[0] + gm.m[1] * a[1] + gm.m[2] * a[2],
                           gm.m[4] * a[0] + gm.m[5] * a[1] + gm.m[6] * a[2],
                           gm.m[8] * a[0] + gm.m[9] * a[1] + gm.m[10] * a[2] };
        // Whole native left-wrist animation (pump AND shot), not only the travel
        // along the barrel: the hand stays at its idle grip point on the gun.
        V3 grip = {};
        if (ShotgunSupportNative(right, grip))
        {
            const V3 now = Apply(gm, V3{ left.w[0], left.w[1], left.w[2] }), rest = Apply(gm, grip);
            anim[0] = now.x - rest.x; anim[1] = now.y - rest.y; anim[2] = now.z - rest.z;
        }
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
    if(!BowModel())fc3pad::SetBowInput(false,0);
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
        const V3 real = OtherCameraWrist319(ShotgunCameraWrist318(ControllerWrist(g_handLeft, 1)));
        g_slide = Dot(Sub(real, wrist), axis);
    }
    static bool wasHolding = false;
    if (g_twoHand && !wasHolding) g_slideGrab = g_slide;
    wasHolding = g_twoHand;

    // Keep the M133's spent chamber across weapon switches, but release its
    // trigger filter while a different known weapon is active.
    if(KnownNonPumpModel())
    {
        if(BowModel()){g_lastTrigger=c.rightTrigger;fc3pad::RequireFullFirePull(false);fc3pad::BlockFire(false);return;}
        if(Lever1887Model()){g_lastTrigger=c.rightTrigger;UpdateLever1887Fire(c);return;} // 1887 lever gate
        if(FlamerModel()){g_lastTrigger=c.rightTrigger;UpdateFlamerFire(c);return;} // flamethrower: dry while the lever is down / canister out
        if(Gm94Model()){g_lastTrigger=c.rightTrigger;UpdateGm94Fire(c);return;} // GM-94: pump + tilt after every shot, cover closed
        if(RpgModel()){g_lastTrigger=c.rightTrigger;UpdateRpgFire(c);return;} // RPG-7: dry until the rocket is seated
        if(FlareModel()){g_lastTrigger=c.rightTrigger;UpdateFlareFire(c);return;} // flare gun: dry until a new shell is in and the barrel is closed
        if(RevolverModel()){g_lastTrigger=c.rightTrigger;UpdateRevolverFire(c);return;} // revolver: no pump; locked while the cylinder is open
        if(SpasModel()){g_lastTrigger=c.rightTrigger;UpdateSpasFire(c);return;} // SPAS: semi-auto; locked only until the forend is cycled after an empty reload
        if(R700Model()){g_lastTrigger=c.rightTrigger;UpdateR700Fire(c);return;} // R700: a shot locks the trigger until RIGHT grip cycles the bolt
        g_lastTrigger=c.rightTrigger;
        fc3pad::RequireFullFirePull(false);fc3pad::BlockFire(false);return;
    }
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

// Two-hand grab gizmo (all weapons: a red palm sphere of diameter 10 cm, and the grabbed part of the weapon in another
// colour).
// Calibrated palm contact and weapon-local support contact share one query with
// the visible markers and grip ownership. No free-hand rotation moves the gun target.
constexpr double kGrabReach = 0.15, kGrabPalmDiameter = 0.10, kGm94AutoRelease = 0.25;
#include "support_grab214.inl"
extern bool g_grabGuides;
extern UINT g_armPoseEpoch;
#ifdef FC3_AMMO_TEST
bool g_weaponTriggerDiagnostics=false;
#else
bool g_weaponTriggerDiagnostics=false;
#endif
int g_weaponTriggerTarget=0;
struct FreeHandAttachment {Mat4 relative={};UINT epoch=~0u;LONG frame=-100000;LONG profile=-1;}g_freeHandAttachment[2];
V3 g_supportPalmDelta={};LONG g_supportPalmFrame=-100000;
LONG g_supportPalmProfile=-1;UINT g_supportPalmEpoch=~0u;
bool FreeHandPalm(int side,V3& palm){
 const auto held309=fc3hands303::Read();
 if(side==1&&(fc3hands303::SupportedSidearm315(g_weaponMechanicProfile)||fc3hands303::MagazineRifle317(g_weaponMechanicProfile)||fc3hands303::Shotgun318(g_weaponMechanicProfile)||fc3hands303::Other319(g_weaponMechanicProfile))&&held309.enabled&&held309.active&&held309.engaged&&!held309.empty&&held309.hand==1){
  const auto& physical=g_freeHandAttachment[0];
  if(!g_physicalValid303[0]||physical.epoch!=g_armPoseEpoch||physical.profile!=g_weaponMechanicProfile||g_frame-physical.frame>2)return false;
  Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[0][i];
  palm=Apply(Multiply(controller,physical.relative),{.6781,.0519,1.0387});return true;
 }

 const auto& a=g_freeHandAttachment[side];if(a.epoch!=g_armPoseEpoch||a.profile!=g_weaponMechanicProfile||g_frame-a.frame>2||!(side?g_handLeftValid:g_handRightValid))return false;
 Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[side][i];
 palm=Apply(Multiply(controller,a.relative),{side?-.6781:.6781,.0519,1.0387});return true;
}
bool GrabLeftHandBody(Mat4& hand); // the drawn left hand (pistol_reload.inl)
Mat4 ArmBodyToCamera();
bool M133AimSupport56(const ArmJoints&,const Mat4&,V3&);bool M133SupportPosition56(V3&);bool M133SupportBox56(Mat4&);double M133SupportDistance56();bool M133SupportNear56();Mat4 Lever1887SupportBox56();double Lever1887SupportDistance56();bool Lever1887PalmBody56(int,V3&);double Lever1887PalmRadius56(int);
bool R700FixedGrabPoints53(V3&,V3&);bool FlamerSupportPoints58(const V3&,const Mat4&,V3&,V3&);bool FlamerSupportBox58(Mat4&);float FlamerSupportDiameter58();double FlamerSupportDistance58();
bool RifleSupportContact234(V3&,V3&);
bool StableSupportContact236(const V3&,const Mat4&,V3&,V3&);
bool LeftSupportContact311(V3&,V3&);
bool WeaponSupportTarget349(V3&);
bool GrabGizmoPoints(const V3& supportCamera, V3& point, V3& palm,const Mat4* parent=nullptr)
{
    Mat4 inverse={};if(!g_handLeftValid||!Invert(ArmBodyToCamera(),inverse))return false;
    if(!(R700Model()&&R700Hold())&&WeaponSupportTarget349(point)){if(!FreeHandPalm(1,palm))palm=ControllerWrist(g_handBody[1],1);return true;}
    if(RepairSupportContact247(point,palm))return true;
    if(g_weaponMechanicProfile==2||g_weaponMechanicProfile==23){if(!R700FixedGrabPoints53(point,palm))return false;SupportGrabAdjustPoint214(point);return true;}
    if((g_weaponMechanicProfile>=3&&g_weaponMechanicProfile<=6)||g_weaponMechanicProfile==0){Mat4 box={};const bool ready=g_weaponMechanicProfile==0?(box=Lever1887SupportBox56(),true):M133SupportBox56(box);if(!ready)return false;point={box.m[3],box.m[7],box.m[11]};if(g_weaponMechanicProfile==0){if(!Lever1887PalmBody56(1,palm))return false;}else if(!FreeHandPalm(1,palm))palm=ControllerWrist(g_handBody[1],1);return true;}
    if(RifleSupportContact234(point,palm))return true;
    if(FlamerModel())return FlamerSupportPoints58(supportCamera,parent?*parent:g_frameGunMove,point,palm);
    if(StableSupportContact236(supportCamera,parent?*parent:g_frameGunMove,point,palm))return true;
    if(SupportGrabEditable214(g_weaponMechanicProfile))return false; // never learn an Insert-only animated fallback
    // Each contact point follows its own parent. Until a matching current hand
    // attachment is available, retain the existing wrist contact fallback.
    point=Apply(inverse,supportCamera);palm=ControllerWrist(g_handBody[1],1);
    V3 actual={};if(g_supportPalmProfile==g_weaponMechanicProfile&&g_supportPalmEpoch==g_armPoseEpoch&&g_frame-g_supportPalmFrame<=2&&FreeHandPalm(1,actual)){
        const Mat4& gun=parent?*parent:g_frameGunMove;
        const V3 d={gun.m[0]*g_supportPalmDelta.x+gun.m[1]*g_supportPalmDelta.y+gun.m[2]*g_supportPalmDelta.z,
                    gun.m[4]*g_supportPalmDelta.x+gun.m[5]*g_supportPalmDelta.y+gun.m[6]*g_supportPalmDelta.z,
                    gun.m[8]*g_supportPalmDelta.x+gun.m[9]*g_supportPalmDelta.y+gun.m[10]*g_supportPalmDelta.z};
        point=Apply(inverse,Add(supportCamera,d));palm=actual;
    }
    SupportGrabAdjustPoint214(point);return true;
}
void GrabGizmo(const V3& supportCamera, double distance)
{
    if(!g_grabGuides)return;
    V3 point={},palm={};if(!GrabGizmoPoints(supportCamera,point,palm))return;
    const float a[3]={static_cast<float>(point.x),static_cast<float>(point.y),static_cast<float>(point.z)},b[3]={static_cast<float>(palm.x),static_cast<float>(palm.y),static_cast<float>(palm.z)};
    const double scale=(std::max)(.1,static_cast<double>(g_worldScale));
    const double reach=SupportGrabReach214(g_weaponMechanicProfile);
    fc3xr::SetGrabMarkers(a,b,distance<reach,SupportGrabBoxDiameter214(g_weaponMechanicProfile),static_cast<float>(kGrabPalmDiameter));
}

// Once per frame (camera frame, after the aim turn): grab / release, and the
// gun direction the aim uses while held with both hands.
bool AutoSupportForWeapon()
{
 if(g_frame-g_weaponMechanicFrame>2)return false;
 // Verified conventional pistol support paths. Specialized loading weapons keep their own hand ownership below.
 if(g_weaponMechanicProfile==1||g_weaponMechanicProfile==12||g_weaponMechanicProfile==20)return g_autoPistolSupport;
 return g_weaponMechanicProfile>=0&&g_autoLongGunSupport;
}
void UpdateTwoHand()
{
    RepairSupportReset248();
    struct WheelSupport54 { ~WheelSupport54(){fc3pad::SetTwoHandWeaponHold(g_twoHand||SpasForendHeld());} } wheelSupport54;
    const auto holding303=fc3hands303::Read();
    if(holding303.enabled&&holding303.engaged&&holding303.empty){g_twoHand=false;fc3pad::SuppressLeftGrip(false);return;}

    if(MacheteHeld())
    {
        g_twoHand=false;g_frameGunValid=false;g_lastTrigger=fc3pad::Latest().rightTrigger;
        fc3pad::SuppressLeftGrip(false);return; // preserve firearm chamber/pump state
    }
    // the gun's move for this frame, before any grab change (from the arms
    // measured last frame); recomputed below once the hold state is known
    g_frameGunValid = GunMove(g_armJoints[0], g_armJoints[1], g_frameGunMove);
    struct Finish {
        Mat4 before;bool beforeValid,wasHeld;
        ~Finish() {
            g_frameGunValid = GunMove(g_armJoints[0], g_armJoints[1], g_frameGunMove);
            if(wasHeld!=g_twoHand){
                if(beforeValid&&g_frameGunValid){
                    const V3 oldForward={before.m[2],before.m[6],before.m[10]};
                    const V3 newForward={g_frameGunMove.m[2],g_frameGunMove.m[6],g_frameGunMove.m[10]};
                    const double angle=std::acos((std::max)(-1.0,(std::min)(1.0,Dot(Unit(oldForward),Unit(newForward)))))*57.2957795;
                    const double shift=Length(Sub({before.m[3],before.m[7],before.m[11]},
                        {g_frameGunMove.m[3],g_frameGunMove.m[7],g_frameGunMove.m[11]}));
                    Log("[AIM HANDOFF] frame%ld profile%d %s forwardDeltaDeg=%.3f shift=%.4fm; same-frame gun poses, diagnostic only",
                        g_frame,g_weaponMechanicProfile,g_twoHand?"grip on":"grip off",angle,shift);
                }else Log("[AIM HANDOFF] frame%ld profile%d %s pose unavailable; diagnostic only",
                    g_frame,g_weaponMechanicProfile,g_twoHand?"grip on":"grip off");
            }
        }
    } finish{g_frameGunMove,g_frameGunValid,g_twoHand};
    float leftGrip = 0, rightGrip = 0;
    fc3pad::Grips(leftGrip, rightGrip);
    const bool nativeManual356=ManualReloadOwnsLeftHand(),support356=M133ControllerSupport356();
    const bool manualLeft=(!support356&&nativeManual356)||PistolOwnsLeft();
    if(nativeManual356&&support356){static LONG sequence=-1;static unsigned lines=0;if(sequence!=g_reloadPose.sequence&&lines++<48){sequence=g_reloadPose.sequence;
      Log("[M133356] physical support retained through native pose sequence=%ld profile=%ld owner=%d; engine pose lifecycle preserved",sequence,g_weaponMechanicProfile,holding303.hand);}}
    if(g_reload.held||manualLeft)leftGrip=0;
    const ArmJoints& right = g_armJoints[0];
    const ArmJoints& left = g_armJoints[1];
    if(BowActive()){g_twoHand=false;fc3pad::SuppressLeftGrip(true);UpdatePump(right,left);return;}
    if(Lever1887LeftHold()){g_twoHand=false;fc3pad::SuppressLeftGrip(true);UpdatePump(right,left);return;} // 1887 loading: gun in the left hand
    if(R700LeftHold()){g_twoHand=false;fc3pad::SuppressLeftGrip(true);UpdatePump(right,left);return;} // R700 physical: gun in the left hand, the right works the bolt
    if(FlamerBusyLeft()){g_twoHand=false;fc3pad::SuppressLeftGrip(true);UpdatePump(right,left);return;} // flamethrower: lever / canister in the left hand
    if(Gm94BusyLeft()){g_twoHand=false;fc3pad::SuppressLeftGrip(true);UpdatePump(right,left);return;} // GM-94: cover / shell in the left hand
    if(RpgActive()&&(RpgClipPlaying()||RpgCarrying())){g_twoHand=false;fc3pad::SuppressLeftGrip(true);UpdatePump(right,left);return;} // RPG-7: rocket in the left hand / Jason inserting
    if(FlareActive()){g_twoHand=false;fc3pad::SuppressLeftGrip(true);UpdatePump(right,left);return;} // flare gun: LEFT grip = barrel / shells
    if(RevolverActive()){g_twoHand=false;fc3pad::SuppressLeftGrip(true);UpdatePump(right,left);return;} // revolver: LEFT grip = cylinder / speedloader / Simple reload
    if(SpasManual()){g_twoHand=SpasTwoHandAim56();fc3pad::SuppressLeftGrip(true);UpdatePump(right,left);SpasTwoHandAimDirection56(right,left);return;} // SPAS Manual: LEFT grip = shell / forend
    const bool possible = !fc3pad::GameplayInputBlocked() && fc3pad::Latest().active && !fc3pad::WeaponSelectionRequested() && MotionInteractionEnabled() && g_eyeViewsSet && g_handRightValid && g_handLeftValid && right.valid && left.valid && !(g_weaponMechanicProfile==39&&g_frame-g_weaponMechanicFrame<=2);
    if (!possible)
    {
        if (g_twoHand) Log("[ARMS] two-handed hold released (hands not tracked)");
        g_twoHand = false;
        fc3pad::SuppressLeftGrip(manualLeft);
        UpdatePump(right,left); // keep fire locked even when the other hand loses tracking
        return;
    }
    const bool autoHold=AutoSupportForWeapon()&&!g_reload.held&&!manualLeft;
    // Measure against the unsupported gun, otherwise the support turn follows the
    // free hand and makes distance-based release impossible.
    Mat4 grabMove={};const bool held=g_twoHand;g_twoHand=false;
    const bool grabValid=GunMove(right,left,grabMove);
    double grabDistance=1e9;
    if(grabValid){
        V3 supportNative={left.w[0],left.w[1],left.w[2]};ShotgunSupportNative(right,supportNative);V3 support=Apply(grabMove,supportNative);
        if(!M133SupportPosition56(support)&&!Lever1887SupportPosition(support)&&!R700SupportPosition(support)&&!Gm94SupportPosition(support))AkSupportPosition(support);
        V3 target={},hand={};if(GrabGizmoPoints(support,target,hand,&grabMove)){grabDistance=Length(Sub(hand,target));Mat4 box={};if(SupportGrabEditable214(g_weaponMechanicProfile)&&!FlamerModel()&&SupportGrabBox214(grabMove,target,box))grabDistance=SupportGrabOutside214(g_weaponMechanicProfile,box,hand)+SupportGrabReach214(g_weaponMechanicProfile)-kGrabPalmDiameter*.5*(std::max)(.1f,g_worldScale);}if(g_weaponMechanicProfile>=3&&g_weaponMechanicProfile<=6)grabDistance=M133SupportDistance56()+kGrabReach-kGrabPalmDiameter*.5*(std::max)(.1f,g_worldScale);else if(Lever1887Model())grabDistance=Lever1887SupportDistance56()+kGrabReach-Lever1887PalmRadius56(1);if(FlamerModel())grabDistance=FlamerSupportDistance58()+SupportGrabReach214(15)-kGrabPalmDiameter*.5*(std::max)(.1f,g_worldScale);if(Gm94Model())grabDistance=Gm94PumpDistance54()+kGrabReach-.035*(std::max)(.1f,g_worldScale);GrabGizmo(support,grabDistance);
    }
    g_twoHand=held;
    // The XR resolver already accepted the current M133 forend squeeze.
    // Do not require a second, narrower legacy support-box hit to hold it.
    if(g_weaponMechanicProfile>=3&&g_weaponMechanicProfile<=6&&ShotgunSupportAccepted342(fc3pad::Latest())&&!manualLeft&&!g_reload.held&&leftGrip>.6f)grabDistance=0;
    if(!g_twoHand){
        if((leftGrip>.6f||autoHold)&&grabDistance<SupportGrabReach214(g_weaponMechanicProfile)&&!manualLeft&&!g_reload.held){
            g_twoHand=true;fc3xr::Buzz(1);
            Log("[ARMS] left hand %s the gun (%.2f m)",autoHold?"auto-held":"grabbed",grabDistance);
        }
    }
    else if (manualLeft||g_reload.held||(autoHold?grabDistance>(std::max)(kGm94AutoRelease,SupportGrabReach214(g_weaponMechanicProfile)+.10):leftGrip<.4f))
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
    if(BowModel())return BowGunMove(handMove);
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
    const auto pumpShader249=ShaderHash(shader);const bool shaderMatches=pumpShader249==0xA38F7CD233BCE12Bull||pumpShader249==0xCB43EE096F50DBDBull;
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
void MuzzleDiscardStage58();int MuzzlePaletteSlot58(ID3D11DeviceContext*);bool MuzzleReadNative58(ID3D11DeviceContext*,UINT,UINT,const float*,Mat4&,int&,int&,UINT=3);void MuzzleStage58(ID3D11DeviceContext*,UINT,UINT,ID3D11Buffer*,const Mat4&,const Mat4&,int);
void BowSightFinalizeWorld55(ID3D11DeviceContext*,UINT,UINT,float*);
void BowSightWorldUploaded55(ID3D11DeviceContext*,UINT,UINT,UINT);
bool BowAttachmentMove54(ID3D11DeviceContext*,UINT,UINT,const float*,const Mat4*,Mat4&);
bool PistolAttachmentMove(ID3D11DeviceContext*,UINT,UINT,Mat4&,const float* palette=nullptr);
bool RifleAttachmentModel(UINT,UINT);
bool PistolPreciseTranslation();bool WritePistolSplitWorld(float*,const Mat4&);
ID3D11Buffer* BuildWeaponPalette(ID3D11DeviceContext* context, UINT start, UINT count,UINT* outputSlot)
{
    MuzzleDiscardStage58();
    if(g_weaponPaletteCb){const BOOL unmirrored338=FALSE;g_weaponPaletteCb->SetPrivateData(kA2000Mirror337,sizeof(unmirrored338),&unmirrored338);}
    MarkReloadShotgun(context,start,count);
    CaptureReloadShell(context,start,count);
    ReplayReloadShell(context);
    // VZ61 and its suppressor are rigid asset parts. A palette left bound by
    // an earlier arm draw must not send them down the skinned-weapon path.
    if (SideAttachment338(context,start,count)||SkorpionRigidDraw(context,start,count)||AkRigidDraw(context,start,count)||M133SightDraw57(context,start,count)) return nullptr;
    const int reflectedSlot=MuzzlePaletteSlot58(context);
    if(reflectedSlot==-1)return nullptr; // a stale bound arm palette does not make a rigid shader skinned
    const UINT paletteSlot=reflectedSlot>=0?UINT(reflectedSlot):3u;
    if(outputSlot)*outputSlot=paletteSlot;
    if (!g_weaponPaletteCb) return nullptr;
    Mat4 handMove = {};
    ID3D11Buffer* bonesBuffer = nullptr;
    context->VSGetConstantBuffers(paletteSlot, 1, &bonesBuffer);
    D3D11_BUFFER_DESC desc = {};
    if (bonesBuffer) bonesBuffer->GetDesc(&desc);
    SafeRelease(bonesBuffer);
    if (desc.ByteWidth != kBonesBytes) return nullptr;      // not skinned
    float bones[48 * 12] = {};
    Mat4 toView = {}, fromView = {};
    if (!DrawSpace(context, bones, toView, fromView,paletteSlot,true)) return nullptr;
    RecordAnimationPaletteSource217(context,bones,paletteSlot);
    // 241: native birth owns flame transport; no projected-vertex relocation.
    Mat4 muzzleNative58={};int muzzleProfile58=-1,muzzleAnchor58=0;const bool muzzleSource58=MuzzleReadNative58(context,start,count,bones,muzzleNative58,muzzleProfile58,muzzleAnchor58,paletteSlot);
    const auto stageMuzzle=[&](){
        if(!muzzleSource58)return;
        ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);
        float rows[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,rows,sizeof(rows));SafeRelease(cb);
        const int offset=WeaponWorldOffset(context);
        if(bytes>=UINT((offset+15)*4))
            MuzzleStage58(context,start,count,g_weaponPaletteCb,muzzleNative58,
                Multiply(HullWorld(rows+offset),BoneMatrix(bones,muzzleAnchor58)),muzzleProfile58);
    };
    if(RepairPalette247(context,bones,toView,start,count)){
        RecordAnimationFinal(bones,2);const bool own=g_own;g_own=true;
        g_oUpdateSubresource(context,g_weaponPaletteCb,0,nullptr,bones,0,0);
        g_weaponPaletteCb->SetPrivateData(kCbCopyGuid,kBonesBytes,bones);g_own=own;return g_weaponPaletteCb;
    }
    if(BowPalette(context,bones,toView,fromView,start,count)){
        stageMuzzle();RecordAnimationFinal(bones,2);
        const bool own=g_own;g_own=true;
        g_oUpdateSubresource(context,g_weaponPaletteCb,0,nullptr,bones,0,0);
        g_weaponPaletteCb->SetPrivateData(kCbCopyGuid,kBonesBytes,bones);
        g_own=own;return g_weaponPaletteCb;
    }
    ObserveShotgunRoot(bones,toView,start,count);
    // Rifle sights/scopes/suppressors follow the corrected gun FRAME, not the bare hand move.
    if (!SpasAttachmentMove231(context,start,count,bones,&toView,handMove)&&!BowAttachmentMove54(context,start,count,bones,&toView,handMove)&&!PistolAttachmentMove(context,start,count,handMove,bones)&&!RifleSkinnedAttachmentMove(context,start,count,bones,toView,handMove)&&!RifleAttachmentMove(context,start,count,handMove)&&!R700GunMove(context,handMove)&&!RightHandMove(handMove)) return nullptr;
    RecordPumpDraw(context, bones, toView, start, count); // raw, before hand movement
    DriveMeasuredForend(context, bones, toView, start, count);
    // The held-shell replay owns the physical shell. Hide the game's separate
    // loading-shell draws for all of manual mode, including after consumption
    // when mode/held have cleared but the native animation is still finishing.
    const bool hiddenShell124=(g_reload.mode||g_manualReloadEnabled)&&ReloadShellDraw(context,start,count);
    if(hiddenShell124)
    {
        MoveReloadShell(bones,fromView);for(int b=0;b<48;++b)bones[b*12+7]=-100;
        static LONG hidden=0;
        if(g_manualReloadEnabled&&InterlockedIncrement(&hidden)<=2)
            Log("[PHYSICAL RELOAD] original animated loading shell hidden (start %u count %u); held shell replay independent",start,count);
    }
    else if(!Mp5Palette(context,bones,toView,fromView,handMove,start,count)) // MP5: skinned rifle, magazine / handle are palette slots
        MoveBones(bones, 0, 47, Multiply(fromView, Multiply(handMove, toView)));
    if(!hiddenShell124&&!PistolModel()&&!RevolverModel()&&!FlamerModel()&&g_reload.shotgunFrame==g_frame)PinWeaponPalette124(bones,toView,kShotgunHandFromGun);
    // The .44 assembly uses BuildRevolverPalette217, including its b4 color
    // passes. An unrelated optic must not be pinned to the receiver origin.
    RevolverPalette(context,bones,toView,start,count); // cylinder / cartridges (after the hand move)
    FlamerPalette(context,bones,toView,start,count); // flamethrower lever / canister
    if(FlamerLeverMirror349(context,bones,toView,start,count)){const BOOL reflected349=2;g_weaponPaletteCb->SetPrivateData(kA2000Mirror337,sizeof(reflected349),&reflected349);}
    const bool own = g_own;
    g_own = true;
    RecordReloadPort(context,bones,start,count);
    M133SightObserveFinal57(context,toView,bones,start,count);
    stageMuzzle();
    RecordAnimationFinal(bones,2);
    g_oUpdateSubresource(context, g_weaponPaletteCb, 0, nullptr, bones, 0, 0);
    Trace1911Palette46(context,bones,48*12*sizeof(float),g_weaponPaletteCb);
    if(NativeGuideShadowScope51())g_weaponPaletteCb->SetPrivateData(kCbCopyGuid,48*12*sizeof(float),bones);
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
    MuzzleDiscardStage58();
    Mat4 handMove = {};
    const bool handMoveValid=SideSkinnedAttachmentMove338(context,start,count,handMove)||BowAttachmentMove54(context,start,count,nullptr,nullptr,handMove)||PistolAttachmentMove(context,start,count,handMove)||RifleAttachmentMove(context,start,count,handMove)||R700GunMove(context,handMove)||RightHandMove(handMove); // 1911 attachments from other model files follow the corrected FRAME // R700 in the left hand: scope passes follow the gun
    ID3D11Buffer* globalsBuffer = nullptr;
    context->VSGetConstantBuffers(0, 1, &globalsBuffer);
    float globals[kSmallCbMax / 4] = {};
    const UINT bytes = ReadCopy(globalsBuffer, globals, sizeof(globals));
    D3D11_BUFFER_DESC desc = {};
    if (globalsBuffer) globalsBuffer->GetDesc(&desc);
    SafeRelease(globalsBuffer);
    if (bytes < 48 || bytes != desc.ByteWidth) return nullptr;
    const int worldOffset = WeaponWorldOffset(context);
    if (worldOffset && bytes < static_cast<UINT>((worldOffset + 15) * 4)) return nullptr;
    float* const worldRows = globals + worldOffset;
    if (worldOffset) { static LONG logged = 0; if (InterlockedIncrement(&logged) == 1) Log("[WEAPON218] reflected world rows start at cb0 float %d (first draw %u/%u); preceding vectors preserved", worldOffset, start, count); }
    Mat4 muzzleNative58={};int muzzleProfile58=-1,muzzleAnchor58=0;const bool muzzleSource58=MuzzleReadNative58(context,start,count,nullptr,muzzleNative58,muzzleProfile58,muzzleAnchor58);
    const bool m133Sight=bytes>=static_cast<UINT>((worldOffset+15)*4)&&M133SightWorld57(context,start,count,worldRows);
    const bool ammoPreview=!m133Sight&&PlaceAmmoPreviewWorld(context,start,count,worldRows);
    if(!handMoveValid&&!ammoPreview&&!m133Sight)return nullptr;
    Mat4 toView = {}, fromView = {};
    if (!DrawSpace(context, nullptr, toView, fromView,3,true)) return nullptr;
    if(!ammoPreview&&!m133Sight){
    const Mat4 pistolNativeWorld=HullWorld(worldRows);
    SpasNativeFrame231(context,start,count,pistolNativeWorld);
    SpasAttachmentMove231(context,start,count,nullptr,nullptr,handMove);
    if(!BowWorld(context,start,count,toView,handMove))PistolWorld(context,start,count,toView,handMove,worldRows);
    // model -> world is the first three rows; the move in view space becomes
    // world' = W * (G^-1 D G) applied before: W' = W X with X in model space
    Mat4 world = {};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c) world.m[r * 4 + c] = worldRows[r * 4 + c];
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
    if(PistolPreciseTranslation()){
        Mat4 full=moved;for(int r=0;r<3;++r)full.m[r*4+3]+=worldRows[12+r];
        WritePistolSplitWorld(worldRows,full);
    }else for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c) worldRows[r * 4 + c] = static_cast<float>(moved.m[r * 4 + c]);
    SpasFinalWorld(context,start,count,worldRows);
    PistolFinalWorld(context,start,count,pistolNativeWorld,worldRows);
    RevolverAssemblyWorld217(context,start,count,worldRows);
    RifleAttachmentProbe(context,start,count,pistolNativeWorld,worldRows,handMove);
    BowSightFinalizeWorld55(context,start,count,worldRows);
    ObservePistolGuideRoot(context,start,count,worldRows,1);
    }
    const BOOL mirrored337=A2000MirrorWorld337(context,start,count,worldRows)?TRUE:FALSE;
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
    g_worldCbs[slot]->SetPrivateData(kA2000Mirror337,sizeof(mirrored337),&mirrored337);
    g_oUpdateSubresource(context, g_worldCbs[slot], 0, nullptr, globals, 0, 0);
    if(muzzleSource58){Mat4 final58=HullWorld(worldRows);const int slot58=MuzzlePaletteSlot58(context);if(slot58>=0){ID3D11Buffer* boneCb58=nullptr;context->VSGetConstantBuffers(UINT(slot58),1,&boneCb58);float nativeBones58[576]={};const UINT count58=ReadCopy(boneCb58,nativeBones58,sizeof(nativeBones58));SafeRelease(boneCb58);if(count58>=UINT((muzzleAnchor58+1)*48))final58=Multiply(final58,BoneMatrix(nativeBones58,muzzleAnchor58));else muzzleProfile58=-1;}if(muzzleProfile58>=0)MuzzleStage58(context,start,count,g_worldCbs[slot],muzzleNative58,final58,muzzleProfile58);}
    BowSightWorldUploaded55(context,start,count,bytes);
    if(m133Sight)M133SightUploaded57(context,start,count,bytes);
    RecordAnimationSolid(context,globals,bytes);
    if(NativeGuideShadowScope51())g_worldCbs[slot]->SetPrivateData(kCbCopyGuid,bytes,globals);
    Stage1911Frame(context,start,count,worldRows,g_worldCbs[slot]);
    Trace1911World46(context,globals,bytes,g_worldCbs[slot]);
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

#include "trigger_finger262.inl"
void CurlFingers(float* bones, int side)
{
    // the hand holding the gun keeps the game's grip
    if (!g_renderAnatomical303&&(side == 0 || g_twoHand)) return;
    const fc3pad::Controllers c = fc3pad::Latest();
    if (!c.active) return;
    double grip = c.leftGrip, trigger = c.leftTrigger;bool thumb=c.leftThumbTouch,index=c.leftIndexTouch;
    if(g_renderAnatomical303){float rg=0,lg=0,rt=0,lt=0;fc3hands303::RawControls(rg,lg,rt,lt);const auto held=fc3hands303::Read();grip=side?lg:rg;trigger=side?lt:rt;if(!held.empty&&held.hand==side)grip=1;const int role=fc3hands303::PhysicalHand(held,side);thumb=role?c.leftThumbTouch:c.rightThumbTouch;index=role?c.leftIndexTouch:c.rightIndexTouch;}

    const double curl[5] = {
        thumb ? 1.0 : 0.25,                                 // thumb
        index ? 0.35 + 0.65 * trigger : 0.0,               // index
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

// Jason's body mesh (shirt, sleeves) shares bones with the arm meshes: the torso
// section carries each arm's elbow and upper-arm bones. Those body bones are
// REPLACED by the arm's final (controller-driven) bones, so the sleeve sits on
// the arm the player sees.
//
// 10-08 rewrite. The old version matched body bones to the arms' original bones
// by equality in the same frame and applied that frame's moves. It stopped
// working for two reasons: the body is now drawn with the world camera (it was
// only called for weapon-camera draws), and the body sections are drawn BEFORE
// the arm sections, so "this frame's" arm moves did not exist yet. Now:
//  - which body bone is which arm bone is learned ONCE per mesh section (by
//    equality, at any quiet moment, against this or last frame's originals) and
//    kept; Jason's known mesh is seeded so it works from the first frame;
//  - the arms' final bones are kept in BODY axes (camera-relative position, body
//    rotation), so the body draw can use them a frame later whatever the camera
//    did in between (the arms are camera-attached; Q handles the turn).
Mat4 ArmBodyToCamera();
Mat4 g_armFinalBody[2][25] = {};     // each arm's final skin matrices, body axes
LONG g_armFinalFrame[2] = { -10, -10 };
LONG g_sleeveStale = 0, g_sleeveMissing = 0;
struct SleeveMap
{
    UINT vbBytes, start, count;
    signed char side[48], slot[48];
    int mapped;
    LONG lastTry;
};
SleeveMap g_sleeveMaps[16] = {};
int g_sleeveMapCount = 0;

SleeveMap* FindSleeveMap(UINT vbBytes, UINT start, UINT count)
{
    for (int i = 0; i < g_sleeveMapCount; ++i)
        if (g_sleeveMaps[i].vbBytes == vbBytes && g_sleeveMaps[i].start == start && g_sleeveMaps[i].count == count)
            return &g_sleeveMaps[i];
    if (g_sleeveMapCount >= 16) return nullptr;
    SleeveMap& m = g_sleeveMaps[g_sleeveMapCount++];
    m = {};
    m.vbBytes = vbBytes; m.start = start; m.count = count; m.lastTry = -1000;
    for (int b = 0; b < 48; ++b) { m.side[b] = -1; m.slot[b] = -1; }
    // Jason's known mesh (316240 bytes): torso section 0 carries both arms' elbow + upper-arm
    // bones (10-02 and 10-08 captures: body slots 7/8/9 = left arm 0/22/23, 11/12/13 = right 0/22/23).
    if (vbBytes == 316240 && start == 0)
    {
        const int seed[6][3] = { { 7, 1, 0 }, { 8, 1, 22 }, { 9, 1, 23 }, { 11, 0, 0 }, { 12, 0, 22 }, { 13, 0, 23 } };
        for (const auto& s : seed) { m.side[s[0]] = static_cast<signed char>(s[1]); m.slot[s[0]] = static_cast<signed char>(s[2]); }
        m.mapped = 6;
        Log("[SLEEVES] body section start %u (%u bytes): built-in map, 6 bones follow the arms", start, vbBytes);
    }
    return &m;
}

ID3D11Buffer* BuildBodyPalette(ID3D11DeviceContext* context)
{
    if (!g_bodyPaletteCb || (!MotionInteractionEnabled()&&!ThrowableArmMotion150()) || !g_eyeViewsSet) return nullptr;
    ID3D11Buffer* bonesBuffer = nullptr;
    context->VSGetConstantBuffers(3, 1, &bonesBuffer);
    D3D11_BUFFER_DESC desc = {};
    if (bonesBuffer) bonesBuffer->GetDesc(&desc);
    SafeRelease(bonesBuffer);
    if (desc.ByteWidth != kBonesBytes) return nullptr;
    ID3D11Buffer* stream = nullptr;
    UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
    D3D11_BUFFER_DESC vbDesc = {};
    if (stream) stream->GetDesc(&vbDesc);
    SafeRelease(stream);
    SleeveMap* map = FindSleeveMap(vbDesc.ByteWidth, g_bodyDrawStart, g_bodyDrawCount);
    if (!map) return nullptr;
    float bones[48 * 12] = {};
    Mat4 toView = {}, fromView = {};
    if (!DrawSpace(context, bones, toView, fromView)) return nullptr;
    if (map->mapped == 0)
    {
        // learn which body bones are arm bones: equal to an arm's original (game) bone
        // this frame or last frame (the game's arm is still between frames often enough)
        if (g_frame - map->lastTry < 30) { InterlockedIncrement(&g_sleeveMissing); return nullptr; }
        map->lastTry = g_frame;
        for (int b = 0; b < 48; ++b)
            for (int side = 0; side < 2 && map->side[b] < 0; ++side)
            {
                if (g_frame - g_armMovesFrame[side] > 1) continue;
                for (int a = 0; a < 25 && map->side[b] < 0; ++a)
                {
                    const float* o = g_armOriginal[side] + a * 12;
                    const float* m = bones + b * 12;
                    bool same = true;
                    for (int k = 0; k < 12 && same; ++k) same = std::fabs(o[k] - m[k]) < 1e-5f;
                    if (same) { map->side[b] = static_cast<signed char>(side); map->slot[b] = static_cast<signed char>(a); ++map->mapped; }
                }
            }
        if (map->mapped == 0) { InterlockedIncrement(&g_sleeveMissing); return nullptr; }
        char text[320] = {};
        for (int b = 0; b < 48; ++b)
        {
            if (map->side[b] < 0) continue;
            char piece[32];
            _snprintf_s(piece, sizeof(piece), _TRUNCATE, " %d=%c%d", b, map->side[b] == 0 ? 'R' : 'L', map->slot[b]);
            strncat_s(text, piece, _TRUNCATE);
        }
        Log("[SLEEVES] body section start %u count %u (%u bytes): %d bones learned from the arms:%s",
            map->start, map->count, map->vbBytes, map->mapped, text);
    }
    const Mat4 q = ArmBodyToCamera();
    bool moved = false, stale = false;
    for (int b = 0; b < 48; ++b)
    {
        const int side = map->side[b];
        if (side < 0) continue;
        if (g_frame - g_armFinalFrame[side] > 2) { stale = true; continue; }
        StoreBone(bones, b, Multiply(fromView, Multiply(q, g_armFinalBody[side][map->slot[b]])));
        moved = true;
    }
    if (stale) InterlockedIncrement(&g_sleeveStale);
    if (!moved) return nullptr;
    const bool own = g_own;
    g_own = true;
    g_oUpdateSubresource(context, g_bodyPaletteCb, 0, nullptr, bones, 0, 0);
    g_own = own;
    if (InterlockedIncrement(&g_sleeveDraws) == 1)
        Log("[ARMS] body mesh bones (sleeves) moved with the arms for the first time");
    return g_bodyPaletteCb;
}

// Passive coverage for normal play: a first sighting of each known weapon
// records its CPU-cached draw poses. Body candidates are observed even when
// they use the main camera, which the older weapon-only timeline omitted.
// Geometry identifies the model family, not the equipped variant/ammo capacity.
bool g_vrBaselineModels[sizeof(fc3weapon::kProfiles)/sizeof(*fc3weapon::kProfiles)] = {};
int g_vrBaselineLastProfile=-1;
// Throwables capture (grenade / molotov / C4 / mine to be done physically, with capture tests). They are first-person
// models (kModels) but not weapon profiles, so nothing tagged them: while one is drawn in the hand the recorder samples every frame
// and the log names it (every 10 s). Observation only.
void ObserveThrowableCapture(ID3D11DeviceContext* context,const DrawCall& call)
{
    if(call.kind!=0||call.d!=0)return;
    const auto g=NativeWeaponGeometry(context,call);
    static const struct{UINT vb,ib;const char* name;}kThrowables[]={{37888,9276,"frag_grenade"},{43360,11358,"molotov"},{143132,32760,"ied_c4_bomb"},
        {49632,12726,"ied_c4_remote"},{119864,21396,"proxmine"},{5920,1644,"rock"}};
    for(unsigned i=0;i<sizeof(kThrowables)/sizeof(*kThrowables);++i)
    {
        if(g.vertexBytes!=kThrowables[i].vb||g.indexBytes!=kThrowables[i].ib)continue;
        // The recorder samples every frame only in the first 2 s of a 10 s window (animation_probe.inl), so the window is renewed
        // every 2 s while it is in the hand: the whole time it is held is recorded frame by frame.
        static ULONGLONG last[8]={},logged[8]={};const ULONGLONG now=GetTickCount64();
        if(now-last[i]>2000){last[i]=now;g_animation.sample=true;g_animation.until=now+10000;}
        if(now-logged[i]>10000)
        {
            logged[i]=now;
            AnimationRecord r;AnimationStamp(r);r.kind=18;r.state=i;r.vertexBytes=g.vertexBytes;r.stride=g.stride;r.offset=g.offset;
            r.count=call.a;r.start=call.b;r.stateWords[0]=g.indexBytes;r.indexFormat=g.format;AnimationStore(r);
            Log("[THROWABLE CAPTURE] %s in hand (draw %u/%u stride %u offset %u): every frame recorded while held",kThrowables[i].name,call.b,call.a,g.stride,g.offset);
        }
        return;
    }
}
#include "weapons8_capture.inl"
void RecordVrBaselineDraw(ID3D11DeviceContext* context,const DrawCall& call,int variant,bool nativeWeapon)
{
    if(!g_inGameplay)return;
    ObserveRepairToolDraw(context,call);
    ObserveWeapons8Capture(context,call,variant);
    if(variant==1&&g_animation.enabled)ObserveThrowableCapture(context,call);
    if(!(variant==1&&nativeWeapon)&&(!g_animation.enabled||!g_animation.sample))return;
    const auto geometry=variant==1&&nativeWeapon?NativeWeaponGeometry(context,call):fc3weapon::Geometry{};
    const int profile=variant==1&&nativeWeapon?fc3weapon::ProfileIndex(geometry):-1;
    if(profile>=0){g_weaponMechanicProfile=profile;g_weaponMechanicFrame=g_frame;}
    if(!g_animation.enabled)return;
    const bool switched=profile>=0&&(g_vrBaselineLastProfile<0||
        std::strcmp(fc3weapon::kProfileNames[profile],fc3weapon::kProfileNames[g_vrBaselineLastProfile])!=0);
    if(profile>=0&&(!g_vrBaselineModels[profile]||switched))
    {
        g_vrBaselineLastProfile=profile;
        const char* name=fc3weapon::kProfileNames[profile];
        for(unsigned i=0;i<sizeof(g_vrBaselineModels)/sizeof(*g_vrBaselineModels);++i)
            if(std::strcmp(name,fc3weapon::kProfileNames[i])==0)g_vrBaselineModels[i]=true;
        g_animation.sample=true;g_animation.until=GetTickCount64()+8000;
        AnimationRecord r;AnimationStamp(r);r.kind=17;r.state=static_cast<UINT>(profile);
        r.vertexBytes=geometry.vertexBytes;r.stride=geometry.stride;r.offset=geometry.offset;
        r.count=call.a;r.start=call.b;r.stateWords[0]=geometry.indexBytes;r.indexFormat=geometry.format;
        AnimationStore(r);
        Log("[VR BASELINE] model geometry %s; automatic pose window, variant/capacity still unverified",name);
    }
    // This bounded candidate set comes from Jason's captured shared VB and
    // known body sections. Do not mark it as weapon geometry or change variant.
    if(!g_animation.sample||!(call.b==0||call.b==3687||call.b==27747))return;
    ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;D3D11_BUFFER_DESC desc={};
    context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);if(vb)vb->GetDesc(&desc);SafeRelease(vb);
    if(desc.ByteWidth!=316240||stride!=40)return;
    RecordAnimationDraw(context,call);
    if(!g_animation.pending)return;
    auto& r=g_animation.draw;r.kind=16;r.event=static_cast<UINT>(variant);
    r.stateWords[0]=static_cast<UINT>(g_armMovesFrame[0]);r.stateWords[1]=static_cast<UINT>(g_armMovesFrame[1]);
    if(r.rawBytes==kBonesBytes)
    {
        for(int side=0;side<2;++side)
        {
            if(g_armMovesFrame[side]!=g_frame)continue;
            for(int b=0;b<48;++b)
                for(int a=0;a<25;++a)
                {
                    bool equal=true;
                    for(int k=0;k<12&&equal;++k)equal=std::fabs(r.raw[b*12+k]-g_armOriginal[side][a*12+k])<1e-5f;
                    if(equal){++r.stateWords[2+side];break;}
                }
        }
    }
    FinishAnimationDraw();
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
    if(g_carArmScope352)return g_carBodyView352;
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
// The neutral arm pose is learned from the game's own arm, and it carries the grip of whatever gun is held at that moment
// (log, 5 sessions: learned while holding the SPAS -> "the left wrist was better"; learned while holding the P416 ->
// "THE LEFT HAND WAS TWISTED"). The P416 foregrip rolls the wrist. So the pose is now learned from the SPAS only and SAVED
// (fc3_neutral_arms.bin): every later session reuses exactly that pose, whatever gun is out. Until a SPAS pose exists the
// first quiet pose is used for that session only, and the first quiet SPAS hold replaces and saves it.
// Delete fc3_neutral_arms.bin to learn it again.
#include "neutral_arms_data.inl"
// FIXED neutral (the arm was wrong for every weapon): the learned pose is used for every gun of the session,
// so one bad learn (today: the P416 foregrip) spoils them all. The game now always uses ONE pose built into the DLL: the
// SPAS-12 idle arms from the capture (neutral_arms_data.inl), whatever gun is out, every session. The file below is no
// longer written or read in the game.
struct SavedNeutralArms { UINT magic, version; int valid[2]; LONG profile[2]; StableArmPose pose[2]; };
SavedNeutralArms g_savedNeutral = {}; bool g_savedNeutralLoaded = false; LONG g_neutralProfile[2] = { -1, -1 };
const LONG kNeutralReferenceProfile = 34; // SPAS-12: its idle forend hold gave the straight wrist
void NeutralArmsPath(wchar_t* path) { _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%sfc3_neutral_arms.bin", fc3::DataFolder()); }
void LoadSavedNeutral()
{
    if (g_savedNeutralLoaded) return; g_savedNeutralLoaded = true;
    wchar_t path[MAX_PATH]; NeutralArmsPath(path);
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) { Log("[ARMS] no saved neutral arm pose yet: it is learned from the SPAS-12 (hold it still once)"); return; }
    SavedNeutralArms s = {}; DWORD got = 0; const BOOL ok = ReadFile(f, &s, sizeof(s), &got, nullptr); CloseHandle(f);
    if (!ok || got != sizeof(s) || s.magic != 0x4D524146u || s.version != 1) { Log("[ARMS] saved neutral arm pose unreadable; learning it again"); return; }
    g_savedNeutral = s;
    Log("[ARMS] saved neutral arm pose loaded (left %s, right %s): the same straight hands every session", s.valid[1] ? "yes" : "no", s.valid[0] ? "yes" : "no");
}
void SaveNeutral(int side, const StableArmPose& pose)
{
    g_savedNeutral.magic = 0x4D524146u; g_savedNeutral.version = 1; g_savedNeutral.valid[side] = 1;
    g_savedNeutral.profile[side] = g_weaponMechanicProfile; g_savedNeutral.pose[side] = pose;
    wchar_t path[MAX_PATH]; NeutralArmsPath(path);
    HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD put = 0; const BOOL ok = f != INVALID_HANDLE_VALUE && WriteFile(f, &g_savedNeutral, sizeof(g_savedNeutral), &put, nullptr) && put == sizeof(g_savedNeutral);
    if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
    Log("[ARMS] %s neutral pose %s (learned holding the SPAS-12): reused every session from now on", side == 0 ? "right" : "left", ok ? "SAVED" : "SAVE FAILED");
}
bool RestoreStableArmPose(ID3D11DeviceContext* context, int side, const Mat4& toView,
                          const Mat4& fromView, float* bones, ArmJoints& joints)
{
    if(side==0&&g_neutralQuietGate)return ControllerNeutralArm124(fromView,bones,joints);
    ID3D11Buffer* stream = nullptr; UINT stride = 0, offset = 0;
    context->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
    if (!stream) return false;
    const Mat4 q = ArmBodyToCamera(); Mat4 qi = {};
    if (!Invert(q, qi)) { stream->Release(); return false; }
    StableArmPose pose = {}; UINT bytes = sizeof(pose);
    const bool persist = false; // superseded by the fixed pose (kept: LoadSavedNeutral / SaveNeutral unused)
    if (persist) LoadSavedNeutral();
    if (g_neutralQuietGate && !(SUCCEEDED(stream->GetPrivateData(kStableArmGuid[side], &bytes, &pose)) && bytes == sizeof(pose) && pose.epoch == g_armPoseEpoch))
    { // the fixed straight pose: view space -> body space with this frame's camera/body rotation (as a live capture would)
        pose = {}; pose.epoch = g_armPoseEpoch;
        // 10-08: BOTH arms come from the RIGHT arm's SPAS idle, the left as its mirror image
        // (x -> -x). The built-in left pose was the left hand under the SPAS forend: measured
        // 90.5 degrees from the mirrored right hand, upper-arm top 39 cm further forward - the
        // "twisted left hand" and the left arm "farther forward than the right". The left mesh
        // is the right one mirrored (MeasureArm's rule), so M_left = Mx M_right Mx is its exact
        // mirror pose; the two hands now match whatever way the controllers are held.
        const double sx[3] = { -1, 1, 1 };
        ArmJoints fixed = joints;
        for (int k = 0; k < 3; ++k)
        {
            const double f = side == 1 ? sx[k] : 1.0;
            fixed.s[k] = f * kNeutralArmJoints[0][k]; fixed.e[k] = f * kNeutralArmJoints[0][3 + k]; fixed.w[k] = f * kNeutralArmJoints[0][6 + k];
        }
        pose.joints = TransformArmJoints(fixed, qi);
        for (int b = 0; b < 25; ++b)
        {
            Mat4 n = BoneMatrix(kNeutralArmView[0], b);
            if (side == 1)
                for (int r = 0; r < 3; ++r) for (int c = 0; c < 4; ++c) n.m[r * 4 + c] *= sx[r] * (c < 3 ? sx[c] : 1.0);
            StoreBone(pose.bones, b, Multiply(qi, n));
        }
        if (FAILED(stream->SetPrivateData(kStableArmGuid[side], sizeof(pose), &pose))) { stream->Release(); return false; }
        Log("[ARMS] %s neutral pose = the fixed pose (right arm's SPAS idle%s, built in); the gun in hand no longer changes it", side == 0 ? "right" : "left", side == 1 ? " mirrored" : "");
        bytes = sizeof(pose);
    }
    const bool have = SUCCEEDED(stream->GetPrivateData(kStableArmGuid[side], &bytes, &pose)) && bytes == sizeof(pose) && pose.epoch == g_armPoseEpoch;
    if (!have && persist && g_savedNeutral.valid[side])
    { // the saved straight pose, whatever gun is out
        pose = g_savedNeutral.pose[side]; pose.epoch = g_armPoseEpoch; g_neutralProfile[side] = g_savedNeutral.profile[side];
        if (FAILED(stream->SetPrivateData(kStableArmGuid[side], sizeof(pose), &pose))) { stream->Release(); return false; }
        Log("[ARMS] %s neutral pose from the saved file (learned holding profile %ld)", side == 0 ? "right" : "left", g_savedNeutral.profile[side]);
    }
    // No saved pose and a non-SPAS pose in use: the first quiet SPAS hold replaces it (and is saved).
    const bool upgrade = have && persist && !g_savedNeutral.valid[side] && g_weaponMechanicProfile == kNeutralReferenceProfile &&
                         g_frame - g_weaponMechanicFrame <= 2 && g_neutralProfile[side] != kNeutralReferenceProfile;
    if ((!have && !(persist && g_savedNeutral.valid[side])) || upgrade)
    {
        // The neutral pose is captured ONCE per session (SPAS: the left hand was no longer straight, it
        // was completely twisted): it was taken on the first stable frame, in the middle of the weapon's equip animation
        // (log: captured right after the SPAS pieces appeared). Only a QUIET game arm may define the neutral: the raw wrist
        // must have moved under 4 mm and turned under 1 degree per frame for 12 consecutive frames. Until then the game's own
        // arm is drawn. After 4 s it is taken anyway (logged) so a swaying idle can never block it forever.
        static Mat4 lastWrist[2] = {}; static int quietFrames[2] = {}; static LONG firstTry[2] = {-1,-1}; static LONG lastFrame[2] = {-100000,-100000};
        if (g_neutralQuietGate)
        {
            const Mat4 wrist = BoneMatrix(bones, 2);
            if (g_frame - lastFrame[side] > 2) { quietFrames[side] = 0; firstTry[side] = g_frame; }
            else
            {
                double move = 0, turn = 0;
                for (int r = 0; r < 3; ++r) { move += (wrist.m[r*4+3] - lastWrist[side].m[r*4+3]) * (wrist.m[r*4+3] - lastWrist[side].m[r*4+3]);
                    for (int c = 0; c < 3; ++c) turn = (std::max)(turn, std::fabs(wrist.m[r*4+c] - lastWrist[side].m[r*4+c])); }
                quietFrames[side] = std::sqrt(move) < .004 && turn < .0175 ? quietFrames[side] + 1 : 0;
            }
            lastWrist[side] = wrist; lastFrame[side] = g_frame;
            const bool forced = g_frame - firstTry[side] > 240;
            if (upgrade && quietFrames[side] < 12) goto useExisting; // keep this session's pose until the SPAS arm is quiet
            if (!upgrade && quietFrames[side] < 12 && !forced && !ReloadPoseActive()) { stream->Release(); return false; } // game arm until it is quiet
            if (forced) Log("[ARMS] %s neutral pose: the game arm never went quiet for 4 s; captured anyway", side == 0 ? "right" : "left");
            else Log("[ARMS] %s neutral pose: game arm quiet for %d frames", side == 0 ? "right" : "left", quietFrames[side]);
        }
        pose.epoch = g_armPoseEpoch;
        pose.joints = TransformArmJoints(joints, qi);
        const Mat4 toBody = Multiply(qi, toView);
        for (int b = 0; b < 25; ++b) StoreBone(pose.bones, b, Multiply(toBody, BoneMatrix(bones, b)));
        if (FAILED(stream->SetPrivateData(kStableArmGuid[side], sizeof(pose), &pose))) { stream->Release(); return false; }
        g_neutralProfile[side] = g_weaponMechanicProfile;
        Log("[ARMS] %s neutral pose captured holding profile %ld; game arm animation replaced by controller IK", side == 0 ? "right" : "left", g_weaponMechanicProfile);
        if (persist && g_weaponMechanicProfile == kNeutralReferenceProfile && g_frame - g_weaponMechanicFrame <= 2) SaveNeutral(side, pose);
    }
useExisting:
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

#include "car_wheel352.inl"
#include "captured_hand_control.inl"
#include "shotgun_support56.inl"
#include "manual_reload.inl"
#include "car_door357.inl"

#include "reload_signal_gate.inl"
Mat4 PistolInteractionRoot(const Mat4&);
void Native1911GuideRequest48(const Mat4&,int);
void NativeWeaponBoxRequest50(const Mat4&,int);void NativeReloadPoints50(V3,V3,float,float,bool,bool,int);
#include "a2000_left336.inl"
#include "pistol_box_geometry.inl"
bool Left1911Reload312();
V3 PhysicalReloadWrist312(const V3&);
#include "sidearm_role315.inl"
#include "shotgun_role318.inl"
#include "other_role319.inl"
#include "ak_role316.inl"
#include "pistol_reload.inl"
#include "pistol_mechanics.inl"
#include "a2000_full337.inl"
#include "pistol_guide_menu.inl"
#include "pistol_native_guide48.inl"
#include "weapon_native_guides50.inl"
#include "pistol_slide_tint49.inl"
#include "pistol_rack_source.inl"
#include "machete_hold.inl"
#include "bow.inl"
#include "bow_sight54.inl"
#include "muzzle_render58.inl"
#include "m133_sight57.inl"
#include "bow_native_guides51.inl"
#include "weapon_guides_bridge51.inl"
#include "machete_collider_guides118.inl"
#include "bow_arrow.inl"
#include "repair_tool.inl"
#include "muzzle_flash111.inl"

#include "controller_grip124.inl"
#include "support_contact236.inl"
#include "svd_lens236.inl"
#include "vector_casing231.inl"
#include "rifle_casing235.inl"
#include "revolver_assembly217.inl"
#include "throwables144.inl"
void RecordHandUiWrist(const Mat4& skin)
{
    // Bone palettes transform bind-space vertices; their translation is not
    // the joint position. Use the same native wrist point as NativeArmSide.
    const V3 wrist=Apply(skin,{-.591,.011,1.149});
    for(int k=0;k<16;++k)g_handUiPose[k]=float(skin.m[k]);
    // The native left-arm draw has a mirrored bind basis (verified in live
    // palettes). A quad pose must be a proper rotation. Remove that bind X
    // reflection after evaluating the real wrist position; never mirror its position.
    const double determinant=skin.m[0]*(skin.m[5]*skin.m[10]-skin.m[6]*skin.m[9])-skin.m[1]*(skin.m[4]*skin.m[10]-skin.m[6]*skin.m[8])+skin.m[2]*(skin.m[4]*skin.m[9]-skin.m[5]*skin.m[8]);
    if(determinant<0){g_handUiPose[0]*=-1;g_handUiPose[4]*=-1;g_handUiPose[8]*=-1;}
    g_handUiPose[3]=float(wrist.x);g_handUiPose[7]=float(wrist.y);g_handUiPose[11]=float(wrist.z);
    g_handUiPoseFrame=g_frame;
}

struct PhysicalArmScope303 {
 bool active=false,oldValid[2]={},oldAnatomical=false;float oldBody[2][16]={},oldPose[2][16]={};
 PhysicalArmScope303(){const auto state=fc3hands303::ReadPresentation314();active=state.enabled&&state.engaged&&(state.empty||fc3hands303::PhysicalHand(state,0)==1)&&MotionInteractionEnabled();if(!active)return;
  oldAnatomical=g_renderAnatomical303;g_renderAnatomical303=true;memcpy(oldBody,g_handBody,sizeof(oldBody));memcpy(oldPose[0],g_handRight,sizeof(g_handRight));memcpy(oldPose[1],g_handLeft,sizeof(g_handLeft));oldValid[0]=g_handRightValid;oldValid[1]=g_handLeftValid;
  g_handRightValid=g_physicalValid303[0];g_handLeftValid=g_physicalValid303[1];
  for(int hand=0;hand<2;++hand)if(g_physicalValid303[hand]){memcpy(g_handBody[hand],g_physicalBody303[hand],sizeof(g_handBody[hand]));Mat4 body={};for(int i=0;i<16;++i)body.m[i]=g_physicalBody303[hand][i];const auto camera=Multiply(ArmBodyToCamera(),body);float* target=hand?g_handLeft:g_handRight;for(int i=0;i<16;++i)target[i]=float(camera.m[i]);}
 }
 ~PhysicalArmScope303(){if(!active)return;memcpy(g_handBody,oldBody,sizeof(oldBody));memcpy(g_handRight,oldPose[0],sizeof(g_handRight));memcpy(g_handLeft,oldPose[1],sizeof(g_handLeft));g_handRightValid=oldValid[0];g_handLeftValid=oldValid[1];g_renderAnatomical303=oldAnatomical;}
};
#include "held_hand305.inl"
#include "support_hand310.inl"
#include "left_reload312.inl"
#include "sidearm_render315.inl"
#include "shotgun_hands318.inl"
#include "other_hands319.inl"
#include "revolver_modifier321.inl"
struct PhysicalArmScope303;
void AkRole316(float*);bool LeftAkHolding316(const PhysicalArmScope303&,int,Mat4&);
void AkMirrorFingers316(float*,const float*);bool RightAkSupport316(const PhysicalArmScope303&,int,Mat4&);
ID3D11Buffer* BuildArmPalette(ID3D11DeviceContext* context, int side)
{
    if(ID3D11Buffer* car=BuildCarArm352(context,side))return car;
    PhysicalArmScope303 physicalArm303;const bool anatomical303=physicalArm303.active;
    const bool previewCached=!anatomical303&&side==1&&AmmoPreviewPoseAvailable();
    const bool tracked = side == 0 ? g_handRightValid : g_handLeftValid;
    if ((!previewCached&&((!MotionInteractionEnabled()&&!ThrowableArmSide150(side))||!g_eyeViewsSet||!tracked||!g_armJoints[side].valid))||!g_armPaletteCb[side])return nullptr;
    float bones[48 * 12] = {}; Mat4 toView = {}, fromView = {};
    if (!DrawSpace(context, bones, toView, fromView)) return nullptr;
    CaptureNativeSupport310(physicalArm303,side,toView,bones);
    CaptureShotgun318(physicalArm303,side,toView,bones);
    CaptureOther319(side,toView,bones);
    CaptureSidearmGrip315(side,bones);
    if(side==1&&SkorpionModel())CapturePistolSlideGrip(context,bones); // raw native reload pose before controller override
    if(previewCached&&!tracked){
        if(!RestoreAmmoPreviewArm(context,bones,fromView))return nullptr;
        PistolGrip(bones);const bool own=g_own;g_own=true;
        RecordAnimationFinal(bones,side);g_oUpdateSubresource(context,g_armPaletteCb[side],0,nullptr,bones,0,0);g_own=own;
        return g_armPaletteCb[side];
    }
    float original[25 * 12]; std::memcpy(original, bones, sizeof(original));
    ArmJoints joints = g_armJoints[side];
    // Preserve the confirmed right-hand weapon attachment. The free/right arm
    // in weapon-restricted areas, and the free left arm, use neutral IK.
    // While gripping, retain the game grip pose and the confirmed pump-slide correction.
    const bool manualLeft=!anatomical303&&side==1&&((!M133ControllerSupport356()&&ManualReloadOwnsLeftHand())||PistolOwnsLeft()||PistolAmmoPreviewActive());
    // Manual M133 reload uses the pre231 controller-owned hand; the failed
    // seating experiments are removed, including their render-time sampling.
    // 1911 physical chambering uses the tracked-controller slide stroke, not animated palm travel.
    // Configured X reload fallbacks retain their original animated hand pose.
    const bool pkmArms=!anatomical303&&PkmArmsNative(); // PKM: the game's reload animation owns both arms (on the gun in the hand)
    const bool revolverNative=!anatomical303&&side==1&&(RevolverNativeLeft()||SpasNativeLeft()||R700NativeLeft()||pkmArms); // Simple revolver / SPAS reload: Jason's left arm, moved with the gun
    const bool nativeReload=!anatomical303&&(revolverNative||(side==1&&!PistolAmmoPreviewActive()&&!g_manualReloadEnabled&&ReloadPoseActive()));
    // The right hand played the firing animation: the GM-94 animates both arms after every shot (its pump); like the
    // R700, both arms use the fixed arm - the game's arm bones never reach the hands.
    // The D50 deliberately waits for 15 quiet FRAME draws before learning its
    // controller attachment. Until then, let its right hand follow the same
    // native draw as the gun; a fixed/controller hand beside an equip-animated
    // gun is the reported first-equip clip. Do not change the 1911 or the
    // accepted two-handed D50 path.
    // Every right-hand palette starts from the fixed pose. Native locomotion,
    // firing and equip wrists never become the visual right-hand source.
    const bool stable = anatomical303 || side==0 || BowModel() ||
        (R700Model()||Gm94Model()||((!g_twoHand||manualLeft||(AkReloadSupport()&&PistolPoseOwned()))&&!nativeReload));
    if(side==1&&!anatomical303){const V3 nativePalm=Apply(Multiply(toView,BoneMatrix(bones,2)),{-.6781,.0519,1.0387});g_supportPalmDelta=Sub(nativePalm,{joints.w[0],joints.w[1],joints.w[2]});g_supportPalmFrame=g_frame;g_supportPalmProfile=g_weaponMechanicProfile;g_supportPalmEpoch=g_armPoseEpoch;}
    if(side==0&&PistolHandling()&&!stable)
    {float neutral[48*12];std::memcpy(neutral,bones,sizeof(neutral));ArmJoints unused=joints;RestoreStableArmPose(context,side,toView,fromView,neutral,unused);}
    if (stable && !RestoreStableArmPose(context, side, toView, fromView, bones, joints)) return nullptr;
    Mat4 handMove = {};
    if (stable||nativeReload)
        handMove = StableControllerHandMove(side, { joints.w[0], joints.w[1], joints.w[2] });
    else if (!(side == 0 ? RightHandMove(handMove) : LeftMove(joints, g_armJoints[0], handMove))) return nullptr;
    { // Free-hand contact is controller-parented, before weapon parts pin the visible hand.
        float neutral[48*12];std::memcpy(neutral,bones,sizeof(neutral));ArmJoints nj=joints;
        if(stable||RestoreStableArmPose(context,side,toView,fromView,neutral,nj)){
            const Mat4 freeMove=StableControllerHandMove(side,{nj.w[0],nj.w[1],nj.w[2]});
            Mat4 ci={},qi={},controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[side][i];
            if(Invert(controller,ci)&&Invert(ArmBodyToCamera(),qi)){
                auto& a=g_freeHandAttachment[side];a.relative=Multiply(ci,Multiply(qi,Multiply(freeMove,Multiply(toView,BoneMatrix(neutral,2)))));a.epoch=g_armPoseEpoch;a.frame=g_frame;a.profile=g_weaponMechanicProfile;
                const V3 palm303=Apply(a.relative,{side?-.6781:.6781,.0519,1.0387});const float local303[]={float(palm303.x),float(palm303.y),float(palm303.z)};fc3xr::SetHandPalm303(side,local303,g_armPoseEpoch);

            }
        }
    }
    bool rifleBound166=false,skorpionBound166=false;
    if(!anatomical303){
    if(BowModel()&&!anatomical303)RecordBowFreeHand(side,Multiply(handMove,Multiply(toView,BoneMatrix(bones,2))));
    if(BowModel()&&!MacheteHeld())BowArmMove(side,toView,bones,handMove);
    if(side==0&&stable)RecordR700FreeHand(Multiply(handMove,Multiply(toView,BoneMatrix(bones,2)))); // the fixed hand on the controller (R700 root)
    if(side==0&&!MacheteHeld()){if(!RpgClipMove(0,toView,bones,handMove)&&!R700RightClipMove(toView,bones,handMove)&&!Lever1887RightHoldingMove(toView,bones,handMove))PistolRightHoldingMove(toView,bones,handMove);} // R700: Jason's right hand works the bolt on the gun
    else if(side==1&&!Lever1887LeftSupportMove(toView,bones,handMove))AkSupportHandMove(toView,bones,handMove); // left arm only (right arm must never take the forend placement)
    if(revolverNative){RevolverLeftNativeMove(handMove);SpasLeftNativeMove(handMove);R700LeftNativeMove(handMove);}
    if(side==1){FlamerLeftLeverMove(toView,bones,handMove);Gm94LeftSupportMove(toView,bones,handMove);RpgClipMove(1,toView,bones,handMove);RevolverLeftClipMove(toView,bones,handMove);SpasLeftClipMove(toView,bones,handMove);SpasLeftSupportMove(toView,bones,handMove);rifleBound166=RifleChargeHandMove(toView,bones,handMove);skorpionBound166=SkorpionChargeHandMove52(toView,bones,handMove);R700LeftSupportMove(toView,bones,handMove);RepairLeftMove249(toView,bones,handMove);}
    if(pkmArms){Mat4 gunMove={};if(PkmGunMove(gunMove))handMove=gunMove;} // both arms: the game's pose moved with the gun // SPAS forend / rifle charging handle held: Jason's hand on it // Jason's revolver / SPAS insert animation owns the left wrist
    if(side==0){
        Mat4 desired={},inverse={};
        if((ThrowableRightHand144(desired)||ControllerRightHand124(desired))&&Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))handMove=Multiply(desired,inverse);
    }
    else if(!PlaceableArm177(side,toView,bones,handMove))GrenadePinArm154(toView,bones,handMove);
    }
    if(side==0&&(fc3xr::SelectorOpen291()||fc3xr::WeaponHolsterWristNeeded297())){
        const Mat4 wrist=Multiply(toView,BoneMatrix(bones,2));
        Mat4 bind={};bind.m[0]=bind.m[5]=bind.m[10]=bind.m[15]=1;bind.m[3]=.591;bind.m[7]=.011;bind.m[11]=1.149;
        Mat4 qi={};if(Invert(ArmBodyToCamera(),qi)){
            const Mat4 desired=Multiply(qi,Multiply(handMove,Multiply(wrist,bind)));float body[16];for(int i=0;i<16;++i)body[i]=float(desired.m[i]);fc3xr::SetSelectorWrist292(body);
        }
        if(!anatomical303)SelectorContactMove292(handMove,wrist);
    }
    Mat4 leftHold305={},holdInverse305={};
    const bool fittedLeft305=Left1911Holding305(physicalArm303,side,leftHold305)&&Invert(Multiply(toView,BoneMatrix(bones,2)),holdInverse305);
    if(fittedLeft305)handMove=Multiply(leftHold305,holdInverse305);
    Mat4 leftOther315={},leftInverse315={};
    const bool fittedOther315=LeftSidearmHolding315(physicalArm303,side,leftOther315)&&Invert(Multiply(toView,BoneMatrix(bones,2)),leftInverse315);
    if(fittedOther315)handMove=Multiply(leftOther315,leftInverse315);
    Mat4 akHand316={},akInverse316={};
    const bool fittedAk316=LeftAkHolding316(physicalArm303,side,akHand316)&&Invert(Multiply(toView,BoneMatrix(bones,2)),akInverse316);
    if(fittedAk316)handMove=Multiply(akHand316,akInverse316);
    const bool supportAk316=RightAkSupport316(physicalArm303,side,akHand316)&&Invert(Multiply(toView,BoneMatrix(bones,2)),akInverse316);
    if(supportAk316)handMove=Multiply(akHand316,akInverse316);
    Mat4 rightSupport310={},supportInverse310={};
    const bool fittedSupport310=Right1911Support310(physicalArm303,side,rightSupport310)&&Invert(Multiply(toView,BoneMatrix(bones,2)),supportInverse310);
    if(fittedSupport310)handMove=Multiply(rightSupport310,supportInverse310);
    Mat4 rightReload312={},reloadInverse312={};
    if(RightReloadSlide312(physicalArm303,side,rightReload312)&&Invert(Multiply(toView,BoneMatrix(bones,2)),reloadInverse312))handMove=Multiply(rightReload312,reloadInverse312);
    Mat4 sidearmReload315={},sidearmInverse315={};
    if(RightSidearmReloadMove315(physicalArm303,side,sidearmReload315)&&Invert(Multiply(toView,BoneMatrix(bones,2)),sidearmInverse315))handMove=Multiply(sidearmReload315,sidearmInverse315);
    Mat4 shotgunHand318={},shotgunInverse318={};
    if(ShotgunArm318(physicalArm303,side,shotgunHand318)&&Invert(Multiply(toView,BoneMatrix(bones,2)),shotgunInverse318))handMove=Multiply(shotgunHand318,shotgunInverse318);
    Mat4 otherHand319={},otherInverse319={};
    if(OtherArm319(physicalArm303,side,otherHand319)&&Invert(Multiply(toView,BoneMatrix(bones,2)),otherInverse319))handMove=Multiply(otherHand319,otherInverse319);
    if(RpgSupportArm349(physicalArm303,side,otherHand319)&&Invert(Multiply(toView,BoneMatrix(bones,2)),otherInverse319))handMove=Multiply(otherHand319,otherInverse319);
    Mat4 bowHand319={},bowInverse319={};
    if(OtherBowArm319(physicalArm303,side,bowHand319)&&Invert(Multiply(toView,BoneMatrix(bones,2)),bowInverse319))handMove=Multiply(bowHand319,bowInverse319);
    Mat4 forearmMove = {}, upperMove = {};
    // Keep the tracked wrist exact during insertion, as the native reload path did.
    const bool manualInsert=manualLeft&&!g_reload.held&&!g_reloadAlignment;
    const bool holding = side == 1 && ((g_twoHand&&!manualLeft)||nativeReload||manualInsert||RevolverClipPlaying()||RpgClipPlaying()||SpasClipPlaying()||SpasForendHeld()||RifleChargeHandHeld()||R700LeftSupportActive());
    const bool armIk84=g_armIk84&&!nativeReload&&!pkmArms&&!ReloadPoseActive(); // candidate84: 77/78 solver on ordinary arms only
    if (!SolveArm(joints, handMove, side, forearmMove, upperMove, true, armIk84)) return nullptr; // hand on the controller; ordinary IK uses a body-relative elbow
    const V3 shirtSlide = g_armSlide[side];  // taken now: later calls (gun, grips) may solve again
    if(armIk84){
        static ULONGLONG next84[2]={};static unsigned samples84[2]={};
        const ULONGLONG now84=GetTickCount64();
        if(now84>=next84[side]&&samples84[side]<2400){
            next84[side]=now84+250;++samples84[side];
            const V3 hand84=Apply(handMove,{joints.w[0],joints.w[1],joints.w[2]});
            Log("[ARMIK84] frame%ld side%d profile%ld twoHand%d stretch %.4f shoulderSlide %.4f/%.4f/%.4f elbow %.4f/%.4f/%.4f hand %.4f/%.4f/%.4f",g_frame,side,g_weaponMechanicProfile,g_twoHand?1:0,g_armStretch77[side],shirtSlide.x,shirtSlide.y,shirtSlide.z,g_armElbow77[side].x,g_armElbow77[side].y,g_armElbow77[side].z,hand84.x,hand84.y,hand84.z);
        }
    }
    const Mat4 viewMoves[3] = { upperMove, forearmMove, handMove };
    // Forearm twist (the forearm skin was visibly twisted): bones 20 / 21 are the forearm's twist bones (20 meets
    // the wrist). They moved rigidly with the forearm, so every roll of the hand landed in the wrist and wrung the skin.
    // They now take 70 % / 35 % of the hand's roll about the forearm axis (swing-twist split of hand vs forearm move).
    Mat4 twistMoves[2] = { forearmMove, forearmMove };
    {
        Mat4 forearmInverse = {};
        const V3 e = Apply(forearmMove, { joints.e[0], joints.e[1], joints.e[2] }), w = Apply(forearmMove, { joints.w[0], joints.w[1], joints.w[2] });
        V3 axis = Sub(w, e); const double axisLength = Length(axis);
        if (axisLength > 1e-6 && Invert(forearmMove, forearmInverse))
        {
            axis = Scale(axis, 1 / axisLength);
            Mat4 twistInverse=forearmInverse,rigidInverse={};
            if(armIk84&&Invert(g_armForearmRigid77[side],rigidInverse))twistInverse=rigidInverse;
            const Mat4 rel = Multiply(handMove, twistInverse); // longitudinal extension is not wrist twist
            const double* m = rel.m; double qw = 0, qx = 0, qy = 0, qz = 0; const double tr = m[0] + m[5] + m[10];
            if (tr > 0) { const double s = std::sqrt(tr + 1) * 2; qw = .25 * s; qx = (m[9] - m[6]) / s; qy = (m[2] - m[8]) / s; qz = (m[4] - m[1]) / s; }
            else if (m[0] > m[5] && m[0] > m[10]) { const double s = std::sqrt(1 + m[0] - m[5] - m[10]) * 2; qw = (m[9] - m[6]) / s; qx = .25 * s; qy = (m[1] + m[4]) / s; qz = (m[2] + m[8]) / s; }
            else if (m[5] > m[10]) { const double s = std::sqrt(1 + m[5] - m[0] - m[10]) * 2; qw = (m[2] - m[8]) / s; qx = (m[1] + m[4]) / s; qy = .25 * s; qz = (m[6] + m[9]) / s; }
            else { const double s = std::sqrt(1 + m[10] - m[0] - m[5]) * 2; qw = (m[4] - m[1]) / s; qx = (m[2] + m[8]) / s; qy = (m[6] + m[9]) / s; qz = .25 * s; }
            double twist = 2 * std::atan2(qx * axis.x + qy * axis.y + qz * axis.z, qw);
            if (twist > 3.14159265358979) twist -= 6.28318530717959; else if (twist < -3.14159265358979) twist += 6.28318530717959;
            const double share[2] = { .70, .35 };
            for (int k = 0; k < 2; ++k)
            {
                const double a = twist * share[k], c = std::cos(a), s = std::sin(a), t = 1 - c; const double u[3] = { axis.x, axis.y, axis.z };
                Mat4 roll = {}; roll.m[15] = 1;
                for (int r = 0; r < 3; ++r) for (int col = 0; col < 3; ++col) roll.m[r * 4 + col] = (r == col ? c : 0) + t * u[r] * u[col];
                roll.m[1] -= s * u[2]; roll.m[2] += s * u[1]; roll.m[4] += s * u[2]; roll.m[6] -= s * u[0]; roll.m[8] -= s * u[1]; roll.m[9] += s * u[0];
                const V3 turned = Apply(roll, w); roll.m[3] = w.x - turned.x; roll.m[7] = w.y - turned.y; roll.m[11] = w.z - turned.z; // about the moved wrist
                twistMoves[k] = Multiply(roll, forearmMove);
            }
        }
    }
    for (int b = 0; b < 25; ++b)
    {
        const Mat4& groupMove = b == 20 ? twistMoves[0] : b == 21 ? twistMoves[1] : viewMoves[ArmGroup(b)];
        const Mat4 desired = Multiply(groupMove, Multiply(toView, BoneMatrix(bones, b)));
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
    const bool macheteGrip=!anatomical303&&side==0&&MacheteGrip(context,bones);
    const bool capturedGrip=!anatomical303&&side==1&&CapturedReloadGrip(context,bones);
    const bool pistolGrip=!anatomical303&&(BowGrip(side,bones)||(side==1?(RepairLeftGrip249(bones)||FlamerLeftGrip(bones)||Gm94LeftGrip(bones)||RpgLeftGrip(bones)||FlareLeftGrip(bones)||R700LeftGrip(bones)||PistolGrip(bones)||Lever1887LeftGrip(bones)||RevolverLeftGrip(bones)||SpasLeftGrip(bones)):(RpgRightGrip(bones)||R700RightGrip(bones)||PistolRightHoldingGrip(bones)||Lever1887RightGrip(bones))));
    if(!anatomical303){if(side==0){Record1887RightHand(context,bones);RecordR700RightHand(context,bones);}else{Record1887LeftHand(context,bones);RecordFlareLeftHand(context,bones);RecordRpgLeftHand(context,bones);RecordGm94LeftHand(context,bones);RecordFlamerLeftHand(context,bones);RecordRevolverLeftHand(context,bones);RecordSpasLeftHand(context,bones);}}
    if(OtherBowFingers319(side,bones)||OtherFingers319(side,bones)||ShotgunFingers318(side,bones)||RightReloadFingers312(side,bones)||RightSidearmReloadFingers315(side,bones)){}
    else if(fittedSupport310)Right1911SupportFingers310(bones);
    else if(fittedLeft305)Left1911Fingers305(bones);
    else if(fittedOther315)LeftSidearmFingers315(bones);
    else if(fittedAk316)AkMirrorFingers316(bones,CurrentRifle().rightGrip);
    else if(supportAk316)AkMirrorFingers316(bones,CurrentRifle().supportGrip);
    else if(anatomical303||(!capturedGrip&&!macheteGrip&&!pistolGrip&&!nativeReload&&!pkmArms))CurlFingers(bones, side);
    if(!anatomical303)ThrowableFingers144(side,bones);
    if(!anatomical303&&side==0&&!nativeReload&&!pkmArms&&!macheteGrip&&!pistolGrip&&!BowModel()&&!ThrowableOwns144()&&g_weaponMechanicProfile>=0)TriggerFinger262(bones); // [TRIGGER262] game-animated grips only; captured right grips (1911/RPG/R700/1887) are accepted fits
    if(side==0&&!anatomical303)SelectorFingers292(bones);
    if(side==1&&(g_pistolReload.rackHeld||g_pistolReload.rackReturning)){
        static ULONGLONG next166=0;static unsigned rows166=0;const auto now166=GetTickCount64();
        if(now166>=next166&&rows166<2400){next166=now166+100;++rows166;const auto& p=g_pistolReload;const V3 wrist=Apply(Multiply(toView,BoneMatrix(bones,2)),{0,0,0});
            Log("[HANDLE166] ms=%llu frame=%ld profile=%ld held=%d return=%d rear=%d pull=%.6f rifleBound=%d skorpionBound=%d rootAge=%ld epochOk=%d tracking=%d/%d pending=%d cycle=%d magazineHeld=%d removal=%d gripPose=%d finalWristView=%.5f/%.5f/%.5f controllerBody=%.5f/%.5f/%.5f",now166,g_frame,g_weaponMechanicProfile,p.rackHeld,p.rackReturning,p.rackRear,p.slidePull,rifleBound166,skorpionBound166,g_frame-p.rootFrame,p.rootEpoch==g_armPoseEpoch,g_handLeftValid,g_handRightValid,p.pending,p.cycle,p.held,p.removalHeld,pistolGrip,wrist.x,wrist.y,wrist.z,g_handBody[1][3],g_handBody[1][7],g_handBody[1][11]);
        }
    }

    if(side==0&&MacheteHeld()&&g_machete.handAttachment116&&g_machete.handEpoch116==g_armPoseEpoch){
        Mat4 pinned={},inverse={};if(MacheteHand116(pinned)&&Invert(BoneMatrix(bones,2),inverse))MoveBones(bones,2,19,Multiply(Multiply(fromView,pinned),inverse));
    }
    if(side==0&&macheteGrip)RecordMacheteHand(context,bones);
    // Candidate 112 (motion controls did not work with the machete out until RT, then only for a second): the
    // hand was recorded only from the game's own machete-arm mesh, which exists only during a native swipe, so the
    // replay lost its hand every other frame. The hand bone (2) is the same on every arm mesh and this palette already
    // follows the controller while the machete is held, so the blade follows it whatever arm the game draws.
    else if(side==0&&MacheteHeld())RecordMacheteHand(context,bones,false);
    if(side==1){if(!(LeftShotgun318()&&M133SupportProfile56())){if(capturedGrip)RecordReloadHand(context,bones);else g_reloadContact.hand=false;}if(!Left1911Reload312())RecordPistolHand(context,bones);CacheAmmoPreviewPose(context,bones);}
    RecordRightReload312(context,side,bones);
    RecordRightSidearmReload315(context,side,bones);
    RecordShotgunHands318(context,side,bones);
    RecordOtherHands319(context,side,bones);
    RecordOtherBow319(context,side,bones);
    { // the final arm bones in body axes, for the shirt (drawn with the other camera, earlier in the frame)
        const Mat4 qb = ArmBodyToCamera();
        Mat4 qi = {}; qi.m[15] = 1;
        for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) qi.m[r * 4 + c] = qb.m[c * 4 + r];
        for (int b = 0; b < 25; ++b)
        {
            Mat4 fv = Multiply(toView, BoneMatrix(bones, b));
            if (ArmGroup(b) == 0 && !armIk84) { fv.m[3] -= shirtSlide.x; fv.m[7] -= shirtSlide.y; fv.m[11] -= shirtSlide.z; } // ordinary IK: shirt sleeve reaches visual shoulder
            g_armFinalBody[side][b] = Multiply(qi, fv);
        }
        g_armFinalFrame[side] = g_frame;
        if(side==1)RecordHandUiWrist(g_armFinalBody[side][2]);
    }
    RecordDisplayedSupport311(physicalArm303,side);
    RecordShotgunSupport318(physicalArm303,side);
    const bool own = g_own; g_own = true;
    RecordAnimationFinal(bones,side);
    g_oUpdateSubresource(context, g_armPaletteCb[side], 0, nullptr, bones, 0, 0);
    g_own = own;
    if(side==0&&MacheteHeld())ReplayMachete(context);
    if((side==1&&capturedGrip)||(side==0&&LeftShotgun318()&&M133SupportProfile56()&&g_reload.held))ReplayReloadShell(context); // shell uses this exact completed hand palette
    if (InterlockedIncrement(&g_armPaletteDraws) == 1)
        Log("[ARMS] arm bones rewritten for the first time (%s arm)", side == 0 ? "right" : "left");
    return g_armPaletteCb[side];
}


#include "transfer1911_307.inl"
#include "ak_hands316.inl"
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

// Village probe (with a weapon out in the village the right stick no longer rotated the view until leaving the
// village): every 2 s in gameplay, the state that decides who owns the camera and the stick.
void VillageProbe(bool headset,bool want)
{
    static ULONGLONG next=0;static LONG lines=0;static bool lastWant=false;static LONG lastDraws=-1;
    const ULONGLONG now=GetTickCount64();const bool drawsChanged=(g_visibleWeaponDrawsLastFrame>0)!=(lastDraws>0);
    if(!g_inGameplay||(now<next&&want==lastWant&&!drawsChanged)||lines>=600)return;
    next=now+2000;lastWant=want;lastDraws=g_visibleWeaponDrawsLastFrame;++lines;
    const auto c=fc3pad::Latest();const float* f=g_frameCamera.forward;
    Log("[VILLAGE PROBE] weapon draws %ld guard %d motion %d | aim want %d ready %d enabled %d body %d | stick R %.2f %.2f L %.2f %.2f | camera yaw %.1f pitch %.1f valid %d | body yaw %.1f | headset %d input blocked %d",
        g_visibleWeaponDrawsLastFrame,g_weaponMotionGuard?1:0,MotionInteractionEnabled()?1:0,want?1:0,fc3aim::Ready()?1:0,g_aimEnabled?1:0,g_bodyValid?1:0,
        c.rightX,c.rightY,c.leftX,c.leftY,std::atan2(f[1],f[0])*57.29578,std::asin((std::max)(-1.f,(std::min)(1.f,f[2])))*57.29578,g_frameCameraValid?1:0,
        g_bodyYaw*57.29578,headset?1:0,fc3pad::GameplayInputBlocked()?1:0);
    // Observe no-weapon camera transitions (including vehicles) without driving
    // the player look setter, which also controls vehicle steering/aim ownership.
    // These are rendered-camera bounds, not proof of the engine's CPU cull camera.
    if(headset&&g_eyeViewsSet&&g_frameCameraValid&&g_visibleWeaponDrawsLastFrame==0&&!fc3pad::GameplayInputBlocked()){
        const float* v=g_eyeViews[0].view;
        const double headYaw=std::atan2(double(v[8]),double(v[10]))*57.29578;
        const double headPitch=std::atan2(double(v[9]),std::hypot(double(v[8]),double(v[10])))*57.29578;
        const double halfX=g_worldProjection.xs>0?std::atan(1.0/g_worldProjection.xs)*57.29578:0;
        const double halfY=g_worldProjection.ys>0?std::atan(1.0/g_worldProjection.ys)*57.29578:0;
        Log("[CULL40] no-weapon camera: frame %ld world votes %ld scale %.4f %.4f halfFOV %.1f %.1f head-relative %.1f %.1f bodyComp %d wideningCovered %d; rendered camera only",
            long(g_frame),long(g_mainBinCount),g_worldProjection.xs,g_worldProjection.ys,halfX,halfY,headYaw,headPitch,g_aimActive?1:0,fc3fov::Covered()?1:0);
    }
}
 #include "rifle_aim_assist.inl"
#include "camera_cone67.inl"
#include "visibility_camera68.inl"
#include "vehicle_look102.inl"
#include "scope_stabilization72.inl"
#include "reticle_aim73.inl"
// A temporary missing gun during a physical throw is not a holster/vehicle
// transition. Preserve the established body basis until its gun returns.
bool ThrowableCamera153(bool headset)
{
    bool seated=false;int seat=-1;
    return (ThrowableOwns144()||ThrowableArmLease150())&&g_bodyValid&&g_motionGun&&g_enabled&&g_headTracking&&!g_flatViewActive&&g_aimEnabled&&headset&&g_eyeViewsSet&&g_inGameplay&&g_frameCameraValid&&
        !g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&!fc3pad::NativeMenuOpen()&&!fc3aim::NativeActionOwnsArms128()&&
        !(fc3aim::VehicleState(seated,seat)&&seated);
}
#include "native_foot281.inl"
#include "native_action282.inl"
#include "selector_scene292.inl"

void AimFrame(bool headset)
{
    // Release a finished script before another lane can acquire its camera;
    // otherwise a delayed script cleanup could cancel a new slide command.
    if(TickNativeAction282(headset))return;
    // A confirmed native slide owns the render camera even with a weapon out.
    // Keep its animation/position but put HMD rotation through the same native
    // producer that is accepted in vehicles. No input gesture can activate it.
    const bool slide280=headset&&g_enabled&&g_headTracking&&g_eyeViewsSet&&g_inGameplay&&g_frameCameraValid&&!g_flatViewActive&&
        !g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&!fc3pad::NativeMenuOpen()&&!fc3aim::NativeActionOwnsArms128()&&fc3aim::SlideCamera280();
    if(slide280){
        float body280[9];const float* observed280[3]={g_frameCamera.right,g_frameCamera.up,g_frameCamera.forward};
        for(int i=0;i<3;++i)for(int j=0;j<3;++j)body280[i*3+j]=g_aimActive?float(g_bodyAxes[i][j]):observed280[i][j];
        if(fc3aim::SetVehicleCamera279(true,g_eyeViewsBody[0].view,g_frameCamera.right,g_frameCamera.up,g_frameCamera.forward,body280)){
            if(!g_slideCamera280){g_slideBodyValid280=g_bodyValid;Log("[SLIDE280] native slide camera acquired; retain visual heading, no gun-camera writes");}
            g_slideCamera280=g_nativeVehicleCamera279=true;fc3aim::Suspend();fc3pad::SetLookOverride(false,0,0);
            g_visibilityCamera68=g_visibilityHandoff68=false;NativeVehicleActual279(g_frameCamera);return;
        }
    }
    if(g_slideCamera280){
        g_slideCamera280=false;fc3aim::SetVehicleCamera279(false,nullptr);g_nativeVehicleCamera279=false;
        // Retain the final slide reference rather than seeding from a camera
        // that already contains HMD rotation. Restore the normal gun owner.
        g_bodyValid=g_slideBodyValid280;QueryPerformanceCounter(&g_aimLast);
        Log("[SLIDE280] slide ended; ordinary camera ownership restored with retained heading");
    }
    if(TickNativeFoot281(headset))return;
    if(fc3aim::NativeActionOwnsArms128()){
        fc3aim::SetVehicleCamera279(false,nullptr);g_nativeVehicleCamera279=false;
        fc3aim::Suspend();fc3pad::SetLookOverride(false,0,0);ResetScopeStabilization72();
        g_bodyValid=g_aimActive=g_visibilityCamera68=g_visibilityHandoff68=false;return;
    }
    // Compensation outlives a temporary controller/weapon loss. Otherwise the
    // gun-driven native camera becomes the headset view during every dropout.
    g_aimActive = g_bodyValid && headset && g_eyeViewsSet && g_inGameplay;
    const bool throwableCamera153=ThrowableCamera153(headset);
    const bool want = throwableCamera153||(!ThrowableOwns144() && !fc3pad::GameplayInputBlocked() && g_aimEnabled && headset && MotionInteractionEnabled() && g_eyeViewsSet && g_handRightValid && g_inGameplay &&
                      g_frameCameraValid && fc3aim::Ready());
    VillageProbe(headset,want);
    if(want&&g_visibilityHandoff68){
        double yaw68=g_visibilityBodyYaw68,pitch68=0,roll68=0;fc3aim::VisibilityBasis68(yaw68,pitch68,roll68);
        g_bodyYaw=yaw68;g_bodyValid=true;g_aimActive=true;QueryPerformanceCounter(&g_aimLast);
        fc3aim::EndVisibility68();g_visibilityCamera68=g_visibilityHandoff68=false;
    }
    if(want){fc3aim::SetVehicleCamera279(false,nullptr);g_nativeVehicleCamera279=false;}
    if (!want)
    {
        g_reticleAimFrame73=-1000;
        ResetScopeStabilization72();
        if(TickVisibilityCamera68(headset)){fc3pad::SetLookOverride(false,0,0);return;}
        if(TickVehicleLook102(headset))return; // vehicle camera only (never on foot): look stick follows the head
        fc3aim::Suspend();
        fc3pad::SetLookOverride(false, 0, 0);
        if (!headset || !g_inGameplay || !g_eyeViewsSet)
        {
            g_bodyValid = false;
            g_aimActive = false;
        }
        // Village fix (VillageProbe log: weapon holstered by the game -> body yaw frozen at 88.5 while the game camera
        // turned to 170 / -135 with the right stick, so the view could not turn and the game culled what was "behind"):
        // a weapon gone for more than 1 s (not a tracking blip) releases the body compensation; the view follows the
        // game camera again, and the next weapon re-seeds the body from the current camera.
        static ULONGLONG holsteredSince = 0;
        const ULONGLONG nowMs = GetTickCount64();
        if (!MotionInteractionEnabled() && g_inGameplay)
        {
            if (!holsteredSince) holsteredSince = nowMs;
            if (g_bodyValid && nowMs - holsteredSince > 1000)
            {
                g_bodyValid = false; g_aimActive = false;
                Log("[AIM] weapon holstered for over 1 s (village / no weapon): turn compensation released, view follows the game camera");
            }
        }
        else holsteredSince = 0;
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
    if(throwableCamera153){
        // Keep the same stick/body basis on both sides of release. The native
        // camera follows the head for visibility, never the throwing hand.
        // Observed draw rotation is still cancelled exactly once per eye.
        g_reticleAimFrame73=-1000;ResetScopeStabilization72();
        V3 head={};for(const auto& eye:g_eyeViewsBody){head.x+=eye.view[8];head.y+=eye.view[9];head.z+=eye.view[10];}
        V3 world={};double* v[3]={&world.x,&world.y,&world.z};
        for(int k=0;k<3;++k){*v[k]=body[0][k]*head.x+body[1][k]*head.y+body[2][k]*head.z;for(int j=0;j<3;++j)g_bodyAxes[k][j]=body[k][j];}
        if(Length(world)>1e-6){world=Unit(world);fc3aim::Drive(std::atan2(world.y,world.x),std::asin((std::max)(-1.0,(std::min)(1.0,world.z))));}
        const float* observed[3]={g_frameCamera.right,g_frameCamera.up,g_frameCamera.forward};
        for(int i=0;i<3;++i)for(int j=0;j<3;++j)g_aimQ[i][j]=observed[i][0]*body[j][0]+observed[i][1]*body[j][1]+observed[i][2]*body[j][2];
        g_aimActive=true;
        static ULONGLONG next153=0;if(GetTickCount64()>=next153){next153=GetTickCount64()+500;Log("[THROW153] stable throw camera frame%ld body %.6f; held %d cleanup %d visible %ld; no hidden-weapon basis handoff",g_frame,g_bodyYaw,ThrowableOwns144()?1:0,ThrowableArmLease150()?1:0,g_visibleWeaponDrawsLastFrame);}
        return;
    }
    // gun direction: the controller's forward axis, body frame -> world
    double h[3] = { g_handBody[0][2], g_handBody[0][6], g_handBody[0][10] };
    V3 bowAxis={};const bool bowAim=BowAimBody(bowAxis);if(bowAim){h[0]=bowAxis.x;h[1]=bowAxis.y;h[2]=bowAxis.z;}
    V3 skorpionBarrel={};
    if(SkorpionAimBody(skorpionBarrel)){h[0]=skorpionBarrel.x;h[1]=skorpionBarrel.y;h[2]=skorpionBarrel.z;}
    V3 pistolBarrel={};const bool pistolFrameAim=PistolAimBody(pistolBarrel);
    if(pistolFrameAim){h[0]=pistolBarrel.x;h[1]=pistolBarrel.y;h[2]=pistolBarrel.z;}
    V3 shotgunBarrel125={};if(OneHandShotgunAim125(shotgunBarrel125)){h[0]=shotgunBarrel125.x;h[1]=shotgunBarrel125.y;h[2]=shotgunBarrel125.z;}
    V3 leverBarrel={};const bool leverAim=!pistolFrameAim&&Lever1887AimBody(leverBarrel); // 1887: displayed barrel
    if(leverAim){h[0]=leverBarrel.x;h[1]=leverBarrel.y;h[2]=leverBarrel.z;}
    if (g_twoHand&&!bowAim&&!pistolFrameAim&&!leverAim)      // other two-handed guns, camera frame -> body frame
        for (int k = 0; k < 3; ++k)
            h[k] = g_aimQ[0][k] * g_barrelCam[0] + g_aimQ[1][k] * g_barrelCam[1] + g_aimQ[2][k] * g_barrelCam[2];
    V3 controllerAxis129={};if(FirearmAxis129(controllerAxis129)){h[0]=controllerAxis129.x;h[1]=controllerAxis129.y;h[2]=controllerAxis129.z;}
    V3 assisted={h[0],h[1],h[2]};
    if(!FirearmSteadyAllowed129()&&(!ScopeStabilizationActive72()||g_scopeAimSteady72<=0))RifleAimAssist(assisted);ScopeAimCommand72(assisted);h[0]=assisted.x;h[1]=assisted.y;h[2]=assisted.z;
    PublishReticleAim73(assisted); // before the head-relative visibility cone
    for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k) g_bodyAxes[i][k] = body[i][k];
    double d[3];
    for (int k = 0; k < 3; ++k) d[k] = body[0][k] * h[0] + body[1][k] * h[1] + body[2][k] * h[2];
    g_shotAimProfile129=g_weaponMechanicProfile;g_shotAimWorld107={d[0],d[1],d[2]};g_shotAimFrame107=g_frame+1;g_shotAimEpoch107=g_armPoseEpoch; // same ray and frame as the reticle
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
            const V3 corrected=CameraConeDirection67({hw[0],hw[1],hw[2]},{d[0],d[1],d[2]},CameraAimCone67());
            d[0]=corrected.x;d[1]=corrected.y;d[2]=corrected.z;
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

Mat4 OpticSceneMatrix132(const Mat4&,int,int);
// AimFrame runs before the support-hand decision for the next draw. On the
// exact frame that decision changes, replace its old ray with the final fixed
// barrel ray. The new visual pose, optical dot and native shot then agree.
void ReconcileGripAim193(bool previousHeld)
{
    if(previousHeld==g_twoHand||!g_aimActive)return;
    V3 finalBody={};
    if(!FirearmAxis129(finalBody)){
        g_reticleAimFrame73=g_shotAimFrame107=-1000;
        ResetScopeStabilization72();
        return;
    }
    ResetScopeStabilization72(); // never smooth across two different grip poses
    ScopeAimCommand72(finalBody);
    PublishReticleAim73(finalBody);
    const V3 world={
        g_bodyAxes[0][0]*finalBody.x+g_bodyAxes[1][0]*finalBody.y+g_bodyAxes[2][0]*finalBody.z,
        g_bodyAxes[0][1]*finalBody.x+g_bodyAxes[1][1]*finalBody.y+g_bodyAxes[2][1]*finalBody.z,
        g_bodyAxes[0][2]*finalBody.x+g_bodyAxes[1][2]*finalBody.y+g_bodyAxes[2][2]*finalBody.z};
    if(!std::isfinite(world.x)||!std::isfinite(world.y)||!std::isfinite(world.z)||Length(world)<.5){
        g_reticleAimFrame73=g_shotAimFrame107=-1000;return;
    }
    g_shotAimWorld107=Unit(world);g_shotAimProfile129=g_weaponMechanicProfile;
    g_shotAimFrame107=g_frame+1;g_shotAimEpoch107=g_armPoseEpoch;
    Log("[AIM193] profile%d %s: final grip barrel republished to reticle and shot in frame%ld",
        g_weaponMechanicProfile,g_twoHand?"on":"off",g_frame);
}
void UpdateEyeBuffers()
{
    if (!g_ctx || !g_eyeCb[0][0]) return;
    ApplyAimToPoses();
    const bool aimHandoffWasHeld = g_twoHand;
    const V3 aimHandoffPublished = g_reticleAimBody73;
    const bool aimHandoffPublishedValid = g_reticleAimEpoch73 == g_armPoseEpoch &&
        std::abs(g_frame - g_reticleAimFrame73) <= 1 && Length(aimHandoffPublished) > 1e-6;
    UpdateTwoHand();
    ReconcileGripAim193(aimHandoffWasHeld);
    // The reticle is published in UpdateAim, before UpdateTwoHand chooses the
    // grip for this draw. Report the same-frame disagreement, separately from
    // the gun-pose jump, before changing either the aim or the reticle path.
    if (aimHandoffWasHeld != g_twoHand && g_aimActive && aimHandoffPublishedValid &&
        (!g_twoHand || g_frameGunValid) && !PistolModel() && !Lever1887Model() && !BowActive()) {
        V3 rawBody = Unit({g_handBody[0][2], g_handBody[0][6], g_handBody[0][10]});
        if (g_twoHand) {
            const V3 rawCam = Unit({g_barrelCam[0], g_barrelCam[1], g_barrelCam[2]});
            rawBody = Unit({
                g_aimQ[0][0] * rawCam.x + g_aimQ[1][0] * rawCam.y + g_aimQ[2][0] * rawCam.z,
                g_aimQ[0][1] * rawCam.x + g_aimQ[1][1] * rawCam.y + g_aimQ[2][1] * rawCam.z,
                g_aimQ[0][2] * rawCam.x + g_aimQ[1][2] * rawCam.y + g_aimQ[2][2] * rawCam.z});
        }
        if (Length(rawBody) > .5 && std::isfinite(rawBody.x) && std::isfinite(rawBody.y) && std::isfinite(rawBody.z)) {
            const double dot = (std::max)(-1.0, (std::min)(1.0, Dot(Unit(aimHandoffPublished), rawBody)));
            Log("[AIM HANDOFF] frame%ld profile%d %s reticleToNewRawAimDeg=%.3f; new grip direction versus already-published reticle, diagnostic only",
                g_frame, g_weaponMechanicProfile, g_twoHand ? "grip on" : "grip off", std::acos(dot) * 57.2957795);
        }
    }
    // Both projection axes come from the same observed camera sample.

    // Zero-separation capture in the headset: both eyes render the left eye's
    // view and field of view, so the two images must come out identical.
    if (g_zeroEyeFrames > 0 && g_eyeViewsSet)
    {
        g_eyeViews[1] = g_eyeViews[0];
        g_eyeViewsBody[1] = g_eyeViewsBody[0];
    }
    const bool flat = g_flatViewEnabled && g_cameraLessFrames >= 3 && !g_frameCameraValid;
    if (flat != g_flatViewActive)
    {
        g_flatViewActive = flat;
        Log("[FLATVIEW] %s at frame %ld (frames without a 3D camera: %d)",
            flat ? "ON: no 3D camera, frame shown as a flat panel" : "OFF: 3D camera back, normal stereo",
            static_cast<long>(g_frame), g_cameraLessFrames);
    }

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

    UpdateOpticScene132();
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
        if (flat)
        {
            // No 3D camera to convert from: every vertex, perspective or flat,
            // lands on the UI panel with its shape unchanged (the same panel
            // the variant-2 UI uses). Clip space: x' = (1+sx) x + ox w.
            float sx = 0, sy = 0, ox = 0, oy = 0;
            if (g_eyeViewsSet)
            {
                const fc3stereo::EyeView& e = g_eyeViews[eye];
                const float side = eye == 0 ? 1.0f : -1.0f;
                const float width = e.tanRight - e.tanLeft, height = e.tanUp - e.tanDown;
                const float panel = std::tan(g_uiWidthDeg * 0.5f * 0.0174532925f);
                const float aspect = g_bbW ? static_cast<float>(g_bbH) / static_cast<float>(g_bbW) : 0.5625f;
                sx = panel / (width * 0.5f) - 1.0f;
                sy = panel * aspect / (height * 0.5f) - 1.0f;
                ox = -(e.tanRight + e.tanLeft) / width +
                     (2.0f / width) * side * static_cast<float>(std::fabs(cameraView.m[3])) / g_uiDepth;
                oy = -(e.tanUp + e.tanDown) / height;
            }
            float values[dxbc::kEyeRows * 4] = {};
            values[0] = sx; values[3] = ox; values[5] = sy; values[7] = oy;     // perspective rows (M - I)
            for (int i = 0; i < 16; ++i) { values[24 + i] = values[i]; values[40 + i] = values[i]; }  // arm rows
            values[16] = ox; values[17] = oy; values[20] = sx; values[21] = sy;  // flat vertices
            values[56] = ox; values[57] = oy; values[60] = sx; values[61] = sy;  // crosshair rows
            g_own = true;
            for (int variant = 0; variant < kEyeVariants; ++variant)
                g_oUpdateSubresource(g_ctx, g_eyeCb[variant][eye], 0, nullptr, values, 0, 0);
            g_own = false;
            worldMatrix.m[0] = 1.0 + sx; worldMatrix.m[3] = ox;
            worldMatrix.m[5] = 1.0 + sy; worldMatrix.m[7] = oy;
            worldMatrix.m[10] = worldMatrix.m[15] = 1.0;
        }
        else
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
                    const bool solved = SolveArm(g_armJoints[handSide], handMove, handSide, forearmMove, upperMove,false,g_armIk84&&!ReloadPoseActive()&&!PkmArmsNative());
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
            ScopeDisplayTurn72(placed,forearmPlaced,upperPlaced,cameraView,variant);
            if(eye==0){Mat4 inverse={};if(Invert(cameraView,inverse)){g_guideShaderPlacement[variant]=Multiply(inverse,placed);g_guideShaderPlacementFrame[variant]=g_frame;}}
            const Mat4 matrix = OpticSceneMatrix132(Multiply(projection, Multiply(placed, ProjectionInverse(game))),variant,eye);
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
            // Visual aiming presentation follows the published physical pose;
            // never feed this close-up transform back to reloads or muzzle58.
            ScopePresentation62(values,variant,eye);
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
                ObserveNativeUiPanel(eye,1+values[20],1+values[21],values[16],values[17]);
                values[56] = values[16]; values[57] = values[17];
                values[60] = values[20]; values[61] = values[21];
                // In motion gameplay the reticle follows the physical weapon
                // ray, independently of the head-driven visibility camera.
                if (g_eyeViewsSet&&g_inGameplay&&!g_flatViewActive&&!fc3pad::NativeMenuOpen()&&!fc3pad::WheelOwnsGrip())
                {
                    const fc3stereo::EyeView& e = g_eyeViews[eye];
                    g_redDotValid[eye]=false;
                    V3 target73={};
                    if (ReticleTarget73(eye,view,target73))
                    {
                        const double width = e.tanRight - e.tanLeft, height = e.tanUp - e.tanDown;
                        const double tx = target73.x / target73.z, ty = target73.y / target73.z;
                        values[56] = static_cast<float>((2.0 * tx - (e.tanRight + e.tanLeft)) / width);
                        values[57] = static_cast<float>((2.0 * ty - (e.tanUp + e.tanDown)) / height);
                        g_redDotNdc[eye][0]=values[56];g_redDotNdc[eye][1]=values[57];
                        g_redDotValid[eye]=std::isfinite(values[56])&&std::isfinite(values[57]);
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
            if(NativeGuideShadowScope51())g_eyeCb[variant][eye]->SetPrivateData(kCbCopyGuid,sizeof(values),values);
            g_own = false;
        }

        // [TILES] eye homography for flat lighting tiles: a game-clip corner
        // (cx, cy) seen at view depth d is (cx d, cy d, a d + b, d) in game clip;
        // the world eye matrix takes it to eye clip. Rows x, y, w of that, as
        // functions of (cx, cy, 1).
        if (g_tileCb[eye])
        {
            const double d = 30.0;
            const double a = g_worldProjection.a, b = g_worldProjection.b;
            float tileRows[12] = {};
            // Coverage margin in game NDC: the eye sits |t| from the game camera,
            // so a surface at 0.35 m appears up to |t|/0.35 radians away from
            // where the far-depth mapping puts it. The pixel test keeps it exact;
            // the margin only has to cover that parallax.
            {
                const double tx = cameraView.m[3], ty = cameraView.m[7], tz = cameraView.m[11];
                const double t = std::sqrt(tx * tx + ty * ty + tz * tz);
                const double angle = (std::fmin)(0.6, t / 0.35);
                tileRows[3] = static_cast<float>(0.02 + 1.6 * angle * std::fabs(g_worldProjection.xs));
                tileRows[7] = static_cast<float>(0.02 + 1.6 * angle * std::fabs(g_worldProjection.ys));
            }
            // The tile coverage margin is raster-space, so scene assistance
            // must enlarge it with the same eye crop as the geometry.
            const double sceneMargin132=OpticSceneMatrix132(HandIdentity(),0,eye).m[0];
            tileRows[3]*=float(sceneMargin132);tileRows[7]*=float(sceneMargin132);
            const int rows[3] = { 0, 1, 3 };
            for (int r = 0; r < 3; ++r)
            {
                const double* m = &worldMatrix.m[rows[r] * 4];
                tileRows[r * 4 + 0] = static_cast<float>(m[0] * d);
                tileRows[r * 4 + 1] = static_cast<float>(m[1] * d);
                tileRows[r * 4 + 2] = static_cast<float>(m[2] * (a * d + b) + m[3] * d);
            }
            g_own = true;
            g_oUpdateSubresource(g_ctx, g_tileCb[eye], 0, nullptr, tileRows, 0, 0);
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
        {
            float normalColumns[20] = {};      // row 4 = 0: tile test off (never read past the buffer)
            std::memcpy(normalColumns, columns, sizeof(columns));
            g_oUpdateSubresource(g_ctx, g_eyePsCb[eye], 0, nullptr, normalColumns, 0, 0);
        }
        if (g_eyePsCbTile[eye])
        {
            float tileColumns[20] = {};
            std::memcpy(tileColumns, columns, sizeof(columns));
            tileColumns[16] = 1.0f;
            g_oUpdateSubresource(g_ctx, g_eyePsCbTile[eye], 0, nullptr, tileColumns, 0, 0);
        }
        g_own = false;
        UpdateScopeProjection62(worldMatrix.m,eye);
    }
}

bool CreateDeviceObjects()
{
    probe66::ResetGpu();
    ClearScopeResources60();
    fc3visual292::Release();
    ClearPhysicalHull();
    ClearReloadShell();
    ClearMacheteVisual();
    fc3aim::ClearMuzzle58();g_muzzleRender58={};g_muzzleLaserFrame203={};
    g_shotHand=ShotHandControl{};g_reloadContact=ReloadHandContact{};g_reloadPose=ReloadPoseCycle{};
    AnimationClose();
    SafeRelease(g_handsClipBuffer287);g_wristCutUploaded287=-1;g_handsClipAvailable287=false;g_handsShaderDevice287=nullptr;
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
    ReleaseRepairToolResources();
    ReleaseRevolverLoader249();
    ReleaseThrowableResources145();ReleaseMuzzleFlashResources111();ReleaseBowArrowResources();
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
    desc.ByteWidth = 48;
    for (int eye = 0; eye < 2; ++eye)
    {
        SafeRelease(g_tileCb[eye]);
        if (FAILED(g_oCreateBuffer(g_dev, &desc, nullptr, &g_tileCb[eye])))
            return false;
    }
    desc.ByteWidth = 80;
    for (int eye = 0; eye < 2; ++eye)
    {
        SafeRelease(g_eyePsCbTile[eye]);
        if (FAILED(g_oCreateBuffer(g_dev, &desc, nullptr, &g_eyePsCbTile[eye])))
            return false;
    }
    desc.ByteWidth = 80;    // five rows: the tile test reads cb13[4]
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
        // [TILES] lighting tile vertex shader (same input and outputs as the game's)
        // The game's tile covers a rectangle of ITS screen. Each corner goes to
        // eye clip space (homogeneous, not divided). Corners far off the game
        // camera's axis can lie behind the eye (the widened game view is ~140
        // degrees), so the geometry shader clips each triangle at w = 0.01
        // before dividing, then writes the game's screen-uv relation.
        // Tile corners on the game screen's border are pushed out 4x: with the head
        // turned the eye sees past the game's view, and those pixels take the
        // nearest border tile's lights instead of staying black.
        // [TILES] Each game tile (two triangles) is drawn once, from the triangle
        // that has only one corner on the rectangle's right edge: its rectangle
        // grown by the parallax margin (border edges pushed out 4x), mapped to
        // eye clip at far depth, clipped at w = 0.01 and divided. The exact
        // rectangle goes to the pixel shader in TEXCOORD0.zw (10-bit fields);
        // the patched pixel shader keeps only pixels whose point projects into
        // it on the game's screen, so every pixel is lit once by its own tile.
        static const char tileSource[] = FC3_TILE_SHADER_SOURCE;
        ID3D10Blob* tileVs = nullptr;
        ID3D10Blob* tileErrors = nullptr;
        compile(tileSource, sizeof(tileSource) - 1, nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &tileVs, &tileErrors);
        SafeRelease(g_tileVs);
        SafeRelease(g_tileGs);
        if (tileVs)
        {
            g_own = true;
            g_oCreateVertexShader(g_dev, tileVs->GetBufferPointer(), tileVs->GetBufferSize(), nullptr, &g_tileVs);
            g_own = false;
            tileVs->Release();
        }
        ID3D10Blob* tileGs = nullptr;
        compile(tileSource, sizeof(tileSource) - 1, nullptr, nullptr, nullptr, "gs", "gs_4_0", 0, 0, &tileGs, nullptr);
        if (tileGs)
        {
            g_dev->CreateGeometryShader(tileGs->GetBufferPointer(), tileGs->GetBufferSize(), nullptr, &g_tileGs);
            tileGs->Release();
        }
        if (!g_tileGs) SafeRelease(g_tileVs);   // both or neither
        static const char tileSimpleSource[] = FC3_TILE_SHADER_SOURCE_SIMPLE;
        SafeRelease(g_tileVsSimple);
        SafeRelease(g_tileGsSimple);
        ID3D10Blob* simpleVs = nullptr;
        ID3D10Blob* simpleGs = nullptr;
        compile(tileSimpleSource, sizeof(tileSimpleSource) - 1, nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &simpleVs, nullptr);
        compile(tileSimpleSource, sizeof(tileSimpleSource) - 1, nullptr, nullptr, nullptr, "gs", "gs_4_0", 0, 0, &simpleGs, nullptr);
        if (simpleVs) { g_oCreateVertexShader(g_dev, simpleVs->GetBufferPointer(), simpleVs->GetBufferSize(), nullptr, &g_tileVsSimple); simpleVs->Release(); }
        if (simpleGs) { g_dev->CreateGeometryShader(simpleGs->GetBufferPointer(), simpleGs->GetBufferSize(), nullptr, &g_tileGsSimple); simpleGs->Release(); }
        if (!g_tileGsSimple) SafeRelease(g_tileVsSimple);
        if (tileErrors)
        {
            Log("[TILES] tile shader compile error: %s", static_cast<const char*>(tileErrors->GetBufferPointer()));
            tileErrors->Release();
        }
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
    Log("[TILES] lighting tile shader %s", g_tileVs ? "ready" : "UNAVAILABLE (lighting tiles stay as the game placed them)");
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
    g_sideBySideDrawn = true;   // AfterPresent puts the left eye back

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

// Grey BMP of |left - right| x4 (half resolution), for zero-separation pairs.
void WriteDifferenceBmp(const wchar_t* fileName, const std::vector<uint8_t>& left,
                        const std::vector<uint8_t>& right, UINT w, UINT h)
{
    const UINT stride = (w * 3 + 3) & ~3u;
    std::vector<uint8_t> pixels(static_cast<size_t>(stride) * h);
    for (UINT y = 0; y < h; ++y)
    {
        uint8_t* out = pixels.data() + static_cast<size_t>(stride) * (h - 1 - y);
        for (UINT x = 0; x < w; ++x)
        {
            const size_t i = static_cast<size_t>(y) * w + x;
            const int d = std::abs(static_cast<int>(left[i]) - static_cast<int>(right[i])) * 4;
            out[3 * x] = out[3 * x + 1] = out[3 * x + 2] = static_cast<uint8_t>(d > 255 ? 255 : d);
        }
    }
    wchar_t path[MAX_PATH] = {};
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%s%s", fc3::DataFolder(), fileName);
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    BITMAPFILEHEADER header = {};
    BITMAPINFOHEADER info = {};
    header.bfType = 0x4D42;
    header.bfOffBits = sizeof(header) + sizeof(info);
    header.bfSize = header.bfOffBits + static_cast<DWORD>(pixels.size());
    info.biSize = sizeof(info);
    info.biWidth = static_cast<LONG>(w);
    info.biHeight = static_cast<LONG>(h);
    info.biPlanes = 1;
    info.biBitCount = 24;
    DWORD written = 0;
    WriteFile(file, &header, sizeof(header), &written, nullptr);
    WriteFile(file, &info, sizeof(info), &written, nullptr);
    WriteFile(file, pixels.data(), static_cast<DWORD>(pixels.size()), &written, nullptr);
    CloseHandle(file);
}

void DumpEyes(ID3D11Texture2D* leftEye, ID3D11Texture2D* rightEye, bool zeroPair = false)
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
        if (zeroPair)
        {
            size_t differing = 0, total = 0;
            for (UINT y = y0; y < y1; ++y)
                for (UINT x = 0; x < w; ++x, ++total)
                {
                    const size_t i = static_cast<size_t>(y) * w + x;
                    if (std::abs(static_cast<int>(left[i]) - static_cast<int>(right[i])) > 8) ++differing;
                }
            Log("[ZERO] %-6s band: %.3f%% of pixels differ by more than 8/255",
                names[band], total ? 100.0 * static_cast<double>(differing) / static_cast<double>(total) : 0.0);
        }
    }
    if (zeroPair)
    {
        wchar_t diffName[48] = {};
        _snwprintf_s(diffName, 48, _TRUNCATE, L"fc3_eye_diff_%d.bmp", dumpIndex);
        WriteDifferenceBmp(diffName, left, right, w, h);
        Log("[ZERO] difference image written (pair #%d): bright = the eyes differ there", dumpIndex);
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

#include "grip_context.inl"
#include "body_interaction285.inl"
#include "throwable_pouch361.inl"
#include "insert_menu.inl"
#include "scope60.inl"
#include "calibration136_runtime.inl"
#include "red_dot.inl"
#include "laser_depth205.inl"
#include "native_ui_pointer.inl"
#include "render_transition_probe.inl"
#include "diagnostic66.inl"

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
bool RunOverlay291(bool (*render)(void*),void* data){const bool previous=g_own;g_own=true;const bool ok=render(data);g_own=previous;return ok;}
void WriteRenderProbe(const char* text){RenderProbeWrite(text);}

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
    InterlockedExchange(&g_minHookReady, 1); // the throwable capture installs its observers only after this
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
    HOOK(cv, 51, HookClearUavUint61, g_oClearUavUint61);
    HOOK(cv, 52, HookClearUavFloat61, g_oClearUavFloat61);
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

bool CopyTexture(ID3D11Texture2D* destination, ID3D11Texture2D* source)
{
    if (!g_ctx || !g_oCopyResource || !destination || !source) return false;
    g_own = true;
    g_oCopyResource(g_ctx, destination, source);
    g_own = false;
    return true;
}

float MainXScale() { return g_mainXScale; }

void SetHandPoses(const float* right, const float* left)
{
    g_physicalAt352=GetTickCount64();
    fc3pad::SetVehicleHands(right,left);
    CalibrationTrack138(right,left); // read-only body wizard samples; tracking/weapon inputs stay unchanged
    if (fc3pad::TakeCalibration() && right && left)
    {
        ++g_armPoseEpoch; // recalibration also refreshes neutral mesh poses
        std::memcpy(g_handRef[0], right, sizeof(g_handRef[0]));
        std::memcpy(g_handRef[1], left, sizeof(g_handRef[1]));
        // 10-08: the wrist offsets are no longer measured here. They were taken from the GAME's
        // animated wrist at that instant (with a two-handed gun out, the left one on the forend),
        // which gave a different left offset every session. The fixed offsets stay.
        Log("[ARMS] calibrated: right hand %.2f %.2f %.2f, left hand %.2f %.2f %.2f (game axes, m); wrist offsets fixed %.2f %.2f %.2f",
            right[3], right[7], right[11], left[3], left[7], left[11], g_wristOffset[0][0], g_wristOffset[0][1], g_wristOffset[0][2]);
    }
    const float* physical303[2]={right,left};for(int hand=0;hand<2;++hand){g_physicalValid303[hand]=physical303[hand]!=nullptr;if(physical303[hand])memcpy(g_physicalBody303[hand],physical303[hand],sizeof(g_physicalBody303[hand]));}
    const auto owner303=fc3hands303::Read();if(fc3hands303::PhysicalHand(owner303,0)==1)std::swap(right,left);
    float rolePose303[2][16];if(right){memcpy(rolePose303[0],right,64);fc3hands303::AdjustRolePose(0,rolePose303[0]);Left1911Role306(rolePose303[0]);SidearmRole315(rolePose303[0]);AkRole316(rolePose303[0]);ShotgunRole318(rolePose303[0]);OtherRole319(rolePose303[0]);right=rolePose303[0];}if(left){memcpy(rolePose303[1],left,64);fc3hands303::AdjustRolePose(1,rolePose303[1]);left=rolePose303[1];}
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
bool InWorldView() { return g_inGameplay||g_worldViewActive; }
bool HeadTrackingEnabled() { return g_headTracking; }
void RequestOptionsToggle299(){InterlockedExchange(&g_optionsToggle299,1);fc3pad::GuardMenuExit();}
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

void AfterPresent(IDXGISwapChain* swapChain)
{
    if (!swapChain || !g_ctx || !g_dev) return;
    ID3D11Texture2D* backbuffer = nullptr;
    if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backbuffer))) || !backbuffer)
        return;
    // [LOADCLEAR] Loading screens and menus never clear the buffer, and the
    // flat panel no longer covers the whole image: older frames stayed visible
    // around it (the "bleed"). With no 3D camera, start each frame black.
    if (g_flatViewActive && g_enabled)
    {
        const FLOAT black[4] = { 0, 0, 0, 1 };
        ID3D11Resource* images[2] = { backbuffer, GetTwin<ID3D11Resource>(backbuffer) };
        g_own = true;
        for (int i = 0; i < 2; ++i)
        {
            if (!images[i]) continue;
            ID3D11RenderTargetView* view = nullptr;
            if (SUCCEEDED(g_dev->CreateRenderTargetView(images[i], nullptr, &view)) && view)
            {
                g_oClearRtv(g_ctx, view, black);
                view->Release();
            }
        }
        g_own = false;
        SafeRelease(images[1]);
        static bool logged = false;
        if (!logged) { logged = true; Log("[LOADCLEAR] no 3D camera: both eye images cleared to black after Present"); }
        g_sideBySideDrawn = false;
        backbuffer->Release();
        return;
    }
    if (!g_sideBySideDrawn || !g_leftCopy) { backbuffer->Release(); return; }
    g_sideBySideDrawn = false;
    D3D11_TEXTURE2D_DESC desc = {};
    backbuffer->GetDesc(&desc);
    if (desc.Width == g_leftCopyW && desc.Height == g_leftCopyH && desc.Format == g_leftCopyFormat)
    {
        g_own = true;
        g_oCopyResource(g_ctx, backbuffer, g_leftCopy);
        g_own = false;
        static bool logged = false;
        if (!logged) { logged = true; Log("[SBSRESTORE] left eye image put back into the game's buffer after Present (stops the side-by-side feedback)"); }
    }
    backbuffer->Release();
}

void PublishWorldPresence283(){
    g_worldViewActive=g_frameCameraValid&&!g_flatViewActive;
    g_inGameplay=g_worldViewActive;
    fc3pad::SetGameplayControls(g_enabled&&g_inGameplay);
}
void ResumeNativeWeaponTimers(ULONGLONG started,ULONGLONG paused)
{
 // A paused native menu must not expire a held magazine, insertion, bolt or shell.
 auto shift=[paused](ULONGLONG& t){if(t)t+=paused;};
 auto clock=[started,paused](ULONGLONG& t){if(t&&t<=started)t+=paused;};
 auto resume=[&](auto& state){shift(state.deadline);shift(state.pulseAt);shift(state.probeAt);state.xWas=false;state.leftGripWas=false;};
 resume(g_flare);resume(g_rpg);resume(g_gm94);resume(g_flamer);shift(g_gm94.shot.until);
 shift(g_flare.shot.until);shift(g_rpg.shot.until);
 clock(g_flare.fallAt);clock(g_gm94.fallAt);clock(g_flamer.fallAt);clock(g_rpg.clipAt);clock(g_rpg.inputPauseAt219);clock(g_gm94Lock.shotAt);
 g_flare.tiltSince=g_flare.rightPrevAt=g_gm94.tiltSince=g_gm94.spentSampleAt=0;
 shift(g_reload.deadline);shift(g_manualReload.deadline);clock(g_manualReload.tick);
 shift(g_lever.deadline);shift(g_lever.probeDeadline);
 if(g_lever.clip>=0&&g_lever.pending&&Lever1887Model())fc3pad::RequestReload();
 clock(g_lever.holdAt);clock(g_lever.releaseAt);clock(g_lever.returnAt);clock(g_lever.clipAt);clock(g_lever.clipPauseAt223);clock(g_lever.clipPulseAt223);
 for(auto& t:g_lever.shakeAt)clock(t);
 clock(g_r700.clipAt);clock(g_r700.clipPauseAt223);clock(g_r700.nativeAt);clock(g_r700.nativeEndAt);clock(g_r700.holdAt);clock(g_r700.shotAt);clock(g_r700.releaseAt);
 shift(g_revolver.deadline);clock(g_revolver.lastTick);clock(g_revolver.nativeAt);clock(g_revolver.nativeEndAt);
 clock(g_revolver.dropAt);clock(g_revolver.clipAt);clock(g_revolver.clipPauseAt223);clock(g_revolver.probeAt);clock(g_revolver.pulseAt);
 g_revolver.tiltSince=g_revolver.rightPrevAt=0;
 shift(g_spas.deadline);clock(g_spas.pulseAt);clock(g_spas.lastTick);clock(g_spas.nativeAt);clock(g_spas.nativeEndAt);clock(g_spas.clipAt);clock(g_spas.clipPauseAt223);
 shift(g_bow.deadline);clock(g_bow.pulseAt);clock(g_bow.fallAt);clock(g_bow.fireAt);g_bow.handAt=0;
 for(auto& p:g_magazineStates){
  CancelMagazinePull286(p); // paused menus/actions cannot complete an old hand-pull gesture
  shift(p.deadline);shift(p.poseHoldUntil);shift(p.nextAmmoAt);
  clock(p.rackReturnAt);clock(p.dropAt);clock(p.insertAt);clock(p.closeAt);
  clock(p.looseAt);clock(p.looseTick);clock(p.inputPauseAt);clock(p.requestAt);
  p.sampleAt=0;p.sampleFrame=-100000;p.contactFrames=0;
 }
}
void OnPresent(IDXGISwapChain* swapChain)
{
    const auto frameTick359=GetTickCount64();static ULONGLONG previousTick359=0;static unsigned stalls359=0;
    if(previousTick359&&frameTick359>=previousTick359&&frameTick359-previousTick359>75&&stalls359++<48){const auto owner359=fc3hands303::Read();const auto inventory359=fc3select290::Read();Log("[FRAME359] interval=%llums frame=%ld profile=%ld owner=%d entity=%08X generation=%u active=%d engaged=%d presentation=%d context=%d inventory=%d nativeEquipped=%08X gunPose=%d reload=%d/%d shell=%d preparing=%d",frameTick359-previousTick359,g_frame,g_weaponMechanicProfile,owner359.hand,owner359.entity,owner359.generation,owner359.active,owner359.engaged,owner359.presentation314,owner359.ownerContext326,inventory359.valid,inventory359.equipped,g_frameGunValid,g_manualReload.pending,g_manualReload.cycle,g_reload.held,fc3visual292::Preparing330());}
    previousTick359=frameTick359;

    if (!swapChain || !g_dev || !g_ctx)
        return;
#ifndef FC3_AMMO_TEST
    fc3throwhost::Tick(InterlockedCompareExchange(&g_minHookReady, 0, 0) != 0); // throwable launch capture: install once, flush queued records (never in a game callback)
#endif

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
    TickInsertMenu(chain.OutputWindow);
    AcquireSRWLockExclusive(&g_nightProbeLock);
    fc3night::FlushOnPresent(g_ctx,fc3::DataFolder());NextNightCaptureWindow(g_frame);
    ReleaseSRWLockExclusive(&g_nightProbeLock);
    {const bool own=g_own;g_own=true;fc3healthprobe::Poll(g_ctx,fc3::DataFolder(),g_frame,[](const char* text){Log("%s",text);});g_own=own;}
    TickNativeUiPointer(chain.OutputWindow);
    if (GetForegroundWindow() == chain.OutputWindow && !g_pistolMenu.open)
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
        if (KeyPressed(VK_NUMPAD1)) { g_dumpRequested = true; g_vehicleProbeMarker90 = true; BeginEnvironment252(); }
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
            g_mainBinCount = bestCount;
            g_mainXScale = bin.xScale;
            g_worldProjection = bin.projection;
            std::memcpy(g_mainRight, bin.right, sizeof(g_mainRight));
        }
    }
    g_cameraLessFrames = bestCount > 0 ? 0 : g_cameraLessFrames + 1;
    const LONG frame = g_frame;
    const bool report = frame == 120 || (frame > 0 && frame % 600 == 0);
    if (report)
    {
        char bins[160] = {};
        int used = 0;
        for (const CameraBin& bin : g_cameraBins)
            if (bin.count && used < 130)
                used += _snprintf_s(bins + used, sizeof(bins) - used, _TRUNCATE, " %.3fx%ld", bin.xScale, bin.count);
        char rejected[200] = {}; int ru = 0;
        for (const RejectedCamera& r : g_rejectedCameras)
            if (r.count && ru < 170) ru += _snprintf_s(rejected + ru, sizeof(rejected) - ru, _TRUNCATE, " %.3f/%.3f(aspect %.3f)x%ld", r.xScale, r.yScale, r.yScale / r.xScale, r.count);
        char sizes[120] = {}; int su = 0;
        for (int i = 0; i < 8; ++i) if (g_cameraBufferSizeCounts[i] && su < 100) su += _snprintf_s(sizes + su, sizeof(sizes) - su, _TRUNCATE, " %uBx%ld", g_cameraBufferSizes[i], g_cameraBufferSizeCounts[i]);
        Log("[CAM] frame %ld main xScale=%.4f, this frame's cameras:%s | rejected perspective writes:%s | camera buffer sizes:%s (expected aspect %.3f)",
            frame, g_mainXScale, bins[0] ? bins : " none", rejected[0] ? rejected : " none", sizes[0] ? sizes : " none", g_bbH ? static_cast<float>(g_bbW) / static_cast<float>(g_bbH) : 0.f);
    }
    TickVehicleAutoCapture91(fc3xr::Running());
    if(g_vehicleAutoCaptureReason91&&g_gridAutoCount258<4){g_gridAutoFrame258=g_frame+1;++g_gridAutoCount258;}
    TickRenderTransitionProbe(chain.OutputWindow);
    std::memset(g_cameraBins, 0, sizeof(g_cameraBins));
    std::memset(g_rejectedCameras, 0, sizeof(g_rejectedCameras));
    std::memset(g_cameraBufferSizeCounts, 0, sizeof(g_cameraBufferSizeCounts));

    ID3D11Texture2D* rightEye = g_enabled ? static_cast<ID3D11Texture2D*>(GetTwin<ID3D11Resource>(backbuffer)) : nullptr;
    TickGameplayAssist();
    TickCalibration136();
    probe66::Before(backbuffer,rightEye);
    RenderScopes60(backbuffer,rightEye);
    RenderRedDot(backbuffer,rightEye);
    RenderLaserDepth205(backbuffer,rightEye);
    RenderNativeUiCursor(backbuffer,rightEye,fc3pad::NativeMenuConfirmed());
    if(!MotionInteractionEnabled()||!g_handRightValid||g_pistolMenu.open||fc3pad::NativeMenuOpen()||fc3pad::WeaponSelectionRequested())fc3aim::ClearMuzzle58();
    RenderInsertMenu(backbuffer,false);
    g_nativeGuideVertices48.ready=false;g_nativeGuidePacketCount50=0;
    // Final draw anchors belong to the image being submitted, before new tracking/aim.
    fc3xr::SetGuideGroupVisibility(g_showWeaponReloadGuides&&PhysicalReloadGuidesEnabled(),g_grabGuides||g_pouchGuide361||GrenadePinGuide154()||MolotovGuide159(),g_pistolMenu.open);
    W7Observe();
    if(g_pistolMenu.open&&W7ProfileIndex(g_weaponMechanicProfile)>=0)Weapons7MenuMarkers();
    else if(BowModel()||(g_pistolMenu.open&&g_weaponMechanicProfile==10))BowGuideMarkers();
    else if(PistolModel()||(g_pistolMenu.open&&MagazineProfile(g_weaponMechanicProfile)))PistolGuideMarkers();
    else if(FlareModel())FlareGuideMarkers();
    else if(RpgModel())RpgGuideMarkers();
    else if(Gm94Model())Gm94GuideMarkers();
    else if(FlamerModel())FlamerGuideMarkers();
    else if(RevolverModel()||(g_pistolMenu.open&&g_weaponMechanicProfile==42))RevolverGuideMarkers();
    else if(SpasModel()||(g_pistolMenu.open&&g_weaponMechanicProfile==34))SpasGuideMarkers();
    else if(g_weaponMechanicProfile==0)Lever1887GuideMarkers();
    else if((g_weaponMechanicProfile>=3&&g_weaponMechanicProfile<=6)||g_weaponMechanicProfile==38)ManualMarker();
    else fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);
    RenderMolotov158(backbuffer,rightEye);
    RenderNative1911Guide48(backbuffer,rightEye);
    RenderCalibration136(backbuffer,rightEye);
    RevolverModifierNote321();
    RenderWeaponGuides50(backbuffer,rightEye);RenderBowGuides51(backbuffer,rightEye);RenderBodyGuides51(backbuffer,rightEye);RenderMacheteColliderGuide118(backbuffer,rightEye);
    PublishHandUi();
    probe66::After(backbuffer,rightEye); // exact pre-XR source pixels, before next tracking
    // A normal holster/draw transition can temporarily have no visible gun.
    // Keep this selector's lease through that transition until identity readback.
    bool selectorSeated290=false;int selectorSeat290=-1;
    const bool selectorFoot290=fc3aim::VehicleState(selectorSeated290,selectorSeat290)&&!selectorSeated290&&!fc3aim::NativeActionOwnsArms128();
    fc3select290::Watch330(g_enabled&&g_motionGun&&g_inGameplay&&!g_flatViewActive&&selectorFoot290);
    const auto visualOwner326=fc3hands303::Read();
    const auto inventory330=fc3select290::Read();
    // Only bootstrap the absent native weapon / hidden preparation case.
    // Ordinary held weapons still require the established motion lease.
    const bool emptyLoadout330=fc3hands303::Configuration().enabled&&
        (fc3visual292::Preparing330()||(inventory330.valid&&!inventory330.equipped));
    const bool visualContext326=g_enabled&&g_motionGun&&g_inGameplay&&g_frameCameraValid&&!g_flatViewActive&&selectorFoot290&&!fc3pad::NativeMenuOpen()&&!MacheteHeld()&&!ThrowableOwns144()&&fc3hands303::RemappedOwner335(visualOwner326,g_weaponMechanicProfile);
    const bool heldChange351=!BodyBusy285(false)&&!(RevolverModel()&&RevolverReloadLease321())&&!(fc3hands303::Shotgun318(g_weaponMechanicProfile)&&ShotgunBusy318())&&!(fc3hands303::Other319(g_weaponMechanicProfile)&&OtherBusy319())&&!(BowModel()&&(g_bow.held||g_bow.nocked||g_bow.shooting||g_bow.pending||g_bow.releasePending));
    {static unsigned entity351=~0u,reports351=0;static bool change351=true;static int profile351=-2;
     if((entity351!=inventory330.equipped||change351!=heldChange351||profile351!=g_weaponMechanicProfile)&&reports351++<128){entity351=inventory330.equipped;change351=heldChange351;profile351=g_weaponMechanicProfile;
      Log("[HOLSTER351 CONTEXT] profile=%ld change=%d logicalEmpty=%d native=%08X reload=%d/%d alignment=%d mag=%d/%d/%d rpg=%d/%d bow=%d/%d/%d/%d/%d",g_weaponMechanicProfile,heldChange351,visualOwner326.empty,inventory330.equipped,g_reload.held,g_reload.pending,g_reloadAlignment,g_pistolReload.held,g_pistolReload.pending,g_pistolReload.rackHeld,g_rpg.carrying,g_rpg.clip,g_bow.held,g_bow.nocked,g_bow.shooting,g_bow.pending,g_bow.releasePending);
     }}
    fc3xr::SetHandsContext303(g_enabled&&g_motionGun&&g_inGameplay&&g_frameCameraValid&&!g_flatViewActive&&selectorFoot290&&(MotionInteractionEnabled()||fc3xr::SelectorOpen291()||emptyLoadout330)&&!MacheteHeld()&&!ThrowableOwns144(),heldChange351,g_armPoseEpoch,BowModel()?1:0,visualContext326);
    fc3xr::SetSelectorAllowed290(g_enabled&&g_motionGun&&g_inGameplay&&g_frameCameraValid&&!g_flatViewActive&&!g_pistolMenu.open&&selectorFoot290&&((MotionInteractionEnabled()&&!BodyBusy285())||fc3xr::SelectorOpen291())&&!MacheteHeld());
    Publish1911Transfer307();
    fc3visual292::Finish();
    if(fc3pad::NativeMenuOpen()&&!fc3xr::SelectorOpen291())fc3visual292::Release();
    const bool headset = fc3xr::Frame(g_dev, backbuffer, rightEye,g_frame,g_vehicleAutoCaptureReason91!=0);
    static int previousMotion = -1;
    const bool motionAllowed = MotionInteractionEnabled();
    if (previousMotion != static_cast<int>(motionAllowed))
    {
        Log("[MOTION AREA] %s: native weapon draws %ld; movement/buttons stay active",
            motionAllowed ? "weapon interaction active" : "arm/weapon overrides paused",
            g_visibleWeaponDrawsLastFrame);
        previousMotion = static_cast<int>(motionAllowed);
    }
    // Validate this completed native draw BEFORE queuing another camera write
    // or deriving next-frame eye compensation from its ownership.
    fc3aim::ProbeNativeView283(g_frameCamera.right,g_frameCamera.up,g_frameCamera.forward,g_frameCameraValid,g_inGameplay,g_visibleWeaponDrawsThisFrame>0);
    AimFrame(headset);
    bool seated128=false;int seat128=-1;
    if(fc3aim::NativeActionOwnsArms128()||(fc3aim::VehicleState(seated128,seat128)&&seated128)){
        StowMachete("native action/vehicle owns hands");fc3pad::SetMacheteContext(false);
    }
    // Freeze physical state while native UI owns input; preserve reload poses and timers.
    static ULONGLONG nativePauseAt=0;
    TickThrowable144(headset);
    TickMolotovProbe166();
    TickBodyInteractions285(headset);
    const auto held303=fc3hands303::Read();
    const bool physicalPaused=(held303.enabled&&held303.engaged&&held303.empty)||fc3pad::SelectorActive290()||ThrowableOwns144()||fc3aim::NativeActionOwnsArms128()||fc3pad::BodyGesture285()||fc3pad::WheelOwnsGrip()||fc3pad::NativeMenuOpen()||(fc3pad::GameplayInputBlocked()&&!g_pistolMenu.open);
    if(physicalPaused){
      for(auto& magazine:g_magazineStates)CancelMagazinePull286(magazine);
      if(g_pistolMenu.open){
        // Insert owns its own pause interval. Do not count it a second time.
        if(nativePauseAt){ResumeNativeWeaponTimers(nativePauseAt,g_insertPauseAt>nativePauseAt?g_insertPauseAt-nativePauseAt:0);nativePauseAt=0;}
        g_w7PausedAt=g_insertPauseAt;
      }else if(!nativePauseAt){nativePauseAt=GetTickCount64();g_w7PausedAt=nativePauseAt;}
    }
    else {
      if(nativePauseAt){
        const auto paused=GetTickCount64()-nativePauseAt;
        if(!g_pistolMenu.open)g_w7PausedAt=0;
        ResumeNativeWeaponTimers(nativePauseAt,paused);nativePauseAt=0;
        if(g_pistolReload.pending&&!g_pistolReload.entryAccepted)fc3pad::RequestReload();
      }
    TickMacheteNativeProbe();
    if(!g_pistolMenu.open)TickMachete(headset && g_inGameplay && MotionInteractionEnabled());
    TickMacheteRoutingProbe();
    TickMacheteQuickProbe();
    TickMacheteBladeContact();
    fc3xr::SetWeaponNotesContext303(true);
    if(!g_pistolMenu.open&&!MacheteHeld())TickPhysicalReload(headset && g_inGameplay && MotionInteractionEnabled());
    TickPistolReload(headset && g_inGameplay && !MacheteHeld());
    TickLever1887(headset && g_inGameplay && !MacheteHeld());
    TickRevolver(headset && g_inGameplay && !MacheteHeld());
    TickSpas(headset && g_inGameplay && !MacheteHeld()); // after the revolver: X ownership
    TickFlare(headset && g_inGameplay && !MacheteHeld()); // after the SPAS: X ownership
    TickRpg(headset && g_inGameplay && !MacheteHeld());
    TickGm94(headset && g_inGameplay && !MacheteHeld());
    TickFlamer(headset && g_inGameplay && !MacheteHeld());
    TickR700(headset && g_inGameplay && !MacheteHeld());
    TickBow(headset && g_inGameplay && !MacheteHeld());
    fc3xr::SetWeaponNotesContext303(false);
    } // native menus cannot advance physical weapon actions
    TickAutoAmmoCapture(headset && g_inGameplay && MotionInteractionEnabled());
    TickAmmoOwnerProbe(headset && g_inGameplay);
    TickAnimationProbe(headset && g_inGameplay);
    fc3xr::SetWeaponNotesContext303(true);UpdateAmmoTestNote();fc3xr::SetWeaponNotesContext303(false);
    MacheteNote();ThrowableNote144();
    TickPhysicalHull(headset && g_inGameplay);
    TickEjectionProbe(headset && g_inGameplay);
    TickActionProbe(headset && g_inGameplay);
    UpdateEyeBuffers();     // for the frame the game renders next
    UpdateGripContext(); // classify the next raw squeeze before any physical mechanic consumes it
    if (rightEye)
    {
        if(g_vehicleAutoCaptureReason91){
            const int reason=g_vehicleAutoCaptureReason91;g_vehicleAutoCaptureReason91=0;
            wchar_t name[96]={};std::vector<uint8_t> luma;UINT w=0,h=0;
            _snwprintf_s(name,96,_TRUNCATE,L"fc3_camera127_%lu_s%02u_left.bmp",GetCurrentProcessId(),g_cameraCaptureSlot127);
            const bool leftOk=ReadEye(backbuffer,name,luma,w,h);
            _snwprintf_s(name,96,_TRUNCATE,L"fc3_camera127_%lu_s%02u_right.bmp",GetCurrentProcessId(),g_cameraCaptureSlot127);
            const bool rightOk=ReadEye(rightEye,name,luma,w,h);
            Log("[CAMERA127] frame %ld final-eye pair %s reason %d PID%lu slot%u leftSource=%p rightSource=%p",g_frame,leftOk&&rightOk?"saved":"readback failed",reason,GetCurrentProcessId(),g_cameraCaptureSlot127,backbuffer,rightEye);
        }
        // Match final pixels to the exact frame captured inside the shadow draw.
        if (g_census != INVALID_HANDLE_VALUE && g_shadowCaptureFrame == g_frame)
        {
            wchar_t name[100]; std::vector<uint8_t> luma; UINT w=0,h=0;
            _snwprintf_s(name,100,_TRUNCATE,L"fc3_shadow_f%ld_final_left.bmp",g_frame);
            ReadEye(backbuffer,name,luma,w,h);
            _snwprintf_s(name,100,_TRUNCATE,L"fc3_shadow_f%ld_final_right.bmp",g_frame);
            ReadEye(rightEye,name,luma,w,h);
            Log("[SHADOWCAP] final eyes matched to shadow frame %ld",g_frame);
        }
        if(g_environment252.frame==g_frame){
            wchar_t name[160];std::vector<uint8_t> luma;UINT w=0,h=0;
            _snwprintf_s(name,160,_TRUNCATE,L"fc3_env255_p%u_t%llu_w%u_s%u_f%ld_final_left.bmp",GetCurrentProcessId(),g_environment252.session,g_environment252.burst,g_environment252.samples,g_frame);ReadEye(backbuffer,name,luma,w,h);
            _snwprintf_s(name,160,_TRUNCATE,L"fc3_env255_p%u_t%llu_w%u_s%u_f%ld_final_right.bmp",GetCurrentProcessId(),g_environment252.session,g_environment252.burst,g_environment252.samples,g_frame);ReadEye(rightEye,name,luma,w,h);
            EnvironmentLog255("[ENV255] frame%ld final scene eyes saved; draw families native%u left%u right%u skipped%u/%u/%u bytes%u",g_frame,g_environment252.count[0],g_environment252.count[1],g_environment252.count[2],g_environment252.skipped[0],g_environment252.skipped[1],g_environment252.skipped[2],static_cast<UINT>(g_environment252.bytes));
            if(g_environment252.metadata!=INVALID_HANDLE_VALUE)FlushFileBuffers(g_environment252.metadata);
        }
        if (g_waterCaptureFrame == g_frame)
        {
            wchar_t name[100]; std::vector<uint8_t> luma; UINT w=0,h=0;
            _snwprintf_s(name,100,_TRUNCATE,L"fc3_water_f%ld_final_left.bmp",g_frame);
            ReadEye(backbuffer,name,luma,w,h);
            _snwprintf_s(name,100,_TRUNCATE,L"fc3_water_f%ld_final_right.bmp",g_frame);
            ReadEye(rightEye,name,luma,w,h);
            Log("[WATERCAP14] final eyes saved frame %ld",g_frame);
        }
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
            DumpEyes(backbuffer, rightEye, true);
            g_separation = zeroTestSeparation;
        }
        // renderfix1 automatic captures (no keys): the loading screen once, and
        // in the headset one gameplay frame (census + shader bytecode), then a
        // zero-separation pair whose difference image shows any pass that
        // differs between the eyes for reasons other than parallax.
        static bool loadingCaptured = false;
        static int headsetCapture = 0;
        // the 3D world with its own camera (the main menu also draws hundreds of doubled draws)
        static LONG worldFrames = 0;
        worldFrames = (g_cameraLessFrames == 0 && g_gameplayFrames > 0) ? worldFrames + 1 : 0;
        if (!loadingCaptured && g_cameraLessFrames == 300)
        {
            loadingCaptured = true;
            Log("[CAPTURE] loading screen (no 3D camera for 300 frames, flat view %s): eye pair + census",
                g_flatViewActive ? "ON" : "OFF");
            g_dumpRequested = true;
        }
        // 366: off by default. This one-shot capture stalled the frame 390 ms while turning
        // the wheel (365 log) and the vehicle sample went stale. [Capture] HeadsetGameplayAuto366=1 re-enables it.
        if (g_autoHeadsetCapture366 && headset && g_eyeViewsSet && headsetCapture == 0 && worldFrames == 900)
        {
            headsetCapture = 1;
            Log("[CAPTURE] headset gameplay: eye pair + census + shader bytecode");
            g_dumpRequested = true;
        }
        else if (headset && headsetCapture == 1 && worldFrames == 960)
        {
            headsetCapture = 2;
            g_zeroEyeFrames = 12;
            Log("[ZERO] headset: both eyes use the left eye's view for 12 frames; the pair must match exactly");
        }
        else if (g_zeroEyeFrames > 0 && --g_zeroEyeFrames == 0)
        {
            headsetCapture = 3;
            DumpEyes(backbuffer, rightEye, true);
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
        Log("[TILES] lighting tiles re-placed for the eye: %ld draws since the last report; pixel shaders with the per-pixel tile test: %ld", InterlockedExchange(&g_tileDraws, 0), g_psTileTest);
        Log("[STAT] duplicate targets created so far %ld (%.0f MB, includes freed ones), duplicate views %ld, vertex shaders patched %ld / left alone %ld, "
            "pixel shaders patched %ld, camera writes %ld, separation %.4f",
            g_stats.twins, g_stats.twinBytes / 1048576.0, g_stats.twinViews,
            g_stats.vsPatched, g_stats.vsUnpatched, g_stats.psPatched, g_stats.cameraWrites, g_separation);
        previous = g_stats;
        previousFrame = frame;
    }

    if (g_frame % 1800 == 0 && g_frame > 0)
        Log("[ARMS] arm draws with rewritten bones %ld, weapon draws: skinned %ld / solid %ld, arm pose unreadable %ld times; sleeve draws %ld (arm bones stale %ld, no map %ld)",
            g_armPaletteDraws, g_weaponPaletteDraws, g_weaponWorldDraws, g_armIkRejected, g_sleeveDraws, g_sleeveStale, g_sleeveMissing);

    ScheduleEnvironment252();
    if(g_gridFrame258==g_frame&&g_gridLog258!=INVALID_HANDLE_VALUE)FlushFileBuffers(g_gridLog258);
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
    // The FOV module only ever sees the WORLD camera: when the world camera is missing (LT zoom-out, weapon camera only, 2026-10-08)
    // the weapon camera as "main" made it widen on top of its own value (factor 4.6, saved by the game into the profile).
    fc3fov::SetHiddenWeapon(g_visibleWeaponDrawsThisFrame==0); // weapon-free views the scale factor cannot cover use the engine's gfx_FOVOverride
    fc3fov::Update(headset && g_headTracking, g_frameCameraValid && g_mainBinCount >= 100, g_worldProjection.xs, g_worldProjection.ys);
    // HandleDraw already distinguishes the actual world view from weapons.
    // Looking up a ladder changes scene complexity, not the game mode.
    PublishWorldPresence283();
    g_mainBinCount = 0;
    TrackShotHandMotion111(); // this frame's world camera, before it is invalidated for the next frame
    g_frameCameraValid = false;
    g_frameCameraTries = 0;
    g_visibleWeaponDrawsLastFrame = g_visibleWeaponDrawsThisFrame;
    g_visibleWeaponDrawsThisFrame = 0;
    g_weaponDrawsLastFrame = g_weaponDrawsThisFrame;
    g_weaponDrawsThisFrame = 0;
    g_lastStereoDraws = g_stats.stereoDraws;
    // [TILES] self-check: read the other parity's tile queries (last frame's)
    {
        const int parity = (g_frame + 1) & 1;   // the frame before this one
        UINT64 kept = 0; bool ready = g_tileQueryUsed[parity] > 0;
        for (int i = 0; i < g_tileQueryUsed[parity] && ready; ++i)
        {
            UINT64 n = 0;
            if (g_ctx->GetData(g_tileQuery[parity][i], &n, sizeof(n), 0) != S_OK) { ready = false; break; }
            kept += n;
        }
        static UINT64 keptWith = 0, keptWithout = 0;
        if (!ready && g_tileQueryUsed[parity] > 0 && g_tileQueryMeasure[parity])
        {
            static int notReady = 0;
            if (notReady++ < 5) Log("[TILES] self-check: measurement frame results not ready yet (skipped)");
        }
        if (ready)
        {
            if (g_tileQueryMeasure[parity]) keptWithout = kept; else keptWith = kept;
            if (g_tileQueryMeasure[parity] && keptWithout > 20000)
            {
                const double ratio = static_cast<double>(keptWith) / static_cast<double>(keptWithout);
                static int reports = 0;
                if (reports++ < 40 || ratio < 0.6)
                    Log("[TILES] self-check: samples kept with the tile test %llu, without %llu, ratio %.2f%s", keptWith, keptWithout, ratio,
                        g_tilePixelTest ? "" : " (test already off)");
                if (g_tilePixelTest && ratio < 0.6 && ++g_tileSelfFail >= 2)
                {
                    g_tilePixelTest = false;
                    Log("[TILES] self-check FAILED twice: per-pixel tile test DISABLED (lighting falls back to renderfix4 tiles)");
                }
                if (ratio >= 0.6) g_tileSelfFail = 0;
            }
        }
        g_tileQueryUsed[(g_frame + 1) & 1] = 0;   // that parity is reused by the next frame
        g_tileMeasureFrame = g_tilePixelTest && ((g_frame + 1) % 120 == 0);
    }
    RenderInsertMenu(backbuffer,true);
    backbuffer->Release();
    ++g_frame;
    if (!headset)
        PaceFrame();    // the headset paces frames itself
}
}

