#include "../src/xr_presenter.cpp"
#include <cstdio>
#include <cstdlib>
extern void SeedInventory303(const fc3select290::Inventory&,int);
namespace fc3visual292 {extern WeaponVisual g_visual[4];extern fc3select290::Inventory g_identity;extern unsigned long long g_cacheTried296[4];}
void Check303(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}printf("PASS: %s\n",why);}
float input303[kInputCount]={};XrPosef pose303[2]={{{0,0,0,1},{.4f,1.2f,-.3f}},{{0,0,0,1},{-.4f,1.2f,-.3f}}};bool tracked303[2]={true,true};
fc3select290::Inventory inventory303;
XrResult XRAPI_CALL Sync303(XrSession,const XrActionsSyncInfo*){return XR_SUCCESS;}
XrResult XRAPI_CALL Vector303(XrSession,const XrActionStateGetInfo* info,XrActionStateVector2f* out){out->isActive=XR_TRUE;out->currentState={input303[size_t(info->action)-1],0};return XR_SUCCESS;}
XrResult XRAPI_CALL Float303(XrSession,const XrActionStateGetInfo* info,XrActionStateFloat* out){out->isActive=XR_TRUE;out->currentState=input303[size_t(info->action)-1];return XR_SUCCESS;}
XrResult XRAPI_CALL Boolean303(XrSession,const XrActionStateGetInfo* info,XrActionStateBoolean* out){out->isActive=XR_TRUE;out->currentState=input303[size_t(info->action)-1]>.5f;return XR_SUCCESS;}
XrResult XRAPI_CALL Locate303(XrSpace space,XrSpace,XrTime,XrSpaceLocation* out){const int hand=space==static_cast<XrSpace>(10)?0:1;out->pose=pose303[hand];out->locationFlags=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;if(tracked303[hand])out->locationFlags|=XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;return XR_SUCCESS;}
void Tick303(int result=0){inventory303.at=GetTickCount64();SeedInventory303(inventory303,result);fc3xr::SetHandsContext303(true,true,1);SyncControllers();}
void Move303(int hand,int slot){const auto centre=Palm292(RackHandle292(slot));const auto offset=testnotes::Rotate(pose303[hand].orientation,{hand?-.0871f:.0871f,.0409f,-.1103f});pose303[hand].position=testnotes::Sub(centre,offset);}
void Neutral303(){input303[kRightGrip]=input303[kLeftGrip]=input303[kRightTrigger]=input303[kLeftTrigger]=0;Tick303();}
int main(){
 g_handsLoaded303=g_holstersLoaded297=true;g_handSettings303={};g_holsters297={};
 g_actionsReady=g_sessionRunning=g_viewsTracked=g_referenceValid=true;g_session=static_cast<XrSession>(1);
 g_views[0].pose=g_views[1].pose={{0,0,0,1},{0,1.6f,0}};g_selectPose290=g_views[0].pose;
 g_bodyPlaced297=g_rackBasisReady292=true;g_gripRelative297={0,0,0,1};g_handSpaces[0]=static_cast<XrSpace>(10);g_handSpaces[1]=static_cast<XrSpace>(11);
 p_xrSyncActions=Sync303;p_xrGetActionStateVector2f=Vector303;p_xrGetActionStateFloat=Float303;p_xrGetActionStateBoolean=Boolean303;p_xrLocateSpace=Locate303;
 for(int i=0;i<kInputCount;++i)g_actions[i]=static_cast<XrAction>(i+1);
 for(int h=0;h<2;++h){const float palm[]={h?-.0871f:.0871f,.0409f,.1103f};fc3xr::SetHandPalm303(h,palm,1);}
 inventory303.valid=true;inventory303.owner=123;inventory303.equipped=100;
 for(int i=0;i<4;++i){auto& w=inventory303.weapons[i];w.entity=100+i;w.wrapper=200+i;w.id=300+i;auto& v=fc3visual292::g_visual[i];v.owner=123;v.id=w.id;v.entity=w.entity;v.ready=true;fc3visual292::g_cacheTried296[i]=w.id;}
 fc3visual292::g_identity=inventory303;fc3pad::SetGameplayControls(true);
 Tick303();Check303(fc3hands303::Read().enabled&&fc3hands303::Read().empty,"shipping holster-first mode begins empty with all four native identities retained");
 input303[kRightTrigger]=1;Tick303();Check303(fc3pad::FireOutput()==0,"empty hands cannot fire from right trigger");input303[kRightTrigger]=0;input303[kLeftTrigger]=1;Tick303();Check303(fc3pad::FireOutput()==0,"empty hands cannot fire from left trigger");Neutral303();
 Move303(1,0);input303[kLeftGrip]=1;Tick303();
 Check303(g_hands303.pendingSlot==0&&g_hands303.pendingHand==1&&g_pick292.requested,"actual tracked left hand grips left holster and requests exact native slot");
 Check303(fc3hands303::Read().empty,"requested draw does not invent a held gun before native readback");
 tracked303[1]=false;Tick303();Check303(g_hands303.pendingSlot<0&&fc3hands303::Read().empty&&!g_selectGesture290.open,"tracking loss cancels an in-flight draw without leaving the selector stuck");tracked303[1]=true;input303[kLeftGrip]=0;Tick303();input303[kLeftGrip]=1;Tick303();Check303(g_pick292.requested,"fresh grip retries safely after a cancelled draw");
 Tick303(1);Check303(!fc3hands303::Read().empty&&fc3hands303::Read().hand==1&&fc3hands303::Read().entity==100,"native equipped readback activates left weapon ownership");
 // A simulated stalled publication must not turn a confirmed left owner into right.
 {const auto saved=g_handsSnapshot303;const auto now=GetTickCount64();
  g_handsSnapshot303.at=now-400;g_handsSnapshot303.confirmed314=now-400;
  const auto stale=fc3hands303::Read(),visual=fc3hands303::ReadPresentation314();
  Check303(!stale.active&&visual.active&&fc3hands303::PhysicalHand(stale,0)==1&&!fc3hands303::SuppressWeapon(stale),"314 stalled publication blocks actions but keeps confirmed left presentation");
  g_handsSnapshot303.confirmed314=now-1600;Check303(!fc3hands303::ReadPresentation314().active,"314 presentation grace expires without reviving stale actions");
  g_handsSnapshot303=saved;
  g_roleAt303=now-400;PublishHands303(false);Check303(!fc3hands303::Read().active&&fc3hands303::ReadPresentation314().active,"314 late context publication retains presentation without interaction permission");
  fc3xr::SetHandsContext303(false,false,1);PublishHands303(false);Check303(!fc3hands303::ReadPresentation314().active,"314 explicit native context loss revokes presentation immediately");
 }
 Neutral303();input303[kLeftTrigger]=1;Tick303();
 Check303(fc3pad::Latest().rightTrigger>.9f&&fc3pad::FireOutput()>0,"physical left trigger reaches real native firing output for left-held gun");
 input303[kLeftTrigger]=0;input303[kRightTrigger]=1;Tick303();Check303(fc3pad::Latest().leftTrigger>.9f&&fc3pad::FireOutput()==0,"right trigger becomes support input and cannot fire a left-held gun");Neutral303();
 input303[kLeftGrip]=1;Tick303();Check303(fc3pad::Latest().rightGrip>.9f,"left holding grip follows primary weapon role");
 input303[kLeftGrip]=0;input303[kRightGrip]=1;Tick303();Check303(fc3pad::Latest().leftGrip>.9f,"right free hand follows support/reload role");Neutral303();
 input303[kRightGrip]=1;input303[kButtonX]=1;Tick303();Check303(fc3pad::Latest().ammoAction==fc3pad::AmmoButtonAction::Release&&fc3pad::Latest().leftGrip>.9f,"right support grip with physical X retains the established magazine-action chord");input303[kButtonX]=0;Neutral303();
 fc3xr::SetWeaponNotesContext303(true);fc3xr::SetTestNote(L"LEFT grip + X; RT fires; RIGHT hand; LEFT stick; LEFT GRIP + Y; RIGHT GRIP + Menu");fc3xr::SetWeaponNotesContext303(false);Check303(wcscmp(g_noteStatus,L"RIGHT grip + X; LT fires; LEFT hand; LEFT stick; LEFT GRIP + Y; RIGHT GRIP + Menu")==0,"weapon notes name actual physical hands while preserving sticks and global menu/inventory chords");
 input303[kMoveStick]=.4f;input303[kLookStick]=-.3f;Tick303();Check303(fabs(fc3pad::Latest().leftX-.4f)<1e-6f&&fabs(fc3pad::Latest().rightX+.3f)<1e-6f,"hand ownership never swaps movement and look sticks");input303[kMoveStick]=input303[kLookStick]=0;
 Move303(1,0);Neutral303();input303[kLeftGrip]=1;Tick303();pose303[1].position={-.4f,1.2f,-.4f};Tick303();pose303[0].position=pose303[1].position;pose303[0].position.x-=.1742f;input303[kRightGrip]=1;Tick303();
 Check303(g_hands303.receiver==0&&g_hands303.hand==1,"shipping transfer waits while original holding grip remains down");input303[kLeftGrip]=0;Tick303();
 Check303(fc3hands303::Read().hand==0&&inventory303.equipped==100,"shipping release transfers same native gun to right hand without re-equipping");
 //309 scoped support publication plus actual XR directional positions.
 auto contact307=[&](bool touching307,bool allowed=true){fc3hands303::PublishTransfer307(true,touching307,allowed,g_hands303.generation,g_hands303.entity,1);};
 pose303[0].position={.1f,1.2f,-.4f};pose303[1].position={0,1.2f,-.4f};
 contact307(true);input303[kLeftGrip]=1;inventory303.at=GetTickCount64();SeedInventory303(inventory303,0);fc3xr::SetHandsContext303(true,false,1);SyncControllers();Check303(g_hands303.hand==0&&g_hands303.receiver==1&&fc3pad::Latest().leftGrip>.9f,"309 fresh left grip arms while preserving native support input");
 pose303[0].position.x+=.3f;pose303[1].position.x+=.3f;contact307(true);Tick303();Check303(g_hands303.hand==0&&g_hands303.receiver==1,"309 moving both hands together cannot complete transfer");
 pose303[0].position.x-=.3f;pose303[1].position.x-=.3f;contact307(true);Tick303();Check303(g_hands303.hand==0,"309 returning both hands together still cannot transfer");
 pose303[1].position.z-=.3f;contact307(true);Tick303();Check303(g_hands303.hand==0,"309 forward pull cannot transfer while armed");
 pose303[1].position.x-=.08f;pose303[1].orientation={0,0,1,0};contact307(true);Tick303();Check303(g_hands303.hand==1&&g_hands303.receiver<0&&inventory303.equipped==100,"309 receiving pull transfers despite rotated wrist and held original grip");
 Neutral303();contact307(true);input303[kRightGrip]=1;input303[kButtonB]=1;Tick303();Check303(fc3pad::Latest().ammoAction==fc3pad::AmmoButtonAction::Release&&fc3pad::Latest().leftGrip>.9f,"309 left-held1911 uses physical right support grip B for magazine action");input303[kButtonB]=0;Neutral303();
 contact307(true);input303[kRightGrip]=1;input303[kButtonX]=1;Tick303();Check303(fc3pad::Latest().ammoAction!=fc3pad::AmmoButtonAction::Release,"309 old physical X cannot also eject a left-held1911 magazine");input303[kButtonX]=0;Neutral303();
 pose303[0].position.x=pose303[1].position.x+.1f;contact307(false);input303[kRightGrip]=1;Tick303();Check303(g_hands303.hand==1&&g_hands303.receiver<0,"309 grip without existing native support cannot arm");contact307(true);Tick303();Check303(g_hands303.receiver<0,"309 already-held grip cannot arm when hands enter support");
 input303[kRightGrip]=0;Tick303();contact307(true);input303[kRightGrip]=1;Tick303();Check303(g_hands303.receiver==0,"309 fresh right grip arms reverse handover");pose303[0].position.x+=.08f;contact307(true);Tick303();Check303(g_hands303.hand==0&&inventory303.equipped==100,"309 right receiving pull returns same native gun without re-equip");
 input303[kLeftGrip]=0;Tick303();pose303[1].position.x=pose303[0].position.x-.1f;contact307(true);g_transfer307.at=GetTickCount64()-121;input303[kLeftGrip]=1;Tick303();Check303(g_hands303.hand==0&&g_hands303.receiver<0,"309 stale publication cannot fall back to legacy palm transfer");
 input303[kLeftGrip]=0;Tick303();contact307(true,false);input303[kLeftGrip]=1;Tick303();Check303(g_hands303.hand==0&&g_hands303.receiver<0,"309 reload busy blocks transfer");
 input303[kLeftGrip]=0;Tick303();fc3hands303::PublishTransfer307(true,true,true,g_hands303.generation+1,g_hands303.entity,1);input303[kLeftGrip]=1;Tick303();Check303(g_hands303.hand==0&&g_hands303.receiver<0,"309 mismatched ownership generation cannot arm");
 fc3hands303::PublishTransfer307(false,false,false,0,0,1);input303[kLeftGrip]=0;Tick303();
 pose303[0].position={.4f,1.2f,-.4f};Tick303();Move303(0,0);Tick303();input303[kRightGrip]=0;Tick303();Check303(fc3hands303::Read().empty,"right-hand return and release stows into original empty holster");
 Neutral303();Move303(1,2);input303[kLeftGrip]=1;Tick303();Check303(g_hands303.pendingSlot==2,"left hand can reach and draw a lower-back slot");inventory303.equipped=102;Tick303(1);Neutral303();
 tracked303[1]=false;input303[kLeftTrigger]=1;Tick303();Check303(fc3pad::FireOutput()==0&&!fc3hands303::Read().active,"lost holding-hand tracking blocks native firing and rendering lease");tracked303[1]=true;Tick303();Check303(fc3pad::FireOutput()==0,"holding trigger through tracking recovery cannot fire before neutral release");Neutral303();
 // Display-only stow feedback follows the exact holding hand without transferring it.
 g_holsters297.guides=true;pose303[1].position={-.4f,1.2f,-.4f};Tick303();Move303(1,2);input303[kLeftGrip]=1;Tick303();
 Check303(g_stowReady304&&g_zonesVisible304&&g_hands303.hand==1&&!g_hands303.empty,"304 stow feedback turns ready only at the held slot without changing ownership");
 tracked303[1]=false;Tick303();Check303(!g_stowReady304,"304 lost hand tracking removes the ready highlight");tracked303[1]=true;input303[kLeftGrip]=1;Tick303();
 g_holsters297.stowOutline=false;Tick303();Check303(!g_stowReady304,"304 disabling stow feedback does not change weapon ownership");g_holsters297.stowOutline=true;
 // Both hands retain the streamlined single-holster preview/release path.
 pose303[1].position={-.4f,1.2f,-.4f};input303[kLeftGrip]=1;Tick303();Move303(1,2);Tick303();input303[kLeftGrip]=0;Tick303();Check303(fc3hands303::Read().empty,"left hand can stow the lower-back weapon before a new draw");
 g_holsters297.mode=fc3holster297::Single;Neutral303();Move303(1,g_singleChoice303);input303[kLeftGrip]=1;Tick303();
 Check303(g_single303&&g_hands303.pendingHand==1&&!g_pick292.requested,"single holster left grip begins preview without an intermediate native equip");
 const int beforeSingle=g_singleChoice303;input303[kLookStick]=.9f;Tick303();
 Check303(g_singleChoice303!=beforeSingle&&fc3pad::Latest().rightX==0,"single-holster flick selects another cached slot while suppressing body turn");
 const int finalSingle=g_singleChoice303;input303[kLeftGrip]=0;Tick303();Check303(g_pick292.requested&&g_hands303.pendingSlot==finalSingle,"single release requests only the final selected slot");
 inventory303.equipped=inventory303.weapons[finalSingle].entity;Tick303(1);input303[kLookStick]=0;Neutral303();
 Check303(fc3hands303::Read().hand==1&&fc3hands303::Read().entity==inventory303.equipped,"single-holster exact readback also equips into physical left hand");
 input303[kLeftGrip]=1;input303[kButtonY]=1;Tick303();Check303(fc3pad::WeaponWheelRequested145(),"physical left-grip Y inventory chord remains left-handed after weapon role mapping");input303[kLeftGrip]=input303[kButtonY]=0;Tick303();
 input303[kLeftGrip]=1;input303[kMenu]=1;Tick303();Check303(fc3pad::NativeUiControls().menu&&fc3pad::NativeUiControls().leftGrip>.9f,"physical left-grip Menu map chord is preserved with left ownership");
 input303[kMenu]=input303[kLeftGrip]=0;
 wchar_t tempFolder303[MAX_PATH],temp303[MAX_PATH];GetTempPathW(MAX_PATH,tempFolder303);GetTempFileNameW(tempFolder303,L"hnd",0,temp303);WritePrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"preserved",temp303);
 fc3hands303::Settings legacy303;legacy303.enabled=false;Check303(fc3hands303::Configure(legacy303,temp303)&&!fc3hands303::Read().enabled&&fc3hands303::PhysicalHand(fc3hands303::Read(),0)==0,"disabling the feature immediately restores the right-hand role fallback");
 auto saved303=legacy303;saved303.enabled=true;saved303.requireHolster=false;Check303(fc3hands303::Configure(saved303,temp303)&&GetPrivateProfileIntW(L"WeaponHands",L"Enabled",0,temp303)==1&&GetPrivateProfileIntW(L"WeaponHands",L"RequireHolster",1,temp303)==0,"either-hand and quick/holster-first choices save through production API");wchar_t sentinel303[32];GetPrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"",sentinel303,32,temp303);Check303(wcscmp(sentinel303,L"preserved")==0,"saving hand ownership settings preserves existing reload settings");DeleteFileW(temp303);

 wchar_t folder304[MAX_PATH],file304[MAX_PATH];GetTempPathW(MAX_PATH,folder304);GetTempFileNameW(folder304,L"hol",0,file304);WritePrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"unchanged",file304);
 auto layout304=fc3xr::WeaponHolsterSettings297();fc3holster297::Seated(layout304);layout304.mode=fc3holster297::Four;layout304.guides=true;layout304.positions[3]={.30f,-.42f,.35f};const auto before304=g_hands303;
 g_bodyPlaced297=true;g_selectPose290.orientation={0,sinf(.37f),0,cosf(.37f)};const auto anchor305=g_selectPose290;
 Check303(fc3xr::ConfigureWeaponHolsters297(layout304,file304),"304 production API saves seated preset, four independent positions and visibility");
 Check303(g_bodyPlaced297&&fabs(g_selectPose290.orientation.y-anchor305.orientation.y)<1e-6f&&fabs(g_selectPose290.orientation.w-anchor305.orientation.w)<1e-6f,"305 adjusting positions preserves the established body yaw instead of resetting it");
 Check303(g_hands303.hand==before304.hand&&g_hands303.entity==before304.entity&&g_hands303.empty==before304.empty,"304 layout adjustment leaves holding hand and native identity unchanged");
 g_holsters297={};g_holstersLoaded297=false;LoadHolsters297(file304);
 Check303(g_holsters297.guides&&g_holsters297.stowOutline&&g_holsters297.custom[3]&&fabs(g_holsters297.positions[3].z-.35f)<1e-5f&&fabs(fc3holster297::Centre(g_holsters297,0).z+.30f)<1e-5f,"304 actual INI loader restores front seated and behind-body custom placements");
 const auto centre304=Palm292(RackHandle292(3));const auto expected304=testnotes::Add(g_selectPose290.position,testnotes::Rotate(g_selectPose290.orientation,{.30f,-.42f,.35f}));Check303(fabs(centre304.x-expected304.x)<1e-5f&&fabs(centre304.y-expected304.y)<1e-5f&&fabs(centre304.z-expected304.z)<1e-5f,"304 native weapon handle and trigger use the exact saved body-relative location");
 GetPrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"",sentinel303,32,file304);Check303(wcscmp(sentinel303,L"unchanged")==0,"304 position saves preserve existing weapon/reload settings");
 auto invalid304=layout304;invalid304.positions[0].x=NAN;Check303(!fc3xr::ConfigureWeaponHolsters297(invalid304,file304),"304 nonfinite custom position is refused");
 WritePrivateProfileStringW(L"WeaponHolsters",L"Slot1",L"nan,0,0",file304);g_holsters297={};g_holstersLoaded297=false;LoadHolsters297(file304);Check303(!g_holsters297.custom[0]&&g_holsters297.custom[3],"304 corrupt slot falls back independently without discarding valid slots");DeleteFileW(file304);
 puts("PASS:303 actual XR holster/input path, either-hand draw, native confirmation, left firing, support roles, stowing and tracking safety; no game/headset launched");
 return 0;
}
