#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "hand_ui_settings.h"
#include "holster297.h"
#include "weapon_hands303.h"

struct ID3D11Device;
struct ID3D11Texture2D;

// OpenXR output for the same-frame stereo build.
//
// The session runs on the game's own D3D11 device. Each Present, the finished
// left and right eye images are copied into per-eye swapchain images and
// submitted as one projection layer. What is declared to the compositor is
// what was rendered: a symmetric frustum with the game's field of view, and
// two parallel eyes exactly one eye-distance apart. The eye distance itself is
// taken FROM the runtime and fed back to the renderer, so the separation in
// the image and the separation the compositor assumes are the same number.
//
// Everything degrades to "run flat": no loader, no runtime, no headset or a
// lost session all leave the game playing on the monitor.
namespace fc3xr
{
fc3holster297::Settings WeaponHolsterSettings297();
bool CaptureHolsterPosition345(fc3holster297::Settings&,int slot,int hand);
bool ConfigureWeaponHolsters297(const fc3holster297::Settings&,const wchar_t* path=nullptr);
bool WeaponHolsterWristNeeded297();
// Per-eye screen UV for the tracked right aim ray at native UI depth (metres).
bool OptionsMenuRay299(float width,float height,bool captured,float* uv);
bool NativeMenuRay298(float depth,float aspect,float* screenUv4); // observation only; idle holsters never own weapon input
void SetSelectorAllowed290(bool allowed);
void SetHandsContext303(bool active,bool canChange,unsigned epoch=0,int nativeHoldingSide=0,bool presentationOnly326=false);
void SetHandPalm303(int physicalHand,const float* controllerLocal,unsigned epoch);
void SetSelectorWrist292(const float* bodyWrist);
bool SelectorOpen291();
// Called once per Present on the render thread, before the monitor side-by-side
// composite. rightEye may be null (stereo off); the left image then feeds both
// eyes. Returns true while frames are being paced by the headset.
bool Frame(ID3D11Device* device, ID3D11Texture2D* leftEye, ID3D11Texture2D* rightEye,LONG renderFrame=0,bool captureReceipt=false);

void SetHandUi(ID3D11Texture2D* texture,const float* renderedLeftWrist,const fc3handui::Settings& settings,bool wheel);
bool HandUiReady(); // preserve native widgets until XR hand-image copy has succeeded
bool HandUiPoseForTest(int panel,float* positionQuaternion); // offline pose geometry fixture
bool Running();
// Take the current head direction and position as "straight ahead".
void Recenter();
unsigned ReferenceGeneration286(); // observation only; cancels old gestures after recenter
// Short vibration on both controllers, 'count' pulses (a signal to the player).
void Buzz(int count);
int FeedbackStrength302();
int GunfireStrength339();
bool ConfigureGunfire339(int percent,const wchar_t* path=nullptr);
// Native local bullet-shot receipt only; producer never calls the XR runtime.
void Gunshot339(unsigned entity,int profile);
bool ConfigureFeedback302(int percent,const wchar_t* path=nullptr);
void InteractionPulse285(int hand); //0 right,1 left; brief focused-session feedback
// Queue one short right-hand impact; consumed on the focused XR thread.
void MacheteHit();
// Current diagnostic instructions shown on a movable OpenXR quad.
void SetTestNote(const wchar_t* status);
void SetWeaponNotesContext303(bool active);
// Completed-render contact points, body axes (z forward), in game units.
void SetReloadMarkers(const float* port,const float* shell,float radius,int contact,float handDiameter=.014f);
void SetHandSphere();
struct BodyReloadGuides51 {bool point=false,hand=false,box=false;float port[3]={},palm[3]={},transform[16]={},dimensions[3]={},radius=0,handDiameter=0;};
BodyReloadGuides51 BodyReloadGuideSnapshot51();
void HideReloadWeaponMarker();void HideReloadMarkers();void HideGrabMarkers();
// Two-hand grab gizmo (all weapons): grab point on the gun + left palm sphere, body axes in game units, diameters in metres.
void SetDoorHighlight363(const float* frame,const float* handle,float width,float height,bool held);
void SetGrabMarkers(const float* point,const float* palm,bool inReach,float pointDiameter,float palmDiameter);
void SetGuideGroupVisibility(bool weaponReload,bool twoHand,bool insertOpen=false);
bool ReloadMarkerBoxVisible(); // tests
bool GuideLayersVisibleForTest();bool GuideBoxAtlasForTest();void GuideFacesForTest(unsigned mask);
void ForceReferenceValid(bool on); // tests: pretend the headset reference exists
void SetOptionsNote(const wchar_t* status);
void SetOptionsTexture(ID3D11Texture2D* texture,int x=0,int y=0,int width=0,int height=0);
}

namespace fc3xr { void SetWeaponBox(const float* bodyTransform,const float* dimensions,int contact); }
