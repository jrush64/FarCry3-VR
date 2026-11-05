#include "locker330351_fixture.inl"
void Poll351(int result=0,bool change=false,bool role=true){inventory303.at=GetTickCount64();SeedInventory303(inventory303,result);fc3xr::SetHandsContext303(role,change,1);SyncControllers();}
void Ready351(bool held=false){
 Reset330(held);g_handSettings303.requireHolster=true;g_rackBasisReady292=g_bodyPlaced297=true;g_prepare330={};fc3grab333::Publish({});
 for(int i=0;i<4;++i){inventory303.weapons[i].entity=501+i;inventory303.weapons[i].id=801+i;inventory303.weapons[i].wrapper=901+i;Visual330(i);fc3visual292::g_cacheTried296[i]=inventory303.weapons[i].id;}
 fc3visual292::g_identity=inventory303;inventory303.equipped=501;
}
int main(){
 BaselineLocker330();const bool baseline351=GetEnvironmentVariableW(L"FC3_BASELINE351",nullptr,0)!=0;
 for(bool wasHeld351:{false,true})for(int slot351=0;slot351<4;++slot351)for(int hand351=0;hand351<2;++hand351){
  Ready351(wasHeld351);Poll351();Poll351();Poll351();Move303(hand351,slot351);Poll351();input303[hand351?kLeftGrip:kRightGrip]=1;Poll351();
  printf("%s:351 locker new visible slot%d hand%d pending%d empty%d renderFail%d change%d\n",baseline351?"BASELINE":"PASS",slot351,hand351,g_hands303.pendingSlot,g_hands303.empty,g_bodyRenderFailed297,g_changeContext303);
  if(!baseline351){Check303(g_hands303.pendingSlot==slot351&&g_pick292.requested,"351 empty hands can draw exact newly registered owned slot despite stale held-weapon change gate");inventory303.equipped=inventory303.weapons[slot351].entity;Poll351(1);Check303(!g_hands303.empty&&g_hands303.entity==inventory303.equipped&&g_hands303.hand==hand351,"351 draw still requires exact native equipped readback");Check303(!EmptyHolsterDraw351(inventory303),"351 confirming held ownership ends empty-hand recovery");g_handSettings303.requireHolster=false;input303[kRightGrip]=input303[kLeftGrip]=0;Poll351();Move303(1-hand351,(slot351+1)%4);input303[(1-hand351)?kLeftGrip:kRightGrip]=1;Poll351();Check303(g_hands303.pendingSlot<0,"351 held-weapon change denial still blocks replacement during real operation");}
 }
 if(!baseline351){
  for(int blocked351=0;blocked351<4;++blocked351){
   Ready351();Poll351();Poll351();Move303(0,0);Poll351();
   if(blocked351==0)g_bodyRenderFailed297=true;
   if(blocked351==1)tracked303[0]=false;
   if(blocked351==2)input303[kRightTrigger]=1;
   input303[kRightGrip]=1;Poll351(0,false,blocked351!=3);
   Check303(g_hands303.pendingSlot<0,"351 empty recovery retains renderer/tracking/trigger/native-context denial");g_bodyRenderFailed297=false;tracked303[0]=true;
  }
  Ready351();input303[kRightGrip]=1;Move303(0,0);Poll351();Poll351();Check303(g_hands303.pendingSlot<0,"351 grip held through locker transition cannot auto draw without neutral release");
 }
 return 0;
}
