#include "../src/fov_boost.cpp"
#include <cstdio>
#include <cstdlib>
namespace fc3 { void Log(const char*,...){} const wchar_t* DataFolder(){return L"";} }
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL %s\n",text);std::exit(1);}}
float g_fakeOverride=0;
void Seed(float* factor)
{
    g_healEnabled=false;g_residue=0;
    g_live=nullptr;g_liveWritten=0;g_liveOriginal=0;g_profileFactor=*factor;
    g_candidates[0]=factor;g_candidateCount=1;g_index=-1;g_wait=0;
    g_state=State::Testing;g_covered=false;g_refinements=0;g_uncoveredFrames=0;g_retryDelay=0;
    g_hiddenWeapon=false;g_blacklistCount=0;
    g_overrideTest=&g_fakeOverride;g_fakeOverride=0;g_overrideActive=false;g_overrideExternal=false;g_overrideDeg=0;
    fc3fov::SetNeed(1.3f,1.5f);
}
// The classic stable binding: written, widened, verified stable, restored, narrow again, written again, widened again.
void Bind(float* factor)
{
    Seed(factor);fc3fov::Update(true,true,1.3f,2.3f);
    for(int i=0;i<5;++i)fc3fov::Update(true,true,.4f,.65f);
    for(int i=0;i<6;++i)fc3fov::Update(true,true,.4f,.65f);
    for(int i=0;i<4;++i)fc3fov::Update(true,true,1.3f,2.3f);
    for(int i=0;i<4;++i)fc3fov::Update(true,true,.4f,.65f);
    Check(g_state==State::Holding&&g_live==factor,"bind: widen restore widen establishes binding");
}
int Probe(bool* bound=nullptr){int s=0,c=0,a=0;bool b=false;fc3fov::ProbeState(s,c,a,b);if(bound)*bound=b;return s;}
int main()
{
    float factor=1.8564f;Seed(&factor);fc3fov::Update(true,true,1.3032f,2.3168f);
    Check(factor>g_profileFactor,"candidate written");
    for(int i=0;i<5;++i)fc3fov::Update(true,true,1.5697f,2.7906f);
    Check(!g_live&&factor==g_profileFactor&&g_state!=State::Holding,"actual 75 to 65 degree door narrowing rejected and restored");
    factor=1.8564f;Seed(&factor);fc3fov::Update(true,true,1.3f,2.3f);
    for(int i=0;i<5;++i)fc3fov::Update(true,true,.4f,.65f);
    Check(g_state==State::Verifying&&factor>g_profileFactor,"widening seen: the value stays in place to tell stable from transient");
    for(int i=0;i<6;++i)fc3fov::Update(true,true,.4f,.65f);
    Check(g_state==State::Restoring&&factor==g_profileFactor,"stable widening must roll back");
    for(int i=0;i<4;++i)fc3fov::Update(true,true,.4f,.65f);
    Check(!g_live&&g_state==State::Testing,"natural animation widening without causal restoration rejected");
    factor=1.8564f;Seed(&factor);fc3fov::Update(true,true,1.3f,2.3f);
    for(int i=0;i<5;++i)fc3fov::Update(true,true,.4f,.65f);
    for(int i=0;i<6;++i)fc3fov::Update(true,true,.4f,.65f);
    for(int i=0;i<4;++i)fc3fov::Update(true,true,1.3f,2.3f);
    Check(g_state==State::Retesting&&factor>g_profileFactor,"restoration confirmed then repeat requested");
    for(int i=0;i<4;++i)fc3fov::Update(true,true,.4f,.65f);
    Check(g_state==State::Holding&&g_live==&factor,"widen restore widen establishes binding");
    const float held=factor;
    for(int i=0;i<95;++i)fc3fov::Update(true,true,1.57f,2.79f);
    Check(g_live==&factor&&factor==held&&!fc3fov::Covered(),"long cinematic narrowing retains known binding without falsely declaring coverage");
    factor=g_liveOriginal;fc3fov::Update(true,false,0,0);
    Check(factor==held,"known value held while camera unavailable");
    fc3fov::Update(true,true,.4f,.65f);Check(fc3fov::Covered(),"wide camera recovers coverage immediately after transition");
    fc3fov::Update(false,false,0,0);Check(factor==1.8564f&&!g_live&&!fc3fov::Covered(),"stop restores even without camera projection");
    factor=1.8564f;Seed(&factor);fc3fov::Update(true,true,1.3f,2.3f);fc3fov::Update(true,false,0,0);
    Check(factor==1.8564f,"lost camera during testing restores candidate");
    factor=1.8564f;Seed(&factor);fc3fov::Update(true,true,1.3f,2.3f);
    for(int i=0;i<5;++i)fc3fov::Update(true,true,.4f,.65f);
    fc3fov::Update(true,false,0,0);
    Check(factor==1.8564f&&g_state==State::Idle,"lost camera during verification restores candidate");
    g_live=&factor;g_liveOriginal=1.8564f;g_liveWritten=2.5f;factor=9;
    fc3fov::HoldKnownValue();Check(!g_live&&factor==9,"repurposed memory never overwritten");
    // 2026-10-08: the game saved the widened factor (4.61) into the profile; the real value goes back over it in memory
    // (verified by narrowing, then restored, then narrowed again), after which the normal widening continues from the real value.
    factor=4.61481f;Seed(&factor);g_residue=4.61481f;g_residueRestore=1.8564f;g_profileFactor=1.8564f;g_scanOtherFactor=4.61481f;
    fc3fov::Update(true,true,.12f,.215f);Check(factor==1.8564f,"residue: the real profile value is written over the saved widened one");
    for(int i=0;i<5;++i)fc3fov::Update(true,true,1.3f,2.3f);
    Check(g_state==State::Verifying&&factor==1.8564f,"residue: narrowing seen, left in place for the stability check");
    for(int i=0;i<6;++i)fc3fov::Update(true,true,1.3f,2.3f);
    Check(g_state==State::Restoring&&factor==4.61481f,"residue: stable narrowing, residue put back to confirm");
    for(int i=0;i<4;++i)fc3fov::Update(true,true,.12f,.215f);
    Check(g_state==State::Retesting&&factor==1.8564f,"residue: wide view returned with the residue, real value written again");
    for(int i=0;i<4;++i)fc3fov::Update(true,true,1.3f,2.3f);
    Check(g_state==State::Holding&&g_live==&factor&&g_liveOriginal==1.8564f&&g_residue==0,"residue: corrected, the real value is the one restored on stop");
    Check(factor>1.8564f&&factor<=1.8564f*2.6f&&g_liveWritten==factor,"residue: widened at once from the real value, capped");
    for(int i=0;i<10;++i)fc3fov::Update(true,true,.4f,.65f);Check(factor==g_liveWritten&&fc3fov::Covered(),"residue: wide view held");
    fc3fov::Update(false,false,0,0);Check(factor==1.8564f,"residue: stop restores the real profile value, never the residue");
    // the weapon camera must never drive a widening: a write is capped to 2.6 x the profile value
    factor=1.8564f;Seed(&factor);fc3fov::Update(true,true,1.5544f,2.79f);
    Check(factor==1.8564f||factor<=1.8564f*2.6f,"a test write never exceeds 2.6 x the profile value");

    // 2026-10-15 vehicle (run 50192): the write widens the view for a tick, then the game takes the value back.
    // Candidate 99 pumped such a value every frame and the vehicle view stayed native (run 67572): now it is
    // restored and never tested again this session.
    factor=1.8564f;Seed(&factor);fc3fov::Update(true,true,1.3f,2.3f);const float written=factor;
    Check(written>1.8564f,"transient: candidate written");
    for(int i=0;i<5;++i)fc3fov::Update(true,true,.12f,.215f);
    Check(g_state==State::Verifying&&factor==written,"transient: widening seen, value left in place");
    factor=1.8564f;for(int i=0;i<6;++i)fc3fov::Update(true,true,1.2f,2.12f);
    Check(!g_live&&factor==1.8564f&&g_blacklistCount==1&&g_blacklist[0]==&factor,"transient: a widening that decayed while the game took the value back is restored and excluded");
    g_state=State::Testing;g_index=-1;g_wait=0;fc3fov::Update(true,true,1.3f,2.3f);
    Check(factor==1.8564f&&!g_live,"transient: the excluded candidate is never written again");
    factor=4.61481f;Seed(&factor);g_residue=4.61481f;g_residueRestore=1.8564f;g_profileFactor=1.8564f;g_scanOtherFactor=4.61481f;
    fc3fov::Update(true,true,.12f,.215f);for(int i=0;i<5;++i)fc3fov::Update(true,true,1.3f,2.3f);
    factor=4.61481f;for(int i=0;i<6;++i)fc3fov::Update(true,true,.3f,.53f);
    Check(g_state==State::Restoring&&g_blacklistCount==0,"residue: a decaying residue test is never excluded");
    g_residue=0;

    // The engine's own gfx_FOVOverride (degrees) for hidden-weapon views the scale factor cannot cover.
    factor=1.8564f;Bind(&factor);const float heldFactor=factor;
    fc3fov::SetHiddenWeapon(true);fc3fov::Update(true,true,1.3f,2.3f);
    bool bound=false;
    Check(g_fakeOverride>100.f&&g_fakeOverride<=170.f&&g_overrideActive&&Probe(&bound)==8&&bound,"override: a narrow hidden-weapon view sets the engine override to the wanted degrees");
    const float first=g_fakeOverride;
    for(int i=0;i<3;++i)fc3fov::Update(true,true,1.0f,1.78f);
    Check(g_fakeOverride>first&&g_fakeOverride<=first*1.5f+1e-4f&&g_fakeOverride<=170.f,"override: a view still short of the want raises it by a bounded step");
    const float peak=g_fakeOverride;
    for(int i=0;i<3;++i)fc3fov::Update(true,true,.2f,.36f);
    Check(g_fakeOverride<peak&&g_fakeOverride>=peak*.67f-1e-4f,"override: an over-wide view lowers it by a bounded step");
    Check(factor==heldFactor&&g_live==&factor&&g_state==State::Holding,"override: the held scale factor is untouched meanwhile");
    g_fakeOverride=0;fc3fov::Update(true,true,1.3f,2.3f);
    Check(!g_overrideActive&&g_fakeOverride==0&&g_overrideExternal,"override: a value changed outside the mod is left alone");
    fc3fov::Update(true,true,1.3f,2.3f);Check(g_overrideActive&&g_fakeOverride>100.f,"override: re-armed once the variable is 0 again");
    const float kept=g_fakeOverride;fc3fov::Update(true,false,0,0);Check(g_fakeOverride==kept&&g_overrideActive,"override: kept through a menu");
    fc3fov::SetHiddenWeapon(false);fc3fov::Update(true,true,1.3f,2.3f);
    Check(!g_overrideActive&&g_fakeOverride==0&&Probe()==static_cast<int>(State::Holding),"override: cleared the moment a weapon is visible");
    fc3fov::SetHiddenWeapon(true);fc3fov::Update(true,true,.4f,.65f);
    Check(!g_overrideActive&&g_fakeOverride==0,"override: a hidden-weapon view the scale factor already covers needs no override");
    fc3fov::Update(true,true,1.3f,2.3f);Check(g_overrideActive,"override: armed when that view narrows");
    fc3fov::Update(true,true,.4f,.65f);Check(g_overrideActive,"override: stays on while it is what covers the view");
    fc3fov::Update(false,false,0,0);Check(g_fakeOverride==0&&!g_overrideActive&&factor==1.8564f,"override: stop clears it and restores the factor");
    g_fakeOverride=55;fc3fov::SetHiddenWeapon(true);factor=1.8564f;Seed(&factor);g_fakeOverride=55;fc3fov::Update(true,true,1.3f,2.3f);
    Check(g_fakeOverride==55&&!g_overrideActive&&factor>1.8564f,"override: a console value already set is never overwritten; the scale factor path still runs");
    g_fakeOverride=0;factor=1.8564f;Seed(&factor);fc3fov::SetHiddenWeapon(true);fc3fov::Update(true,true,1.3f,2.3f);
    Check(g_overrideActive&&factor==1.8564f,"override: no test write while the override owns the view");
    fc3fov::SetHiddenWeapon(false);fc3fov::Update(false,false,0,0);g_overrideTest=nullptr;
    std::puts("PASS: residue in the profile corrected in memory and on stop; recorded door narrowing, causal widen/verify/restore/widen, transient retention, immediate coverage recovery, stop/lost-camera restoration, stale ownership; transient (vehicle) candidates restored and excluded; engine gfx_FOVOverride for narrow hidden-weapon views: set, tuned both ways, bounded, cleared on weapon/stop, kept through menus, never fights a console value, no test writes meanwhile");
}
