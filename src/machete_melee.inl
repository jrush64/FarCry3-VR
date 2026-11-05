// Motion requests the game's established R3 melee action. Native targeting,
// range and takedown rules still resolve hits; there is no blade collision API.
struct MacheteStroke
{
    V3 previous={},start={};ULONGLONG time=0,quiet=0,fastStart=0;
    bool seeded=false,armed=false;UINT fast=0;
    double speed=0,distance=0;
};
bool StepMacheteStroke(MacheteStroke& s,V3 position,ULONGLONG now)
{
    if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z))
    {s=MacheteStroke{};return false;}
    if(!s.seeded){s.previous=position;s.time=now;s.seeded=true;return false;}
    const ULONGLONG elapsed=now>=s.time?now-s.time:0;
    const V3 delta={position.x-s.previous.x,position.y-s.previous.y,position.z-s.previous.z};
    const double travel=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
    const V3 before=s.previous;s.previous=position;s.time=now;
    if(elapsed<5||elapsed>80||travel>.35)
    {s=MacheteStroke{};s.previous=position;s.time=now;s.seeded=true;return false;}
    s.speed=travel*1000/elapsed;
    if(s.speed>10){s=MacheteStroke{};s.previous=position;s.time=now;s.seeded=true;return false;}
    if(!s.armed)
    {
        if(s.speed<1.2)
        {
            if(!s.quiet)s.quiet=now;
            s.armed=true;s.fast=0; // re-arm as the cut slows; no pause between successive swings
        }
        else s.quiet=0;
        return false;
    }
    if(s.speed<1.1){s.fast=0;s.fastStart=0;s.distance=0;return false;}
    if(!s.fast||now-s.fastStart>300){s.start=before;s.fastStart=now-elapsed;s.fast=0;}
    ++s.fast;
    const V3 stroke={position.x-s.start.x,position.y-s.start.y,position.z-s.start.z};
    s.distance=std::sqrt(stroke.x*stroke.x+stroke.y*stroke.y+stroke.z*stroke.z);
    if(s.fast<2||s.distance<.11)return false;
    s.armed=false;s.quiet=0;s.fast=0;return true;
}
#pragma pack(push,4)
struct MacheteMeleeRecord
{
    UINT magic=0x4D4D5031,version=1,bytes=sizeof(MacheteMeleeRecord),event=0;
    ULONGLONG ms=0;LONG frame=0;UINT sequence=0,flags=0;
    float speed=0,distance=0,trigger=0,grip=0;
    float controller[16]={};
};
#pragma pack(pop)
struct MacheteMeleeState
{
#ifdef FC3_AMMO_TEST
    bool enabled=false;
#else
    bool enabled=false; // contact volumes own damage; never synthesize attacks during movement
#endif
    MacheteStroke stroke;
    HANDLE file=INVALID_HANDLE_VALUE;wchar_t path[MAX_PATH]={};
    ULONGLONG lastSample=0,lastFlush=0,retry=0,requested=0,cooldown=0,lastNative=0;
    UINT sequence=0,observed=0,bytes=0;DWORD error=0;bool pending=false,triggerLatched=true,full=false;
    UINT epoch=0xffffffffu;float scale=0;
} g_macheteMelee;
void CloseMacheteMeleeProbe()
{
    if(g_macheteMelee.file==INVALID_HANDLE_VALUE)return;
    FlushFileBuffers(g_macheteMelee.file);CloseHandle(g_macheteMelee.file);
    g_macheteMelee.file=INVALID_HANDLE_VALUE;
}
bool OpenMacheteMeleeProbe(ULONGLONG now)
{
    auto& m=g_macheteMelee;if(m.file!=INVALID_HANDLE_VALUE)return true;
    if(m.full||now<m.retry)return false;m.retry=now+1000;
    wchar_t folder[MAX_PATH]={};
#ifdef FC3_AMMO_TEST
    GetTempPathW(MAX_PATH,folder);
#else
    wcscpy_s(folder,fc3::DataFolder());
#endif
    _snwprintf_s(m.path,MAX_PATH,_TRUNCATE,L"%sfc3_machete_melee_%lu_%llu.bin",folder,GetCurrentProcessId(),now);
    m.file=CreateFileW(m.path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(m.file==INVALID_HANDLE_VALUE){m.error=GetLastError();Log("[MACHETE MELEE] capture open failed, error %lu; will retry",m.error);return false;}
    m.bytes=0;m.lastSample=0;m.lastFlush=now;m.error=0;
    Log("[MACHETE MELEE] automatic motion/native-swipe capture opened; hits require live target confirmation");return true;
}
void WriteMacheteMelee(UINT event,ULONGLONG now)
{
    auto& m=g_macheteMelee;
    if(!m.enabled||!OpenMacheteMeleeProbe(now))return;
    if(!event&&now-m.lastSample<50)return;m.lastSample=now;
    if(m.bytes>=8u*1024*1024)
    {Log("[MACHETE MELEE] motion trace reached 8 MB; attack/response events remain in the main log");m.full=true;CloseMacheteMeleeProbe();return;}
    const auto c=fc3pad::Latest();MacheteMeleeRecord r;
    r.event=event;r.ms=now;r.frame=g_frame;r.sequence=m.sequence;
    r.flags=(MacheteHeld()?1:0)|(MacheteReady()?2:0)|(m.stroke.armed?4:0)|(m.pending?8:0)|
        ((fc3pad::ButtonsOutput()&0x0080)?16:0)|(c.rightStickClick?32:0)|(c.leftStickClick?64:0)|
        (g_handRightValid?128:0);
    r.speed=static_cast<float>(m.stroke.speed);r.distance=static_cast<float>(m.stroke.distance);
    r.trigger=c.rightTrigger;r.grip=c.rightGrip;memcpy(r.controller,g_handBody[0],sizeof(r.controller));
    DWORD written=0;
    if(!WriteFile(m.file,&r,sizeof(r),&written,nullptr)||written!=sizeof(r))
    {m.error=GetLastError()?GetLastError():ERROR_WRITE_FAULT;Log("[MACHETE MELEE] capture write failed, error %lu",m.error);CloseMacheteMeleeProbe();return;}
    m.bytes+=sizeof(r);
    if(event||now-m.lastFlush>=1000)
    {
        if(!FlushFileBuffers(m.file)){m.error=GetLastError();Log("[MACHETE MELEE] capture flush failed, error %lu",m.error);CloseMacheteMeleeProbe();}
        m.lastFlush=now;
    }
}
void CancelMacheteBladeStroke();
void StopMacheteMelee()
{
    CancelMacheteBladeStroke();
    auto& m=g_macheteMelee;
    if(m.enabled&&m.file!=INVALID_HANDLE_VALUE)WriteMacheteMelee(4,GetTickCount64());
    CloseMacheteMeleeProbe();m.stroke=MacheteStroke{};m.pending=false;m.cooldown=0;
    m.triggerLatched=true;fc3pad::CancelMacheteAttack();
}
void BeginMacheteBladeStroke(UINT sequence,ULONGLONG now);
bool RequestMacheteMelee(const char* reason,ULONGLONG now)
{
    auto& m=g_macheteMelee;
    if(now<m.cooldown||!fc3pad::RequestMacheteAttack())return false;
    if(m.pending){Log("[MACHETE MELEE] request %u: no blade response before the next physical input; new stroke proceeds",m.sequence);WriteMacheteMelee(3,now);}
    m.pending=true;m.requested=now;m.cooldown=now+220;++m.sequence; // input pulse needs a released edge; engine owns animation pacing
    BeginMacheteBladeStroke(m.sequence,now);
    // The native swipe temporarily removes firearm body draws. Retain the
    // held tool through request/animation/return without disabling safe areas.
    g_machete.motionUntil=(std::max)(g_machete.motionUntil,now+2200);
    Log("[MACHETE MELEE] request %u: %s, %.2f m/s, %.3f m stroke; ONE native R3 pulse; damage unconfirmed",
        m.sequence,reason,m.stroke.speed,m.stroke.distance);
    if(g_animation.enabled)
    {
        g_animation.sample=true;g_animation.until=GetTickCount64()+10000;
        AnimationRecord r;r.kind=2;r.event=reason[0]=='p'?8:16;r.requested=m.sequence;
        AnimationStamp(r);AnimationStore(r);
    }
    WriteMacheteMelee(1,now);return true; // retain the spent-stroke latch until the hand slows
}
void ObserveNativeMachete(ULONGLONG now)
{
    auto& m=g_macheteMelee;m.lastNative=now;
    if(!m.enabled||!MacheteHeld()||!m.pending)return;
    m.pending=false;m.observed=m.sequence;
    Log("[MACHETE MELEE] request %u: native machete blade draw observed after %llu ms; this confirms a swipe visual, NOT a hit",
        m.sequence,now-m.requested);WriteMacheteMelee(2,now);
    if(g_animation.enabled)
    {
        AnimationRecord r;r.kind=2;r.event=32;r.requested=m.sequence;
        AnimationStamp(r);AnimationStore(r);
    }
}
void TickMacheteMelee(const fc3pad::Controllers& c,ULONGLONG now)
{
    auto& m=g_macheteMelee;
    // Consume every press even when drawing, busy or cooling down. Never queue
    // a held trigger as an attack for later; draw/stow also requires release.
    const bool trigger=c.rightTrigger>.7f&&!m.triggerLatched;
    if(c.rightTrigger<.3f)m.triggerLatched=false;else if(c.rightTrigger>.7f)m.triggerLatched=true;
    if(!m.enabled||!MacheteHeld())return;
    if((c.rightStickClick&&!g_macheteHoldToDraw)||c.leftStickClick)fc3pad::CancelMacheteAttack();
    if(m.pending&&now-m.requested>1800)
    {
        Log("[MACHETE MELEE] request %u: no native blade draw observed; no automatic retry or damage claim",m.sequence);
        m.pending=false;WriteMacheteMelee(3,now);
    }
    const bool eligible=MacheteReady()&&now-g_machete.started>=1000&&
        g_machete.handFrame>=g_frame-1&&g_machete.handFrame<=g_frame&&
        (!c.rightStickClick||g_macheteHoldToDraw)&&!c.leftStickClick&&now>=m.cooldown&&
        c.active&&g_handRightValid; // continuous blade draws must not disable the swing detector
    if(m.epoch!=g_armPoseEpoch||m.scale!=g_worldScale)
    {m.stroke=MacheteStroke{};m.epoch=g_armPoseEpoch;m.scale=g_worldScale;}
    if(!eligible){m.stroke=MacheteStroke{};WriteMacheteMelee(0,now);return;}
    const float* p=g_handBody[0];const double scale=g_worldScale;
    V3 position={};
    // The blade tip includes wrist rotation; body coordinates exclude head
    // turns and player locomotion from the velocity measurement.
    if(!MacheteStrokePosition116(position)&&scale>.01)position={p[3]/scale,p[7]/scale,p[11]/scale};
    const bool swing=scale>.01&&StepMacheteStroke(m.stroke,position,now);
    if(trigger)RequestMacheteMelee("right-trigger diagnostic",now);
    else if(swing)RequestMacheteMelee("physical swing",now); // always use the proven native attack path
    WriteMacheteMelee(0,now);
}
