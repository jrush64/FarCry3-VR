// Native wheel selection remains authoritative. Grip release dispatches the
// native physical projectile, with or without its pin. Manual extraction owns
// grenade arming/audio; Molotov ignition remains native until its own milestone.
#include "throw_frag_grenade_clips.inl"
#include "throw_molotov_clips.inl"
struct ThrowableState144 {
    bool manualPin154=true,arcAssist156=true;float boost156=1.5f,molotovBoost167=1.25f,c4Boost178=1.25f,mineBoost178=1.25f;float itemStrength180[4]={1.5f,1.875f,1.25f,1.25f};
    bool zeroDrop148=false,enabled=true,owned=false,drawing=false,gripped=false,previousB=false,releasing=false,launched=false;
    int kind=0;LONG seen=-100000;UINT epoch=0;ULONGLONG started=0,ignoreUntil=0,releaseAt=0;
    unsigned completedBefore=0;float strength=1;
    fc3manual::History history;fc3throw::Prepared143 prepared;
    Mat4 frozenFrameWorld={},nativeMolotovWorld={};LONG nativeMolotovFrame=-100000;
    ID3D11Buffer* particles=nullptr;UINT particleBytes=0;unsigned particleMoves=0,particleProbes=0;LONG particleProbeFrame=-100000;int releaseProblem=0;
    ID3D11Buffer* world[8]={};UINT worldBytes[8]={};
}g_throw144;
#include "throwable_pouch361.h"
bool g_pouchEnabled361=false,g_pouchSession361=false,g_pouchGuide361=false;float g_pouchPosition361[3]={.30f,-.35f,.05f};fc3pouch361::State g_pouchState361;
bool PouchDraw361(bool,const fc3pad::Controllers&);
fc3pad::Controllers ThrowableControls361(){auto c=fc3pad::Latest();const auto h=fc3hands303::Read();if(g_pouchSession361||(g_pouchEnabled361&&!g_throw144.owned&&h.enabled&&h.empty)){float rg=0,lg=0,rt=0,lt=0;fc3hands303::RawControls(rg,lg,rt,lt);c.rightGrip=rg;c.leftGrip=lg;c.rightTrigger=rt;c.leftTrigger=lt;}return c;}

