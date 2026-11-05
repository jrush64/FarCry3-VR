#include "../src/stereo.cpp"
#include <d3dcompiler.h>
#include <cstdlib>
void Check(bool ok,const char* what) { if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);} }
void STDMETHODCALLTYPE Update(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s,const D3D11_BOX* box,const void* p,UINT rp,UINT dp)
{ c->UpdateSubresource(r,s,box,p,rp,dp); }
ID3D11Buffer* Buffer(ID3D11Device* d,UINT size,UINT bind)
{
    D3D11_BUFFER_DESC desc={};desc.ByteWidth=size;desc.BindFlags=bind;ID3D11Buffer* b=nullptr;
    Check(SUCCEEDED(d->CreateBuffer(&desc,nullptr,&b)),"buffer creation");return b;
}
void Read(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Buffer* source,float* out)
{
    D3D11_BUFFER_DESC desc={};source->GetDesc(&desc);desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* stage=nullptr;Check(SUCCEEDED(d->CreateBuffer(&desc,nullptr,&stage)),"staging buffer");c->CopyResource(stage,source);
    D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(c->Map(stage,0,D3D11_MAP_READ,0,&m)),"read upload");std::memcpy(out,m.pData,desc.ByteWidth);c->Unmap(stage,0);stage->Release();
}
void Compare(const float* a,const float* b,int n,const char* what)
{for(int i=0;i<n;++i)Check(std::isfinite(a[i]) && std::fabs(a[i]-b[i])<2e-5f,what);}
void STDMETHODCALLTYPE ProbeCopy(ID3D11DeviceContext* c,ID3D11Resource* d,ID3D11Resource* s){c->CopyResource(d,s);}
HRESULT STDMETHODCALLTYPE ProbeMap(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s,D3D11_MAP t,UINT f,D3D11_MAPPED_SUBRESOURCE* m){return c->Map(r,s,t,f,m);}
void STDMETHODCALLTYPE ProbeUnmap(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s){c->Unmap(r,s);}
void STDMETHODCALLTYPE EjectCopyRange(ID3D11DeviceContext* c,ID3D11Resource* d,UINT ds,UINT x,UINT y,UINT z,ID3D11Resource* s,UINT ss,const D3D11_BOX* b){c->CopySubresourceRegion(d,ds,x,y,z,s,ss,b);}
#include "physical_ejection_fixture.inl"
int main()
{
    wchar_t path[MAX_PATH];GetSystemDirectoryW(path,MAX_PATH);wcscat_s(path,L"\\d3d11.dll");
    auto create=reinterpret_cast<decltype(&D3D11CreateDevice)>(GetProcAddress(LoadLibraryW(path),"D3D11CreateDevice"));
    ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;
    Check(SUCCEEDED(create(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"WARP");
    g_oUpdateSubresource=Update;g_motionGun=true;g_eyeViewsSet=true;g_handLeftValid=g_handRightValid=true;
    g_visibleWeaponDrawsLastFrame=0;g_twoHand=false;
    for(int side=0;side<2;++side)
    {
        float* pose=side==0?g_handRight:g_handLeft;std::memset(pose,0,16*sizeof(float));
        pose[0]=pose[5]=pose[10]=pose[15]=1;pose[3]=side==0?.3f:-.3f;pose[7]=-.2f;pose[11]=.45f;
        std::memset(g_handRef[side],0,16*sizeof(float));g_handRef[side][0]=g_handRef[side][5]=g_handRef[side][10]=g_handRef[side][15]=1;
        ArmJoints j={{side==0?.2:-.2,-.15,-.2},{side==0?.35:-.35,-.3,.05},{side==0?.35:-.35,-.2,.4},true};g_armJoints[side]=j;
        g_armPaletteCb[side]=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    }
    g_bodyPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    ID3D11Buffer* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    ID3D11Buffer* cb[4]={Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER),nullptr,Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER)};
    float globals[88]={};globals[0]=globals[5]=globals[10]=1;cb[0]->SetPrivateData(kCbCopyGuid,sizeof(globals),globals);
    CameraSummary cam={};cam.right[0]=cam.up[1]=cam.forward[2]=1;cam.mainView=1;cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
    context->VSSetConstantBuffers(0,4,cb);
    float original[576]={};
    for(int b=0;b<48;++b){original[b*12]=original[b*12+5]=original[b*12+10]=1;original[b*12+3]=b*.001f;original[b*12+7]=b*.002f;original[b*12+11]=-b*.003f;}
    cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);
    g_neutralQuietGate=false;float baseline[2][576];
    for(int side=0;side<2;++side){Check(BuildArmPalette(context,side)!=nullptr,"neutral arm draw");Read(device,context,g_armPaletteCb[side],baseline[side]);}
    for(int frame=0;frame<60;++frame)
    {
        g_frame=frame;float animation[576];std::memcpy(animation,original,sizeof(animation));
        for(int b=0;b<25;++b)
        {
            Mat4 m=BoneMatrix(original,b), turn={};const double a=.2*std::sin(frame*.2+b*.15);
            turn.m[0]=turn.m[5]=std::cos(a);turn.m[1]=-std::sin(a);turn.m[4]=std::sin(a);turn.m[10]=turn.m[15]=1;
            turn.m[3]=.1*std::sin(frame*.3);turn.m[7]=.09*std::cos(frame*.4);turn.m[11]=.08*std::sin(frame*.2+b*.1);
            StoreBone(animation,b,Multiply(turn,m));
        }
        cb[3]->SetPrivateData(kCbCopyGuid,sizeof(animation),animation);
        for(int side=0;side<2;++side)
        {
            g_armJoints[side].e[1]=-.3+.15*std::sin(frame*.2);g_armJoints[side].w[2]=.4+.1*std::cos(frame*.3);
            Check(BuildArmPalette(context,side)!=nullptr,"animated arm draw");float out[576];Read(device,context,g_armPaletteCb[side],out);
            Compare(out,baseline[side],300,"pumping/running palettes must not move controller-driven arms");
        }
        // Current animated bones are used to identify sleeves, but receive the
        // exact final arm correction, including the removed source animation.
        // 10-08 arm rewrite (deterministic arms): only the body bones that ARE arm bones are replaced - Jason's mesh: body slots
        // 7/8/9 = left arm bones 0/22/23, 11/12/13 = right arm 0/22/23 - by that arm's FINAL bones (upper-arm group: minus the
        // rigid arm's slide, so the shirt keeps the shoulder). Every other body bone keeps the game's own animation.
        Check(BuildBodyPalette(context)!=nullptr,"sleeve palette");float sleeves[576];Read(device,context,g_bodyPaletteCb,sleeves);
        {const int seed[6][3]={{7,1,0},{8,1,22},{9,1,23},{11,0,0},{12,0,22},{13,0,23}};
         for(const auto& s:seed){const float* got=sleeves+s[0]*12;const float* arm=baseline[s[1]]+s[2]*12;const V3 slide=g_armSlide[s[1]];
          const float off[12]={0,0,0,static_cast<float>(slide.x),0,0,0,static_cast<float>(slide.y),0,0,0,static_cast<float>(slide.z)};
          for(int k=0;k<12;++k)Check(std::fabs(got[k]-(arm[k]-off[k]))<1e-3f,"sleeve bones = that arm's final bones (shirt keeps the shoulder: no slide)");}
         for(int b=0;b<48;++b){bool mapped=false;for(const auto& s:seed)mapped=mapped||s[0]==b;if(mapped)continue;
          for(int k=0;k<12;++k)Check(std::fabs(sleeves[b*12+k]-animation[b*12+k])<1e-5f,"unmapped body bones keep the game's animation");}}
    }
    // Body/camera rotations must rotate the stable result once, not leave it
    // pinned in the old camera axes or apply the body rotation twice.
    g_aimActive=true;const double yaw=.6;g_aimQ[0][0]=g_aimQ[2][2]=std::cos(yaw);g_aimQ[0][2]=std::sin(yaw);g_aimQ[2][0]=-std::sin(yaw);g_aimQ[1][1]=1;
    const Mat4 q=ArmBodyToCamera();float controller[16];std::memcpy(controller,g_handLeft,sizeof(controller));Mat4 cp={};for(int i=0;i<16;++i)cp.m[i]=controller[i];cp=Multiply(q,cp);for(int i=0;i<16;++i)g_handLeft[i]=static_cast<float>(cp.m[i]);
    Check(BuildArmPalette(context,1)!=nullptr,"body-rotated stable arm");float turned[576];Read(device,context,g_armPaletteCb[1],turned);
    float expected[576];std::memcpy(expected,baseline[1],sizeof(expected));for(int b=0;b<25;++b)StoreBone(expected,b,Multiply(q,BoneMatrix(baseline[1],b)));
    Compare(turned,expected,300,"body rotation applied exactly once");
    g_aimActive=false;std::memcpy(g_handLeft,controller,sizeof(controller));
    // With a visible weapon, the right hand keeps the confirmed existing path.
    g_visibleWeaponDrawsLastFrame=6;g_frameGunValid=true;g_frameGunMove={};g_frameGunMove.m[0]=g_frameGunMove.m[5]=g_frameGunMove.m[10]=g_frameGunMove.m[15]=1;
    cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);
    Mat4 hand=g_frameGunMove,fore={},upper={};Check(SolveArm(g_armJoints[0],hand,0,fore,upper,true),"armed solve");
    const Mat4 moves[3]={upper,fore,hand};std::memcpy(expected,original,sizeof(expected));for(int b=0;b<25;++b)StoreBone(expected,b,Multiply(moves[ArmGroup(b)],BoneMatrix(original,b)));
    Check(BuildArmPalette(context,0)!=nullptr,"armed right hand");Read(device,context,g_armPaletteCb[0],turned);Compare(turned,expected,300,"armed right attachment unchanged");
    // While holding, preserve the original grip pose and physical slide path.
    g_twoHand=true;g_slideGrab=.1;g_slide=.06;g_slideAxis[0]=0;g_slideAxis[1]=0;g_slideAxis[2]=1;
    g_pumpAnimView[0]=0;g_pumpAnimView[1]=0;g_pumpAnimView[2]=-.09;
    Mat4 grip={};Check(LeftMove(g_armJoints[1],g_armJoints[0],grip),"left grip move");
    Check(SolveArm(g_armJoints[1],grip,1,fore,upper,true),"left grip solve");
    const Mat4 gripMoves[3]={upper,fore,grip};std::memcpy(expected,original,sizeof(expected));
    for(int b=0;b<25;++b)StoreBone(expected,b,Multiply(gripMoves[ArmGroup(b)],BoneMatrix(original,b)));
    Check(BuildArmPalette(context,1)!=nullptr,"held left hand");Read(device,context,g_armPaletteCb[1],turned);
    Compare(turned,expected,300,"held left hand retains game grip pose and pump-slide correction");
    // Releasing immediately restores the stable free-hand pose.
    g_twoHand=false;Check(BuildArmPalette(context,1)!=nullptr,"released left hand");Read(device,context,g_armPaletteCb[1],turned);
    Compare(turned,baseline[1],300,"release restores animation-free controller hand");
    g_motionGun=false;Check(BuildArmPalette(context,1)==nullptr,"motion-arm toggle restores game path");
    g_oCopyResource=ProbeCopy;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap;
    context->UpdateSubresource(cb[3],0,nullptr,original,0,0);
    context->VSSetConstantBuffers(0,4,cb);
    g_actionProbe=ActionProbeState{};g_actionProbe.begin=g_frame;g_actionProbe.event=1;
    g_actionProbe.resourceCount=64; // bypass disk mesh exports in this GPU-palette fixture
    g_actionProbe.file=CreateFileW(L"action_probe_fixture.bin",GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
    Check(g_actionProbe.file!=INVALID_HANDLE_VALUE,"probe fixture file");
    DrawCall action={};action.a=6795;action.b=9633;action.d=7;
    RecordActionDraw(context,action);
    Check(g_actionProbe.draws==1,"one raw action record");
    SetFilePointer(g_actionProbe.file,0,nullptr,FILE_BEGIN);
    ActionProbeRecord captured={};DWORD probeRead=0;
    Check(ReadFile(g_actionProbe.file,&captured,sizeof(captured),&probeRead,nullptr)&&probeRead==sizeof(captured),"trace readback");
    Check(captured.magic==0x41505231&&captured.version==1&&captured.event==1,"trace version and event");
    Check(captured.a==6795&&captured.b==9633&&captured.base==7&&captured.stride==40,"draw identity");
    Check(captured.cbBytes[3]==2304&&captured.cbFloats[3]==576,"complete bone palette");
    Compare(captured.cb[3],original,576,"probe stores original GPU bones");
    float untouched[576];Read(device,context,cb[3],untouched);Compare(untouched,original,576,"probe does not mutate game buffer");
    CloseActionProbe("fixture finished");
    std::puts("PASS: action probe raw GPU palettes, draw identity, file format, no buffer mutation");
    g_oCopySubresourceRegion=EjectCopyRange;
    D3D11_BUFFER_DESC effectDesc={};effectDesc.ByteWidth=256;effectDesc.Usage=D3D11_USAGE_DYNAMIC;
    effectDesc.BindFlags=D3D11_BIND_VERTEX_BUFFER;effectDesc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer* effect=nullptr;Check(SUCCEEDED(device->CreateBuffer(&effectDesc,nullptr,&effect)),"dynamic effect buffer");
    unsigned char effects[256];for(int i=0;i<256;++i)effects[i]=static_cast<unsigned char>(i);
    D3D11_MAPPED_SUBRESOURCE effectMap={};Check(SUCCEEDED(context->Map(effect,0,D3D11_MAP_WRITE_DISCARD,0,&effectMap)),"effect map");
    memcpy(effectMap.pData,effects,256);context->Unmap(effect,0);
    ID3D11Buffer* effectIndices=Buffer(device,12,D3D11_BIND_INDEX_BUFFER);
    const unsigned short effectIndexData[6]={99,99,2,4,3,99};context->UpdateSubresource(effectIndices,0,nullptr,effectIndexData,0,0);
    UINT effectStride=16,effectOffset=16;context->IASetVertexBuffers(0,1,&effect,&effectStride,&effectOffset);
    context->IASetIndexBuffer(effectIndices,DXGI_FORMAT_R16_UINT,2);
    effectDesc.ByteWidth=64;ID3D11Buffer* effectInstance=nullptr;
    Check(SUCCEEDED(device->CreateBuffer(&effectDesc,nullptr,&effectInstance)),"effect instance stream");
    Check(SUCCEEDED(context->Map(effectInstance,0,D3D11_MAP_WRITE_DISCARD,0,&effectMap)),"instance map");
    for(int i=0;i<64;++i)static_cast<unsigned char*>(effectMap.pData)[i]=static_cast<unsigned char>(200+i);
    context->Unmap(effectInstance,0);UINT instanceStride=16,instanceOffset=8;
    context->IASetVertexBuffers(1,1,&effectInstance,&instanceStride,&instanceOffset);
    g_frame=1000;g_ejection=EjectionState{};g_ejection.sequence=1;g_ejection.begun=GetTickCount64();g_ejection.burstUntil=g_ejection.begun+2000;
    g_ejection.file=CreateFileW(L"ejection_probe_fixture.bin",GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
    Check(g_ejection.file!=INVALID_HANDLE_VALUE,"ejection fixture file");
    g_chambered=false;g_pumpStage=1;g_twoHand=true;g_slideGrab=.1;g_slide=.04;MarkEjectionProbe(1);
    DrawCall effectCall={2,3,2,1,2,1};RecordEjectionDraw(context,effectCall,true);
    Check(g_ejection.draws==1,"first effect snapshot");
    for(int i=0;i<256;++i)effects[i]=static_cast<unsigned char>(255-i);
    Check(SUCCEEDED(context->Map(effect,0,D3D11_MAP_WRITE_DISCARD,0,&effectMap)),"effect reused map");
    memcpy(effectMap.pData,effects,256);context->Unmap(effect,0);g_frame=1002;
    Check(SUCCEEDED(context->Map(effectInstance,0,D3D11_MAP_WRITE_DISCARD,0,&effectMap)),"reused instance map");
    for(int i=0;i<64;++i)static_cast<unsigned char*>(effectMap.pData)[i]=static_cast<unsigned char>(100+i);
    context->Unmap(effectInstance,0);
    RecordEjectionDraw(context,effectCall,true);Check(g_ejection.draws==2,"reused effect snapshot");
    g_chambered=true;g_slide=.1;MarkEjectionProbe(2);
    SetFilePointer(g_ejection.file,0,nullptr,FILE_BEGIN);
    auto fileRead=[&](void* ptr,UINT size){DWORD n=0;Check(ReadFile(g_ejection.file,ptr,size,&n,nullptr)&&n==size,"ejection chunk read");};
    EjectionHeader ejectHeader={};EjectionMarker ejectMarker={};fileRead(&ejectHeader,sizeof(ejectHeader));fileRead(&ejectMarker,sizeof(ejectMarker));
    Check(ejectHeader.magic==0x45505231&&ejectHeader.type==0&&ejectMarker.event==1&&!ejectMarker.chambered,"rear marker recorded separately");
    for(int snapshot=0;snapshot<2;++snapshot)
    {
        EjectionDraw capturedEffect={};fileRead(&ejectHeader,sizeof(ejectHeader));fileRead(&capturedEffect,sizeof(capturedEffect));
        Check(ejectHeader.type==1&&ejectHeader.bytes==sizeof(capturedEffect)+48+6+64,"variable payload size");
        Check(capturedEffect.vertexSliceOffset==64&&capturedEffect.vertexSliceBytes==48,"VB range includes stream offset and base vertex");
        Check(capturedEffect.indexSliceOffset==4&&capturedEffect.indexSliceBytes==6,"IB range includes start index and binding offset");
        Check(capturedEffect.raw.vertexBuffer==reinterpret_cast<uintptr_t>(effect),"same reused buffer identity");
        unsigned char verts[48],ids[6];fileRead(verts,48);fileRead(ids,6);
        for(int i=0;i<48;++i)Check(verts[i]==static_cast<unsigned char>(snapshot?255-(64+i):64+i),"captures changing contents of reused effect buffer");
        Check(memcmp(ids,effectIndexData+2,6)==0,"index snapshot exact");
        Check(capturedEffect.raw.kind==2&&capturedEffect.raw.b==2&&capturedEffect.raw.c==1&&capturedEffect.raw.e==2,"instanced draw arguments preserved");
        Check(capturedEffect.extra[0].buffer==reinterpret_cast<uintptr_t>(effectInstance)&&capturedEffect.extra[0].offset==8&&capturedEffect.extra[0].snapshotBytes==64,"instance stream metadata");
        unsigned char instanceBytes[64];fileRead(instanceBytes,64);
        for(int i=0;i<64;++i)Check(instanceBytes[i]==static_cast<unsigned char>((snapshot?100:200)+i),"changing instance stream captured");
        Check(capturedEffect.cbShadowMask&(1u<<3),"original bone shadow copied");Compare(capturedEffect.raw.cb[3],original,576,"effect raw palette unchanged");
    }
    fileRead(&ejectHeader,sizeof(ejectHeader));fileRead(&ejectMarker,sizeof(ejectMarker));Check(ejectHeader.type==0&&ejectMarker.event==2&&ejectMarker.chambered,"forward marker recorded separately");
    std::vector<unsigned char> rejected;Check(!EjectionReadRange(context,effect,250,16,rejected),"out of bounds effect slice rejected");
    g_ejection.draws=4096;RecordEjectionDraw(context,effectCall,true);
    Check(g_ejection.dataLimited&&g_ejection.file!=INVALID_HANDLE_VALUE,"geometry cap preserves pump-marker channel");
    g_lastShotFrame=-100000;g_ejection.pulled=g_ejection.returned=false;
    g_chambered=false;g_pumpStage=1;g_twoHand=false;TickEjectionProbe(true);
    Check(!g_ejection.pulled,"one-handed pose cannot create physical pull marker");
    g_twoHand=true;TickEjectionProbe(true);Check(g_ejection.pulled&&!g_ejection.returned,"tick recognizes rearward threshold");
    g_chambered=true;TickEjectionProbe(true);Check(g_ejection.returned,"tick recognizes forward completion");
    EndEjectionProbe("fixture finished");effect->Release();effectIndices->Release();effectInstance->Release();
    std::puts("PASS: ejection probe live buffer reuse, bounded vertex/index slices, raw palette and rear/forward markers");
    TestPhysicalHull(device,context);
    context->ClearState();context->Flush();for(auto* b:cb)if(b)b->Release();vb->Release();g_armPaletteCb[0]->Release();g_armPaletteCb[1]->Release();g_bodyPaletteCb->Release();context->Release();device->Release();
    std::puts("PASS: free-arm animation suppressed; sleeves/body turns agree; armed right and held-left grip paths preserved; release restores stable pose; toggle restores game path.");
}
