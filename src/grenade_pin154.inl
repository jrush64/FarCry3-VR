// Mechanical extraction requests native activation155 only after pin removal.
struct GrenadePinState154 {
    bool held=false,out=false,previousGrip=false,guides=true;
    double fraction=0,reach=.09;V3 start={};LONG finishedFrame=-100000;
}g_grenadePin154;
struct GrenadePinPass154 {HullPart part[3];unsigned long long shader=0;LONG frame=-100000;uintptr_t targets[8]={};unsigned targetCount=0;};
struct GrenadePinVisual154 {GrenadePinPass154 pass[6];int count=0;ID3D11DeviceContext* deferred=nullptr;unsigned captured=0,replayed=0;}g_pinVisual154;
bool GrenadePinRequired154(){return g_throw144.manualPin154&&g_throw144.owned&&g_throw144.kind==1;}
bool GrenadePinActive154(){return GrenadePinRequired154()&&!g_throw144.releasing&&!g_throw144.launched;}
bool GrenadePinGuide154(){return GrenadePinActive154()&&g_grenadePin154.guides&&!g_grenadePin154.out;}
void ResetGrenadePin154(){const bool guide=g_grenadePin154.guides;const double reach=g_grenadePin154.reach;g_grenadePin154={};g_grenadePin154.guides=guide;g_grenadePin154.reach=reach;g_grenadePin154.previousGrip=ThrowableControls361().leftGrip>.7f;}
void PauseGrenadePin154(){g_grenadePin154.held=false;if(!g_grenadePin154.out)g_grenadePin154.fraction=0;g_grenadePin154.previousGrip=ThrowableControls361().leftGrip>.7f;}
Mat4 GrenadeKeyRoot154(int i){Mat4 inverse={};if(!Invert(ThrowMat144(kThrowGrenadeClipA[i].part[0]),inverse))return HandIdentity();return Multiply(ThrowMat144(kThrowGrenadeClipA[kThrowGrenadeAIdx_hold].part[0]),inverse);}
Mat4 GrenadeKeyHand154(int i){return Multiply(GrenadeKeyRoot154(i),ThrowMat144(kThrowGrenadeClipA[i].leftWrist));}
V3 GrenadePinPalm154(int i){return Apply(GrenadeKeyHand154(i),{-.6781,.0519,1.0387});}
constexpr int kPinExtractEnd155=4; // Stop before the captured arm toss-away sweep.
double GrenadePinTravel154(){return .05;}
void GrenadePinSample154(Mat4& hand,float* fingers,Mat4* part=nullptr){
    const auto& s=g_grenadePin154;const double f=(std::max)(0.0,(std::min)(1.0,s.fraction));
    const double t=kThrowGrenadeClipA[kThrowGrenadeAIdx_pinGripped].t+f*(kThrowGrenadeClipA[kPinExtractEnd155].t-kThrowGrenadeClipA[kThrowGrenadeAIdx_pinGripped].t);
    int a=kThrowGrenadeAIdx_pinGripped;while(a<kPinExtractEnd155&&kThrowGrenadeClipA[a+1].t<=t)++a;
    const int b=(std::min)(a+1,kPinExtractEnd155);const double span=kThrowGrenadeClipA[b].t-kThrowGrenadeClipA[a].t,w=span>1e-6?(t-kThrowGrenadeClipA[a].t)/span:0;
    hand=PistolRigidBlend(GrenadeKeyHand154(a),GrenadeKeyHand154(b),w);
    if(fingers)for(int bone=0;bone<17;++bone){const Mat4 m=PistolRigidBlend(BoneMatrix(kThrowGrenadeClipA[a].leftFingers,bone),BoneMatrix(kThrowGrenadeClipA[b].leftFingers,bone),w);for(int j=0;j<12;++j)fingers[bone*12+j]=float(m.m[j]);}
    if(part)for(int p=1;p<=3;++p)part[p-1]=PistolRigidBlend(Multiply(GrenadeKeyRoot154(a),ThrowMat144(kThrowGrenadeClipA[a].part[p])),Multiply(GrenadeKeyRoot154(b),ThrowMat144(kThrowGrenadeClipA[b].part[p])),w);
}
Mat4 GrenadePart154(int part){if(!part||!GrenadePinRequired154())return GrenadePart146(part);Mat4 hand={},parts[3];GrenadePinSample154(hand,nullptr,parts);return parts[part-1];}
bool GrenadePinReady154(){for(int i=0;i<g_pinVisual154.count;++i)if(g_pinVisual154.pass[i].part[0].ready&&g_pinVisual154.pass[i].part[1].ready)return true;return false;}
bool GrenadePinArm154(const Mat4& toView,const float* bones,Mat4& move){
    if(!GrenadePinActive154()||(!g_grenadePin154.held&&g_grenadePin154.finishedFrame!=g_frame)||!g_handLeftValid||!g_frameCameraValid)return false;
    Mat4 frame={},hand={},inverse={};if(!ThrowableFrame144(g_frameCamera,frame)||!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
    GrenadePinSample154(hand,nullptr);move=Multiply(Multiply(frame,hand),inverse);return true;
}
bool GrenadePinFingers154(int side,float* bones){
    if(side!=1||!GrenadePinActive154()||(!g_grenadePin154.held&&g_grenadePin154.finishedFrame!=g_frame)||!g_handLeftValid)return false;
    Mat4 hand={};float fingers[204];GrenadePinSample154(hand,fingers);const Mat4 wrist=BoneMatrix(bones,2);
    for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(fingers,b-3)));return true;
}
void TickGrenadePin154(){
    auto& s=g_grenadePin154;const auto c=ThrowableControls361();s.previousGrip=c.leftGrip>.7f;
    if(!GrenadePinActive154())return;
    if(!g_handLeftValid||!c.active){PauseGrenadePin154();return;}
    if(s.held&&c.leftGrip<.3f){s.held=false;if(!s.out)s.fraction=0;Log("[PIN154] left grip released; pin %s",s.out?"OUT":"returned to socket");}
    Mat4 frame={},qi={},body={},inverse={};V3 palm={};
    if(!g_frameCameraValid||!ThrowableFrame144(g_frameCamera,frame)||!Invert(ArmBodyToCamera(),qi)||!FreeHandPalm(1,palm))return;
    body=Multiply(qi,frame);if(!Invert(body,inverse))return;const V3 local=Apply(inverse,palm),origin=GrenadePinPalm154(kThrowGrenadeAIdx_pinGripped);
    const V3 delta=Sub(GrenadePinPalm154(kPinExtractEnd155),origin),axis=Unit(delta);const double distance=Length(Sub(local,origin));
    if(c.leftGrip>.7f&&!s.held&&!s.out&&distance<=s.reach&&GrenadePinReady154()){s.held=true;s.start=local;s.fraction=0;Log("[PIN154] LEFT GRIP acquired native pin pose; stroke %.3fm",GrenadePinTravel154());}
    if(s.held&&!s.out){const V3 moved=Sub(local,s.start);const double along=Dot(moved,axis);
        if(along>=-.01){s.fraction=(std::max)(0.0,(std::min)(1.0,Length(moved)/GrenadePinTravel154()));if(s.fraction>=.99){s.fraction=1;s.out=true;s.held=false;s.finishedFrame=g_frame;const bool requested=fc3throw::RequestPinOut155(g_throw144.prepared);Log("[PIN155] pin OUT; 5cm extraction complete; left hand FREE next frame; native activation request %d",requested?1:0);}}
    }
    if(s.guides&&!s.out){const V3 target=Apply(body,s.held?Add(origin,Scale(axis,GrenadePinTravel154())):origin);const float p[3]={float(target.x),float(target.y),float(target.z)},h[3]={float(palm.x),float(palm.y),float(palm.z)};const float scale=(std::max)(.1f,g_worldScale);fc3xr::SetGrabMarkers(p,h,distance<=s.reach,float(.035/scale),float(.05/scale));}
}
bool GrenadePinNote154(){
    if(!GrenadePinActive154())return false;
    if(!GrenadePinReady154())fc3xr::SetTestNote(L"GRENADE PIN\nWaiting for native pin meshes. Keep RIGHT GRIP held.");
    else if(g_grenadePin154.held&&!g_grenadePin154.out){wchar_t text[256];swprintf_s(text,L"PULL THE PIN - %.0f%%\nKeep LEFT GRIP held. Pull about 5 cm away from the grenade.\nHand and pin STOP at full travel.",100*g_grenadePin154.fraction);fc3xr::SetTestNote(text);}
    else if(g_grenadePin154.out)fc3xr::SetTestNote(fc3throw::PinArmed155(g_throw144.prepared)?L"PIN OUT - LIVE GRENADE\nYour LEFT HAND is free.\nSwing and RELEASE RIGHT GRIP to throw.":L"PIN OUT\nYour LEFT HAND is free.\nWaiting for native arming acknowledgement.");
    else fc3xr::SetTestNote(L"GRENADE - PIN IN (SAFE)\nRIGHT GRIP holds it. Release to DROP or THROW.\nTo arm: LEFT PALM to ring, LEFT GRIP and pull 5cm.\nUse the weapon wheel to stow it while still held.");
    return true;
}
int GrenadePinPart154(const DrawCall& call){return call.kind==0?(call.b==2334&&call.a==504?0:call.b==2838&&call.a==324?1:call.b==3162&&call.a==1476?2:-1):-1;}
int GrenadePinPassIndex154(ID3D11DeviceContext* c,bool create){
    unsigned long long shader=0;DXGI_FORMAT formats[8]={},depth=DXGI_FORMAT_UNKNOWN;if(!BowArrowPassKey(c,shader,formats,depth))return -1;
    bool color=false;for(auto f:formats)color=color||f!=DXGI_FORMAT_UNKNOWN;if(!color)return -1;
    for(int i=0;i<g_pinVisual154.count;++i){const auto& p=g_pinVisual154.pass[i];if(p.shader!=shader)continue;const HullPart* first=nullptr;for(const auto& x:p.part)if(x.ready){first=&x;break;}if(!first){if(create)return i;continue;}
        bool same=first->depthFormat==depth;for(int j=0;j<8;++j)same=same&&first->formats[j]==formats[j];if(same)return i;}
    if(!create||g_pinVisual154.count==6)return -1;const int i=g_pinVisual154.count++;g_pinVisual154.pass[i].shader=shader;return i;
}
void CaptureGrenadePin154(ID3D11DeviceContext* c,const DrawCall& call){
    if(!g_dev||!g_throw144.manualPin154)return;const int part=GrenadePinPart154(call);if(part<0)return;
    const int pass=GrenadePinPassIndex154(c,true);if(pass<0)return;auto& p=g_pinVisual154.pass[pass].part[part];if(p.ready)return;
    ID3D11Buffer* cb=nullptr;c->VSGetConstantBuffers(0,1,&cb);std::vector<float> data(kSmallCbMax/4);const UINT bytes=ReadCopy(cb,data.data(),kSmallCbMax);SafeRelease(cb);if(bytes<80)return;data.resize(bytes/4);
    if(!SnapshotVisualPart(c,call,p,data)){ClearBowArrowPart(p);return;}
    if(!g_pinVisual154.deferred&&FAILED(g_dev->CreateDeferredContext(0,&g_pinVisual154.deferred))){ClearBowArrowPart(p);return;}
    ++g_pinVisual154.captured;Log("[PIN154] retained native pin part%d pass%d for persistent stereo draw",part,pass);
}
bool ThrowablePinHidden154(ID3D11DeviceContext* c,const DrawCall& call){return GrenadePinRequired154()&&ThrowableGeometry144(c)==1&&GrenadePinPart154(call)>=0;}
void ReplayGrenadePin154(ID3D11DeviceContext* c,const DrawCall& call){
    if(!GrenadePinActive154()||call.kind!=0||call.b!=0||call.a!=2334||!g_pinVisual154.deferred||(g_grenadePin154.out&&!g_grenadePin154.held))return;
    const int bank=GrenadePinPassIndex154(c,false);if(bank<0)return;auto& cache=g_pinVisual154.pass[bank];
    ID3D11RenderTargetView* target[8]={},*twin[8]={};ID3D11DepthStencilView* depth=nullptr,*twinDepth=nullptr;c->OMGetRenderTargets(8,target,&depth);
    const uintptr_t key=reinterpret_cast<uintptr_t>(target[0]);bool done=false;if(cache.frame!=g_frame){cache.frame=g_frame;cache.targetCount=0;}for(unsigned i=0;i<cache.targetCount;++i)done=done||cache.targets[i]==key;
    ID3D11Buffer* cameraBuffer=nullptr;c->VSGetConstantBuffers(1,1,&cameraBuffer);CameraSummary camera={};Mat4 frame={},inverse={};
    bool ok=!done&&PistolGuideCamera(c,camera)&&ThrowableFrame144(camera,frame)&&Invert(HullView(camera),inverse)&&depth&&g_eyeCb[1][0]&&g_eyeCb[1][1];
    if(ok){for(int i=0;i<8;++i)if(target[i]){twin[i]=static_cast<ID3D11RenderTargetView*>(TwinView(target[i],ViewKind::RenderTarget,false));ok=ok&&twin[i];}twinDepth=static_cast<ID3D11DepthStencilView*>(TwinView(depth,ViewKind::DepthStencil,false));ok=ok&&twinDepth;}
    if(ok){D3D11_VIEWPORT viewport[16]={};UINT n=16;c->RSGetViewports(&n,viewport);D3D11_RECT scissor[16]={};UINT ns=16;c->RSGetScissorRects(&ns,scissor);
        const bool own=g_own;g_own=true;auto* dc=g_pinVisual154.deferred;dc->ClearState();dc->RSSetViewports(n,viewport);dc->RSSetScissorRects(ns,scissor);
        for(int i=0;i<3;++i){auto& p=cache.part[i];if(!p.ready)continue;auto data=p.data;WritePistolWorld(data.data(),Multiply(inverse,Multiply(frame,GrenadePart154(i+1))));g_oUpdateSubresource(c,p.globals,0,nullptr,data.data(),0,0);
            UINT stride=32,offset=0;dc->IASetVertexBuffers(0,1,&p.vb,&stride,&offset);dc->IASetIndexBuffer(p.ib,DXGI_FORMAT_R16_UINT,p.indexOffset);dc->IASetInputLayout(p.layout);dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);dc->VSSetShader(p.vs,nullptr,0);dc->PSSetShader(p.ps,nullptr,0);
            ID3D11Buffer* cb[3]={p.globals,cameraBuffer,p.clip};dc->VSSetConstantBuffers(0,3,cb);dc->PSSetConstantBuffers(0,8,p.psCb);dc->PSSetShaderResources(0,16,p.textures);dc->PSSetSamplers(0,16,p.samplers);dc->RSSetState(p.raster);dc->OMSetDepthStencilState(p.depth,p.stencil);dc->OMSetBlendState(p.blend,p.blendFactor,p.mask);
            for(int eye=0;eye<2;++eye){dc->OMSetRenderTargets(p.targets,eye?twin:target,eye?twinDepth:depth);dc->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyeCb[1][eye]);if(HasFlag(p.ps,kPsPatchedGuid))dc->PSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyePsCb[eye]);dc->DrawIndexed(p.draw.a,p.draw.b,p.draw.d);}
        }
        ID3D11CommandList* list=nullptr;if(SUCCEEDED(dc->FinishCommandList(FALSE,&list))){c->ExecuteCommandList(list,TRUE);SafeRelease(list);if(cache.targetCount<8)cache.targets[cache.targetCount++]=key;++g_pinVisual154.replayed;}g_own=own;
    }
    for(int i=0;i<8;++i){SafeRelease(target[i]);SafeRelease(twin[i]);}SafeRelease(depth);SafeRelease(twinDepth);SafeRelease(cameraBuffer);
}
void ReleaseGrenadePin154(){for(auto& pass:g_pinVisual154.pass)for(auto& p:pass.part)ClearBowArrowPart(p);SafeRelease(g_pinVisual154.deferred);g_pinVisual154={};ResetGrenadePin154();}
