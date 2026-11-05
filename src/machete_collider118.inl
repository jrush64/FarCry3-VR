// Blade-local capsule. Graphics, preview and collision use the same endpoints.
struct MacheteColliderSettings118
{
    float offset[3]={},rotation[3]={};
    float length=.345f,radius=.025f,minSpeed=.5f,damageScale=1.f,cooldownMs=350.f;
    bool show=true,haptics=true;
}g_macheteCollider118;
const wchar_t* kMacheteColliderKeys118[]={L"ColliderX",L"ColliderY",L"ColliderZ",L"ColliderRotX",L"ColliderRotY",L"ColliderRotZ",L"ColliderLength",L"ColliderRadius",L"ColliderSpeed",L"ColliderDamage",L"ColliderCooldownMs"};
float& MacheteColliderField118(MacheteColliderSettings118& s,int n){return n<3?s.offset[n]:n<6?s.rotation[n-3]:n==6?s.length:n==7?s.radius:n==8?s.minSpeed:n==9?s.damageScale:s.cooldownMs;}
bool ValidMacheteColliderField118(int n,double v)
{
    if(!std::isfinite(v))return false;
    return n<3?fabs(v)<=1:n<6?fabs(v)<=180:n==6?v>=.05&&v<=1.2:n==7?v>=.005&&v<=.2:n==8?v>=0&&v<=5:n==9?v>=.1&&v<=5:v>=50&&v<=2000;
}
void LoadMacheteCollider118(const wchar_t* path)
{
    MacheteColliderSettings118 s;
    for(int n=0;n<11;++n){wchar_t text[64]={};GetPrivateProfileStringW(L"Machete",kMacheteColliderKeys118[n],L"",text,64,path);if(!text[0])continue;wchar_t* end=nullptr;double value=wcstod(text,&end);if(end&&!*end&&ValidMacheteColliderField118(n,value))MacheteColliderField118(s,n)=float(value);}
    s.haptics=GetPrivateProfileIntW(L"Machete",L"HitHaptics",1,path)!=0;
    s.show=GetPrivateProfileIntW(L"Machete",L"ShowCollider",1,path)!=0;g_macheteCollider118=s;
}
bool SaveMacheteCollider118(const wchar_t* path,int field=-1)
{
    bool ok=true;for(int n=0;n<11;++n)if(field<0||field==n){float value=MacheteColliderField118(g_macheteCollider118,n);if(!ValidMacheteColliderField118(n,value)){ok=false;continue;}wchar_t text[64];swprintf_s(text,L"%.6f",value);ok=(WritePrivateProfileStringW(L"Machete",kMacheteColliderKeys118[n],text,path)!=0)&&ok;}
    if(field<0)ok=(WritePrivateProfileStringW(L"Machete",L"HitHaptics",g_macheteCollider118.haptics?L"1":L"0",path)!=0)&&ok;
    if(field<0)ok=(WritePrivateProfileStringW(L"Machete",L"ShowCollider",g_macheteCollider118.show?L"1":L"0",path)!=0)&&ok;return ok;
}
Mat4 MacheteColliderLocal118(const MacheteColliderSettings118& s)
{
    V3 base=g_machete.blade>=0?g_machete.bladeBase:V3{.026123844,.002990814,.083468123};
    V3 tip=g_machete.blade>=0?g_machete.bladeTip:V3{-.007904294,.002990814,.428632465};
    Mat4 basis=TurnAndMove({0,0,1},Sub(tip,base),{0,0,0},{0,0,0});
    for(int axis=0;axis<3;++axis){Mat4 rot=HandIdentity();const int a=(axis+1)%3,b=(axis+2)%3;const double t=s.rotation[axis]*.0174532925199433;rot.m[a*4+a]=rot.m[b*4+b]=cos(t);rot.m[a*4+b]=sin(t);rot.m[b*4+a]=-sin(t);basis=Multiply(basis,rot);}
    V3 center=Scale(Add(base,tip),.5);basis.m[3]=center.x+s.offset[0];basis.m[7]=center.y+s.offset[1];basis.m[11]=center.z+s.offset[2];return basis;
}
double MacheteColliderHalf118(const MacheteColliderSettings118& s){return (std::max)(0.0,double(s.length)*.5-s.radius);}
struct MacheteColliderPose118
{
    ULONGLONG ms=0;UINT epoch=0,flags=0;float scale=1;
    V3 base={},tip={},bodyBase={},bodyTip={};
    float radius=0,minSpeed=0,damageScale=1;UINT cooldownMs=350;
}g_macheteColliderPose118;
void SnapshotMacheteCollider118(const MacheteQuickPose& pose,UINT flags)
{
    MacheteColliderPose118 out;out.ms=pose.ms;out.epoch=pose.epoch;out.flags=flags;out.scale=pose.scale;
    const auto& s=g_macheteCollider118;out.radius=s.radius;out.minSpeed=s.minSpeed;out.damageScale=s.damageScale;out.cooldownMs=UINT(s.cooldownMs);
    Mat4 hand={},controller={},camera=HandIdentity();for(int i=0;i<16;++i){hand.m[i]=pose.hand[i];controller.m[i]=pose.controllerBody[i];}
    for(int r=0;r<3;++r){camera.m[r*4+3]=pose.cameraPosition[r];for(int c=0;c<3;++c)camera.m[r*4+c]=pose.cameraBasis[c*3+r];}
    const Mat4 local=Multiply(kMacheteFromHand[0],MacheteColliderLocal118(s));
    const Mat4 world=Multiply(camera,Multiply(hand,local)),body=Multiply(controller,Multiply(g_machete.controllerHand116,local));
    const double half=MacheteColliderHalf118(s);out.base=Apply(world,{0,0,-half});out.tip=Apply(world,{0,0,half});out.bodyBase=Apply(body,{0,0,-half});out.bodyTip=Apply(body,{0,0,half});
    if(!g_machete.handAttachment116||g_machete.handEpoch116!=pose.epoch||!std::isfinite(Length(out.base))||!std::isfinite(Length(out.tip)))out.flags=0;
    g_macheteColliderPose118=out; // caller holds the snapshot lock
}
// Split along the blade so a ground contact at its bottom cannot suppress a
// separate upper-blade/enemy contact. All four casts stay inside the capsule.
void MacheteColliderSlice118(const MacheteColliderPose118& previous,const MacheteColliderPose118& current,int slice,V3& start,V3& end,V3& axis,float& half)
{
    const double t=(slice+.5)/4.;start=Add(previous.base,Scale(Sub(previous.tip,previous.base),t));end=Add(current.base,Scale(Sub(current.tip,current.base),t));
    const V3 direction=Sub(current.tip,current.base);const double len=Length(direction);axis=len>1e-6?Scale(direction,1/len):V3{0,0,1};half=float(len/8.);
}
bool MacheteColliderMotion118(const MacheteColliderPose118& previous,const MacheteColliderPose118& current,double& speed)
{
    speed=0;if(current.ms<=previous.ms||current.ms-previous.ms<4||current.ms-previous.ms>120||current.epoch!=previous.epoch||current.scale!=previous.scale||current.scale<.01f)return false;
    const double bodyTravel=(std::max)(Length(Sub(current.bodyBase,previous.bodyBase)),Length(Sub(current.bodyTip,previous.bodyTip)));
    const double worldTravel=(std::max)(Length(Sub(current.base,previous.base)),Length(Sub(current.tip,previous.tip)));
    if(!std::isfinite(bodyTravel)||!std::isfinite(worldTravel)||bodyTravel>.6*current.scale||worldTravel>1)return false;
    speed=bodyTravel*1000/(current.ms-previous.ms)/current.scale;return speed>=current.minSpeed;
}
