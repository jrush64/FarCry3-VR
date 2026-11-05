// Flare gun (flare_gun.xbg, profile 16). Design: fire one shot, take the spent shell out physically and put a
// new one in; like the revolver, but it closes only with a backward shake.
//  - Rigid mesh, one world matrix per draw (captures/weapons7-29048, work/flare_gen.py -> flare_clips.inl): FRAME = 3708/4002,
//    barrel = 0/1590 (identity on FRAME when closed; breaks open 72.4 deg about FRAME -X through the hinge pivot), hammer =
//    7710/906 (cocked +40 deg about +X), shell = 8616/138 + 8754/330 (the same matrix; part of the gun mesh, always drawn).
//    1590/1056 and 2646/1062 never move against FRAME.
//  - The game's own reload runs automatically after every shot; it is refused (reload signal gate), so the gun stays at rest and
//    every moving part is placed here from FRAME: part = (its ordinary moved world) x inv(its rest on FRAME) x new pose on FRAME.
//  - Play: after a shot the hammer is down and the spent shell is in the chamber; the trigger is dry until a new shell is in
//    and the barrel is closed.
//      OPEN: LEFT grip on the BARREL box and pull the muzzle down (the barrel breaks open).
//      SPENT SHELL OUT: LEFT grip in the CHAMBER box takes it into the left hand; let go and it falls. Tipping the muzzle up
//        with the barrel open also slides it out.
//      NEW SHELL: LEFT grip + X with the chamber empty puts one in the left hand; bring it to the CHAMBER box: it goes in
//        (the engine's own Begin/transfer/End add the round, capacity and reserve checked).
//      CLOSE: only a backwards flick of the gun (the right controller moving back/up fast); the hammer cocks.
//  - Type 10 (ulc_type10flaregun.xbg, profile 38) was not in the capture: it keeps the game's own reload for now.
#include "flare_clips.inl"
struct Flare
{
 NativeShot219 shot;
 // chamber: 0 empty, 1 loaded, 2 spent
 int chamber=1;bool known=false,open=false,grab=false,carrying=false,carryingSpent=false,pending=false,refused=false;
 bool falling=false,leftGripWas=false,xWas=false,xOwned=false,hammerDown=false,triggerWas=false;
 double angle=0;V3 grabStart={};ULONGLONG lastTick=0,deadline=0,pulseAt=0,fallAt=0,tiltSince=0,probeAt=0;bool probing=false;
 Mat4 frameBody={};LONG frameFrame=-100000;Mat4 leftWrist={};LONG leftFrame=-100000;Mat4 fallBody={};
 V3 rightPrev={};ULONGLONG rightPrevAt=0;UINT ammoState=0,ammoWrapper=0;LONG loads=0,gripLogs=0,flickLogs=0;
} g_flare;
#define kFlareHand (g_w7Guides[0].hand)
#define kFlareHandDiameter (g_w7Guides[0].diameter)
 // default palm offset + 10 cm sphere (all guns)
const float kFlareBarrelBox[3]={0.f,0.f,.02f};
#define kFlareBarrelBoxSize (g_w7Guides[0].size[0])
 // bigger (it was hard to grab) // barrel-local (origin = its turn point, breech 10 cm behind it): the barrel body
#define kFlareChamberBoxSize (g_w7Guides[0].size[1])
 // bigger (spent shell out by hand)
