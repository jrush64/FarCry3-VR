#define main LegacyFeedback302
#include "haptics302_xr_test.cpp"
#undef main

fc3hands303::Snapshot Owner339(int hand,ULONGLONG now) {
 fc3hands303::Snapshot s;s.enabled=s.active=s.engaged=true;s.entity=0x1234;s.generation=7;s.hand=hand;s.at=now;return s;
}
void Publish339(int hand){AcquireSRWLockExclusive(&g_handsLock303);g_handsSnapshot303=Owner339(hand,GetTickCount64());ReleaseSRWLockExclusive(&g_handsLock303);}
void Ready339(int hand) {
 syncResult302=XR_SUCCESS;g_sessionRunning=g_actionsReady=true;sent302.clear();
 ClearFeedback302();SyncControllers();Publish339(hand);sent302.clear();
}
int main(){
 LegacyFeedback302();
 // Deterministic producer/consumer timing: no sleeps or timing-sensitive volley assertions.
 {
  fc3gunfire339::Queue q;auto s=Owner339(1,1000);
  Check302(!q.Request(s.entity,7,s,1000),"339 no gunshot queue before a focused XR receipt");
  q.SetLive(1000,true);Check302(q.Request(s.entity,7,s,1000),"339 local A2000 shot accepted");
  Check302(!q.Request(s.entity,7,s,1001),"339 duplicate volley receipts coalesce");
  auto p=q.Take(s,1002);Check302(p.hand==1&&p.duration==18000000&&p.amplitude==.3f,"339 SMG shape and left physical owner captured");
  Check302(q.Take(s,1003).hand<0,"339 native receipt consumed once");
  s.at=1033;Check302(q.Request(s.entity,7,s,1033),"339 automatic fire accepts next round without backlog");
  s.hand=0;++s.generation;Check302(q.Take(s,1034).hand<0,"339 transfer discards pending old-owner feedback");
  s.at=1100;q.SetLive(1100,true);q.Request(s.entity,0,s,1100);
  Check302(q.Take(s,1181).hand<0,"339 slow-frame recoil expires after80ms");
  s.at=1200;q.SetLive(1200,true);q.Request(s.entity,1,s,1200);q.SetLive(1201,false);q.SetLive(1202,true);
  Check302(q.Take(s,1202).hand<0,"339 focus loss cannot replay recoil on return");
  s.at=1300;q.SetLive(1300,true);q.Request(s.entity,1,s,1300);q.SetLive(1500,true);s.at=1500;
  Check302(q.Take(s,1500).hand<0,"339 XR frame gap clears native shot receipt");
  Check302(!q.Request(0x9999,1,s,1500),"339 mismatched native weapon stays silent");
  s.empty=true;Check302(!q.Request(s.entity,1,s,1500),"339 stowed weapon stays silent");s.empty=false;
  s.active=false;Check302(!q.Request(s.entity,1,s,1500),"339 scripted/menu ownership stays silent");s.active=true;
  Check302(!q.Request(s.entity,1,s,1601),"339 stale physical-owner snapshot rejected");
  for(int profile:{15,18,32,40,99})Check302(fc3gunfire339::Shape(profile).duration==0,"339 nonbullet tools/explosives/unknowns excluded");
  for(int profile:{0,1,2,3,4,5,6,7,8,9,12,14,16,17,20,21,22,23,25,28,31,34,35,36,39,41,42,43})Check302(fc3gunfire339::Shape(profile).duration>0,"339 supported bullet-firearm profile has a bounded pulse");
 }
 wchar_t folder[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,folder);GetTempFileNameW(folder,L"gfi",0,path);
 WritePrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"preserved",path);
 LoadFeedback302(path);Check302(fc3xr::GunfireStrength339()==65,"339 default gunfire strength65percent");
 Check302(fc3xr::ConfigureGunfire339(80,path),"339 independent gunfire strength saves");LoadFeedback302(path);
 Check302(fc3xr::GunfireStrength339()==80&&!fc3xr::ConfigureGunfire339(101,path),"339 saved strength reloads and invalid input rejected");
 wchar_t sentinel[32];GetPrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"",sentinel,32,path);Check302(wcscmp(sentinel,L"preserved")==0,"339 gunfire settings preserve reload configuration");
 WritePrivateProfileStringW(L"Haptics",L"GunfireStrength",L"invalid",path);LoadFeedback302(path);Check302(fc3xr::GunfireStrength339()==65,"339 malformed strength defaults safely");
 fc3xr::ConfigureFeedback302(50,path);fc3xr::ConfigureGunfire339(80,path);
 for(int hand=0;hand<2;++hand){
  Ready339(hand);fc3xr::Gunshot339(0x1234,1);SyncControllers();
  Check302(sent302.size()==1&&sent302[0].action==g_actions[hand==0?kHapticRight:kHapticLeft]&&fabs(sent302[0].amplitude-.2f)<1e-6f&&sent302[0].duration==25000000,"339 real XR sync emits single holding-hand shot; overall and gunfire strengths compose");
  sent302.clear();SyncControllers();Check302(sent302.empty(),"339 held trigger without native shot receipt emits no pulse");
 }
 Ready339(1);fc3xr::Gunshot339(0x1234,0);fc3xr::InteractionPulse285(1);SyncControllers();
 Check302(sent302.size()==1&&sent302[0].duration==25000000,"339 holster/reload feedback wins simultaneous recoil");
 Ready339(0);fc3xr::Gunshot339(0x1234,1);fc3xr::MacheteHit();SyncControllers();Check302(sent302.size()==1&&sent302[0].duration==45000000,"339 melee feedback wins simultaneous right recoil");
 Ready339(1);fc3xr::Gunshot339(0x1234,7);syncResult302=XR_SESSION_NOT_FOCUSED;SyncControllers();sent302.clear();syncResult302=XR_SUCCESS;SyncControllers();Check302(sent302.empty(),"339 shipping focus transition discards recoil");
 Ready339(1);fc3xr::Gunshot339(0x1234,7);Publish339(0);SyncControllers();Check302(sent302.empty(),"339 shipping hand transfer rejects stale receipt");
 fc3xr::ConfigureGunfire339(0,path);Ready339(1);fc3xr::Gunshot339(0x1234,1);SyncControllers();Check302(sent302.empty(),"339 independent zero mutes gunfire");
 fc3xr::InteractionPulse285(1);SyncControllers();Check302(sent302.size()==1,"339 independent gunfire mute preserves holster/reload feedback");
 fc3xr::ConfigureGunfire339(100,path);fc3xr::ConfigureFeedback302(0,path);Ready339(0);fc3xr::Gunshot339(0x1234,1);SyncControllers();Check302(sent302.empty(),"339 overall zero also mutes gunfire");
 fc3xr::ConfigureFeedback302(100,path);Ready339(1);failApply302=true;fc3xr::Gunshot339(0x1234,7);SyncControllers();failApply302=false;sent302.clear();SyncControllers();Check302(sent302.empty(),"339 failed runtime apply cannot replay old shot");
 DeleteFileW(path);puts("PASS:339 native-shot receipt routing and feedback302 regressions;offline only");return 0;
}
