void TestGuideGroups42(){
 fc3xr::ForceReferenceValid(true);const float p[3]={.1f,.2f,.3f},h[3]={-.1f,.2f,.3f};float pose[7];
 fc3xr::SetGuideGroupVisibility(true,true);fc3xr::SetReloadMarkers(p,h,.05f,0,.1f);fc3xr::SetGrabMarkers(p,h,false,.2f,.1f);
 for(bool reload:{false,true})for(bool support:{false,true}){
  fc3xr::SetGuideGroupVisibility(reload,support);
  Check(fc3xr::MarkerPoseForTest(0,pose)==reload&&fc3xr::MarkerPoseForTest(1,pose)==reload,"reload group master independently gates native XR reload primitives");
  Check(fc3xr::MarkerPoseForTest(11,pose)==support&&fc3xr::MarkerPoseForTest(11,pose)==support,"support group master independently gates native XR support primitives");
  // Another group's publication cannot override either master.
  fc3xr::SetReloadMarkers(p,h,.05f,1,.1f);fc3xr::SetGrabMarkers(p,h,true,.2f,.1f);
  Check(fc3xr::MarkerPoseForTest(0,pose)==reload&&fc3xr::MarkerPoseForTest(11,pose)==support,"fresh/repeated publications cannot re-enable disabled group");
 }
 fc3xr::SetGuideGroupVisibility(true,true);fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);
 Check(!fc3xr::MarkerPoseForTest(0,pose)&&fc3xr::MarkerPoseForTest(11,pose),"clearing reload guide cannot clear support gizmo");
 fc3xr::ForceReferenceValid(false);
 puts("PASS: guide42 four independent XR group combinations, repeated-publication isolation and reload-clear isolation");
}
