// Mode0 physical insertion, Mode1 one-shell X, Mode2 original bulk X.
struct ManualReloadSettings
{
    int mode=0;
    bool marker=true;
    float port[3]={-.00199296f,-.01781280f,.07463275f};
    float radius=.065f;
} g_manualSettings;
bool g_manualReloadEnabled=true;
void ApplyManualReloadMode();
bool g_reloadAlignment=false;
void ManualSettingsPath(wchar_t* path)
{_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_stereo.ini",fc3::DataFolder());}
float ManualReadSetting(const wchar_t* path,const wchar_t* key,float fallback)
{
    wchar_t text[64]={};GetPrivateProfileStringW(L"Reload",key,L"",text,64,path);
    wchar_t* end=nullptr;const float value=static_cast<float>(wcstod(text,&end));
    return end==text||*end||!std::isfinite(value)?fallback:value;
}
void LoadManualReloadSettings()
{
    wchar_t path[MAX_PATH];ManualSettingsPath(path);
    const float mode=ManualReadSetting(path,L"Mode",0);
    g_manualSettings.mode=mode>=0&&mode<=2?static_cast<int>(mode):0;
    g_manualSettings.marker=ManualReadSetting(path,L"ShowMarker",1)!=0;
    const wchar_t* keys[]={L"PortX",L"PortY",L"PortZ"};
    for(int i=0;i<3;++i)
    {const float v=ManualReadSetting(path,keys[i],g_manualSettings.port[i]);if(std::fabs(v)<=.5f)g_manualSettings.port[i]=v;}
    const float radius=ManualReadSetting(path,L"Radius",g_manualSettings.radius);
    if(radius>=.015f&&radius<=.15f)g_manualSettings.radius=radius;
    g_manualReloadEnabled=g_manualSettings.mode==0;
    Log("[MANUAL RELOAD] config mode %d port %.5f %.5f %.5f radius %.3f marker %d",g_manualSettings.mode,g_manualSettings.port[0],g_manualSettings.port[1],g_manualSettings.port[2],g_manualSettings.radius,g_manualSettings.marker);
}
bool SaveManualReloadSettings(const wchar_t* explicitPath=nullptr)
{
    wchar_t path[MAX_PATH];ManualSettingsPath(path);const wchar_t* target=explicitPath?explicitPath:path;
    wchar_t value[64];bool ok=true;
    const wchar_t* keys[]={L"Mode",L"ShowMarker",L"PortX",L"PortY",L"PortZ",L"Radius"};
    const float values[]={static_cast<float>(g_manualSettings.mode),g_manualSettings.marker?1.f:0.f,g_manualSettings.port[0],g_manualSettings.port[1],g_manualSettings.port[2],g_manualSettings.radius};
    for(int i=0;i<6;++i){_snwprintf_s(value,64,_TRUNCATE,L"%.6f",values[i]);ok=WritePrivateProfileStringW(L"Reload",keys[i],value,target)!=FALSE&&ok;}
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,target);return ok;
}
