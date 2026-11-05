// One pawn hit per contact episode. Native broad-phase overlap maintains a
// latched contact even when a short ray starts/ends inside the target. Only
// the original exact shape rays can authorize a new damaging contact.
struct MacheteCuts121 {
    struct Target {UINT id=0,seen=0;bool pawn=false,latched=false;ULONGLONG missing=0,lastHit=0;};
    Target targets[64];UINT count=0,step=0,queries=0,hits=0,rearms=0,feedback=0;
    bool active=false,damage=true;ULONGLONG now=0;
}g_macheteCuts121;
bool MacheteCutNeedsProbe121()
{
    for(UINT i=0;i<g_macheteCuts121.count;++i)if(g_macheteCuts121.targets[i].latched)return true;
    return false;
}
MacheteCuts121::Target* MacheteCutTarget121(UINT id,bool create=false)
{
    auto& c=g_macheteCuts121;for(UINT i=0;i<c.count;++i)if(c.targets[i].id==id)return &c.targets[i];
    if(!create||!id)return nullptr;
    if(c.count==64){UINT oldest=64;for(UINT i=0;i<64;++i)if(!c.targets[i].latched&&(oldest==64||c.targets[i].lastHit<c.targets[oldest].lastHit))oldest=i;
        if(oldest==64)return nullptr;auto& t=c.targets[oldest];t=MacheteCuts121::Target{};t.id=id;return &t;}
    auto& target=c.targets[c.count++];target=MacheteCuts121::Target{};target.id=id;return &target;
}
void MacheteCutTouch121(UINT id)
{
    auto& c=g_macheteCuts121;if(!c.active)return;
    if(auto* t=MacheteCutTarget121(id)){t->seen=c.step;t->missing=0;}
}
bool MacheteCutBroadHold121(UINT id)
{
    if(!g_macheteCuts121.active)return false;
    auto* t=MacheteCutTarget121(id);if(!t||!t->pawn||!t->latched)return false;
    MacheteCutTouch121(id);return true;
}
// -1 preserves legacy scenery cooldown; pawn contacts use exit/reentry.
int MacheteCutBlocked121(UINT id,ULONGLONG now)
{
    if(!g_macheteCuts121.active)return -1;
    auto* t=MacheteCutTarget121(id);if(!t||!t->pawn)return -1;
    return t->latched||(t->lastHit&&(now<t->lastHit||now-t->lastHit<90))?1:0;
}
void MacheteCutHit121(UINT id)
{
    auto& c=g_macheteCuts121;if(!c.active)return;
    if(auto* t=MacheteCutTarget121(id,true)){t->pawn=true;t->latched=true;t->seen=c.step;t->missing=0;t->lastHit=c.now;++c.hits;}
}
void MacheteCutPause121()
{
    auto& c=g_macheteCuts121;c.active=false;
    for(UINT i=0;i<c.count;++i)c.targets[i].missing=0;
}
void MacheteCutBegin121(ULONGLONG now,bool damage)
{
    auto& c=g_macheteCuts121;
    if(c.now&&(now<c.now||now-c.now>120))MacheteCutPause121();
    if(++c.step==0){c.step=1;for(UINT i=0;i<c.count;++i)c.targets[i].seen=0;}
    c.now=now;c.damage=damage;c.queries=0;c.active=true;
}
void MacheteCutEnd121(bool valid)
{
    auto& c=g_macheteCuts121;
    if(!valid||c.queries!=4){MacheteCutPause121();return;}
    for(UINT i=0;i<c.count;++i){auto& t=c.targets[i];if(!t.pawn||!t.latched||t.seen==c.step)continue;
        if(!t.missing)t.missing=c.now;
        if(c.now>=t.missing&&c.now-t.missing>=70&&c.now>=t.lastHit&&c.now-t.lastHit>=90){t.latched=false;t.missing=0;++c.rearms;}
    }
    c.active=false;
}
