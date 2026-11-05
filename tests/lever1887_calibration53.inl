void TestLever1887Calibration53(){
 const auto saved=g_lever;const auto scale=g_worldScale;g_worldScale=1;g_lever=Lever1887{};auto& L=g_lever;
 L.gateRadius=.09f;L.grabRadius=.12f;L.gate[0]=.021f;L.left[1]=.034f;
 Check(fabs(Lever1887GuideSize52(1,0)-.18f)<1e-6,"legacy gate dimensions inherit saved radius");
 Check(Lever1887GuideContact52(1,{L.gate[0]+.08,L.gate[1],L.gate[2]})&&!Lever1887GuideContact52(1,{L.gate[0]+.10,L.gate[1],L.gate[2]}),"old files retain exact sphere contact");
 L.guideBox[1]=true;L.guideSize[1][0]=.04f;L.guideSize[1][1]=.20f;L.guideSize[1][2]=.06f;L.guideRot[1][2]=90;
 Check(Lever1887GuideContact52(1,{L.gate[0]+.08,L.gate[1],L.gate[2]})&&!Lever1887GuideContact52(1,{L.gate[0],L.gate[1]+.08,L.gate[2]}),"edited gate rotation and dimensions drive physical contact");
 L.guideBox[0]=true;L.guideSize[0][0]=.02f;L.guideSize[0][1]=.03f;L.guideSize[0][2]=.04f;
 const Mat4 loop=Lever1887GuideLocal52(0,true,true);Check(Lever1887GuideContact52(0,{loop.m[3],loop.m[7],loop.m[11]},true,true),"open mirrored lever uses same calibrated box center");
 wchar_t folder[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,folder);Check(GetTempFileNameW(folder,L"l53",0,path)!=0,"1887 test settings path");
 Check(SaveLever1887Settings(path),"1887 additive calibration save");g_lever=Lever1887{};LoadLever1887Settings(path);
 Check(g_lever.guideBox[0]&&g_lever.guideBox[1]&&fabs(g_lever.guideSize[1][1]-.20f)<1e-6&&g_lever.guideRot[1][2]==90,"both independent box settings round trip");
 Check(fabs(g_lever.left[1]-.034f)<1e-6&&fabs(g_lever.gate[0]-.021f)<1e-6&&fabs(g_lever.gateRadius-.09f)<1e-6,"saved whole gun placement, gate point and old radius preserved");DeleteFileW(path);
 g_lever=saved;g_worldScale=scale;puts("PASS: lever53 additive gate/lever dimensions and rotations; real contacts, mirrored open lever, saved numeric calibration round trip");
}