void CaptureGrenadePin154(ID3D11DeviceContext*,const DrawCall&);
bool GrenadePinFingers154(int,float*);
bool MolotovFingers158(int,float*);
int PlaceableGeometry177(ID3D11DeviceContext*);
bool PlaceableSupported177(int);const float* PlaceableRight177();
double ThrowableBoost167();
void ResetPlaceable177();void PausePlaceable177();
bool PlaceableFingers177(int,float*);
ID3D11Buffer* PlaceablePalette177(ID3D11DeviceContext*,UINT,UINT);
ID3D11Buffer* PlaceableWorld177(ID3D11DeviceContext*,UINT,UINT);
void ResetThrowable144(const char*);
// Item/input ownership ends before the native arm animation does. Keep a
// separate visual lease until an actual firearm draw resumes, with a bounded
// fallback for unarmed areas. Never extend the projectile or native RB hold.
struct ThrowableArmState150 { UINT epoch=0;LONG launchedFrame=-100000;ULONGLONG until=0; }g_throwArm150;
void RefreshThrowableLaunch157(){
    auto& s=g_throw144;ULONGLONG launchedAt=0;
    if(!s.owned||!s.releasing||s.launched||s.epoch!=g_armPoseEpoch||!fc3throw::ReleaseCompleted157(s.prepared,launchedAt))return;
    const auto now=GetTickCount64();s.launched=true;g_throwArm150={s.epoch,g_frame,now+2500};
    Log("[THROW157] exact native launch consumed at draw/frame%ld: release-to-launch %llu ms, launch-to-visual %llu ms; held mesh retired, controller arm cleanup retained",g_frame,launchedAt>=s.releaseAt?launchedAt-s.releaseAt:0,now>=launchedAt?now-launchedAt:0);
}
bool ThrowableArmLease150(){RefreshThrowableLaunch157();return g_throwArm150.until>GetTickCount64()&&g_throwArm150.epoch==g_armPoseEpoch;}
bool ThrowableArmMotion150()
{bool seated=false;int seat=-1;return ThrowableArmLease150()&&g_motionGun&&g_inGameplay&&!fc3aim::NativeActionOwnsArms128()&&!(fc3aim::VehicleState(seated,seat)&&seated)&&!fc3pad::NativeMenuOpen();}
// 181: after Y or an automatic return the native item plays its holster
// animation on the native rig. Keep its mesh hidden and BOTH tracked arms until
// the firearm is actually drawn, so no chest pose or snap is visible, and never
// re-adopt the retiring item while it is only holstering.
struct ThrowableRetire181 {int kind=0;ULONGLONG until=0;LONG seen=-100000;int tailKind=0;ULONGLONG tailUntil=0;}g_retire181;
bool ThrowableRetiring181(){return g_retire181.kind!=0&&GetTickCount64()<g_retire181.until;}
// 183: the item stays visible in the tracked hand while the game holsters it; only the
// last native frames after the firearm appears are hidden (tail), never the switch itself.
bool ThrowableTailHidden183(int kind){return g_retire181.tailKind==kind&&GetTickCount64()<g_retire181.tailUntil;}
void BeginRetire181(int kind,const char* why){if(kind<3)return;g_retire181={kind,GetTickCount64()+2500,g_frame,0,0};Log("[THROW183] %s: %s stays in the tracked hand, both arms tracked, until the firearm is drawn",why,kind==3?"C4":"mine");}
void EndRetire181(const char* why){if(g_retire181.kind)Log("[THROW181] retire ended (%s)",why);const int kind=g_retire181.kind;g_retire181={};if(kind){g_retire181.tailKind=kind;g_retire181.tailUntil=GetTickCount64()+400;}}
bool ThrowableArmSide150(int side){return ThrowableArmMotion150()&&(side==0||ThrowableRetiring181());}
int PlaceableVisualKind183(){return g_throw144.kind>=3?g_throw144.kind:ThrowableRetiring181()?g_retire181.kind:0;}
struct ChargeDraw184 {LONG frame=-100000;}g_chargeDraw184;
bool PlaceableCharge177(ID3D11DeviceContext*);
bool ThrowableHidden144(){RefreshThrowableLaunch157();return g_throw144.launched;}
bool ThrowableOwns144(){return g_throw144.owned;}
bool ThrowableLease144(){RefreshThrowableLaunch157();return g_throw144.owned&&g_throw144.kind!=0&&!g_throw144.launched;}
Mat4 ThrowMat144(const float* rows){Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=rows[i];return m;}
const float* ThrowableRight144(){return PlaceableVisualKind183()>=3?PlaceableRight177():g_throw144.kind==1?kThrowGrenadeClipA[kThrowGrenadeAIdx_hold].rightWrist:kThrowMolotovClipB[kThrowMolotovBIdx_rest].rightWrist;}
ULONGLONG ThrowableArchetype144(){return g_throw144.kind==1?0x03c006e5ac354e19ULL:g_throw144.kind==2?0x000006f6a4abae8dULL:0;}
int ThrowableGeometry144(ID3D11DeviceContext* c){const auto g=NativeWeaponGeometry(c,DrawCall{});return g.vertexBytes==37888&&g.indexBytes==9276&&g.stride==32?1:g.vertexBytes==43360&&g.indexBytes==11358&&g.stride==40?2:PlaceableGeometry177(c);}
void ObserveThrowable144(ID3D11DeviceContext* c,const DrawCall& call){
    auto& s=g_throw144;if(!s.enabled||call.kind!=0)return;const int kind=ThrowableGeometry144(c);if(!kind)return;
    if(kind==3&&PlaceableCharge177(c))g_chargeDraw184.frame=g_frame;
    if(g_retire181.kind==kind&&GetTickCount64()<g_retire181.until){g_retire181.seen=g_frame;return;}
    if(ThrowableTailHidden183(kind))return;
    if(GetTickCount64()<s.ignoreUntil&&!s.drawing)return;
    CameraSummary camera={};if(!PistolGuideCamera(c,camera))return;
    if(s.kind&&s.kind!=kind){s.history.Clear();s.prepared={};s.gripped=false;ResetPlaceable177();}
    s.kind=kind;s.seen=g_frame;if(kind>=3)fc3pad::SetPlaceableInput180(true);
    if(kind==1)CaptureGrenadePin154(c,call);
    if(kind==2){ID3D11Buffer* cb=nullptr;float rows[88]={};c->VSGetConstantBuffers(0,1,&cb);
        if(ReadCopy(cb,rows,sizeof(rows))>=60){s.nativeMolotovWorld=HullWorld(rows);s.nativeMolotovFrame=g_frame;}SafeRelease(cb);}

}
bool ThrowableRightHand144(Mat4& cameraHand){
    if((!ThrowableLease144()&&!ThrowableArmLease150())||!g_handRightValid)return false;SeedControllerHand124();
    if(g_freeRightEpoch!=g_armPoseEpoch)return false;
    Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
    cameraHand=Multiply(ArmBodyToCamera(),Multiply(controller,g_freeRightFromController));return true;
}
bool ThrowableFrame144(const CameraSummary& camera,Mat4& frameCamera){
    auto& s=g_throw144;if(!ThrowableLease144())return false;
    if(s.releasing){frameCamera=Multiply(HullView(camera),s.frozenFrameWorld);return true;}
    Mat4 hand={},inverse={};if(!ThrowableRightHand144(hand)||!Invert(ThrowMat144(ThrowableRight144()),inverse))return false;
    frameCamera=Multiply(hand,inverse);return true;
}
bool ThrowableFingers144(int side,float* bones){
    if(PlaceableVisualKind183()>=3)return PlaceableFingers177(side,bones);
    if(side)return MolotovFingers158(side,bones)||GrenadePinFingers154(side,bones);
    if(side||!ThrowableLease144())return false;
    const float* grip=g_throw144.kind==1?kThrowGrenadeClipA[kThrowGrenadeAIdx_hold].rightFingers:kThrowMolotovClipB[kThrowMolotovBIdx_rest].rightFingers;
    const Mat4 wrist=BoneMatrix(bones,2);for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(grip,b-3)));return true;
}
bool ThrowableVisual144(ID3D11DeviceContext* c){const int g=ThrowableGeometry144(c);if(!g)return false;
    return (g_throw144.owned&&g==g_throw144.kind)||(ThrowableRetiring181()&&g==g_retire181.kind)||ThrowableTailHidden183(g);}
