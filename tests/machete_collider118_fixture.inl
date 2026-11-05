UINT g_actorScratch118[6]={},g_colliderCalls118=0,g_colliderSourceConfig118=0;bool g_colliderPrivate118=true;float g_colliderDamage118=0;
void __cdecl MacheteContactTickFixture118(UINT,UINT config){if(g_macheteCuts121.active&&!g_macheteCuts121.damage)Check(g_macheteActor119.pass,"idle contact checks cannot run the scenery damage pass");++g_colliderCalls118;g_colliderPrivate118=g_colliderPrivate118&&config!=g_colliderSourceConfig118;memcpy(&g_colliderDamage118,reinterpret_cast<void*>(config+8),4);}
__declspec(naked) void MacheteActorEventFixture118(){
    __asm mov eax,[esp+3ch]
    __asm mov [g_actorScratch118],eax
    __asm mov eax,[esp+40h]
    __asm mov [g_actorScratch118+4],eax
    __asm mov eax,[esp+44h]
    __asm mov [g_actorScratch118+8],eax
    __asm mov eax,[esp+94h]
    __asm mov [g_actorScratch118+12],eax
    __asm mov eax,[esp+98h]
    __asm mov [g_actorScratch118+16],eax
    __asm mov eax,[esp+9ch]
    __asm mov [g_actorScratch118+20],eax
    __asm mov eax,1
    __asm ret
}
__declspec(naked) void MacheteActorSkipFixture118(){__asm mov eax,2 __asm ret}
__declspec(naked) void __cdecl CallMacheteActorFixture118(UINT* output,void* entry,UINT frame,UINT wrapper,UINT contact)
{
    __asm push ebp
    __asm mov ebp,esp
    __asm push ebx
    __asm push esi
    __asm push edi
    __asm mov eax,[ebp+24]
    __asm mov ebx,[ebp+20]
    __asm mov edx,[ebp+12]
    __asm mov ecx,[ebp+16]
    __asm sub esp,512
    __asm mov dword ptr [esp+28],0ffffffffh
    __asm mov dword ptr [esp+32],0ffffffffh
    __asm mov dword ptr [esp+36],0ffffffffh
    __asm push ebp
    __asm push eax
    __asm push 0
    __asm push 0
    __asm push 0
    __asm push 0
    __asm push 0
    __asm mov ebp,ecx
    __asm mov esi,13579bdfh
    __asm mov edi,ebx
    __asm xorps xmm0,xmm0
    __asm mov dword ptr [esp+34h],3f800000h
    __asm push 203h
    __asm popfd
    __asm call edx
    __asm pushfd
    __asm pop ecx
    __asm add esp,24
    __asm pop ebp
    __asm mov edx,[ebp+8]
    __asm mov [edx],eax
    __asm mov [edx+4],edi
    __asm mov [edx+8],esi
    __asm mov [edx+12],ecx
    __asm movups [edx+16],xmm0
    __asm add esp,512
    __asm pop edi
    __asm pop esi
    __asm pop ebx
    __asm pop ebp
    __asm ret
}
void TestMacheteCollider118()
{
    const auto savedCuts121=g_macheteCuts121;
    const auto initialized118=MH_Initialize();Check(initialized118==MH_OK||initialized118==MH_ERROR_ALREADY_INITIALIZED,"collider hook fixture initializes MinHook");
    const auto savedSettings=g_macheteCollider118;const auto savedBlade=g_machete;const auto savedContact=g_macheteBladeContact;
    g_machete.blade=-1;g_macheteCollider118=MacheteColliderSettings118{};
    auto local=MacheteColliderLocal118(g_macheteCollider118);const V3 direction=Unit(Sub(kMacheteBladeTip,kMacheteBladeBase));
    Check(Length(Sub({local.m[2],local.m[6],local.m[10]},direction))<1e-6,"default capsule is aligned to the measured blade axis");
    const V3 center=Apply(local,{0,0,0});g_macheteCollider118.offset[0]=.12f;const auto moved=MacheteColliderLocal118(g_macheteCollider118);
    Check(fabs(moved.m[3]-center.x-.12)<1e-6,"contact offset moves the actual blade-local collision volume");
    g_macheteCollider118.rotation[1]=90;const auto turned=MacheteColliderLocal118(g_macheteCollider118);
    Check(fabs(Dot({local.m[2],local.m[6],local.m[10]},{turned.m[2],turned.m[6],turned.m[10]}))<1e-5,"contact rotation changes the actual capsule direction");
    MacheteColliderPose118 a,b;a.ms=1000;b.ms=1016;a.scale=b.scale=1;a.epoch=b.epoch=2;a.base={0,0,0};a.tip={0,0,.3};b.base={.05,0,0};b.tip={.05,0,.3};a.bodyBase=a.base;a.bodyTip=a.tip;b.bodyBase=b.base;b.bodyTip=b.tip;b.minSpeed=.5;
    double speed=0;Check(MacheteColliderMotion118(a,b,speed)&&speed>3,"a moving physical blade permits contact queries");
    b.bodyBase=a.bodyBase;b.bodyTip=a.bodyTip;Check(!MacheteColliderMotion118(a,b,speed),"locomotion/world movement alone cannot cause a physical blade attack");
    b.bodyBase=b.base;b.bodyTip=b.tip;b.epoch=3;Check(!MacheteColliderMotion118(a,b,speed),"recenter/calibration cannot synthesize a collision stroke");b.epoch=2;b.ms=1300;Check(!MacheteColliderMotion118(a,b,speed),"stale blade pose cannot sweep damage");b.ms=1016;
    for(int slice=0;slice<4;++slice){V3 start,end,axis;float half=0;MacheteColliderSlice118(a,b,slice,start,end,axis,half);Check(fabs(start.z-(slice+.5)*.3/4)<1e-7&&fabs(end.x-.05)<1e-7&&fabs(half-.3/8)<1e-7,"four blade sub-capsules cover the length independently of floor contacts");}

    wchar_t temp[MAX_PATH],ini[MAX_PATH];GetTempPathW(MAX_PATH,temp);swprintf_s(ini,L"%sfc3_collider118_%lu.ini",temp,GetCurrentProcessId());DeleteFileW(ini);
    Check(SaveMacheteCollider118(ini),"all blade collider edits save");const auto expected=g_macheteCollider118;g_macheteCollider118=MacheteColliderSettings118{};LoadMacheteCollider118(ini);
    Check(fabs(g_macheteCollider118.offset[0]-expected.offset[0])<1e-6&&g_macheteCollider118.rotation[1]==90,"saved collider position and rotation survive reload");
    g_macheteCollider118.radius=.06f;Check(SaveMacheteCollider118(ini,7),"individual collider radius saves");g_macheteCollider118=MacheteColliderSettings118{};LoadMacheteCollider118(ini);Check(fabs(g_macheteCollider118.radius-.06f)<1e-6&&fabs(g_macheteCollider118.offset[0]-.12f)<1e-6,"saving a single collider field preserves other settings");
    WritePrivateProfileStringW(L"Machete",L"ColliderRadius",L"nan",ini);LoadMacheteCollider118(ini);Check(g_macheteCollider118.radius==MacheteColliderSettings118{}.radius,"invalid collider dimensions cannot enter native queries");DeleteFileW(ini);

    // Execute the production x86 hook against the exact displaced engine
    // instructions. Event/cleanup destinations are harmless fixture returns.
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(page!=nullptr,"actor ABI fixture page");
    const BYTE original[]={0x8b,0x7c,0x24,0x18,0xf3,0x0f,0x10,0x47,0x14,0xf3,0x0f,0x10,0x4f,0x18,0xf3,0x0f,0x10,0x57,0x1c,0xb8,3,0,0,0,0xc3};memcpy(page,original,sizeof(original));
    const UINT contact=reinterpret_cast<UINT>(page+256),frame=reinterpret_cast<UINT>(page+512),wrapper=reinterpret_cast<UINT>(page+768),config=reinterpret_cast<UINT>(page+1024);
    const float point[3]={.01f,.02f,.03f};memcpy(page+256+0x14,point,12);memcpy(page+512+12,&config,4);const UINT entity=0x12345678;memcpy(page+768+12,&entity,4);
    const auto oldEvent=g_macheteActorEvent118,oldSkip=g_macheteActorSkip118;void* oldOriginal=g_macheteActorOriginal118;
    g_macheteActorEvent118=reinterpret_cast<UINT>(MacheteActorEventFixture118);g_macheteActorSkip118=reinterpret_cast<UINT>(MacheteActorSkipFixture118);
    auto& state=g_macheteBladeContact;state=MacheteBladeContactState{};state.callThread=GetCurrentThreadId();state.inCall=0;
    UINT baseline[8]={},out[8]={};CallMacheteActorFixture118(baseline,page,frame,wrapper,contact);
    MH_STATUS status=MH_CreateHook(page,reinterpret_cast<void*>(MacheteActorBoundary118),&g_macheteActorOriginal118);Check(status==MH_OK&&MH_EnableHook(page)==MH_OK,"real actor-contact MinHook fixture installs");
    CallMacheteActorFixture118(out,page,frame,wrapper,contact);Check(memcmp(out,baseline,sizeof(out))==0,"ordinary native melee retains original registers, flags, SSE and displaced instructions");
    state.inCall=1;state.queryApplied118=true;state.callConfig118=config;state.half=.1f;state.radius=.03f;state.start={-.01,0,0};state.end={0,0,0};state.cooldownCount=0;
    CallMacheteActorFixture118(out,page,frame,wrapper,contact);Check(out[0]==1&&out[1]==contact+0x14&&out[2]==config&&(out[3]&1),"a real collider contact routes to native event creation with correct EDI position, ESI config and flags");
    Check(state.actorContacts118==1&&state.cooldownCount==1,"native entity identity receives one collision damage dispatch");
    Check(g_actorScratch118[0]==0&&g_actorScratch118[1]==0&&g_actorScratch118[2]==0,"direct event path initializes the native temporary list before its shared destructor");
    Check(g_actorScratch118[3]==0x3f800000&&g_actorScratch118[4]==0&&g_actorScratch118[5]==0,"damage direction follows physical blade movement rather than the camera axis");
    CallMacheteActorFixture118(out,page,frame,wrapper,contact);Check(out[0]==2&&state.actorContacts118==1,"another body part of the same enemy cannot receive duplicate slice damage");
    state.queryApplied118=false;state.cooldownCount=0;CallMacheteActorFixture118(out,page,frame,wrapper,contact);Check(out[0]==2,"failed physics-query substitution cannot fall back to camera damage");
    MH_DisableHook(page);MH_RemoveHook(page);VirtualFree(page,0,MEM_RELEASE);g_macheteActorOriginal118=oldOriginal;g_macheteActorEvent118=oldEvent;g_macheteActorSkip118=oldSkip;if(initialized118==MH_OK)MH_Uninitialize();
    Check(!MacheteMeleeState{}.enabled,"production movement/RT swipe bridge is disabled by default");
    const auto priorPose118=g_macheteColliderPose118;const bool priorGameplay118=g_inGameplay,priorMenu118=fc3pad::MenuOpen();g_inGameplay=true;fc3pad::SetPistolCalibration(false);g_machete.held=true;
    BYTE privateConfig118[0x200]={};float originalDamage118=120;memcpy(privateConfig118+8,&originalDamage118,4);UINT strategy118[16]={};
    state=MacheteBladeContactState{};state.enabled=true;state.engine=reinterpret_cast<UINT>(MacheteContactTickFixture118)-0x12d7b60;strategy118[0]=state.engine+0x1f99568;strategy118[9]=1;state.strategy=reinterpret_cast<UINT>(strategy118);state.config=reinterpret_cast<UINT>(privateConfig118);g_colliderSourceConfig118=state.config;g_colliderCalls118=0;g_colliderPrivate118=true;
    a.ms=10000;b.ms=10016;a.flags=b.flags=317;a.epoch=b.epoch=2;a.scale=b.scale=1;b.radius=.025f;b.damageScale=2;b.cooldownMs=350;b.minSpeed=.5f;b.bodyBase=b.base;b.bodyTip=b.tip;
    g_macheteColliderPose118=a;MacheteBladeTick(a.ms);g_macheteColliderPose118=b;MacheteBladeTick(b.ms);
    Check(g_colliderCalls118==8&&state.calls==8&&g_colliderPrivate118&&g_colliderDamage118==240,"production simulation tick issues scenery and character passes for four collider slices with a private scaled damage config");
    float sourceDamage118=0;memcpy(&sourceDamage118,privateConfig118+8,4);Check(sourceDamage118==120&&state.callConfig118==0&&!state.inCall,"physics tick preserves shared game damage data and releases call ownership");
    MacheteBladeTick(b.ms);Check(g_colliderCalls118==8,"repeated weapon ticks cannot damage twice from the same pose sample");
    auto idle121=b;idle121.ms+=16;g_macheteColliderPose118=idle121;MacheteBladeTick(idle121.ms);Check(g_colliderCalls118==8,"stationary blade with no pending contact needs no idle physics work");
    MacheteCutBegin121(idle121.ms,true);g_macheteCuts121.queries=4;MacheteCutHit121(0x1234);MacheteCutEnd121(true);
    idle121.ms+=16;g_macheteColliderPose118=idle121;MacheteBladeTick(idle121.ms);Check(g_colliderCalls118==12&&!g_macheteCuts121.damage,"stationary blade still checks separation in four observation-only actor passes");
    fc3pad::SetPistolCalibration(true);g_macheteColliderPose118.ms+=16;MacheteBladeTick(g_macheteColliderPose118.ms);Check(g_colliderCalls118==12&&!state.havePrev,"opening Insert immediately pauses collider damage");fc3pad::SetPistolCalibration(false);
    MacheteBladeTick(g_macheteColliderPose118.ms+1000);Check(g_colliderCalls118==12,"stale collider snapshot cannot make an engine damage call");
    const auto oldMelee118=g_macheteMelee;g_macheteMelee=MacheteMeleeState{};fc3pad::Controllers controls118={};controls118.active=true;controls118.rightTrigger=1;fc3pad::Publish(controls118);fc3pad::SetMacheteContext(true);fc3pad::SetMacheteHandling(true);fc3pad::CancelMacheteSource();fc3pad::CancelMacheteAttack();
    for(int i=0;i<80;++i)TickMacheteMelee(controls118,10000+i*16);
    Check(g_macheteMelee.sequence==0&&!(fc3pad::ButtonsOutput()&0x80),"holding RT and movement ticks never synthesize attack requests in collider mode");
    fc3pad::SetMacheteHandling(false);fc3pad::SetMacheteContext(false);controls118.rightTrigger=0;fc3pad::Publish(controls118);g_macheteMelee=oldMelee118;fc3pad::SetPistolCalibration(priorMenu118);g_inGameplay=priorGameplay118;g_macheteColliderPose118=priorPose118;
    g_macheteCuts121=savedCuts121;g_macheteCollider118=savedSettings;g_machete=savedBlade;g_macheteBladeContact=savedContact;
    std::puts("PASS: collider118 blade-local position/rotation/length/radius, independent blade slices, physical-only velocity, persistent per-field settings and guarded native entity damage hook; ordinary melee ABI unchanged, duplicate enemy hits suppressed, no camera fallback or gesture attack bridge");
}
void TestMacheteColliderGuide118(ID3D11Device* device,ID3D11DeviceContext* context)
{
    const auto settings=g_macheteCollider118;const auto blade=g_machete;const auto guide=g_macheteColliderGuide118;
    auto* oldDevice=g_dev;auto* oldContext=g_ctx;const bool aim=g_aimActive;const auto place=g_guideShaderPlacement[1];const LONG placeFrame=g_guideShaderPlacementFrame[1];const auto oldFrame=g_frame;float oldBody[16];memcpy(oldBody,g_handBody[0],sizeof(oldBody));
    g_dev=device;g_ctx=context;g_aimActive=false;++g_frame;g_guideShaderPlacement[1]=HandIdentity();g_guideShaderPlacementFrame[1]=g_frame;g_macheteCollider118=MacheteColliderSettings118{};
    g_machete.held=true;g_machete.blade=-1;g_machete.handAttachment116=true;g_machete.handEpoch116=g_armPoseEpoch;
    Mat4 inverse={};Check(Invert(kMacheteFromHand[0],inverse),"blade bind inverse for preview fixture");const Mat4 turn=TurnAndMove({0,0,1},{0,1,0},{0,0,0},{0,0,0});g_machete.controllerHand116=Multiply(turn,inverse);
    for(int i=0;i<16;++i)g_handBody[0][i]=i%5==0?1.f:0.f;
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);float world[88]={};world[0]=world[5]=world[10]=1;globals->SetPrivateData(kCbCopyGuid,sizeof(world),world);
    float native[224]={};native[0]=native[5]=2;native[11]=.5f;native[15]=1;camera->SetPrivateData(kCbCopyGuid,sizeof(native),native);CameraSummary summary={};summary.mainView=1;summary.right[0]=summary.up[1]=summary.forward[2]=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(summary),&summary);ID3D11Buffer* cb[]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
    ID3D11Buffer* eyes[2]={g_eyeCb[1][0],g_eyeCb[1][1]};for(int eye=0;eye<2;++eye){g_eyeCb[1][eye]=Buffer(device,256,D3D11_BIND_CONSTANT_BUFFER);float rows[64]={};rows[3]=eye?.07f:-.07f;g_eyeCb[1][eye]->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);}
    CaptureMacheteColliderGuide118(context);Check(g_macheteColliderGuide118.frame==g_frame&&g_macheteColliderGuide118.count==136,"production guide captures full capsule wireframe from bound native camera and both-eye matrices");
    for(UINT i=0;i<g_macheteColliderGuide118.count;++i)Check(fabs(g_macheteColliderGuide118.clip[1][i][0]-g_macheteColliderGuide118.clip[0][i][0]-.14f)<1e-5,"capsule guide uses independent stereo eye placement");
    ID3D11Texture2D* textures[2]={};D3D11_TEXTURE2D_DESC td={};td.Width=160;td.Height=120;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;for(auto*& t:textures)Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&t)),"capsule preview eye texture");
    for(auto* t:textures){ID3D11RenderTargetView* target=nullptr;device->CreateRenderTargetView(t,nullptr,&target);const float black[4]={};context->ClearRenderTargetView(target,black);SafeRelease(target);}
    RenderMacheteColliderGuide118(textures[0],textures[1]);double center[2]={};
    for(int eye=0;eye<2;++eye){auto desc=td;desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&stage)),"capsule staging");context->CopyResource(stage,textures[eye]);D3D11_MAPPED_SUBRESOURCE map={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&map)),"capsule pixels map");UINT count=0;for(UINT y=0;y<td.Height;++y)for(UINT x=0;x<td.Width;++x){const auto* pixel=static_cast<const BYTE*>(map.pData)+y*map.RowPitch+x*4;if(pixel[2]>180){++count;center[eye]+=x;}}context->Unmap(stage,0);SafeRelease(stage);Check(count>20,"real capsule guide pixels appear in each eye image");center[eye]/=count;}
    Check(center[1]-center[0]>9,"rendered capsule preview has the expected stereo disparity");

    wchar_t temp[MAX_PATH],ini[MAX_PATH];GetTempPathW(MAX_PATH,temp);swprintf_s(ini,L"%sfc3_collider_menu118_%lu.ini",temp,GetCurrentProcessId());DeleteFileW(ini);
    auto* previousGui=ImGui::GetCurrentContext();auto* gui=ImGui::CreateContext();ImGui::SetCurrentContext(gui);auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1100,900};io.DeltaTime=1.f/60;unsigned char* pixels=nullptr;int width=0,height=0;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    auto ui=[&](){ImGui::NewFrame();ImGui::SetNextWindowPos({0,0},ImGuiCond_Always);ImGui::SetNextWindowSize({1000,850},ImGuiCond_Always);ImGui::Begin("Blade collider test");MacheteColliderMenu118(ini);ImGui::End();ImGui::Render();};
    ui();ui();const float radius=g_macheteCollider118.radius;
    auto click=[&](ImVec2 at){io.AddMousePosEvent(at.x,at.y);ui();io.AddMouseButtonEvent(0,true);ui();io.AddMouseButtonEvent(0,false);ui();};
    click(g_macheteSlider118[7]);Check(g_macheteCollider118.radius>radius+.05f,"native Insert capsule-radius slider accepts mouse edits");const float savedRadius=g_macheteCollider118.radius;click(g_macheteSave118[7]);g_macheteCollider118=MacheteColliderSettings118{};LoadMacheteCollider118(ini);Check(fabs(g_macheteCollider118.radius-savedRadius)<1e-6,"native per-slider Save button persists the real collision radius");
    ImGui::DestroyContext(gui);ImGui::SetCurrentContext(previousGui);DeleteFileW(ini);
    context->ClearState();for(int eye=0;eye<2;++eye){SafeRelease(textures[eye]);SafeRelease(g_eyeCb[1][eye]);g_eyeCb[1][eye]=eyes[eye];}SafeRelease(globals);SafeRelease(camera);
    memcpy(g_handBody[0],oldBody,sizeof(oldBody));g_frame=oldFrame;g_guideShaderPlacement[1]=place;g_guideShaderPlacementFrame[1]=placeFrame;g_aimActive=aim;g_dev=oldDevice;g_ctx=oldContext;g_machete=blade;g_macheteCollider118=settings;g_macheteColliderGuide118=guide;
    std::puts("PASS: collider118 production capsule preview renders actual stereo pixels; Insert mouse slider edits and per-slider Save change and persist the real collision volume");
}
