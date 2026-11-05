ULONGLONG g_w7PausedAt=0;
ULONGLONG W7Now(){return g_w7PausedAt?g_w7PausedAt:GetTickCount64();}
// Per-weapon calibration; zero offsets preserve the accepted mechanics.
struct W7GuideSettings {
 float move[3][3]={},rotation[3][3]={},size[3][3]={};
 float hand[3]={-.0871f,.0409f,-.1103f},diameter=.10f,ammo[3]={},ammoRotation[3]={};
 float rocketContact[3]={},contactAnchor=0;
 float pumpMove54[3]={},pumpRotation54[3]={},pumpSize54[3]={.12f,.16f,.12f};
 float insertMove55[3]={},insertRotation55[3]={},insertSize55[3]={};bool separateInsert55=false;
 float pumpStroke54=.12f;
 float rocketSize54[3]={.12f,.12f,.12f};bool rocketBox54=false;
 Mat4 ammoWrist54={};UINT ammoEpoch54=0;bool ammoWristKnown54=false;
 bool preview=false,ammoPreview54=false,removalPreview54=false;int target=0,result=0;
};
const wchar_t* kW7Sections[4]={L"FlareGuides",L"RpgGuides",L"Gm94Guides",L"FlamerGuides"};
const char* kW7Names[4]={"Flare gun","RPG-7","GM-94","Flamethrower"};
const int kW7BoxCounts[4]={2,1,3,2};
// rpg7_rocket.xbg: 1004 vertices, rear shaft end Y=-0.35967161 m; FX_ROCKET_FIRE at Y=-0.36221114 confirms negative Y is the tail.
const float kRpgRocketTailContactY=-.35967161f;
W7GuideSettings W7Defaults(int w){W7GuideSettings s{};const float sizes[4][3][3]={{{.12f,.28f,.12f},{.10f,.10f,.10f}},{{.16f,.16f,.16f}},{{.08f,.10f,.07f},{.08f,.08f,.08f},{.08f,.08f,.08f}},{{.08f,.08f,.08f},{.12f,.12f,.12f}}};memcpy(s.size,sizes[w],sizeof(s.size));if(w==1){s.diameter=.12f;s.contactAnchor=1;s.rocketContact[1]=kRpgRocketTailContactY;}return s;}
W7GuideSettings g_w7Guides[4]={W7Defaults(0),W7Defaults(1),W7Defaults(2),W7Defaults(3)};
int W7ProfileIndex(LONG p){return p==16?0:p==32?1:p==18?2:p==15?3:-1;}
int W7MenuIndex(const char* n){return strcmp(n,"FLARE_GUN")==0?0:strcmp(n,"RPG-7")==0?1:strcmp(n,"GM-94")==0?2:strcmp(n,"LPO-50")==0?3:-1;}
float* W7Value(W7GuideSettings& s,int n){if(n<27){int b=n/9,k=n%9;return k<3?&s.move[b][k]:k<6?&s.size[b][k-3]:&s.rotation[b][k-6];}return n<30?&s.hand[n-27]:n==30?&s.diameter:n<34?&s.ammo[n-31]:n<37?&s.ammoRotation[n-34]:n<40?&s.rocketContact[n-37]:&s.contactAnchor;}
int W7Kind(int n){if(n>=37)return n==40?3:0;return n<27?(n%9<3?0:n%9<6?1:2):n==30?1:n>=34?2:0;}
void W7Key(int n,wchar_t (&key)[32]){swprintf_s(key,L"Value%d",n);}
bool W7ExtraSave54(int w,const wchar_t* path);void W7ExtraLoad54(int w,const wchar_t* path);void LoadFlamerInsert55(const wchar_t* path);
bool W7Save(int w,int only,const wchar_t* explicitPath){wchar_t path[MAX_PATH];ManualSettingsPath(path);if(explicitPath)wcscpy_s(path,explicitPath);bool ok=true;for(int n=0;n<(w==1?41:37);++n){if(only>=0&&n!=only)continue;wchar_t key[32],value[64];W7Key(n,key);swprintf_s(value,L"%.8g",static_cast<double>(*W7Value(g_w7Guides[w],n)));if(!WritePrivateProfileStringW(kW7Sections[w],key,value,path))ok=false;}if(w==1&&(only<0||only>=37)&&!WritePrivateProfileStringW(kW7Sections[1],L"RocketContactVersion",L"1",path))ok=false;if(only<0||(w==3&&g_w7Guides[3].separateInsert55&&GetPrivateProfileIntW(kW7Sections[3],L"SeparateInsert55",0,path)==0))ok=W7ExtraSave54(w,path)&&ok;return ok;}
void LoadWeapons7Guides(const wchar_t* path){for(int w=0;w<4;++w){W7ExtraLoad54(w,path);for(int n=0;n<(w==1?41:37);++n){wchar_t key[32],value[64];W7Key(n,key);GetPrivateProfileStringW(kW7Sections[w],key,L"",value,64,path);wchar_t* end=nullptr;const float f=static_cast<float>(wcstod(value,&end));const int k=W7Kind(n);if(end!=value&&!*end&&std::isfinite(f)&&(k==0?fabs(f)<=.5:k==1?f>=.01&&f<=.4:k==2?fabs(f)<=180:(f==0||f==1)))*W7Value(g_w7Guides[w],n)=f;}
 // The first rocket-local build saved origin (the neck) as its default. Migrate only that untouched legacy origin.
 // A saved version keeps an intentionally calibrated zero, nonzero custom offsets, and palm mode unchanged.
 if(w==3)LoadFlamerInsert55(path);
 if(w==1&&GetPrivateProfileIntW(kW7Sections[w],L"RocketContactVersion",0,path)<1){auto& g=g_w7Guides[w];if(g.contactAnchor>=.5f&&g.rocketContact[0]==0&&g.rocketContact[1]==0&&g.rocketContact[2]==0)g.rocketContact[1]=kRpgRocketTailContactY;}
 }}
