#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <xinput.h>
#include <MinHook.h>
#include <cmath>
#include <cstring>

#include "fc3_log.h"
#include "head_track.h"

using fc3::Log;

namespace
{
constexpr float kPi = 3.14159265358979f;

// ---- injection into the game's mouse device ---------------------------------
using GetDeviceStateFn = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, LPVOID);
using GetDeviceDataFn = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
using XInputGetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);

GetDeviceStateFn g_oGetDeviceState[2] = {};     // [ANSI, wide] interface
GetDeviceDataFn g_oGetDeviceData[2] = {};
XInputGetStateFn g_xinputGetState = nullptr;

volatile LONG g_pendingX = 0, g_pendingY = 0;   // counts waiting for the game's next poll
volatile LONG g_deliveredX = 0, g_deliveredY = 0;
volatile LONG g_physicalMouse = 0;              // real mouse movement seen since last frame
volatile LONG g_pollsState = 0, g_pollsData = 0;
void* g_mouseDevice = nullptr;                  // last device identified as a mouse
void* g_notMouse[4] = {};

bool IsMouseDevice(IUnknown* device)
{
    if (device == g_mouseDevice) return true;
    for (void* known : g_notMouse)
        if (known == device) return false;
    DIDEVCAPS caps = {};
    caps.dwSize = sizeof(caps);
    const bool mouse = SUCCEEDED(static_cast<IDirectInputDevice8A*>(device)->GetCapabilities(&caps)) &&
                       GET_DIDEVICE_TYPE(caps.dwDevType) == DI8DEVTYPE_MOUSE;
    if (mouse)
    {
        g_mouseDevice = device;
    }
    else
    {
        static int next = 0;
        g_notMouse[next++ & 3] = device;
    }
    return mouse;
}

HRESULT GetDeviceStateCommon(int which, IUnknown* device, DWORD size, LPVOID data)
{
    const HRESULT result = g_oGetDeviceState[which](device, size, data);
    // Only the mouse uses these two state sizes.
    if (SUCCEEDED(result) && data && (size == sizeof(DIMOUSESTATE) || size == sizeof(DIMOUSESTATE2)))
    {
        DIMOUSESTATE* state = static_cast<DIMOUSESTATE*>(data);
        if (state->lX != 0 || state->lY != 0)
            InterlockedExchange(&g_physicalMouse, 1);
        const LONG x = InterlockedExchange(&g_pendingX, 0);
        const LONG y = InterlockedExchange(&g_pendingY, 0);
        state->lX += x;
        state->lY += y;
        InterlockedExchangeAdd(&g_deliveredX, x);
        InterlockedExchangeAdd(&g_deliveredY, y);
        const LONG polls = InterlockedIncrement(&g_pollsState);
        if (polls == 1)
            Log("[HEAD] the game reads the mouse with GetDeviceState (size %lu)", size);
    }
    return result;
}

