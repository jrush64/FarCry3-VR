// Native type-2 capsule, request scoped. No engine object or shared config writes.
const V3 kMacheteBladeBase={.02612384408712387,.0029908139258623123,.08346812333911657};
const V3 kMacheteBladeTip={-.007904293946921825,.0029908139258623123,.4286324651911855};
struct MacheteBladeRay { V3 base={},tip={}; };
struct MacheteBladeStrokeQuery {
    V3 points[64]={};UINT count=0,sequence=0,epoch=0,pawn=0,thread=0,invalidReason=0;
    ULONGLONG invalidTime=0;UINT invalidFlags=0;float invalidJump=0;
    ULONGLONG requested=0;float scale=0;bool valid=false;
} g_bladeStroke;
#ifdef FC3_AMMO_TEST
bool g_macheteSweptEnabled=false;
#else
bool g_macheteSweptEnabled=false; // native swipe keeps its stock capsule and actor search
#endif
bool MacheteBladeRayFromPose(const MacheteQuickPose& p,MacheteBladeRay& ray)
{
    if(!std::isfinite(p.scale)||p.scale<.01f||p.scale>100.f)return false;
    Mat4 hand={},camera=HandIdentity();
    for(int i=0;i<16;++i){hand.m[i]=p.hand[i];if(!std::isfinite(hand.m[i]))return false;}
    for(int r=0;r<3;++r)
    {
        if(!std::isfinite(p.cameraPosition[r]))return false;
        camera.m[r*4+3]=p.cameraPosition[r];
        for(int c=0;c<3;++c)camera.m[r*4+c]=p.cameraBasis[c*3+r];
    }
    Mat4 cameraBasis={};if(!HandBasis(camera,cameraBasis))return false;
    for(int c=0;c<3;++c)
    {
        const V3 a={camera.m[c],camera.m[4+c],camera.m[8+c]};
        if(!std::isfinite(Length(a))||std::fabs(Length(a)-1)>.01)return false;
        for(int d=c+1;d<3;++d)
            if(std::fabs(Dot(a,{camera.m[d],camera.m[4+d],camera.m[8+d]}))>.01)return false;
    }
    // Inverse of HullView: transpose orthonormal camera axes plus world position.
    // Do not apply WorldScale twice: the completed rendered hand already uses it.
    const Mat4 blade=Multiply(camera,Multiply(hand,kMacheteFromHand[0]));
    const V3 modelBase=g_machete.blade>=0?g_machete.bladeBase:kMacheteBladeBase,modelTip=g_machete.blade>=0?g_machete.bladeTip:kMacheteBladeTip;
    ray.base=Apply(blade,modelBase);ray.tip=Apply(blade,modelTip);
    const double length=Length(Sub(ray.tip,ray.base));
    return std::isfinite(length)&&length>.05&&length<2.0&&
        Length(Sub(ray.base,{p.cameraPosition[0],p.cameraPosition[1],p.cameraPosition[2]}))<3.0;
}
void BeginMacheteBladeStroke(UINT sequence,ULONGLONG now)
{
    if(!TryAcquireSRWLockExclusive(&g_macheteQuickPoseLock))return;
    g_bladeStroke=MacheteBladeStrokeQuery{};
    const auto& p=g_macheteQuickPose;MacheteBladeRay blade;
    if(g_macheteSweptEnabled&&(g_macheteQuickPoseFlags&61)==61&&now>=p.ms&&now-p.ms<=100&&MacheteBladeRayFromPose(p,blade))
    {
        g_bladeStroke.valid=true;g_bladeStroke.sequence=sequence;g_bladeStroke.requested=now;
        g_bladeStroke.epoch=p.epoch;g_bladeStroke.scale=p.scale;
        g_bladeStroke.points[0]=blade.base;g_bladeStroke.points[1]=blade.tip;g_bladeStroke.count=2;
    }
    ReleaseSRWLockExclusive(&g_macheteQuickPoseLock);
}
// Called with the snapshot lock held. Collect only the early physical stroke;
// the native impact callback arrives later and must not use the returned hand.
void CaptureMacheteBladePose(const MacheteQuickPose& p,UINT flags)
{
    auto& s=g_bladeStroke;if(!s.valid)return;
    if((flags&37)!=37||p.epoch!=s.epoch||p.scale!=s.scale){s.valid=false;s.invalidReason=(flags&37)!=37?1:p.epoch!=s.epoch?2:3;s.invalidTime=p.ms;s.invalidFlags=flags;return;}
    // A native attack can omit hand draws; retain recorded geometry through
    // that gap but never collect an unobserved/stale rendered hand.
    if((flags&24)!=24)return;
    if(p.ms<s.requested||p.ms-s.requested>250)return;
    MacheteBladeRay b;if(!MacheteBladeRayFromPose(p,b)){s.valid=false;s.invalidReason=4;s.invalidTime=p.ms;s.invalidFlags=flags;return;}
    if(s.count>=2&&Length(Sub(b.tip,s.points[s.count-1]))>.5){s.valid=false;s.invalidReason=5;s.invalidTime=p.ms;s.invalidFlags=flags;s.invalidJump=float(Length(Sub(b.tip,s.points[s.count-1])));return;}
    if(s.count+2<=64){s.points[s.count++]=b.base;s.points[s.count++]=b.tip;}
}
bool BladeCapsule(const MacheteBladeStrokeQuery& s,V3& a,V3& b,double& radius)
{
    if(!s.valid||s.count<2)return false;double longest=0;
    for(UINT i=0;i<s.count;++i)for(UINT j=i+1;j<s.count;++j){const double n=Length(Sub(s.points[i],s.points[j]));if(n>longest){longest=n;a=s.points[i];b=s.points[j];}}
    if(longest<.05||longest>1.5)return false;
    const V3 axis=Unit(Sub(b,a));radius=.025;
    for(UINT i=0;i<s.count;++i){const V3 d=Sub(s.points[i],a);const double t=(std::max)(0.0,(std::min)(longest,Dot(d,axis)));radius=(std::max)(radius,Length(Sub(d,Scale(axis,t)))+.025);}
    return std::isfinite(radius)&&radius<=.3;
}
void BindMacheteBladeRequest(const MacheteQuickRecord& r)
{
    if(!TryAcquireSRWLockExclusive(&g_macheteQuickPoseLock))return;
    auto& s=g_bladeStroke;
    if(s.valid&&!s.pawn&&r.sequence==s.sequence&&r.pose.epoch==s.epoch&&r.ms>=s.requested&&r.ms-s.requested<=250&&r.strategy){s.pawn=r.strategy;s.thread=r.thread;}
    ReleaseSRWLockExclusive(&g_macheteQuickPoseLock);
}
bool CorrectMacheteSweptQuery(MacheteQuickRecord& r,UINT stack)
{
    // Reserved QMP1 words report why each phase retained its stock query.
    // This adds evidence only; it does not relax any gameplay/write guard.
    r.flags|=512;r.payload[36]=0x52454131;
    auto reject=[&](UINT reason){r.event[6]=reason;return false;};
    if(!g_macheteSweptEnabled)return reject(1);
    if((r.flags&48)!=48)return reject(2);
    if((r.poseFlags&45)!=45)return reject(3);
    if(r.ms<r.pose.ms||r.ms-r.pose.ms>100)return reject(4);
    MacheteBladeStrokeQuery stroke;
    if(!TryAcquireSRWLockShared(&g_macheteQuickPoseLock))return reject(5);
    stroke=g_bladeStroke;ReleaseSRWLockShared(&g_macheteQuickPoseLock);
    r.payload[37]=stroke.valid?1:0;r.payload[38]=stroke.count;r.payload[39]=stroke.invalidReason;
    r.payload[40]=stroke.invalidFlags;r.payload[41]=UINT(stroke.invalidTime>=stroke.requested?stroke.invalidTime-stroke.requested:0);
    memcpy(r.payload+42,&stroke.invalidJump,4);r.payload[43]=stroke.sequence;r.payload[44]=stroke.pawn;r.payload[45]=stroke.thread;r.payload[46]=stroke.epoch;
    r.payload[47]=UINT(r.ms>=stroke.requested?r.ms-stroke.requested:0);
    if(!stroke.valid)return reject(6);
    if(r.sequence!=stroke.sequence)return reject(7);
    if(r.strategy!=stroke.pawn||!stroke.pawn)return reject(8);
    if(r.thread!=stroke.thread)return reject(9);
    if(r.pose.epoch!=stroke.epoch)return reject(10);
    if(r.ms<stroke.requested||r.ms-stroke.requested>1200)return reject(11);
    float original[25];memcpy(original,r.payload,sizeof(original));
    const V3 origin={original[0],original[1],original[2]},axis={original[22],original[23],original[24]};
    const V3 camera={r.pose.cameraPosition[0],r.pose.cameraPosition[1],r.pose.cameraPosition[2]},forward={r.pose.cameraBasis[6],r.pose.cameraBasis[7],r.pose.cameraBasis[8]};
    const float gap=float(Length(Sub(origin,camera))),axisLength=float(Length(axis)),agreement=float(Dot(Unit(axis),Unit(forward)));
    memcpy(r.payload+48,&gap,4);memcpy(r.payload+49,&axisLength,4);memcpy(r.payload+50,&agreement,4);
    if(gap>.15)return reject(12);if(axisLength<.9)return reject(13);if(agreement<.98)return reject(14);
    V3 a,b;double radius=0;const bool capsule=BladeCapsule(stroke,a,b,radius);
    const float span=float(Length(Sub(b,a))),rad=float(radius);memcpy(r.payload+51,&span,4);memcpy(r.payload+52,&rad,4);
    if(!capsule)return reject(15);
    const V3 d=Unit(Sub(b,a)),centre=Scale(Add(a,b),.5);const float halfLength=float(Length(Sub(b,a))*.5),thickness=float(radius);
    const float base[3]={float(a.x),float(a.y),float(a.z)},tip[3]={float(b.x),float(b.y),float(b.z)},dir[3]={float(d.x),float(d.y),float(d.z)},position[3]={float(centre.x),float(centre.y),float(centre.z)};
    // At this exact boundary: EAX=ESP+0xe8 at 1144DE20 after its
    // three pushes. type2 packet offsets +08 axis,+14 radius,+18 halfLength,+1c centre.
    // ESP+0x100 is halfLength, NEVER a position vector.
    UINT vtable=0;float nativeRadius=0,nativeHalf=0;
    if(!ReloadTraceRead(stack+0xe8,&vtable,4)||!ReloadTraceRead(stack+0xfc,&nativeRadius,4)||!ReloadTraceRead(stack+0x100,&nativeHalf,4))return reject(16);
    r.payload[53]=vtable;memcpy(r.payload+54,&nativeRadius,4);memcpy(r.payload+55,&nativeHalf,4);
    if(vtable!=r.engine+0x1f58ac0||fabs(nativeRadius-.25f)>.001f||fabs(nativeHalf-.05f)>.001f)return reject(17);
    V3 up=fabs(d.z)<.9?V3{0,0,1}:V3{0,1,0};const V3 right=Unit(Cross(d,up));up=Cross(right,d);
    const float matrix[16]={float(right.x),float(right.y),float(right.z),0,dir[0],dir[1],dir[2],0,float(up.x),float(up.y),float(up.z),0,base[0],base[1],base[2],1};
    __try {
        memcpy(reinterpret_cast<void*>(stack+0x5c),base,12);memcpy(reinterpret_cast<void*>(stack+0x48),tip,12);memcpy(reinterpret_cast<void*>(stack+0x3c),dir,12);
        memcpy(reinterpret_cast<void*>(stack+0xf0),dir,12);memcpy(reinterpret_cast<void*>(stack+0xfc),&thickness,4);memcpy(reinterpret_cast<void*>(stack+0x100),&halfLength,4);memcpy(reinterpret_cast<void*>(stack+0x104),position,12);
        memcpy(reinterpret_cast<void*>(stack+0x110),matrix,64);
    } __except(EXCEPTION_EXECUTE_HANDLER){return reject(18);}
    memcpy(r.payload+25,base,12);memcpy(r.payload+28,tip,12);memcpy(r.payload+31,position,12);memcpy(r.payload+34,&thickness,4);memcpy(r.payload+35,&halfLength,4);
    r.flags|=256;r.event[6]=0;r.event[7]=stroke.count;InterlockedIncrement(&g_macheteQuick.corrected);return true;
}

void CancelMacheteBladeStroke()
{ AcquireSRWLockExclusive(&g_macheteQuickPoseLock);g_bladeStroke=MacheteBladeStrokeQuery{};ReleaseSRWLockExclusive(&g_macheteQuickPoseLock);}
