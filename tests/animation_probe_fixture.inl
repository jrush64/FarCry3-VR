void TestAnimationTimeline(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();g_animation=AnimationProbe{};g_animation.enabled=true;g_animationQueueCount=0;
    g_dev=device;g_ctx=context;g_frame=2000;g_lastShotFrame=-100000;g_twoHand=false;g_reload=PhysicalReloadState{};
    fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
    float raw[576]={};for(int b=0;b<48;++b){raw[b*12]=raw[b*12+5]=raw[b*12+10]=1;raw[b*12+3]=b*.001f;}
    float finalPose[576];memcpy(finalPose,raw,sizeof(raw));finalPose[3]=.25f;
    ID3D11Buffer* palette=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER),*globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*cameraBuffer=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
    palette->SetPrivateData(kCbCopyGuid,sizeof(raw),raw);context->UpdateSubresource(palette,0,nullptr,raw,0,0);
    float world[88]={};world[0]=world[5]=world[10]=1;globals->SetPrivateData(kCbCopyGuid,sizeof(world),world);
    CameraSummary camera={};camera.mainView=1;camera.right[0]=camera.up[1]=camera.forward[2]=1;cameraBuffer->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);
    ID3D11Buffer* cb[4]={globals,cameraBuffer,nullptr,palette};context->VSSetConstantBuffers(0,4,cb);
    auto* code=HullCompile("float4 main(uint id:SV_VertexID):SV_Position{return float4(0,0,0,1);}","vs_5_0");ID3D11VertexShader* vs=nullptr;Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs)),"animation fixture shader");code->Release();
    unsigned long long hash=kArmVsHash;vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);context->VSSetShader(vs,nullptr,0);
    ID3D11Buffer* vb=Buffer(device,257896,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=336;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    TickAnimationProbe(true);DrawCall call={};call.a=108;call.b=18;RecordAnimationDraw(context,call);RecordAnimationFinal(finalPose,1);FinishAnimationDraw();
    Check(g_animation.ring.size()>=2&&g_animation.file==INVALID_HANDLE_VALUE,"pre-action palettes retained before any request without opening a file");
    const auto& pending=g_animation.ring.back();Check(pending.rawBytes==2304&&pending.finalBytes==2304&&pending.side==1,"source and final arm pose paired with identified side");
    Compare(pending.raw,raw,576,"raw animation probe leaves original palette exact");Compare(pending.finalPose,finalPose,576,"final palette snapshot exact");
    g_lastShotFrame=++g_frame;TickAnimationProbe(true);Check(g_animation.file!=INVALID_HANDLE_VALUE,"shot automatically opens durable trace with pre-roll");
    wchar_t saved[MAX_PATH];wcscpy_s(saved,g_animation.path);
    // Repeated held inputs do not manufacture event edges; controller loss does not cancel the active trace.
    const LONG shots=g_animation.shots;controls.active=false;fc3pad::Publish(controls);TickAnimationProbe(false);Check(g_animation.shots==shots&&g_animation.file!=INVALID_HANDLE_VALUE,"tracking loss preserves capture and held shot marker does not repeat");
    controls.active=true;controls.x=true;fc3pad::Publish(controls);TickAnimationProbe(true);
    g_reload.requests=1;TickAnimationProbe(true);
    // Queue a real readable state snapshot; callback has no file IO until Present drains it.
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,16384,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"native animation state fixture");
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288;
    *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;*reinterpret_cast<UINT*>(weapon+0xcc)=4;
    ProbeNativeAnimation(state,wrapper,0,-1,0x12345678,4);Check(g_animationQueueCount==1,"native callback queues state and return site");
    *reinterpret_cast<UINT*>(weapon+0xcc)=5;ProbeNativeAnimation(state,wrapper,0,-1,0x12345678,5);TickAnimationProbe(false);Check(g_animationQueueCount==0,"native events drain through Present even when tracking is unavailable");
    g_animation.sample=true;++g_frame;
    hash=kArmVsHash;vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);
    call.b=17304;RecordAnimationDraw(context,call);RecordAnimationFinal(finalPose,1);FinishAnimationDraw();
    call.b=6861;RecordAnimationDraw(context,call);RecordAnimationFinal(finalPose,0);FinishAnimationDraw();
    call.b=18;hash=0xA38F7CD233BCE12Bull;vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);
    RecordAnimationDraw(context,call);RecordAnimationFinal(finalPose,2);FinishAnimationDraw();AnimationClose();
    HANDLE file=CreateFileW(saved,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);Check(file!=INVALID_HANDLE_VALUE,"closed animation timeline readable");
    LARGE_INTEGER size;GetFileSizeEx(file,&size);Check(size.QuadPart>sizeof(AnimationRecord)*4&&size.QuadPart%sizeof(AnimationRecord)==0,"complete fixed-size animation packets durable on disk");
    bool header=false,pre=false,shot=false,x=false,generated=false,before=false,after=false,shell=false;
    for(LONGLONG off=0;off<size.QuadPart;off+=sizeof(AnimationRecord))
    {
        AnimationRecord r;DWORD n=0;Check(ReadFile(file,&r,sizeof(r),&n,nullptr)&&n==sizeof(r)&&r.magic==0x414E5031&&r.bytes==sizeof(r),"animation serialized layout");
        if(r.kind==0)header=r.clock>0;
        if(r.kind==1&&r.side==1){pre=true;Compare(r.raw,raw,576,"saved native finger/arm palette exact");Compare(r.finalPose,finalPose,576,"saved controlled pose exact");}
        if(r.kind==2){shot|=(r.event&1)!=0;x|=(r.event&2)!=0;generated|=(r.event&4)!=0;}
        if(r.kind==4)before=r.loaded==4&&r.stateWords[0]==actor;
        if(r.kind==5)after=r.loaded==5;
        if(r.kind==1&&r.side==2)shell=true;
    }
    Check(header&&pre&&shot&&x&&generated&&before&&after&&shell,"saved pre-roll, shot/X/generated-request markers, raw/final poses and native before/after values");
    CloseHandle(file);float unchanged[576];Read(device,context,palette,unchanged);Compare(unchanged,raw,576,"probe does not mutate source GPU palette");
    // Save actual production packets for the companion analyzer; do not delete the evidence.
    std::wprintf(L"ANIMATION FIXTURE: %ls (%lld bytes, record size %u)\n",saved,size.QuadPart,static_cast<UINT>(sizeof(AnimationRecord)));
    // Observe a body draw using the MAIN camera before either arm. This was
    // absent from the original fixture and is required to diagnose real sleeves.
    const int priorProfile=g_weaponMechanicProfile;
    g_inGameplay=true;g_animation.until=0;g_animation.sample=true;++g_frame;
    g_armMovesFrame[0]=g_armMovesFrame[1]=g_frame-1;
    ID3D11Buffer* bodyVb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);
    stride=40;offset=0;context->IASetVertexBuffers(0,1,&bodyVb,&stride,&offset);
    call.b=0;call.a=3687;RecordVrBaselineDraw(context,call,0,false);
    Check(g_animation.ring.back().kind==16&&g_animation.ring.back().event==0,"main-camera body observation before arms retained");
    Check(g_animation.ring.back().stateWords[0]==static_cast<UINT>(g_frame-1)&&g_animation.ring.back().stateWords[2]==0,"body-before-arms is distinguishable from palette mismatch");
    Compare(g_animation.ring.back().raw,raw,576,"body capture uses source palette without modifying it");
    const auto bodyRecords=g_animation.ring.size();
    RecordVrBaselineDraw(context,call,0,false);Check(g_animation.ring.size()==bodyRecords,"duplicate body observation bounded per frame");
    // The pistol has rigid 32-byte streams, unlike the shotgun's skinned stream.
    ID3D11Buffer* pistolVb=Buffer(device,177904,D3D11_BIND_VERTEX_BUFFER),*pistolIb=Buffer(device,32682,D3D11_BIND_INDEX_BUFFER);
    stride=32;offset=240;context->IASetVertexBuffers(0,1,&pistolVb,&stride,&offset);context->IASetIndexBuffer(pistolIb,DXGI_FORMAT_R16_UINT,0);
    call.b=294;call.a=4470;std::memset(g_vrBaselineModels,0,sizeof(g_vrBaselineModels));
    RecordVrBaselineDraw(context,call,0,false);Check(g_animation.until==0,"world-camera matching geometry cannot open weapon window");
    RecordVrBaselineDraw(context,call,1,true);Check(g_animation.until>GetTickCount64()&&g_animation.ring.back().kind==17,"first pistol sighting opens an automatic metadata window");
    g_animation.retry=0;Check(AnimationOpen(),"automatic metadata durable after retry interval");
    const auto until=g_animation.until;RecordVrBaselineDraw(context,call,1,true);Check(g_animation.until==until,"repeat pistol draws do not extend capture indefinitely");
    ID3D11Buffer* rifleVb=Buffer(device,341536,D3D11_BIND_VERTEX_BUFFER),*rifleIb=Buffer(device,65556,D3D11_BIND_INDEX_BUFFER);
    stride=32;offset=0;context->IASetVertexBuffers(0,1,&rifleVb,&stride,&offset);context->IASetIndexBuffer(rifleIb,DXGI_FORMAT_R16_UINT,0);
    call.b=2853;call.a=13368;RecordVrBaselineDraw(context,call,1,true);
    Check(g_animation.modelFamilies==2,"two distinct weapon families counted only after successful file writes");
    g_animation.until=0;offset=240;context->IASetVertexBuffers(0,1,&pistolVb,&stride,&offset);context->IASetIndexBuffer(pistolIb,DXGI_FORMAT_R16_UINT,0);
    call.b=294;call.a=4470;RecordVrBaselineDraw(context,call,1,true);
    Check(g_animation.until>GetTickCount64()&&g_animation.ring.back().kind==17,"returning to a previously seen weapon rearms expired capture");
    Check(g_animation.modelFamilies==2,"revisiting and replayed pre-roll do not inflate saved family count");
    SafeRelease(rifleVb);SafeRelease(rifleIb);
    const auto count=g_animation.ring.size();offset=0;context->IASetVertexBuffers(0,1,&pistolVb,&stride,&offset);
    RecordVrBaselineDraw(context,call,1,true);Check(g_animation.ring.size()==count,"wrong pistol stream offset never qualifies");
    AnimationClose();Read(device,context,palette,unchanged);Compare(unchanged,raw,576,"expanded observation leaves source GPU bones unchanged");
    context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(nullptr,DXGI_FORMAT_UNKNOWN,0);
    SafeRelease(bodyVb);SafeRelease(pistolVb);SafeRelease(pistolIb);g_inGameplay=false;g_weaponMechanicProfile=priorProfile;
    g_animation.until=GetTickCount64()+10000;g_animation.retry=0;g_animation.totalBytes=1023u*1024*1024;Check(AnimationOpen()&&!g_animation.full,"256 MB no longer stops a capture (FAMAS reload was lost at 256 MB)");AnimationClose();
    g_animation.until=GetTickCount64()+10000;g_animation.retry=0;g_animation.totalBytes=1024u*1024*1024;
    Check(!AnimationOpen()&&g_animation.full,"capture cap (1 GB) is explicit and preserves saved files");
    VirtualFree(page,0,MEM_RELEASE);g_animation=AnimationProbe{};g_animationQueueCount=0;g_reload=PhysicalReloadState{};
    context->ClearState();SafeRelease(palette);SafeRelease(globals);SafeRelease(cameraBuffer);SafeRelease(vb);SafeRelease(vs);g_dev=nullptr;g_ctx=nullptr;
    std::puts("PASS: animation pre-roll, automatic durable files, native callback queue, source/final palette pairing, shot/manual/generated reload timing, tracking-loss preservation and visible cap");
}

