struct ReticlePreset52 {const char* name;ImU32 colour;};
const ReticlePreset52 kReticleColours52[]={
 {"Red",IM_COL32(255,30,30,255)},{"Green",IM_COL32(40,255,80,255)},
 {"Cyan",IM_COL32(30,240,255,255)},{"Blue",IM_COL32(60,120,255,255)},
 {"Yellow",IM_COL32(255,230,30,255)},{"White",IM_COL32(255,255,255,255)},
 {"Magenta",IM_COL32(255,40,220,255)},{"Orange",IM_COL32(255,140,30,255)}
};
int g_redDotColour52=0;bool g_redDotColourSaveFailed52=false;
int g_laserMode194=0;bool g_laserSaveFailed194=false;
int g_spreadPercent206=0;bool g_spreadSaveFailed206=false;
int ReticlePresetIndex52(int value){return value>=0&&value<8?value:0;}
ImU32 ReticleColour52(){return kReticleColours52[ReticlePresetIndex52(g_redDotColour52)].colour;}
void LoadReticleColour52(const wchar_t* path){g_redDotColour52=path?ReticlePresetIndex52(static_cast<int>(GetPrivateProfileIntW(L"Reticle",L"Colour",0,path))):0;g_laserMode194=path?(std::max)(0,(std::min)(3,int(GetPrivateProfileIntW(L"Reticle",L"LaserMode",0,path)))):0;fc3aim::SetLaserEnabled194(g_laserMode194!=0);
 g_spreadPercent206=path?(std::max)(0,(std::min)(100,int(GetPrivateProfileIntW(L"Reticle",L"SpreadPercent",0,path)))):0;fc3aim::SetSpreadPercent206(g_spreadPercent206);}
bool SaveSpreadPercent206(const wchar_t* path){if(!path)return false;wchar_t value[8];swprintf_s(value,L"%d",(std::max)(0,(std::min)(100,g_spreadPercent206)));return WritePrivateProfileStringW(L"Reticle",L"SpreadPercent",value,path)!=0;}
bool SaveReticleColour52(const wchar_t* path){if(!path)return false;wchar_t value[8];swprintf_s(value,L"%d",ReticlePresetIndex52(g_redDotColour52));return WritePrivateProfileStringW(L"Reticle",L"Colour",value,path)!=0;}
bool SaveLaserMode194(const wchar_t* path){if(!path)return false;wchar_t value[8];swprintf_s(value,L"%d",(std::max)(0,(std::min)(3,g_laserMode194)));return WritePrivateProfileStringW(L"Reticle",L"LaserMode",value,path)!=0;}
