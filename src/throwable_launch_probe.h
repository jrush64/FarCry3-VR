#pragma once
#include <windows.h>
#include "manual_throwables143.h"

namespace fc3throw {
#pragma pack(push,4)
// The future host supplies a pose already converted to the native world frame.
// Validity and epoch are explicit; a recent timestamp does not prove tracking.
struct PoseSample {
    ULONGLONG wallMs=0, xrTime=0, localOwnerId=0;
    UINT frame=0, epoch=0, validity=0;
    float position[3]={}, orientation[4]={}, linearVelocity[3]={}, angularVelocity[3]={};
};
// v2 corrects ownerEntity/ownerId to wrapper+0x28 owning pawn reference.
// v1 mislabeled wrapper+4 (the gadget weapon entity) as its owner.
struct LaunchRecord {
    UINT magic=0x544c5031, version=2, bytes=sizeof(LaunchRecord), phase=0;
    ULONGLONG wallMs=0;
    UINT thread=0, flags=0;
    UINT strategy=0, wrapper=0, ownerEntity=0, projectileRef=0, projectileEntity=0, dispatchTarget=0;
    ULONGLONG archetype=0, ownerId=0, projectileId=0;
    UINT counter=0xffffffffu, infiniteFlag=0, forcePhysics=0;
    float origin[3]={}, velocity[3]={}, localAngularImpulse[3]={}, projectilePosition[3]={};
    UINT registers[9]={}, eventWords[8]={};
    PoseSample pose;
};
#pragma pack(pop)
static_assert(sizeof(PoseSample)==88,"throw pose schema");
static_assert(sizeof(LaunchRecord)==296,"throw launch schema");
struct Stats { UINT queued=0; LONG lost=0, phaseCounts[6]={}; DWORD writeError=0; bool full=false; };

// Production capture is linked into the proxy. Physical writes additionally
// require an explicit release ticket for one freshly verified local projectile.
// Caller owns MinHook lifetime. Install validates PE identity plus all six sites.
bool Install();
void PublishPose(const PoseSample& pose);
bool FlushTo(HANDLE file); // invoke outside native callbacks; caller closes file
Stats Status();
// Disable before clearing host context or unloading. Do not unload while a game
// thread could still be inside a detour; integration must provide quiescence.
void Disable();

using IdentityReader143=bool (*)(ULONGLONG&);
struct Prepared143 {
    ULONGLONG owner=0,projectile=0,archetype=0,ms=0;
    UINT strategy=0,projectileRef=0;unsigned serial=0,epoch=0;
    UINT nativeWrapper149=0,nativeRef149=0,nativeConfig149=0;
    ULONGLONG nativeEntityId149=0;
};
// completed means native launch dispatch reached, not confirmed explosion/damage.
struct ManualStats143 {unsigned prepared=0,queued=0,applied=0,completed=0,rejected=0;bool pending=false;
    unsigned updates148=0,earlyCalls148=0,readyBlocks148=0,lastFlags148=0,lastGate148=0;
    unsigned expired148=0,contextRejects148=0,vectorRejects148=0;
};
// Only the normal host supplies the fresh owner-bound native identity reader.
// No reader means no physical writes, regardless of pose/capture ID guesses.
void SetIdentityReader143(IdentityReader143);
// Refreshed by an active manual interaction with valid focused tracking. The
// current frontend does not enable this until held-item/native arming is wired.
void PublishContext143(ULONGLONG owner,unsigned epoch,bool active);
bool CurrentPrepared143(Prepared143&);
// Native preparation precedes the first model draw. Adopt only a fresh,
// revalidated candidate matching the item actually observed after wheel choice.
bool AdoptPrepared144(ULONGLONG archetype,Prepared143&,ULONGLONG previouslyHeldProjectile=0);
bool CurrentLocalOwner144(ULONGLONG&);
bool QueueRelease143(const Prepared143&,const fc3manual::Release&);
// Exact native launch receipt, readable during draw preparation before Present.
// A different projectile's cumulative completion count must never hide this one.
bool ReleaseCompleted157(const Prepared143&,ULONGLONG& launchMs);
bool PublishMolotovFlame160(const Prepared143&,const float* nativeMatrix);
// Bounded read-only child-emitter evidence. before is previous native child pose,
// captured before this update's composition; local is authored effect definition.
struct MolotovChildSnapshot178 {
 ULONGLONG ms=0;UINT count=0;struct Child {bool valid=false;UINT flags=0;float scale=0,local[16]={},before[16]={};} child[4];
};
MolotovChildSnapshot178 InspectMolotovChildren178(const Prepared143& target);
void MolotovFlameStatus160(unsigned& updates,unsigned& spawns);
struct MolotovSnapshot166 {bool entity=false,component=false,lit=false,heldIdentity=false,positionValid=false;UINT componentAddress=0;float position[3]={},nativeFlame[3]={},ragTarget[3]={};ULONGLONG effectMs=0,effectPublishedMs169=0;bool effectSpawn169=false;float effectSource169[16]={},effectApplied169[16]={};unsigned updates=0,spawns=0,blocked=0,armed=0;};
MolotovSnapshot166 InspectMolotov166(const Prepared143&,UINT knownComponent=0);

// Read-only D50 candidate audio observations. Publisher must first verify
// PistolLocal. stage: 0 invalidate, 1 begin/new generation, 2 transfer,
// 3 native end, 4 heartbeat (retains current stage). flags are host telemetry.
struct D50AudioRecord170 {
 ULONGLONG wallMs=0,beginMs=0,owner=0,receiverActor=0,weaponEntity=0;
 UINT generation=0,epoch=0,stage=0,flags=0,state=0,wrapper=0,weapon=0;
 UINT receiver=0,reference=0,weaponReference=0,sound=0,type=0,index=0,firstPerson=0;
 UINT receiverMatch=0; // 0 unknown, 1 local actor, 2 observed wrapper entity
};
struct D50AudioStats170 {UINT recorded=0,dropped=0,rejectedContext=0,unknownReceiver=0,deferred225=0,played225=0,cancelled225=0;};
void PublishD50Audio170(UINT stage,UINT state,UINT wrapper,UINT weapon,UINT epoch,UINT flags);
UINT DrainD50Audio170(D50AudioRecord170* records,UINT capacity);
D50AudioStats170 D50AudioStatus170();
bool RequestD50Sound225(UINT state,UINT wrapper,UINT weapon,UINT epoch);
bool PumpD50Sound225(UINT state,UINT wrapper,UINT weapon,UINT epoch);
bool BindD50ManualSound226(UINT state,UINT wrapper,UINT weapon,UINT epoch,bool needsRack);

void SetMolotovIntent158(bool);
bool IgniteMolotov158(const Prepared143&);
bool MolotovLit158(const Prepared143&);
void SetPinIntent155(bool enabled);
bool RequestPinOut155(const Prepared143&);
bool LooseRelease156(const Prepared143&);
void PinSoundStatus156(unsigned& blocked,unsigned& played);
bool PinArmed155(const Prepared143&);
void SafetyStatus155(unsigned& blocked,unsigned& armed,unsigned& stowed);
void CancelRelease143(); // cancels only mod trajectory ownership, never native fuse/item state
ManualStats143 ManualStatus143();
void StopCapture143(); // capture disk/cap failure must not remove gameplay hooks
void SetUnlimited147(bool enabled);
bool Unlimited147();
unsigned PreservedStock147();

// C4 and proximity mines use their own native strategy and inventory path.
// A capability receipt is required before the frontend may claim controls.
enum PlaceableKind177 { None177=0,C4177=1,Mine177=2 };
struct PlaceableStatus177 {
 bool supported=false,prepared=false,pending=false;
 unsigned completed=0,rejected=0;
 UINT strategy=0,reference=0;
 ULONGLONG owner=0,item=0,observedMs=0;
 unsigned observed=0,detonated=0,consumed178=0,correctionFailed178=0;
 bool stockKnown179=false; UINT stock179=0,placedCount179=0;
 unsigned preservedStock179=0;
 UINT physics179=0,physicsVtable179=0,velocitySetter179=0,physicsBody179=0;unsigned velocityDeferred179=0,velocityExpired179=0;unsigned finalVelocity179=0;float requestedVelocity179[3]={};
 unsigned prepared181=0,prepareFailed181=0,reapplied181=0;
};
void PublishPlaceableContext177(ULONGLONG owner,unsigned epoch,bool active);
PlaceableStatus177 PlaceableNativeStatus177(PlaceableKind177);
// Matrix follows native affine column-vector convention; translation 12..14.
bool QueuePlaceable177(PlaceableKind177,ULONGLONG owner,unsigned epoch,const float* nativeMatrix);
bool QueueThrowPlaceable178(PlaceableKind177,ULONGLONG owner,unsigned epoch,const float* nativeMatrix,const float* worldVelocity);
bool QueueDetonate177(ULONGLONG owner,unsigned epoch);
void CancelPlaceable177();
// 181: ask the native mine strategy to prepare/present its next item after a direct toss.
void RequestPreparePlaceable181(PlaceableKind177);
bool PopNativeNote181(char* out,unsigned bytes); // queued backend log lines, oldest first
}
