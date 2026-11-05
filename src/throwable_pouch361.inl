bool PouchDraw361(bool headset,const fc3pad::Controllers& c){
 g_pouchGuide361=false;const auto h=fc3hands303::Read();const auto now=GetTickCount64();
 const bool allowed=g_pouchEnabled361&&headset&&g_throw144.enabled&&!g_throw144.owned&&h.enabled&&h.empty&&
  c.active&&!c.menu&&!c.a&&!c.b&&!c.x&&!c.y&&c.leftTrigger<.2f&&c.rightTrigger<.2f&&
  !fc3pad::NativeMenuOpen()&&!fc3pad::MenuOpen()&&!fc3pad::GameplayInputBlocked()&&!fc3pad::SelectorActive290()&&
  !fc3pad::WeaponSelectionRequested()&&!fc3pad::DoorInputActive363()&&g_enabled&&g_motionGun&&g_inGameplay&&fc3aim::NativeFootCamera281()&&
  !BodyBusy285(false)&&g_physicalValid303[0]&&now>=g_physicalAt352&&now-g_physicalAt352<=100&&BodyHead285();
 float distance=1000;V3 palm={};
 if(allowed){
  Mat4 physical={};for(int n=0;n<16;++n)physical.m[n]=g_physicalBody303[0][n];palm=Apply(physical,{0,-.015,.015});
  const auto relative=fc3body285::Relative({float(palm.x),float(palm.y),float(palm.z)},g_bodyRuntime285.head,g_bodyRuntime285.yaw,g_worldScale);
  distance=fc3body285::Distance(relative,{g_pouchPosition361[0],g_pouchPosition361[1],g_pouchPosition361[2]});
  if(distance<.5f||g_pistolMenu.open){
   const float yaw=g_bodyRuntime285.yaw,scale=g_worldScale;const auto head=g_bodyRuntime285.head;
   const float point[]={head.x+scale*(std::cos(yaw)*g_pouchPosition361[0]+std::sin(yaw)*g_pouchPosition361[2]),head.y+scale*g_pouchPosition361[1],head.z+scale*(-std::sin(yaw)*g_pouchPosition361[0]+std::cos(yaw)*g_pouchPosition361[2])};
   const float hand[]={float(palm.x),float(palm.y),float(palm.z)};fc3xr::SetGrabMarkers(point,hand,distance<=.12f,.24f,.08f);g_pouchGuide361=true;
  }
 }
 const bool draw=g_pouchState361.Step(allowed,g_physicalValid303[0],c.rightGrip,distance,g_armPoseEpoch,now);
 if(draw){fc3xr::InteractionPulse285(0);Log("[POUCH361] fresh empty-hand pouch grip requests current native wheel-selected throwable; old grip+B/A disabled");}
 return draw;
}
