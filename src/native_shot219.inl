// A trigger request is not evidence of a shot. Preserve physical chamber state
// until the same local weapon's native loaded count actually decreases.
struct NativeShot219 {
    bool pending=false,waitRelease=false;
    int before=-1;
    UINT state=0,wrapper=0,weapon=0,epoch=0;
    ULONGLONG until=0;
};
bool PollNativeShot219(NativeShot219& shot,bool active,bool ready,UINT state,UINT wrapper,int loaded,ULONGLONG now)
{
    if(!shot.pending)return false;
    const bool same=active&&ready&&loaded>=0&&shot.state==state&&shot.wrapper==wrapper&&
        shot.weapon&&shot.weapon==ReloadRead(wrapper,0x48)&&shot.epoch==g_armPoseEpoch;
    const bool fired=same&&loaded<shot.before;
    if(fired||!same||now>=shot.until){shot.pending=false;shot.waitRelease=true;}
    return fired;
}
void BeginNativeShot219(NativeShot219& shot,UINT state,UINT wrapper,int loaded,ULONGLONG now)
{
    shot.pending=true;shot.before=loaded;shot.state=state;shot.wrapper=wrapper;
    shot.weapon=ReloadRead(wrapper,0x48);shot.epoch=g_armPoseEpoch;shot.until=now+180;
}