Mat4 W7Transform(Mat4 base,const float* move,const float* rotation){const V3 at=Apply(base,{move[0],move[1],move[2]});for(int axis=0;axis<3;++axis){Mat4 r=HandIdentity();const int p=(axis+1)%3,q=(axis+2)%3;const double a=rotation[axis]*.0174532925199433;r.m[p*4+p]=r.m[q*4+q]=cos(a);r.m[p*4+q]=sin(a);r.m[q*4+p]=-sin(a);base=Multiply(base,r);}base.m[3]=at.x;base.m[7]=at.y;base.m[11]=at.z;return base;}
Mat4 W7Adjust(int w,int b,Mat4 base){return W7Transform(base,g_w7Guides[w].move[b],g_w7Guides[w].rotation[b]);}
Mat4 W7Ammo(int w,Mat4 base){return W7Transform(base,g_w7Guides[w].ammo,g_w7Guides[w].ammoRotation);}

// Additive named keys leave every existing Value0..40 calibration untouched.
bool W7ExtraSave54(int w,const wchar_t* path){
 bool ok=true;auto& s=g_w7Guides[w];const wchar_t* names[]={L"PumpOffset",L"PumpRotation",L"PumpSize",L"RocketSize"};
 for(int a=0;a<3;++a)for(int group=0;group<4;++group){if((group<3&&w!=2)||(group==3&&w!=1))continue;wchar_t key[64],value[64];swprintf_s(key,L"%s%c54",names[group],L'X'+a);const float* v=group==0?s.pumpMove54:group==1?s.pumpRotation54:group==2?s.pumpSize54:s.rocketSize54;swprintf_s(value,L"%.8g",double(v[a]));ok=(WritePrivateProfileStringW(kW7Sections[w],key,value,path)!=FALSE)&&ok;}
 if(w==3&&s.separateInsert55){const wchar_t* names55[]={L"InsertOffset",L"InsertRotation",L"InsertSize"};for(int group=0;group<3;++group)for(int a=0;a<3;++a){wchar_t key[64],value[32];swprintf_s(key,L"%s%c55",names55[group],L'X'+a);const float* v=group==0?s.insertMove55:group==1?s.insertRotation55:s.insertSize55;swprintf_s(value,L"%.8g",double(v[a]));ok=(WritePrivateProfileStringW(kW7Sections[w],key,value,path)!=FALSE)&&ok;}ok=(WritePrivateProfileStringW(kW7Sections[w],L"SeparateInsert55",L"1",path)!=FALSE)&&ok;}
 if(w==2){wchar_t v[32];swprintf_s(v,L"%.8g",double(s.pumpStroke54));ok=(WritePrivateProfileStringW(kW7Sections[w],L"PumpStroke54",v,path)!=FALSE)&&ok;}
 if(w==1)ok=(WritePrivateProfileStringW(kW7Sections[w],L"RocketBox54",s.rocketBox54?L"1":L"0",path)!=FALSE)&&ok;return ok;
}
void W7ExtraLoad54(int w,const wchar_t* path){
 auto& s=g_w7Guides[w];const wchar_t* names[]={L"PumpOffset",L"PumpRotation",L"PumpSize",L"RocketSize"};
 for(int a=0;a<3;++a)for(int group=0;group<4;++group){if((group<3&&w!=2)||(group==3&&w!=1))continue;wchar_t key[64],value[64];swprintf_s(key,L"%s%c54",names[group],L'X'+a);GetPrivateProfileStringW(kW7Sections[w],key,L"",value,64,path);wchar_t* end=nullptr;const float f=float(wcstod(value,&end));if(end==value||*end||!std::isfinite(f)||(group==1?fabs(f)>180:group>=2?(f<.01f||f>.4f):fabs(f)>.5f))continue;float* v=group==0?s.pumpMove54:group==1?s.pumpRotation54:group==2?s.pumpSize54:s.rocketSize54;v[a]=f;}
 if(w==2){wchar_t v[32];GetPrivateProfileStringW(kW7Sections[w],L"PumpStroke54",L"0.12",v,32,path);wchar_t* end=nullptr;const float f=float(wcstod(v,&end));if(end!=v&&!*end&&std::isfinite(f)&&f>=.05f&&f<=.25f)s.pumpStroke54=f;}
 if(w==1)s.rocketBox54=GetPrivateProfileIntW(kW7Sections[w],L"RocketBox54",0,path)!=0;
}
bool W7AmmoPreview54(int w){return g_pistolMenu.open&&W7ProfileIndex(g_weaponMechanicProfile)==w&&g_w7Guides[w].ammoPreview54;}