HRESULT GetDeviceDataCommon(int which, IUnknown* device, DWORD objectSize, LPDIDEVICEOBJECTDATA data,
                            LPDWORD count, DWORD flags)
{
    const DWORD capacity = count ? *count : 0;
    const HRESULT result = g_oGetDeviceData[which](device, objectSize, data, count, flags);
    if (FAILED(result) || !data || !count || (flags & DIGDD_PEEK) || objectSize < sizeof(DIDEVICEOBJECTDATA_DX3) ||
        !IsMouseDevice(device))
        return result;

    const LONG polls = InterlockedIncrement(&g_pollsData);
    if (polls == 1)
        Log("[HEAD] the game reads the mouse with GetDeviceData (buffered, entry size %lu)", objectSize);

    BYTE* entries = reinterpret_cast<BYTE*>(data);
    DWORD used = *count;
    DWORD sequence = 0;
    DIDEVICEOBJECTDATA_DX3* xEntry = nullptr;
    DIDEVICEOBJECTDATA_DX3* yEntry = nullptr;
    for (DWORD i = 0; i < used; ++i)
    {
        auto* entry = reinterpret_cast<DIDEVICEOBJECTDATA_DX3*>(entries + i * objectSize);
        sequence = entry->dwSequence;
        if (entry->dwOfs == DIMOFS_X) { xEntry = entry; if (entry->dwData) InterlockedExchange(&g_physicalMouse, 1); }
        if (entry->dwOfs == DIMOFS_Y) { yEntry = entry; if (entry->dwData) InterlockedExchange(&g_physicalMouse, 1); }
    }

    const LONG pending[2] = { InterlockedExchange(&g_pendingX, 0), InterlockedExchange(&g_pendingY, 0) };
    DIDEVICEOBJECTDATA_DX3* existing[2] = { xEntry, yEntry };
    const DWORD offsets[2] = { DIMOFS_X, DIMOFS_Y };
    LONG delivered[2] = { 0, 0 };
    for (int axis = 0; axis < 2; ++axis)
    {
        if (pending[axis] == 0) continue;
        if (existing[axis])
        {
            existing[axis]->dwData = static_cast<DWORD>(static_cast<LONG>(existing[axis]->dwData) + pending[axis]);
            delivered[axis] = pending[axis];
        }
        else if (used < capacity)
        {
            BYTE* slot = entries + used * objectSize;
            std::memset(slot, 0, objectSize);
            auto* entry = reinterpret_cast<DIDEVICEOBJECTDATA_DX3*>(slot);
            entry->dwOfs = offsets[axis];
            entry->dwData = static_cast<DWORD>(pending[axis]);
            entry->dwTimeStamp = GetTickCount();
            entry->dwSequence = ++sequence;
            ++used;
            delivered[axis] = pending[axis];
        }
        else
        {
            // no room this poll: keep it for the next one
            InterlockedExchangeAdd(axis == 0 ? &g_pendingX : &g_pendingY, pending[axis]);
        }
    }
    *count = used;
    InterlockedExchangeAdd(&g_deliveredX, delivered[0]);
    InterlockedExchangeAdd(&g_deliveredY, delivered[1]);
    return result;
}

HRESULT STDMETHODCALLTYPE HookGetDeviceStateA(IUnknown* d, DWORD s, LPVOID p) { return GetDeviceStateCommon(0, d, s, p); }
HRESULT STDMETHODCALLTYPE HookGetDeviceStateW(IUnknown* d, DWORD s, LPVOID p) { return GetDeviceStateCommon(1, d, s, p); }
HRESULT STDMETHODCALLTYPE HookGetDeviceDataA(IUnknown* d, DWORD s, LPDIDEVICEOBJECTDATA p, LPDWORD c, DWORD f) { return GetDeviceDataCommon(0, d, s, p, c, f); }
HRESULT STDMETHODCALLTYPE HookGetDeviceDataW(IUnknown* d, DWORD s, LPDIDEVICEOBJECTDATA p, LPDWORD c, DWORD f) { return GetDeviceDataCommon(1, d, s, p, c, f); }

bool HookOne(void* target, void* detour, void** original, const char* name)
{
    const MH_STATUS create = MH_CreateHook(target, detour, original);
    if (create == MH_ERROR_ALREADY_CREATED) return true;    // ANSI and wide share the function
    const MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : create;
    if (create != MH_OK || enable != MH_OK)
    {
        Log("[HEAD] hook FAILED %s create=%d enable=%d", name, static_cast<int>(create), static_cast<int>(enable));
        return false;
    }
    return true;
}

// ---- angles -----------------------------------------------------------------
float Wrap(float angle)
{
    while (angle > kPi) angle -= 2.0f * kPi;
    while (angle < -kPi) angle += 2.0f * kPi;
    return angle;
}

float Clamp(float value, float low, float high)
{
    return value < low ? low : value > high ? high : value;
}

