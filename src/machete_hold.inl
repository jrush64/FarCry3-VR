// Persistent visual ownership, separate from native melee damage. The first
// acquisition requests one stock swipe; cached draws never request melee again.
#include "captured_machete_grip.inl"
struct MacheteVisual
{
    HullPart parts[3];ID3D11Buffer* skin[3]={};
    int blade=-1,partCount=3;V3 bladeBase={},bladeTip={}; // kMacheteBlades index; parts in use; blade axis in model space
    ID3D11DeviceContext* deferred=nullptr;
    Mat4 hand={};LONG handFrame=-100000,replayFrame=-100000;
    Mat4 controllerHand116={};UINT handEpoch116=~0u;bool handAttachment116=false;
    uintptr_t nativeTargets116[8]={};LONG nativeFrames116[8]={};int nativeCount116=0;
    UINT nativeDraws116=0,nativeFail116=0;
    // Candidate 115 (run 10:32 status lines): the game draws the arm twice per frame in two passes with
    // different cameras; the blade went into the first pass only and the visible pass was skipped as
    // "already drawn". The hand now belongs to the pass (render target) it was recorded in, and the
    // blade is drawn once per pass, in every pass where the arm hand was recorded.
    uintptr_t handTarget=0;uintptr_t doneTargets[8]={};int doneCount=0;LONG doneFrame=-100000;
    ULONGLONG started=0,motionUntil=0;bool held=false,previousClick=false,previousChord=false,logged=false,bodyDraw285=false;
} g_machete;
struct MachetePass116 { HullPart parts[3];ID3D11Buffer* skin[3]={}; } g_machetePass116[3];
bool g_macheteEnabled=true,g_macheteHoldToDraw=false; // future Insert selector; default Toggle
void ReleaseMachetePart117(HullPart& p,ID3D11Buffer*& skin)
{
    SafeRelease(p.layout);SafeRelease(p.vs);SafeRelease(p.ps);SafeRelease(p.vb);SafeRelease(p.ib);
    SafeRelease(p.globals);SafeRelease(p.clip);SafeRelease(skin);
    for(auto*& b:p.psCb)SafeRelease(b);for(auto*& t:p.textures)SafeRelease(t);for(auto*& s:p.samplers)SafeRelease(s);
    SafeRelease(p.raster);SafeRelease(p.depth);SafeRelease(p.blend);p=HullPart{};
}
bool MacheteHeld(){return g_machete.held;}
// Candidate 114 replay status counters (see TickMacheteReplayStatus).
struct MacheteReplayStatus{UINT handGrip=0,handBone=0,handFail=0,calls=0,drawn=0,notReady=0,noHandThisFrame=0,already=0,otherPass=0,camera=0,targets=0,parts=0,twins=0,commandList=0,passes=0;ULONGLONG next=0;V3 lastHand={};}g_macheteReplayStatus;
// The pass a draw belongs to: its first render target view.
uintptr_t MacheteDrawTarget(ID3D11DeviceContext* context)
{
    ID3D11RenderTargetView* rtv=nullptr;context->OMGetRenderTargets(1,&rtv,nullptr);
    const uintptr_t id=reinterpret_cast<uintptr_t>(rtv);if(rtv)rtv->Release();return id;
}
// Only an explicitly drawn machete can bridge native attack geometry gaps.
// A missing firearm cannot acquire ownership, and the grace is bounded.
bool MacheteReady();
// Candidate 108 (once drawn, the machete could not be moved freely; after RT,
// motion controls worked only for a second or two): the game never draws an idle machete, so the timed
// grace ran out between swipes and motion stopped. While the mod's own persistent blade replay is
// ready the blade IS visible, so the lease holds for as long as it is held; the timed grace covers
// only the acquisition before the first swipe completes.
bool MacheteMotionLease(){return g_machete.held && (MacheteReady() || GetTickCount64()<g_machete.motionUntil);}
// Per-blade draw signatures (offline asset corpus + live captures). machete.xbg: three sections, blade first.
// tanto_tp.xbg (what the native swipe draws for the Japanese tanto, run 14616): two sections, blade second.
struct MacheteBladeProfile{const char* name;UINT vb,ib,stride,offset;unsigned long long vs;int partCount;UINT starts[3],counts[3];V3 bladeBase,bladeTip;};
const MacheteBladeProfile kMacheteBlades[2]={
    {"machete.xbg",71720,12120,40,0,0xA38F7CD233BCE12Bull,3,{0,2910,4704},{2910,1794,1356},{.02612384408712387,.0029908139258623123,.08346812333911657},{-.007904293946921825,.0029908139258623123,.4286324651911855}},
    {"tanto_tp.xbg",83720,12780,40,0,0xA38F7CD233BCE12Bull,2,{0,6000,0},{6000,390,0},{-.0044,.003,.1212},{-.0044,.003,.4339}},
};
bool MacheteReady(){if(!g_machete.deferred)return false;for(int i=0;i<g_machete.partCount;++i)if(!g_machete.parts[i].ready)return false;return true;}
bool MacheteBladeContactActive();
bool MacheteStrokePosition116(V3& position);
// Swing-to-native-attack bridge; holding resources stay independent.
#include "machete_melee.inl"
void __cdecl ObserveMacheteRouting(UINT kind,const UINT* saved);
#include "machete_native_probe.inl"
#include "machete_routing_probe.inl"
bool MacheteBladeObserveQuick(UINT kind,const UINT* saved,UINT stack);
#include "machete_quick_probe.inl"
#include "machete_blade_contact.inl"
void LoadMacheteSettings(const wchar_t* path)
{
    LoadMacheteCollider118(path);
    wchar_t value[32]={};GetPrivateProfileStringW(L"Machete",L"Enabled",L"1",value,32,path);
    g_macheteEnabled=wcstod(value,nullptr)!=0;
    GetPrivateProfileStringW(L"Machete",L"HoldToDraw",L"0",value,32,path);
    g_macheteHoldToDraw=wcstod(value,nullptr)!=0;
    fc3pad::SetMacheteHoldMode(g_macheteHoldToDraw);
}
void ClearMacheteVisual()
{
    StopMacheteMelee();
    for(int bank=-1;bank<3;++bank)for(int i=0;i<3;++i)
    {
        auto& p=bank<0?g_machete.parts[i]:g_machetePass116[bank].parts[i];
        SafeRelease(p.layout);SafeRelease(p.vs);SafeRelease(p.ps);SafeRelease(p.vb);SafeRelease(p.ib);
        SafeRelease(p.globals);SafeRelease(p.clip);SafeRelease(bank<0?g_machete.skin[i]:g_machetePass116[bank].skin[i]);
        for(auto*& b:p.psCb)SafeRelease(b);for(auto*& t:p.textures)SafeRelease(t);for(auto*& s:p.samplers)SafeRelease(s);
        SafeRelease(p.raster);SafeRelease(p.depth);SafeRelease(p.blend);
    }
    for(auto& bank:g_machetePass116)bank=MachetePass116{};
    SafeRelease(g_machete.deferred);const bool previousClick=fc3pad::Latest().rightStickClick;g_machete=MacheteVisual{};g_machete.previousClick=previousClick;
    const auto c=fc3pad::Latest();g_machete.previousChord=c.rightStickClick&&c.rightGrip>.7f&&!c.leftStickClick;
    fc3pad::SetMacheteContext(false);
    fc3pad::SetMacheteHandling(false);fc3pad::CancelMacheteSource();
}
void StowMachete(const char* reason)
{
    if(g_machete.held)Log("[MACHETE] put away: %s; firearm chamber/ammo unchanged",reason);
    StopMacheteMelee();g_machete.motionUntil=0;g_machete.held=false;g_machete.bodyDraw285=false;g_machete.handFrame=-100000;
    fc3pad::CancelMacheteSource();fc3pad::SetMacheteHandling(false);
}
void TickMacheteReplayStatus();
void TickMachete(bool gameplay)
{
    TickMacheteReplayStatus();
    bool seated128=false;int seat128=-1;
    if(fc3aim::NativeActionOwnsArms128()||(fc3aim::VehicleState(seated128,seat128)&&seated128)){
        StowMachete("native action/vehicle owns hands");fc3pad::SetMacheteContext(false);
        const auto c=fc3pad::Latest();g_machete.previousClick=c.rightStickClick;
        g_machete.previousChord=c.rightStickClick&&c.rightGrip>.7f&&!c.leftStickClick;return;
    }
    const auto c=fc3pad::Latest();
    if(g_machete.held&&g_visibleWeaponDrawsLastFrame>0)
        g_machete.motionUntil=(std::max)(g_machete.motionUntil,GetTickCount64()+350);
    const bool context=gameplay&&g_enabled&&MotionInteractionEnabled()&&g_eyeViewsSet&&g_handRightValid&&c.active&&g_macheteEnabled;
    fc3pad::SetMacheteContext(context);
    const bool chord=c.rightStickClick&&c.rightGrip>.7f&&!c.leftStickClick;
    const bool edge=g_macheteHoldToDraw?chord&&!g_machete.previousChord:c.rightStickClick&&!g_machete.previousClick;
    g_machete.previousClick=c.rightStickClick;g_machete.previousChord=chord;
    if(!context){StowMachete("tracking/gameplay/feature unavailable");return;}
    if(fc3pad::WeaponSelectionRequested()){StowMachete("weapon selection");return;}
    if(g_macheteHoldToDraw&&g_machete.held&&!g_machete.bodyDraw285&&!chord)StowMachete("hold combination released");
    if(edge&&chord)
    {
        if(g_machete.held)StowMachete("draw toggle");
        else if(g_reload.held||g_reload.pending||ReloadPoseActive()||g_reloadAlignment)
            Log("[MACHETE] draw waits for the current shell/reload to finish");
        else
        {
            g_machete.held=true;g_machete.started=GetTickCount64();g_machete.motionUntil=g_machete.started+3000;g_machete.handFrame=-100000;
            g_machete.logged=false;fc3pad::CancelReloadRequest();fc3pad::SetMacheteHandling(true);
            if(!MacheteReady())
            {
                fc3pad::RequestMacheteSource();
                Log("[MACHETE] first draw requests ONE native swipe to acquire live materials; blade collider owns damage after acquisition");
            }
            else Log("[MACHETE] cached draw; no native melee request");
        }
    }
    if(g_machete.held&&MacheteReady())fc3pad::CancelMacheteSource();
    if(g_machete.held&&!MacheteReady()&&GetTickCount64()-g_machete.started>3000)
        StowMachete("source capture timed out; no automatic retry");
    TickMacheteMelee(c,GetTickCount64());
}
void MacheteNote()
{
    if(!MacheteHeld())return;
    wchar_t note[384];swprintf_s(note,L"MACHETE\nRight grip + R3: put away.\nBlade contact damage: %s | enemy events %u\nInsert > MACHETE: adjust contact capsule.\nPhysical movement only; RT does not request a swipe.",g_macheteBladeContact.installed?L"ready":L"waiting for collision hooks",g_macheteBladeContact.actorHits);
    fc3xr::SetTestNote(note);
}