bool W7PreviewWristReady54(int w){const auto& s=g_w7Guides[w];return W7AmmoPreview54(w)&&s.ammoWristKnown54&&s.ammoEpoch54==g_armPoseEpoch;}
Mat4 W7VisualWrist54(int w,const Mat4& live){return W7PreviewWristReady54(w)?g_w7Guides[w].ammoWrist54:live;}

void EnsureFlamerInsert55(){auto& s=g_w7Guides[3];if(s.separateInsert55)return;memcpy(s.insertMove55,s.move[1],sizeof(s.insertMove55));memcpy(s.insertRotation55,s.rotation[1],sizeof(s.insertRotation55));memcpy(s.insertSize55,s.size[1],sizeof(s.insertSize55));s.separateInsert55=true;}
void LoadFlamerInsert55(const wchar_t* path){auto& s=g_w7Guides[3];s.separateInsert55=false;if(!GetPrivateProfileIntW(kW7Sections[3],L"SeparateInsert55",0,path))return;EnsureFlamerInsert55();const wchar_t* names[]={L"InsertOffset",L"InsertRotation",L"InsertSize"};for(int group=0;group<3;++group)for(int a=0;a<3;++a){wchar_t key[64],text[64];swprintf_s(key,L"%s%c55",names[group],L'X'+a);GetPrivateProfileStringW(kW7Sections[3],key,L"",text,64,path);wchar_t* end=nullptr;const float v=float(wcstod(text,&end));if(end==text||*end||!std::isfinite(v)||(group==1?fabs(v)>180:group==2?(v<.01||v>.4):fabs(v)>.5))continue;float* dest=group==0?s.insertMove55:group==1?s.insertRotation55:s.insertSize55;dest[a]=v;}}
const float* FlamerInsertSize55(){const auto& s=g_w7Guides[3];return s.separateInsert55?s.insertSize55:s.size[1];}
Mat4 FlamerInsertAdjust55(Mat4 base){const auto& s=g_w7Guides[3];return s.separateInsert55?W7Transform(base,s.insertMove55,s.insertRotation55):W7Adjust(3,1,base);}
