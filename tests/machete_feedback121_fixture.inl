#include "../src/xr_hit_haptics121.h"
UINT g_hitPulses121=0;
XRAPI_ATTR XrResult XRAPI_CALL HitHapticFixture121(XrSession session,const XrHapticActionInfo* info,const XrHapticBaseHeader* header)
{
    Check(session==(XrSession)0x1111&&info->action==(XrAction)0x2222,"hit haptic selects the right action and current session only");
    const auto* pulse=reinterpret_cast<const XrHapticVibration*>(header);
    Check(pulse->type==XR_TYPE_HAPTIC_VIBRATION&&pulse->duration==45000000&&fabs(pulse->amplitude-.55f)<1e-6,"hit pulse has bounded 45ms duration and restrained strength");++g_hitPulses121;return XR_SUCCESS;
}
DWORD WINAPI HitQueueThread121(void* queue){static_cast<fc3hit121::Queue*>(queue)->Request(1000);return 0;}
void TestMacheteFeedback121()
{
    fc3hit121::Queue queue;g_hitPulses121=0;
    auto consume=[&](DWORD now,bool ready=true,bool focused=true){return fc3hit121::Consume(queue,now,ready,focused,(XrSession)0x1111,(XrAction)0x2222,HitHapticFixture121);};
    HANDLE thread=CreateThread(nullptr,0,HitQueueThread121,&queue,0,nullptr);Check(thread!=nullptr&&WaitForSingleObject(thread,1000)==WAIT_OBJECT_0,"simulation thread can queue hit feedback without calling XR");CloseHandle(thread);
    queue.Request(1002);queue.Request(1003);Check(consume(1010)&&g_hitPulses121==1&&!consume(1011),"multi-slice requests coalesce to one XR pulse");
    queue.Request(1100);Check(!consume(1300)&&g_hitPulses121==1,"stale hit feedback is discarded");
    queue.Request(1400);Check(!consume(1401,true,false)&&!consume(1402),"unfocused feedback cannot play later on refocus");
    queue.Request(1500);Check(!consume(1501,false)&&!consume(1502),"unavailable XR actions consume and drop pending feedback");
    queue.Request(0xfffffff0u);Check(consume(15)&&g_hitPulses121==2,"feedback age handles tick-counter wrap");

    const auto oldCuts=g_macheteCuts121;const auto oldBlade=g_macheteBladeContact;g_macheteCuts121=MacheteCuts121{};g_macheteBladeContact=MacheteBladeContactState{};
    const UINT pawn=0x1234;
    auto begin=[&](ULONGLONG now,bool damage=true){MacheteCutBegin121(now,damage);g_macheteCuts121.queries=4;};
    begin(1000);MacheteCutHit121(pawn);MacheteBladeSetCooldown(pawn,1000);MacheteCutEnd121(true);
    for(ULONGLONG now=1100;now<=2000;now+=100){begin(now,false);Check(MacheteCutBroadHold121(pawn)&&MacheteBladeOnCooldown(pawn,now),"an embedded blade remains latched after the old cooldown expires, even at rest");MacheteCutEnd121(true);}
    begin(2020);MacheteCutEnd121(true);begin(2050);Check(MacheteBladeOnCooldown(pawn,2050),"a short shape-query gap cannot rearm a cut");MacheteCutEnd121(true);
    begin(2090);MacheteCutEnd121(true);begin(2100);Check(!MacheteBladeOnCooldown(pawn,2100),"70ms of full blade exit rearms a distinct cut");MacheteCutHit121(pawn);MacheteBladeSetCooldown(pawn,2100);MacheteCutEnd121(true);
    begin(2120);MacheteCutEnd121(true);begin(2190);MacheteCutEnd121(true);begin(2200);Check(!MacheteBladeOnCooldown(pawn,2200),"next valid cut need not wait the previous 350ms timer");MacheteCutHit121(pawn);MacheteCutEnd121(true);
    begin(2210);g_macheteCuts121.queries=3;MacheteCutEnd121(true);begin(2300);MacheteCutEnd121(true);begin(2330);Check(MacheteBladeOnCooldown(pawn,2330),"a failed or incomplete query set cannot establish a blade exit");MacheteCutEnd121(true);
    MacheteCutPause121();begin(2400);MacheteCutEnd121(true);begin(2430);Check(MacheteBladeOnCooldown(pawn,2430),"menu/tracking pause does not count as observed blade separation");MacheteCutEnd121(true);
    begin(2440);for(UINT i=0;i<63;++i)MacheteCutHit121(0x2000+i);
    Check(!MacheteCutTarget121(0x9999,true)&&MacheteCutTarget121(pawn)->latched,"full contact memory never evicts a latched target");
    const UINT reusable=g_macheteCuts121.targets[1].id;g_macheteCuts121.targets[1].latched=false;
    Check(MacheteCutTarget121(0x9999,true)&&!MacheteCutTarget121(reusable)&&MacheteCutTarget121(pawn)->latched,"long sessions reuse cleared targets while preserving embedded-contact protection");
    g_macheteCuts121=oldCuts;g_macheteBladeContact=oldBlade;
    const auto settings=g_macheteCollider118;wchar_t folder[MAX_PATH],ini[MAX_PATH];GetTempPathW(MAX_PATH,folder);swprintf_s(ini,L"%sfc3_hit121_%lu.ini",folder,GetCurrentProcessId());
    g_macheteCollider118.haptics=false;Check(SaveMacheteCollider118(ini),"hit vibration toggle saves with collider settings");g_macheteCollider118.haptics=true;LoadMacheteCollider118(ini);Check(!g_macheteCollider118.haptics,"hit vibration off survives restart");DeleteFileW(ini);g_macheteCollider118=settings;
    std::puts("PASS: feedback121 right-hand XR pulse, cross-thread queue, coalescing, stale/focus/availability guards; contact exit/reentry, embedded/stationary hold, quick repeated cuts, missing-query and pause rejection");
}

