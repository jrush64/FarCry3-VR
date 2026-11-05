#include "xr_hit_haptics121.h"
#include "haptics302.h"
#include "gunfire339.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_loader_negotiation.h>

#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "fc3_log.h"
#include "fov_boost.h"
#include "stereo.h"
#include "vr_pad.h"
#include "support_continuation356.h"
#include "aim_probe.h"
#include "xr_presenter.h"
#include "options_pointer299.h"
#include "weapon_notes303.h"
#include "hand_ui_format.h"
#include "xr_hand_tracking.h"
#include "xr_body_pose.h"
#include "test_notes_ui.h"
#include "selector290.h"
#include "selector_scene292.h"
#include "selector_policy290.h"

using fc3::Log;

namespace
{
// Every entry point is resolved at run time. Nothing here is a link-time
// dependency, so a missing loader or runtime cannot stop the game starting.
#define XR_FUNCS(X)                        \
    X(xrGetSystem)                         \
    X(xrCreateSession)                     \
    X(xrDestroySession)                    \
    X(xrCreateReferenceSpace)              \
    X(xrEnumerateViewConfigurationViews)   \
    X(xrEnumerateSwapchainFormats)         \
    X(xrCreateSwapchain)                   \
    X(xrDestroySwapchain)                  \
    X(xrEnumerateSwapchainImages)          \
    X(xrAcquireSwapchainImage)             \
    X(xrWaitSwapchainImage)                \
    X(xrReleaseSwapchainImage)             \
    X(xrBeginSession)                      \
    X(xrEndSession)                        \
    X(xrWaitFrame)                         \
    X(xrBeginFrame)                        \
    X(xrEndFrame)                          \
    X(xrLocateViews)                       \
    X(xrPollEvent)                         \
    X(xrStringToPath)                      \
    X(xrPathToString)                      \
    X(xrCreateActionSet)                   \
    X(xrCreateAction)                      \
    X(xrSuggestInteractionProfileBindings) \
    X(xrAttachSessionActionSets)           \
    X(xrSyncActions)                       \
    X(xrGetActionStateBoolean)             \
    X(xrGetActionStateFloat)               \
    X(xrGetActionStateVector2f)            \
    X(xrGetCurrentInteractionProfile)      \
    X(xrCreateActionSpace)                 \
    X(xrLocateSpace)                       \
    X(xrApplyHapticFeedback)               \
    X(xrStopHapticFeedback)                \
    X(xrGetD3D11GraphicsRequirementsKHR)

#define XR_DECL(fn) PFN_##fn p_##fn = nullptr;
XR_FUNCS(XR_DECL)
#undef XR_DECL
PFN_xrGetInstanceProcAddr p_xrGetInstanceProcAddr = nullptr;
PFN_xrCreateInstance p_xrCreateInstance = nullptr;

XrInstance g_instance = XR_NULL_HANDLE;
XrSystemId g_system = XR_NULL_SYSTEM_ID;
XrSession g_session = XR_NULL_HANDLE;
XrSpace g_space = XR_NULL_HANDLE;
XrSwapchain g_chain[2] = { XR_NULL_HANDLE, XR_NULL_HANDLE };
ID3D11Texture2D* g_images[2][8] = {};
uint32_t g_imageCount[2] = {};
uint32_t g_width = 0, g_height = 0;
DXGI_FORMAT g_format = DXGI_FORMAT_UNKNOWN;

bool g_tried = false;
bool g_instanceReady = false;
bool g_sessionRunning = false;      // between xrBeginSession and xrEndSession
bool g_frameBegun = false;
bool g_shouldRender = false;
bool g_viewsValid = false;
XrTime g_displayTime = 0;
fc3stereo::EyeView g_probeEyes[2]={};
XrView g_views[2] = {};             // located for the frame now being rendered
LONG g_submitted = 0, g_failures = 0, g_emptyFrames = 0;
// Head tracking. The game's camera is the "body": it points where the stick
// turned it. The head pose is measured relative to a reference taken when
// tracking starts (or on recentre): that yaw is "straight ahead of the body"
// and that position is "where the game camera is".
bool g_referenceValid = false;
volatile LONG g_referenceGeneration286=0;
float g_referenceYaw = 0;
XrVector3f g_referencePosition = {};
bool g_viewsTracked = false;
bool g_frameTracked = false;        // the frame in flight was rendered from the eye poses
LONG g_trackedFrames = 0;

// ---- steady cadence ---------------------------------------------------------
// The game's frame time rarely matches the headset's refresh. Left alone, the
// number of refreshes each frame is shown for alternates (2,3,2,3...), which
// reads as judder whenever the head turns. Every frame is therefore held to
// the same whole number of refresh periods.
LARGE_INTEGER g_qpcFrequency = {};
LONGLONG g_lastWaitReturn = 0;      // QPC when xrWaitFrame last returned
LONGLONG g_periodNs = 0;            // headset refresh period
float g_workMs[120] = {};           // recent "time the frame needed" samples
int g_workIndex = 0, g_workCount = 0;
int g_paceNow = 1, g_paceLowerStreak = 0;
LONG g_paceHistogram[8] = {};

double QpcMs(LONGLONG ticks)
{
    return static_cast<double>(ticks) * 1000.0 / static_cast<double>(g_qpcFrequency.QuadPart);
}

void PaceBeforeWait()
{
    if (!g_qpcFrequency.QuadPart) QueryPerformanceFrequency(&g_qpcFrequency);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (g_lastWaitReturn == 0 || g_periodNs <= 0)
        return;
    const double periodMs = static_cast<double>(g_periodNs) / 1.0e6;
    const double work = QpcMs(now.QuadPart - g_lastWaitReturn);
    g_workMs[g_workIndex] = static_cast<float>(work);
    g_workIndex = (g_workIndex + 1) % 120;
    if (g_workCount < 120) ++g_workCount;

    const int setting = fc3stereo::PacePeriods();
    if (setting < 0)
        return;
    int wanted = setting;
    if (setting == 0)
    {
        // 90th percentile of the recent frame times, plus a little slack
        float sorted[120];
        std::memcpy(sorted, g_workMs, sizeof(float) * g_workCount);
        for (int i = 1; i < g_workCount; ++i)
        {
            const float v = sorted[i];
            int j = i - 1;
            while (j >= 0 && sorted[j] > v) { sorted[j + 1] = sorted[j]; --j; }
            sorted[j + 1] = v;
        }
        const double p90 = sorted[(g_workCount * 9) / 10 < g_workCount ? (g_workCount * 9) / 10 : g_workCount - 1];
        wanted = static_cast<int>(std::ceil((p90 + 0.75) / periodMs));
        if (wanted < 1) wanted = 1;
        if (wanted > 6) wanted = 6;
        if (wanted > g_paceNow)
        {
            g_paceNow = wanted;             // slower: follow at once
            g_paceLowerStreak = 0;
        }
        else if (wanted < g_paceNow)
        {
            if (++g_paceLowerStreak > 240)  // faster: only after it has held for a while
            {
                Log("[PACE] frames now fit %d refresh period(s), was %d", wanted, g_paceNow);
                g_paceNow = wanted;
                g_paceLowerStreak = 0;
            }
        }
        else
        {
            g_paceLowerStreak = 0;
        }
        wanted = g_paceNow;
    }

    // Hold until just before the chosen refresh boundary; xrWaitFrame then
    // releases on the boundary itself.
    const double targetMs = wanted * periodMs - 2.0;
    for (;;)
    {
        QueryPerformanceCounter(&now);
        const double elapsed = QpcMs(now.QuadPart - g_lastWaitReturn);
        if (elapsed >= targetMs) break;
        if (targetMs - elapsed > 2.5) Sleep(1);
        else YieldProcessor();
    }
}

bool Ok(XrResult result, const char* what)
{
    if (XR_SUCCEEDED(result)) return true;
    const char* hint = "";
    if (result == XR_ERROR_RUNTIME_UNAVAILABLE) hint = " (no OpenXR runtime running)";
    if (result == XR_ERROR_FORM_FACTOR_UNAVAILABLE) hint = " (headset not connected or not ready)";
    if (result == XR_ERROR_API_VERSION_UNSUPPORTED) hint = " (API version refused)";
    Log("[XR] %s failed: %d%s", what, static_cast<int>(result), hint);
    return false;
}


// ---- motion controllers -----------------------------------------------------
enum ControllerInput
{
    kMoveStick, kLookStick,                         // vector2
    kLeftTrigger, kRightTrigger, kLeftGrip, kRightGrip,   // float
    kButtonA, kButtonB, kButtonX, kButtonY, kMenu, kLeftClick, kRightClick,   // boolean
    kLeftIndexTouch, kRightIndexTouch, kLeftStickTouch, kRightStickTouch,       // boolean: finger sensors
    kXTouch, kYTouch, kATouch, kBTouch, kLeftThumbRest, kRightThumbRest,
    kRightAim, kLeftAim,                            // pose
    kHapticRight, kHapticLeft,                      // vibration output
    kInputCount
};
XrActionSet g_actionSet = XR_NULL_HANDLE;
XrAction g_actions[kInputCount] = {};
XrSpace g_handSpaces[2] = { XR_NULL_HANDLE, XR_NULL_HANDLE };     // right, left
bool g_actionsReady = false;

XrPath Path(const char* text)
{
    XrPath path = XR_NULL_PATH;
    p_xrStringToPath(g_instance, text, &path);
    return path;
}

void SuggestBindings(const char* profile, const char* const paths[kInputCount])
{
    XrActionSuggestedBinding bindings[kInputCount] = {};
    uint32_t count = 0;
    for (int i = 0; i < kInputCount; ++i)
    {
        if (!paths[i]) continue;
        bindings[count].action = g_actions[i];
        bindings[count].binding = Path(paths[i]);
        ++count;
    }
    XrInteractionProfileSuggestedBinding suggested = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    suggested.interactionProfile = Path(profile);
    suggested.suggestedBindings = bindings;
    suggested.countSuggestedBindings = count;
    const XrResult result = p_xrSuggestInteractionProfileBindings(g_instance, &suggested);
    if (!XR_SUCCEEDED(result)) Log("[XR] controller bindings for %s refused: %d", profile, static_cast<int>(result));
}

// Creates the controller actions and attaches them to the session.
void CreateControllerActions()
{
    XrActionSetCreateInfo setInfo = { XR_TYPE_ACTION_SET_CREATE_INFO };
    strcpy_s(setInfo.actionSetName, "gameplay");
    strcpy_s(setInfo.localizedActionSetName, "Gameplay");
    if (!Ok(p_xrCreateActionSet(g_instance, &setInfo, &g_actionSet), "xrCreateActionSet")) return;

    static const struct { const char* name; XrActionType type; } kinds[kInputCount] = {
        { "move", XR_ACTION_TYPE_VECTOR2F_INPUT }, { "look", XR_ACTION_TYPE_VECTOR2F_INPUT },
        { "left_trigger", XR_ACTION_TYPE_FLOAT_INPUT }, { "right_trigger", XR_ACTION_TYPE_FLOAT_INPUT },
        { "left_grip", XR_ACTION_TYPE_FLOAT_INPUT }, { "right_grip", XR_ACTION_TYPE_FLOAT_INPUT },
        { "button_a", XR_ACTION_TYPE_BOOLEAN_INPUT }, { "button_b", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { "button_x", XR_ACTION_TYPE_BOOLEAN_INPUT }, { "button_y", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { "menu", XR_ACTION_TYPE_BOOLEAN_INPUT }, { "left_stick_click", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { "right_stick_click", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { "left_index_touch", XR_ACTION_TYPE_BOOLEAN_INPUT }, { "right_index_touch", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { "left_stick_touch", XR_ACTION_TYPE_BOOLEAN_INPUT }, { "right_stick_touch", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { "x_touch", XR_ACTION_TYPE_BOOLEAN_INPUT }, { "y_touch", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { "a_touch", XR_ACTION_TYPE_BOOLEAN_INPUT }, { "b_touch", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { "left_thumbrest", XR_ACTION_TYPE_BOOLEAN_INPUT }, { "right_thumbrest", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { "right_aim", XR_ACTION_TYPE_POSE_INPUT }, { "left_aim", XR_ACTION_TYPE_POSE_INPUT },
        { "right_haptic", XR_ACTION_TYPE_VIBRATION_OUTPUT }, { "left_haptic", XR_ACTION_TYPE_VIBRATION_OUTPUT } };
    for (int i = 0; i < kInputCount; ++i)
    {
        XrActionCreateInfo info = { XR_TYPE_ACTION_CREATE_INFO };
        info.actionType = kinds[i].type;
        strcpy_s(info.actionName, kinds[i].name);
        strcpy_s(info.localizedActionName, kinds[i].name);
        if (!Ok(p_xrCreateAction(g_actionSet, &info, &g_actions[i]), "xrCreateAction")) return;
    }

    static const char* const touch[kInputCount] = {
        "/user/hand/left/input/thumbstick", "/user/hand/right/input/thumbstick",
        "/user/hand/left/input/trigger/value", "/user/hand/right/input/trigger/value",
        "/user/hand/left/input/squeeze/value", "/user/hand/right/input/squeeze/value",
        "/user/hand/right/input/a/click", "/user/hand/right/input/b/click",
        "/user/hand/left/input/x/click", "/user/hand/left/input/y/click",
        "/user/hand/left/input/menu/click", "/user/hand/left/input/thumbstick/click",
        "/user/hand/right/input/thumbstick/click",
        "/user/hand/left/input/trigger/touch", "/user/hand/right/input/trigger/touch",
        "/user/hand/left/input/thumbstick/touch", "/user/hand/right/input/thumbstick/touch",
        "/user/hand/left/input/x/touch", "/user/hand/left/input/y/touch",
        "/user/hand/right/input/a/touch", "/user/hand/right/input/b/touch",
        "/user/hand/left/input/thumbrest/touch", "/user/hand/right/input/thumbrest/touch",
        "/user/hand/right/input/aim/pose", "/user/hand/left/input/aim/pose",
        "/user/hand/right/output/haptic", "/user/hand/left/output/haptic" };
    static const char* const index[kInputCount] = {
        "/user/hand/left/input/thumbstick", "/user/hand/right/input/thumbstick",
        "/user/hand/left/input/trigger/value", "/user/hand/right/input/trigger/value",
        "/user/hand/left/input/squeeze/value", "/user/hand/right/input/squeeze/value",
        "/user/hand/right/input/a/click", "/user/hand/right/input/b/click",
        "/user/hand/left/input/a/click", "/user/hand/left/input/b/click",
        nullptr, "/user/hand/left/input/thumbstick/click",
        "/user/hand/right/input/thumbstick/click",
        "/user/hand/left/input/trigger/touch", "/user/hand/right/input/trigger/touch",
        "/user/hand/left/input/thumbstick/touch", "/user/hand/right/input/thumbstick/touch",
        "/user/hand/left/input/a/touch", "/user/hand/left/input/b/touch",
        "/user/hand/right/input/a/touch", "/user/hand/right/input/b/touch",
        nullptr, nullptr,
        "/user/hand/right/input/aim/pose", "/user/hand/left/input/aim/pose",
        "/user/hand/right/output/haptic", "/user/hand/left/output/haptic" };
    SuggestBindings("/interaction_profiles/oculus/touch_controller", touch);
    SuggestBindings("/interaction_profiles/valve/index_controller", index);

    XrSessionActionSetsAttachInfo attach = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    attach.countActionSets = 1;
    attach.actionSets = &g_actionSet;
    if (!Ok(p_xrAttachSessionActionSets(g_session, &attach), "xrAttachSessionActionSets")) return;
    for (int hand = 0; hand < 2; ++hand)
    {
        XrActionSpaceCreateInfo spaceInfo = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
        spaceInfo.action = g_actions[hand == 0 ? kRightAim : kLeftAim];
        spaceInfo.poseInActionSpace.orientation.w = 1.0f;
        Ok(p_xrCreateActionSpace(g_session, &spaceInfo, &g_handSpaces[hand]), "xrCreateActionSpace");
    }
    g_actionsReady = true;
    Log("[XR] controller actions ready (Touch and Index bindings suggested)");
}

void LogInteractionProfile()
{
    if (!g_actionsReady) return;
    const char* hands[2] = { "/user/hand/left", "/user/hand/right" };
    for (int h = 0; h < 2; ++h)
    {
        XrInteractionProfileState state = { XR_TYPE_INTERACTION_PROFILE_STATE };
        if (!XR_SUCCEEDED(p_xrGetCurrentInteractionProfile(g_session, Path(hands[h]), &state))) continue;
        char text[XR_MAX_PATH_LENGTH] = "none";
        uint32_t length = 0;
        if (state.interactionProfile != XR_NULL_PATH)
            p_xrPathToString(g_instance, state.interactionProfile, sizeof(text), &length, text);
        Log("[XR] %s controller: %s", h == 0 ? "left" : "right", text);
    }
}

volatile LONG g_interactionPulse285=0;
fc3hit121::Queue g_macheteHit121;
void NotesControls(fc3pad::Controllers& controls);
XrResult XRAPI_CALL ApplyFeedback302(XrSession,const XrHapticActionInfo*,const XrHapticBaseHeader*);

void Pulse()
{
    for (int hand = 0; hand < 2; ++hand)
    {
        XrHapticActionInfo info = { XR_TYPE_HAPTIC_ACTION_INFO };
        info.action = g_actions[hand == 0 ? kHapticRight : kHapticLeft];
        XrHapticVibration vibration = { XR_TYPE_HAPTIC_VIBRATION };
        vibration.duration = 120000000;     // 120 ms
        vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
        vibration.amplitude = 0.8f;
        if(info.action)ApplyFeedback302(g_session, &info, reinterpret_cast<const XrHapticBaseHeader*>(&vibration));
    }
}

#include "xr_haptics302.inl"
#include "xr_weapon_hands303.inl"

// Reads the controllers and hands them to the gamepad hook.
fc3ui::OptionsChord299 g_optionsChord299;
void SelectorControls290(fc3pad::Controllers& controls);
XrResult g_lastSync345=XR_ERROR_ACTIONSET_NOT_ATTACHED;
void SyncControllers()
{
    g_lastSync345=XR_ERROR_ACTIONSET_NOT_ATTACHED;
    if(!g_actionsReady)TickFeedback302(XR_ERROR_ACTIONSET_NOT_ATTACHED);
    fc3pad::Controllers out = {};
    if (g_actionsReady)
    {
        XrActiveActionSet active = { g_actionSet, XR_NULL_PATH };
        XrActionsSyncInfo sync = { XR_TYPE_ACTIONS_SYNC_INFO };
        sync.countActiveActionSets = 1;
        sync.activeActionSets = &active;
        const XrResult synced = p_xrSyncActions(g_session, &sync);g_lastSync345=synced;
        TickFeedback302(synced);
        static XrResult lastSync = XR_SUCCESS;
        if (synced != lastSync)
        {
            Log("[XR] controller sync result %d (0 = ok, %d = headset app not focused)", static_cast<int>(synced),
                static_cast<int>(XR_SESSION_NOT_FOCUSED));
            lastSync = synced;
        }
        if (synced == XR_SUCCESS)
        {
            XrActionStateGetInfo get = { XR_TYPE_ACTION_STATE_GET_INFO };
            float values[kInputCount] = {};
            bool any = false;
            for (int i = 0; i < kRightAim; ++i)
            {
                get.action = g_actions[i];
                if (i <= kLookStick)
                {
                    XrActionStateVector2f v = { XR_TYPE_ACTION_STATE_VECTOR2F };
                    if (XR_SUCCEEDED(p_xrGetActionStateVector2f(g_session, &get, &v)) && v.isActive)
                    {
                        any = true;
                        (i == kMoveStick ? out.leftX : out.rightX) = v.currentState.x;
                        (i == kMoveStick ? out.leftY : out.rightY) = v.currentState.y;
                    }
                }
                else if (i <= kRightGrip)
                {
                    XrActionStateFloat v = { XR_TYPE_ACTION_STATE_FLOAT };
                    if (XR_SUCCEEDED(p_xrGetActionStateFloat(g_session, &get, &v)) && v.isActive)
                    {
                        any = true;
                        values[i] = v.currentState;
                    }
                }
                else
                {
                    XrActionStateBoolean v = { XR_TYPE_ACTION_STATE_BOOLEAN };
                    if (XR_SUCCEEDED(p_xrGetActionStateBoolean(g_session, &get, &v)) && v.isActive)
                    {
                        any = true;
                        values[i] = v.currentState ? 1.0f : 0.0f;
                    }
                }
            }
            out.active = any;
            out.leftTrigger = values[kLeftTrigger];
            out.rightTrigger = values[kRightTrigger];
            out.leftGrip = values[kLeftGrip];
            out.rightGrip = values[kRightGrip];
            out.a = values[kButtonA] > 0.5f;
            out.b = values[kButtonB] > 0.5f;
            out.x = values[kButtonX] > 0.5f;
            out.y = values[kButtonY] > 0.5f;
            out.menu = values[kMenu] > 0.5f;
            out.leftStickClick = values[kLeftClick] > 0.5f;
            out.rightStickClick = values[kRightClick] > 0.5f;
            out.leftIndexTouch = values[kLeftIndexTouch] > 0.5f;
            out.rightIndexTouch = values[kRightIndexTouch] > 0.5f;
            out.leftThumbTouch = values[kLeftStickTouch] > 0.5f || values[kXTouch] > 0.5f ||
                                 values[kYTouch] > 0.5f || values[kLeftThumbRest] > 0.5f;
            out.rightThumbTouch = values[kRightStickTouch] > 0.5f || values[kATouch] > 0.5f ||
                                  values[kBTouch] > 0.5f || values[kRightThumbRest] > 0.5f;
        }
    }
    RememberRaw303(out);
    NotesControls(out);
    const auto settingsChord299=g_optionsChord299.Step(out.active,out.menu,out.rightGrip,out.leftGrip);
    if(settingsChord299.toggle)fc3stereo::RequestOptionsToggle299();
    if(settingsChord299.consume)out.menu=false;
    SelectorControls290(out);
    WeaponRoleControls303(out);
    fc3pad::Publish(out);
}

// ---- finding the runtime ----------------------------------------------------
// First choice: the standard loader DLL next to the game executable.
bool ResolveThroughLoader()
{
    wchar_t path[MAX_PATH] = {};
    HMODULE loader = nullptr;
    if (GetModuleFileNameW(nullptr, path, MAX_PATH))
    {
        if (wchar_t* slash = wcsrchr(path, L'\\'))
        {
            wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"openxr_loader.dll");
            loader = LoadLibraryW(path);
        }
    }
    if (!loader)
    {
        Log("[XR] openxr_loader.dll not loaded (error %lu)", GetLastError());
        return false;
    }
    p_xrGetInstanceProcAddr = reinterpret_cast<PFN_xrGetInstanceProcAddr>(
        GetProcAddress(loader, "xrGetInstanceProcAddr"));
    if (!p_xrGetInstanceProcAddr) return false;
    Log("[XR] using openxr_loader.dll");
    return true;
}

// Fallback: talk to the active runtime directly, the same handshake the loader
// performs. Avoids depending on a second DLL being allowed to load.
bool ResolveThroughRuntime()
{
    wchar_t json[MAX_PATH] = {};
    DWORD size = sizeof(json);
    if (GetEnvironmentVariableW(L"XR_RUNTIME_JSON", json, MAX_PATH) == 0 &&
        RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\OpenXR\\1", L"ActiveRuntime",
                     RRF_RT_REG_SZ, nullptr, json, &size) != ERROR_SUCCESS)
    {
        Log("[XR] no active OpenXR runtime is registered for 32-bit applications");
        return false;
    }

    HANDLE file = CreateFileW(json, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        Log("[XR] cannot open runtime manifest %ls", json);
        return false;
    }
    char text[4096] = {};
    DWORD read = 0;
    ReadFile(file, text, sizeof(text) - 1, &read, nullptr);
    CloseHandle(file);

    const char* key = std::strstr(text, "\"library_path\"");
    const char* open = key ? std::strchr(key + 14, '"') : nullptr;
    if (!open) return false;
    char library[MAX_PATH] = {};
    size_t length = 0;
    for (const char* c = open + 1; *c && *c != '"' && length + 1 < MAX_PATH; ++c)
    {
        if (*c == '\\' && c[1] == '\\') ++c;        // JSON escaped backslash
        library[length++] = *c == '/' ? '\\' : *c;
    }

    wchar_t full[MAX_PATH] = {};
    wchar_t wide[MAX_PATH] = {};
    MultiByteToWideChar(CP_UTF8, 0, library, -1, wide, MAX_PATH);
    if (wide[0] == L'.' || !wcschr(wide, L':'))
    {
        wcscpy_s(full, json);
        if (wchar_t* slash = wcsrchr(full, L'\\')) *(slash + 1) = 0;
        const wchar_t* relative = (wide[0] == L'.' && wide[1] == L'\\') ? wide + 2 : wide;
        wcscat_s(full, relative);
    }
    else
    {
        wcscpy_s(full, wide);
    }

    HMODULE runtime = LoadLibraryExW(full, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!runtime)
    {
        Log("[XR] cannot load runtime %ls (error %lu)", full, GetLastError());
        return false;
    }
    const auto negotiate = reinterpret_cast<PFN_xrNegotiateLoaderRuntimeInterface>(
        GetProcAddress(runtime, "xrNegotiateLoaderRuntimeInterface"));
    if (!negotiate) return false;

    XrNegotiateLoaderInfo loaderInfo = {};
    loaderInfo.structType = XR_LOADER_INTERFACE_STRUCT_LOADER_INFO;
    loaderInfo.structVersion = XR_LOADER_INFO_STRUCT_VERSION;
    loaderInfo.structSize = sizeof(loaderInfo);
    loaderInfo.minInterfaceVersion = 1;
    loaderInfo.maxInterfaceVersion = XR_CURRENT_LOADER_RUNTIME_VERSION;
    loaderInfo.minApiVersion = XR_MAKE_VERSION(1, 0, 0);
    loaderInfo.maxApiVersion = XR_MAKE_VERSION(1, 0x3ff, 0xfff);
    XrNegotiateRuntimeRequest request = {};
    request.structType = XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST;
    request.structVersion = XR_RUNTIME_INFO_STRUCT_VERSION;
    request.structSize = sizeof(request);
    if (!Ok(negotiate(&loaderInfo, &request), "runtime negotiation") || !request.getInstanceProcAddr)
        return false;
    p_xrGetInstanceProcAddr = request.getInstanceProcAddr;
    Log("[XR] using the runtime directly: %ls", full);
    return true;
}

DXGI_FORMAT FormatFamily(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    default:
        return format;
    }
}

void DestroySwapchains()
{
    for (int eye = 0; eye < 2; ++eye)
    {
        if (g_chain[eye] != XR_NULL_HANDLE) p_xrDestroySwapchain(g_chain[eye]);
        g_chain[eye] = XR_NULL_HANDLE;
        g_imageCount[eye] = 0;
    }
    g_width = g_height = 0;
}

// Eye images are the size and format family of the frame the game presents, so
// the per-frame copy is a plain GPU copy with no scaling.
bool CreateSwapchains(const D3D11_TEXTURE2D_DESC& frame)
{
    DestroySwapchains();

    uint32_t count = 0;
    int64_t formats[64] = {};
    p_xrEnumerateSwapchainFormats(g_session, 0, &count, nullptr);
    if (count > 64) count = 64;
    p_xrEnumerateSwapchainFormats(g_session, count, &count, formats);

    // The game's frame holds display-encoded colour. Asking for the sRGB
    // format of the same family tells the compositor exactly that.
    const DXGI_FORMAT family = FormatFamily(frame.Format);
    const DXGI_FORMAT preferred[2] = {
        family == DXGI_FORMAT_B8G8R8A8_TYPELESS ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        family == DXGI_FORMAT_B8G8R8A8_TYPELESS ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM };
    g_format = DXGI_FORMAT_UNKNOWN;
    for (int p = 0; p < 2 && g_format == DXGI_FORMAT_UNKNOWN; ++p)
        for (uint32_t i = 0; i < count; ++i)
            if (static_cast<DXGI_FORMAT>(formats[i]) == preferred[p]) { g_format = preferred[p]; break; }
    if (g_format == DXGI_FORMAT_UNKNOWN)
    {
        Log("[XR] runtime offers no swapchain format in the family of the game's frame (format %u)",
            static_cast<unsigned>(frame.Format));
        return false;
    }

    for (int eye = 0; eye < 2; ++eye)
    {
        XrSwapchainCreateInfo info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
        info.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        info.format = static_cast<int64_t>(g_format);
        info.sampleCount = 1;
        info.width = frame.Width;
        info.height = frame.Height;
        info.faceCount = 1;
        info.arraySize = 1;
        info.mipCount = 1;
        if (!Ok(p_xrCreateSwapchain(g_session, &info, &g_chain[eye]), "xrCreateSwapchain")) return false;

        uint32_t images = 0;
        p_xrEnumerateSwapchainImages(g_chain[eye], 0, &images, nullptr);
        if (images == 0 || images > 8) { Log("[XR] unexpected swapchain image count %u", images); return false; }
        XrSwapchainImageD3D11KHR list[8];
        for (uint32_t i = 0; i < images; ++i) list[i] = { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR };
        if (!Ok(p_xrEnumerateSwapchainImages(g_chain[eye], images, &images,
                                             reinterpret_cast<XrSwapchainImageBaseHeader*>(list)),
                "xrEnumerateSwapchainImages")) return false;
        for (uint32_t i = 0; i < images; ++i) g_images[eye][i] = list[i].texture;
        g_imageCount[eye] = images;
    }
    g_width = frame.Width;
    g_height = frame.Height;
    Log("[XR] eye images %ux%u, format %u, %u images each", g_width, g_height,
        static_cast<unsigned>(g_format), g_imageCount[0]);
    return true;
}

bool Init()
{
    if (!ResolveThroughLoader() && !ResolveThroughRuntime())
    {
        Log("[XR] no OpenXR available - running on the monitor only");
        return false;
    }
    if (!XR_SUCCEEDED(p_xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrCreateInstance",
                                              reinterpret_cast<PFN_xrVoidFunction*>(&p_xrCreateInstance))))
        return false;

    const char* extensions[] = { XR_KHR_D3D11_ENABLE_EXTENSION_NAME };
    XrInstanceCreateInfo create = { XR_TYPE_INSTANCE_CREATE_INFO };
    create.enabledExtensionCount = 1;
    create.enabledExtensionNames = extensions;
    strcpy_s(create.applicationInfo.applicationName, "Far Cry 3 VR");
    create.applicationInfo.applicationVersion = 1;
    strcpy_s(create.applicationInfo.engineName, "Dunia 2");
    // 1.0 first: a 1.0 runtime refuses a 1.1 request outright.
    const XrVersion versions[2] = { XR_API_VERSION_1_0, XR_CURRENT_API_VERSION };
    XrResult result = XR_ERROR_API_VERSION_UNSUPPORTED;
    for (int v = 0; v < 2 && !XR_SUCCEEDED(result); ++v)
    {
        create.applicationInfo.apiVersion = versions[v];
        result = p_xrCreateInstance(&create, &g_instance);
    }
    if (!Ok(result, "xrCreateInstance")) return false;

#define XR_LOAD(fn) \
    if (!XR_SUCCEEDED(p_xrGetInstanceProcAddr(g_instance, #fn, reinterpret_cast<PFN_xrVoidFunction*>(&p_##fn)))) \
    { Log("[XR] cannot resolve %s", #fn); return false; }
    XR_FUNCS(XR_LOAD)
#undef XR_LOAD
    return true;
}

// Needs the headset to be connected, so it is retried until it succeeds.
bool CreateSession(ID3D11Device* device, bool quiet)
{
    XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    const XrResult found = p_xrGetSystem(g_instance, &systemInfo, &g_system);
    if (!XR_SUCCEEDED(found))
    {
        if (!quiet) Ok(found, "xrGetSystem");
        return false;
    }

    XrGraphicsRequirementsD3D11KHR requirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
    if (!Ok(p_xrGetD3D11GraphicsRequirementsKHR(g_instance, g_system, &requirements),
            "xrGetD3D11GraphicsRequirementsKHR")) return false;

    XrGraphicsBindingD3D11KHR binding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
    binding.device = device;
    XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.next = &binding;
    sessionInfo.systemId = g_system;
    if (!Ok(p_xrCreateSession(g_instance, &sessionInfo, &g_session), "xrCreateSession")) return false;

    XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    if (!Ok(p_xrCreateReferenceSpace(g_session, &spaceInfo, &g_space), "xrCreateReferenceSpace")) return false;
    g_referenceValid = false; // A new LOCAL space needs its own reference.
    InterlockedIncrement(&g_referenceGeneration286);

    Log("[XR] session created on the game's device; waiting for the headset to be ready");
    CreateControllerActions();
    return true;
}

void PollEvents()
{
    XrEventDataBuffer event = { XR_TYPE_EVENT_DATA_BUFFER };
    while (p_xrPollEvent(g_instance, &event) == XR_SUCCESS)
    {
        if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
        {
            const auto* change = reinterpret_cast<const XrEventDataSessionStateChanged*>(&event);
            Log("[XR] session state %d", static_cast<int>(change->state));
            if (change->state == XR_SESSION_STATE_READY && !g_sessionRunning)
            {
                XrSessionBeginInfo begin = { XR_TYPE_SESSION_BEGIN_INFO };
                begin.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                if (Ok(p_xrBeginSession(g_session, &begin), "xrBeginSession"))
                {
                    g_sessionRunning = true;
                    Log("[XR] SESSION RUNNING - frames are going to the headset");
                }
            }
            else if (change->state == XR_SESSION_STATE_STOPPING && g_sessionRunning)
            {
                ClearFeedback302();PauseHandsSession303();
                p_xrEndSession(g_session);
                g_sessionRunning = false;
                g_frameBegun = false;
                Log("[XR] session stopped - monitor only");
            }
            else if (change->state == XR_SESSION_STATE_EXITING || change->state == XR_SESSION_STATE_LOSS_PENDING)
            {
                ClearFeedback302();PauseHandsSession303();
                g_sessionRunning = false;
                g_frameBegun = false;
            }
        }
        else if (event.type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED)
        {
            LogInteractionProfile();
        }
        event = { XR_TYPE_EVENT_DATA_BUFFER };
    }
}

#include "native_ui_ray298.h"
ULONGLONG g_viewsLocatedAt298=0;
#include "xr_options_menu.inl"
#include "xr_test_notes.inl"
#include "xr_reload_markers.inl"
#include "xr_hand_ui.inl"
#include "xr_selector290.inl"
// Copies the eye images in and ends the frame begun at the previous Present.
void FinishFrame(ID3D11Texture2D* leftEye, ID3D11Texture2D* rightEye,LONG renderFrame,bool captureReceipt)
{
    // Attempt identity never aliases successful-submission count after an EndFrame failure.
    static unsigned long long attempts174=0;const auto attempt174=++attempts174;
    const LONG serial174=g_submitted+1;
    ID3D11Texture2D* sources174[2]={leftEye,rightEye?rightEye:leftEye};
    ID3D11Texture2D* destinations174[2]={};UINT indices174[2]={};
    unsigned issued174=0,released174=0;
    XrCompositionLayerProjectionView views[2] = {};
    XrCompositionLayerProjection layer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    XrCompositionLayerQuad notes={XR_TYPE_COMPOSITION_LAYER_QUAD};
    XrCompositionLayerQuad reloadMarkers[21]={};
    XrCompositionLayerQuad handUi[3]={};
    XrCompositionLayerProjection selector291={};
    XrCompositionLayerProjectionView selectorViews291[2]={};
    const XrCompositionLayerBaseHeader* layers[24] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer) };
    bool submit = false;

    D3D11_TEXTURE2D_DESC frame = {};
    leftEye->GetDesc(&frame);
    if (g_shouldRender && g_viewsValid && frame.SampleDesc.Count == 1)
    {
        if (frame.Width != g_width || frame.Height != g_height)
            CreateSwapchains(frame);

        bool copied = g_width != 0;
        for (int eye = 0; eye < 2 && copied; ++eye)
        {
            uint32_t index = 0;
            XrSwapchainImageAcquireInfo acquire = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
            if (!XR_SUCCEEDED(p_xrAcquireSwapchainImage(g_chain[eye], &acquire, &index))) { copied = false; break; }
            XrSwapchainImageWaitInfo wait = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
            wait.timeout = XR_INFINITE_DURATION;
            if (XR_SUCCEEDED(p_xrWaitSwapchainImage(g_chain[eye], &wait)) && index < g_imageCount[eye]){
                destinations174[eye]=g_images[eye][index];indices174[eye]=index;
                if(fc3stereo::CopyTexture(destinations174[eye],sources174[eye]))issued174|=1u<<eye;
            }
            else
                copied = false;
            XrSwapchainImageReleaseInfo release = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            if(XR_SUCCEEDED(p_xrReleaseSwapchainImage(g_chain[eye], &release)))released174|=1u<<eye;
        }

        if (copied)
        {
            // Declared = rendered. The game drew a symmetric frustum whose
            // half-angles follow from its projection scale, from two parallel
            // eyes. Both eyes therefore share one orientation, and sit at the
            // positions the runtime reported when this frame was started.
            const float xScale = fc3stereo::MainXScale();
            const float yScale = xScale * static_cast<float>(frame.Width) / static_cast<float>(frame.Height);
            const float halfX = std::atan(1.0f / xScale);
            const float halfY = std::atan(1.0f / yScale);
            if (g_frameTracked) ++g_trackedFrames;
            for (int eye = 0; eye < 2; ++eye)
            {
                views[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
                if (g_frameTracked)
                {
                    // Rendered from exactly this eye pose with exactly this
                    // field of view, so that is what is declared.
                    views[eye].pose = g_views[eye].pose;
                    views[eye].fov = g_views[eye].fov;
                }
                else
                {
                    // Not tracked: a head-locked picture with the game's own view.
                    views[eye].pose.orientation = g_views[0].pose.orientation;
                    views[eye].pose.position = g_views[eye].pose.position;
                    views[eye].fov.angleLeft = -halfX;
                    views[eye].fov.angleRight = halfX;
                    views[eye].fov.angleUp = halfY;
                    views[eye].fov.angleDown = -halfY;
                }
                views[eye].subImage.swapchain = g_chain[eye];
                views[eye].subImage.imageRect.extent = { static_cast<int32_t>(g_width), static_cast<int32_t>(g_height) };
            }
            layer.space = g_space;
            layer.viewCount = 2;
            layer.views = views;
            submit = true;

            const LONG count = g_submitted + 1;
            static bool lastMenu=false;static ULONGLONG xrProbeTick=0;
            const auto now=GetTickCount64();const bool menu=fc3pad::MenuOpen();
            if(now-xrProbeTick>=250||lastMenu!=menu)
            {
                xrProbeTick=now;lastMenu=menu;char text[1200]={};
                _snprintf_s(text,sizeof(text),_TRUNCATE,"xr_submit,%llu,%ld,insert=%d,renderedSize=%ux%u,tracked=%d,referenceValid=%d,referenceYaw=%.6f,declaredFov=%.6f/%.6f/%.6f/%.6f,headPosition=%.6f/%.6f/%.6f,headQuaternion=%.6f/%.6f/%.6f/%.6f,requestedEyeView=%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f\r\n",now,count,menu,g_width,g_height,g_frameTracked,g_referenceValid,g_referenceYaw,views[0].fov.angleLeft,views[0].fov.angleRight,views[0].fov.angleUp,views[0].fov.angleDown,views[0].pose.position.x,views[0].pose.position.y,views[0].pose.position.z,views[0].pose.orientation.x,views[0].pose.orientation.y,views[0].pose.orientation.z,views[0].pose.orientation.w,g_probeEyes[0].view[0],g_probeEyes[0].view[1],g_probeEyes[0].view[2],g_probeEyes[0].view[3],g_probeEyes[0].view[4],g_probeEyes[0].view[5],g_probeEyes[0].view[6],g_probeEyes[0].view[7],g_probeEyes[0].view[8],g_probeEyes[0].view[9],g_probeEyes[0].view[10],g_probeEyes[0].view[11]);
                fc3stereo::WriteRenderProbe(text);
            }

            if (count == 1 || count % 1800 == 0)
                Log("[XR] frame %ld: declared field of view %.1f x %.1f degrees, eye distance %.1f mm, "
                    "empty frames %ld, failures %ld, head-tracked frames %ld",
                    count, (views[0].fov.angleRight-views[0].fov.angleLeft)*57.29578f,
                    (views[0].fov.angleUp-views[0].fov.angleDown)*57.29578f,
                    fc3stereo::EyeSeparation() * 2000.0f, g_emptyFrames, g_failures, g_trackedFrames);
        }
    }

    XrFrameEndInfo end = { XR_TYPE_FRAME_END_INFO };
    end.displayTime = g_displayTime;
    end.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    end.layerCount = submit ? 1 : 0;
    if(submit&&NotesLayer(notes,leftEye)){layers[1]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&notes);end.layerCount=2;}
    if(submit&&HandUiLayers(handUi))for(int i=0;i<3;++i)if(handUi[i].subImage.swapchain)layers[end.layerCount++]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&handUi[i]);
    if(submit&&SelectorLayer291(selector291,selectorViews291,leftEye))layers[end.layerCount++]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&selector291);
    if(submit&&ReloadMarkerLayers(reloadMarkers,leftEye))
    {   for(int i=19;i<21&&end.layerCount<16;++i)if(GuideMarkerVisible(i))layers[end.layerCount++]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&reloadMarkers[i]);
        for(int i=0;i<19&&end.layerCount<16;++i)if(GuideMarkerVisible(i))layers[end.layerCount++]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&reloadMarkers[i]);
    }
    end.layers = submit ? layers : nullptr;
    const XrResult endResult174=p_xrEndFrame(g_session, &end);
    if (XR_SUCCEEDED(endResult174))
    {
        if (submit) ++g_submitted; else ++g_emptyFrames;
    }
    else
    {
        ++g_failures;
    }
    static ULONGLONG receiptAt174=0;static unsigned receipts174=0,captureReceipts175=0;
    const auto receiptNow174=GetTickCount64();
    // Preserve a separate budget for late automatic captures after ordinary sampling ends.
    if(renderFrame>0&&(captureReceipt?captureReceipts175<32:receipts174<1024)&&(captureReceipt||receiptNow174-receiptAt174>=250)){
        receiptAt174=receiptNow174;if(captureReceipt)++captureReceipts175;else ++receipts174;char text[640]={};
        _snprintf_s(text,sizeof(text),_TRUNCATE,"xr_copy174,%llu,%ld,attempt=%llu,serial=%ld,src0=%p,src1=%p,dst0=%p,dst1=%p,index0=%u,index1=%u,issuedMask=%u,releasedMask=%u,projection=%d,endResult=%d,displayTime=%lld,capture=%d\r\n",
            receiptNow174,renderFrame,attempt174,serial174,sources174[0],sources174[1],destinations174[0],destinations174[1],indices174[0],indices174[1],issued174,released174,submit,int(endResult174),g_displayTime,captureReceipt);
        fc3stereo::WriteRenderProbe(text);
    }
    g_frameBegun = false;
}

// Starts the next frame and records where the eyes will be for it.
void StartFrame()
{
    PaceBeforeWait();
    XrFrameState state = { XR_TYPE_FRAME_STATE };
    XrFrameWaitInfo wait = { XR_TYPE_FRAME_WAIT_INFO };
    if (!XR_SUCCEEDED(p_xrWaitFrame(g_session, &wait, &state))) { ++g_failures; return; }
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (g_lastWaitReturn != 0 && g_periodNs > 0 && g_qpcFrequency.QuadPart)
        {
            const double periods = QpcMs(now.QuadPart - g_lastWaitReturn) * 1.0e6 / static_cast<double>(g_periodNs);
            int bucket = static_cast<int>(periods + 0.5);
            if (bucket < 0) bucket = 0;
            if (bucket > 7) bucket = 7;
            ++g_paceHistogram[bucket];
            static int reported = 0;
            if (++reported % 900 == 0)
            {
                Log("[PACE] refresh %.1f Hz, holding %d period(s); frames shown for 1/2/3/4/5+ periods: "
                    "%ld / %ld / %ld / %ld / %ld",
                    1.0e9 / static_cast<double>(g_periodNs), g_paceNow, g_paceHistogram[1], g_paceHistogram[2],
                    g_paceHistogram[3], g_paceHistogram[4],
                    g_paceHistogram[5] + g_paceHistogram[6] + g_paceHistogram[7]);
                std::memset(g_paceHistogram, 0, sizeof(g_paceHistogram));
            }
        }
        g_lastWaitReturn = now.QuadPart;
        g_periodNs = state.predictedDisplayPeriod;
    }
    XrFrameBeginInfo begin = { XR_TYPE_FRAME_BEGIN_INFO };
    if (!XR_SUCCEEDED(p_xrBeginFrame(g_session, &begin))) { ++g_failures; return; }
    g_frameBegun = true;
    g_displayTime = state.predictedDisplayTime;
    g_shouldRender = state.shouldRender == XR_TRUE;

    uint32_t count = 0;
    XrViewState viewState = { XR_TYPE_VIEW_STATE };
    XrViewLocateInfo locate = { XR_TYPE_VIEW_LOCATE_INFO };
    locate.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    locate.displayTime = g_displayTime;
    locate.space = g_space;
    g_views[0] = { XR_TYPE_VIEW };
    g_views[1] = { XR_TYPE_VIEW };
    g_viewsValid = XR_SUCCEEDED(p_xrLocateViews(g_session, &locate, &viewState, 2, &count, g_views)) &&
        count == 2 && (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) &&
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT);
    // "valid" can be a placeholder pose before the headset is on a head; the
    // reference for head tracking is only taken from a really tracked pose.
    g_viewsTracked = g_viewsValid && (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_TRACKED_BIT) &&
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_TRACKED_BIT);
    g_viewsLocatedAt298=GetTickCount64();
    SyncControllers();

    // The headset's field of view: what the game's own view must cover before
    // the picture can be drawn from the head pose.
    if (g_viewsValid)
    {
        float needX = 0, needY = 0;
        for (int eye = 0; eye < 2; ++eye)
        {
            const XrFovf& fov = g_views[eye].fov;
            const float extremes[4] = { std::tan(-fov.angleLeft), std::tan(fov.angleRight),
                                        std::tan(fov.angleUp), std::tan(-fov.angleDown) };
            if (extremes[0] > needX) needX = extremes[0];
            if (extremes[1] > needX) needX = extremes[1];
            if (extremes[2] > needY) needY = extremes[2];
            if (extremes[3] > needY) needY = extremes[3];
        }
        fc3fov::SetNeed(needX, needY);
    }

    // Source-camera coverage controls culling margin, not the headset projection
    // or tracking reference. A doorway's temporary zoom must not switch the
    // submitted eye frustum or re-anchor the body to the gun camera.
    const bool track = g_viewsValid && g_viewsTracked && fc3stereo::HeadTrackingEnabled() &&
                       fc3stereo::InWorldView();
    if (!track)
    {
        if (g_frameTracked) Log("[XR] head tracking paused (menu, loading, or headset tracking unavailable)");
        g_frameTracked = false;
        // A tracking/menu/visibility gap is not a recenter. Preserve the
        // reference; explicit Recenter remains authoritative.
        fc3stereo::SetEyeViews(nullptr);
        fc3stereo::SetHandPoses(nullptr, nullptr);
        if (g_viewsValid)
        {
            // Head-locked: only the eye distance comes from the headset.
            const float dx = g_views[1].pose.position.x - g_views[0].pose.position.x;
            const float dy = g_views[1].pose.position.y - g_views[0].pose.position.y;
            const float dz = g_views[1].pose.position.z - g_views[0].pose.position.z;
            const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (distance > 0.04f && distance < 0.09f)
                fc3stereo::SetEyeSeparation(distance * 0.5f);
        }
        return;
    }

    // Reference: head yaw and head position that count as "the game camera".
    const XrQuaternionf& headQ = g_views[0].pose.orientation;
    const XrVector3f centre = {
        (g_views[0].pose.position.x + g_views[1].pose.position.x) * 0.5f,
        (g_views[0].pose.position.y + g_views[1].pose.position.y) * 0.5f,
        (g_views[0].pose.position.z + g_views[1].pose.position.z) * 0.5f };
    if (!g_referenceValid)
    {
        const float fx = -(2.0f * (headQ.x * headQ.z + headQ.w * headQ.y));
        const float fz = -(1.0f - 2.0f * (headQ.x * headQ.x + headQ.y * headQ.y));
        g_referenceYaw = std::atan2(-fx, -fz);
        g_referencePosition = centre;
        g_referenceValid = true;
        Log("[XR] head tracking engaged; reference yaw %.1f degrees, head height %.2f m",
            g_referenceYaw * 57.29578f, centre.y);
    }

    const float scale = fc3stereo::WorldScale();
    const float cy = std::cos(-g_referenceYaw), sy = std::sin(-g_referenceYaw);
    // inverse reference yaw as a quaternion about +Y
    const float ry = std::sin(-g_referenceYaw * 0.5f), rw = std::cos(-g_referenceYaw * 0.5f);
    fc3stereo::EyeView eyes[2] = {};
    float needX = 0, needY = 0;
    for (int eye = 0; eye < 2; ++eye)
    {
        const XrQuaternionf& q = g_views[eye].pose.orientation;
        // relative orientation = inverseReferenceYaw * q
        const float x = rw * q.x + ry * q.z;
        const float y = rw * q.y + ry * q.w;
        const float z = rw * q.z - ry * q.x;
        const float w = rw * q.w - ry * q.y;
        // rotation matrix of the eye in the body frame (OpenXR axes)
        const float r[3][3] = {
            { 1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y) },
            { 2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x) },
            { 2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y) } };
        // eye position in the body frame (OpenXR axes), in game units
        const float px = g_views[eye].pose.position.x - g_referencePosition.x;
        const float py = g_views[eye].pose.position.y - g_referencePosition.y;
        const float pz = g_views[eye].pose.position.z - g_referencePosition.z;
        const float t[3] = { (px * cy + pz * sy) * scale, py * scale, (-px * sy + pz * cy) * scale };