bool ThrowableKeepVertexShadow145(const D3D11_BUFFER_DESC& d){
    return ThrowableLease144()&&g_throw144.kind==2&&d.Usage==D3D11_USAGE_DYNAMIC&&
        (d.BindFlags&D3D11_BIND_VERTEX_BUFFER)&&d.ByteWidth>=12&&d.ByteWidth<=192*1024;
}
// Native Molotov FX_FIRE node: parent model root, not a cloth palette slot.
V3 MolotovFire145(const Mat4& root){return Apply(root,{0,-.024668170139193535,.24620996415615082});}
// Legacy176 helper name: this is Anchor (glass draw2799 slot0 / cloth draw0 slot1).
// Actual cloth also uses GENERICBONE (draw0 slot0);177 samples both weights.
Mat4 MolotovClothFrame176(const Mat4& frame){return Multiply(frame,ThrowMat144(kThrowMolotovClipB[kThrowMolotovBIdx_rest].part[2]));}
#include "molotov_rag177.inl"
bool MolotovFlameWorld176(const CameraSummary& camera,Mat4& flame){
 Mat4 frame={},inverse={};if(!ThrowableFrame144(camera,frame)||!Invert(HullView(camera),inverse))return false;
 flame=MolotovEmitterBasis178(Multiply(inverse,Multiply(frame,ThrowMat144(kThrowMolotovClipB[kThrowMolotovBIdx_rest].part[1]))));const V3 wick=Apply(inverse,MolotovRagPoint177(frame,true));
 flame.m[3]=wick.x;flame.m[7]=wick.y;flame.m[11]=wick.z;return true;
}
bool MolotovEmitter145(const Mat4& root,const float* globals,UINT bytes){
    if(bytes<364||!RepairRigidMatrix(root))return false;
    const V3 tip=MolotovFire145(root);
    const double dx=globals[88]-tip.x,dy=globals[89]-tip.y,dz=globals[90]-tip.z;
    return std::isfinite(dx)&&std::isfinite(dy)&&std::isfinite(dz)&&dx*dx+dy*dy+dz*dz<=.0225;
}
struct MolotovEffectCensus146 {ULONGLONG keys[64]={};UINT count=0,packets=0;}g_molotovCensus146;
void ProbeMolotovEffect146(ID3D11DeviceContext* c,const DrawCall& call,int variant){
    if(!ThrowableLease144()||g_throw144.kind!=2||g_molotovCensus146.packets>=64)return;
    ID3D11BlendState* blend=nullptr;float factor[4];UINT mask=0;c->OMGetBlendState(&blend,factor,&mask);D3D11_BLEND_DESC bd={};if(blend)blend->GetDesc(&bd);SafeRelease(blend);if(!bd.RenderTarget[0].BlendEnable)return;
    ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;c->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC desc={};if(vb)vb->GetDesc(&desc);
    if(!vb||!ThrowableKeepVertexShadow145(desc)||stride<12||stride>64){SafeRelease(vb);return;}
    ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;c->VSGetShader(&vs,nullptr,nullptr);c->PSGetShader(&ps,nullptr,nullptr);const auto hash=ShaderHash(vs),pixel=ShaderHash(ps);
    const auto key=hash^pixel^(static_cast<ULONGLONG>(stride)<<32)^static_cast<UINT>(variant);auto& census=g_molotovCensus146;
    for(UINT i=0;i<census.count;++i)if(census.keys[i]==key){SafeRelease(vb);SafeRelease(vs);SafeRelease(ps);return;}
    if(census.count>=64){SafeRelease(vb);SafeRelease(vs);SafeRelease(ps);return;}
    std::vector<unsigned char> vertices(desc.ByteWidth);std::vector<UINT> references;DXGI_FORMAT format;UINT indexOffset=0;
    if(ReadCopy(vb,vertices.data(),desc.ByteWidth)!=desc.ByteWidth||!RepairReferencedVertices(c,call,desc.ByteWidth,stride,offset,references,format,indexOffset)){SafeRelease(vb);SafeRelease(vs);SafeRelease(ps);return;}
    census.keys[census.count++]=key;++census.packets;
    const V3 fire=MolotovFire145(g_throw144.nativeMolotovWorld);double nearest=1e18;
    AnimationRecord packet;AnimationStamp(packet);packet.kind=23;packet.shader=hash;packet.vertexBytes=desc.ByteWidth;packet.stride=stride;packet.offset=offset;packet.start=call.b;packet.count=call.a;packet.drawKind=call.kind;packet.side=variant;
    for(int i=0;i<12;++i)packet.finalPose[i]=float(g_throw144.nativeMolotovWorld.m[i]);
    for(UINT k=0;k<references.size();++k){const auto at=static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(references[k])*stride;if(at+12>vertices.size())continue;float p[3];memcpy(p,vertices.data()+at,12);if(k<128){memcpy(packet.finalPose+16+k*4,p,12);packet.finalPose[16+k*4+3]=float(references[k]);}const double dx=p[0]-fire.x,dy=p[1]-fire.y,dz=p[2]-fire.z;const double d=dx*dx+dy*dy+dz*dz;if(std::isfinite(d)&&d<nearest)nearest=d;}
    ID3D11Buffer* globals=nullptr;c->VSGetConstantBuffers(0,1,&globals);packet.rawBytes=ReadCopy(globals,packet.raw,sizeof(packet.raw));SafeRelease(globals);packet.stateWords[0]=static_cast<UINT>(references.size());
    Log("[MOLOTOV146 CENSUS] vs %016llX ps %016llX variant %d stride %u vb %u indices %zu closest native FX_FIRE %.3f cb0 %u; read-only candidate",hash,pixel,variant,stride,desc.ByteWidth,references.size(),std::sqrt(nearest),packet.rawBytes);
    if(g_animation.enabled)AnimationStore(packet);
    for(int side=0;side<2;++side){ID3D11DeviceChild* shader=side?static_cast<ID3D11DeviceChild*>(ps):static_cast<ID3D11DeviceChild*>(vs);if(!shader)continue;UINT size=0;shader->GetPrivateData(kEjectionBytecodeGuid,&size,nullptr);if(size&&size<=65536){std::vector<unsigned char> bytes(size);if(SUCCEEDED(shader->GetPrivateData(kEjectionBytecodeGuid,&size,bytes.data()))){wchar_t name[96];swprintf_s(name,L"fc3_molotov146_%s_%016llX.dxbc",side?L"ps":L"vs",side?pixel:hash);WriteBlob(name,bytes);}}}
    SafeRelease(vb);SafeRelease(vs);SafeRelease(ps);
}
bool BeginMolotovParticles145(ID3D11DeviceContext* c,const DrawCall& call,int variant,ID3D11Buffer*& original,UINT& stride,UINT& offset){
    ProbeMolotovEffect146(c,call,variant);
    // Native160 changes only the bound Molotov's own idle effect transform.
    // Do not identify or move scene fires by shader/nearby particle vertices.
    (void)original;(void)stride;(void)offset;return false;
}

