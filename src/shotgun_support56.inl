int g_m133GuideTarget56=0;
// M133/signature share one receiver-local support target. The game-side idle
// support initializes its local point once; animation cannot move it afterward.
struct M133SupportSettings56 {float position[3]={},size[3]={.12f,.18f,.12f},rotation[3]={};bool positioned=false;}g_m133Support56;
bool g_m133SupportLoaded56=false;UINT g_m133SupportEpoch56=~0u;bool g_m133SupportLearned56=false;V3 g_m133SupportIdle56={};
bool M133SupportProfile56(){return g_weaponMechanicProfile>=3&&g_weaponMechanicProfile<=6;}
float* M133SupportValue56(int n){return n<3?&g_m133Support56.position[n]:n<6?&g_m133Support56.size[n-3]:&g_m133Support56.rotation[n-6];}
void LoadM133Support56(const wchar_t* explicitPath=nullptr){
 if(g_m133SupportLoaded56&&!explicitPath)return;g_m133SupportLoaded56=true;wchar_t path[MAX_PATH];ManualSettingsPath(path);if(explicitPath)wcscpy_s(path,explicitPath);
 const wchar_t* keys[]={L"PositionX",L"PositionY",L"PositionZ",L"SizeX",L"SizeY",L"SizeZ",L"RotationX",L"RotationY",L"RotationZ"};
 g_m133Support56.positioned=GetPrivateProfileIntW(L"M133Support",L"Positioned",0,path)!=0;
 for(int n=0;n<9;++n){wchar_t text[64];GetPrivateProfileStringW(L"M133Support",keys[n],L"",text,64,path);wchar_t* end=nullptr;const float v=float(wcstod(text,&end));if(end!=text&&!*end&&std::isfinite(v)&&(n<3?fabs(v)<=2:n<6?v>=.01f&&v<=.6f:fabs(v)<=180))*M133SupportValue56(n)=v;}
}
bool SaveM133Support56(const wchar_t* explicitPath=nullptr){
 wchar_t path[MAX_PATH];ManualSettingsPath(path);if(explicitPath)wcscpy_s(path,explicitPath);const wchar_t* keys[]={L"PositionX",L"PositionY",L"PositionZ",L"SizeX",L"SizeY",L"SizeZ",L"RotationX",L"RotationY",L"RotationZ"};bool ok=true;
 for(int n=0;n<9;++n){wchar_t text[64];swprintf_s(text,L"%.8g",double(*M133SupportValue56(n)));ok=WritePrivateProfileStringW(L"M133Support",keys[n],text,path)!=FALSE&&ok;}ok=WritePrivateProfileStringW(L"M133Support",L"Positioned",g_m133Support56.positioned?L"1":L"0",path)!=FALSE&&ok;WritePrivateProfileStringW(nullptr,nullptr,nullptr,path);return ok;
}
bool M133SupportRoot56(Mat4& root){
 if(!M133SupportProfile56()||!g_reloadContact.port||g_reloadContact.portFrame>g_frame||(!g_pistolMenu.open&&g_frame-g_reloadContact.portFrame>1))return false;
 LoadM133Support56();root=g_reloadContact.portParent;Mat4 inverse={};return Invert(root,inverse);
}
Mat4 M133SupportLocal56(){
 Mat4 local=HandIdentity();for(int a=0;a<3;++a){Mat4 r=HandIdentity();const int u=(a+1)%3,v=(a+2)%3;const double t=g_m133Support56.rotation[a]*.0174532925199433;r.m[u*4+u]=r.m[v*4+v]=cos(t);r.m[u*4+v]=-sin(t);r.m[v*4+u]=sin(t);local=Multiply(local,r);}
 const auto& s=g_m133Support56;const V3 p=s.positioned?V3{s.position[0],s.position[1],s.position[2]}:g_m133SupportIdle56;local.m[3]=p.x;local.m[7]=p.y;local.m[11]=p.z;return local;
}
bool M133SupportBox56(Mat4& box){Mat4 root={};if(!M133SupportRoot56(root)||(!g_m133Support56.positioned&&(!g_m133SupportLearned56||g_m133SupportEpoch56!=g_armPoseEpoch)))return false;Mat4 local=M133SupportLocal56();const auto h=fc3hands303::Read();if(h.enabled&&!h.empty&&h.hand==1&&h.nativeHoldingSide==0){Mat4 mirror=HandIdentity();mirror.m[0]=-1;local=Multiply(mirror,Multiply(local,mirror));}const Mat4 placed=Multiply(root,local);if(!HandBasis(placed,box))return false;box.m[3]=placed.m[3];box.m[7]=placed.m[7];box.m[11]=placed.m[11];return true;}
bool M133SupportPosition56(V3& supportCamera){
 Mat4 root={},inverseBody={},inverseRoot={};if(!M133SupportRoot56(root)||!Invert(ArmBodyToCamera(),inverseBody)||!Invert(root,inverseRoot))return false;
 if(g_m133SupportEpoch56!=g_armPoseEpoch){g_m133SupportEpoch56=g_armPoseEpoch;g_m133SupportLearned56=false;}
 if(!g_m133Support56.positioned&&!g_m133SupportLearned56){
  if(!g_shotHand.leftValid||!g_shotHand.valid||g_shotHand.epoch!=g_armPoseEpoch||g_frame-g_shotHand.frame>1)return false;
  V3 contact=supportCamera;if(g_supportPalmProfile==g_weaponMechanicProfile&&g_supportPalmEpoch==g_armPoseEpoch&&g_frame-g_supportPalmFrame<=2){const auto& m=g_frameGunMove;contact=Add(contact,{m.m[0]*g_supportPalmDelta.x+m.m[1]*g_supportPalmDelta.y+m.m[2]*g_supportPalmDelta.z,m.m[4]*g_supportPalmDelta.x+m.m[5]*g_supportPalmDelta.y+m.m[6]*g_supportPalmDelta.z,m.m[8]*g_supportPalmDelta.x+m.m[9]*g_supportPalmDelta.y+m.m[10]*g_supportPalmDelta.z});}
  g_m133SupportIdle56=Apply(inverseRoot,Apply(inverseBody,contact));g_m133SupportLearned56=true;
  for(int a=0;a<3;++a)g_m133Support56.position[a]=float(a==0?g_m133SupportIdle56.x:a==1?g_m133SupportIdle56.y:g_m133SupportIdle56.z);
 }
 Mat4 box={};if(!M133SupportBox56(box))return false;supportCamera=Apply(ArmBodyToCamera(),{box.m[3],box.m[7],box.m[11]});return true;
}
double M133SupportDistance56(){
 Mat4 box={},inverse={};if(!g_handLeftValid||!M133SupportBox56(box)||!Invert(box,inverse))return 1e9;V3 palm={};if(!FreeHandPalm(1,palm))palm=ControllerWrist(g_handBody[1],1);const V3 p=Apply(inverse,palm);const double values[]={p.x,p.y,p.z},scale=(std::max)(.1f,g_worldScale);double d2=0;for(int a=0;a<3;++a){const double d=(std::max)(0.0,fabs(values[a])-g_m133Support56.size[a]*.5*scale);d2+=d*d;}return sqrt(d2);
}
bool M133SupportNear56(){
 if(!M133SupportProfile56())return false;Mat4 root={};if(!M133SupportRoot56(root))return true; // no fresh target: reserve grip, never turn intended support into ammo
 Mat4 box={};if(!M133SupportBox56(box)){
  const bool held=g_twoHand;g_twoHand=false;Mat4 move={};const bool ready=GunMove(g_armJoints[0],g_armJoints[1],move);g_twoHand=held;if(!ready)return true;
  V3 native={g_armJoints[1].w[0],g_armJoints[1].w[1],g_armJoints[1].w[2]};if(!ShotgunSupportNative(g_armJoints[0],native))return true;
  V3 support=Apply(move,native);const Mat4 old=g_frameGunMove;g_frameGunMove=move;const bool positioned=M133SupportPosition56(support);g_frameGunMove=old;if(!positioned)return true;
 }
 return M133SupportDistance56()<=kGrabPalmDiameter*.5*(std::max)(.1f,g_worldScale);
}
bool M133AimSupport56(const ArmJoints& right,const Mat4& unsupportedMove,V3& supportCamera){
 // A guide edit calibrates contact, not a new native skeleton. Keep the existing
 // idle support steering until arms and gun share one confirmed pose owner.
 // Live PID28416 showed up to39cm hand/FRAME divergence with the reconstruction.
 (void)right;(void)unsupportedMove;(void)supportCamera;return false;
}
bool M133SupportMenu56(){bool changed=false;
 LoadM133Support56();const char* labels[]={"Position X","Position Y","Position Z","Size X","Size Y","Size Z","Rotation X","Rotation Y","Rotation Z"};
 for(int n=0;n<9;++n){float v=*M133SupportValue56(n)*(n>=6?1.f:100.f);if(ImGui::SliderFloat(labels[n],&v,n>=6?-180.f:n>=3?1.f:-200.f,n>=6?180.f:n>=3?60.f:200.f,n>=6?"%.1f deg":"%.2f cm")){*M133SupportValue56(n)=v*(n>=6?1.f:.01f);g_m133Support56.positioned=true;changed=true;}}
 return changed;
}
