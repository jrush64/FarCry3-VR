void TestPumpFireLock()
{
    const bool previousPump=g_pumpEnabled;g_pumpEnabled=true;g_chambered=true;g_pumpFire=PumpFireAllowance{};g_lastShotFrame=-100000;g_lastTrigger=0;g_twoHand=false;
    fc3pad::Controllers c={};c.active=true;ArmJoints right={},left={};right.valid=left.valid=true;
    right.w[0]=left.w[0]=.25;right.w[1]=left.w[1]=-.1;right.w[2]=.25;left.w[2]=.65;
    g_frame=30000;c.rightTrigger=.6f;fc3pad::Publish(c);UpdatePump(right,left);Check(g_chambered&&fc3pad::FireOutput()==0,"partial trigger cannot fire without the pump detecting the shot");
    c.rightTrigger=1;fc3pad::Publish(c);++g_frame;UpdatePump(right,left);const LONG firstShot=g_lastShotFrame;
    Check(!g_chambered&&fc3pad::FireOutput()==255,"first full pull reaches the game once and locks the chamber");
    for(int i=0;i<1200;++i)
    {
        ++g_frame;c.rightTrigger=i%2?1.f:0.f;fc3pad::Publish(c);UpdatePump(right,left);
        Check(!g_chambered&&g_lastShotFrame==firstShot&&fc3pad::FireOutput()==0,"1200-frame repeated trigger taps cannot reopen the shot allowance");
    }
    const bool priorManual=g_manualReloadEnabled;g_manualReloadEnabled=true;
    c.rightTrigger=0;c.x=false;fc3pad::Publish(c);UpdatePump(right,left);
    c.x=true;c.leftGrip=1;fc3pad::Publish(c);++g_frame;UpdatePump(right,left);
    Check(!g_chambered&&fc3pad::FireOutput()==0,"manual shell pickup X cannot chamber a spent shotgun");
    c.b=true;fc3pad::Publish(c);++g_frame;UpdatePump(right,left);
    Check(!g_chambered,"manual calibration/save X cannot bypass pump state");
    c.x=false;c.b=false;c.leftGrip=0;fc3pad::Publish(c);UpdatePump(right,left);g_manualReloadEnabled=priorManual;
    std::puts("PASS: manual pickup/save X cannot chamber; full physical rear/forward pump remains required");
    // The tracking-loss branch must still update the lock, not return early.
    g_motionGun=g_eyeViewsSet=g_handRightValid=true;g_handLeftValid=false;c.rightTrigger=1;fc3pad::Publish(c);UpdateTwoHand();Check(fc3pad::FireOutput()==0,"lost left-hand tracking cannot bypass pump fire lock");g_handLeftValid=true;
    float priorHand[16];memcpy(priorHand,g_handLeft,sizeof(priorHand));memset(g_handLeft,0,sizeof(priorHand));g_handLeft[0]=g_handLeft[5]=g_handLeft[10]=g_handLeft[15]=1;
    g_handLeft[3]=.25f;g_handLeft[7]=-.08f;g_handLeft[11]=.75f;g_frameGunValid=true;g_frameGunMove=HandIdentity();g_twoHand=true;
    ++g_frame;UpdatePump(right,left);Check(g_pumpStage==0&&!g_chambered,"grabbing alone does not chamber");
    g_handLeft[11]-=.08f;++g_frame;UpdatePump(right,left);Check(g_pumpStage==1&&!g_chambered&&fc3pad::FireOutput()==0,"rearward stroke alone still blocks fire");
    g_handLeft[11]+=.08f;++g_frame;UpdatePump(right,left);Check(g_chambered&&fc3pad::FireOutput()==0,"completed pump waits for trigger release before firing again");
    c.rightTrigger=0;fc3pad::Publish(c);++g_frame;UpdatePump(right,left);c.rightTrigger=1;fc3pad::Publish(c);++g_frame;UpdatePump(right,left);
    Check(!g_chambered&&g_lastShotFrame!=firstShot&&fc3pad::FireOutput()==255,"completed physical pump and fresh full pull allow exactly the next shot");
    g_twoHand=false;for(int i=0;i<200;++i){++g_frame;UpdatePump(right,left);if(i>=6)Check(fc3pad::FireOutput()==0,"held trigger expires once without reopening at frame100");}
    g_pumpEnabled=false;UpdatePump(right,left);Check(fc3pad::FireOutput()==255,"pump toggle restores ordinary trigger output");
    // A spent M133 cannot lock unrelated weapons, or become chambered just
    // because the user briefly selects a pistol, rifle or semi-auto shotgun.
    const int priorProfile=g_weaponMechanicProfile;g_pumpEnabled=true;g_chambered=false;g_pumpFire.passing=false;g_pumpFire.waitRelease=true;
    const fc3weapon::Geometry otherWeapons[]={
        {177904,32682,32,240,294,4470,57}, // 1911
        {341536,65556,32,0,2853,13368,57}, // P416
        {127600,30018,32,240,4908,7401,57}}; // SPAS12
    const LONG spentShot=g_lastShotFrame;
    for(const auto& geometry:otherWeapons)
    {
        g_weaponMechanicProfile=fc3weapon::ProfileIndex(geometry);Check(g_weaponMechanicProfile>=0,"other player weapon recognized");
        c.rightTrigger=.5f;fc3pad::Publish(c);++g_frame;UpdatePump(right,left);
        Check(fc3pad::FireOutput()>0&&!g_chambered&&g_lastShotFrame==spentShot,"partial native trigger works on other weapon without chambering M133");
        for(int n=0;n<10;++n){c.rightTrigger=n%2?1.f:0.f;fc3pad::Publish(c);++g_frame;UpdatePump(right,left);Check(n%2?fc3pad::FireOutput()==255:fc3pad::FireOutput()==0,"other weapons keep repeated native trigger pulls");}
        g_twoHand=true;g_frameGunValid=true;g_frameGunMove=HandIdentity();g_slide=-.5;g_slideGrab=.1;g_pumpAnimView[2]=-.09;
        Mat4 support={};Check(LeftMove(left,right,support),"non-pump support hand");
        for(int k=0;k<16;++k)Check(support.m[k]==g_frameGunMove.m[k],"M133 slide correction cannot move another weapon's support hand");
    }
    g_twoHand=false;g_weaponMechanicProfile=fc3weapon::ProfileIndex({257896,47478,40,336,486,4071,57});
    ++g_frame;UpdatePump(right,left);Check(!g_chambered&&fc3pad::FireOutput()==0,"switching back preserves spent M133 and blocked trigger");
    c.rightTrigger=0;fc3pad::Publish(c);++g_frame;UpdatePump(right,left);c.rightTrigger=1;fc3pad::Publish(c);++g_frame;UpdatePump(right,left);
    Check(!g_chambered&&fc3pad::FireOutput()==0,"switch and fresh trigger cannot bypass physical pump");
    g_weaponMechanicProfile=priorProfile;g_pumpAnimView[2]=0;
    std::puts("PASS: 1911/P416/SPAS12 keep native trigger and support grip; M133 spent chamber survives switches without bypass");
    memcpy(g_handLeft,priorHand,sizeof(priorHand));g_pumpEnabled=previousPump;g_chambered=true;g_pumpFire=PumpFireAllowance{};g_lastShotFrame=-100000;g_lastTrigger=0;g_twoHand=false;g_motionGun=false;
    fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);c.rightTrigger=0;fc3pad::Publish(c);
    std::puts("PASS: actual filtered RT output; partial-pull parity; 1200 spam frames stay locked; tracking loss; full rear/forward pump plus trigger release; initial-shot allowance never reopens");
}
