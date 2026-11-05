void TestLever1887Palm56()
{
 const auto oldLever=g_lever;const auto oldControls=fc3pad::Latest();const auto oldAttachment0=g_freeHandAttachment[0],oldAttachment1=g_freeHandAttachment[1];
 const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame,oldFrame=g_frame;
 const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldLeft=g_handLeftValid,oldRight=g_handRightValid,oldTwo=g_twoHand,oldMenu=g_pistolMenu.open;
 float body[2][16];memcpy(body,g_handBody,sizeof(body));const float oldScale=g_worldScale;
 g_weaponMechanicProfile=0;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_twoHand=false;g_pistolMenu.open=false;g_worldScale=1;
 auto& L=g_lever;L=Lever1887{};L.hold=true;L.chambered=false;L.displayBody=HandIdentity();L.displayFrame=L.rightHandFrame=L.leftHandFrame=g_frame;L.rightHandBody=L.leftHandBody=HandIdentity();
 for(int side=0;side<2;++side){for(int i=0;i<16;++i)g_handBody[side][i]=float(HandIdentity().m[i]);auto& a=g_freeHandAttachment[side];a.relative=HandIdentity();a.epoch=g_armPoseEpoch;a.frame=g_frame;a.profile=0;}
 L.palmOffset56[0][0]=.02f;L.palmOffset56[1][2]=-.03f;L.palmDiameter56[0]=.12f;L.palmDiameter56[1]=.08f;
 V3 right={},left={};Check(Lever1887PalmBody56(0,right)&&fabs(right.x-(.6781+.02))<1e-6,"1887 right contact is actual calibrated palm, not wrist");
 Check(Lever1887PalmBody56(1,left)&&fabs(left.z-(1.0387-.03))<1e-6,"1887 left palm has independent live calibration");
 L.rightHandBody.m[3]=9;L.rightHandBody.m[7]=-4;V3 still={};Check(Lever1887PalmBody56(0,still)&&Length(Sub(still,right))<1e-8,"1887 controller-fixed free palm ignores pinned/animated lever wrist");
 L.guideBox[0]=true;for(int a=0;a<3;++a)L.guideSize[0][a]=.02f;
 const V3 edge={L.lever[0]+.065,L.lever[1],L.lever[2]};Check(!Lever1887GuideContact52(0,edge)&&Lever1887GuideContact52(0,edge,false,false,.06),"1887 visible hand sphere radius is used against calibrated box contact");
 fc3pad::Controllers c={};c.active=true;c.leftGrip=1;fc3pad::Publish(c);
 // Closed lever reserves the free hand even when pickup is deliberately overridden.
 g_handBody[0][3]=3;g_handBody[0][7]=3;g_handBody[0][11]=3;c.rightGrip=1;fc3pad::Publish(c);TickLever1887(true);Check(!L.shell&&!L.grabbing,"1887 closed lever blocks implicit shell pickup away from weapon");
 c.x=true;fc3pad::Publish(c);TickLever1887(true);Check(!L.shell,"1887 closed lever blocks both-grip X shell override");
 c.x=false;c.rightGrip=0;fc3pad::Publish(c);TickLever1887(true);
 // Place the real palm at the lever loop, then move raw controller along the
 // lever rail while leaving the drawn/pinned hand completely unchanged.
 g_handBody[0][3]=float(L.lever[0]-.6781-L.palmOffset56[0][0]);g_handBody[0][7]=float(L.lever[1]-.0519);g_handBody[0][11]=float(L.lever[2]-1.0387);
 L.rightGripWas=false;c.rightGrip=1;fc3pad::Publish(c);TickLever1887(true);Check(L.grabbing&&!L.shell,"1887 lever contact wins over implicit grip-only ammo pickup");
 const V3 axis=Unit(Sub(V3{kL1887LeverOpenPoint[0],kL1887LeverOpenPoint[1],kL1887LeverOpenPoint[2]},V3{kL1887LeverPoint[0],kL1887LeverPoint[1],kL1887LeverPoint[2]}));
 for(int a=0;a<3;++a)g_handBody[0][a*4+3]+=float((a==0?axis.x:a==1?axis.y:axis.z)*L.pull*1.1);
 TickLever1887(true);Check(L.frac>=.99,"1887 physical lever stroke follows raw controller despite fixed drawn wrist");
 c.rightGrip=0;fc3pad::Publish(c);TickLever1887(true);Check(L.leverOpen&&!L.grabbing,"1887 release retains fully opened lever state");
 // A fresh grip away from weapon parts needs no A or X and takes one shell.
 for(int a=0;a<3;++a)g_handBody[0][a*4+3]+=1.0f;c.rightGrip=1;c.a=c.x=false;fc3pad::Publish(c);TickLever1887(true);Check(L.shell,"1887 free RIGHT grip takes shell without A or X while gun stays left");
 g_handRightValid=false;Check(!Lever1887PalmBody56(0,right),"1887 invalid controller tracking cannot publish a false palm contact");
 wchar_t temp[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,temp);GetTempFileNameW(temp,L"p56",0,path);
 const float savedLever=L.lever[0],savedGate=L.gate[1],savedSize=L.guideSize[0][0];
 Check(SaveLever1887Settings(path),"1887 additive palm settings save to isolated fixture file");
 memset(L.palmOffset56,0,sizeof(L.palmOffset56));L.palmDiameter56[0]=L.palmDiameter56[1]=.01f;LoadLever1887Settings(path);
 Check(fabs(L.palmOffset56[0][0]-.02)<1e-6&&fabs(L.palmOffset56[1][2]+.03)<1e-6&&fabs(L.palmDiameter56[0]-.12)<1e-6&&fabs(L.palmDiameter56[1]-.08)<1e-6,"1887 independent palm positions and diameters survive restart");
 Check(fabs(L.lever[0]-savedLever)<1e-6&&fabs(L.gate[1]-savedGate)<1e-6&&fabs(L.guideSize[0][0]-savedSize)<1e-6,"1887 new palm keys preserve saved legacy positions and box sizes");DeleteFileW(path);
 g_lever=oldLever;g_freeHandAttachment[0]=oldAttachment0;g_freeHandAttachment[1]=oldAttachment1;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_frame=oldFrame;
 g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_twoHand=oldTwo;g_pistolMenu.open=oldMenu;g_worldScale=oldScale;memcpy(g_handBody,body,sizeof(body));fc3pad::Publish(oldControls);
 puts("PASS:1887 calibrated real palms, independent diameters, pinned-hand exclusion, raw physical lever stroke and grip-only free-right ammo with weapon-contact priority");
}