// World "up" axis of the game, latched from the camera's own up vector.
int g_upAxis = -1;
float g_upSign = 1.0f;

// Yaw (positive = turning left) and pitch (positive = looking up) of the game camera.
bool GameAngles(const fc3head::Camera& camera, float& yaw, float& pitch)
{
    if (g_upAxis < 0)
    {
        int axis = 0;
        for (int i = 1; i < 3; ++i)
            if (std::fabs(camera.up[i]) > std::fabs(camera.up[axis])) axis = i;
        if (std::fabs(camera.up[axis]) < 0.8f)
            return false;       // looking steeply up or down: wait for a level view
        g_upAxis = axis;
        g_upSign = camera.up[axis] > 0 ? 1.0f : -1.0f;
        Log("[HEAD] game world up axis = %c%c", g_upSign > 0 ? '+' : '-', "XYZ"[axis]);
    }
    const int a = g_upAxis, b = (a + 1) % 3, c = (a + 2) % 3;
    const float* f = camera.forward;
    // forward x up, to learn the handedness from the camera's right vector
    float upVector[3] = { 0, 0, 0 };
    upVector[a] = g_upSign;
    const float cross[3] = {
        f[1] * upVector[2] - f[2] * upVector[1],
        f[2] * upVector[0] - f[0] * upVector[2],
        f[0] * upVector[1] - f[1] * upVector[0] };
    const float handed = cross[0] * camera.right[0] + cross[1] * camera.right[1] + cross[2] * camera.right[2];
    const float raw = std::atan2(f[c], f[b]) * g_upSign;
    yaw = handed >= 0 ? raw : -raw;
    pitch = std::asin(Clamp(f[a] * g_upSign, -1.0f, 1.0f));
    return true;
}

void HeadAngles(const float q[4], float& yaw, float& pitch)
{
    // forward = q * (0,0,-1)
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float fx = -(2.0f * (x * z + w * y));
    const float fy = -(2.0f * (y * z - w * x));
    const float fz = -(1.0f - 2.0f * (x * x + y * y));
    yaw = std::atan2(-fx, -fz);
    pitch = std::asin(Clamp(fy, -1.0f, 1.0f));
}

// ---- controller state -------------------------------------------------------
bool g_haveCamera = false;          // this frame
float g_gameYaw = 0, g_gamePitch = 0;
bool g_havePrevious = false;
float g_previousYaw = 0, g_previousPitch = 0;
int g_framesWithoutCamera = 1000;

float g_bodyYaw = 0;                // game yaw minus head yaw
bool g_bodyValid = false;

// response of the camera per injected count, radians, signed, per axis
float g_gain[2] = { -0.004f, -0.004f };
// least-squares accumulators for two possible input latencies (0 or 1 frame)
float g_corr[2][2] = {};            // [axis][latency] sum(delta * counts)
float g_power[2][2] = {};           // [axis][latency] sum(counts^2)
int g_latency = 0;
LONG g_countsNow[2] = {};           // delivered during the frame just rendered
LONG g_countsBefore[2] = {};        // delivered the frame before
int g_stickFrames = 0;              // > 0 while the stick or the real mouse is turning the view

float g_smoothHead[4] = { 0, 0, 0, 1 };
bool g_smoothValid = false;
float g_targetYaw = 0, g_targetPitch = 0;
float g_errorSum = 0;               // for the status line
int g_errorCount = 0;
bool g_active = false;
int g_stall = 0;                    // frames in a row the camera ignored the injected input
int g_suspend = 0;                  // frames left before trying again

bool StickActive()
{
    bool active = InterlockedExchange(&g_physicalMouse, 0) != 0;
    if (g_xinputGetState)
    {
        XINPUT_STATE state = {};
        if (g_xinputGetState(0, &state) == ERROR_SUCCESS)
        {
            const int x = state.Gamepad.sThumbRX, y = state.Gamepad.sThumbRY;
            if (x > 9000 || x < -9000 || y > 9000 || y < -9000) active = true;
        }
    }
    return active;
}
}

