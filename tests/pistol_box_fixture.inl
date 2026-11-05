void TestPistolBoxes()
{
 auto saved=g_pistolGuides;float oldScale=g_worldScale;g_worldScale=1.25f;
 g_pistolGuides=PistolGuideSettings{};g_pistolGuides.box=true;g_pistolGuides.handDiameter=.028f; // the geometry below is for a 2.8 cm sphere (the default is now 10 cm)
 g_pistolGuides.port[0]=g_pistolGuides.port[1]=g_pistolGuides.port[2]=0;
 g_pistolGuides.size[0][0]=.04f;g_pistolGuides.size[0][1]=.2f;g_pistolGuides.size[0][2]=.06f;
 Mat4 root=HandIdentity();root.m[3]=.3;root.m[7]=-.2;root.m[11]=.6;
 Check(PistolGuideContact(root,0,{.3,-.09,.6}),"long box accepts insertion along length");
 Check(!PistolGuideContact(root,0,{.356,-.2,.6}),"narrow box rejects contact outside width");
 g_pistolGuides.angles[0][2]=90;
 Check(PistolGuideContact(root,0,{.41,-.2,.6})&&!PistolGuideContact(root,0,{.3,-.144,.6}),"rotated box contact matches rotated visible axes");
 Check(PistolGuideContact(root,0,{.3,-.174,.6}),"hand sphere surface overlaps box even with centre outside");
 Check(!PistolGuideContact(root,0,{.445,-.153,.6}),"sphere cannot contact outside rotated corner by radius alone on each axis");
 wchar_t path[MAX_PATH];GetTempPathW(MAX_PATH,path);wcscat_s(path,L"fc3-box-fixture.ini");
 WritePrivateProfileStringW(L"Shotgun",L"Untouched",L"47",path);
 Check(SavePistolGuides(path),"box dimensions and rotation save per weapon");
 g_pistolGuides=PistolGuideSettings{};LoadPistolGuides(path);
 Check(g_pistolGuides.box&&std::fabs(g_pistolGuides.size[0][1]-.2f)<1e-6&&g_pistolGuides.angles[0][2]==90,"saved box dimensions and angles reload exactly");
 g_pistolGuides.size[0][0]=.09f;g_pistolGuides.size[0][1]=.3f;
 Check(SavePistolGuideValue(12,path),"single box dimension saves independently");LoadPistolGuides(path);
 Check(std::fabs(g_pistolGuides.size[0][0]-.09f)<1e-6&&std::fabs(g_pistolGuides.size[0][1]-.2f)<1e-6&&GetPrivateProfileIntW(L"Shotgun",L"Untouched",0,path)==47,"saving one box slider preserves other unsaved settings and shotgun");
 DeleteFileW(path);g_pistolGuides=saved;g_worldScale=oldScale;
 std::puts("PASS: oriented pistol boxes share visual/contact geometry and preserve saved settings");
}