        // Game view space is x right, y up, z FORWARD; OpenXR's is z backward.
        // view = S * transpose(r) * S, translation = -S * transpose(r) * t, S = diag(1,1,-1)
        const float flip[3] = { 1.0f, 1.0f, -1.0f };
        float* m = eyes[eye].view;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
                m[i * 4 + j] = flip[i] * flip[j] * r[j][i];
            m[i * 4 + 3] = -flip[i] * (r[0][i] * t[0] + r[1][i] * t[1] + r[2][i] * t[2]);
        }
        m[15] = 1.0f;

        const XrFovf& fov = g_views[eye].fov;
        eyes[eye].tanLeft = std::tan(fov.angleLeft);
        eyes[eye].tanRight = std::tan(fov.angleRight);
        eyes[eye].tanUp = std::tan(fov.angleUp);
        eyes[eye].tanDown = std::tan(fov.angleDown);
        const float extremes[4] = { -eyes[eye].tanLeft, eyes[eye].tanRight, eyes[eye].tanUp, -eyes[eye].tanDown };
        if (extremes[0] > needX) needX = extremes[0];
        if (extremes[1] > needX) needX = extremes[1];
        if (extremes[2] > needY) needY = extremes[2];
        if (extremes[3] > needY) needY = extremes[3];
    }
    memcpy(g_probeEyes,eyes,sizeof(eyes));
    fc3stereo::SetEyeViews(eyes);
    fc3fov::SetNeed(needX, needY);
    g_frameTracked = true;

    // Controllers, located for the same display time and expressed the same
    // way: relative to the reference, in game view axes (z forward = where the
    // controller points).
    float hands[2][16] = {};
    bool handValid[2] = {};
    for (int hand = 0; hand < 2 && g_actionsReady; ++hand)
    {
        if (g_handSpaces[hand] == XR_NULL_HANDLE) continue;
        XrSpaceLocation location = { XR_TYPE_SPACE_LOCATION };
        if (!XR_SUCCEEDED(p_xrLocateSpace(g_handSpaces[hand], g_space, g_displayTime, &location))) continue;
        if (!fc3xr::TrackedHandLocation(location)) continue;
        const XrQuaternionf& q = location.pose.orientation;
        const float x = rw * q.x + ry * q.z;
        const float y = rw * q.y + ry * q.w;
        const float z = rw * q.z - ry * q.x;
        const float w = rw * q.w - ry * q.y;
        const float r[3][3] = {
            { 1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y) },
            { 2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x) },
            { 2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y) } };
        const float px = location.pose.position.x - g_referencePosition.x;
        const float py = location.pose.position.y - g_referencePosition.y;
        const float pz = location.pose.position.z - g_referencePosition.z;
        const float t[3] = { (px * cy + pz * sy) * scale, py * scale, (-px * sy + pz * cy) * scale };
        const float flip[3] = { 1.0f, 1.0f, -1.0f };
        float* m = hands[hand];
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j) m[i * 4 + j] = flip[i] * flip[j] * r[i][j];
            m[i * 4 + 3] = flip[i] * t[i];
        }
        m[15] = 1.0f;
        handValid[hand] = true;
    }
    static int previousHands = -1;
    const int trackedHands = (handValid[0] ? 1 : 0) | (handValid[1] ? 2 : 0);
    if (trackedHands != previousHands)
    {
        Log("[XR HANDS] right %s, left %s; untracked controller poses are withheld",
            handValid[0] ? "tracked" : "untracked", handValid[1] ? "tracked" : "untracked");
        previousHands = trackedHands;
    }
    fc3stereo::SetHandPoses(handValid[0] ? hands[0] : nullptr, handValid[1] ? hands[1] : nullptr);
    static bool reported = false;
    if (handValid[0] && !reported)
    {
        reported = true;
        Log("[XR] right controller tracked: %.2f %.2f %.2f m from the head reference (game axes)",
            hands[0][3], hands[0][7], hands[0][11]);
    }
}
}

