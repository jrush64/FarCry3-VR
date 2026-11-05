// C4/mine presentation is enabled only when the native backend verifies the
// corresponding native throw service. Never feed either through the frag launcher.
#include "throw_ied_c4_clips.inl"
#include "throw_proxmine_clips.inl"
struct PlaceableState177 {
 bool grip=false,previousRemoteGrip=false,previousRemoteTrigger=false,turnHeld=false,armed=false,placed=false,pending=false,failed=false,unavailable=false,physicsWarning=false;
 bool rearm181=false;unsigned prepare181=0;ULONGLONG adopted184=0;ULONGLONG releasedItem180=0,releasedAt180=0;double turn=0,turnBase=0;unsigned final180=0,expired180=0,completion=0,rejected=0,consumed=0,correctionFailed=0;ULONGLONG detonateAt=0;Mat4 turnStart={};
}g_place177;
struct PlaceableReturn179 {ULONGLONG at=0;LONG frame=0;}g_placeReturn179;
void TickPlaceableReturn179(ULONGLONG now){
 if(!g_placeReturn179.at)return;
 if(g_weaponMechanicFrame>=g_placeReturn179.frame&&g_frame-g_weaponMechanicFrame<=2){g_placeReturn179={};return;}
 if(now>=g_placeReturn179.at){g_placeReturn179={};fc3pad::RequestPreviousWeapon179();}
}
fc3throw::PlaceableKind177 PlaceableKind177(int kind){return kind==3?fc3throw::C4177:kind==4?fc3throw::Mine177:fc3throw::None177;}
bool PlaceableSupported177(int kind){return kind>=3&&kind<=4&&fc3throw::PlaceableNativeStatus177(PlaceableKind177(kind)).supported;}
int PlaceableGeometry177(ID3D11DeviceContext* c){
 const auto g=NativeWeaponGeometry(c,DrawCall{});
 if((g.vertexBytes==143132&&g.indexBytes==32760&&(g.stride==40||g.stride==28))||
    (g.vertexBytes==49632&&g.indexBytes==12726&&(g.stride==32||g.stride==20)))return 3;
 if(g.vertexBytes==119864&&g.indexBytes==21396&&(g.stride==40||g.stride==28))return 4;
 return 0;
}
bool PlaceableCharge177(ID3D11DeviceContext* c){const auto g=NativeWeaponGeometry(c,DrawCall{});return g.vertexBytes==143132&&g.indexBytes==32760;}
void ResetPlaceable177(){g_place177={};g_place177.previousRemoteGrip=fc3pad::Latest().rightGrip>.7f;g_place177.previousRemoteTrigger=fc3pad::Latest().rightTrigger>.3f;fc3throw::PublishPlaceableContext177(0,0,false);fc3throw::CancelPlaceable177();}
bool EmptyPlaceableReturn179(int kind,const fc3throw::PlaceableStatus177& status){
 // A live C4 charge still needs its remote even when inventory is exhausted.
 return g_place177.placed&&!g_place177.pending&&status.stockKnown179&&status.stock179==0&&
  (kind==4||(kind==3&&g_place177.detonateAt&&status.placedCount179==0));
}
void ReturnPlaceableWeapon179(){
 Log("[THROW180] kind%d returning to previous weapon",g_throw144.kind);
 g_throwArm150={g_throw144.epoch,g_frame,GetTickCount64()+2500};BeginRetire181(g_throw144.kind,"return to previous weapon");
 ResetThrowable144("empty placeable completed");g_throw144.ignoreUntil=GetTickCount64()+1500;
 g_placeReturn179={GetTickCount64()+250,g_frame};fc3pad::RequestPreviousWeapon179(false);
}
bool RecheckPlaceableStock179(const fc3throw::PlaceableStatus177& status){
 if(!g_throw144.owned||g_throw144.kind<3||!EmptyPlaceableReturn179(g_throw144.kind,status))return false;
 ReturnPlaceableWeapon179();return true;
}
void ReceiptPlaceable177(const fc3throw::PlaceableStatus177& status){
 auto& p=g_place177;if(!p.pending)return;
 if(status.completed!=p.completion||status.consumed178!=p.consumed){p.pending=false;p.placed=true;p.failed=false;p.turnHeld=false;p.physicsWarning=status.correctionFailed178!=p.correctionFailed;
  if(p.physicsWarning)Log("[THROW178] native item consumed but velocity/transform correction not acknowledged; retire held visual, never retry replacement item");
  p.releasedAt180=GetTickCount64();if(g_throw144.kind==4){g_throw144.launched=true;g_throw144.releaseAt=GetTickCount64();g_throwArm150={g_throw144.epoch,g_frame,GetTickCount64()+2500};
   // 181: a direct native toss never fires the animation event that makes the mine strategy prepare its next item. Ask for it now.
   fc3throw::RequestPreparePlaceable181(fc3throw::Mine177);p.prepare181=1;Log("[THROW181] mine tossed: native next-mine prepare requested");}
  Log("[PLACE177] native throw acknowledged kind%d completed%u",g_throw144.kind,status.completed);
  Log("[THROW179] receipt kind%d stockKnown%d stock%u placed%u preserved%u",g_throw144.kind,status.stockKnown179,status.stock179,status.placedCount179,status.preservedStock179);
  Log("[THROW179] physics receipt kind%d component%08X vtable%08X setter%08X requested %.3f/%.3f/%.3f finalVelocity%u body%08X deferred%u expired%u",g_throw144.kind,status.physics179,status.physicsVtable179,status.velocitySetter179,status.requestedVelocity179[0],status.requestedVelocity179[1],status.requestedVelocity179[2],status.finalVelocity179,status.physicsBody179,status.velocityDeferred179,status.velocityExpired179);
  if(EmptyPlaceableReturn179(g_throw144.kind,status))ReturnPlaceableWeapon179();
 }else if(status.rejected!=p.rejected){p.pending=false;p.failed=true;p.turnHeld=false;Log("[PLACE177] native throw rejected kind%d; regrip to retry",g_throw144.kind);}
}
void AdvancePlaceable180(const fc3throw::PlaceableStatus177& status,ULONGLONG now){
 auto& s=g_throw144;auto& p=g_place177;if(!p.placed||p.pending)return;
 g_throwArm150={s.epoch,g_frame,now+2500};
 if(EmptyPlaceableReturn179(s.kind,status)){ReturnPlaceableWeapon179();return;}
 const ULONGLONG age=p.releasedAt180&&now>=p.releasedAt180?now-p.releasedAt180:0;
 // 181: the next native item is adopted automatically for BOTH kinds, no grip
 // chord. Adopt as soon as the native replacement exists; a fresh
 // squeeze of the throwing grip is required before the next throw.
 const bool next=status.stockKnown179&&status.stock179>0&&status.prepared&&status.item&&status.item!=p.releasedItem180;
 if(next){
  const ULONGLONG item=status.item;
  ResetPlaceable177();p.rearm181=true;p.adopted184=now;s.launched=false;s.history.Clear();s.started=now;s.gripped=false;
  if(s.kind==3)fc3throw::RequestPreparePlaceable181(fc3throw::C4177); // engine creates the charge itself but did not present it; present it now
  Log("[THROW181] next native %s in hand %llu ms after the toss (item%016llX); squeeze %s GRIP again before throwing; first-person draws seen in the last 2 frames: %s",s.kind==3?"C4 charge":"mine",age,item,s.kind==3?"LEFT":"RIGHT",g_frame-s.seen<=2?"yes":"NO");return;
 }
 if(s.kind==4&&p.prepare181==1&&age>=600){p.prepare181=2;fc3throw::RequestPreparePlaceable181(fc3throw::Mine177);Log("[THROW181] no next mine after %llu ms; second native prepare request",age);}
 // If native preparation never supplies a new mine, do not strand empty hands.
 if(s.kind==4&&p.releasedAt180&&age>2500){Log("[THROW181] native replacement mine unavailable after %llu ms; return to weapon instead of retaining empty pose",age);ReturnPlaceableWeapon179();return;}
}
void PausePlaceable177(){
 if(g_place177.pending)ReceiptPlaceable177(fc3throw::PlaceableNativeStatus177(PlaceableKind177(g_throw144.kind)));
 if(g_place177.pending){g_place177.pending=false;g_place177.failed=true;Log("[PLACE177] queued throw cancelled on tracking/context pause");}
 g_place177.grip=false;g_place177.turnHeld=false;if(!g_place177.armed)g_place177.turn=0;g_place177.previousRemoteGrip=fc3pad::Latest().rightGrip>.7f;g_place177.previousRemoteTrigger=fc3pad::Latest().rightTrigger>.3f;fc3throw::PublishPlaceableContext177(0,0,false);}
