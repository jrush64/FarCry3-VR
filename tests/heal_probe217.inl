void TestHealProbe217(ID3D11Device* device,ID3D11DeviceContext* context){
    g_dev=device;g_ctx=context;g_oCopyResource=ProbeCopy;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap;
    const auto saved=g_actionProbe;g_actionProbe=ActionProbeState{};
    Check(HealInjectorGeometry217({27424,6144,32,0,0,3072,57})&&HealInjectorGeometry217({10848,2112,32,0,0,1056,57}),"heal217 installed syringe LOD signatures");
    Check(!HealInjectorGeometry217({143132,32760,40,1372,216,3903,57})&&!HealInjectorGeometry217({43360,11358,40,0,0,2799,57}),"heal217 C4 and Molotov are not syringe evidence");
    Check(!HealInjectorGeometry217({27424,6144,40,0,0,3072,57})&&!HealInjectorGeometry217({27424,6144,32,0,0,3072,42}),"heal217 stream and index-format guards");
    for(UINT key=0;key<64;++key)HealSample217(key,false,1000);
    Check(g_actionProbe.healBackground217==48&&HealSample217(999,true,1000),"heal217 background saturation preserves independent syringe slots");
    Check(!HealSample217(999,true,1015)&&HealSample217(999,true,1016),"heal217 syringe sampling uses time, not frame-rate-dependent every-third-frame flooding");
    for(UINT tick=1;tick<20;++tick)for(UINT key=0;key<48;++key)HealSample217(key,false,1000+tick*250);
    Check(g_actionProbe.healBackground217==256&&HealSample217(999,true,6000),"heal217 background cap reserves candidate budget for a delayed native injector");
    context->ClearState();auto* vb=Buffer(device,27424,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,6144,D3D11_BIND_INDEX_BUFFER);
    auto* cameraBuffer=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);CameraSummary cam={};cam.mainView=1;cameraBuffer->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
    ID3D11Buffer* cb[2]={nullptr,cameraBuffer};context->VSSetConstantBuffers(0,2,cb);UINT stride=32,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
    g_actionProbe=ActionProbeState{};g_actionProbe.event=3;g_actionProbe.sequence=1;g_actionProbe.begin=g_frame;g_actionProbe.started=GetTickCount64();g_actionProbe.resourceCount=64;g_actionProbe.healBackground217=256;
    g_actionProbe.file=CreateFileW(L"heal217_fixture.bin",GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
    Check(g_actionProbe.file!=INVALID_HANDLE_VALUE,"heal217 alternate-camera fixture file");DrawCall call={};call.a=3072;
    RecordHealAlternate217(context,call,0);Check(g_actionProbe.draws==1&&g_actionProbe.file!=INVALID_HANDLE_VALUE,"heal217 real alternate syringe geometry captured after ordinary draws exhaust their budget");
    SetFilePointer(g_actionProbe.file,0,nullptr,FILE_BEGIN);ActionProbeRecord r={};DWORD read=0;
    Check(ReadFile(g_actionProbe.file,&r,sizeof(r),&read,nullptr)&&read==sizeof(r)&&r.vertexBytes==27424&&r.event==3&&r.stride==32,"heal217 alternate capture keeps native draw identity and binary format");
    const HANDLE before=g_actionProbe.file;fc3pad::Controllers controls={};controls.active=true;controls.x=true;fc3pad::Publish(controls);TickActionProbe(true);
    Check(g_actionProbe.file==before&&g_actionProbe.event==3,"heal217 reload input cannot replace an active delayed-injector capture");controls.x=false;fc3pad::Publish(controls);
    g_actionProbe.healCandidates217=1792;RecordHealAlternate217(context,call,0);Check(g_actionProbe.draws==1&&g_actionProbe.file!=INVALID_HANDLE_VALUE,"heal217 full sampling budget keeps bounded window open instead of ending it early");
    CloseActionProbe("heal217 fixture");
    // Real file exports: one address used in different roles must yield both
    // snapshots, the exact failure that omitted the C4 bomb's index buffer.
    auto* reused=Buffer(device,64,D3D11_BIND_VERTEX_BUFFER|D3D11_BIND_INDEX_BUFFER);unsigned char payload[64]={};for(UINT i=0;i<64;++i)payload[i]=BYTE(i);
    context->UpdateSubresource(reused,0,nullptr,payload,0,0);g_actionProbe.resourceCount=g_actionProbe.meshBytes=0;
    ActionMesh(context,reused,L"vb0");ActionMesh(context,reused,L"ib");
    Check(g_actionProbe.resourceCount==2&&g_actionProbe.meshBytes==128,"heal217 reused buffer address exports independent VB and IB roles");
    wchar_t stem[96]={};ActionProbeStem217(stem,96);
    for(const wchar_t* role:{L"vb0",L"ib"}){wchar_t path[MAX_PATH]={};_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s%s_%s_%016llX.bin",fc3::DataFolder(),stem,role,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(reused)));
        HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);Check(file!=INVALID_HANDLE_VALUE,"heal217 both role snapshots exist");
        unsigned char bytes[64]={};DWORD n=0;Check(ReadFile(file,bytes,64,&n,nullptr)&&n==64&&!memcmp(bytes,payload,64),"heal217 role exports retain exact GPU bytes");CloseHandle(file);DeleteFileW(path);}
    reused->Release();context->ClearState();vb->Release();ib->Release();cameraBuffer->Release();g_actionProbe=saved;
    puts("PASS: heal217 rejects explosive misidentification, reserves late-syringe samples after background saturation, records alternate-camera native draws without render changes, preserves bounded binary capture");
}
