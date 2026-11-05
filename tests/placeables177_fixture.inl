// Tests actual captured part transforms and uploaded palettes. Native service
// acceptance is covered separately; a cosmetic pose is not placement proof.
void TestPlaceables177(ID3D11Device* device,ID3D11DeviceContext* context){
 const auto oldReturn=g_placeReturn179;const auto oldArm=g_throwArm150;const auto oldInput=fc3pad::Latest();const auto oldThrow=g_throw144;const auto oldPlace=g_place177;const auto oldAttachment=g_freeHandAttachment[1];const auto oldRight=g_freeRightFromController;const auto oldRightEpoch=g_freeRightEpoch;
 auto equal=[](const Mat4& a,const Mat4& b){for(int i=0;i<12;++i)if(fabs(a.m[i]-b.m[i])>4e-4)return false;return true;};
 auto identity=HandIdentity();g_throw144.owned=true;g_throw144.epoch=g_armPoseEpoch;g_throw144.launched=false;g_throw144.releasing=false;
 g_freeRightFromController=identity;g_freeRightEpoch=g_armPoseEpoch;g_freeHandAttachment[1].relative=identity;g_freeHandAttachment[1].frame=g_frame;g_freeHandAttachment[1].epoch=g_armPoseEpoch;g_freeHandAttachment[1].profile=g_weaponMechanicProfile;
 struct Mesh177 {UINT vb,ib,stride;int kind;};const Mesh177 meshes[]={{143132,32760,40,3},{49632,12726,32,3},{119864,21396,40,4},{119864,21396,32,0}};
 for(const auto& mesh:meshes){auto* vb=Buffer(device,mesh.vb,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,mesh.ib,D3D11_BIND_INDEX_BUFFER);UINT stride=mesh.stride,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
  Check(PlaceableGeometry177(context)==mesh.kind,"177 C4 charge, remote and mine use exact captured geometry; wrong stream rejected");
  if(mesh.kind&&mesh.stride==40){g_throw144.kind=mesh.kind;g_place177={};Mat4 frame={},to={},from={};float before[576]={};Check(DrawSpace(context,before,to,from)&&PlaceableFrame177(mesh.kind==3?1:0,frame),"177 controller-owned placeable frame available");
   auto* uploaded=PlaceablePalette177(context,mesh.kind==3?0:135,mesh.kind==3?216:10563);Check(uploaded!=nullptr,"177 native skinned placeable uploads tracked palette");float out[576]={};Read(device,context,uploaded,out);
   const int bones=mesh.kind==3?3:7;for(int i=0;i<bones;++i){const Mat4 source=ThrowMat144(mesh.kind==3?kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].part[i+1]:kThrowMineClipA[kThrowMineAIdx_rest].part[i+1]);Check(equal(Multiply(to,BoneMatrix(out,i)),Multiply(frame,source)),"177 actual GPU palette retains native part-to-holding-wrist attachment");}
   Mat4 sourceInverse={},tracked={};const float* sourcePart=mesh.kind==3?kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].part[1]:kThrowMineClipA[kThrowMineAIdx_rest].part[1];const float* wrist=mesh.kind==3?kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].leftWrist:kThrowMineClipA[kThrowMineAIdx_rest].rightWrist;
   Check(Invert(ThrowMat144(sourcePart),sourceInverse)&&PlaceableControllerHand177(mesh.kind==3?1:0,tracked)&&equal(Multiply(Multiply(Multiply(to,BoneMatrix(out,0)),sourceInverse),ThrowMat144(wrist)),tracked),"177 rendered charge meets LEFT wrist; rendered mine meets RIGHT wrist");
  }
  if(mesh.kind==3&&mesh.stride==32){g_throw144.kind=3;Mat4 frame={},viewInverse={};CameraSummary camera={};Check(PlaceableFrame177(0,frame)&&PistolGuideCamera(context,camera)&&Invert(HullView(camera),viewInverse),"177 tracked remote frame available");auto* uploaded=PlaceableWorld177(context,0,894);Check(uploaded!=nullptr,"177 C4 remote gets actual rigid world upload");float out[kSmallCbMax/4]={};Read(device,context,uploaded,out);Check(equal(HullWorld(out),Multiply(viewInverse,Multiply(frame,ThrowMat144(kThrowC4ClipThrow[kThrowC4ThrowIdx_settled].part[4])))),"177 actual rigid remote upload follows RIGHT controller");}
  context->IASetVertexBuffers(0,0,nullptr,nullptr,nullptr);context->IASetIndexBuffer(nullptr,DXGI_FORMAT_R16_UINT,0);SafeRelease(vb);SafeRelease(ib);
 }
 g_place177={};g_place177.turnHeld=true;Mat4 wrist={},partsA[7],partsB[7];g_place177.turn=0;MineSample177(wrist,nullptr,partsA);const Mat4 firstHand=wrist;
 g_place177.turn=1;MineSample177(wrist,nullptr,partsB);const Mat4 lastHand=wrist;g_place177.turn=4;MineSample177(wrist,nullptr,nullptr);Check(equal(lastHand,wrist),"177 mine hand stops at captured full-turn endpoint instead of continuing with controller");
 g_place177.turn=-2;MineSample177(wrist,nullptr,nullptr);Check(equal(firstHand,wrist),"177 mine hand cannot reverse past mechanism start");
 Check(!equal(partsA[2],partsB[2]),"177 actual knob bone moves during manual twist");
 g_place177.turn=.5;g_place177.turnHeld=true;Mat4 partialParts[7],releasedParts[7];MineSample177(wrist,nullptr,partialParts);g_place177.turnHeld=false;MineSample177(wrist,nullptr,releasedParts);
 Check(equal(partialParts[2],releasedParts[2]),"178 releasing left grip frees hand without resetting partial knob position");
 double travel=0;const V3 axis=MineTurnAxis177(travel);Check(travel>.70&&travel<.86,"177 captured mine knob has approximately45-degree bounded travel");
 auto rotate=[&](double angle){Mat4 r=HandIdentity();const double c=cos(angle),s=sin(angle),v=1-c;const double x=axis.x,y=axis.y,z=axis.z;r.m[0]=c+x*x*v;r.m[1]=x*y*v-z*s;r.m[2]=x*z*v+y*s;r.m[4]=y*x*v+z*s;r.m[5]=c+y*y*v;r.m[6]=y*z*v-x*s;r.m[8]=z*x*v-y*s;r.m[9]=z*y*v+x*s;r.m[10]=c+z*z*v;return r;};
 Check(fabs(MineTwist177(identity,rotate(.523598775598*.5))-.5)<.002,"178 fifteen-degree wrist turn maps to half captured knob travel");
 Check(MineTwist177(identity,rotate(-travel))==1&&MineTwist177(identity,rotate(travel*1.5))==1,"178 either twist direction arms naturally and excess rotation clamps at endpoint");
 g_place177.grip=true;g_place177.turnHeld=true;g_place177.armed=false;g_place177.turn=.5;PausePlaceable177();Check(!g_place177.grip&&!g_place177.turnHeld&&g_place177.turn==0,"177 tracking/menu pause cannot invent placement release or keep left hand attached");
 g_place177={};Check(!RemotePress177(1,0),"178 right grip alone cannot detonate");
 Check(RemotePress177(1,1)&&!RemotePress177(1,1)&&!RemotePress177(1,.5f),"178 fresh RT while gripping emits one detonation despite jitter");
 Check(!RemotePress177(0,0)&&!RemotePress177(0,1)&&!RemotePress177(1,1),"178 trigger before grip cannot become accidental detonation when grip joins");
 Check(!RemotePress177(1,0)&&RemotePress177(1,1),"178 deliberate repeated detonation needs fresh trigger edge");
 fc3manual::History throwHistory;fc3manual::Release measured;fc3manual::ReleaseMotion152 motion;
 for(unsigned i=0;i<10;++i)throwHistory.Add({1000+i*10,77,12,{i*.03,0,0},{i*.03,0,0},true});
 Check(throwHistory.AtRelease(1090,measured,1,&motion,2,true)&&measured.velocity.x>4.5,"178 shared physical release history gives moving C4 or mine boosted hand velocity");
 throwHistory.Clear();for(unsigned i=0;i<10;++i)throwHistory.Add({1000+i*10,77,12,{0,0,0},{0,0,0},true});
 Check(throwHistory.AtRelease(1090,measured,1,&motion,8,true)&&fc3manual::Length(measured.velocity)<.001,"178 stationary C4 or mine remains a drop even at maximum boost");
 g_throw144.kind=4;g_throw144.launched=false;g_place177={};g_place177.pending=true;g_place177.turnHeld=true;g_place177.completion=7;
 fc3throw::PlaceableStatus177 receipt={};receipt.completed=8;ReceiptPlaceable177(receipt);
 Check(g_place177.placed&&!g_place177.pending&&!g_place177.turnHeld&&g_throw144.launched&&ThrowableArmLease150(),"177 mine acknowledgement retires held mesh, frees knob hand and preserves controller arm cleanup lease");
 Check(!RecheckPlaceableStock179(receipt)&&g_throw144.owned,"179 launch receipt with unsampled stock does not force a weapon switch");
 receipt.stockKnown179=true;receipt.stock179=1;
 Check(!RecheckPlaceableStock179(receipt)&&g_throw144.launched,"179 postreceipt stock recheck preserves a launch while native count is still nonzero");
 receipt.stock179=0;Check(RecheckPlaceableStock179(receipt)&&!g_throw144.owned&&g_placeReturn179.at,"179 later depleted stock sample retires ownership even after launched became true");
 g_placeReturn179={};g_throw144.owned=true;
 g_throw144.launched=false;g_throw144.kind=3;g_place177={};g_place177.pending=true;g_place177.rejected=4;receipt={};receipt.rejected=5;ReceiptPlaceable177(receipt);
 Check(g_place177.failed&&!g_place177.pending&&!g_place177.placed&&!g_throw144.launched,"177 rejected placement retains held item for explicit regrip retry");
 g_place177={};g_place177.pending=true;g_place177.consumed=3;g_place177.correctionFailed=1;g_throw144.kind=3;receipt={};receipt.consumed178=4;receipt.correctionFailed178=2;receipt.rejected=1;ReceiptPlaceable177(receipt);
 Check(g_place177.placed&&!g_place177.pending&&g_place177.physicsWarning&&!g_place177.failed,"178 native consumption retires held item despite correction failure; never retry replacement stock");
 g_place177={};g_place177.pending=true;const auto nativeStatus=fc3throw::PlaceableNativeStatus177(fc3throw::C4177);g_place177.completion=nativeStatus.completed;g_place177.rejected=nativeStatus.rejected;PausePlaceable177();
 Check(!g_place177.pending&&g_place177.failed,"177 pause cancels pending frontend wait so cancelled native request cannot strand item");
 fc3pad::Controllers input={};input.active=true;input.y=true;input.rightTrigger=1;fc3pad::Publish(input);fc3pad::SetThrowableInput144(true,true);
 Check(!(fc3pad::ButtonsOutput()&0x8000)&&fc3pad::FireOutput()==0,"177 owned placeable blocks Y/native throw and gun trigger");
 input.y=false;input.rightTrigger=0;input.leftGrip=0;fc3pad::Publish(input);
 g_place177={};g_place177.placed=true;receipt={};
 Check(!EmptyPlaceableReturn179(4,receipt),"179 unknown stock never requests previous weapon");
 receipt.stockKnown179=true;receipt.stock179=1;
 Check(!EmptyPlaceableReturn179(4,receipt),"179 retained or unlimited stock never triggers empty return");
 receipt.stock179=0;Check(EmptyPlaceableReturn179(4,receipt),"179 empty mine returns after native launch receipt");
 receipt.placedCount179=1;g_place177.detonateAt=GetTickCount64();
 Check(!EmptyPlaceableReturn179(3,receipt),"179 final C4 keeps remote while native placed charge remains");
 receipt.placedCount179=0;Check(EmptyPlaceableReturn179(3,receipt),"179 empty C4 returns after deliberate detonation clears native charges");
 g_throw144.kind=3;g_throw144.owned=true;g_throw144.epoch=g_armPoseEpoch;input.rightTrigger=1;fc3pad::Publish(input);
 ReturnPlaceableWeapon179();
 Check(!g_throw144.owned&&!(fc3pad::ButtonsOutput()&0x8000)&&fc3pad::FireOutput()==0,"179 empty return gives native depletion time to restore firearm before fallback Y");
 fc3pad::SetPlaceableInput180(true);TickPlaceableReturn179(g_placeReturn179.at);
 Check(!g_throw144.owned&&(fc3pad::ButtonsOutput()&0x8000)&&!(fc3pad::ButtonsOutput()&0x200)&&fc3pad::FireOutput()==0&&ThrowableArmLease150(),"179 empty return releases shoulder, sends native Y and shields held remote RT while preserving camera arm cleanup");
 fc3pad::SetPlaceableInput180(false);Sleep(220);Check(!(fc3pad::ButtonsOutput()&0x8000)&&fc3pad::FireOutput()==0,"179 previous weapon pulse ends without repeated toggling; held remote trigger cannot fire restored gun");
 const auto priorMechanicFrame=g_weaponMechanicFrame;g_placeReturn179={GetTickCount64(),g_frame-1};g_weaponMechanicFrame=g_frame;TickPlaceableReturn179(GetTickCount64());
 Check(!g_placeReturn179.at&&!(fc3pad::ButtonsOutput()&0x8000),"179 native firearm restoration cancels fallback instead of toggling back to throwable");g_weaponMechanicFrame=priorMechanicFrame;
 g_throw144.owned=true;g_throw144.kind=4;g_throw144.epoch=g_armPoseEpoch;fc3pad::SetPlaceableInput180(true);input.rightTrigger=0;input.y=false;fc3pad::Publish(input);input.y=true;fc3pad::Publish(input);input.y=false;fc3pad::Publish(input);fc3pad::SetThrowableInput144(true,false);
 TickThrowable144(true);Check(!g_throw144.owned&&(fc3pad::ButtonsOutput()&0x8000)&&!(fc3pad::ButtonsOutput()&0x200)&&ThrowableRetiring181()&&ThrowableArmLease150(),"181 explicit Y stows manual C4 or mine at once, hides its native holster and keeps the tracked arms");
 Check(!ThrowableHidden144()&&PlaceableVisualKind183()==4,"183 retiring item stays visible in the tracked hand while it holsters");g_retire181={};
 // Equipped explosive Y alone selects previous weapon; Left Grip + Y opens the normal wheel.
 input={};input.active=true;input.rightGrip=1;input.leftGrip=0;fc3pad::SetPlaceableInput180(true);fc3pad::SetThrowableInput144(true,false);fc3pad::Publish(input);
 Check(!(fc3pad::ButtonsOutput()&0x200),"180 mine/C4 hold never primes the last grenade or Molotov");
 input.y=true;fc3pad::Publish(input);fc3pad::SetThrowableInput144(false,false);Check(fc3pad::WeaponSelectionRequested()&&(fc3pad::ButtonsOutput()&0x8000)&&!(fc3pad::ButtonsOutput()&(0x200|0x100)),"181 Y press requests the previous weapon immediately; no wheel, heal or grenade");
 Sleep(220);fc3pad::Publish(input);Check(!fc3pad::WeaponSelectionRequested()&&!fc3pad::WeaponWheelRequested145()&&!(fc3pad::ButtonsOutput()&(0x8000|0x200|0x100)),"181 held Y ends after one bounded pulse: no repeat and no hold-Y wheel");
 Sleep(150);fc3pad::Publish(input);Check(!(fc3pad::ButtonsOutput()&(0x8000|0x100)),"181 still-held Y stays silent");
 input.y=false;fc3pad::Publish(input);input.y=true;fc3pad::Publish(input);Check(fc3pad::WeaponSelectionRequested()&&(fc3pad::ButtonsOutput()&0x8000),"181 release then press is a fresh previous-weapon request");
 input.y=false;fc3pad::Publish(input);
 fc3pad::SetDedicatedInteractions(true);fc3pad::SetGameplayControls(true);
 input.leftGrip=1;fc3pad::Publish(input);input.y=true;fc3pad::Publish(input);

 Check(fc3pad::WeaponWheelRequested145()&&fc3pad::WeaponSelectionRequested()&&(fc3pad::ButtonsOutput()&0x100)&&!(fc3pad::ButtonsOutput()&0x8000),"186 mine/C4 Left Grip + Y opens ordinary LB weapon wheel without previous-weapon pulse");
 input.y=false;fc3pad::Publish(input);input.leftGrip=0;input.rightGrip=0;fc3pad::Publish(input);
 fc3pad::SetPlaceableInput180(false);Sleep(220);fc3pad::Publish(input);Check(!(fc3pad::ButtonsOutput()&(0x100|0x8000|0x200)),"181 release neither switches again nor throws prior throwable");
 g_throw144.kind=4;g_throw144.owned=true;g_throw144.launched=true;g_throw144.epoch=g_armPoseEpoch;g_place177={};g_place177.placed=true;g_place177.releasedItem180=101;g_place177.releasedAt180=GetTickCount64()-200;
 receipt={};receipt.stockKnown179=true;receipt.stock179=1;receipt.prepared=true;receipt.item=101;
 AdvancePlaceable180(receipt,GetTickCount64());Check(g_throw144.launched&&g_place177.placed,"180 consumed mine cannot be mistaken for replacement");
 receipt.item=102;AdvancePlaceable180(receipt,GetTickCount64());Check(!g_throw144.launched&&!g_place177.placed&&!g_place177.armed&&g_throw144.owned&&g_place177.rearm181,"181 next native mine restores tracked hold automatically, resets arming and waits for a fresh squeeze");
 g_place177.placed=true;g_place177.releasedItem180=102;g_place177.releasedAt180=GetTickCount64()-2600;AdvancePlaceable180(receipt,GetTickCount64());Check(!g_throw144.owned&&g_placeReturn179.at&&ThrowableRetiring181(),"181 missing native replacement returns gun rather than stranding empty hands");
 g_retire181={};g_placeReturn179={};
 g_throw144.kind=3;g_throw144.owned=true;g_throw144.epoch=g_armPoseEpoch;g_place177={};g_place177.placed=true;g_place177.releasedItem180=201;g_place177.releasedAt180=GetTickCount64()-100;
 receipt={};receipt.stockKnown179=true;receipt.stock179=1;receipt.prepared=true;receipt.item=202;receipt.placedCount179=1;
 AdvancePlaceable180(receipt,GetTickCount64());Check(!g_place177.placed&&g_throw144.owned&&g_place177.rearm181,"185 next C4 adopted immediately without artificial empty-hand delay");
 g_place177.releasedAt180=GetTickCount64()-400;AdvancePlaceable180(receipt,GetTickCount64());Check(!g_place177.placed&&g_throw144.owned&&g_place177.rearm181,"181 next C4 charge arrives in the LEFT hand automatically, no grip chord");
 fc3pad::SetPlaceableInput180(false);fc3pad::SetThrowableInput144(false,false);fc3pad::Publish(oldInput);g_throwArm150=oldArm;
 for(int i=0;i<8;++i)if(g_throw144.world[i]!=oldThrow.world[i])SafeRelease(g_throw144.world[i]);
 g_throw144=oldThrow;g_place177=oldPlace;g_placeReturn179=oldReturn;g_freeHandAttachment[1]=oldAttachment;g_freeRightFromController=oldRight;g_freeRightEpoch=oldRightEpoch;
}
