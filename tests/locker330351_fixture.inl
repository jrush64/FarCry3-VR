#define main OriginalXR330
#include "hands303_xr_test.cpp"
#undef main
extern int PendingPreparation330();
extern bool HasReadOnlyWatch330();
void Visual330(int slot){auto& v=fc3visual292::g_visual[slot];v.owner=inventory303.owner;v.entity=inventory303.weapons[slot].entity;v.id=inventory303.weapons[slot].id;v.ready=true;}
void Reset330(bool held){
 memset(input303,0,sizeof(input303));fc3select290::KeepAlive(false);fc3visual292::SetPreparing330(false);fc3pad::SetSelector290(false);g_prepare330={};g_holsters297={};g_holstersLoaded297=true;g_handSettings303.enabled=true;g_selectGesture290={};g_hands303={};g_hands303.initialized=true;g_hands303.player=123;g_hands303.slot=held?2:-1;g_hands303.entity=held?102:0;g_hands303.empty=!held;g_hands303.hand=1;g_hands303.generation=900;g_hands303.reference=fc3xr::ReferenceGeneration286();g_hands303.epoch=1;g_hands303.last=GetTickCount64();g_bodyRenderFailed297=false;
 inventory303.valid=true;inventory303.equipped=held?102:0;
 for(auto& v:fc3visual292::g_visual)v=fc3visual292::WeaponVisual{};
 fc3visual292::g_identity=inventory303;for(int i=0;i<4;++i)fc3visual292::g_cacheTried296[i]=inventory303.weapons[i].id;
}
void Poll330(int result=0,bool allowed=true){inventory303.at=GetTickCount64();SeedInventory303(inventory303,result);fc3xr::SetHandsContext303(allowed,allowed,1);SyncControllers();}
void Start330(){Poll330();Poll330();g_prepare330.changedAt-=600;g_prepare330.idleAt-=600;Poll330();}
int BaselineLocker330(){
 OriginalXR330();Sleep(800);
 fc3select290::Watch330(true);Check303(HasReadOnlyWatch330()&&PendingPreparation330()<0,"330 read-only watch does not queue a native selection");fc3select290::Watch330(false);
 for(bool held:{false,true}){
  Reset330(held);const auto saved=g_hands303;Start330();
  Check303(g_prepare330.active&&PendingPreparation330()==0&&fc3visual292::Preparing330()&&fc3visual292::Hidden(),"330 actual XR route automatically requests first missing model and hides native preparation");
  Check303(g_hands303.entity==saved.entity&&g_hands303.hand==saved.hand&&g_hands303.empty==saved.empty&&fc3hands303::Read().empty&&!fc3hands303::Read().active,"330 temporary native equips preserve real owner while presenting empty tracked hands");
  Check303(fc3pad::FireOutput()==0,"330 automatic preparation blocks native firing");
  for(int slot=0;slot<4;++slot){
   Check303(g_prepare330.target==slot&&PendingPreparation330()==slot,"330 every actual native request matches missing owned slot");inventory303.equipped=inventory303.weapons[slot].entity;Poll330(1);Check303(g_prepare330.active&&g_prepare330.phase==2,"330 model capture awaits exact equipped native readback");Visual330(slot);Poll330(1);
   Check303(g_hands303.entity==saved.entity&&g_hands303.hand==saved.hand&&g_hands303.empty==saved.empty,"330 capture pass never becomes a hand transfer");
  }
  if(held){Check303(g_prepare330.active&&PendingPreparation330()==2&&!fc3hands303::Read().active,"330 original left-owned native slot restored before controls resume");inventory303.equipped=102;Poll330(1);}
  Check303(!g_prepare330.active&&!fc3visual292::Preparing330()&&!fc3visual292::Hidden(),"330 preparation clears hiding on completion");
  Check303(g_hands303.entity==saved.entity&&g_hands303.hand==saved.hand&&g_hands303.empty==saved.empty,"330 completing all four keeps original logical held or empty state");
  Poll330(1);Check303(!g_prepare330.active&&PendingPreparation330()<0,"330 complete caches do not retrigger weapon cycling");
 }
 Reset330(false);Visual330(0);Visual330(2);Visual330(3);Start330();Check303(PendingPreparation330()==1,"330 XR path skips every cached slot");
 Poll330(0,false);Check303(g_prepare330.active&&!fc3visual292::Preparing330()&&PendingPreparation330()<0,"330 native action context cancels queued selection and suspends preparation");
 puts("PASS:330 real XR routing, owned native requests, full four-slot automation, empty/left ownership preservation, firing mask, original restoration, partial cache and native-action suspension; offline only");
}


