#pragma once

// Owner-bound native CPawn look access. Camera conventions are checked
// passively; startup never scans memory, turns the camera, or tests writes.
namespace fc3aim
{
// Passive twelve-second native-healing evidence window; no action requests.
void BeginHealObservation299();
inline bool SpecificNativeAction283(int state){return state>0&&(state&~0x1f3f)==0&&(state&(state-1))==0;}
// Once per frame. forward: the main camera's forward axis in world space
// (Z up); valid: a main-view camera was seen this frame; gameplay: the 3D
// world is on screen.
void Probe(const float forward[3], bool valid, bool gameplay, bool weaponVisible = true);
void ProbeNativeView283(const float* right,const float* up,const float* forward,bool valid,bool gameplay,bool weaponVisible);
// True while a fresh native local-player sample has a verified convention.
bool Ready();
// Confirmed hidden-camera divergence, until passive native/draw reacquisition.
bool HiddenCameraBlocked127();
// Points the game camera: yaw = atan2(forward.y, forward.x), pitch = asin(forward.z).
void Drive(double yaw, double pitch);
// Separate visibility-camera lane for hidden weapons. Head offsets are added
// to fresh native look, preserving native stick/vehicle camera changes.
bool VisibilityBasis68(double& yaw,double& pitch,double& roll);
bool DriveVisibility68(double headYaw,double headPitch);
void EndVisibility68();
// The native look yaw (world, same convention as the rendered camera yaw on foot).
bool NativeLookYaw(double& yaw);
// Fresh native local entity handle, sampled only at the existing game input
// seam. A projectile owner must match both words; a rendered model is not ID.
bool LocalPlayerIdentity143(unsigned long long& entityId);
// Local player's GetPawnVehicleType path, sampled on the game input thread.
// `seat` is the legacy output name: it contains VEHICLE TYPE, not a seat index (audit169).
// False when unknown or stale; callers keep their own fallback. Bool ownership remains valid.
bool VehicleState(bool& inVehicle,int& seat);
// Owner-bound native camera composition after the vehicle's seat-local limits.
bool SetVehicleCamera279(bool active,const float* headView,const float* right=nullptr,const float* up=nullptr,const float* forward=nullptr,const float* visualBody=nullptr,bool nativeFootView=false);
bool NativeFootCamera281(); // fresh local on-foot owner, independent of CPawn look agreement
int NativeActionCamera282(); // fresh specific native action; excludes arm-release grace and unknown owners
bool NativeHandoff284(); // proven same-camera action exit, retained until ordinary aim can own it
bool SlideCamera280(); // exact native current-beautifier state, not sprint/crouch input heuristics
bool VehicleCameraBasis279(const float* right,const float* up,const float* forward,float* body,unsigned int& serial);
// Read-only native vehicle observation; basis rows are local X/Y/Z expressed in world space.
// No axis is labelled forward until vehicle/seat conventions have been verified.
struct VehicleSample170 {
    unsigned long long pawnId=0,vehicleId=0,ms=0;
    int actualSeat=-1,type=-1;
    float basis[3][3]={},position[3]={};
    bool identityValid=false,seatValid=false,basisValid=false;
    //173: optional copied native pawn observations; not proof the driver camera is active.
    float pawnBasis173[3][3]={},localLook173[3]={},worldLook173[3]={};
    bool pawnBasisValid173=false,lookValid173=false;
};
bool GetVehicleSample170(VehicleSample170& out);
//352: re-copy the verified entity transform at draw time; no engine writes.
bool GetVehicleDrawPose352(VehicleSample170& out);
void SetVehicleSampleForTest187(const VehicleSample170* sample); // fixture only; nullptr restores native sampling
bool NativeActionOwnsArms128();
void SetNativeActionForTest128(int state);
void SetVehicleStateForTest(int state); // fixtures: -1 native, 0 on foot, 1 in a vehicle
// Only the game's existing XInput seam performs native reads and writes.
void InputTick(const void* caller);
// Cancel outstanding look ownership immediately.
void Suspend();
// 210: refuse Jason's native ApplyDamage while God mode is on (verified hook).
void SetGodMode210(bool on);
}
