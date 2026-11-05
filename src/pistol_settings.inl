struct PistolGuideSettings
{
    float port[3]={.00114438f,-.01311607f,-.065f};
    float slide[3]={0,.06f,.08f};
    float charge[3]={.045f,.31807f,.112254f}; // AK right-side handle, FRAME local
    float hand[3]={-.0871f,.0409f,-.1103f}; // offset from the wrist point to the PALM, wrist axes (default = the AK palm)
    float portRadius=.07f,slideRadius=.08f,handDiameter=.10f;
    int ammoPickup=2;float replacementDelay=1.f,catchRadius=.15f,tossHeight=1.f;
    bool pullAssist286=false;float pullRange286=2.5f,pullDistance286=.12f;
    bool preview=false;
    bool ammoPreview=false; // session-only visual calibration, never inventory state
    float ammoPosition[3]={},ammoRotation[3]={};
#ifdef FC3_AMMO_TEST
    bool box=false;
#else
    bool box=true;
#endif
    float size[3][3]={{.14f,.14f,.14f},{.16f,.16f,.16f},{.09f,.10f,.09f}};
    float angles[3][3]={};
};
extern volatile LONG g_weaponMechanicProfile;
PistolGuideSettings g_magazineGuides[20];
bool g_hideGameplayGuides=false;
bool g_grabGuides=true; // [VRGuides] GrabGuides: two-hand grab gizmos on every weapon
bool g_showWeaponReloadGuides=true;
bool PhysicalReloadGuidesEnabled();
int g_guideLoadFamily=-1,g_menuGuideFamily=0;
int MagazineGuideFamily();
#define g_pistolGuides (g_magazineGuides[MagazineGuideFamily()])
// Families: 0 1911/6P9 (shared skeleton), 1 Skorpion, 2 AK, 3 A2000, 4 STG-90, 5 MP5, 6 FAMAS F1, 7 BZ19, 8 P416, 9 MK16,
// 10 Desert Eagle D50 (a PISTOL family: RifleFamily() is false for it).
bool RifleProfile(LONG p){return p==8||p==7||p==41||p==28||p==14||p==9||p==31||p==25||p==21||p==17||p==35||p==2||p==36||p==22||p==23||p==43;}
bool RifleFamily(int f){return f>=2&&f!=10;}
bool MagazineProfile(LONG p){return p==20||p==1||p==12||p==39||RifleProfile(p);}
int MagazineFamilyOfProfile(LONG p){return p==8?2:p==39?1:p==7?3:p==41?4:p==28?5:p==14?6:p==9?7:p==31?8:p==25?9:p==12?10:p==21?11:p==17?12:p==35?13:p==2?14:p==36?15:p==22?16:p==23?17:p==43?18:0;}
const wchar_t* MagazineSettingsSection(){const int f=MagazineGuideFamily();return f==18?L"PkmReload":f==17?L"Z93Reload":f==16?L"MkgReload":f==15?L"U100Reload":f==14?L"R700Reload":f==13?L"SvdReload":f==12?L"AceReload":f==11?L"VectorReload":f==10?L"D50Reload":f==9?L"Mk16Reload":f==8?L"P416Reload":f==7?L"Bz19Reload":f==6?L"FamasReload":f==5?L"Mp5Reload":f==4?L"Stg90Reload":f==3?L"A2000Reload":f==2?L"AkReload":f==1?L"SkorpionReload":L"PistolReload";}
struct PistolGuideMenu
{
    bool open=false,chord=false,a=false,b=false,x=false,y=false,place=false;
    int target=0,result=0;ULONGLONG tick=0;
} g_pistolMenu;
int MagazineGuideFamily(){return g_guideLoadFamily>=0?g_guideLoadFamily:(g_pistolMenu.open?g_menuGuideFamily:MagazineFamilyOfProfile(g_weaponMechanicProfile));}
float PistolSetting(const wchar_t* path,const wchar_t* key,float fallback)
{
    wchar_t text[64]={};GetPrivateProfileStringW(MagazineSettingsSection(),key,L"",text,64,path);
    wchar_t* end=nullptr;const float value=static_cast<float>(wcstod(text,&end));
    return end==text||*end||!std::isfinite(value)?fallback:value;
}
void LoadPistolGuides(const wchar_t* explicitPath=nullptr)
{
    wchar_t path[MAX_PATH];ManualSettingsPath(path);if(explicitPath)wcscpy_s(path,explicitPath);
    const wchar_t* keys[3][3]={{L"PortX",L"PortY",L"PortZ"},{L"SlideX",L"SlideY",L"SlideZ"},{L"HandX",L"HandY",L"HandZ"}};
    float* points[]={g_pistolGuides.port,g_pistolGuides.slide,g_pistolGuides.hand};
    for(int p=0;p<3;++p)for(int i=0;i<3;++i){if(p==2&&PistolSetting(path,L"HandSpace",1)!=2)continue;float v=PistolSetting(path,keys[p][i],points[p][i]);if(std::fabs(v)<=.5f)points[p][i]=v;}
    float v=PistolSetting(path,L"PortRadius",g_pistolGuides.portRadius);if(v>=.015f&&v<=.15f)g_pistolGuides.portRadius=v;
    v=PistolSetting(path,L"SlideRadius",g_pistolGuides.slideRadius);if(v>=.015f&&v<=.15f)g_pistolGuides.slideRadius=v;
    v=PistolSetting(path,L"HandDiameter",g_pistolGuides.handDiameter);if(v>=.01f&&v<=.20f)g_pistolGuides.handDiameter=v; // 10 cm max was too small
    g_pistolGuides.ammoPickup=(std::max)(0,(std::min)(2,static_cast<int>(PistolSetting(path,L"AmmoPickup",2))));
    g_pistolGuides.replacementDelay=(std::max)(0.f,(std::min)(10.f,PistolSetting(path,L"ReplacementDelay",1)));
    g_pistolGuides.catchRadius=(std::max)(.05f,(std::min)(.3f,PistolSetting(path,L"CatchRadius",.15f)));
    g_pistolGuides.tossHeight=(std::max)(.5f,(std::min)(2.5f,PistolSetting(path,L"TossHeight",1)));
    g_pistolGuides.pullAssist286=PistolSetting(path,L"PointPull",0)!=0;
    g_pistolGuides.pullRange286=(std::max)(.5f,(std::min)(3.f,PistolSetting(path,L"PointPullRange",2.5f)));
    g_pistolGuides.pullDistance286=(std::max)(.08f,(std::min)(.3f,PistolSetting(path,L"PointPullDistance",.12f)));
    g_pistolGuides.preview=PistolSetting(path,L"ShowGuides",0)!=0;
    g_pistolGuides.box=PistolSetting(path,L"BoxGuides",1)!=0;
    const wchar_t* ammoKeys[2][3]={{L"AmmoPosX",L"AmmoPosY",L"AmmoPosZ"},{L"AmmoRotX",L"AmmoRotY",L"AmmoRotZ"}};
    for(int i=0;i<3;++i){float v=PistolSetting(path,ammoKeys[0][i],0);g_pistolGuides.ammoPosition[i]=std::fabs(v)<=.5f?v:0;v=PistolSetting(path,ammoKeys[1][i],0);g_pistolGuides.ammoRotation[i]=std::fabs(v)<=180?v:0;}
    const wchar_t* dims[2][3]={{L"PortWidth",L"PortLength",L"PortHeight"},{L"SlideWidth",L"SlideLength",L"SlideHeight"}};
    const wchar_t* angles[2][3]={{L"PortRotX",L"PortRotY",L"PortRotZ"},{L"SlideRotX",L"SlideRotY",L"SlideRotZ"}};
    for(int t=0;t<2;++t)for(int i=0;i<3;++i){
      float v=PistolSetting(path,dims[t][i],2*(t?g_pistolGuides.slideRadius:g_pistolGuides.portRadius));
      g_pistolGuides.size[t][i]=v>=.01f&&v<=.4f?v:.14f;
      v=PistolSetting(path,angles[t][i],0);g_pistolGuides.angles[t][i]=std::fabs(v)<=180?v:0;
    }
    const wchar_t* chargeKeys[]={L"ChargeX",L"ChargeY",L"ChargeZ",L"ChargeWidth",L"ChargeLength",L"ChargeHeight",L"ChargeRotX",L"ChargeRotY",L"ChargeRotZ"};
    for(int i=0;i<9;++i){float* out=i<3?&g_pistolGuides.charge[i]:i<6?&g_pistolGuides.size[2][i-3]:&g_pistolGuides.angles[2][i-6];float v=PistolSetting(path,chargeKeys[i],*out);if(i<3?std::fabs(v)<=.5f:i<6?v>=.01f&&v<=.4f:std::fabs(v)<=180)*out=v;}
}
float* PistolGuideValue(int);
bool SavePistolGuideValue(int,const wchar_t*);
bool SavePistolGuides(const wchar_t* explicitPath=nullptr)
{
    wchar_t path[MAX_PATH];ManualSettingsPath(path);const wchar_t* target=explicitPath?explicitPath:path;
    const wchar_t* keys[]={L"PortX",L"PortY",L"PortZ",L"SlideX",L"SlideY",L"SlideZ",L"HandX",L"HandY",L"HandZ",L"PortRadius",L"SlideRadius",L"HandDiameter",L"ShowGuides"};
    const float values[]={g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2],g_pistolGuides.slide[0],g_pistolGuides.slide[1],g_pistolGuides.slide[2],g_pistolGuides.hand[0],g_pistolGuides.hand[1],g_pistolGuides.hand[2],g_pistolGuides.portRadius,g_pistolGuides.slideRadius,g_pistolGuides.handDiameter,g_pistolGuides.preview?1.f:0.f};
    bool ok=true;for(int i=0;i<13;++i){wchar_t text[64];_snwprintf_s(text,64,_TRUNCATE,L"%.6f",values[i]);ok=WritePrivateProfileStringW(MagazineSettingsSection(),keys[i],text,target)!=FALSE&&ok;}
    const wchar_t* ammoKeys[]={L"AmmoPickup",L"ReplacementDelay",L"CatchRadius",L"TossHeight"};
    const float ammoValues[]={static_cast<float>(g_pistolGuides.ammoPickup),g_pistolGuides.replacementDelay,g_pistolGuides.catchRadius,g_pistolGuides.tossHeight};
    for(int i=0;i<4;++i){wchar_t text[64];_snwprintf_s(text,64,_TRUNCATE,L"%.6f",ammoValues[i]);ok=WritePrivateProfileStringW(MagazineSettingsSection(),ammoKeys[i],text,target)!=FALSE&&ok;}
    const wchar_t* pullKeys[]={L"PointPull",L"PointPullRange",L"PointPullDistance"};
    const float pullValues[]={g_pistolGuides.pullAssist286?1.f:0.f,g_pistolGuides.pullRange286,g_pistolGuides.pullDistance286};
    for(int i=0;i<3;++i){wchar_t text[64];swprintf_s(text,L"%.6f",pullValues[i]);ok=(WritePrivateProfileStringW(MagazineSettingsSection(),pullKeys[i],text,target)!=FALSE)&&ok;}
    for(int n=12;n<39;++n)ok=SavePistolGuideValue(n,target)&&ok;
    ok=WritePrivateProfileStringW(MagazineSettingsSection(),L"BoxGuides",g_pistolGuides.box?L"1":L"0",target)!=FALSE&&ok;
    ok=WritePrivateProfileStringW(MagazineSettingsSection(),L"HandSpace",L"2",target)!=FALSE&&ok;WritePrivateProfileStringW(nullptr,nullptr,nullptr,target);return ok;
}

