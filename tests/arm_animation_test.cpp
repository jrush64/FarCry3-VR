#include "../src/stereo.cpp"
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
    context->ClearState();context->Flush();for(auto* b:cb)if(b)b->Release();vb->Release();g_armPaletteCb[0]->Release();g_armPaletteCb[1]->Release();g_bodyPaletteCb->Release();context->Release();device->Release();
    std::puts("PASS: free-arm animation suppressed; sleeves/body turns agree; armed right and held-left grip paths preserved; release restores stable pose; toggle restores game path.");
}
