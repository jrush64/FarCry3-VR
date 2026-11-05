// Native12D63A1 is reached only after a real convex contact was resolved
// and the player/self contact was rejected. Restore the native surface-impact
// branch here, never the camera-selected character search at12D68C6.
struct MacheteWorldState126 {
    struct Contact {V3 point={};ULONGLONG until=0;} contacts[16];
    UINT allowed=0,rejected=0,suppressed=0,feedback=0;ULONGLONG lastFeedback=0;
}g_macheteWorld126;
void* g_macheteWorldOriginal126=nullptr;
bool MacheteWorldTouch126(V3 point,ULONGLONG now)
{
    auto& w=g_macheteWorld126;const auto& b=g_macheteBladeContact;
    if(!std::isfinite(Length(point)))return false;
    const V3 motion=Sub(b.end,b.start);const double length2=Dot(motion,motion);
    const double t=length2>1e-9?(std::max)(0.,(std::min)(1.,Dot(Sub(point,b.start),motion)/length2)):0;
    if(Length(Sub(point,Add(b.start,Scale(motion,t))))>b.half+b.radius+.05){++w.rejected;return false;}
    UINT oldest=0;for(UINT i=0;i<16;++i){if(now<w.contacts[i].until&&Length(Sub(point,w.contacts[i].point))<.15){++w.suppressed;return false;}if(w.contacts[i].until<w.contacts[oldest].until)oldest=i;}
    w.contacts[oldest]={point,now+b.cooldownMs};++w.allowed;return true;
}
void __cdecl MacheteWorldContact126(UINT* saved)
{
    if(!saved||!MacheteBladeInCall()||g_macheteActor119.pass)return;
    const auto& b=g_macheteBladeContact;
    if(!b.queryApplied118||!g_macheteCuts121.active||!g_macheteCuts121.damage)return;
    const UINT frame=saved[2],stack=saved[3]+4;float xyz[3]={};
    if(MacheteNativeRead(frame,12)!=b.callConfig118||!ReloadTraceRead(stack+0x28,xyz,12))return;
    const ULONGLONG now=GetTickCount64();const V3 point={xyz[0],xyz[1],xyz[2]};
    if(!MacheteWorldTouch126(point,now))return;
    *reinterpret_cast<BYTE*>(frame+0x14)=0; // native material-impact ray starts at rewritten blade origin
    auto& w=g_macheteWorld126;
    if(g_macheteCollider118.haptics&&(!w.lastFeedback||now-w.lastFeedback>=90)){fc3xr::MacheteHit();w.lastFeedback=now;++w.feedback;}
    if(w.allowed<=16)Log("[MACHETE WORLD126] physical surface %.3f/%.3f/%.3f: native material impact branch permitted; character search unchanged",point.x,point.y,point.z);
}
