// Candidate 111: the game's own muzzle flash and tracer particles.
// MUZZLE58/107 move the bullet to the drawn muzzle, but the mod never moved the flash or tracer
// (stereo.cpp [SHOT DRAWS] note). They are born at the NATIVE first-person muzzle, which sits on the
// eye's centre line about 0.24 m above the drawn D50 tip (PID63244 shots 1-6) and 0.4-0.6 m from the
// drawn M133 tip. Run 45704's [SHOT DRAWS] probe saw a single blended signature while firing every
// weapon: pooled particles in a 147456-byte dynamic VB (stride 36, VS 8CBC4FD2A59FD8A3) with no
// readable world constants, i.e. world-space vertices, as for the repair torch (repair_tool.inl).
// For 1 s after a local corrected shot, world-camera (variant 0) blended draws whose referenced
// vertices lie within 0.4 m of the native muzzle have those vertices moved by the same native->drawn
// transport as the bullet, drawn from a private VB copy. The game's VB is never written. Variant 1
// (weapon camera) draws already follow the gun in the eye matrix and are left alone.
#include "flamer_stream239.inl"
#include "flamer_projection241.inl"
bool FlamerKeepVertexShadow242(const D3D11_BUFFER_DESC&);
constexpr double kMuzzleFlashRadius111=.4;
constexpr ULONGLONG kMuzzleFlashWindowMs111=1000;
struct MuzzleFlash111 {V3 local={};ULONGLONG shotMs=0;unsigned sequence=0;UINT epoch=0,profile=~0u;bool localValid=false,seen=false;LONG moved[65]={},refused=0;} g_muzzleFlash111;
ID3D11Buffer* g_muzzleFlashVb111=nullptr;UINT g_muzzleFlashVbBytes111=0;
ULONGLONG g_muzzleFlashClockForTest111=0;
ULONGLONG MuzzleFlashMs111(){return g_muzzleFlashClockForTest111?g_muzzleFlashClockForTest111:GetTickCount64();}
// Pure: the native muzzle in the native FRAME's own axes, so later frames can follow the gun.
bool MuzzleFlashLocal111(const Mat4& nativeFrame,const V3& muzzleWorld,V3& local)
{
    Mat4 inverse={};if(!RepairRigidMatrix(nativeFrame)||!Invert(nativeFrame,inverse))return false;
    local=Apply(inverse,muzzleWorld);return std::isfinite(local.x)&&std::isfinite(local.y)&&std::isfinite(local.z)&&Length(local)<1.5;
}
// New local shot: anchor its native muzzle to the latest confirmed core draw (<= 2 frames old).
void MuzzleFlashPoll111()
{
    float muzzle[3]={};unsigned sequence=0;auto& f=g_muzzleFlash111;const auto& pose=g_muzzleFlashPose111;
    if(!fc3aim::LastShotMuzzle111(muzzle,sequence))return;
    if(f.seen&&sequence==f.sequence)return;
    f.seen=true;f.sequence=sequence; // polled on every pooled-VB Map, so a new sequence is this frame's shot
    if(!pose.valid||pose.epoch!=g_armPoseEpoch||g_frame-pose.frame>2)return;
    V3 local={};if(!MuzzleFlashLocal111(pose.native,V3{muzzle[0],muzzle[1],muzzle[2]},local))return;
    f.local=local;f.shotMs=MuzzleFlashMs111();f.epoch=pose.epoch;f.profile=pose.profile;f.localValid=true;
}
// 244: the flamethrower jet. Live 243 census: while the trigger is held, every blended particle draw is this same
// 131072-byte WORLD-space pool (strides 28-56, 2-16 quads, world constants bound), the pool whose revolver muzzle
// flash 111 already moves correctly. 241 had excluded profile 15 from 111 outright, so the jet stayed at the native
// (head-locked) nozzle. The window is open while the trigger is held; the selection is a cone from the nozzle.
bool FlamerJetWindow243(){const auto& pose=g_muzzleFlashPose111;if(fc3aim::BoneRewriteInstalled246())return false;return g_weaponMechanicProfile==15&&FlamerModel()&&MotionInteractionEnabled()&&fc3pad::Latest().rightTrigger>.3f&&pose.valid&&pose.profile==15&&pose.epoch==g_armPoseEpoch&&g_frame-pose.frame<=2;}
// Pure: referenced vertices (leading float3, world) inside the nozzle cone of the native FRAME are moved by move.
// 245: the cone follows the game's own flame birth record (native origin + spray axis, fresh every frame while
// firing); the FRAME nozzle/+Y guess of 244 missed every jet vertex by 0.9-2.2 m in live 244. Cone: 0.5 m behind
// to 9 m ahead of the origin along the axis, radius 0.9 m + 0.18 m per metre. Fallback: FRAME nozzle and +Y.
struct JetCone245 {V3 origin={},axis={};bool birth=false;ULONGLONG ageMs=0;double minAlong=1e9,minRadial=1e9;UINT inside=0;};
bool JetCone245Setup(const Mat4& native,JetCone245& cone)
{
    float origin[3]={},axis[3]={};unsigned long long age=0;
    if(fc3aim::LastFlameBirth245(origin,axis,age)&&age<=250&&Length({axis[0],axis[1],axis[2]})>.5){cone.origin={origin[0],origin[1],origin[2]};cone.axis=Unit({axis[0],axis[1],axis[2]});cone.birth=true;cone.ageMs=age;return true;}
    cone.origin=Apply(native,kFlamerNozzle239);cone.axis=Unit({native.m[1],native.m[5],native.m[9]});cone.birth=false;cone.ageMs=age;return Length(cone.axis)>.5;
}
UINT MuzzleFlashMoveCone243(unsigned char* data,UINT bytes,UINT stride,UINT offset,const Mat4& native,const Mat4& move,const std::vector<UINT>& references,double* nearest,JetCone245* report)
{
    if(nearest)*nearest=-1;JetCone245 cone;if(!data||stride<12||stride>64||offset>=bytes||!JetCone245Setup(native,cone))return 0;UINT moved=0;double best=1e18;
    for(UINT vertex:references)
    {
        const unsigned long long at=static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(vertex)*stride;if(at+12>bytes)continue;
        float p[3];memcpy(p,data+at,sizeof(p));if(!std::isfinite(p[0])||!std::isfinite(p[1])||!std::isfinite(p[2]))continue;
        const V3 u=Sub(V3{p[0],p[1],p[2]},cone.origin);const double d=Length(u);if(d<best)best=d;
        const double along=Dot(u,cone.axis),radial=Length(Sub(u,Scale(cone.axis,along)));
        if(along<cone.minAlong)cone.minAlong=along;if(radial<cone.minRadial)cone.minRadial=radial;
        if(along<-.5||along>9||radial>.9+.18*(std::max)(0.,along))continue;
        const V3 out=Apply(move,V3{p[0],p[1],p[2]});p[0]=static_cast<float>(out.x);p[1]=static_cast<float>(out.y);p[2]=static_cast<float>(out.z);memcpy(data+at,p,sizeof(p));++moved;
    }
    cone.inside=moved;if(report)*report=cone;if(nearest&&best<1e18)*nearest=best;return moved;
}
bool MuzzleFlashWindow111()
{
    if(g_weaponMechanicProfile==15)return FlamerJetWindow243();
    if(FlamerStreamActive239())return true;
    MuzzleFlashPoll111();const auto& f=g_muzzleFlash111;
    return f.localValid&&f.epoch==g_armPoseEpoch&&MuzzleFlashMs111()-f.shotMs<=kMuzzleFlashWindowMs111&&g_muzzleFlashPose111.valid&&g_frame-g_muzzleFlashPose111.frame<=2;
}
bool MuzzleFlashKeepVertexShadow111(const D3D11_BUFFER_DESC& d)
{if(FlamerKeepVertexShadow242(d))return true;if(g_weaponMechanicProfile==15)return FlamerJetWindow243()&&d.Usage==D3D11_USAGE_DYNAMIC&&(d.BindFlags&D3D11_BIND_VERTEX_BUFFER)&&d.ByteWidth>=12&&d.ByteWidth<=192*1024;
if(FlamerStreamActive239())return d.Usage==D3D11_USAGE_DYNAMIC&&(d.BindFlags&D3D11_BIND_VERTEX_BUFFER)&&d.ByteWidth==147456;
return d.Usage==D3D11_USAGE_DYNAMIC&&(d.BindFlags&D3D11_BIND_VERTEX_BUFFER)&&d.ByteWidth>=12&&d.ByteWidth<=192*1024&&MuzzleFlashWindow111();}
// Pure: referenced vertices (leading float3, world) within radius of centre are moved by move.
UINT MuzzleFlashMoveVertices111(unsigned char* data,UINT bytes,UINT stride,UINT offset,const V3& centre,double radius,const Mat4& move,const std::vector<UINT>& references,double* nearest)
{
    if(nearest)*nearest=-1;if(!data||stride<12||stride>64||offset>=bytes)return 0;UINT moved=0;double best=1e18;
    for(UINT vertex:references)
    {
        const unsigned long long at=static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(vertex)*stride;if(at+12>bytes)continue;
        float p[3];memcpy(p,data+at,sizeof(p));const double x=p[0]-centre.x,y=p[1]-centre.y,z=p[2]-centre.z;
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))continue;const double d2=x*x+y*y+z*z;if(d2<best)best=d2;if(d2>radius*radius)continue;
        const V3 out=Apply(move,V3{p[0],p[1],p[2]});p[0]=static_cast<float>(out.x);p[1]=static_cast<float>(out.y);p[2]=static_cast<float>(out.z);memcpy(data+at,p,sizeof(p));++moved;
    }
    if(nearest&&best<1e18)*nearest=std::sqrt(best);return moved;
}
bool BeginMuzzleFlashDraw111(ID3D11DeviceContext* context,const DrawCall& call,int variant,ID3D11Buffer*& original,UINT& stride,UINT& offset)
{
    original=nullptr;if(variant!=0||!MotionInteractionEnabled()||!MuzzleFlashWindow111())return false;
    ID3D11BlendState* blend=nullptr;float factor[4];UINT mask=0;context->OMGetBlendState(&blend,factor,&mask);D3D11_BLEND_DESC bd={};if(blend)blend->GetDesc(&bd);SafeRelease(blend);if(!bd.RenderTarget[0].BlendEnable)return false;
    ID3D11Buffer* vb=nullptr;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC desc={};if(vb)vb->GetDesc(&desc);
    if(!vb||desc.Usage!=D3D11_USAGE_DYNAMIC||desc.ByteWidth>192*1024||stride<12||stride>64||call.a>16384){SafeRelease(vb);return false;}
    LONG copied=-100000;UINT frameBytes=sizeof(copied);if(FAILED(vb->GetPrivateData(kRepairVertexFrameGuid,&frameBytes,&copied))||g_frame-copied<0||g_frame-copied>1){SafeRelease(vb);return false;}
    std::vector<unsigned char> vertices(desc.ByteWidth);if(ReadCopy(vb,vertices.data(),desc.ByteWidth)!=desc.ByteWidth){SafeRelease(vb);return false;}
    std::vector<UINT> references;DXGI_FORMAT indexFormat=DXGI_FORMAT_UNKNOWN;UINT indexOffset=0;if(!RepairReferencedVertices(context,call,desc.ByteWidth,stride,offset,references,indexFormat,indexOffset)){SafeRelease(vb);return false;}
    auto& f=g_muzzleFlash111;const auto& pose=g_muzzleFlashPose111;const bool stream=FlamerStreamActive239();
    ID3D11VertexShader* streamVs=nullptr;context->VSGetShader(&streamVs,nullptr,nullptr);const auto streamHash=ShaderHash(streamVs);SafeRelease(streamVs);
    if(stream&&(streamHash!=0x8CBC4FD2A59FD8A3ull||desc.ByteWidth!=147456||stride!=36)){SafeRelease(vb);return false;}
    const bool jet243=g_weaponMechanicProfile==15;const V3 centre=stream?Apply(g_flamerStream239.native,kFlamerNozzle239):jet243?Apply(pose.native,kFlamerNozzle239):Apply(pose.native,f.local);
    double nearest=-1;JetCone245 cone245;const UINT moved=stream?(g_frameCameraValid?MoveFlamerClipStream240(vertices.data(),desc.ByteWidth,stride,offset,references,g_flamerStream239,g_frameCamera):0):jet243?MuzzleFlashMoveCone243(vertices.data(),desc.ByteWidth,stride,offset,pose.native,pose.move,references,&nearest,&cone245):MuzzleFlashMoveVertices111(vertices.data(),desc.ByteWidth,stride,offset,centre,kMuzzleFlashRadius111,pose.move,references,&nearest);
    const auto shader=streamHash;
    if(!moved)
    {
        if(stream){static UINT misses=0;if(misses++<6){float first[3]={};if(!references.empty()&&static_cast<unsigned long long>(offset)+static_cast<unsigned long long>(references[0])*stride+12<=vertices.size())memcpy(first,vertices.data()+offset+references[0]*stride,12);Log("[FLAME240] no connected NDC stream: camera valid%d projection %.4f/%.4f/%.6f/%.6f, first position %.4f/%.4f/%.6f, snapshot age%ld",g_frameCameraValid,g_frameCamera.xScale,g_frameCamera.yScale,g_frameCamera.zScale,g_frameCamera.zOffset,first[0],first[1],first[2],g_frame-g_flamerStream239.frame);}}

        SafeRelease(vb);
        if(jet243){static LONG jetLeft=0;if(nearest>=0&&nearest<6&&jetLeft<60){++jetLeft;Log("[JET245] draw left alone: nearest vertex %.2f m from the cone origin %.2f/%.2f/%.2f (%s, age %llu ms, axis %.2f/%.2f/%.2f); min along %.2f m, min radial %.2f m over %zu vertices (VS %016llX, VB %u stride %u, count %u)",nearest,cone245.origin.x,cone245.origin.y,cone245.origin.z,cone245.birth?"native flame birth":"FRAME nozzle fallback",cone245.ageMs,cone245.axis.x,cone245.axis.y,cone245.axis.z,cone245.minAlong,cone245.minRadial,references.size(),shader,desc.ByteWidth,stride,call.a);}}
        else if(nearest>=0&&nearest<3&&f.refused<16){++f.refused;Log("[MUZZLE FLASH111] blended world draw left alone: nearest referenced vertex %.2f m from the native muzzle (VS %016llX, VB %u stride %u, kind %u count %u, profile %ld)",nearest,shader,desc.ByteWidth,stride,call.kind,call.a,g_weaponMechanicProfile);}
        return false;
    }
    if(g_muzzleFlashVbBytes111!=desc.ByteWidth){SafeRelease(g_muzzleFlashVb111);g_muzzleFlashVbBytes111=0;D3D11_BUFFER_DESC d={};d.ByteWidth=desc.ByteWidth;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;if(FAILED(g_oCreateBuffer(g_dev,&d,nullptr,&g_muzzleFlashVb111))){SafeRelease(vb);return false;}g_muzzleFlashVbBytes111=d.ByteWidth;}
    const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_muzzleFlashVb111,0,nullptr,vertices.data(),0,0);context->IASetVertexBuffers(0,1,&g_muzzleFlashVb111,&stride,&offset);g_own=own;original=vb;
    if(jet243){static LONG jetMoved=0;if(jetMoved<40||jetMoved%200==0){Log("[JET245] %u of %zu vertices moved to the drawn nozzle (cone from %s, age %llu ms, origin %.2f/%.2f/%.2f axis %.2f/%.2f/%.2f; VS %016llX stride %u count %u)",moved,references.size(),cone245.birth?"native flame birth":"FRAME nozzle fallback",cone245.ageMs,cone245.origin.x,cone245.origin.y,cone245.origin.z,cone245.axis.x,cone245.axis.y,cone245.axis.z,shader,stride,call.a);}++jetMoved;return true;}
    if(stream){static UINT shown=0;if(shown++<6)Log("[FLAME240] NDC -> native world -> nozzle -> NDC stream: %u/%zu referenced vertices attached to current nozzle; original particle buffer preserved",moved,references.size());return true;}
    const UINT slot=f.profile<64?f.profile:64;
    if(++f.moved[slot]<=(jet243?24:6)){const V3 drawn=Apply(pose.move,centre);Log("[MUZZLE FLASH111] profile %u: %u of %zu referenced particle vertices within %.1f m of the native muzzle moved to the drawn muzzle (VS %016llX, VB %u stride %u, kind %u count %u); native %.3f %.3f %.3f drawn %.3f %.3f %.3f",f.profile,moved,references.size(),kMuzzleFlashRadius111,shader,desc.ByteWidth,stride,call.kind,call.a,centre.x,centre.y,centre.z,drawn.x,drawn.y,drawn.z);}
    return true;
}
#include "flamer_draw242.inl"
void ReleaseMuzzleFlashResources111(){g_flamerStream239={};ReleaseActionWhite231();ReleaseVectorCasing231();ReleaseRifleCasings235();ReleaseSvdLens236();SafeRelease(g_muzzleFlashVb111);g_muzzleFlashVbBytes111=0;g_muzzleFlash111=MuzzleFlash111{};g_muzzleFlashPose111=MuzzleFlashPose111{};}
