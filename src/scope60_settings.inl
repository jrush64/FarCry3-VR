extern bool g_redDot;
bool g_centerDot135=false;
bool g_holoGuide133=false;
bool g_scopeSceneEnabled132=false,g_holoSceneEnabled132=false,g_scopeLensEnabled132=true,g_holoLensEnabled132=false;
float g_scopeSceneZoom132=1.35f,g_holoSceneZoom132=1.2f,g_holoLensZoom132=1.5f;
void LoadOpticAssist132(const wchar_t* path){
 g_redDot=GetPrivateProfileIntW(L"Reticle",L"Enabled",1,path)!=0;
 g_centerDot135=GetPrivateProfileIntW(L"Reticle",L"CentreInSightWhenZoomed",0,path)!=0; // 206: default OFF, the centre marker is not the shot
 g_holoGuide133=GetPrivateProfileIntW(L"Scopes",L"SightAlignmentGuide",0,path)!=0;
 g_scopeSceneEnabled132=GetPrivateProfileIntW(L"Scopes",L"ScopeSceneZoomEnabled",0,path)!=0;
 g_holoSceneEnabled132=GetPrivateProfileIntW(L"Scopes",L"SightSceneZoomEnabled",0,path)!=0;
 g_scopeLensEnabled132=GetPrivateProfileIntW(L"Scopes",L"ScopeLensZoomEnabled",1,path)!=0;
 g_holoLensEnabled132=GetPrivateProfileIntW(L"Scopes",L"SightLensZoomEnabled",0,path)!=0;
 g_scopeSceneZoom132=(std::max)(1.f,(std::min)(3.f,GetPrivateProfileIntW(L"Scopes",L"ScopeSceneZoomPercent",135,path)/100.f));
 g_holoSceneZoom132=(std::max)(1.f,(std::min)(3.f,GetPrivateProfileIntW(L"Scopes",L"SightSceneZoomPercent",120,path)/100.f));
 g_holoLensZoom132=(std::max)(1.f,(std::min)(4.f,GetPrivateProfileIntW(L"Scopes",L"SightLensZoomPercent",150,path)/100.f));
}
bool SaveOpticAssist132(const wchar_t* path){
 const bool dotSaved135=WritePrivateProfileStringW(L"Reticle",L"Enabled",g_redDot?L"1":L"0",path)!=0&&WritePrivateProfileStringW(L"Reticle",L"CentreInSightWhenZoomed",g_centerDot135?L"1":L"0",path)!=0;
 bool ok=WritePrivateProfileStringW(L"Scopes",L"SightAlignmentGuide",g_holoGuide133?L"1":L"0",path)!=0;const wchar_t* keys[]={L"ScopeSceneZoomEnabled",L"SightSceneZoomEnabled",L"ScopeLensZoomEnabled",L"SightLensZoomEnabled",L"ScopeSceneZoomPercent",L"SightSceneZoomPercent",L"SightLensZoomPercent"};
 const int values[]={int(g_scopeSceneEnabled132),int(g_holoSceneEnabled132),int(g_scopeLensEnabled132),int(g_holoLensEnabled132),int(g_scopeSceneZoom132*100+.5f),int(g_holoSceneZoom132*100+.5f),int(g_holoLensZoom132*100+.5f)};
 for(int i=0;i<7;++i){wchar_t value[16];swprintf_s(value,L"%d",values[i]);ok=(WritePrivateProfileStringW(L"Scopes",keys[i],value,path)!=0)&&ok;}return ok&&dotSaved135;
}
bool HoloLensActive132();
bool OpticZoomHeld133();
void UpdateOpticScene132();
bool g_holoEnabled131=true;float g_holoDotMoa131=4.f,g_holoSteady131=.75f;int g_scopeEye131=0;
extern float g_weaponSteady129;
// Additive scope controls. Never pass LT through to native ADS globally.
float g_scopeSizeSteady67=.65f;
float g_scopeAimSteady72=.65f;
int g_scopeMode60=0;float g_scopeZoom60=4.f,g_reflexZoom63=1.5f;bool g_scopeEnabled60=true,g_scopeSaveFailed60=false,g_scopePreview65=false;
void LoadScopeSettings60(const wchar_t* path){LoadOpticAssist132(path);g_holoSteady131=(std::max)(0.f,(std::min)(1.f,GetPrivateProfileIntW(L"Scopes",L"SightSteadinessPercent",75,path)/100.f));g_scopeEye131=(std::max)(0,(std::min)(2,int(GetPrivateProfileIntW(L"Scopes",L"ScopeEye",0,path))));g_holoEnabled131=GetPrivateProfileIntW(L"Scopes",L"PhysicalReflex",1,path)!=0;g_holoDotMoa131=(std::max)(2.f,(std::min)(12.f,GetPrivateProfileIntW(L"Scopes",L"ReflexDotMoaTenths",40,path)/10.f));g_weaponSteady129=(std::max)(0.f,(std::min)(1.f,GetPrivateProfileIntW(L"Scopes",L"WeaponSteadinessPercent",45,path)/100.f));g_scopeAimSteady72=(std::max)(0.f,(std::min)(1.f,GetPrivateProfileIntW(L"Scopes",L"AimSteadinessPercent",65,path)/100.f));g_scopeSizeSteady67=(std::max)(0.f,(std::min)(1.f,GetPrivateProfileIntW(L"Scopes",L"SizeSteadinessPercent",65,path)/100.f));g_scopePreview65=GetPrivateProfileIntW(L"Scopes",L"PreviewInInsert",0,path)!=0;g_scopeMode60=0;g_scopeEnabled60=GetPrivateProfileIntW(L"Scopes",L"Enabled",1,path)!=0;g_reflexZoom63=(std::max)(1.f,(std::min)(4.f,GetPrivateProfileIntW(L"Scopes",L"ReflexMagnificationPercent",150,path)/100.f));g_scopeZoom60=(std::max)(1.f,(std::min)(8.f,GetPrivateProfileIntW(L"Scopes",L"MagnificationPercent",400,path)/100.f));}
bool SaveScopeSettings60(const wchar_t* path){if(!path)return false;const bool savedAssist132=SaveOpticAssist132(path);wchar_t value[16];swprintf_s(value,L"%d",int(g_holoDotMoa131*10+.5f));bool savedHolo131=WritePrivateProfileStringW(L"Scopes",L"ReflexDotMoaTenths",value,path)!=0&&WritePrivateProfileStringW(L"Scopes",L"PhysicalReflex",g_holoEnabled131?L"1":L"0",path)!=0;swprintf_s(value,L"%d",int(g_holoSteady131*100+.5f));savedHolo131=(WritePrivateProfileStringW(L"Scopes",L"SightSteadinessPercent",value,path)!=0)&&savedHolo131;swprintf_s(value,L"%d",g_scopeEye131);savedHolo131=(WritePrivateProfileStringW(L"Scopes",L"ScopeEye",value,path)!=0)&&savedHolo131;swprintf_s(value,L"%d",int(g_weaponSteady129*100+.5f));const bool savedWeapon129=WritePrivateProfileStringW(L"Scopes",L"WeaponSteadinessPercent",value,path)!=0;swprintf_s(value,L"%d",int(g_scopeAimSteady72*100+.5f));const bool savedAim72=WritePrivateProfileStringW(L"Scopes",L"AimSteadinessPercent",value,path)!=0;swprintf_s(value,L"%d",int(g_scopeSizeSteady67*100+.5f));bool savedSteady67=WritePrivateProfileStringW(L"Scopes",L"SizeSteadinessPercent",value,path)!=0;bool ok=WritePrivateProfileStringW(L"Scopes",L"Mode",g_scopeMode60?L"1":L"0",path)!=0;ok=(WritePrivateProfileStringW(L"Scopes",L"Enabled",g_scopeEnabled60?L"1":L"0",path)!=0)&&ok;ok=(WritePrivateProfileStringW(L"Scopes",L"PreviewInInsert",g_scopePreview65?L"1":L"0",path)!=0)&&ok;swprintf_s(value,L"%d",int(g_scopeZoom60*100+.5f));ok=(WritePrivateProfileStringW(L"Scopes",L"MagnificationPercent",value,path)!=0)&&ok;swprintf_s(value,L"%d",int(g_reflexZoom63*100+.5f));return (WritePrivateProfileStringW(L"Scopes",L"ReflexMagnificationPercent",value,path)!=0)&&ok&&savedSteady67&&savedAim72&&savedWeapon129&&savedHolo131&&savedAssist132;}
struct DrawCall;
void CaptureScope60(ID3D11DeviceContext*,const DrawCall&,ID3D11Buffer*,int,int,ID3D11Buffer* =nullptr);
void RenderScopes60(ID3D11Texture2D*,ID3D11Texture2D*);
void ClearScopeResources60();

