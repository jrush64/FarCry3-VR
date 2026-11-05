// Only visibility is stored here. Existing weapon contact/calibration keys are untouched.
bool g_weaponGuideShown[64];bool g_weaponGuideFlagsLoaded=false;
int WeaponGuideProfile(const char* name){
 const char* names[]={"KW1911","6P9","DesertEagle","AK47","A2000","STGW-90","VZ61","MP5","Famas","BZ19","P416","MK16","KSV","GalilAce52","SVD","R700","U100","MKG","M93","PKM","BOW","LPO-50","FLARE_GUN","GM-94","RPG-7","HUNTER","Spas12","1887","M133","ULC_Type10FlareGun","ChineseSword","MACHETE","SHIV","TANTO","ULC_TribalKnife"};
 const int profiles[]={20,1,12,8,7,41,39,28,14,9,31,25,21,17,35,2,36,22,23,43,10,15,16,18,32,42,34,0,3,38,11,24,33,19,37};
 for(int i=0;i<35;++i)if(strcmp(name,names[i])==0)return profiles[i];return -1;
}
void LoadWeaponGuideFlags(const wchar_t* explicitPath=nullptr){
 if(g_weaponGuideFlagsLoaded&&!explicitPath)return;g_weaponGuideFlagsLoaded=true;
 wchar_t path[MAX_PATH];ManualSettingsPath(path);
 if(explicitPath)wcscpy_s(path,explicitPath);
 for(int i=0;i<64;++i){wchar_t key[40];swprintf_s(key,L"ShowPhysical_%d",i);
#ifdef FC3_AMMO_TEST
 g_weaponGuideShown[i]=explicitPath?GetPrivateProfileIntW(L"WeaponGuides",key,1,path)!=0:true;
#else
 g_weaponGuideShown[i]=GetPrivateProfileIntW(L"WeaponGuides",key,1,path)!=0;
#endif
 }
}
bool PhysicalReloadGuidesEnabled(){LoadWeaponGuideFlags();const int p=g_weaponMechanicProfile>=3&&g_weaponMechanicProfile<=6?3:g_weaponMechanicProfile;return p<0?true:p<64&&g_weaponGuideShown[p];}
bool SaveWeaponGuideFlag(int profile,bool show,const wchar_t* path){
 if(profile<0||profile>=64||!path)return false;wchar_t key[40];swprintf_s(key,L"ShowPhysical_%d",profile);
 return WritePrivateProfileStringW(L"WeaponGuides",key,show?L"1":L"0",path)!=0;
}
void WeaponGuideCheckbox(const char* name){
 LoadWeaponGuideFlags();const int profile=WeaponGuideProfile(name);if(profile<0)return;
 if(ImGui::Checkbox("Show physical reload guide",&g_weaponGuideShown[profile])){
  wchar_t path[MAX_PATH];ManualSettingsPath(path);
  g_pistolMenu.result=SaveWeaponGuideFlag(profile,g_weaponGuideShown[profile],g_menuSavePath?g_menuSavePath:path)?1:2;
 }
}
// Pure visuals use the same saved local points and radii as 1887 contact checks.

void Lever1887GuideMarkers(){
 const auto& s=g_lever;
 if(g_pistolMenu.open&&g_leverGuideTarget==2){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 if(!PhysicalReloadGuidesEnabled()||s.simple||s.displayFrame<0||(!g_pistolMenu.open&&g_frame-s.displayFrame>2)){
  fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;
 }
 const bool gate=g_pistolMenu.open?g_leverGuideTarget==0:s.shell||s.leverOpen;
 const float scale=(std::max)(.1f,g_worldScale),radius=(gate?s.gateRadius:s.grabRadius)*scale;
 const int target=gate?1:0;Mat4 box=Multiply(s.displayBody,Lever1887GuideLocal52(target,!gate&&s.leverOpen));const V3 center={box.m[3],box.m[7],box.m[11]};
 V3 palm={};const bool hand=Lever1887PalmBody56(0,palm);float p[3]={float(center.x),float(center.y),float(center.z)},h[3]={float(palm.x),float(palm.y),float(palm.z)},matrix[16],dims[3];for(int i=0;i<3;++i)dims[i]=Lever1887GuideSize52(target,i)*scale;
 for(int i=0;i<16;++i)matrix[i]=float(box.m[i]);
 fc3xr::SetReloadMarkers(p,hand?h:nullptr,radius,0,float(2*Lever1887PalmRadius56(0)));fc3xr::SetWeaponBox(matrix,dims,0);if(hand)fc3xr::SetHandSphere();
}
