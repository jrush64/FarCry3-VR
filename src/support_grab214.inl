// Per-model two-hand support contact. Default values reproduce the old 15 cm
// centre reach and leave the captured support point untouched.
struct SupportGrab214 {float offset[3]={};float reach=.15f;float sizeScale[3]={1,1,1};};
SupportGrab214 g_supportGrab214[44];
bool SupportGrabEditable214(int profile){
 if(profile<0||profile>=44)return false;
 switch(profile){
 case 0:case 3:case 4:case 5:case 6:case 10:case 11:case 16:
 case 18:case 19:case 24:case 27:case 29:case 30:case 33:case 34:
 case 37:case 38:case 39:case 42:return false;
 default:return true;
 }
}
void SupportGrabSection214(int profile,wchar_t (&out)[40]){swprintf_s(out,L"SupportGrab%02d",profile);}
void LoadSupportGrab214(const wchar_t* path){
 for(int profile=0;profile<44;++profile){
  auto& s=g_supportGrab214[profile];s=SupportGrab214{};
  if(!SupportGrabEditable214(profile))continue;
  wchar_t section[40],value[64];SupportGrabSection214(profile,section);
  const wchar_t* keys[]={L"OffsetX",L"OffsetY",L"OffsetZ",L"Reach",L"SizeScaleX",L"SizeScaleY",L"SizeScaleZ"};
  for(int n=0;n<7;++n){GetPrivateProfileStringW(section,keys[n],L"",value,64,path);if(!value[0])continue;
   wchar_t* end=nullptr;const double v=wcstod(value,&end);
   if(end==value||*end||!std::isfinite(v))continue;
   if(n<3){if(std::fabs(v)<=.5)s.offset[n]=float(v);}else if(n==3){if(v>=.06&&v<=.35)s.reach=float(v);}else if(v>=.25&&v<=3)s.sizeScale[n-4]=float(v);
  }
 }
}
bool SaveSupportGrab214(int profile,const wchar_t* explicitPath=nullptr){
 if(!SupportGrabEditable214(profile))return false;
 wchar_t path[MAX_PATH],section[40];ManualSettingsPath(path);SupportGrabSection214(profile,section);
 const wchar_t* target=explicitPath?explicitPath:path;const auto& s=g_supportGrab214[profile];
 const float values[]={s.offset[0],s.offset[1],s.offset[2],s.reach,s.sizeScale[0],s.sizeScale[1],s.sizeScale[2]};
 const wchar_t* keys[]={L"OffsetX",L"OffsetY",L"OffsetZ",L"Reach",L"SizeScaleX",L"SizeScaleY",L"SizeScaleZ"};bool ok=true;
 for(int n=0;n<7;++n){wchar_t value[64];swprintf_s(value,L"%.6f",values[n]);ok=WritePrivateProfileStringW(section,keys[n],value,target)!=FALSE&&ok;}
 WritePrivateProfileStringW(nullptr,nullptr,nullptr,target);return ok;
}
double SupportGrabReach214(int profile){return SupportGrabEditable214(profile)?g_supportGrab214[profile].reach:kGrabReach;}
float SupportGrabBoxDiameter214(int profile){
 const double scale=(std::max)(.1,static_cast<double>(g_worldScale));
 return float((std::max)(.01,2*(SupportGrabReach214(profile)/scale-kGrabPalmDiameter*.5)));
}
void SupportGrabBoxSize214(int profile,float (&out)[3]){
 const float base=SupportGrabBoxDiameter214(profile);
 for(int a=0;a<3;++a)out[a]=base*(SupportGrabEditable214(profile)?g_supportGrab214[profile].sizeScale[a]:1.f);
}
bool SupportGrabBox214(const Mat4& parent,const V3& point,Mat4& box){
 box=parent;
 if(SupportGrabEditable214(g_weaponMechanicProfile)&&g_weaponMechanicProfile!=15){Mat4 root={},grip={};if(ControllerCurrentRoot124(root,grip))box=root;}
 for(int col=0;col<3;++col){double length=0;for(int row=0;row<3;++row)length+=box.m[row*4+col]*box.m[row*4+col];length=std::sqrt(length);if(!std::isfinite(length)||length<1e-8)return false;for(int row=0;row<3;++row)box.m[row*4+col]/=length;}
 box.m[3]=point.x;box.m[7]=point.y;box.m[11]=point.z;return true;
}
double SupportGrabOutside214(int profile,const Mat4& box,const V3& palm){
 Mat4 inverse={};if(!Invert(box,inverse))return 1e9;
 const V3 p=Apply(inverse,palm);const double local[]={p.x,p.y,p.z},scale=(std::max)(.1,static_cast<double>(g_worldScale));float size[3];SupportGrabBoxSize214(profile,size);
 double sum=0;for(int a=0;a<3;++a){const double d=(std::max)(0.0,std::fabs(local[a])-size[a]*scale*.5);sum+=d*d;}return std::sqrt(sum);
}
// Settings are authored in the native right-owner weapon frame. Reflect the
// lateral correction with the support contact when its owner changes sides.
V3 SupportGrabOffset350(int profile){
 const auto& stored=g_supportGrab214[profile].offset;V3 offset={stored[0],stored[1],stored[2]};const auto h=fc3hands303::ReadPresentation314();
 if(h.enabled&&h.active&&h.engaged&&!h.empty&&h.hand==1&&h.nativeHoldingSide==0){
  if((fc3hands303::MagazineRifle317(profile)&&(fc3mirror338::Supported(profile)||profile==14||profile==31))||(fc3hands303::Other319(profile)&&profile!=32))offset.x=-offset.x;
 }
 return offset;
}
void SupportGrabAdjustPoint214(V3& point){
 const int profile=g_weaponMechanicProfile;if(!SupportGrabEditable214(profile))return;
 const auto o=SupportGrabOffset350(profile);
 if(o.x==0&&o.y==0&&o.z==0)return;
 Mat4 root={},grip={};if(!ControllerCurrentRoot124(root,grip))return;
 point.x+=root.m[0]*o.x+root.m[1]*o.y+root.m[2]*o.z;
 point.y+=root.m[4]*o.x+root.m[5]*o.y+root.m[6]*o.z;
 point.z+=root.m[8]*o.x+root.m[9]*o.y+root.m[10]*o.z;
}