ID3D11Buffer* ThrowablePalette144(ID3D11DeviceContext* c,UINT start,UINT count){
 ClearMirrorPalette338();
    if(PlaceableVisualKind183()>=3)return PlaceablePalette177(c,start,count);
    if(g_throw144.kind!=2||!g_weaponPaletteCb)return nullptr;
    const bool body=start==0&&count==2799,cloth=start==2799&&count==2880;if(!body&&!cloth)return nullptr;
    float bones[576]={};Mat4 to={},from={},frame={};CameraSummary camera={};
    if(!DrawSpace(c,bones,to,from)||!PistolGuideCamera(c,camera)||!ThrowableFrame144(camera,frame))return nullptr;
    const auto& key=kThrowMolotovClipB[kThrowMolotovBIdx_rest];
    StoreBone(bones,0,Multiply(from,body?Multiply(frame,ThrowMat144(key.part[1])):MolotovClothFrame176(frame)));
    if(body)StoreBone(bones,1,Multiply(from,Multiply(frame,ThrowMat144(key.part[2]))));
    const bool own=g_own;g_own=true;g_oUpdateSubresource(c,g_weaponPaletteCb,0,nullptr,bones,0,0);g_weaponPaletteCb->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);g_own=own;return g_weaponPaletteCb;
}
Mat4 GrenadePart146(int part){
    const auto& held=kThrowGrenadeClipA[kThrowGrenadeAIdx_hold];
    if(!part)return ThrowMat144(held.part[0]);
    const auto& attached=kThrowGrenadeClipA[kThrowGrenadeAIdx_appear];Mat4 inverse={};
    if(!Invert(ThrowMat144(attached.part[0]),inverse))return HandIdentity();
    return Multiply(ThrowMat144(held.part[0]),Multiply(inverse,ThrowMat144(attached.part[part])));
}
#include "molotov_lighter158.inl"
#include "grenade_pin154.inl"
#include "placeables177.inl"
ID3D11Buffer* ThrowableWorld144(ID3D11DeviceContext* c,UINT start,UINT count){
    if(PlaceableVisualKind183()>=3)return PlaceableWorld177(c,start,count);
    if(g_throw144.kind!=1)return nullptr;int part=start==0&&count==2334?0:start==2334&&count==504?1:start==2838&&count==324?2:start==3162&&count==1476?3:-1;if(part<0)return nullptr;
    ID3D11Buffer* cb=nullptr;c->VSGetConstantBuffers(0,1,&cb);float data[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,data,sizeof(data));D3D11_BUFFER_DESC desc={};if(cb)cb->GetDesc(&desc);SafeRelease(cb);
    if(bytes<80||bytes!=desc.ByteWidth)return nullptr;
    CameraSummary camera={};Mat4 frame={},inverse={};if(!PistolGuideCamera(c,camera)||!ThrowableFrame144(camera,frame)||!Invert(HullView(camera),inverse))return nullptr;
    WritePistolWorld(data,Multiply(inverse,Multiply(frame,GrenadePart154(part))));
    auto& s=g_throw144;int slot=-1;for(int i=0;i<8;++i)if(s.worldBytes[i]==bytes){slot=i;break;}
    if(slot<0)for(int i=0;i<8;++i)if(!s.world[i]){desc.Usage=D3D11_USAGE_DEFAULT;desc.CPUAccessFlags=0;desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;desc.MiscFlags=0;desc.StructureByteStride=0;if(SUCCEEDED(g_oCreateBuffer(g_dev,&desc,nullptr,&s.world[i]))){slot=i;s.worldBytes[i]=bytes;}break;}
    if(slot<0)return nullptr;const bool own=g_own;g_own=true;g_oUpdateSubresource(c,s.world[slot],0,nullptr,data,0,0);s.world[slot]->SetPrivateData(kCbCopyGuid,bytes,data);g_own=own;return s.world[slot];
}
void ResetThrowable144(const char* reason){
    g_pouchSession361=false;fc3pad::SetPhysicalThrowable361(false);
    ResetPlaceable177();
    ResetMolotov158();
    ResetGrenadePin154();
    auto& s=g_throw144;if(s.owned)Log("[THROW155] %s; unarmed frag uses native cancel; active fuse is never reset",reason);
    s.owned=s.drawing=s.gripped=s.releasing=s.launched=false;s.kind=0;s.seen=-100000;s.history.Clear();s.prepared={};s.releaseProblem=0;s.nativeMolotovFrame=-100000;s.ignoreUntil=GetTickCount64()+400;
    fc3pad::SetThrowableInput144(false,false);
    fc3throw::CancelRelease143();
}
// 182: sliders show 0.20..3.00 but the physics is unchanged: 3.00 is exactly the old 8x.
constexpr double kStrengthScale182=8.0/3.0;
double ThrowableBoost167(){const int i=g_throw144.kind-1;return i>=0&&i<4?double(g_throw144.itemStrength180[i])*kStrengthScale182:1.0;}
const wchar_t* kStrengthKeys180[]={L"GrenadeStrength",L"MolotovStrength",L"C4Strength",L"MineStrength"};
const wchar_t* kTossKeys182[]={L"GrenadeToss",L"MolotovToss",L"C4Toss",L"MineToss"};
void LoadItemStrength180(const wchar_t* path){
 auto& s=g_throw144;
 // Flatten old grenade/Molotov settings once; equipped explosives start with a restrained toss.
 const float defaults[]={float((std::min)(8.0,double(s.strength)*s.boost156)),float((std::min)(8.0,double(s.strength)*(std::min)(8.0,double(s.boost156)*s.molotovBoost167))),1.25f,1.25f};
 for(int i=0;i<4;++i){
  wchar_t text[40]={};GetPrivateProfileStringW(L"Throwables",kTossKeys182[i],L"",text,40,path);wchar_t* end=nullptr;double v=wcstod(text,&end);
  if(text[0]&&end&&!*end&&std::isfinite(v)&&v>=.2&&v<=3){s.itemStrength180[i]=float(v);continue;}
  // Migrate the 180 value once: same throw, tighter number.
  wchar_t old[40]={};GetPrivateProfileStringW(L"Throwables",kStrengthKeys180[i],L"",old,40,path);end=nullptr;v=wcstod(old,&end);
  const double value180=old[0]&&end&&!*end&&std::isfinite(v)&&v>=.5&&v<=8?v:defaults[i];
  s.itemStrength180[i]=float((std::max)(.2,(std::min)(3.0,value180/kStrengthScale182)));
 }
}

