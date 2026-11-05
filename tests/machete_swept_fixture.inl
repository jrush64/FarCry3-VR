#include "machete_blade_live_data.inl"
void TestSweptBlade()
{
    for(const auto& sample:kBladeLiveData){MacheteQuickPose p;p.scale=1;for(int i=0;i<16;++i)p.hand[i]=float(sample[i]);for(int i=0;i<3;++i)p.cameraPosition[i]=float(sample[16+i]);for(int i=0;i<9;++i)p.cameraBasis[i]=float(sample[19+i]);MacheteBladeRay ray;Check(MacheteBladeRayFromPose(p,ray),"captured rendered blade valid");Check(Length(Sub(ray.base,{sample[28],sample[29],sample[30]}))<1e-5&&Length(Sub(ray.tip,{sample[31],sample[32],sample[33]}))<1e-5,"30 real poses match independent mesh/world coordinates");}
    g_macheteSweptEnabled=true;const ULONGLONG now=GetTickCount64();MacheteQuickPose p;p.ms=now;p.epoch=17;p.scale=1;p.cameraBasis[0]=p.cameraBasis[4]=p.cameraBasis[8]=1;const Mat4 h=HandIdentity();for(int i=0;i<16;++i)p.hand[i]=float(h.m[i]);g_macheteQuickPose=p;g_macheteQuickPoseFlags=61;
    BeginMacheteBladeStroke(12,now);Check(g_bladeStroke.valid,"fresh pose seeds stroke");
    for(int i=1;i<=5;++i){p.ms=now+i*20;p.hand[3]=i*.035f;CaptureMacheteBladePose(p,61);}Check(g_bladeStroke.count==12,"blade endpoints captured through motion");
    V3 a,b;double radius;Check(BladeCapsule(g_bladeStroke,a,b,radius),"bounded sweep capsule");const V3 axis=Unit(Sub(b,a));const double len=Length(Sub(b,a));
    for(UINT i=0;i<g_bladeStroke.count;++i){const V3 d=Sub(g_bladeStroke.points[i],a);const double t=(std::max)(0.0,(std::min)(len,Dot(d,axis)));Check(Length(Sub(d,Scale(axis,t)))<=radius,"capsule covers recorded endpoints");}
    const UINT count=g_bladeStroke.count;CaptureMacheteBladePose(p,45);Check(g_bladeStroke.valid&&g_bladeStroke.count==count,"native hand-draw gap retains recorded sweep");p.ms=now+400;p.hand[3]=.7f;CaptureMacheteBladePose(p,61);Check(g_bladeStroke.count==count,"idle return cannot move delayed contact");
    auto record=[&](){MacheteQuickRecord r;r.ms=now+450;r.pose=p;r.pose.ms=r.ms;r.poseFlags=61;r.flags=48;r.sequence=12;r.thread=GetCurrentThreadId();r.strategy=0x12345678;r.engine=0x50000000;float native[25]={};native[24]=1;memcpy(r.payload,native,sizeof(native));return r;};
    auto req=record();req.ms=now+10;BindMacheteBladeRequest(req);auto other=req;other.strategy++;BindMacheteBladeRequest(other);Check(g_bladeStroke.pawn==req.strategy,"later actor request cannot replace bound owner");
    BYTE* page=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"native locals");memset(page,0x5a,4096);const UINT stack=reinterpret_cast<UINT>(page),vtable=req.engine+0x1f58ac0;memcpy(page+0xe8,&vtable,4);float nr=.25f,nh=.05f;memcpy(page+0xfc,&nr,4);memcpy(page+0x100,&nh,4);const std::vector<BYTE> before(page,page+4096);
    auto r=record();r.poseFlags=45;Check(CorrectMacheteSweptQuery(r,stack)&&(r.flags&256),"native query uses recorded sweep");float half,thick,centre[3];memcpy(&half,page+0x100,4);memcpy(&thick,page+0xfc,4);memcpy(centre,page+0x104,12);
    Check((r.flags&512)&&r.event[6]==0&&r.payload[36]==0x52454131,"successful query reports applied reason");
    Check(fabs(half-len*.5)<1e-5&&fabs(thick-radius)<1e-5&&Length(Sub({centre[0],centre[1],centre[2]},Scale(Add(a,b),.5)))<1e-5,"capsule fields are dimensions then centre");
    for(UINT i=0;i<4096;++i){bool changed=(i>=0x110&&i<0x150)||(i>=0xf0&&i<0x110);for(UINT off:{0x5cu,0x48u,0x3cu})changed|=i>=off&&i<off+12;if(!changed)Check(page[i]==before[i],"only owned ray/capsule/frame fields written");}
    auto reject=[&](MacheteQuickRecord r,const char* why){memcpy(page,before.data(),4096);Check(!CorrectMacheteSweptQuery(r,stack)&&memcmp(page,before.data(),4096)==0,why);Check((r.flags&512)&&r.event[6]>0,"every rejected query has a reason");};
    r=record();r.strategy++;reject(r,"other actor excluded");r=record();r.sequence++;reject(r,"other sequence excluded");r=record();r.thread++;reject(r,"other thread excluded");r=record();r.pose.epoch++;reject(r,"recenter excluded");r=record();r.poseFlags&=~4u;reject(r,"tracking loss excluded");r=record();r.ms+=101;reject(r,"stale pose excluded");r=record();r.ms=now+1201;r.pose.ms=r.ms;reject(r,"expired request excluded");
    g_macheteSweptEnabled=false;r=record();reject(r,"disabled query untouched");g_macheteSweptEnabled=true;p.ms=now+50;p.hand[3]=2;CaptureMacheteBladePose(p,61);Check(!g_bladeStroke.valid,"teleport discards stroke");
    r=record();memcpy(page,before.data(),4096);Check(!CorrectMacheteSweptQuery(r,stack)&&r.event[6]==6&&r.payload[39]==5&&r.payload[40]==61&&r.payload[41]==50,"delayed second query preserves precise stroke invalidation evidence");Check(memcmp(page,before.data(),4096)==0,"diagnostic rejection leaves native stack unchanged");
    g_bladeStroke.valid=true;CancelMacheteBladeStroke();Check(!g_bladeStroke.valid,"explicit stow discards delayed stroke before redraw");
    g_macheteSweptEnabled=false;g_bladeStroke=MacheteBladeStrokeQuery{};VirtualFree(page,0,MEM_RELEASE);
    puts("PASS: native capsule dimensions/centre layout, physical sweep coverage/delayed freeze, real world poses, bounded native writes and unrelated actor/thread/epoch/tracking exclusion");
}