int MachetePart(ID3D11DeviceContext* context,const DrawCall& draw)
{
    if(draw.kind!=0||draw.d!=0)return -1;
    int blade=-1,part=-1;
    for(int b=0;b<2&&part<0;++b)for(int i=0;i<kMacheteBlades[b].partCount;++i)if(draw.b==kMacheteBlades[b].starts[i]&&draw.a==kMacheteBlades[b].counts[i]){blade=b;part=i;break;}
    if(part<0)return -1;const auto& p=kMacheteBlades[blade];
    ID3D11VertexShader* shader=nullptr;context->VSGetShader(&shader,nullptr,nullptr);
    const auto hash=ShaderHash(shader);SafeRelease(shader);if(hash!=p.vs&&hash!=0xCB43EE096F50DBDBull)return -1;
    ID3D11Buffer* vb=nullptr,*ib=nullptr;UINT stride=0,offset=0,indexOffset=0;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
    context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);context->IAGetIndexBuffer(&ib,&format,&indexOffset);
    D3D11_BUFFER_DESC vd={},id={};if(vb)vb->GetDesc(&vd);if(ib)ib->GetDesc(&id);SafeRelease(vb);SafeRelease(ib);
    if(!(vd.ByteWidth==p.vb&&id.ByteWidth==p.ib&&stride==p.stride&&offset==p.offset&&format==DXGI_FORMAT_R16_UINT&&indexOffset==0))return -1;
    // one blade per acquisition: the first matching draw selects the profile
    if(g_machete.blade<0){g_machete.blade=blade;g_machete.partCount=p.partCount;g_machete.bladeBase=p.bladeBase;g_machete.bladeTip=p.bladeTip;Log("[MACHETE] blade profile %s (%d sections)",p.name,p.partCount);}
    else if(g_machete.blade!=blade)return -1;
    return part;
}

