#pragma once

// Widens the game's own field of view while the headset is tracking.
//
// The eye cameras can only show what the game actually drew, and the game
// only draws (and keeps loaded) what is inside its own camera's view. For the
// picture to fill the headset, the game's view has to be at least as wide and
// as tall as the headset's.
//
// The game's field of view is 75 degrees times a factor stored in the player's
// profile (FOVScaleFactor). This module finds the live copy of that factor in
// memory and raises it. Every candidate is verified by its effect: a write is
// kept only if the camera's measured projection actually changes, otherwise it
// is put back. The profile file itself is never touched, and the original
// value is restored when tracking stops.
namespace fc3fov
{
// Largest half-angle tangents the game's view must cover (the headset's).
void SetNeed(float tanHorizontal, float tanVertical);
// Once per frame. xScale / yScale: the game camera's measured projection scale.
// live: the 3D world is on screen, so the camera measurement means something.
void Update(bool wanted, bool live, float xScale, float yScale);
// True while the game's own view is wide and tall enough for the headset
// (as of the last Update with a live camera).
bool Covered();
// Presenter hint, once per frame before Update: no visible weapon this frame
// (vehicle-like). Such a view, when the scale factor leaves it narrow, is
// widened through the engine's own gfx_FOVOverride variable instead.
void SetHiddenWeapon(bool hidden);
// True while gfx_FOVOverride owns the view: a hidden-weapon camera the scale
// factor could not cover (a vehicle, a cinematic). The vehicle look uses it.
bool OverrideActive();
void ForceOverrideForTest(bool active); // fixtures only
// Passive vehicle diagnosis: state 0 idle, 1 scan, 2 test, 3 restore,
// 4 retest, 5 hold, 6 failed, 7 verifying (widening seen, telling stable from
// transient), 8 engine override active (gfx_FOVOverride owns the view; bound
// is then also true). Never scans or writes on this call.
void ProbeState(int& state,int& candidates,int& attempts,bool& bound);
}
