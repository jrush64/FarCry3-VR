#include "pistol_insert_path.inl"
Mat4 PistolRigidBlend(const Mat4& a,const Mat4& b,double t)
{
    t=(std::max)(0.0,(std::min)(1.0,t));if(t==0)return a;if(t==1)return b;
    Mat4 result={};result.m[15]=1;
    V3 x={a.m[0]*(1-t)+b.m[0]*t,a.m[4]*(1-t)+b.m[4]*t,a.m[8]*(1-t)+b.m[8]*t};
    V3 y={a.m[1]*(1-t)+b.m[1]*t,a.m[5]*(1-t)+b.m[5]*t,a.m[9]*(1-t)+b.m[9]*t};
    if(Length(x)<.00001)x={b.m[0],b.m[4],b.m[8]};x=Unit(x);y=Sub(y,Scale(x,Dot(x,y)));
    if(Length(y)<.00001)y=Sub(V3{b.m[1],b.m[5],b.m[9]},Scale(x,Dot(x,{b.m[1],b.m[5],b.m[9]})));
    if(Length(y)<.00001)y=Cross(std::fabs(x.z)<.9?V3{0,0,1}:V3{0,1,0},x);
    y=Unit(y);V3 z=Unit(Cross(x,y));
    // Keep the inputs' handedness (Z93/R700 hand-over, 2026-10-09: the gun roots there are MIRRORED matrices; rebuilding Z as X x Y
    // made every blended frame mirrored the other way - the recorded gun flipped upside down for ~6 frames, then snapped back).
    {const V3 ax={a.m[0],a.m[4],a.m[8]},ay={a.m[1],a.m[5],a.m[9]},az={a.m[2],a.m[6],a.m[10]};if(Dot(Cross(ax,ay),az)<0)z=Scale(z,-1);}
    result.m[0]=x.x;result.m[4]=x.y;result.m[8]=x.z;
    result.m[1]=y.x;result.m[5]=y.y;result.m[9]=y.z;
    result.m[2]=z.x;result.m[6]=z.y;result.m[10]=z.z;
    for(int r=0;r<3;++r)result.m[r*4+3]=a.m[r*4+3]*(1-t)+b.m[r*4+3]*t;
    return result;
}
void BeginPistolInsert()
{
    auto& p=g_pistolReload;p.insertStart=kPistolInsertKeys[0];Mat4 viewInverse={},rootInverse={};
    Mat4 insertionRoot=p.rootBody;const bool rootReady=Resolve1911Insertion(p.rootBody,insertionRoot,false);
    if(rootReady&&g_frame-p.rootFrame<=1&&g_frame-p.handFrame<=1&&Invert(ArmBodyToCamera(),viewInverse)&&Invert(insertionRoot,rootInverse))
        {p.insertStart=Multiply(rootInverse,Multiply(viewInverse,p.hand));if(A2000LeftCharge336())p.insertStart=A2000ChargeTransform336(p.insertStart);}
    p.insertActive=true;p.insertAt=GetTickCount64();
    Log("[PISTOL INSERT] held magazine -> measured native seating path; one existing ammo transaction");
}
Mat4 PistolInsertRelative(double age)
{
    auto& p=g_pistolReload;
    if(AkModel())return PistolRigidBlend(p.insertStart,*CurrentRifle().clipRest,age/.28);
    if(SkorpionModel())return PistolRigidBlend(p.insertStart,kSkorpionClipRest,age/.28);
    if(D50Model())return PistolRigidBlend(p.insertStart,kD50ClipRestM,age/.28);
    if(age<=0)return p.insertStart;
    if(age<.06)return PistolRigidBlend(p.insertStart,kPistolInsertKeys[0],age/.06);
    if(age>=.28)return kPistolClipRest;
    const double cursor=(age-.06)/.22*5;const int key=(std::min)(4,static_cast<int>(cursor));
    return PistolRigidBlend(kPistolInsertKeys[key],kPistolInsertKeys[key+1],cursor-key);
}