bool MachetePassMatches116(ID3D11DeviceContext* context,const HullPart& part)
{
    if(!part.ready)return false;
    ID3D11RenderTargetView* targets[8]={};ID3D11DepthStencilView* depth=nullptr;
    context->OMGetRenderTargets(8,targets,&depth);
    bool ok=HullFormat(depth)==part.depthFormat;
    for(int i=0;i<8;++i){ok=ok&&HullFormat(targets[i])==part.formats[i];SafeRelease(targets[i]);}
    SafeRelease(depth);return ok;
}
struct MachetePassScope116
{
    int bank=-1;bool valid=false;
    MachetePassScope116(ID3D11DeviceContext* context,bool capture)
    {
        if(!g_machete.parts[0].ready||MachetePassMatches116(context,g_machete.parts[0])){valid=true;return;}
        for(int i=0;i<3;++i)if(MachetePassMatches116(context,g_machetePass116[i].parts[0])){bank=i;break;}
        if(bank<0&&capture)for(int i=0;i<3;++i)if(!g_machetePass116[i].parts[0].ready){bank=i;break;}
        valid=bank>=0;if(valid)Swap();
    }
    void Swap(){for(int i=0;i<3;++i){std::swap(g_machete.parts[i],g_machetePass116[bank].parts[i]);std::swap(g_machete.skin[i],g_machetePass116[bank].skin[i]);}}
    ~MachetePassScope116(){if(bank>=0)Swap();}
};
bool NativeMacheteDraw116(ID3D11DeviceContext* context,const DrawCall& draw)
{
    return MachetePart(context,draw)>=0;
}
bool MacheteHand116(Mat4& hand)
{
    if(!g_handRightValid||!g_machete.handAttachment116||g_machete.handEpoch116!=g_armPoseEpoch)return false;
    Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
    hand=Multiply(ArmBodyToCamera(),Multiply(controller,g_machete.controllerHand116));
    for(double v:hand.m)if(!std::isfinite(v))return false;
    return true;
}
bool MacheteStrokePosition116(V3& position)
{
    if(!g_machete.handAttachment116||g_machete.handEpoch116!=g_armPoseEpoch||g_worldScale<=.01f)return false;
    Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
    const Mat4 blade=Multiply(controller,Multiply(g_machete.controllerHand116,kMacheteFromHand[0]));
    position=Apply(blade,g_machete.blade>=0?g_machete.bladeTip:kMacheteBladeTip);
    position=Scale(position,1.0/g_worldScale);
    return std::isfinite(position.x)&&std::isfinite(position.y)&&std::isfinite(position.z);
}
ID3D11Buffer* NativeMachetePalette116(ID3D11DeviceContext* context,const DrawCall& draw)
{
 ClearMirrorPalette338();
    auto& m=g_machete;const uintptr_t target=MacheteDrawTarget(context);
    int slot=0;for(;slot<m.nativeCount116;++slot)if(m.nativeTargets116[slot]==target)break;
    if(slot<8){if(slot==m.nativeCount116)++m.nativeCount116;m.nativeTargets116[slot]=target;m.nativeFrames116[slot]=g_frame;}
    // A fallback may already have rendered this pass before its first native
    // piece arrived. Keep that frame's single blade, then prefer native draws.
    if(m.doneFrame==g_frame)for(int i=0;i<m.doneCount;++i)if(m.doneTargets[i]==target)return nullptr;
    float bones[576]={};Mat4 toView={},fromView={},hand={};
    if(!g_weaponPaletteCb||!DrawSpace(context,bones,toView,fromView)||!MacheteHand116(hand)){++m.nativeFail116;return nullptr;}
    const int part=MachetePart(context,draw);if(part<0)return nullptr;
    // Absolute hand attachment removes the canned swipe's animated root.
    // Applying a gun delta alone would keep that animation on the blade.
    const Mat4 root=Multiply(fromView,Multiply(hand,kMacheteFromHand[part]));
    for(int i=0;i<48;++i)StoreBone(bones,i,HandIdentity());StoreBone(bones,0,root);
    const bool own=g_own;g_own=true;
    g_oUpdateSubresource(context,g_weaponPaletteCb,0,nullptr,bones,0,0);
    g_own=own;++m.nativeDraws116;
    if(m.nativeDraws116<=6)Log("[MACHETE NATIVE116] target %p section %d palette follows controller; original pass pipeline",reinterpret_cast<void*>(target),part);
    return g_weaponPaletteCb;
}