namespace fc3hands303 {
Transfer307 TransferContact307(){AcquireSRWLockShared(&g_handsLock303);auto t=g_transfer307;ReleaseSRWLockShared(&g_handsLock303);if(!t.at||GetTickCount64()-t.at>120)t.contact=t.allowed=false;return t;}
void PublishTransfer307(bool enabled,bool contact,bool allowed,unsigned generation,unsigned entity,unsigned epoch,bool rearOnly316,bool modifier321){Transfer307 t;t.modifier321=modifier321;t.rearOnly316=rearOnly316;t.enabled=enabled;t.contact=contact;t.allowed=allowed;t.generation=generation;t.entity=entity;t.epoch=epoch;t.reference=fc3xr::ReferenceGeneration286();t.at=GetTickCount64();AcquireSRWLockExclusive(&g_handsLock303);g_transfer307=t;ReleaseSRWLockExclusive(&g_handsLock303);}
Snapshot Read(){AcquireSRWLockShared(&g_handsLock303);auto s=g_handsSnapshot303;ReleaseSRWLockShared(&g_handsLock303);const auto now=GetTickCount64();if(s.at&&now-s.at>150)s.active=false;
 if(!s.confirmed314||now<s.confirmed314||now-s.confirmed314>1500)s.presentation314=false;return s;}
Snapshot ReadPresentation314(){auto s=Read();s.active=s.active||s.presentation314;s.engaged=s.engaged||s.presentation314;return s;}
void AdjustRolePose(int role,float* pose){
 if(!pose||role<0||role>1)return;const auto held=Read();const int physical=PhysicalHand(held,role);if(physical==role)return;
 float actual[3],canonical[3];AcquireSRWLockShared(&g_handsLock303);const bool valid=g_handPalmEpoch303[role]==g_roleEpoch303&&g_handPalmEpoch303[physical]==g_roleEpoch303;memcpy(actual,g_handPalm303[physical],12);memcpy(canonical,g_handPalm303[role],12);ReleaseSRWLockShared(&g_handsLock303);if(!valid)return;
 for(int row=0;row<3;++row)for(int col=0;col<3;++col)pose[row*4+3]+=pose[row*4+col]*(actual[col]-canonical[col]);
}
Settings Configuration(){LoadHands303();return g_handSettings303;}
bool Configure(const Settings& settings,const wchar_t* explicitPath){
 wchar_t path[MAX_PATH];if(explicitPath)wcsncpy_s(path,explicitPath,_TRUNCATE);else _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_stereo.ini",fc3::DataFolder());
 const bool a=WritePrivateProfileStringW(L"WeaponHands",L"Enabled",settings.enabled?L"1":L"0",path)!=FALSE;
 const bool b=WritePrivateProfileStringW(L"WeaponHands",L"RequireHolster",settings.requireHolster?L"1":L"0",path)!=FALSE;
 g_handSettings303=settings;g_handsLoaded303=true;g_hands303={};g_handGeneration303=~0u;g_handTriggerWait303=true;g_single303=false;g_selectGesture290.Close();g_pick292.Cancel();g_bodyRenderFailed297=false;PublishHands303(false);return a&&b;
}
void RawControls(float& rightGrip,float& leftGrip,float& rightTrigger,float& leftTrigger){AcquireSRWLockShared(&g_handsLock303);rightGrip=g_rawHands303.rightGrip;leftGrip=g_rawHands303.leftGrip;rightTrigger=g_rawHands303.rightTrigger;leftTrigger=g_rawHands303.leftTrigger;ReleaseSRWLockShared(&g_handsLock303);}
}
namespace fc3xr
{
void SetHandPalm303(int hand,const float* point,unsigned epoch){if(hand<0||hand>1||!point)return;for(int i=0;i<3;++i)if(!std::isfinite(point[i])||fabs(point[i])>2)return;AcquireSRWLockExclusive(&g_handsLock303);memcpy(g_handPalm303[hand],point,12);g_handPalmEpoch303[hand]=epoch;ReleaseSRWLockExclusive(&g_handsLock303);}
void SetHandsContext303(bool active,bool canChange,unsigned epoch,int nativeHoldingSide,bool presentationOnly326){g_presentationContext326=presentationOnly326;if(g_nativeHoldingSide303!=nativeHoldingSide){g_nativeHoldingSide303=nativeHoldingSide;++g_hands303.generation;g_hands303.receiver=-1;}g_roleContext303=active;g_changeContext303=canChange;g_roleAt303=GetTickCount64();g_roleEpoch303=epoch;if(!active&&!presentationOnly326){AcquireSRWLockExclusive(&g_handsLock303);g_handsSnapshot303.active=g_handsSnapshot303.engaged=g_handsSnapshot303.presentation314=g_handsSnapshot303.ownerContext326=false;ReleaseSRWLockExclusive(&g_handsLock303);}}
fc3holster297::Settings WeaponHolsterSettings297(){LoadHolsters297();return g_holsters297;}
bool CaptureHolsterPosition345(fc3holster297::Settings& settings,int slot,int hand){
 if(slot<0||slot>3||hand<0||hand>1||!g_sessionRunning||!g_referenceValid||!g_viewsTracked||!g_handSpaces[hand]||!p_xrLocateSpace)return false;
 XrSpaceLocation loc={XR_TYPE_SPACE_LOCATION};if(!XR_SUCCEEDED(p_xrLocateSpace(g_handSpaces[hand],g_space,g_displayTime,&loc))||!TrackedHandLocation(loc))return false;
 XrQuaternionf basis=g_selectPose290.orientation;if(!g_bodyPlaced297){const auto forward=testnotes::Rotate(g_views[0].pose.orientation,{0,0,-1});const float yaw=std::atan2(-forward.x,-forward.z);basis={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};}
 const XrVector3f head={(g_views[0].pose.position.x+g_views[1].pose.position.x)*.5f,(g_views[0].pose.position.y+g_views[1].pose.position.y)*.5f,(g_views[0].pose.position.z+g_views[1].pose.position.z)*.5f};
 const auto point=testnotes::Rotate(testnotes::Inverse(basis),testnotes::Sub(Palm303(loc.pose,hand),head));const fc3holster297::Point local={point.x,point.y-settings.height,point.z};if(!fc3holster297::ValidPoint(local))return false;
 g_selectPose290.position=head;g_selectPose290.orientation=basis;g_bodyPlaced297=true;g_anchorAt297=GetTickCount64();
 settings.positions[slot]=local;settings.custom[slot]=true;settings.guides=true;return true;
}
bool ConfigureWeaponHolsters297(const fc3holster297::Settings& settings,const wchar_t* explicitPath){
 if(!fc3holster297::Valid(settings)||(g_selectGesture290.open&&(g_warm292.inflight||g_pick292.requested)))return false;
 const bool preserveAnchor305=g_bodyPlaced297&&g_holsters297.mode!=fc3holster297::Front&&settings.mode!=fc3holster297::Front;
 g_selectGesture290.Close();g_pick292.Cancel();g_singleHeld297=g_stickWait297=false;g_bodyVisible297=g_bodyPlaced297=g_bodyReady297=g_bodyRenderFailed297=false;g_bodyGripWas297=true;g_bodyLast297=0;fc3visual292::ClearContact();fc3select290::KeepAlive(false);fc3pad::SetSelector290(false);
 g_bodyPlaced297=preserveAnchor305;if(preserveAnchor305)g_anchorAt297=GetTickCount64();
 g_zonesVisible304=g_stowReady304=g_stowCandidate304=false;g_holsters297=settings;g_holstersLoaded297=true;wchar_t path[MAX_PATH];if(explicitPath)wcsncpy_s(path,explicitPath,_TRUNCATE);else _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_stereo.ini",fc3::DataFolder());
 wchar_t mode[16],height[32],radius[32];swprintf_s(mode,L"%d",settings.mode);swprintf_s(height,L"%.4f",settings.height);swprintf_s(radius,L"%.4f",settings.radius);
 const bool a=WritePrivateProfileStringW(L"WeaponHolsters",L"Mode",mode,path)!=FALSE,b=WritePrivateProfileStringW(L"WeaponHolsters",L"HeightOffset",height,path)!=FALSE,c=WritePrivateProfileStringW(L"WeaponHolsters",L"GrabRadius",radius,path)!=FALSE;bool saved=a&&b&&c;
 saved=(WritePrivateProfileStringW(L"WeaponHolsters",L"ShowZones",settings.guides?L"1":L"0",path)!=FALSE)&&saved;
 saved=(WritePrivateProfileStringW(L"WeaponHolsters",L"StowOutline",settings.stowOutline?L"1":L"0",path)!=FALSE)&&saved;
 for(int i=0;i<4;++i){wchar_t key[32],value[128];swprintf_s(key,L"Slot%d",i+1);const auto point=settings.positions[i];swprintf_s(value,L"%.4f,%.4f,%.4f",point.x,point.y,point.z);saved=(WritePrivateProfileStringW(L"WeaponHolsters",key,settings.custom[i]?value:nullptr,path)!=FALSE)&&saved;}
 return saved;
}
bool OptionsMenuRay299(float width,float height,bool captured,float* uv){
 if(!uv||!g_optionsTexture||!g_sessionRunning||!g_viewsValid||!g_viewsTracked||!g_actionsReady||!g_handSpaces[0]||!p_xrLocateSpace||!g_viewsLocatedAt298||GetTickCount64()-g_viewsLocatedAt298>120)return false;
 D3D11_TEXTURE2D_DESC td={};g_optionsTexture->GetDesc(&td);if(width!=td.Width||height!=td.Height)return false;
 XrSpaceLocation location={XR_TYPE_SPACE_LOCATION};if(!XR_SUCCEEDED(p_xrLocateSpace(g_handSpaces[0],g_space,g_displayTime,&location))||!TrackedHandLocation(location))return false;
 const auto inverse=testnotes::Inverse(g_views[0].pose.orientation);const auto origin=testnotes::Rotate(inverse,testnotes::Sub(location.pose.position,g_views[0].pose.position));
 const auto direction=testnotes::Rotate(inverse,testnotes::Rotate(location.pose.orientation,{0,0,-1}));
 return fc3ui::OptionsHit299({origin.x,origin.y,origin.z},{direction.x,direction.y,direction.z},width,height,g_optionsCrop.x,g_optionsCrop.y,g_optionsCrop.width,g_optionsCrop.height,captured,uv[0],uv[1]);
}
bool NativeMenuRay298(float depth,float aspect,float* screenUv4){
 if(!screenUv4||!g_sessionRunning||!g_viewsValid||!g_viewsTracked||!g_actionsReady||!g_handSpaces[0]||!p_xrLocateSpace||!g_viewsLocatedAt298||GetTickCount64()-g_viewsLocatedAt298>120||!std::isfinite(aspect)||aspect<=0)return false;
 XrSpaceLocation location={XR_TYPE_SPACE_LOCATION};if(!XR_SUCCEEDED(p_xrLocateSpace(g_handSpaces[0],g_space,g_displayTime,&location))||!TrackedHandLocation(location))return false;
 const auto direction=testnotes::Rotate(location.pose.orientation,{0,0,-1});float uv[4];
 for(int eye=0;eye<2;++eye){const auto inverse=testnotes::Inverse(g_frameTracked?g_views[eye].pose.orientation:g_views[0].pose.orientation);const auto origin=testnotes::Rotate(inverse,testnotes::Sub(location.pose.position,g_views[eye].pose.position)),ray=testnotes::Rotate(inverse,direction);
  float left,right,down,up;if(g_frameTracked){const auto fov=g_views[eye].fov;left=std::tan(fov.angleLeft);right=std::tan(fov.angleRight);down=std::tan(fov.angleDown);up=std::tan(fov.angleUp);}else{const float scale=fc3stereo::MainXScale();if(!std::isfinite(scale)||scale<=0)return false;right=1/scale;left=-right;up=aspect/scale;down=-up;}
  if(!fc3ui::ProjectRay298({origin.x,origin.y,origin.z},{ray.x,ray.y,ray.z},depth,left,right,down,up,uv[eye*2],uv[eye*2+1]))return false;
 }memcpy(screenUv4,uv,sizeof(uv));return true;
}
bool SelectorOpen291(){return g_selectGesture290.open;}
bool WeaponHolsterWristNeeded297(){return g_holsters297.mode!=fc3holster297::Front&&g_selectAllowed290;}
void SetSelectorWrist292(const float* body){PublishSelectorWrist293(body);}
void SetSelectorAllowed290(bool allowed){g_selectAllowed290=allowed;g_selectAllowedAt290=GetTickCount64();}
int FeedbackStrength302(){LoadFeedback302();return int(InterlockedCompareExchange(&g_feedbackStrength302,0,0));}
int GunfireStrength339(){LoadFeedback302();return int(InterlockedCompareExchange(&g_gunfireStrength339,0,0));}
bool ConfigureGunfire339(int percent,const wchar_t* explicitPath){
 if(percent<0||percent>100)return false;
 wchar_t path[MAX_PATH],text[16];if(explicitPath)wcsncpy_s(path,explicitPath,_TRUNCATE);else _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_stereo.ini",fc3::DataFolder());
 swprintf_s(text,L"%d",percent);const bool saved=WritePrivateProfileStringW(L"Haptics",L"GunfireStrength",text,path)!=FALSE;
 InterlockedExchange(&g_gunfireStrength339,percent);ClearFeedback302();return saved;
}
void Gunshot339(unsigned entity,int profile){
 if(InterlockedCompareExchange(&g_gunfireStrength339,0,0)<=0)return;
 const auto owner=fc3hands303::Read();
 if(g_gunfire339.Request(entity,profile,owner,GetTickCount64())){
  static volatile LONG receipts=0;if(InterlockedIncrement(&receipts)<=12)Log("[GUNFIRE339] native shot profile%d entity%08X physicalHand%d generation%u",profile,entity,owner.hand,owner.generation);
 }
}
bool ConfigureFeedback302(int percent,const wchar_t* explicitPath){
 if(percent<0||percent>100)return false;
 wchar_t path[MAX_PATH],text[16];if(explicitPath)wcsncpy_s(path,explicitPath,_TRUNCATE);else _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_stereo.ini",fc3::DataFolder());
 swprintf_s(text,L"%d",percent);const bool saved=WritePrivateProfileStringW(L"Haptics",L"Strength",text,path)!=FALSE;
 InterlockedExchange(&g_feedbackStrength302,percent);g_feedbackLoaded302=true;ClearFeedback302();return saved;
}
unsigned ReferenceGeneration286(){return static_cast<unsigned>(InterlockedCompareExchange(&g_referenceGeneration286,0,0));}
void InteractionPulse285(int hand){if(hand>=0&&hand<2&&g_feedback302.Accepting(GetTickCount()))InterlockedOr(&g_interactionPulse285,1<<hand);}
void SetHandUi(ID3D11Texture2D* texture,const float* wrist,const fc3handui::Settings& settings,bool wheel)
{
 if(texture)texture->AddRef();if(g_nativeHandUi)g_nativeHandUi->Release();g_nativeHandUi=texture;g_nativeHandSettings=settings;g_nativeHandWheel=wheel;g_nativeHandValid=wrist&&g_referenceValid;
 if(!g_nativeHandValid)return;for(int k=0;k<16;++k)if(!std::isfinite(wrist[k])){g_nativeHandValid=false;return;}
 g_nativeHandValid=BodyToReferencePose(wrist,(std::max)(.1f,fc3stereo::WorldScale()),g_referenceYaw,g_referencePosition,g_nativeHandPose);
 if(!g_nativeHandValid){static unsigned rejected=0;if(rejected++<8)Log("[HANDUI] wrist pose rejected basis=%f/%f/%f;%f/%f/%f;%f/%f/%f",wrist[0],wrist[1],wrist[2],wrist[4],wrist[5],wrist[6],wrist[8],wrist[9],wrist[10]);}
}
bool HandUiPoseForTest(int panel,float* values){if(!values||panel<0||panel>2||!g_nativeHandValid||!fc3handui::Valid(g_nativeHandSettings.panels[panel]))return false;const auto p=HandUiPanelPose(panel);values[0]=p.position.x;values[1]=p.position.y;values[2]=p.position.z;values[3]=p.orientation.x;values[4]=p.orientation.y;values[5]=p.orientation.z;values[6]=p.orientation.w;return true;}
bool HandUiReady(){return g_handUiCopyReady&&g_sessionRunning&&g_referenceValid;}
void SetOptionsTexture(ID3D11Texture2D* texture,int x,int y,int width,int height)
{if(texture)texture->AddRef();if(g_optionsTexture)g_optionsTexture->Release();g_optionsTexture=texture;g_optionsCrop={x,y,width,height};}

bool Running()
{
    return g_sessionRunning;
}

void MacheteHit(){if(g_feedback302.Accepting(GetTickCount()))g_macheteHit121.Request(GetTickCount());}

void Buzz(int count)
{
    g_feedback302.Request(count,GetTickCount());
}
BodyReloadGuides51 g_bodyReloadGuides51;
void SetReloadMarkers(const float* port,const float* shell,float radius,int contact,float handDiameter)
{g_bodyReloadGuides51={};g_bodyReloadGuides51.point=port&&std::isfinite(radius)&&radius>0;g_bodyReloadGuides51.hand=g_bodyReloadGuides51.point&&shell;if(g_bodyReloadGuides51.point)memcpy(g_bodyReloadGuides51.port,port,12);if(g_bodyReloadGuides51.hand)memcpy(g_bodyReloadGuides51.palm,shell,12);g_bodyReloadGuides51.radius=radius;g_bodyReloadGuides51.handDiameter=std::isfinite(handDiameter)&&handDiameter>0?handDiameter:.014f;ReloadMarkerPoints(port,shell,radius,contact,handDiameter);}
void SetHandSphere(){ReloadMarkerSphere();}
void HideReloadMarkers(){for(int i=0;i<10;++i)g_reloadMarkerVisible[i]=false;}
void HideGrabMarkers(){for(int i=10;i<19;++i)g_reloadMarkerVisible[i]=false;}
void HideReloadWeaponMarker(){g_reloadMarkerVisible[0]=false;for(int i=2;i<8;++i)g_reloadMarkerVisible[i]=false;}
void SetDoorHighlight363(const float* frame,const float* handle,float width,float height,bool held){
 g_reloadMarkerVisible[19]=g_reloadMarkerVisible[20]=false;
 const float scale=(std::max)(.1f,fc3stereo::WorldScale());
 if(!g_referenceValid||!frame||!handle||!std::isfinite(width)||!std::isfinite(height)||width<.2f||height<.2f||width>3||height>3)return;
 XrPosef pose[2];if(!BodyBoxToReferencePose(frame,scale,g_referenceYaw,g_referencePosition,pose[0])||!BodyBoxToReferencePose(handle,scale,g_referenceYaw,g_referencePosition,pose[1]))return;
 for(int i=0;i<2;++i){if(g_viewsValid){const auto normal=testnotes::Rotate(pose[i].orientation,{0,0,1}),toHead=testnotes::Sub(g_views[0].pose.position,pose[i].position);if(normal.x*toHead.x+normal.y*toHead.y+normal.z*toHead.z<0)pose[i].orientation=testnotes::Multiply(pose[i].orientation,{0,1,0,0});}g_reloadMarkerPose[19+i]=pose[i];g_reloadMarkerVisible[19+i]=true;}
 g_doorHighlightSize363[0]={width/scale,height/scale};g_doorHighlightSize363[1]={.19f,.12f};g_doorHighlightHeld363=held;g_doorHighlightAt363=GetTickCount64();g_doorHighlightReference363=ReferenceGeneration286();
}
void SetGrabMarkers(const float* point,const float* palm,bool inReach,float pointDiameter,float palmDiameter)
{GrabMarkerPoints(point,palm,inReach,pointDiameter,palmDiameter);}
void SetGuideGroupVisibility(bool weaponReload,bool twoHand,bool insertOpen){g_weaponReloadGroup=weaponReload;g_twoHandGroup=twoHand;g_guideInsertOpen=insertOpen;}
void SetWeaponBox(const float* transform,const float* dimensions,int contact)
{if(g_bodyReloadGuides51.point&&transform&&dimensions){g_bodyReloadGuides51.box=true;memcpy(g_bodyReloadGuides51.transform,transform,64);memcpy(g_bodyReloadGuides51.dimensions,dimensions,12);}ReloadMarkerBox(transform,dimensions,contact);}
BodyReloadGuides51 BodyReloadGuideSnapshot51(){return g_bodyReloadGuides51;}
bool ReloadMarkerBoxVisible(){for(int i=2;i<8;++i)if(GuideMarkerVisible(i))return true;return false;}
bool GuideLayersVisibleForTest(){return AnyGuideVisible();}
bool GuideBoxAtlasForTest(){return ReloadBoxFaceVisible();}
void GuideFacesForTest(unsigned mask){for(int i=0;i<19;++i)g_reloadMarkerVisible[i]=(mask&(1u<<i))!=0;}
void ForceReferenceValid(bool on){g_referenceValid=on;}
void ForceReferenceFrameForTest(float yaw,const float* position){g_referenceYaw=yaw;g_referencePosition=position?XrVector3f{position[0],position[1],position[2]}:XrVector3f{};g_referenceValid=true;}
bool MarkerPoseForTest(int index,float* out){if(!out||!GuideMarkerVisible(index))return false;const auto& p=g_reloadMarkerPose[index];out[0]=p.position.x;out[1]=p.position.y;out[2]=p.position.z;out[3]=p.orientation.x;out[4]=p.orientation.y;out[5]=p.orientation.z;out[6]=p.orientation.w;return true;}
void SetOptionsNote(const wchar_t* status)
{g_noteOptions=status!=nullptr;if(status){wcsncpy_s(g_optionStatus,status,_TRUNCATE);g_noteVisible=true;g_noteDragging=false;}g_noteDirty=true;}
bool g_weaponNotesContext303=false;
void SetWeaponNotesContext303(bool active){g_weaponNotesContext303=active;}
void SetTestNote(const wchar_t* status)
{
    std::wstring physical303;if(status&&g_weaponNotesContext303&&fc3hands303::PhysicalHand(fc3hands303::Read(),0)==1){const auto held308=fc3hands303::Read();const auto transfer308=fc3hands303::TransferContact307();physical303=fc3hands303::WeaponNote303(status,transfer308.enabled&&transfer308.entity==held308.entity&&transfer308.generation==held308.generation);status=physical303.c_str();}
    if(status&&wcscmp(status,g_noteStatus)){wcsncpy_s(g_noteStatus,status,_TRUNCATE);g_noteDirty=true;}
}

void Recenter()
{
    g_referenceValid = false;
    InterlockedIncrement(&g_referenceGeneration286);
}

bool Frame(ID3D11Device* device, ID3D11Texture2D* leftEye, ID3D11Texture2D* rightEye,LONG renderFrame,bool captureReceipt)
{
    if (!device || !leftEye)
        return false;
    if (!g_tried)
    {
        g_tried = true;
        if (GetEnvironmentVariableW(L"FC3_STEREO_NO_XR", nullptr, 0) > 0)
        {
            Log("[XR] disabled by FC3_STEREO_NO_XR");
            return false;
        }
        g_instanceReady = Init();
    }
    if (!g_instanceReady)
        return false;
    if (g_space == XR_NULL_HANDLE)
    {
        // The headset may be connected after the game starts: keep trying,
        // about every five seconds, without flooding the log.
        static int countdown = 0;
        static int attempts = 0;
        if (countdown-- > 0)
            return false;
        countdown = 300;
        if (g_session == XR_NULL_HANDLE && !CreateSession(device, attempts++ > 0))
        {
            if (attempts == 1) Log("[XR] will keep checking for the headset");
            return false;
        }
        if (g_space == XR_NULL_HANDLE)
            return false;
    }

    PollEvents();
    if (!g_sessionRunning)
        return false;

    if (g_frameBegun)
        FinishFrame(leftEye, rightEye,renderFrame,captureReceipt);
    StartFrame();
    return true;
}
}