namespace fc3head
{
void InstallInputHooks()
{
    static bool done = false;
    if (done) return;
    done = true;

    const MH_STATUS init = MH_Initialize();
    if (init != MH_OK && init != MH_ERROR_ALREADY_INITIALIZED) return;

    if (HMODULE xinput = LoadLibraryW(L"xinput1_3.dll"))
        g_xinputGetState = reinterpret_cast<XInputGetStateFn>(GetProcAddress(xinput, "XInputGetState"));

    // The device functions are found by creating a private mouse device and
    // reading its vtable. Hooking the functions themselves covers every device
    // the game creates, before or after this point.
    int hooked = 0;
    for (int wide = 0; wide < 2; ++wide)
    {
        IUnknown* directInput = nullptr;
        if (FAILED(DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
                                      wide ? IID_IDirectInput8W : IID_IDirectInput8A,
                                      reinterpret_cast<void**>(&directInput), nullptr)) || !directInput)
            continue;
        IUnknown* device = nullptr;
        const HRESULT created = wide
            ? static_cast<IDirectInput8W*>(directInput)->CreateDevice(GUID_SysMouse, reinterpret_cast<IDirectInputDevice8W**>(&device), nullptr)
            : static_cast<IDirectInput8A*>(directInput)->CreateDevice(GUID_SysMouse, reinterpret_cast<IDirectInputDevice8A**>(&device), nullptr);
        if (SUCCEEDED(created) && device)
        {
            void** table = *reinterpret_cast<void***>(device);
            if (HookOne(table[9], wide ? static_cast<void*>(&HookGetDeviceStateW) : static_cast<void*>(&HookGetDeviceStateA),
                        reinterpret_cast<void**>(&g_oGetDeviceState[wide]), "GetDeviceState")) ++hooked;
            if (HookOne(table[10], wide ? static_cast<void*>(&HookGetDeviceDataW) : static_cast<void*>(&HookGetDeviceDataA),
                        reinterpret_cast<void**>(&g_oGetDeviceData[wide]), "GetDeviceData")) ++hooked;
            device->Release();
        }
        directInput->Release();
    }
    // When both interfaces share one function, the second hook attempt reports
    // "already created" and its trampoline stays null: route it to the first.
    if (!g_oGetDeviceState[1]) g_oGetDeviceState[1] = g_oGetDeviceState[0];
    if (!g_oGetDeviceData[1]) g_oGetDeviceData[1] = g_oGetDeviceData[0];
    if (!g_oGetDeviceState[0]) g_oGetDeviceState[0] = g_oGetDeviceState[1];
    if (!g_oGetDeviceData[0]) g_oGetDeviceData[0] = g_oGetDeviceData[1];
    Log("[HEAD] mouse input hooks %d/4, gamepad read %s", hooked, g_xinputGetState ? "ok" : "unavailable");
}