const float* PlaceableRight177(){return PlaceableVisualKind183()==3?kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].rightWrist:kThrowMineClipA[kThrowMineAIdx_rest].rightWrist;}
bool PlaceableControllerHand177(int side,Mat4& cameraHand){
 if(!(side?g_handLeftValid:g_handRightValid))return false;
 if(!side){SeedControllerHand124();if(g_freeRightEpoch!=g_armPoseEpoch)return false;
  Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];cameraHand=Multiply(ArmBodyToCamera(),Multiply(controller,g_freeRightFromController));return true;}
 const auto& a=g_freeHandAttachment[1];if(a.epoch!=g_armPoseEpoch||a.profile!=g_weaponMechanicProfile||g_frame-a.frame>2)return false;
 Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[1][i];cameraHand=Multiply(ArmBodyToCamera(),Multiply(controller,a.relative));return true;
}
bool PlaceableFrame177(int side,Mat4& frame){
 Mat4 hand={},inverse={};const float* native=side?kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].leftWrist:PlaceableRight177();
 if(!PlaceableControllerHand177(side,hand)||!Invert(ThrowMat144(native),inverse))return false;
 frame=Multiply(hand,inverse);return true;
}
Mat4 MineKeyRoot177(int i){Mat4 inverse={};if(!Invert(ThrowMat144(kThrowMineClipA[i].part[0]),inverse))return HandIdentity();return Multiply(ThrowMat144(kThrowMineClipA[kThrowMineAIdx_rest].part[0]),inverse);}
void MineSample177(Mat4& wrist,float* fingers,Mat4* parts){
 if(!g_place177.turnHeld&&!g_place177.armed&&g_place177.turn<=0){const auto& key=kThrowMineClipA[kThrowMineAIdx_rest];wrist=ThrowMat144(key.leftWrist);if(fingers)memcpy(fingers,key.leftFingers,sizeof(key.leftFingers));if(parts)for(int i=0;i<7;++i)parts[i]=ThrowMat144(key.part[i+1]);return;}
 const double f=(std::max)(0.0,(std::min)(1.0,g_place177.turn));
 const double t=kThrowMineClipA[kThrowMineAIdx_turnStart].t+f*(kThrowMineClipA[kThrowMineAIdx_turnDone].t-kThrowMineClipA[kThrowMineAIdx_turnStart].t);
 int a=kThrowMineAIdx_turnStart;while(a<kThrowMineAIdx_turnDone&&kThrowMineClipA[a+1].t<=t)++a;
 const int b=(std::min)(a+1,kThrowMineAIdx_turnDone);const double span=kThrowMineClipA[b].t-kThrowMineClipA[a].t,w=span>1e-6?(t-kThrowMineClipA[a].t)/span:0;
 const Mat4 rootA=MineKeyRoot177(a),rootB=MineKeyRoot177(b);
 wrist=PistolRigidBlend(Multiply(rootA,ThrowMat144(kThrowMineClipA[a].leftWrist)),Multiply(rootB,ThrowMat144(kThrowMineClipA[b].leftWrist)),w);
 if(fingers)for(int i=0;i<17;++i){const Mat4 pose=PistolRigidBlend(BoneMatrix(kThrowMineClipA[a].leftFingers,i),BoneMatrix(kThrowMineClipA[b].leftFingers,i),w);for(int j=0;j<12;++j)fingers[i*12+j]=float(pose.m[j]);}
 if(parts)for(int i=0;i<7;++i)parts[i]=PistolRigidBlend(Multiply(rootA,ThrowMat144(kThrowMineClipA[a].part[i+1])),Multiply(rootB,ThrowMat144(kThrowMineClipA[b].part[i+1])),w);
}
V3 MineTurnAxis177(double& travel){
 // part3 is palette slot2, the actual turning knob. part2/slot1 is fixed.
 Mat4 inverse={};const Mat4 a=Multiply(MineKeyRoot177(kThrowMineAIdx_turnStart),ThrowMat144(kThrowMineClipA[kThrowMineAIdx_turnStart].part[3])),b=Multiply(MineKeyRoot177(kThrowMineAIdx_turnDone),ThrowMat144(kThrowMineClipA[kThrowMineAIdx_turnDone].part[3]));
 if(!Invert(a,inverse)){travel=0;return {};}
 const Mat4 delta=Multiply(b,inverse);const V3 vector={delta.m[9]-delta.m[6],delta.m[2]-delta.m[8],delta.m[4]-delta.m[1]};
 travel=atan2(Length(vector),delta.m[0]+delta.m[5]+delta.m[10]-1);return Unit(vector);
}
double MineTwist177(const Mat4& initial,const Mat4& current){
 // A natural wrist twist need not line up perfectly with the authored knob axis.
 // Thirty degrees in either direction traverses the captured 45-degree mechanism.
 Mat4 inverse={};if(!Invert(initial,inverse))return 0;
 const Mat4 delta=Multiply(current,inverse);const V3 vector={delta.m[9]-delta.m[6],delta.m[2]-delta.m[8],delta.m[4]-delta.m[1]};
 return (std::max)(0.0,(std::min)(1.0,atan2(Length(vector),delta.m[0]+delta.m[5]+delta.m[10]-1)/.523598775598));
}
bool PlaceableArm177(int side,const Mat4& toView,const float* bones,Mat4& move){
 if(g_throw144.kind!=4||!ThrowableLease144()||side!=1||!g_place177.turnHeld)return false;
 Mat4 frame={},wrist={},inverse={};if(!PlaceableFrame177(0,frame)||!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 MineSample177(wrist,nullptr,nullptr);move=Multiply(Multiply(frame,wrist),inverse);return true;
}
bool PlaceableFingers177(int side,float* bones){
 const int kind=PlaceableVisualKind183();if(kind<3||!(ThrowableLease144()||ThrowableRetiring181()))return false;
 const float* grip=nullptr;float fingers[204];Mat4 unused={};
 if(kind==3){if(side&&g_place177.placed)return false;grip=side?kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].leftFingers:kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].rightFingers;
  if(!side&&g_place177.detonateAt){const ULONGLONG age=GetTickCount64()-g_place177.detonateAt;if(age<240){const double f=age<120?age/120.0:(240-age)/120.0;for(int i=0;i<17;++i){const Mat4 pose=PistolRigidBlend(BoneMatrix(grip,i),BoneMatrix(kThrowC4ClipDetonate[kThrowC4DetonateIdx_thumbPress].rightFingers,i),f);for(int j=0;j<12;++j)fingers[i*12+j]=float(pose.m[j]);}grip=fingers;}}}
 else if(!side)grip=kThrowMineClipA[kThrowMineAIdx_rest].rightFingers;
 else if(g_place177.turnHeld){MineSample177(unused,fingers,nullptr);grip=fingers;}
 if(!grip)return false;const Mat4 wrist=BoneMatrix(bones,2);for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(grip,b-3)));return true;
}
bool PlaceableDrawHidden177(ID3D11DeviceContext* c){return g_throw144.kind==3&&g_place177.placed&&PlaceableCharge177(c);}
ID3D11Buffer* PlaceablePalette177(ID3D11DeviceContext* c,UINT start,UINT count){
 ClearMirrorPalette338();
 const int kind=PlaceableVisualKind183();if(kind<3||!g_weaponPaletteCb)return nullptr;
 const auto geometry=NativeWeaponGeometry(c,DrawCall{});if(geometry.stride!=40||(kind==3&&!PlaceableCharge177(c)))return nullptr;
 float bones[576]={};Mat4 to={},from={},frame={};if(!DrawSpace(c,bones,to,from)||!PlaceableFrame177(kind==3?1:0,frame))return nullptr;
 if(kind==3){const auto& key=kThrowC4ClipThrow[kThrowC4ThrowIdx_settled];for(int i=0;i<3;++i)StoreBone(bones,i,Multiply(from,Multiply(frame,ThrowMat144(key.part[i+1]))));}
 else{Mat4 wrist={},parts[7];MineSample177(wrist,nullptr,parts);for(int i=0;i<7;++i)StoreBone(bones,i,Multiply(from,Multiply(frame,parts[i])));}
 const bool own=g_own;g_own=true;g_oUpdateSubresource(c,g_weaponPaletteCb,0,nullptr,bones,0,0);g_weaponPaletteCb->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);g_own=own;return g_weaponPaletteCb;
}
ID3D11Buffer* PlaceableWorld177(ID3D11DeviceContext* c,UINT start,UINT count){
 const int kind=PlaceableVisualKind183();if(kind<3)return nullptr;const bool charge=kind==3&&PlaceableCharge177(c);int part=0;
 if(kind==3&&!charge){part=start==4875?5:start==5517?6:start==246?7:4;}
 CameraSummary camera={};Mat4 frame={},inverse={};if(!PistolGuideCamera(c,camera)||!PlaceableFrame177(charge?1:0,frame)||!Invert(HullView(camera),inverse))return nullptr;
 Mat4 local=ThrowMat144(kind==3?kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].part[part]:kThrowMineClipA[kThrowMineAIdx_rest].part[0]);
 const ULONGLONG remoteAge=g_place177.detonateAt?GetTickCount64()-g_place177.detonateAt:240;
 if(kind==3&&part==7&&remoteAge<240){Mat4 ri={};const auto& key=kThrowC4ClipDetonate[kThrowC4DetonateIdx_rtPress];if(Invert(ThrowMat144(key.part[4]),ri))local=PistolRigidBlend(local,Multiply(ThrowMat144(kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].part[4]),Multiply(ri,ThrowMat144(key.part[7]))),double(remoteAge<120?remoteAge:240-remoteAge)/120.0);}
 ID3D11Buffer* cb=nullptr;c->VSGetConstantBuffers(0,1,&cb);float rows[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,rows,sizeof(rows));D3D11_BUFFER_DESC desc={};if(cb)cb->GetDesc(&desc);SafeRelease(cb);
 if(bytes<80||bytes!=desc.ByteWidth)return nullptr;WritePistolWorld(rows,Multiply(inverse,Multiply(frame,local)));
 auto& s=g_throw144;int slot=-1;for(int i=0;i<8;++i)if(s.worldBytes[i]==bytes){slot=i;break;}
 if(slot<0)for(int i=0;i<8;++i)if(!s.world[i]){desc.Usage=D3D11_USAGE_DEFAULT;desc.CPUAccessFlags=0;desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;desc.MiscFlags=0;desc.StructureByteStride=0;if(SUCCEEDED(g_oCreateBuffer(g_dev,&desc,nullptr,&s.world[i]))){slot=i;s.worldBytes[i]=bytes;}break;}
 if(slot<0)return nullptr;const bool own=g_own;g_own=true;g_oUpdateSubresource(c,s.world[slot],0,nullptr,rows,0,0);s.world[slot]->SetPrivateData(kCbCopyGuid,bytes,rows);g_own=own;return s.world[slot];
}
void TickMineTurn177(){
 auto& s=g_place177;const auto c=fc3pad::Latest();if(!g_handLeftValid||!c.active||c.rightGrip<.3f||c.leftGrip<.3f){s.turnHeld=false;return;}
 Mat4 frame={},inverse={},left={};if(!PlaceableFrame177(0,frame)||!PlaceableControllerHand177(1,left)||!Invert(frame,inverse)){s.turnHeld=false;return;}
 const Mat4 local=Multiply(inverse,left);const V3 palm=Apply(local,{-.6781,.0519,1.0387});const V3 target=Apply(Multiply(MineKeyRoot177(kThrowMineAIdx_turnStart),ThrowMat144(kThrowMineClipA[kThrowMineAIdx_turnStart].leftWrist)),{-.6781,.0519,1.0387});
 if(!s.turnHeld&&!s.armed&&c.leftGrip>.7f&&Length(Sub(palm,target))<=.13){s.turnHeld=true;s.turnStart=local;s.turnBase=s.turn;Log("[PLACE177] mine knob acquired by LEFT grip");}
 if(s.turnHeld&&!s.armed){s.turn=(std::min)(1.0,s.turnBase+MineTwist177(s.turnStart,local));if(s.turn>=.99){s.turn=1;s.armed=true;Log("[PLACE177] mine manual turn reached captured endpoint; ready for physical throw");}}
}
bool RemotePress177(float grip,float trigger){
 const bool pressed=trigger>.7f&&!g_place177.previousRemoteTrigger&&grip>.7f;
 if(trigger>.7f)g_place177.previousRemoteTrigger=true;else if(trigger<.3f)g_place177.previousRemoteTrigger=false;
 return pressed;
}
bool TickPlaceable177(){
 auto& s=g_throw144;if(s.kind<3)return false;auto& p=g_place177;const auto c=fc3pad::Latest();const auto status=fc3throw::PlaceableNativeStatus177(PlaceableKind177(s.kind));
 if(status.finalVelocity179!=p.final180||status.velocityExpired179!=p.expired180){p.final180=status.finalVelocity179;p.expired180=status.velocityExpired179;Log("[THROW180] post-update kind%d velocity commits%u expired%u body%08X",s.kind,p.final180,p.expired180,status.physicsBody179);}
 if(!status.supported){PausePlaceable177();p.unavailable=true;fc3pad::SetThrowableInput144(true,false);return true;}p.unavailable=false;
 if(s.kind==3&&!p.placed&&!g_handLeftValid){PausePlaceable177();fc3pad::SetThrowableInput144(true,false);return true;}
 ULONGLONG owner=0;if(!fc3throw::CurrentLocalOwner144(owner)){PausePlaceable177();fc3pad::SetThrowableInput144(true,false);return true;}
 fc3throw::PublishPlaceableContext177(owner,s.epoch,true);fc3pad::SetThrowableInput144(true,false);
 ReceiptPlaceable177(status);
 if(!s.owned)return true;
 if(p.adopted184&&GetTickCount64()-p.adopted184>=300){p.adopted184=0;
  const bool drawn=s.kind==3?g_frame-g_chargeDraw184.frame<=3:g_frame-s.seen<=3;
  Log("[THROW185] next %s geometry %s observed 300 ms after adoption (draw census only; hand visibility unverified)",s.kind==3?"C4 charge":"mine",drawn?"was":"was NOT");}
 if(EmptyPlaceableReturn179(s.kind,status)){ReturnPlaceableWeapon179();return true;}
 if(s.kind==4&&!p.pending&&!p.placed)TickMineTurn177();
 const float grip=s.kind==3?c.leftGrip:c.rightGrip;
 Mat4 frame={},inverse={},root={};const bool frameReady=g_frameCameraValid&&PlaceableFrame177(s.kind==3?1:0,frame)&&Invert(HullView(g_frameCamera),inverse);
 const auto now=GetTickCount64();
 if(frameReady&&!p.pending&&!p.placed){
  root=Multiply(inverse,Multiply(frame,ThrowMat144(s.kind==3?kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].part[0]:kThrowMineClipA[kThrowMineAIdx_rest].part[0])));
  const V3 position=Apply(root,{0,0,0}),camera={g_frameCamera.position[0],g_frameCamera.position[1],g_frameCamera.position[2]};
  s.history.Add({now,owner,s.epoch,{position.x,position.y,position.z},{position.x-camera.x,position.y-camera.y,position.z-camera.z},true});
 }
 if(grip<.3f)p.rearm181=false;
 if(grip>.7f&&!p.rearm181)p.grip=true;
 if(grip<.3f&&p.grip&&!p.pending&&!p.placed){
  p.grip=false;p.turnHeld=false;p.failed=true;
  if((s.kind==3||p.armed)&&status.prepared&&frameReady){
   fc3manual::Release release;fc3manual::ReleaseMotion152 motion={};
   const double boost=ThrowableBoost167();
   if(!s.history.AtRelease(now,release,1,&motion,boost,s.arcAssist156)){
    const V3 position=Apply(root,{0,0,0});release={now,owner,s.epoch,{position.x,position.y,position.z},{0,0,0}};
   }
   const double speed180=fc3manual::Length(release.velocity);if(speed180>12)release.velocity=release.velocity*(12/speed180);
   float native[16]={};for(int r=0;r<4;++r)for(int col=0;col<4;++col)native[r*4+col]=float(root.m[col*4+r]);
   native[12]=float(release.origin.x);native[13]=float(release.origin.y);native[14]=float(release.origin.z);
   const float velocity[3]={float(release.velocity.x),float(release.velocity.y),float(release.velocity.z)};
   p.releasedItem180=status.item;p.completion=status.completed;p.rejected=status.rejected;p.consumed=status.consumed178;p.correctionFailed=status.correctionFailed178;p.pending=fc3throw::QueueThrowPlaceable178(PlaceableKind177(s.kind),owner,s.epoch,native,velocity);p.failed=!p.pending;
   Log("[THROW178] release kind%d owner%016llX item%016llX queued%d boost%.2f speed%.3f assisted%d",s.kind,owner,status.item,p.pending,boost,fc3manual::Length(release.velocity),motion.assisted?1:0);
   Log("[THROW179] release kind%d origin %.3f/%.3f/%.3f velocity %.3f/%.3f/%.3f sampled %.3f selected %.3f strokeAge%llu",s.kind,release.origin.x,release.origin.y,release.origin.z,release.velocity.x,release.velocity.y,release.velocity.z,motion.sampledSpeed,motion.selectedSpeed,motion.strokeAge);
  }
 }
 const bool remote=RemotePress177(c.rightGrip,c.rightTrigger);if(s.kind==3&&remote&&(p.placed||status.placedCount179>0)){const bool queued=fc3throw::QueueDetonate177(owner,s.epoch);if(queued)p.detonateAt=GetTickCount64();Log("[THROW178] RIGHT GRIP + fresh RT remote press owner%016llX queued%d",owner,queued);}
 return true;
}
bool PlaceableNote177(){
 if(!g_throw144.owned||g_throw144.kind<3)return false;
 if(g_place177.unavailable)fc3xr::SetTestNote(L"MANUAL THROW UNAVAILABLE\nNative throw service did not verify. Use the wheel to stow this item.");
 else if(g_place177.physicsWarning)fc3xr::SetTestNote(g_throw144.kind==3?L"C4 RELEASED - THROW CORRECTION FAILED\nHold RIGHT GRIP and squeeze RT to detonate.\nThis item has already left your inventory.":L"MINE RELEASED - THROW CORRECTION FAILED\nThis item has already left your inventory.");
 else if(g_place177.pending)fc3xr::SetTestNote(L"RELEASING\nWaiting for native throw acknowledgement.");
 else if(g_place177.failed)fc3xr::SetTestNote(g_throw144.kind==4&&!g_place177.armed?L"MINE NOT ARMED\nHold RIGHT GRIP again. Use LEFT GRIP at the knob and twist to its stop.":L"THROW NOT ACCEPTED\nGrip again, then release to retry.");
 else if(g_throw144.kind==3)fc3xr::SetTestNote(g_place177.placed?L"C4 REMOTE\nHold RIGHT GRIP and squeeze RT to detonate.\nThe next charge comes to your LEFT hand by itself. Y: previous weapon. LEFT GRIP + Y: wheel.":L"C4\nHold LEFT GRIP, swing and release to throw.\nRIGHT GRIP + RT detonates placed charges. Y: previous weapon. LEFT GRIP + Y: wheel.");
 else if(g_place177.armed)fc3xr::SetTestNote(L"MINE ARMED\nSwing and release RIGHT GRIP to throw. Release LEFT GRIP to free your hand.");
 else if(g_place177.turnHeld)fc3xr::SetTestNote(L"TURN THE MINE KNOB\nKeep LEFT GRIP held and twist your wrist either way until it stops.");
 else fc3xr::SetTestNote(L"PROXIMITY MINE\nHold RIGHT GRIP. Bring LEFT HAND to the knob,\nhold LEFT GRIP and twist either way to arm. Release RIGHT GRIP to throw.\nUnarmed release keeps the mine in hand. Y: previous weapon. LEFT GRIP + Y: wheel.");
 return true;
}
