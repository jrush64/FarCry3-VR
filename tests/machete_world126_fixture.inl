void TestMacheteWorld126()
{
 const auto old=g_macheteBladeContact;const auto actor=g_macheteActor119;const auto cuts=g_macheteCuts121;const auto world=g_macheteWorld126;const bool haptics=g_macheteCollider118.haptics;
 auto& b=g_macheteBladeContact;b=MacheteBladeContactState{};g_macheteWorld126={};g_macheteCollider118.haptics=false;
 BYTE stack[128]={},frame[32]={},config[0x200]={};UINT saved[8]={};saved[3]=reinterpret_cast<UINT>(stack)-4;saved[2]=reinterpret_cast<UINT>(frame);const UINT cp=reinterpret_cast<UINT>(config);memcpy(frame+12,&cp,4);b.callConfig118=cp;
 b.start={0,0,0};b.end={.05,0,0};b.half=.1f;b.radius=.025f;b.cooldownMs=350;b.queryApplied118=true;b.inCall=1;b.callThread=GetCurrentThreadId();g_macheteActor119.pass=false;g_macheteCuts121.active=g_macheteCuts121.damage=true;
 const float point[3]={.02f,.01f,0};memcpy(stack+0x28,point,12);frame[0x14]=1;MacheteWorldContact126(saved);Check(frame[0x14]==0&&g_macheteWorld126.allowed==1,"real post-self-filter surface contact restores native material impact branch");
 frame[0x14]=1;MacheteWorldContact126(saved);Check(frame[0x14]==1&&g_macheteWorld126.allowed==1,"four blade slices cannot repeat the same surface impact immediately");
 g_macheteWorld126={};g_macheteActor119.pass=true;MacheteWorldContact126(saved);Check(frame[0x14]==1,"character pass never enables world or camera fallback");g_macheteActor119.pass=false;
 g_macheteCuts121.damage=false;MacheteWorldContact126(saved);Check(frame[0x14]==1,"stationary separation observation cannot create surface hits");g_macheteCuts121.damage=true;b.queryApplied118=false;MacheteWorldContact126(saved);Check(frame[0x14]==1,"failed blade query cannot enable native impacts");b.queryApplied118=true;
 float farPoint[3]={5,0,0};memcpy(stack+0x28,farPoint,12);MacheteWorldContact126(saved);Check(frame[0x14]==1&&g_macheteWorld126.rejected==1,"out-of-blade contact cannot enable distant scenery impact");
 Check(!MacheteWorldTouch126({NAN,0,0},1000),"invalid surface positions rejected");g_macheteWorld126={};Check(MacheteWorldTouch126({0,0,0},1000)&&!MacheteWorldTouch126({0,0,0},1100)&&MacheteWorldTouch126({0,0,0},1400),"surface cooldown permits repeated separate strikes");
 // Actual production trampoline: displaced config load/compare, register,
 // flags and SSE behavior remain unchanged for ordinary native melee.
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(page!=nullptr,"world ABI page");const BYTE prefix[]={0x8b,0x5d,0x0c,0x83,0xbb,0x1c,0x01,0x00,0x00,0x00,0xb8,3,0,0,0,0xc3};memcpy(page,prefix,sizeof(prefix));b.inCall=0;
 UINT expected[8]={},actual[8]={};CallMacheteActorFixture118(expected,page,reinterpret_cast<UINT>(frame),cp,0);const auto initialized=MH_Initialize();Check(initialized==MH_OK||initialized==MH_ERROR_ALREADY_INITIALIZED,"world hook init");void* previous=g_macheteWorldOriginal126;Check(MH_CreateHook(page,reinterpret_cast<void*>(MacheteWorldBoundary126),&g_macheteWorldOriginal126)==MH_OK&&MH_EnableHook(page)==MH_OK,"production surface-impact hook installs");CallMacheteActorFixture118(actual,page,reinterpret_cast<UINT>(frame),cp,0);Check(memcmp(expected,actual,sizeof(actual))==0,"ordinary native surface hook preserves registers flags SSE and relocated instructions");MH_DisableHook(page);MH_RemoveHook(page);g_macheteWorldOriginal126=previous;VirtualFree(page,0,MEM_RELEASE);if(initialized==MH_OK)MH_Uninitialize();
 g_macheteBladeContact=old;g_macheteActor119=actor;g_macheteCuts121=cuts;g_macheteWorld126=world;g_macheteCollider118.haptics=haptics;
 puts("PASS: world126 real-contact-only native surface path, ordinary-hook ABI, per-surface cooldown, no character/camera or stationary fallback, invalid/far query rejection");
}
void TestFastTurnCapture126()
{
 fc3stereo::EyeView eye={};for(int i=0;i<16;++i)eye.view[i]=i%5==0?1.f:0.f;eye.tanLeft=eye.tanDown=-1;eye.tanRight=eye.tanUp=1;
 Check(!FastTurnCapture126(.9f,0,eye,.8f,.8f),"stick input with native/eye agreement does not capture");const double a=.6;eye.view[8]=float(sin(a));eye.view[10]=float(cos(a));Check(FastTurnCapture126(.9f,0,eye,.8f,.8f),"rapid stick exposing area beyond native view captures including armed gameplay");Check(!FastTurnCapture126(.1f,0,eye,.8f,.8f),"quiet stick does not trigger fast-turn capture");Check(!FastTurnCapture126(.9f,0,eye,0,.8f),"unknown native projection cannot classify a mismatch");eye.view[8]=-eye.view[8];Check(FastTurnCapture126(-.9f,0,eye,.8f,.8f),"left/right turns captured symmetrically");puts("PASS: culling126 automatic fast-stick/native-view mismatch capture predicate; observation only");
}

