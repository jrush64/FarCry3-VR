void TestGuides44(){
 const LONG oldProfile=g_weaponMechanicProfile;LoadWeaponGuideFlags();bool saved[64];memcpy(saved,g_weaponGuideShown,sizeof(saved));
 for(const auto& weapon:kMenuWeapons){const int p=WeaponGuideProfile(weapon.name);Check(p>=0&&p<44,"every weapon menu maps to a real native profile");g_weaponMechanicProfile=p;
  g_weaponGuideShown[p]=false;Check(!PhysicalReloadGuidesEnabled(),"per-weapon off controls its physical guide");g_weaponGuideShown[p]=true;Check(PhysicalReloadGuidesEnabled(),"per-weapon on controls its physical guide");}
 g_weaponGuideShown[3]=false;for(int p=3;p<=6;++p){g_weaponMechanicProfile=p;Check(!PhysicalReloadGuidesEnabled(),"all four M133 mesh draws share one visibility choice");}g_weaponGuideShown[3]=true;
 wchar_t dir[MAX_PATH],path[MAX_PATH],text[64];GetTempPathW(MAX_PATH,dir);GetTempFileNameW(dir,L"g44",0,path);
 WritePrivateProfileStringW(L"PistolReload",L"PortX",L"0.12345678",path);WritePrivateProfileStringW(L"PistolReload",L"PortWidth",L"0.09876543",path);WritePrivateProfileStringW(L"Gm94Reload",L"HandDiameter",L"0.13579",path);
 Check(SaveWeaponGuideFlag(20,false,path)&&SaveWeaponGuideFlag(8,true,path),"new visibility saves without saving/resetting calibration");LoadWeaponGuideFlags(path);
 Check(!g_weaponGuideShown[20]&&g_weaponGuideShown[8]&&g_weaponGuideShown[34],"visibility reload retains independent choices and defaults");
 GetPrivateProfileStringW(L"PistolReload",L"PortX",L"",text,64,path);Check(wcscmp(text,L"0.12345678")==0,"saved position preserved exactly");GetPrivateProfileStringW(L"PistolReload",L"PortWidth",L"",text,64,path);Check(wcscmp(text,L"0.09876543")==0,"saved box size preserved exactly");GetPrivateProfileStringW(L"Gm94Reload",L"HandDiameter",L"",text,64,path);Check(wcscmp(text,L"0.13579")==0,"other weapon hand size preserved exactly");
 SupportGrab214 oldGrabs[44];memcpy(oldGrabs,g_supportGrab214,sizeof(oldGrabs));
 g_supportGrab214[20].offset[0]=.084f;g_supportGrab214[20].offset[2]=-.026f;g_supportGrab214[20].reach=.21f;g_supportGrab214[20].sizeScale[0]=1.75f;g_supportGrab214[20].sizeScale[1]=.65f;g_supportGrab214[20].sizeScale[2]=1.2f;
 Check(SaveSupportGrab214(20,path),"1911 two-hand grab position and reach save independently");
 LoadSupportGrab214(path);Check(std::fabs(g_supportGrab214[20].offset[0]-.084f)<1e-5&&std::fabs(g_supportGrab214[20].offset[2]+.026f)<1e-5&&std::fabs(SupportGrabReach214(20)-.21)<1e-5&&std::fabs(g_supportGrab214[20].sizeScale[0]-1.75f)<1e-5&&std::fabs(g_supportGrab214[20].sizeScale[1]-.65f)<1e-5&&std::fabs(g_supportGrab214[20].sizeScale[2]-1.2f)<1e-5,"1911 grab position, reach and independent XYZ sizes survive reload");
 const float oldWorldScale=g_worldScale;g_worldScale=1;float grabSize[3];SupportGrabBoxSize214(20,grabSize);Check(grabSize[0]>grabSize[2]&&grabSize[2]>grabSize[1],"visible support box keeps independently scaled XYZ dimensions");Mat4 grabBox={};Check(SupportGrabBox214(HandIdentity(),{},grabBox),"support detector uses drawable box transform");const double justOutside=SupportGrabOutside214(20,grabBox,{grabSize[0]*.5+.03,0,0});Check(std::fabs(justOutside-.03)<1e-5,"grab detector tracks scaled X face");Check(SupportGrabOutside214(20,grabBox,{0,grabSize[1]*.5+.04,0})>.039,"grab detector tracks scaled Y face");g_worldScale=oldWorldScale;
 Check(SupportGrabEditable214(15)&&!SupportGrabEditable214(42)&&SupportGrabReach214(42)==kGrabReach,"flamethrower support is adjustable while revolver reload grip stays separate");
 GetPrivateProfileStringW(L"PistolReload",L"PortX",L"",text,64,path);Check(wcscmp(text,L"0.12345678")==0,"saving support grab does not change reload placement");
 memcpy(g_supportGrab214,oldGrabs,sizeof(oldGrabs));DeleteFileW(path);
 fc3xr::ForceReferenceValid(true);const float scale=(std::max)(.1f,fc3stereo::WorldScale());float p[3]={.1f*scale,.2f*scale,scale},h[3]={-.1f*scale,.2f*scale,scale},pose[7];
 fc3xr::SetGuideGroupVisibility(true,true,true);fc3xr::SetReloadMarkers(p,h,.05f,0,.1f);fc3xr::SetHandSphere();fc3xr::SetGrabMarkers(p,h,false,.2f,.1f);
 Check(fc3xr::MarkerPoseForTest(1,pose)&&fc3xr::MarkerPoseForTest(8,pose)&&fc3xr::MarkerPoseForTest(9,pose),"Insert retains all three loading sphere planes");
 int faces=0;for(int i:{10,14,15,16,17,18})if(fc3xr::MarkerPoseForTest(i,pose)){++faces;Check(std::isfinite(pose[0])&&std::isfinite(pose[6]),"holding cube has finite actual XR face poses");}Check(faces>=1&&faces<=3,"holding point is a spatial cube with only nearest faces");
 fc3xr::SetGuideGroupVisibility(false,true,true);Check(!fc3xr::MarkerPoseForTest(1,pose)&&fc3xr::MarkerPoseForTest(11,pose),"global physical hide overrides Insert without hiding holding sphere");
 fc3xr::SetGuideGroupVisibility(true,false,true);Check(fc3xr::MarkerPoseForTest(1,pose)&&!fc3xr::MarkerPoseForTest(11,pose),"global holding hide overrides Insert without hiding loading sphere");
 fc3xr::SetGuideGroupVisibility(true,false,false);
 for(int face=2;face<8;++face){fc3xr::GuideFacesForTest(1u<<face);Check(fc3xr::GuideLayersVisibleForTest()&&fc3xr::GuideBoxAtlasForTest(),"any visible box face admits gameplay layers and selects square artwork");}
 fc3xr::GuideFacesForTest(1u<<1);Check(fc3xr::GuideLayersVisibleForTest()&&!fc3xr::GuideBoxAtlasForTest(),"loading sphere alone keeps correct artwork");
 fc3xr::GuideFacesForTest(0);Check(!fc3xr::GuideLayersVisibleForTest(),"no visible enabled guide produces no layers");
 fc3xr::GuideFacesForTest(1u<<3);fc3xr::SetGuideGroupVisibility(false,false,false);Check(!fc3xr::GuideLayersVisibleForTest(),"global hide still overrides raw visible face");fc3xr::GuideFacesForTest(0);
 fc3xr::SetGuideGroupVisibility(true,true);fc3xr::ForceReferenceValid(false);memcpy(g_weaponGuideShown,saved,sizeof(saved));g_weaponMechanicProfile=oldProfile;
 puts("PASS: guides44 all weapon toggles, M133 aliases, independent persistence without calibration writes, Insert sphere/cube poses and global overrides");
}
