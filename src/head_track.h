#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Head rotation for Far Cry 3.
//
// The game's real camera is turned by feeding look movement into the mouse
// device the game already reads (DirectInput 8). The camera's actual direction
// is measured every frame from the camera constant buffer of the frame that
// was just rendered, and that measurement closes the loop:
//
//   target direction  = body yaw + head yaw,  head pitch
//   injected movement = (target - measured) / measured-response-per-count
//
// The pose declared to the compositor for a frame is built from the MEASURED
// camera direction of that frame, never from what was asked for. The tag and
// the pixels therefore cannot disagree, whatever the game's mouse sensitivity,
// smoothing or input latency turn out to be.
//
// The right stick still turns the body: stick-driven rotation is absorbed into
// the body yaw instead of being fought.
namespace fc3head
{
struct Camera
{
    float right[3];
    float up[3];
    float forward[3];
};

// Finds DirectInput's device functions and hooks them. Call once, outside DllMain.
void InstallInputHooks();

// The frame that is about to be presented was rendered with this camera
// (null = unknown this frame). Call once per Present, before TagOrientation.
void FrameRendered(const Camera* camera);

// Orientation (OpenXR quaternion x,y,z,w) the frame just rendered should be
// declared with. False = no measurement; the caller stays head-locked.
bool TagOrientation(float quaternion[4]);

// Head orientation the next frame should be rendered for (OpenXR quaternion).
// Computes the look movement to inject. active = false stops injecting.
void Command(const float headQuaternion[4], bool active);

void Recenter();
void TestPoll(long& x, long& y);   // offline test only
void LogStatus(long frame);
}
