#include "locker331_xr_fixture.inl"
int main(){
 Original330XR();
 for(bool held:{false,true}){
  Reset330(held);Visual330(0);Visual330(2);Visual330(3);Start330();const auto saved=g_hands303;
  inventory303.equipped=101;Poll330(1);Check303(g_prepare330.phase==2,"331 native arrival enters capture, not completion");inventory303.equipped=0;Poll330(1);
  Check303(g_prepare330.active&&g_prepare330.phase==1&&PendingPreparation330()==1&&g_prepare330.attempts331[1]==1,"331 actual XR loss of1887 native equip clears completed queue and dispatches exact same owned slot");
  Check303(fc3visual292::Hidden()&&fc3hands303::Read().empty&&fc3pad::FireOutput()==0&&g_hands303.entity==saved.entity&&g_hands303.hand==saved.hand,"331 recovered request remains hidden, cannot fire and cannot become a hand transfer");
  inventory303.equipped=101;Poll330(1);Visual330(1);Poll330(1);
  if(held){Check303(PendingPreparation330()==2,"331 capture recovery restores original native slot");inventory303.equipped=102;Poll330(1);}
  Check303(!g_prepare330.active&&!g_prepare330.failed&&g_hands303.entity==saved.entity&&g_hands303.empty==saved.empty,"331 recovered capture returns original held/empty state");
 }
 Reset330(false);Visual330(0);Visual330(2);Visual330(3);Start330();
 for(int n=0;n<3;++n){inventory303.equipped=101;Poll330(1);inventory303.equipped=0;Poll330(1);}
 Check303(!g_prepare330.active&&g_prepare330.failed==2&&PendingPreparation330()<0,"331 XR repeated native loss stops at finite attempt limit");
 inventory303.equipped=100;Poll330(1);g_prepare330.settledAt331-=800;Poll330(1);g_prepare330.idleAt-=400;Poll330(1);
 Check303(g_prepare330.rearmed331&&g_prepare330.active&&PendingPreparation330()==1,"331 native equipped context recovery automatically revisits missing1887 without manually selecting1887");
 puts("PASS:331 actual XR native equip-loss retry in empty/left-held cases, exact slot, hidden/fire/owner isolation, restoration, retry cap and automatic context recovery; offline only");
}
