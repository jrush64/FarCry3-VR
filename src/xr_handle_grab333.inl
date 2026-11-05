void ResolveHandleGrab333(fc3hands303::Input& input,const fc3pad::Controllers& c,const XrSpaceLocation* loc,const XrVector3f* palm,ULONGLONG now){
 g_ammoPouch334=false;
 g_supportTracked342=false;input.supportKeep345=false;
 const auto& held=g_hands303;const int receiving356=1-held.hand;
 const bool latched356=receiving356>=0&&receiving356<2&&input.context&&!held.empty&&held.gripAction333[receiving356]==fc3grab333::Support;
 const float grip356=receiving356==0?c.rightGrip:c.leftGrip;
 const auto b=fc3support356::Live().Choose(fc3grab333::Read(),held.entity,held.generation,g_roleEpoch303,input.reference,
     latched356&&c.active,input.tracked[0]&&input.tracked[1],grip356,now);
 if(b.profile>=3&&b.profile<=6){static bool previous=false;static unsigned lines=0;
  if(previous!=b.supportCarry356&&lines++<80){previous=b.supportCarry356;Log("[SUPPORT356] continuation=%d profile=%d entity=%08X generation=%u sourceAge=%llu tracked=%d/%d grip=%.3f",b.supportCarry356,b.profile,b.entity,b.generation,now>=b.at?now-b.at:~0ull,input.tracked[0],input.tracked[1],grip356);}}
 if(!b.mode||b.entity!=held.entity||b.epoch!=g_roleEpoch303||b.reference!=input.reference||held.hand<0||held.hand>1)return;
 input.direct333=true;input.busy333=b.busy;input.reloadNear333=b.reloadNear;
 if(!b.enabled||b.generation!=held.generation||now<b.at||now-b.at>150){input.transferAllowed307=false;input.reloadNear333=false;input.busy333=b.busy&&b.generation==held.generation&&now>=b.at&&now-b.at<=150;return;}
 const int other=1-held.hand;
 if(input.tracked[0]&&input.tracked[1]){
  g_supportTracked342=true;
  const auto& pose=loc[held.hand].pose;const auto handle=testnotes::Add(pose.position,testnotes::Rotate(pose.orientation,{b.handle[0],b.handle[1],b.handle[2]}));
  const auto delta=testnotes::Sub(palm[other],handle);const float distance=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
  float front=1000;if(b.support){const auto centre=testnotes::Add(pose.position,testnotes::Rotate(pose.orientation,{b.foregrip[0],b.foregrip[1],b.foregrip[2]}));const auto d=testnotes::Sub(palm[other],centre);front=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);}
  float action=1000;bool actionNear=false;
  for(unsigned n=0;n<b.actionCount&&n<6;++n){const auto& a=b.actions[n];const auto centre=testnotes::Add(pose.position,testnotes::Rotate(pose.orientation,{a.point[0],a.point[1],a.point[2]}));const auto d=testnotes::Sub(palm[other],centre);const float length=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);if(std::isfinite(length)&&length<action){action=length;actionNear=length<=a.radius;}}
  input.handle333=std::isfinite(distance)&&distance<=.09f&&distance+.012f<front&&distance<=action+.012f;
  const bool shotgun342=(b.profile>=3&&b.profile<=6)||b.profile==34;
  input.support333=b.support&&front<=(shotgun342?.20f:.18f)&&!input.handle333;
  input.supportKeep345=b.support&&std::isfinite(front)&&front<=(shotgun342?.32f:.28f);
  // Classify current tracked palm against the actual action targets; do not
  // let last frame's free-hand position assign this squeeze to the wrong part.
  if(b.actionCount)input.reloadNear333=actionNear&&action+.012f<front&&!input.handle333;
  // Preserve deliberate existing magazine-button commands away from a grabbed
  // handle/support part. A grip alone in empty space still creates no ammo.
  const bool ammoButton=held.hand!=g_nativeHoldingSide303?c.b:c.x;
  if(ammoButton&&!input.handle333&&!input.support333)input.reloadNear333=true;
  const auto forward=testnotes::Rotate(g_views[0].pose.orientation,{0,0,-1});const float yaw=std::atan2(-forward.x,-forward.z);const XrQuaternionf level={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};
  const XrVector3f head={(g_views[0].pose.position.x+g_views[1].pose.position.x)*.5f,(g_views[0].pose.position.y+g_views[1].pose.position.y)*.5f,(g_views[0].pose.position.z+g_views[1].pose.position.z)*.5f};
  for(const auto& offset:{XrVector3f{other?-.18f:.18f,-.65f,.03f},XrVector3f{0,-.38f,-.12f}}){const auto pouch=testnotes::Add(head,testnotes::Rotate(level,offset));const auto d=testnotes::Sub(palm[other],pouch);input.ammoNear333=input.ammoNear333||d.x*d.x+d.y*d.y+d.z*d.z<=.18f*.18f;}
  // At the forend, a nearby body pouch must not steal this support squeeze.
  // A genuinely closer loading port retains its existing reload priority.
  if(shotgun342&&input.support333&&!input.reloadNear333)input.ammoNear333=false;
 }
 // This gesture uses verified native handle/controller coordinates, not the
 // holster renderer's readiness lease. The existing generation-change fire
 // guard requires trigger release after transfer, so resting on a trigger
 // must not silently prevent a handle grab.
 g_ammoPouch334=b.profile==39&&input.ammoNear333;
 input.transferAllowed307=input.context&&!b.supportCarry356&&!b.busy&&!c.x&&!c.y&&!c.a&&!c.b&&!c.menu;
}
