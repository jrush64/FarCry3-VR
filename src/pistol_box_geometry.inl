int PistolEffectiveGuideTarget(int target){return target==1&&g_weaponMechanicProfile==21?0:target;}
const float* PistolGuidePoint(int target){target=PistolEffectiveGuideTarget(target);if(A2000LeftCharge336()){static float mirrored[3];for(int i=0;i<3;++i)mirrored[i]=(target==3?g_pistolGuides.charge:target==1?g_pistolGuides.slide:g_pistolGuides.port)[i]*(i==0?-1.f:1.f);return mirrored;}return target==3?g_pistolGuides.charge:target==1?g_pistolGuides.slide:g_pistolGuides.port;}
int PistolBoxIndex(int target){target=PistolEffectiveGuideTarget(target);return target==3?2:target;}
Mat4 PistolGuideBox(const Mat4& source,int target)
{
 Mat4 root=PistolInteractionRoot(source);if((target==0||g_weaponMechanicProfile==20)&&!Resolve1911Insertion(source,root,g_pistolMenu.open))return {};
 Mat4 basis=root;for(int c=0;c<3;++c){double len=0;for(int r=0;r<3;++r)len+=basis.m[r*4+c]*basis.m[r*4+c];len=std::sqrt(len);if(len<1e-8)return {};for(int r=0;r<3;++r)basis.m[r*4+c]/=len;}
 const float* angles=g_pistolGuides.angles[PistolBoxIndex(target)];
 for(int axis=0;axis<3;++axis){Mat4 rot={};rot.m[0]=rot.m[5]=rot.m[10]=rot.m[15]=1;const int a=(axis+1)%3,b=(axis+2)%3;double t=angles[axis]*.0174532925199433;if(A2000LeftCharge336()&&axis!=0)t=-t;rot.m[a*4+a]=rot.m[b*4+b]=std::cos(t);rot.m[a*4+b]=std::sin(t);rot.m[b*4+a]=-std::sin(t);basis=Multiply(basis,rot);}
 const float* point=PistolGuidePoint(target);V3 center=Apply(root,{point[0],point[1],point[2]});basis.m[3]=center.x;basis.m[7]=center.y;basis.m[11]=center.z;return basis;
}
bool PistolGuideContact(const Mat4& source,int target,V3 point)
{
 Mat4 root=PistolInteractionRoot(source);if((target==0||g_weaponMechanicProfile==20)&&!Resolve1911Insertion(source,root,false))return false;
 target=PistolEffectiveGuideTarget(target);
 const float scale=(std::max)(.1f,g_worldScale);
 const double handRadius=g_pistolGuides.handDiameter*.5*scale;
 if(!g_pistolGuides.box&&target!=3){const float* c=PistolGuidePoint(target);return Length(Sub(point,Apply(root,{c[0],c[1],c[2]})))<=(target?g_pistolGuides.slideRadius:g_pistolGuides.portRadius)*scale+handRadius;}
 Mat4 inverse={};if(!Invert(PistolGuideBox(root,target),inverse))return false;V3 local=Apply(inverse,point);
 // Sphere versus OBB: closest point on the box, including edges and corners.
 double distance2=0;const double v[3]={local.x,local.y,local.z};
 for(int a=0;a<3;++a){const double outside=(std::max)(0.0,std::fabs(v[a])-g_pistolGuides.size[PistolBoxIndex(target)][a]*scale*.5);distance2+=outside*outside;}
 return distance2<=handRadius*handRadius;
}
void PistolBoxMarker(const Mat4& root,int target,int contact)
{
 if(!g_pistolGuides.box&&target!=3)return;
 if(NativeGuideScope50()){fc3xr::HideReloadWeaponMarker();NativeWeaponBoxRequest50(root,target);return;}
 Mat4 box=PistolGuideBox(root,target);float transform[16],size[3];for(int i=0;i<16;++i)transform[i]=static_cast<float>(box.m[i]);for(int i=0;i<3;++i)size[i]=g_pistolGuides.size[PistolBoxIndex(target)][i]*(std::max)(.1f,g_worldScale);fc3xr::SetWeaponBox(transform,size,contact);
}