const wchar_t* kPistolGuideKeys[]={L"PortX",L"PortY",L"PortZ",L"SlideX",L"SlideY",L"SlideZ",L"HandX",L"HandY",L"HandZ",L"PortRadius",L"SlideRadius",L"HandDiameter",L"PortWidth",L"PortLength",L"PortHeight",L"SlideWidth",L"SlideLength",L"SlideHeight",L"PortRotX",L"PortRotY",L"PortRotZ",L"SlideRotX",L"SlideRotY",L"SlideRotZ",L"AmmoPosX",L"AmmoPosY",L"AmmoPosZ",L"AmmoRotX",L"AmmoRotY",L"AmmoRotZ",L"ChargeX",L"ChargeY",L"ChargeZ",L"ChargeWidth",L"ChargeLength",L"ChargeHeight",L"ChargeRotX",L"ChargeRotY",L"ChargeRotZ"};
float* PistolGuideValue(int n)
{if(n<0||n>=39)return nullptr;if(n>=36)return &g_pistolGuides.angles[2][n-36];if(n>=33)return &g_pistolGuides.size[2][n-33];if(n>=30)return &g_pistolGuides.charge[n-30];if(n>=27)return &g_pistolGuides.ammoRotation[n-27];if(n>=24)return &g_pistolGuides.ammoPosition[n-24];if(n>=18)return &g_pistolGuides.angles[(n-18)/3][(n-18)%3];if(n>=12)return &g_pistolGuides.size[(n-12)/3][(n-12)%3];if(n<3)return &g_pistolGuides.port[n];if(n<6)return &g_pistolGuides.slide[n-3];if(n<9)return &g_pistolGuides.hand[n-6];return n==9?&g_pistolGuides.portRadius:n==10?&g_pistolGuides.slideRadius:&g_pistolGuides.handDiameter;}
float PistolGuideDefault(int n)
{PistolGuideSettings d={};if(MagazineGuideFamily()==1){d.port[0]=0;d.port[1]=.155034f;d.port[2]=-.055f;d.slide[0]=.026f;d.slide[1]=.14320966f;d.slide[2]=.04219692f;}if(MagazineGuideFamily()==2){d.port[0]=0;d.port[1]=.27f;d.port[2]=-.06f;d.slide[0]=0;d.slide[1]=.27f;d.slide[2]=-.06f;}if(MagazineGuideFamily()==3){d.port[0]=d.slide[0]=0;d.port[1]=d.slide[1]=.1193f;d.port[2]=d.slide[2]=-.1152f;d.charge[0]=-.03f;d.charge[1]=.0983f;d.charge[2]=.0887f;}if(MagazineGuideFamily()==4){d.port[0]=d.slide[0]=0;d.port[1]=d.slide[1]=.1774f;d.port[2]=d.slide[2]=-.1666f;}if(MagazineGuideFamily()==5){d.port[0]=d.slide[0]=-.0577f;d.port[1]=d.slide[1]=.1273f;d.port[2]=d.slide[2]=-.1606f;d.charge[0]=-.03f;d.charge[1]=.25f;d.charge[2]=.07f;}if(MagazineGuideFamily()==7){d.port[0]=d.slide[0]=-.0893f;d.port[1]=d.slide[1]=.1258f;d.port[2]=d.slide[2]=-.0100f;d.charge[0]=-.1243f;d.charge[1]=-.0637f;d.charge[2]=.0194f;}if(MagazineGuideFamily()==8){d.port[0]=d.slide[0]=-.0454f;d.port[1]=d.slide[1]=.0793f;d.port[2]=d.slide[2]=-.1459f;d.charge[0]=-.0465f;d.charge[1]=-.1573f;d.charge[2]=.1247f;}if(MagazineGuideFamily()==9){d.port[0]=d.slide[0]=-.0371f;d.port[1]=d.slide[1]=.0957f;d.port[2]=d.slide[2]=-.1178f;d.charge[0]=-.1037f;d.charge[1]=.0118f;d.charge[2]=-.0510f;}if(MagazineGuideFamily()==10){d.port[0]=-.0228f;d.port[1]=-.0429f;d.port[2]=-.0206f;d.slide[0]=-.0829f;d.slide[1]=.0056f;d.slide[2]=.0982f;}if(MagazineGuideFamily()==18){d.port[0]=-.1150f;d.port[1]=.1246f;d.port[2]=-.0581f;d.slide[0]=-.1155f;d.slide[1]=.1164f;d.slide[2]=-.0578f;}if(MagazineGuideFamily()==17){d.port[0]=d.slide[0]=0;d.port[1]=d.slide[1]=.1747f;d.port[2]=d.slide[2]=.0894f;d.charge[0]=.0544f;d.charge[1]=.1087f;d.charge[2]=.0978f;d.size[2][0]=.0889f;d.size[2][1]=.0603f;d.size[2][2]=.1606f;}if(MagazineGuideFamily()==15){d.port[0]=d.slide[0]=-.1199f;d.port[1]=d.slide[1]=.0218f;d.port[2]=d.slide[2]=-.0778f;d.charge[0]=-.0829f;d.charge[1]=.0381f;d.charge[2]=.0139f;}if(MagazineGuideFamily()==16){d.port[0]=d.slide[0]=-.1775f;d.port[1]=d.slide[1]=-.3223f;d.port[2]=d.slide[2]=-.0860f;d.charge[0]=-.0933f;d.charge[1]=-.3278f;d.charge[2]=.1159f;}if(MagazineGuideFamily()==14){d.port[0]=d.slide[0]=0;d.port[1]=d.slide[1]=.2782f;d.port[2]=d.slide[2]=.0631f;d.charge[0]=-.0245f;d.charge[1]=.2388f;d.charge[2]=.0497f;d.size[2][0]=.178f;d.size[2][1]=.080f;d.size[2][2]=.132f;}if(MagazineGuideFamily()==11){d.port[0]=d.slide[0]=-.0257f;d.port[1]=d.slide[1]=.0271f;d.port[2]=d.slide[2]=-.2016f;d.charge[0]=-.1060f;d.charge[1]=.0240f;d.charge[2]=.0438f;}if(MagazineGuideFamily()==12){d.port[0]=d.slide[0]=-.0810f;d.port[1]=d.slide[1]=.0964f;d.port[2]=d.slide[2]=-.0785f;d.charge[0]=-.0998f;d.charge[1]=.0859f;d.charge[2]=.0906f;}if(MagazineGuideFamily()==13){d.port[0]=d.slide[0]=-.0804f;d.port[1]=d.slide[1]=.1677f;d.port[2]=d.slide[2]=-.0883f;d.charge[0]=-.0486f;d.charge[1]=.2197f;d.charge[2]=-.0173f;}if(MagazineGuideFamily()==6){d.port[0]=d.slide[0]=-.0051f;d.port[1]=d.slide[1]=-.1866f;d.port[2]=d.slide[2]=-.1679f;d.charge[0]=-.1466f;d.charge[1]=.0175f;d.charge[2]=.0614f;}if(n>=36)return 0;if(n>=33)return d.size[2][n-33];if(n>=30)return d.charge[n-30];if(n>=18)return 0;if(n>=12)return d.size[(n-12)/3][(n-12)%3];return n<3?d.port[n]:n<6?d.slide[n-3]:n<9?d.hand[n-6]:n==9?d.portRadius:n==10?d.slideRadius:d.handDiameter;}
bool SavePistolGuideValue(int n,const wchar_t* explicitPath=nullptr)
{float* value=PistolGuideValue(n);if(!value||!std::isfinite(*value))return false;
 wchar_t path[MAX_PATH];ManualSettingsPath(path);if(explicitPath)wcscpy_s(path,explicitPath);
 wchar_t text[64];_snwprintf_s(text,64,_TRUNCATE,L"%.6f",*value);
 bool ok=WritePrivateProfileStringW(MagazineSettingsSection(),kPistolGuideKeys[n],text,path)!=FALSE;
 if(n>=6&&n<9)ok=WritePrivateProfileStringW(MagazineSettingsSection(),L"HandSpace",L"2",path)!=FALSE&&ok;
 WritePrivateProfileStringW(nullptr,nullptr,nullptr,path);return ok;
}
