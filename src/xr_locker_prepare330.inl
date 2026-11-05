#include "locker_prepare330.h"
fc3prepare330::State g_prepare330;bool g_prepareRequestFailed330=false;
bool LockerPrepareControls330(fc3pad::Controllers& c){
 const auto now=GetTickCount64();const auto inv=fc3select290::Read();
 if(inv.valid){g_selectShown290=inv;fc3visual292::Preload296(inv);}
 const bool eligible=g_handSettings303.enabled&&g_holsters297.mode!=fc3holster297::Front&&g_roleContext303&&now-g_roleAt303<=120&&c.active&&g_viewsTracked&&g_referenceValid&&!fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()&&!fc3pad::GameplayInputBlocked()&&!c.menu&&g_hands303.pendingSlot<0&&!g_selectGesture290.open&&(g_prepare330.active||g_changeContext303);
 const bool idle=c.leftGrip<.35f&&c.rightGrip<.35f&&c.leftTrigger<.2f&&c.rightTrigger<.2f&&!c.a&&!c.b&&!c.x&&!c.y&&!c.menu;
 unsigned ready=0;for(int i=0;i<4;++i)if(fc3visual292::Read(inv,i))ready|=1u<<i;
 const auto out=g_prepare330.Step(inv,ready,eligible,idle,g_prepareRequestFailed330?-1:fc3select290::Result(),now);g_prepareRequestFailed330=false;
 if(out.rearmed331)Log("[LOCKER331] native equipped identity stable again; rearmed one bounded missing-model recovery pass");
 if(out.retry331){fc3select290::KeepAlive(false);Log("[LOCKER331] recover interrupted capture slot=%d equipped=%08X expected=%08X retry=%u",out.target+1,inv.equipped,inv.weapons[out.target].entity,g_prepare330.attempts331[out.target]);}
 if(out.changed){fc3select290::KeepAlive(false);fc3visual292::SetPreparing330(false);fc3visual292::Target(inv,-1);Log("[LOCKER330] loadout changed; invalidated stale preparation");}
 if(out.started)Log("[LOCKER330] automatic missing-appearance preparation started originalSlot=%d readyMask=%X",g_prepare330.original+1,ready);
 if(out.failed)Log("[LOCKER330] appearance/restore step refused or timed out failedMask=%X",g_prepare330.failed);
 if(out.finished){fc3visual292::SetPreparing330(false);fc3visual292::Target(inv,-1);fc3pad::SetSelector290(false);g_handTriggerWait303=true;g_handGripWait303[0]=c.rightGrip>.25f;g_handGripWait303[1]=c.leftGrip>.25f;Log("[LOCKER330] automatic preparation ended readyMask=%X failedMask=%X restored=%d",ready,g_prepare330.failed,g_prepare330.original<0||inv.equipped==inv.weapons[g_prepare330.original].entity);}
 if(!g_prepare330.active)return false;
 fc3select290::KeepAlive(eligible);fc3visual292::SetPreparing330(out.active);fc3pad::SetSelector290(out.active);
 if(out.active){
  fc3visual292::Target(inv,out.target);
  if(out.request>=0){g_prepareRequestFailed330=!fc3select290::Request(inv,out.request);Log("[LOCKER330] native owned-weapon preparation request slot=%d restore=%d accepted=%d",out.request+1,g_prepare330.phase==3,!g_prepareRequestFailed330);}
  c.a=c.b=c.x=c.y=c.rightStickClick=false;c.leftTrigger=c.rightTrigger=c.leftGrip=c.rightGrip=0;
 }
 g_bodyVisible297=g_stowReady304=g_zonesVisible304=false;PublishHands303(false);return true;
}