void CaptureMachete(ID3D11DeviceContext* context,const DrawCall& draw)
{
    const int i=MachetePart(context,draw);if(i<0)return;
    ActionProbeNoteBladeSeen(); // read-only: preserve late R3 recording slots
    if(i==0)ObserveNativeMachete(GetTickCount64());
    // Shadow/depth-only draws are not persistent colour-visual sources. They
    // must never invalidate acquisition or take a slot in the colour cache.
    ID3D11RenderTargetView* color117=nullptr;context->OMGetRenderTargets(1,&color117,nullptr);
    const bool hasColor117=color117!=nullptr;SafeRelease(color117);if(!hasColor117)return;
    MachetePassScope116 pass116(context,true);if(!pass116.valid)return;
    if(g_machete.parts[i].ready)return;
    ID3D11Buffer* globals=nullptr,*skin=nullptr;context->VSGetConstantBuffers(0,1,&globals);context->VSGetConstantBuffers(3,1,&skin);
    std::vector<float> data(kSmallCbMax/4);const UINT bytes=ReadCopy(globals,data.data(),kSmallCbMax);
    D3D11_BUFFER_DESC sd={};if(skin)skin->GetDesc(&sd);bool ok=bytes>=80&&sd.ByteWidth==2304;
    ID3D11GeometryShader* gs=nullptr;ID3D11HullShader* hs=nullptr;ID3D11DomainShader* ds=nullptr;
    context->GSGetShader(&gs,nullptr,nullptr);context->HSGetShader(&hs,nullptr,nullptr);context->DSGetShader(&ds,nullptr,nullptr);
    ok=ok&&!gs&&!hs&&!ds;SafeRelease(gs);SafeRelease(hs);SafeRelease(ds);
    if(ok)
    {
        data.resize(bytes/4);auto& p=g_machete.parts[i];
        ok=SnapshotVisualPart(context,draw,p,data)&&HullClone(context,skin,D3D11_BIND_CONSTANT_BUFFER,&g_machete.skin[i]);
        for(UINT slot=1;slot<8&&ok;++slot)
        {
            ID3D11Buffer* b=nullptr;context->PSGetConstantBuffers(slot,1,&b);
            ok=HullClone(context,b,D3D11_BIND_CONSTANT_BUFFER,&p.psCb[slot]);SafeRelease(b);
        }
        if(ok&&!g_machete.deferred)ok=SUCCEEDED(g_dev->CreateDeferredContext(0,&g_machete.deferred));
    }
    SafeRelease(globals);SafeRelease(skin);
    if(!ok)
    {
        // Candidate 116 regressed draw/hold: a later unsupported shader pass
        // cleared the entire tool after the first colour pass acquired it.
        // Discard only this failed section; preserve - Hinny that GOAT - ownership, input edges,
        // the valid primary cache, and every other pass's retained resources.
        ReleaseMachetePart117(g_machete.parts[i],g_machete.skin[i]);
        static UINT failures117=0;if(++failures117<=8)Log("[MACHETE CAPTURE117] skipped unsupported pass section %d bank %d globals %u skin %u; held state preserved",i,pass116.bank,bytes,sd.ByteWidth);
    }
    else if(MacheteReady())Log("[MACHETE] blade, grip and guard retained; native swipe may finish normally");
}
bool MacheteGrip(ID3D11DeviceContext* context,float* bones)
{
    if(!MacheteHeld())return false;
    ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);
    D3D11_BUFFER_DESC d={};if(vb)vb->GetDesc(&d);SafeRelease(vb);
    if(d.ByteWidth!=316240||stride!=40||offset!=0)return false;
    const Mat4 hand=BoneMatrix(bones,2);
    for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(hand,BoneMatrix(kMacheteGripRelative,b-3)));
    return true;
}
void CaptureMacheteColliderGuide118(ID3D11DeviceContext*);
void RecordMacheteHand(ID3D11DeviceContext* context,const float* bones,bool fromGrip=true)
{
    if(!MacheteHeld())return;
    auto& rs=g_macheteReplayStatus;
    if(HandDrawView(context,bones,2,g_machete.hand)){g_machete.handFrame=g_frame;g_machete.handTarget=MacheteDrawTarget(context);
        Mat4 controller={},ci={},qi={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
        if((!g_machete.handAttachment116||g_machete.handEpoch116!=g_armPoseEpoch)&&Invert(controller,ci)&&Invert(ArmBodyToCamera(),qi)){
            g_machete.controllerHand116=Multiply(ci,Multiply(qi,g_machete.hand));g_machete.handEpoch116=g_armPoseEpoch;g_machete.handAttachment116=true;
        }
        Mat4 pinned118={};if(MacheteHand116(pinned118))g_machete.hand=pinned118;
        CaptureMacheteColliderGuide118(context);
        ++(fromGrip?rs.handGrip:rs.handBone);rs.lastHand={g_machete.hand.m[3],g_machete.hand.m[7],g_machete.hand.m[11]};}
    else{g_machete.handFrame=-100000;++rs.handFail;}
}
void TickMacheteReplayStatus()
{
    auto& rs=g_macheteReplayStatus;const ULONGLONG now=GetTickCount64();
    if(!MacheteHeld()){rs=MacheteReplayStatus{};return;}
    if(now<rs.next)return;rs.next=now+500;
    static UINT lines=0;if(++lines>240)return;
    Log("[MACHETE REPLAY] hand from machete arm %u, from any arm %u, failed %u | replay calls %u drawn %u (second-pass draws %u) | skipped: not ready %u, no hand this frame %u, other pass %u, this pass already drawn %u, camera %u, targets %u, parts %u, twin views %u, command list %u | hand %.3f/%.3f/%.3f",
        rs.handGrip,rs.handBone,rs.handFail,rs.calls,rs.drawn,rs.passes,rs.notReady,rs.noHandThisFrame,rs.otherPass,rs.already,rs.camera,rs.targets,rs.parts,rs.twins,rs.commandList,rs.lastHand.x,rs.lastHand.y,rs.lastHand.z);
    Log("[MACHETE NATIVE116] moved %u, unavailable %u, passes %d; swing requests %u",g_machete.nativeDraws116,g_machete.nativeFail116,g_machete.nativeCount116,g_macheteMelee.sequence);
    const V3 keep=rs.lastHand;rs=MacheteReplayStatus{};rs.next=now+500;rs.lastHand=keep;
}
// Candidate 114 status (the held machete froze again while the arm and the swing speed were live).
// Read-only counters for every way the replay can skip; one [MACHETE REPLAY] line per 500 ms while held.
void ReplayMachete(ID3D11DeviceContext* context)
{
    auto& rs=g_macheteReplayStatus;
    if(MacheteHeld())++rs.calls;
    if(!MacheteHeld())return;
    MachetePassScope116 pass116(context,false);
    if(!pass116.valid){++rs.targets;return;}
    if(!MacheteReady()){++rs.notReady;return;}
    const uintptr_t nativeTarget116=MacheteDrawTarget(context);
    for(int i=0;i<g_machete.nativeCount116;++i)if(g_machete.nativeTargets116[i]==nativeTarget116&&g_machete.nativeFrames116[i]>=g_frame-1)return;
    if(g_machete.handFrame!=g_frame){++rs.noHandThisFrame;return;}
    const uintptr_t pass=MacheteDrawTarget(context);
    if(pass!=g_machete.handTarget){++rs.otherPass;return;} // never draw with another pass's hand/camera
    if(g_machete.doneFrame!=g_frame){g_machete.doneFrame=g_frame;g_machete.doneCount=0;}
    for(int i=0;i<g_machete.doneCount;++i)if(g_machete.doneTargets[i]==pass){++rs.already;return;}
    ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);
    CameraSummary camera={};UINT bytes=sizeof(camera);
    bool ok=cameraBuffer&&SUCCEEDED(cameraBuffer->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&
        bytes==sizeof(camera)&&camera.mainView&&std::fabs(camera.xScale-g_mainXScale)>g_mainXScale*.01f;
    if(!ok)++rs.camera;const bool cameraOk=ok;
    ID3D11RenderTargetView* targets[8]={},*twins[8]={};ID3D11DepthStencilView* depth=nullptr,*twinDepth=nullptr;
    context->OMGetRenderTargets(8,targets,&depth);const auto& first=g_machete.parts[0];
    for(UINT i=0;i<8;++i)ok=ok&&HullFormat(targets[i])==first.formats[i];
    ok=ok&&depth&&HullFormat(depth)==first.depthFormat&&g_eyeCb[1][0]&&g_eyeCb[1][1];
    if(cameraOk&&!ok)++rs.targets;const bool targetsOk=ok;
    Mat4 modelMoves[3]={};
    for(int i=0;i<g_machete.partCount&&ok;++i)
    {
        const auto& p=g_machete.parts[i];for(UINT s=0;s<8;++s)ok=ok&&p.formats[s]==first.formats[s];
        ok=ok&&p.depthFormat==first.depthFormat;Mat4 inverse={};
        ok=ok&&Invert(Multiply(HullView(camera),HullWorld(p.data.data())),inverse);
        modelMoves[i]=Multiply(inverse,Multiply(g_machete.hand,kMacheteFromHand[i]));
        for(double value:modelMoves[i].m)ok=ok&&std::isfinite(value);
    }
    if(targetsOk&&!ok)++rs.parts;const bool partsOk=ok;
    if(ok)
    {
        for(UINT i=0;i<first.targets;++i)if(targets[i]){twins[i]=static_cast<ID3D11RenderTargetView*>(TwinView(targets[i],ViewKind::RenderTarget,false));ok=ok&&twins[i];}
        twinDepth=static_cast<ID3D11DepthStencilView*>(TwinView(depth,ViewKind::DepthStencil,false));ok=ok&&twinDepth;
    }
    if(partsOk&&!ok)++rs.twins;
    if(ok)
    {
        auto* dc=g_machete.deferred;const bool own=g_own;g_own=true;
        D3D11_VIEWPORT viewports[16]={};UINT n=16;context->RSGetViewports(&n,viewports);dc->RSSetViewports(n,viewports);
        D3D11_RECT rects[16]={};n=16;context->RSGetScissorRects(&n,rects);dc->RSSetScissorRects(n,rects);
        dc->GSSetShader(nullptr,nullptr,0);dc->HSSetShader(nullptr,nullptr,0);dc->DSSetShader(nullptr,nullptr,0);
        for(int i=0;i<g_machete.partCount;++i)
        {
            auto& p=g_machete.parts[i];float bones[576]={};for(int b=0;b<48;++b)StoreBone(bones,b,HandIdentity());
            StoreBone(bones,0,modelMoves[i]);g_oUpdateSubresource(context,g_machete.skin[i],0,nullptr,bones,0,0);
            UINT stride=40,offset=0;dc->IASetVertexBuffers(0,1,&p.vb,&stride,&offset);dc->IASetIndexBuffer(p.ib,DXGI_FORMAT_R16_UINT,p.indexOffset);
            dc->IASetInputLayout(p.layout);dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            dc->VSSetShader(p.vs,nullptr,0);dc->PSSetShader(p.ps,nullptr,0);
            ID3D11Buffer* cb[4]={p.globals,cameraBuffer,p.clip,g_machete.skin[i]};dc->VSSetConstantBuffers(0,4,cb);
            dc->PSSetConstantBuffers(0,8,p.psCb);dc->PSSetShaderResources(0,16,p.textures);dc->PSSetSamplers(0,16,p.samplers);
            dc->RSSetState(p.raster);dc->OMSetDepthStencilState(p.depth,p.stencil);dc->OMSetBlendState(p.blend,p.blendFactor,p.mask);
            for(int eye=0;eye<2;++eye)
            {
                dc->OMSetRenderTargets(p.targets,eye?twins:targets,eye?twinDepth:depth);
                dc->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyeCb[1][eye]);
                if(HasFlag(p.ps,kPsPatchedGuid))dc->PSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyePsCb[eye]);
                dc->DrawIndexed(p.draw.a,p.draw.b,p.draw.d);
            }
        }
        ID3D11CommandList* list=nullptr;if(SUCCEEDED(dc->FinishCommandList(FALSE,&list)))
        {
            context->ExecuteCommandList(list,TRUE);SafeRelease(list);g_machete.replayFrame=g_frame;++rs.drawn;
            if(g_machete.doneCount<8){g_machete.doneTargets[g_machete.doneCount++]=pass;if(g_machete.doneCount>1)++rs.passes;}
            if(!g_machete.logged){Log("[MACHETE] persistent replay in both first-person eyes, measured right-hand grip; source untouched");g_machete.logged=true;}
        }
        else ++rs.commandList;
        g_own=own;
    }
    for(UINT i=0;i<8;++i){SafeRelease(targets[i]);SafeRelease(twins[i]);}
    SafeRelease(depth);SafeRelease(twinDepth);SafeRelease(cameraBuffer);
}