void ScopeMenu60();

bool ScopeNativeGlass60(ID3D11DeviceContext*,const DrawCall&,int);
// Weapon-free scene graph follows existing stereo cameras; no controller or
// native ADS camera changes. All associations remain on their D3D objects.
const GUID kScopeClean61={0x61d31100,0x5a1e,0x4c61,{0x90,0x80,0x61,0x10,0x31,0x32,0x33,1}};
const GUID kScopeCleanStamp61={0x61d31100,0x5a1e,0x4c61,{0x90,0x80,0x61,0x10,0x31,0x32,0x33,2}};
void ScopeCleanTrack61(ID3D11DeviceChild*);
bool ScopeCleanAllowed61();
void ScopeCleanReset61();
extern UINT g_armPoseEpoch;

const GUID kScopeCleanRight61={0x61d31100,0x5a1e,0x4c61,{0x90,0x80,0x61,0x10,0x31,0x32,0x33,3}};
// Separate raster camera for the optical scene. These never replace native
// camera constants, controller transforms or the physical muzzle publication.
ID3D11Buffer* g_scopeVs62[2]={},*g_scopePs62[2]={},*g_scopePsTile62[2]={},*g_scopeTile62[2]={};
LONG g_scopeProjectionFrame62[2]={-1,-1};
float g_scopePresentation62[2][16]={};
float g_scopeSceneAxis62[2][2]={};
void UpdateScopeProjection62(const double*,int);
void ScopePresentation62(float*,int,int);
void ResetScopeProjection62();

bool ScopeTwoHand63();
float ScopeZoom63();

namespace probe66 { void Draw(ID3D11DeviceContext*,const DrawCall&,int,int,bool); void ResetGpu(); }
