#pragma once

// Motion controllers as an Xbox pad.
//
// The game reads its gamepad through XInputGetState (xinput1_3.dll, ordinal 2).
// That function is hooked; the headset presenter publishes the controllers'
// buttons, sticks and triggers each frame, and they are merged into what the
// game receives (a real pad keeps working alongside them).
namespace fc3pad
{
enum class AmmoButtonAction : unsigned char { None, Interaction, Pickup, Release, PickupOverride, Consumed };
struct Controllers
{
    bool active;                    // the runtime reports controllers in use
    float leftX, leftY, rightX, rightY;     // thumbsticks, -1..1, up = +y
    float leftTrigger, rightTrigger, leftGrip, rightGrip;   // 0..1
    bool a, b, x, y, menu, leftStickClick, rightStickClick;
    AmmoButtonAction ammoAction;     // assigned by Publish, never by hardware
    // finger sensors: index finger resting on the trigger, thumb resting on a
    // button, the stick or the thumb rest
    bool leftIndexTouch, rightIndexTouch, leftThumbTouch, rightThumbTouch;
    bool weaponRoles303;float physicalLeftGrip303,physicalRightGrip303; // preserve physical inventory/menu chords
    bool ammoPouch334; // current physical Skorpion ammo pouch, normalized reload hand
    bool ammoGripReserved326; //1911 pass grip remains available to native support, not reload grabs
    bool shotgunSupport342;int shotgunSupportProfile342;
    unsigned shotgunSupportEntity342,shotgunSupportGeneration342;unsigned long long shotgunSupportAt342;
};
inline bool SupportHold342(const Controllers& c,int profile,unsigned entity,unsigned generation,unsigned long long now){
 return c.active&&c.shotgunSupport342&&c.leftGrip>.35f&&c.shotgunSupportProfile342==profile&&
  entity&&c.shotgunSupportEntity342==entity&&c.shotgunSupportGeneration342==generation&&
  now>=c.shotgunSupportAt342&&now-c.shotgunSupportAt342<=120;
}

// Hooks XInputGetState (watch only). Call once MinHook is initialised.
void Install();
// Plugs a virtual Xbox 360 pad into Windows (ViGEm bus) on a worker thread.
// Called as early as possible so the game finds the pad at startup.
void StartVirtualPad();
// Called by the presenter after xrSyncActions, every frame.
void Publish(const Controllers& state);
void SetHandsStowed303(bool blocked);
// Latest tracked hands in the body/vehicle-facing game basis. Driving owns no
// camera pose; missing tracking immediately releases its steering lease.
void SetVehicleHands(const float* right, const float* left);
// Render-thread lease:1 wheel selection,2 release/commit,3 hand-pointed wheel,4 body grab.
// Expires after120ms without fresh publication. Zero cancels.
void SetBodyInput285(int mode,float x=0,float y=0);
bool BodySelection285();
bool BodyGesture285();
Controllers BodyControls285(); // normalized controls before wheel masking
void SetSelector290(bool active);
bool SelectorActive290();
//352: a short-lived lease from the actual current car wheel draw.
bool DoorInputActive363();bool ThrowableInputActive363();
void SetDoorInput357(bool held,bool enter); // bounded exterior handle request, no reload engine call
void SetStickThrottle365(bool enabled); // 365: left stick Y drives accelerator/brake in cars and handlebar vehicles
bool StickThrottle365();
void SetVehicleSmoothing363(bool handlebars,float milliseconds);
float VehicleSmoothing363(bool handlebars);
void SetCarWheelInput352(unsigned long long vehicle,float demand);
//355: separate, short render lease for identified ATV/jetski handlebars.
void SetHandlebarInput355(unsigned long long vehicle,int type,float demand);
void SetHandlebarContext355(unsigned long long vehicle,int type);
void SetHandlebarEnabled355(bool enabled);
bool HandlebarEnabled355();
void SetHandlebarLock355(float degrees);
float HandlebarLock355();
// Rim rotation from captured current angle to full native steering.
void SetCarWheelFullLockDegrees(float degrees);
float CarWheelFullLockDegrees();
void SetCarWheelSteeringEnabled(bool enabled);
bool CarWheelSteeringEnabled();
// True once the right trigger has been pulled (used to time a capture).
bool FirePressed();
// True while the right trigger is held right now.
bool FireHeld();
// True once after both thumbsticks were held clicked for a second
// (re-aligns the player's hands with Jason's hands).
bool TakeCalibration();
// While on, the virtual pad's right stick is driven by the mod (x, y in -1..1)
// instead of the controller - used to turn the camera for measurements.
void SetLookOverride(bool on, float x, float y);
// The right controller's stick as last published (-1..1).
void LookStick(float& x, float& y);
// Grip values as last published (0..1).
void Grips(float& left, float& right);
// Everything the controllers reported last frame.
Controllers Latest();
Controllers AmmoControls(); // same grips/poses, X only for a latched ammo chord
AmmoButtonAction AmmoAction(const Controllers&);
bool AmmoPickupOverride(const Controllers&);
void SetGripOnlyAmmo(bool);
bool GripOnlyAmmoEnabled();
// While on, the left grip does not press LB (it is holding the gun's front).
void SuppressLeftGrip(bool on);
// While on, the trigger does not reach the game (the shotgun is not pumped yet).
void BlockFire(bool on);
// Bow owns RT only while equipped; one native draw/release per physical shot.
void SetBowInput(bool on,float trigger);
// Wheel still chooses the native item. Only an explicit draw/held throwable
// lease owns RB and prevents the same grip/trigger from firing/reloading.
void SetThrowableInput144(bool owned,bool shoulder);
void SetPhysicalThrowable361(bool active); // only explicit empty-hand pouch session
void SetPlaceableInput180(bool equipped);
// Short native previous-weapon input after an acknowledged empty manual item.
void RequestPreviousWeapon179(bool pulse=true);
void SetLighterHeld161(bool held);
void SetGameplayControls(bool on); // native menus retain their face-button mapping
bool DedicatedInteractionLayout();
void SetGripContext(bool valid,bool nearWeapon); // refreshed geometry; decision latched at squeeze
bool WheelOwnsGrip();
void SetTwoHandWeaponHold(bool on);
void SetScopeFocus61(bool on);
void SetDedicatedInteractions(bool on);
bool DedicatedInteractionsEnabled();
void SetSwapBX(bool on); // swaps hardware B/X before gameplay and physical reload routing
bool SwapBXEnabled();
bool WeaponSelectionRequested();
bool WeaponWheelRequested145();
bool InteractionOnly(); // dedicated B interaction without a physical insertion pulse
// Pump mode uses the same full-pull threshold for shot detection and game input.
void RequireFullFirePull(bool on);
// Actual converted RT output, also used by offline pump/input regression checks.
unsigned char FireOutput();
short SteeringOutput(); // final left-stick X from the same conversion path used by the game
unsigned char RightTriggerOutput365();unsigned char LeftTriggerOutput365();short ThumbLYOutput365(); // 365 test accessors
// Motion shotgun grips/left trigger control physical handling instead of LB/RB/LT.
// X remains ordinary reload. Scope is set by actual gameplay/shotgun identity.
void SetShotgunHandling(bool on);
void SetOneShellX(bool on);
// Raw X stays available for pickup/save; only generated insertion requests reach reload.
void SetManualReload(bool on,bool alignment);
// 1911 physical magazine input owns raw X and grips only in its verified context.
void SetMagazineHandling(bool on,bool busy=false);
void SetLeverHandling(bool on);
void SetLeverXOwned(bool on);
void SetXOwned(bool on); // revolver Manual: X opens/closes the cylinder
void SetPistolCalibration(bool on);
bool MenuOpen(); // mod Insert/options only; used by the headset options layer
bool NativeMenuOpen(); // verified engine pause state, plus immediate Start/Select opening guard
bool GameplayInputBlocked();
bool NativeInputBlocked();
bool NativeMenuConfirmed();
Controllers NativeUiControls();
void SetNativeMousePointer(bool);
void SetNativePointerTrigger298(bool);
void GuardMenuExit();
void BeginOneShellX();
void EndOneShellX();
bool ManualShotgunInputActive(); // native reload gate distinguishes interaction from insertion
bool ReloadOutput();
unsigned char AimOutput();
unsigned short ButtonsOutput();
void CancelReloadRequest();
// Brief native reload request; raw controller state remains unchanged.
void RequestReload();
// Persistent visual machete ownership; source acquisition is one native swipe.
void SetMacheteHoldMode(bool on);
void SetMacheteContext(bool on);
void SetMacheteHandling(bool on);
void RequestMacheteSource();
void CancelMacheteSource();
// One scoped native melee pulse, independent from cold visual acquisition.
bool RequestMacheteAttack();
void CancelMacheteAttack();
bool MacheteFireBlocked();
}