void LoadThrowables144(const wchar_t* path){
    g_pouchEnabled361=GetPrivateProfileIntW(L"Throwables",L"Pouch361",0,path)!=0;
    for(int axis=0;axis<3;++axis){wchar_t key[32],text[48];swprintf_s(key,L"Pouch361%c",L"XYZ"[axis]);GetPrivateProfileStringW(L"Throwables",key,L"",text,48,path);wchar_t* end=nullptr;const double value=wcstod(text,&end);if(end!=text&&!*end&&std::isfinite(value)&&value>=-1&&value<=1)g_pouchPosition361[axis]=float(value);}

    const wchar_t* keys178[]={L"C4Boost",L"MineBoost"};float* values178[]={&g_throw144.c4Boost178,&g_throw144.mineBoost178};
    for(int i=0;i<2;++i){wchar_t text[40]={};GetPrivateProfileStringW(L"Throwables",keys178[i],L"1.25",text,40,path);wchar_t* end=nullptr;const double value=wcstod(text,&end);*values178[i]=end&&!*end&&std::isfinite(value)&&value>=.5&&value<=4?float(value):1.25f;}
    wchar_t text167[40]={};GetPrivateProfileStringW(L"Throwables",L"MolotovBoost",L"1.25",text167,40,path);wchar_t* end167=nullptr;double value167=wcstod(text167,&end167);g_throw144.molotovBoost167=end167&&!*end167&&std::isfinite(value167)&&value167>=.5&&value167<=4?float(value167):1.25f;
    LoadLighterSettings163(path);
    g_molotov158.enabled=GetPrivateProfileIntW(L"Throwables",L"ManualMolotovLighter",1,path)!=0;
    g_throw144.arcAssist156=GetPrivateProfileIntW(L"Throwables",L"ArcReleaseAssist",1,path)!=0;wchar_t boost[40]={};GetPrivateProfileStringW(L"Throwables",L"ThrowBoost",L"1.5",boost,40,path);wchar_t* boostEnd=nullptr;const float boostValue=float(wcstod(boost,&boostEnd));if(boostEnd&&!*boostEnd&&std::isfinite(boostValue)&&boostValue>=.5f&&boostValue<=4)g_throw144.boost156=boostValue;
g_throw144.manualPin154=GetPrivateProfileIntW(L"Throwables",L"ManualGrenadePin",1,path)!=0;g_grenadePin154.guides=GetPrivateProfileIntW(L"Throwables",L"PinGuides",1,path)!=0;g_throw144.zeroDrop148=GetPrivateProfileIntW(L"Throwables",L"GrenadeDropProof",0,path)!=0;fc3throw::SetUnlimited147(GetPrivateProfileIntW(L"Throwables",L"UnlimitedStock",1,path)!=0);auto& s=g_throw144;s.enabled=GetPrivateProfileIntW(L"Throwables",L"PhysicalRelease",1,path)!=0;wchar_t value[48]={};GetPrivateProfileStringW(L"Throwables",L"Strength",L"1",value,48,path);wchar_t* end=nullptr;const float v=float(wcstod(value,&end));if(end&&!*end&&std::isfinite(v)&&v>=.5f&&v<=2.5f)s.strength=v;LoadItemStrength180(path);Log("[THROW151] release mode: %s; unlimited stock %s",s.zeroDrop148?"DIAGNOSTIC ZERO-SPEED DROP":"PHYSICAL HAND VELOCITY",fc3throw::Unlimited147()?"ON":"OFF");}
bool SaveThrowables144(const wchar_t* path){
 for(int i=0;i<4;++i){wchar_t text[40];swprintf_s(text,L"%.3f",g_throw144.itemStrength180[i]);if(!WritePrivateProfileStringW(L"Throwables",kTossKeys182[i],text,path))return false;
  swprintf_s(text,L"%.3f",(std::min)(8.0,double(g_throw144.itemStrength180[i])*kStrengthScale182));if(!WritePrivateProfileStringW(L"Throwables",kStrengthKeys180[i],text,path))return false;}
 const wchar_t* keys178[]={L"C4Boost",L"MineBoost"};const float values178[]={g_throw144.c4Boost178,g_throw144.mineBoost178};
 for(int i=0;i<2;++i){wchar_t text[40];swprintf_s(text,L"%.3f",values178[i]);if(!WritePrivateProfileStringW(L"Throwables",keys178[i],text,path))return false;}
if(!WritePrivateProfileStringW(L"Throwables",L"Pouch361",g_pouchEnabled361?L"1":L"0",path))return false;
for(int axis=0;axis<3;++axis){wchar_t key[32],text[48];swprintf_s(key,L"Pouch361%c",L"XYZ"[axis]);swprintf_s(text,L"%.4f",g_pouchPosition361[axis]);if(!WritePrivateProfileStringW(L"Throwables",key,text,path))return false;}
wchar_t text167[40];swprintf_s(text167,L"%.3f",g_throw144.molotovBoost167);if(!WritePrivateProfileStringW(L"Throwables",L"MolotovBoost",text167,path))return false;if(!WritePrivateProfileStringW(L"Throwables",L"ManualMolotovLighter",g_molotov158.enabled?L"1":L"0",path))return false;wchar_t boost[40];swprintf_s(boost,L"%.3f",g_throw144.boost156);if(!WritePrivateProfileStringW(L"Throwables",L"ThrowBoost",boost,path)||!WritePrivateProfileStringW(L"Throwables",L"ArcReleaseAssist",g_throw144.arcAssist156?L"1":L"0",path))return false;wchar_t value[48]={};swprintf_s(value,L"%.4f",g_throw144.strength);return WritePrivateProfileStringW(L"Throwables",L"ManualGrenadePin",g_throw144.manualPin154?L"1":L"0",path)&&WritePrivateProfileStringW(L"Throwables",L"PinGuides",g_grenadePin154.guides?L"1":L"0",path)&&WritePrivateProfileStringW(L"Throwables",L"GrenadeDropProof",g_throw144.zeroDrop148?L"1":L"0",path)&&WritePrivateProfileStringW(L"Throwables",L"UnlimitedStock",fc3throw::Unlimited147()?L"1":L"0",path)&&WritePrivateProfileStringW(L"Throwables",L"PhysicalRelease",g_throw144.enabled?L"1":L"0",path)&&WritePrivateProfileStringW(L"Throwables",L"Strength",value,path);}
void ThrowableMenu144(){
    if(!ImGui::CollapsingHeader("Throwables"))return;
    LighterMenu163();
    ImGui::BeginDisabled(g_throw144.owned);bool pouchChange361=ImGui::Checkbox("Physical throwable pouch (prototype)",&g_pouchEnabled361);pouchChange361=ImGui::SliderFloat3("Throwable pouch position",g_pouchPosition361,-1.f,1.f,"%.2f m")||pouchChange361;ImGui::EndDisabled();
    ImGui::BeginDisabled(g_throw144.owned);bool change=ImGui::Checkbox("Physical grenade / Molotov release",&g_throw144.enabled)||pouchChange361;ImGui::EndDisabled();
    ImGui::BeginDisabled(g_throw144.owned);change=ImGui::Checkbox("Diagnostic only: force grenade drop (disables throwing)",&g_throw144.zeroDrop148)||change;ImGui::EndDisabled();
    ImGui::BeginDisabled(g_throw144.owned);change=ImGui::Checkbox("Manual grenade pin (LEFT GRIP)",&g_throw144.manualPin154)||change;ImGui::EndDisabled();
    ImGui::BeginDisabled(g_throw144.owned);change=ImGui::Checkbox("Manual Molotov lighting (LEFT GRIP + LT)",&g_molotov158.enabled)||change;ImGui::EndDisabled();
    change=ImGui::Checkbox("Grenade pin contact / pull guide",&g_grenadePin154.guides)||change;
    bool unlimited=fc3throw::Unlimited147();if(ImGui::Checkbox("Unlimited throwables (all four)",&unlimited)){fc3throw::SetUnlimited147(unlimited);change=true;}
    ImGui::TextWrapped("Keeps your existing stock when used. Obtain at least one of the selected item first.");
    const char* labels180[]={"Grenade throw strength","Molotov throw strength","C4 toss strength","Mine toss strength"};
    for(int i=0;i<4;++i)change=ImGui::SliderFloat(labels180[i],&g_throw144.itemStrength180[i],.2f,3.f,"%.2f")||change;
    change=ImGui::Checkbox("Overhand release timing assistance",&g_throw144.arcAssist156)||change;
    ImGui::TextWrapped("Each strength affects only that item's hand-motion speed; 3.00 is the old 8x, the physics is unchanged. Higher values throw farther, up to the speed limit. A still hand drops. C4/mines use a shorter toss with a 12 m/s maximum and stick to the first surface they hit.");
    if(change){wchar_t path[MAX_PATH];ManualSettingsPath(path);SaveThrowables144(path);}
    ImGui::TextWrapped("Pouch prototype: stow the firearm first, reach the right hand into the pouch sphere and squeeze grip. The native wheel chooses grenade or Molotov. Left hand handles the pin or lighter; release right grip to throw. Grip+A/B draw stays disabled. C4/mines keep their existing controls.");
}
void TickThrowable144(bool headset){
    auto& s=g_throw144;const auto c=ThrowableControls361();const auto now=GetTickCount64();s.previousB=c.b;
    const bool pouchDraw361=PouchDraw361(headset,c);
    if(s.owned&&s.kind<3&&!g_pouchSession361){ResetThrowable144("grenades/Molotovs paused unless explicitly drawn from361 pouch");return;}
    fc3pad::SetPlaceableInput180(s.kind>=3&&(s.owned||g_frame-s.seen<=2));
    {char note[320];for(int i=0;i<8&&fc3throw::PopNativeNote181(note,sizeof note);++i)Log("%s",note);}
    if(!headset||!g_inGameplay||!c.active||fc3pad::GameplayInputBlocked()||fc3pad::WeaponSelectionRequested())g_placeReturn179={};else TickPlaceableReturn179(now);
    if(g_retire181.kind&&now>=g_retire181.until)EndRetire181("lease expired");
    // While retiring, the hidden item's own holster draws must not count as the firearm.
    const LONG armFrame181=g_retire181.kind?g_retire181.seen:g_throwArm150.launchedFrame;
    if(g_throwArm150.until&&(now>=g_throwArm150.until||g_throwArm150.epoch!=g_armPoseEpoch||
       (g_weaponMechanicFrame>armFrame181&&g_frame-g_weaponMechanicFrame<=2))){
        Log("[THROW150] right-arm cleanup ended; firearm observed or visual lease expired");g_throwArm150={};
        if(g_retire181.kind)EndRetire181("firearm drawn");
    }
    bool seated=false;int seat=-1;const bool vehicle=fc3aim::VehicleState(seated,seat)&&seated;
    fc3throw::SetMolotovIntent158(g_pouchSession361&&g_molotov158.enabled); // only explicit pouch owner
    fc3throw::SetPinIntent155(g_pouchSession361&&s.manualPin154); // firearm grabs grant no pin intent
    const bool placeableSelection179=s.owned&&s.kind>=3&&fc3pad::WeaponSelectionRequested();
    if(!s.enabled||!g_motionGun||!g_inGameplay||vehicle||fc3aim::NativeActionOwnsArms128()||MacheteHeld()||((s.owned&&s.kind<3)?fc3pad::WeaponWheelRequested145():fc3pad::WeaponSelectionRequested())){
        if(placeableSelection179&&!vehicle&&!fc3aim::NativeActionOwnsArms128()){g_throwArm150={s.epoch,g_frame,now+2500};BeginRetire181(s.kind,"Y previous weapon");}
        else if(fc3pad::WeaponWheelRequested145()&&ThrowableArmLease150())g_throwArm150.until=now+2500;
        else if(!fc3pad::WeaponSelectionRequested())g_throwArm150={};
        if(s.owned)ResetThrowable144("context/selection changed");return;
    }
    if(!headset||g_pistolMenu.open||fc3pad::GameplayInputBlocked()||!c.active||!g_handRightValid||!g_eyeViewsSet){
        s.history.Clear();s.gripped=false;PauseGrenadePin154();PauseMolotov158();PausePlaceable177();
        fc3throw::PublishContext143(0,0,false);
        // Preserve native hold while paused; never turn loss of tracking into
        // an invented physical release or rewind a running native fuse.
        if(s.owned){
            if(!headset&&g_frame-s.seen>5&&now-s.started>500)ResetThrowable144("native item gone after XR loss");
            else fc3pad::SetThrowableInput144(true,s.kind<3&&!s.releasing);
        }return;
    }
    if(!s.owned&&pouchDraw361){ // a stale prior C4/mine visual is not the new wheel-selected throwable
        g_pouchSession361=true;g_pouchGuide361=false;fc3pad::SetPhysicalThrowable361(true);
        s.owned=s.drawing=true;s.started=now;s.kind=0;s.seen=-100000;s.history.Clear();s.gripped=true;s.epoch=g_armPoseEpoch;s.ignoreUntil=0;
        fc3throw::SetPinIntent155(s.manualPin154);
        fc3pad::SetThrowableInput144(true,true);Log("[THROW144] draw current native wheel-selected throwable (empty-hand physical pouch); no inventory selection changed");
    }
    if(!s.owned&&s.kind>=3&&g_frame-s.seen<=2&&now>=s.ignoreUntil){s.owned=true;s.started=now;s.epoch=g_armPoseEpoch;s.gripped=true;Log("[THROW144] take over native selected %s",s.kind==1?"grenade":s.kind==2?"Molotov":s.kind==3?"C4":"mine");}
    if(!s.owned)return;
    g_twoHand=false;fc3pad::SetTwoHandWeaponHold(false);fc3xr::HideGrabMarkers();
    if(s.epoch!=g_armPoseEpoch){ResetThrowable144("recenter/calibration changed");return;}
    if(s.drawing&&s.kind){s.drawing=false;ResetGrenadePin154();ResetPlaceable177();Log("[THROW177] selected item visual acquired (kind%d); item-specific controls now own the hands",s.kind);}
    if(s.drawing){if(now-s.started>2500)ResetThrowable144("no supported native item appeared (empty stock or unsupported item)");return;}
    // Native inventory sampling follows dispatch. A launch receipt can precede
    // the final stock sample; keep checking during the bounded visual retirement
    // interval, before its launched early-return and 650ms ownership timeout.
    if(s.kind>=3&&g_place177.placed&&RecheckPlaceableStock179(fc3throw::PlaceableNativeStatus177(PlaceableKind177(s.kind))))return;
    if(s.kind>=3&&s.owned){
        AdvancePlaceable180(fc3throw::PlaceableNativeStatus177(PlaceableKind177(s.kind)),now);
        if(!s.owned)return;
        if(g_weaponMechanicFrame>s.seen&&g_frame-g_weaponMechanicFrame<=2){ResetThrowable144("native firearm restored");return;}
    }
    if(s.kind<3&&((g_frame-s.seen>5&&now-s.started>500)||(s.launched&&now-s.releaseAt>650))){ResetThrowable144("native item finished/weapon restored");return;}
    if(s.launched&&s.kind<3){fc3pad::SetThrowableInput144(true,false);return;}
    if(TickPlaceable177())return;
    ULONGLONG owner=0;
    if(!fc3throw::CurrentLocalOwner144(owner)){s.history.Clear();
        if(s.gripped&&c.rightGrip<.3f){s.gripped=false;s.releaseProblem=1;Log("[THROW145] release blocked: fresh player identity unavailable; will retry while grip is open");}return;}
    fc3throw::PublishContext143(owner,s.epoch,true);
    fc3throw::Prepared143 current;
    if(fc3throw::CurrentPrepared143(current)&&current.archetype==ThrowableArchetype144())s.prepared=current;
    else fc3throw::AdoptPrepared144(ThrowableArchetype144(),s.prepared,s.prepared.projectile);
    if(s.kind==2&&!s.releasing&&!s.launched&&g_frameCameraValid){
        Mat4 cloth={};if(MolotovFlameWorld176(g_frameCamera,cloth)){
            float native[16]={};for(int r=0;r<4;++r)for(int col=0;col<4;++col)native[r*4+col]=float(cloth.m[col*4+r]);
            fc3throw::PublishMolotovFlame160(s.prepared,native);
            static ULONGLONG next160=0;if(now>=next160){unsigned updates=0,spawns=0;fc3throw::MolotovFlameStatus160(updates,spawns);Log("[MOLOTOV160] native held-flame uploads %u spawn overrides %u; exact projectile %016llX",updates,spawns,s.prepared.projectile);next160=now+1500;}
        }
    }
    TickGrenadePin154();TickMolotov158();
    if(s.releasing){RefreshThrowableLaunch157();fc3pad::SetThrowableInput144(true,false);return;}
    Mat4 frame={},inverse={};if(!g_frameCameraValid||!ThrowableFrame144(g_frameCamera,frame)||!Invert(HullView(g_frameCamera),inverse))return;
    const Mat4 frameWorld=Multiply(inverse,frame);Mat4 body={};
    if(s.kind==1)body=Multiply(frameWorld,ThrowMat144(kThrowGrenadeClipA[kThrowGrenadeAIdx_hold].part[0]));
    else body=Multiply(frameWorld,ThrowMat144(kThrowMolotovClipB[kThrowMolotovBIdx_rest].part[0]));
    const V3 p=Apply(body,{0,0,0}),origin={g_frameCamera.position[0],g_frameCamera.position[1],g_frameCamera.position[2]};
    s.history.Add({now,owner,s.epoch,{p.x,p.y,p.z},{p.x-origin.x,p.y-origin.y,p.z-origin.z},true});
    if(c.rightGrip>.7f){s.gripped=true;s.releaseProblem=0;}
    if(c.rightGrip<.3f){fc3manual::Release release;
        const bool safeRelease156=(ManualMolotov158()&&g_molotov158.manualIgnition159)?(fc3throw::MolotovLit158(s.prepared)||fc3throw::LooseRelease156(s.prepared)):(!GrenadePinRequired154()||g_grenadePin154.out||fc3throw::LooseRelease156(s.prepared));
        const bool dropProof=s.kind==1&&s.zeroDrop148;
        if(dropProof)release={now,owner,s.epoch,{p.x,p.y,p.z},{0,0,0}};
        fc3manual::ReleaseMotion152 motion;bool historyReady=dropProof||s.history.AtRelease(now,release,1,&motion,ThrowableBoost167(),s.arcAssist156);
        if(!historyReady&&std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)){release={now,owner,s.epoch,{p.x,p.y,p.z},{0,0,0}};historyReady=true;Log("[THROW156] release without enough motion samples: zero-speed physical drop");}
        if(historyReady){fc3manual::Sample samples[16];const unsigned count=s.history.Copy(samples,16);for(unsigned i=0;i<count;++i)Log("[THROW156 SAMPLE] ms%llu relative %.4f %.4f %.4f",samples[i].ms,samples[i].relative.x,samples[i].relative.y,samples[i].relative.z);}
        if(historyReady&&!dropProof)Log("[THROW152] release fit %.3f -> %.3f m/s; recent stroke age %llu ms; timing assist %d",motion.sampledSpeed,motion.selectedSpeed,motion.strokeAge,motion.assisted?1:0);
        if(historyReady&&s.kind==2)Log("[MOLOTOV180] strength %.3f; final speed %.3f m/s",ThrowableBoost167(),fc3manual::Length(release.velocity));
        if(historyReady){
            bool queued=false;
            s.completedBefore=fc3throw::ManualStatus143().completed;
            queued=safeRelease156&&s.prepared.serial&&fc3throw::QueueRelease143(s.prepared,release);
            if(queued&&dropProof)Log("[DROP148] ZERO-VELOCITY grenade release requested; no aim vector and no velocity fit; waiting for native launch acknowledgement");
            if(queued){g_grenadePin154.held=false;s.releasing=true;s.releaseAt=now;s.frozenFrameWorld=frameWorld;fc3pad::SetThrowableInput144(true,false);Log("[THROW151] %s release: measured speed %.3f m/s",dropProof?"DIAGNOSTIC DROP":"PHYSICAL",fc3manual::Length(release.velocity));Log("[THROW144] grip release %.3f/%.3f/%.3f velocity %.3f/%.3f/%.3f; native launch ticket %u",release.origin.x,release.origin.y,release.origin.z,release.velocity.x,release.velocity.y,release.velocity.z,s.prepared.serial);}
        }
        if(!s.releasing){s.gripped=false;s.releaseProblem=!historyReady?2:!s.prepared.serial?3:4;
            Log("[THROW145] release BLOCKED reason %d (1=identity 2=motion history 3=projectile ownership 4=ticket); will retry while grip is open",s.releaseProblem);}
    }else fc3pad::SetThrowableInput144(true,true);
}
void ThrowableNote144(){
    const auto& s=g_throw144;if(!s.owned)return;
    if(PlaceableNote177())return;
    if(MolotovNote158()||GrenadePinNote154())return;
    if(s.releaseProblem)fc3xr::SetTestNote(L"RELEASE BLOCKED\nNative launch is not confirmed. Release will retry while RIGHT GRIP is open.");
    else if(s.drawing)fc3xr::SetTestNote(L"DRAWING SELECTED THROWABLE\nKeep RIGHT GRIP held.");
    else if(s.kind==1&&s.zeroDrop148)fc3xr::SetTestNote(s.launched?L"NATIVE DROP DISPATCHED":s.releasing?L"DROP REQUESTED\nWaiting for native launch.":L"GRENADE DROP TEST\nKeep your hand still. RELEASE RIGHT GRIP.\nGrenade should fall immediately. Repeat to check stock.");
    else fc3xr::SetTestNote(s.releasing?L"RELEASE REQUESTED":s.prepared.serial?L"PHYSICAL THROW\nHold RIGHT GRIP. Swing and RELEASE RIGHT GRIP.":L"HOLD RIGHT GRIP\nWaiting for the native projectile.");
}


void ReleaseThrowableResources145(){ClearMolotov158();ReleaseGrenadePin154();SafeRelease(g_throw144.particles);g_throw144.particleBytes=0;for(int i=0;i<8;++i){SafeRelease(g_throw144.world[i]);g_throw144.worldBytes[i]=0;}}
