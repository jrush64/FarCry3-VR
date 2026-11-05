#pragma once
namespace fc3aim {
// Row-major 3x4 WORLD transform from the exact unmodified weapon FRAME to its
// final VR FRAME. Only the equipped core draw may publish this; never optics,
// magazines, limbs, particles, deferred stale frames, or a guessed controller.
void PublishMuzzle58(const float nativeToFinalWorld[12],const float nativeRootWorld[3],
                    unsigned equippedWrapper,unsigned equippedWeapon,unsigned profile,unsigned epoch,bool active,bool mirrored340=false);
void ClearMuzzle58();
// Candidate 107: the aim/reticle ray of the frame (world axes) and the native camera forward of
// the same core draw. The native shot (camera forward plus the game's spread) is turned from that
// camera forward onto the aim ray. The gun move is not applied to it a second time: the camera
// already follows the gun, so doing so added the native hip angle of the first-person weapon.
void PublishMuzzleAim107(const float aimWorld[3],const float cameraForward[3]);
// Pure: rotate shot by the minimal rotation taking cameraForward onto aim. False when the shot is
// more than 20 degrees from the camera forward or the aim more than 75 degrees from it.
bool AimShotDirection107(const float cameraForward[3],const float aim[3],const float shot[3],float out[3]);
// Candidate 111: the native muzzle (world) of the latest local corrected shot and a sequence number
// that changes with every shot (muzzle flash/tracer particle transport on the render thread).
bool LastShotMuzzle111(float muzzle[3],unsigned& sequence);
// Candidate 206: muzzleLocal is the native muzzle expressed in the published native weapon FRAME, so
// the render thread re-anchors it to the CURRENT frame's drawn weapon; distance is the first scene
// hit along the published barrel ray (25 m when clear). point/origin are kept for logs and fixtures.
struct LaserSample194 {float origin[3]={},point[3]={},nativeMuzzle[3]={},muzzleLocal[3]={};float distance=0;UINT profile=~0u,epoch=0;ULONGLONG ms=0;bool hit=false,localValid=false;};
void SetLaserEnabled194(bool enabled);
// 243: a local projectile launched within the last 1.5 s (flight readback running).
bool ProjectileFlightActive242();
// 245: the NATIVE origin and spray axis of the latest local flame birth (before the mod rotated it), for the jet cone.
bool LastFlameBirth245(float* origin,float* axis,unsigned long long& ageMs);
// 246: the engine's bone world-matrix getter is hooked and rewrites the local flamethrower's bones to the drawn pose.
bool BoneRewriteInstalled246();
// Candidate 206: the exact native weapon FRAME (row-major 3x4, world) of the publication, so the laser
// can express the native muzzle in gun-local coordinates. Call after PublishMuzzle58 on the same draw.
void PublishMuzzleFrame206(const float nativeFrame[12]);
// Candidate 206: native bullet spread kept around the barrel ray, 0..100 percent (shotgun pellets always 100).
void SetSpreadPercent206(int percent);int SpreadPercent206();
// Candidate 208: the local pawn's EQUIPPED weapon (wrapper, weapon id, strategy), sampled on the game input
// thread from the same pawn field the HUD ammo widget reads (pawn+0xBE4+0xB4 -> reference -> +0xC entity).
// Replaces the HUD-widget wrapper, which only refreshes when the ammo count changes (first shot).
bool EquippedWeapon206(unsigned& wrapper,unsigned& weapon,unsigned& strategy);
// Candidate 206: cumulative local-shot outcomes for the periodic summary log.
void ShotCounters206(unsigned& corrected,unsigned& rejected);
bool ReadLaser194(LaserSample194& sample);
// Pure transport: native muzzle point (including native suppressor attachment)
// and shot direction after the game's own spread. Reject invalid/non-rigid poses.
bool TransformMuzzle58(const float move[12],const float origin[3],const float direction[3],
                       float finalOrigin[3],float finalDirection[3],bool reflected340=false);
bool TransformProjectileMuzzle58(const float move[12],const float origin[3],const float angles[3],
                                 float finalOrigin[3],float finalAngles[3]);
}
