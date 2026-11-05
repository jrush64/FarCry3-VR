#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ID3D11Device;
struct ID3D11Texture2D;
struct IDXGISwapChain;

// Same-frame stereo for Far Cry 3.
//
// Every draw that can differ between the eyes is issued twice: once into the
// game's own render targets (left eye) and once into a duplicate set the mod
// owns (right eye). The eye offset is applied inside the game's vertex shaders,
// which are patched as they are created. Shadow maps and anything else that
// never sees the main view stay single.
namespace fc3stereo
{
void RequestOptionsToggle299();
// Called for every device the process creates. Installs the inline hooks once.
void OnDeviceCreated(ID3D11Device* device);
// Called when the game's main swapchain is chosen.
void OnPrimarySwapChain(IDXGISwapChain* swapChain, ID3D11Device* device);
// Called immediately before the real Present of the main swapchain.
void OnPresent(IDXGISwapChain* swapChain);
// Called right after the real Present returns.
void AfterPresent(IDXGISwapChain* swapChain);
void WriteRenderProbe(const char* text);
bool RunOverlay291(bool (*render)(void*),void* data);

// One eye's camera, relative to the game's own camera.
struct EyeView
{
    // Takes a point in the game camera's view space (x right, y up, z forward,
    // 1 unit = 1 metre) to the eye's view space. Row-major, column vectors.
    float view[16];
    // Tangents of the eye's frustum half-angles (left and down are negative).
    float tanLeft, tanRight, tanUp, tanDown;
};

// ---- used by the headset presenter ----
// Render both eyes from these cameras from the next frame on (null = go back
// to the built-in pair: two parallel eyes with the game's own field of view).
void SetEyeViews(const EyeView* eyes);
// Controller poses: hand to game camera, game view axes (x right, y up,
// z forward = where the controller points), row-major, world units.
// Null = that hand is not tracked this frame.
void SetHandPoses(const float* right, const float* left);
// Whole-texture GPU copy on the game's context that the stereo hooks ignore.
bool CopyTexture(ID3D11Texture2D* destination, ID3D11Texture2D* source); // true means copy issued, not GPU completion
// Horizontal projection scale of the game's main camera (1 / tan(half fov)).
float MainXScale();
// Half the distance between the eyes, in world units (1 unit = 1 metre).
float EyeSeparation();
// The headset reports its eye distance; the renderer adopts it (times the
// user's world-scale factor).
void SetEyeSeparation(float halfDistanceMetres);
// Main camera axes (world space, unit length) the frame being presented was
// rendered with. False when no main-view camera was seen this frame.
bool FrameCamera(float right[3], float up[3], float forward[3]);
// True while the 3D world is on screen (not menus or loading screens).
bool InGameplay();
bool InWorldView(); // world-camera rendering independent of gameplay input heuristic
bool HeadTrackingEnabled();
float WorldScale();
// 0 = automatic, -1 = off, N = hold every frame to N headset refresh periods.
int PacePeriods();
}