bool FlareModel(){return g_weaponMechanicProfile==16&&g_frame-g_weaponMechanicFrame<=2;}
bool FlareActive(){return FlareModel()&&MotionInteractionEnabled()&&fc3pad::Latest().active&&g_handLeftValid&&g_handRightValid;}
bool FlareFrameReady(){return g_frame-g_flare.frameFrame<=2;}
bool FlareLeftReady(){return g_frame-g_flare.leftFrame<=2;}
Mat4 FlareKeyMat(const float* v){Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=v[i];return m;}
// Rotation by `degrees` about the axis (unit, FRAME) through the pivot (FRAME).
Mat4 FlareHinge(const float* axis,const float* pivot,double degrees)
{
 const double a=degrees*3.14159265358979/180,c=std::cos(a),s=std::sin(a),k=1-c,u[3]={axis[0],axis[1],axis[2]};Mat4 m=HandIdentity();
 for(int r=0;r<3;++r)for(int col=0;col<3;++col)m.m[r*4+col]=(r==col?c:0)+k*u[r]*u[col];
 m.m[1]-=s*u[2];m.m[2]+=s*u[1];m.m[4]+=s*u[2];m.m[6]-=s*u[0];m.m[8]-=s*u[1];m.m[9]+=s*u[0];
 for(int r=0;r<3;++r){double v=pivot[r];for(int col=0;col<3;++col)v-=m.m[r*4+col]*pivot[col];m.m[r*4+3]=v;}
 return m;
}
// The barrel stood straight up when open: recordings show the closed barrel at (0, .196, .030) on FRAME,
// not at FRAME (flare_clips.inl's "closed = identity" was wrong), and the captured open pose turns it about ITS OWN origin. Barrel =
// closed rest x turn about its local -X; the barrel draw's own world is that rest at idle.
const float kFlareBarrelRest[3]={kFlareBarrelOpenFromFrame[3],kFlareBarrelOpenFromFrame[7],kFlareBarrelOpenFromFrame[11]};
Mat4 FlareBarrelRest(){Mat4 m=HandIdentity();m.m[3]=kFlareBarrelRest[0];m.m[7]=kFlareBarrelRest[1];m.m[11]=kFlareBarrelRest[2];return m;}
Mat4 FlareBarrel(){static const float origin[3]={0,0,0};return Multiply(FlareBarrelRest(),FlareHinge(kFlareBarrelHingeAxis,origin,g_flare.angle));}
Mat4 FlareHammer(double degrees){return FlareHinge(kFlareHammerAxis,kFlareHammerPivot,degrees);}
// Box b (0 = barrel grab, 1 = chamber) in FRAME space, following the barrel.
Mat4 FlareBox(int b)
{
 const Mat4 barrel=FlareBarrel();Mat4 m=barrel;
 const V3 local=b?V3{kFlareChamberMouthBarrelLocal[0],kFlareChamberMouthBarrelLocal[1],kFlareChamberMouthBarrelLocal[2]}:V3{kFlareBarrelBox[0],kFlareBarrelBox[1],kFlareBarrelBox[2]};
 const V3 c=Apply(barrel,local);m.m[3]=c.x;m.m[7]=c.y;m.m[11]=c.z;return W7Adjust(0,b,m);
}
double FlareBoxDistance(int b,const V3& pointFrame)
{
 Mat4 inverse={};if(!Invert(FlareBox(b),inverse))return 1e9;const V3 l=Apply(inverse,pointFrame);const double v[3]={l.x,l.y,l.z};
 const float* size=b?kFlareChamberBoxSize:kFlareBarrelBoxSize;double d2=0;
 for(int a=0;a<3;++a){const double o=(std::max)(0.0,std::fabs(v[a])-size[a]*.5);d2+=o*o;}return std::sqrt(d2);
}
bool FlarePalmBody(V3& palm){const auto& F=g_flare;if(!FlareLeftReady())return false;palm=Apply(F.leftWrist,{-.591+kFlareHand[0],.011+kFlareHand[1],1.149+kFlareHand[2]});return true;}
bool FlarePalmFrame(V3& palm){Mat4 inverse={};V3 body={};if(!FlareFrameReady()||!FlarePalmBody(body)||!Invert(g_flare.frameBody,inverse))return false;palm=Apply(inverse,body);return true;}
// Rounds in the gun (engine count) once the request seam has seen the weapon; -1 unknown.
int FlareLoadedRounds(){const auto& F=g_flare;if(!F.ammoWrapper||!ReloadSignalLocal(F.ammoState,F.ammoWrapper))return -1;const UINT w=ReloadRead(F.ammoWrapper,0x48);if(!w)return -1;return static_cast<int>(ReloadRead(w,0xcc));}
// The spent shell falling (body space): out of the chamber backwards, then gravity, with a tumble.
Mat4 FlareFallBody(double s)
{
 const auto& F=g_flare;const double scale=(std::max)(.1f,g_worldScale);
 V3 back={-F.fallBody.m[1],-F.fallBody.m[5],-F.fallBody.m[9]};const double bl=Length(back);if(bl>1e-6)back=Scale(back,1/bl);
 const V3 q0={F.fallBody.m[3],F.fallBody.m[7],F.fallBody.m[11]};V3 q=Add(q0,Scale(back,.6*scale*s));q.y-=.5*9.81*s*s*scale;
 const double a=11*s,c=std::cos(a),sn=std::sin(a);Mat4 spin=HandIdentity();spin.m[5]=c;spin.m[6]=-sn;spin.m[9]=sn;spin.m[10]=c;
 Mat4 m=Multiply(F.fallBody,spin);m.m[3]=q.x;m.m[7]=q.y;m.m[11]=q.z;return m;
}
// ---- draws (after the ordinary right-hand move): every moving part placed from FRAME
bool FlareFinalWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4&,float* globals)
{
 if(!FlareModel()||!MotionInteractionEnabled())return false;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 // The game also draws the flare ROUND (flare.xbg, vb 1600 / ib 264) at the parked spot (0, .144, .061) - in view in VR, it
 // was the round floating above the grip. Hidden while the flare gun is in hand.
 if(d.vertexBytes==1600&&d.indexBytes==264){Mat4 hidden=HullWorld(globals);for(int r=0;r<3;++r)for(int col=0;col<3;++col)hidden.m[r*4+col]*=1e-4;WritePistolWorld(globals,hidden);return true;}
 if(d.vertexBytes!=91584||d.indexBytes!=18168||d.stride!=32||d.format!=57)return false;
 auto& F=g_flare;
 const bool fixedDraw=(start==1590&&count==1056)||(start==2646&&count==1062);
 const bool frameDraw=start==3708&&count==4002,barrelDraw=start==0&&count==1590,hammerDraw=start==7710&&count==906,shellDraw=(start==8616&&count==138)||(start==8754&&count==330);
 if(!fixedDraw&&!frameDraw&&!barrelDraw&&!hammerDraw&&!shellDraw)return false;
 CameraSummary camera={};if(!PistolGuideCamera(context,camera))return false;
 const Mat4 view=HullView(camera);Mat4 viewInverse={},bodyInverse={};if(!Invert(view,viewInverse)||!Invert(ArmBodyToCamera(),bodyInverse))return false;
 const Mat4 moved=HullWorld(globals);
 // FRAME of this draw: its ordinary world x inv(its rest on FRAME) (the game's reload is refused, so every part sits at rest)
 Mat4 frame=moved;
 if(hammerDraw){Mat4 i={};if(Invert(FlareHammer(kFlareHammerCockedDeg),i))frame=Multiply(moved,i);}
 else if(barrelDraw){Mat4 i={};if(Invert(FlareBarrelRest(),i))frame=Multiply(moved,i);}
 else if(shellDraw){Mat4 i={};if(Invert(FlareKeyMat(kFlareShellRestFromFrame),i))frame=Multiply(moved,i);}
 ControllerGripWorld124(context,FlareKeyMat(kFlareRightHandFromFrame),nullptr,frame);
 if(fixedDraw){WritePistolWorld(globals,frame);return true;}
 if(frameDraw){WritePistolWorld(globals,frame);F.frameBody=Multiply(bodyInverse,Multiply(view,frame));F.frameFrame=g_frame;return true;}
 if(barrelDraw){WritePistolWorld(globals,Multiply(frame,FlareBarrel()));return true;}
 if(hammerDraw){WritePistolWorld(globals,Multiply(frame,FlareHammer(F.hammerDown?0:kFlareHammerCockedDeg)));return true;}
 // shell
 if((F.carrying&&FlareLeftReady())||W7PreviewWristReady54(0))
 {WritePistolWorld(globals,Multiply(viewInverse,Multiply(ArmBodyToCamera(),SidearmCarried315(W7VisualWrist54(0,F.leftWrist),W7Ammo(0,FlareKeyMat(F.carryingSpent?kFlareSpentShellFromLeftWrist:kFlareShellFromLeftWrist))))));return true;}
 if(F.falling)
 {
  const double s=(W7Now()-F.fallAt)/1000.0;
  if(s<1.2){WritePistolWorld(globals,Multiply(viewInverse,Multiply(ArmBodyToCamera(),FlareFallBody(s))));return true;}
 }
 // The game PARKS this shell outside the gun (off-screen in the flat game, in view in VR);
 // An earlier build handed it back to the game when closed, so it floated above the grip. It is only ever shown in the open breech or the hand.
 Mat4 hidden=moved;for(int r=0;r<3;++r)for(int col=0;col<3;++col)hidden.m[r*4+col]*=1e-4;
 if(F.chamber==0){WritePistolWorld(globals,hidden);return true;} // nothing in the chamber
 // (The shell floated above the grip.) The breech seat is only right with the barrel open; closed, it goes back
 // to the game's own parked pose inside the barrel.
 if(F.angle<1){WritePistolWorld(globals,hidden);return true;} // closed: inside the barrel, not visible
 WritePistolWorld(globals,Multiply(frame,Multiply(FlareBarrel(),FlareKeyMat(kFlareShellFromBarrelSeat))));return true;
}
void RecordFlareLeftHand(ID3D11DeviceContext* context,const float* bones)
{
 if(!FlareModel())return;Mat4 view={},inverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
 g_flare.leftWrist=Multiply(inverse,view);g_flare.leftFrame=g_frame;
}
bool FlareLeftGrip(float* bones)
{
 const auto& F=g_flare;if(!FlareActive()||!F.carrying)return false;
 const Mat4 wrist=BoneMatrix(bones,2);const float* fingers=F.carryingSpent?kFlareLeftSpentShellFingers:kFlareLeftShellFingers;
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(fingers,b-3)));return true;
}
void UpdateFlareFire(const fc3pad::Controllers& c)
{
 auto& F=g_flare;fc3pad::RequireFullFirePull(false);
 const bool active=FlareActive(),pull=c.rightTrigger>.7f;const int loaded=FlareLoadedRounds();const auto now=W7Now();
 if(active&&loaded<0&&F.known){F.known=false;F.probeAt=0;}
 // A known live round disappearing is a native shot even if the trigger
 // was already held while the initial owner probe completed.
 if(active&&F.known&&loaded==0&&F.chamber==1&&!F.carrying&&!F.pending&&!F.shot.pending){F.chamber=2;F.hammerDown=true;Log("[FLARE240] known native round consumed: spent shell available");}
 const bool chamberReady=F.chamber==1&&!F.carrying&&!F.pending;
 const bool ready=!F.open&&F.angle<.5&&chamberReady;
 const bool wasPending=F.shot.pending;
 const bool confirmed=PollNativeShot219(F.shot,active,chamberReady,F.ammoState,F.ammoWrapper,loaded,now);
 if(confirmed){
  F.chamber=2;F.hammerDown=true;Log("[FLARE219] native shot confirmed: spent shell in chamber, hammer down");
 }
 // A late native shot may arrive after the input window closes. Re-read via
 // the existing local probe rather than leave a live physical chamber over
 // an empty native weapon, or invent a spent shell from the timeout alone.
 if(wasPending&&!F.shot.pending&&!confirmed){F.known=false;F.probeAt=0;}
 if(c.rightTrigger<.3f&&!F.shot.pending)F.shot.waitRelease=false;
 if(active&&ready&&loaded>0&&pull&&!F.triggerWas&&!F.shot.pending&&!F.shot.waitRelease)
  BeginNativeShot219(F.shot,F.ammoState,F.ammoWrapper,loaded,now);
 fc3pad::BlockFire(active&&(!ready||loaded<=0||F.shot.waitRelease)&&!F.shot.pending);
 F.triggerWas=pull;
}
// Reload request seam (game thread): learns the weapon; seats the carried shell (engine Begin/transfer/End, one round).
void FlareRequest(UINT state,UINT wrapper)
{
 if(g_pistolMenu.open)return;
 auto& F=g_flare;if(!ReloadSignalLocal(state,wrapper))return;
 F.ammoState=state;F.ammoWrapper=wrapper;
 if(F.probing){F.probing=false;fc3pad::CancelReloadRequest();const int n=FlareLoadedRounds();if(!F.known&&n>=0){F.known=true;F.chamber=n>0?1:2;F.hammerDown=n<=0;}Log("[FLARE] weapon known: %d round(s) loaded",n);}
 if(!F.pending||W7Now()>F.deadline)return;
 F.pending=false;fc3pad::CancelReloadRequest();
 UINT loaded=0,capacity=0,reserve=0;const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 if(!Lever1887Ammo(wrapper,loaded,capacity,reserve))
 {F.refused=true;fc3xr::Buzz(0);Log("[FLARE] capacity unavailable: no native transfer");return;}
 if((std::max)(loaded,before)>=capacity||!reserve)
 {F.refused=true;fc3xr::Buzz(0);Log("[FLARE] shell refused: %u/%u, reserve %u",(std::max)(loaded,before),capacity,reserve);return;}
 const bool ok=AkInstantNativeTriple(state,wrapper);const UINT after=ReloadRead(weapon,0xcc);
 if(after>before){F.carrying=false;F.chamber=1;++F.loads;fc3xr::Buzz(1);}
 Log("[FLARE] shell seated: native Begin/transfer/End, loaded %u -> %u, engine ok=%d",before,after,ok);
}
void TickFlare(bool gameplay)
{
 if(g_pistolMenu.open)return;
 auto& F=g_flare;const auto c=fc3pad::AmmoControls();
 const bool active=gameplay&&FlareActive();
 if(FlareModel()||F.xOwned){fc3pad::SetXOwned(active);F.xOwned=active;} // X never reaches the game's reload while it is in hand
 const bool lg=c.leftGrip>.7f,lgEdge=lg&&!F.leftGripWas,lgUp=c.leftGrip<.35f,xEdge=c.x&&!F.xWas;F.leftGripWas=fc3pad::GripOnlyAmmoEnabled()?(lg||(c.leftGrip>=.35f&&F.leftGripWas)):lg;F.xWas=c.x;
 const ULONGLONG now=W7Now();const double dt=F.lastTick?(std::min)(.1,(now-F.lastTick)/1000.0):0;F.lastTick=now;
 if(!active){F.grab=false;F.rightPrevAt=0;F.tiltSince=0;if(!FlareModel()){if(F.pending)fc3pad::CancelReloadRequest();F.pending=F.carrying=F.falling=false;}return;} // chamber/open state kept across switches; a lost hand cannot retain a grab/flick origin
 if(F.pending&&now>F.deadline){F.pending=false;fc3pad::CancelReloadRequest();Log("[FLARE] the engine never took the shell: it stays in the left hand");}
 if(F.pending&&now-F.pulseAt>=250){F.pulseAt=now;fc3pad::RequestReload();}
 if(F.probing&&now-F.probeAt>400){F.probing=false;fc3pad::CancelReloadRequest();}
 if(!F.known&&!F.probing&&!F.pending&&now-F.probeAt>2000){F.probing=true;F.probeAt=now;fc3pad::RequestReload();Log("[FLARE] probing the weapon once to read its loaded round (the reload itself stays refused)");}
 if(F.falling&&now-F.fallAt>1200)F.falling=false;
 // barrel angle: opens fast, closes at the native ~0.15 s
 const double target=F.open?kFlareBarrelOpenDeg:0;const double step=(F.open?900:500)*dt;
 F.angle=F.angle<target?(std::min)(target,F.angle+step):(std::max)(target,F.angle-step);
 const bool settled=std::fabs(F.angle-target)<.5;
 const double scale=(std::max)(.1f,g_worldScale),handR=kFlareHandDiameter*.5*scale;
 V3 palm={};const bool palmOk=FlarePalmFrame(palm);
 const double dBarrel=palmOk?FlareBoxDistance(0,palm):1e9,dChamber=palmOk?FlareBoxDistance(1,palm):1e9;
 const bool inBarrel=dBarrel<=handR,inChamber=F.open&&settled&&dChamber<=handR;
 // spent shell slides out when the open barrel's muzzle points up (~40 degrees)
 {
  const double by=F.frameBody.m[5],len=std::sqrt(F.frameBody.m[1]*F.frameBody.m[1]+by*by+F.frameBody.m[9]*F.frameBody.m[9]);
  const bool up=FlareFrameReady()&&len>1e-6&&by/len>.64&&F.open&&settled&&F.chamber==2&&!F.carrying;
  if(!up)F.tiltSince=0;else if(!F.tiltSince)F.tiltSince=now;
  if(F.tiltSince&&now-F.tiltSince>=100){F.fallBody=Multiply(F.frameBody,Multiply(FlareBarrel(),FlareKeyMat(kFlareShellFromBarrelSeat)));F.falling=true;F.fallAt=now;F.chamber=0;F.tiltSince=0;fc3xr::Buzz(0);Log("[FLARE] muzzle up: the spent shell slides out");}
 }
 if(((xEdge&&lg)||(fc3pad::GripOnlyAmmoEnabled()&&lgEdge&&palmOk&&!inBarrel&&!inChamber&&F.open&&settled&&F.chamber==0))&&!F.carrying&&!F.pending&&!F.grab)
 {
  if(F.open&&settled&&F.chamber==0){F.carrying=true;F.carryingSpent=false;F.refused=false;fc3xr::Buzz(1);Log("[FLARE] LEFT grip + X: new shell in the left hand");}
  else{fc3xr::Buzz(0);Log(!F.open?"[FLARE] LEFT grip + X: nothing - open the barrel first (grip it, pull the muzzle down)":F.chamber==2?"[FLARE] LEFT grip + X: nothing - take the spent shell out first":"[FLARE] LEFT grip + X: nothing - a shell is already in");}
 }
 else if(lgEdge&&!F.carrying&&!F.grab&&!F.pending)
 {
  const char* action="nothing";
  if(inChamber&&F.chamber==2){F.carrying=true;F.carryingSpent=true;F.chamber=0;fc3xr::Buzz(1);action="spent shell taken out";}
  else if(inBarrel){F.grab=true;F.grabStart=palm;fc3xr::Buzz(0);action=F.open?"barrel grabbed (push it up to close)":"barrel grabbed (pull the muzzle down to open)";}
  if(InterlockedIncrement(&F.gripLogs)<=60)Log("[FLARE] LEFT grip: %s | hand %s; BARREL box %.3f, CHAMBER box %.3f (sphere r %.3f); barrel %s, chamber %d",
   action,palmOk?"ok":"MISSING",dBarrel,dChamber,handR,F.open?"open":"closed",F.chamber);
 }
 if(F.grab)
 {
  if(lgUp||!palmOk)F.grab=false;
  // any 3 cm pull opens it (it only took a straight-down pull)
  else if(!F.open&&Length(Sub(palm,F.grabStart))>=.03*scale){F.open=true;F.grab=false;fc3xr::Buzz(0);Log("[FLARE] barrel broken open by hand");}
  // Manual close as an alternative to the snap: grip the open barrel and push it 3 cm (any way): closed.
  else if(F.open&&settled&&Length(Sub(palm,F.grabStart))>=.03*scale){F.open=false;F.hammerDown=false;F.grab=false;fc3xr::Buzz(1);Log("[FLARE] barrel closed by hand; hammer cocked");}
 }
 // carried shell: the new one goes in at the chamber; letting go drops it (the spent one falls)
 if(F.carrying&&!F.carryingSpent&&!F.pending&&!F.refused&&inChamber){F.pending=true;F.deadline=now+3000;F.pulseAt=now;fc3pad::RequestReload();Log("[FLARE] new shell at the chamber: engine fill requested");}
 if(!inChamber)F.refused=false;
 if(F.carrying&&lgUp&&!F.pending)
 {
  if(F.carryingSpent&&FlareLeftReady()){F.fallBody=SidearmCarried315(W7VisualWrist54(0,F.leftWrist),W7Ammo(0,FlareKeyMat(kFlareSpentShellFromLeftWrist)));F.falling=true;F.fallAt=now;}
  Log(F.carryingSpent?"[FLARE] spent shell dropped":"[FLARE] new shell dropped, no ammo change");F.carrying=false;
 }
 // CLOSE: a backwards flick of the gun (right controller moving back toward you / up, fast) - the only way to close it
 {
  const V3 rp={g_handBody[0][3],g_handBody[0][7],g_handBody[0][11]};
  // The snap close must not interfere with tilting the gun sideways: no flick close while the spent
  // shell is still in (that is when it gets tipped out), nor while the hand is on the barrel.
  if(F.rightPrevAt&&now>F.rightPrevAt&&now-F.rightPrevAt<100&&FlareFrameReady()&&F.open&&settled&&!F.carrying&&!F.pending&&F.chamber!=2&&!F.grab&&!F.shot.pending)
  {
   const double fdt=(now-F.rightPrevAt)/1000.0;const V3 v=Scale(Sub(rp,F.rightPrev),1/fdt);
   V3 fwd={F.frameBody.m[1],F.frameBody.m[5],F.frameBody.m[9]},up={F.frameBody.m[2],F.frameBody.m[6],F.frameBody.m[10]};
   const double fl=Length(fwd),ul=Length(up);
   if(fl>1e-6&&ul>1e-6)
   {
    const double back=-Dot(v,Scale(fwd,1/fl)),lift=Dot(v,Scale(up,1/ul)),flick=(back+lift)*.70710678;
    if(flick>.45*scale&&InterlockedIncrement(&F.flickLogs)<=40)Log("[FLARE] flick %.2f m/s (back %.2f, up %.2f)%s",flick,back,lift,flick>.7*scale?": CLOSE":"");
    // 0.7 m/s (a 0.91 m/s flick did not close it)
    if(flick>.7*scale){F.open=false;F.hammerDown=false;fc3xr::Buzz(1);Log("[FLARE] barrel closed by a backwards flick; hammer cocked");}
   }
  }
  F.rightPrev=rp;F.rightPrevAt=now;
 }
}
void FlareGuideMarkers()
{
 const auto& F=g_flare;
 if(!FlareActive()||!FlareFrameReady()||!PhysicalReloadGuidesEnabled()||g_pistolMenu.open){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 const bool wantChamber=g_weaponTriggerDiagnostics?g_weaponTriggerTarget==0:F.open&&(F.chamber==2||F.carrying);
 const int b=wantChamber?1:0;const float scale=(std::max)(.1f,g_worldScale);const double handR=kFlareHandDiameter*.5*scale;
 V3 p={};const bool palmOk=FlarePalmFrame(p);const int contact=F.grab||(palmOk&&FlareBoxDistance(b,p)<=handR)?1:0;
 const Mat4 box=Multiply(F.frameBody,FlareBox(b));Mat4 basis=box;
 for(int col=0;col<3;++col){double len=0;for(int r=0;r<3;++r)len+=basis.m[r*4+col]*basis.m[r*4+col];len=std::sqrt(len);if(len<1e-8)return;for(int r=0;r<3;++r)basis.m[r*4+col]/=len;}
 V3 hand={};const bool showHand=FlarePalmBody(hand);const float* size=b?kFlareChamberBoxSize:kFlareBarrelBoxSize;
 float a[3]={static_cast<float>(box.m[3]),static_cast<float>(box.m[7]),static_cast<float>(box.m[11])},h[3]={static_cast<float>(hand.x),static_cast<float>(hand.y),static_cast<float>(hand.z)};
 fc3xr::SetReloadMarkers(a,showHand?h:nullptr,size[0]*.5f*scale,contact,kFlareHandDiameter*scale);
 float transform[16],dims[3];for(int i=0;i<16;++i)transform[i]=static_cast<float>(basis.m[i]);for(int i=0;i<3;++i)dims[i]=size[i]*scale;
 fc3xr::SetWeaponBox(transform,dims,contact);if(showHand)fc3xr::SetHandSphere();
}
// The same in every pass for the flare shell (unless it is in the hand, falling, or in the open breech) and the flare round.
bool FlareSkipDraw(ID3D11DeviceContext* context,const DrawCall& call)
{
 if(call.kind!=0||!FlareModel()||!MotionInteractionEnabled())return false;
 const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes==1600&&d.indexBytes==264)return true; // the flare round model
 if(d.vertexBytes!=91584||d.indexBytes!=18168||!((call.b==8616&&call.a==138)||(call.b==8754&&call.a==330)))return false;
 const auto& F=g_flare;if(F.carrying||(F.falling&&W7Now()-F.fallAt<1200))return false;
 return F.chamber==0||F.angle<1;
}
bool FlareNote()
{
 if(!FlareActive())return false;const auto& F=g_flare;
 fc3xr::SetTestNote(F.pending?L"FLARE GUN - LOADING":F.carrying?(F.carryingSpent?L"FLARE GUN - SPENT SHELL\nLet go of LEFT grip to drop it.":L"FLARE GUN - NEW SHELL\nPut it in the CHAMBER box."):
  F.open?(F.chamber==2?L"FLARE GUN - OPEN\nLEFT grip in the CHAMBER box: spent shell out\n(or tip the muzzle up).":F.chamber==0?L"FLARE GUN - OPEN, EMPTY\nLEFT grip + X: new shell.":L"FLARE GUN - LOADED\nFlick the gun backwards to close."):
  F.chamber==1?L"FLARE GUN":L"FLARE GUN - FIRED\nLEFT grip the BARREL box, pull the muzzle down: open.");
 return true;
}