void FrameRendered(const Camera* camera)
{
    g_countsBefore[0] = g_countsNow[0];
    g_countsBefore[1] = g_countsNow[1];
    g_countsNow[0] = InterlockedExchange(&g_deliveredX, 0);
    g_countsNow[1] = InterlockedExchange(&g_deliveredY, 0);

    if (StickActive()) g_stickFrames = 6;
    else if (g_stickFrames > 0) --g_stickFrames;

    g_haveCamera = camera && GameAngles(*camera, g_gameYaw, g_gamePitch);
    if (!g_haveCamera)
    {
        ++g_framesWithoutCamera;
        g_havePrevious = false;
        return;
    }
    // After a gap (menu, loading, cutscene) the body direction is re-latched
    // so the view does not jump.
    if (g_framesWithoutCamera > 30) g_bodyValid = false;
    g_framesWithoutCamera = 0;

    if (g_havePrevious)
    {
        const float delta[2] = { Wrap(g_gameYaw - g_previousYaw), g_gamePitch - g_previousPitch };
        if (g_stickFrames == 0)
        {
            // Learn how far the camera turns per injected count, and whether
            // the game applies input the same frame or one frame later.
            for (int axis = 0; axis < 2; ++axis)
            {
                const float counts[2] = { static_cast<float>(g_countsNow[axis]), static_cast<float>(g_countsBefore[axis]) };
                for (int latency = 0; latency < 2; ++latency)
                {
                    g_corr[axis][latency] = g_corr[axis][latency] * 0.97f + delta[axis] * counts[latency];
                    g_power[axis][latency] = g_power[axis][latency] * 0.97f + counts[latency] * counts[latency];
                }
            }
            // Input going in but the camera not answering = a menu, a cutscene
            // or a paused game. Stop pushing and try again a little later.
            const float expected = std::fabs(static_cast<float>(g_latency ? g_countsBefore[0] : g_countsNow[0]) * g_gain[0]) +
                                   std::fabs(static_cast<float>(g_latency ? g_countsBefore[1] : g_countsNow[1]) * g_gain[1]);
            if (expected > 0.01f)
            {
                if (std::fabs(delta[0]) + std::fabs(delta[1]) < expected * 0.02f) ++g_stall;
                else g_stall = 0;
                if (g_stall > 25)
                {
                    static int reported = 0;
                    if (reported++ < 8) Log("[HEAD] the camera is not responding to input - pausing tracking for a moment");
                    g_stall = 0;
                    g_suspend = 180;
                    g_bodyValid = false;
                }
            }
            const float score0 = std::fabs(g_corr[0][0]) + std::fabs(g_corr[1][0]);
            const float score1 = std::fabs(g_corr[0][1]) + std::fabs(g_corr[1][1]);
            g_latency = score1 > score0 * 1.5f ? 1 : (score0 > score1 * 1.5f ? 0 : g_latency);
            for (int axis = 0; axis < 2; ++axis)
            {
                if (g_power[axis][g_latency] > 200.0f)
                {
                    const float gain = g_corr[axis][g_latency] / g_power[axis][g_latency];
                    if (std::fabs(gain) > 0.00002f && std::fabs(gain) < 0.05f)
                        g_gain[axis] = gain;
                }
            }
        }
        else if (g_bodyValid)
        {
            // Stick (or real mouse) turned the view: that part belongs to the
            // body, not the head. The injected part is subtracted out.
            const float injected = static_cast<float>(g_latency ? g_countsBefore[0] : g_countsNow[0]) * g_gain[0];
            g_bodyYaw = Wrap(g_bodyYaw + delta[0] - injected);
        }
    }
    g_previousYaw = g_gameYaw;
    g_previousPitch = g_gamePitch;
    g_havePrevious = true;
}

bool TagOrientation(float q[4])
{
    if (!g_haveCamera || !g_bodyValid || !g_active)
        return false;
    // The frame was rendered looking along the measured direction. In the
    // headset's space that is: yaw relative to the body, the measured pitch,
    // and no roll (the game camera never rolls).
    const float yaw = Wrap(g_gameYaw - g_bodyYaw);
    const float pitch = g_gamePitch;
    const float sy = std::sin(yaw * 0.5f), cy = std::cos(yaw * 0.5f);
    const float sx = std::sin(pitch * 0.5f), cx = std::cos(pitch * 0.5f);
    q[0] = cy * sx;
    q[1] = sy * cx;
    q[2] = -sy * sx;
    q[3] = cy * cx;
    return true;
}