void TestAnimationBatch169()
{
 const auto old=g_animation;g_animation=AnimationProbe{};
 wchar_t folder[MAX_PATH]={},path[MAX_PATH]={};GetTempPathW(MAX_PATH,folder);Check(GetTempFileNameW(folder,L"a69",0,path)!=0,"169 capture temporary file");
 g_animation.file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);Check(g_animation.file!=INVALID_HANDLE_VALUE,"169 batch writer opens");
 for(UINT i=0;i<259;++i){AnimationRecord r;r.kind=1;r.event=i;r.rawBytes=2304;r.side=i%2;for(UINT j=0;j<576;++j)r.raw[j]=static_cast<float>(i*576+j);Check(AnimationWrite(r),"169 batch accepts complete record");}
 Check(g_animation.writeCalls169==32&&g_animation.writeCount169==3,"169 256 records use32 writes and retain3 pending");
 Check(AnimationFlush169()&&g_animation.writeCount169==0&&g_animation.poses==259,"169 periodic flush drains all pending records and commits counters");
 AnimationClose();Check(g_animation.writeCalls169==33,"169 259 records need33 writes, not259");
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);Check(f!=INVALID_HANDLE_VALUE,"169 saved batch readable");
 for(UINT i=0;i<259;++i){AnimationRecord expected;expected.kind=1;expected.event=i;expected.rawBytes=2304;expected.side=i%2;for(UINT j=0;j<576;++j)expected.raw[j]=static_cast<float>(i*576+j);AnimationRecord actual;DWORD n=0;Check(ReadFile(f,&actual,sizeof(actual),&n,nullptr)&&n==sizeof(actual)&&!memcmp(&actual,&expected,sizeof(actual)),"169 every serialized byte and record order preserved");}
 DWORD n=0;BYTE tail=0;Check(ReadFile(f,&tail,1,&n,nullptr)&&n==0,"169 no trailing partial record");CloseHandle(f);
 g_animation=AnimationProbe{};g_animation.file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);AnimationRecord r;Check(AnimationWrite(r),"169 queue before failed write");Check(!AnimationFlush169()&&g_animation.error!=0&&g_animation.lost==1&&g_animation.poses==0&&g_animation.file==INVALID_HANDLE_VALUE,"169 write failure closes safely and reports unsaved record without false saved counters");
 DeleteFileW(path);g_animation=old;puts("PASS:169 animation batching preserves259 exact ordered packets in33 writes, periodic/close drain, saved counters and write-failure reporting; no FPS claim.");
}
