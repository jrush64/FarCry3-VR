void TestHealProbe215(){
 const auto oldAt=g_healHoldAt215;const bool oldCaptured=g_healHoldCaptured215;const LONG oldChecked=g_healProbeCheckedFrame215,oldTriggered=g_healProbeTriggeredFrame215;
 g_healHoldAt215=0;g_healHoldCaptured215=false;g_healProbeCheckedFrame215=g_healProbeTriggeredFrame215=-1;
 fc3pad::Controllers c={};c.active=true;c.y=true;
 Check(!HealProbeHoldEdge215(true,c,1000,100)&&!HealProbeHoldEdge215(true,c,1300,101),"brief Y tap never opens health injector capture");
 Check(HealProbeHoldEdge215(true,c,1501,102)&&g_healProbeTriggeredFrame215==102&&!HealProbeHoldEdge215(true,c,1600,103),"one bounded capture edge follows deliberate hold-Y");
 c.y=false;Check(!HealProbeHoldEdge215(true,c,1700,104),"Y release resets health recorder");
 c.y=true;c.leftGrip=1;Check(!HealProbeHoldEdge215(true,c,2300,105),"left-grip Y weapon wheel does not open health recorder");
 c.leftGrip=0;Check(!HealProbeHoldEdge215(true,c,2400,106)&&HealProbeHoldEdge215(true,c,2901,107),"later independent hold can be recorded");
 Check(!HealProbeHoldEdge215(false,c,3500,108),"non-gameplay hold cannot start a health recorder");
 g_healHoldAt215=oldAt;g_healHoldCaptured215=oldCaptured;g_healProbeCheckedFrame215=oldChecked;g_healProbeTriggeredFrame215=oldTriggered;
 puts("PASS: heal215 deliberate hold-Y recording edge, wheel exclusion, repeat and gameplay guard");
}