void Command(const float head[4], bool active)
{
    if (g_suspend > 0) --g_suspend;
    g_active = active && g_haveCamera && g_suspend == 0;
    if (!g_active)
    {
        InterlockedExchange(&g_pendingX, 0);
        InterlockedExchange(&g_pendingY, 0);
        g_smoothValid = false;
        return;
    }

    // One filtered head orientation. Nearly 1:1 when the head is moving, a
    // little smoothing when it is almost still.
    if (!g_smoothValid)
    {
        std::memcpy(g_smoothHead, head, sizeof(g_smoothHead));
        g_smoothValid = true;
    }
    else
    {
        float dot = 0;
        for (int i = 0; i < 4; ++i) dot += g_smoothHead[i] * head[i];
        const float sign = dot < 0 ? -1.0f : 1.0f;
        const float angle = 2.0f * std::acos(Clamp(std::fabs(dot), 0.0f, 1.0f));     // radians this frame
        const float follow = Clamp(0.5f + angle * 60.0f, 0.5f, 1.0f);
        float length = 0;
        for (int i = 0; i < 4; ++i)
        {
            g_smoothHead[i] = g_smoothHead[i] + (head[i] * sign - g_smoothHead[i]) * follow;
            length += g_smoothHead[i] * g_smoothHead[i];
        }
        length = std::sqrt(length);
        if (length > 0.0001f)
            for (int i = 0; i < 4; ++i) g_smoothHead[i] /= length;
    }

    float headYaw = 0, headPitch = 0;
    HeadAngles(g_smoothHead, headYaw, headPitch);
    if (!g_bodyValid)
    {
        g_bodyYaw = Wrap(g_gameYaw - headYaw);
        g_bodyValid = true;
        Log("[HEAD] tracking engaged");
    }
    g_targetYaw = Wrap(g_bodyYaw + headYaw);
    g_targetPitch = Clamp(headPitch, -1.45f, 1.45f);

    // Where the camera will be once input already delivered has taken effect.
    float predicted[2] = { g_gameYaw, g_gamePitch };
    if (g_latency == 1)
    {
        predicted[0] += static_cast<float>(g_countsNow[0]) * g_gain[0];
        predicted[1] += static_cast<float>(g_countsNow[1]) * g_gain[1];
    }
    float error[2] = { Wrap(g_targetYaw - predicted[0]), g_targetPitch - predicted[1] };
    g_errorSum += std::fabs(error[0]) + std::fabs(error[1]);
    ++g_errorCount;

    volatile LONG* pending[2] = { &g_pendingX, &g_pendingY };
    for (int axis = 0; axis < 2; ++axis)
    {
        // never step the view in one frame: at most about 9 degrees per frame
        const float step = Clamp(error[axis] * 0.9f, -0.16f, 0.16f);
        const float counts = step / g_gain[axis];
        const LONG whole = static_cast<LONG>(counts >= 0 ? counts + 0.5f : counts - 0.5f);
        InterlockedExchange(pending[axis], whole > 4000 ? 4000 : whole < -4000 ? -4000 : whole);
    }
}

// Offline test access: does what the game's mouse poll does.
void TestPoll(long& x, long& y)
{
    x = InterlockedExchange(&g_pendingX, 0);
    y = InterlockedExchange(&g_pendingY, 0);
    InterlockedExchangeAdd(&g_deliveredX, x);
    InterlockedExchangeAdd(&g_deliveredY, y);
}

void Recenter()
{
    g_bodyValid = false;
    Log("[HEAD] recentred");
}

void LogStatus(long frame)
{
    Log("[HEAD] frame %ld: %s, mouse polls state %ld / buffered %ld, response %.5f / %.5f rad per count, "
        "input latency %d frame, mean error %.3f deg, body yaw %.1f deg",
        frame, g_active ? "tracking" : "idle", g_pollsState, g_pollsData, g_gain[0], g_gain[1], g_latency,
        g_errorCount ? g_errorSum / g_errorCount * 57.29578f : 0.0f, g_bodyYaw * 57.29578f);
    g_errorSum = 0;
    g_errorCount = 0;
}
}
