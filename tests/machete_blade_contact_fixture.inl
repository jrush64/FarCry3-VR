// Offline checks for the blade-contact sweep gate, per-target cooldown and
// blade profile table. No engine, no hooks: pure functions only.
void TestMacheteBladeContact()
{
    V3 start,end,axis;float half=0;double speed=0;
    const V3 base0={0,0,1},tip0={0,0,1.35},base1={.05,0,1},tip1={.05,0,1.35};
    const V3 base2={.01,0,1},tip2={.01,0,1.35};
    Check(!MacheteBladeSweep(base0,tip0,base2,tip2,16,1.0,start,end,axis,half,speed),"a 1 cm step (0.6 m/s) is below the speed floor: no contact call");
    Check(MacheteBladeSweep(base0,tip0,base1,tip1,16,1.0,start,end,axis,half,speed),"a 5 cm step in 16 ms (3.1 m/s) sweeps");
    Check(speed>3.0&&speed<3.3&&fabs(half-.175f)<1e-4f&&fabs(axis.z-1)<1e-9&&fabs(start.x)<1e-9&&fabs(end.x-.05)<1e-9,"sweep runs from the previous blade centre to the current one along the blade axis");
    Check(!MacheteBladeSweep(base0,tip0,base1,tip1,2,1.0,start,end,axis,half,speed),"sub-4 ms samples are rejected");
    Check(!MacheteBladeSweep(base0,tip0,base1,tip1,200,1.0,start,end,axis,half,speed),"stale samples over 120 ms are rejected");
    const V3 jumpBase={2,0,1},jumpTip={2,0,1.35};
    Check(!MacheteBladeSweep(base0,tip0,jumpBase,jumpTip,16,1.0,start,end,axis,half,speed),"a 2 m jump (tracking loss) is rejected");
    auto& b=g_macheteBladeContact;b.cooldownCount=0;b.cooldownMs=350;
    Check(!MacheteBladeOnCooldown(7,1000),"unknown target is not on cooldown");
    MacheteBladeSetCooldown(7,1000);
    Check(MacheteBladeOnCooldown(7,1200)&&!MacheteBladeOnCooldown(7,1400)&&!MacheteBladeOnCooldown(8,1200),"one damaging contact per target per 350 ms; other targets unaffected");
    for(UINT i=0;i<20;++i)MacheteBladeSetCooldown(100+i,2000);
    Check(b.cooldownCount==16&&MacheteBladeOnCooldown(119,2100),"cooldown table holds 16 targets and evicts the oldest");
    Check(kMacheteBlades[0].partCount==3&&kMacheteBlades[0].vb==71720&&kMacheteBlades[1].partCount==2&&kMacheteBlades[1].vb==83720&&kMacheteBlades[1].starts[1]==6000&&kMacheteBlades[1].counts[1]==390,"blade table: machete three sections, tanto_tp two with the 390-index blade second");
    Check(kMacheteBlades[1].bladeTip.z>kMacheteBlades[1].bladeBase.z&&fabs((kMacheteBlades[1].bladeTip.z-kMacheteBlades[1].bladeBase.z)-.3127)<1e-3,"tanto blade axis is 0.31 m long in model space");
    Check(!b.enabled&&!b.installed,"offline build never enables or installs blade contact");
    Check(fabs(b.bladeRadius-.025f)<1e-6f&&!b.strikeAssist&&b.strikeSpeed==2.5&&b.rearmSpeed==1.2&&b.damageScale==2.f,"candidate 113 defaults: one strike per swing at 2.5 m/s, re-arm below 1.2 m/s, double damage");
    {const V3 d=MacheteStrikeDirection({.4,0,-.5},.35);Check(fabs(d.z/std::hypot(d.x,d.y)+.35)<1e-9&&d.x>0,"a strike aimed down at the floor is lifted to the downward pitch limit (19 degrees), keeping its horizontal direction");}
    {const V3 d=MacheteStrikeDirection({.3,.4,.05},.35);Check(fabs(d.x-.6)<.02&&fabs(d.y-.8)<.02&&d.z>0,"a near-level strike keeps its direction");}
    {const V3 d=MacheteStrikeDirection({0,0,-.6},.35);Check(Length(d)<1e-9,"a tip straight below the head gives no strike direction");}
    bool armed=true;
    Check(!MacheteStrikeStep(armed,1.5,2.5,1.2)&&armed,"a slow movement never strikes");
    Check(MacheteStrikeStep(armed,3.0,2.5,1.2)&&!armed,"a real swing strikes on its first fast sample");
    Check(!MacheteStrikeStep(armed,6.0,2.5,1.2)&&!MacheteStrikeStep(armed,2.0,2.5,1.2)&&!armed,"the same swing never strikes twice, even when it speeds up or slows a little");
    Check(!MacheteStrikeStep(armed,1.0,2.5,1.2)&&armed,"the blade slowing down re-arms at once, no timer");
    Check(MacheteStrikeStep(armed,2.6,2.5,1.2),"the very next swing strikes again");
    int swings=0;armed=true;const double trace[]={0.5,3,7,4,0.8,3.5,9,1.1,2.8,6,0.4};for(double s:trace)if(MacheteStrikeStep(armed,s,2.5,1.2))++swings;
    Check(swings==3,"three back-to-back swings, three strikes");
    std::puts("PASS: blade contact sweep gate (speed floor, stale/jump rejection), per-target cooldown table, blade profile table, candidate 111 one-strike-per-swing gate and defaults");
}
