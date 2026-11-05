void TestGlider257(){
 const auto saved=g_gliderLook257;const auto view=g_eyeViewsBody[0];const bool active=g_aimActive;const bool body=g_bodyValid;
 double savedAxes[3][3],savedQ[3][3];memcpy(savedAxes,g_bodyAxes,sizeof(savedAxes));memcpy(savedQ,g_aimQ,sizeof(savedQ));const double savedYaw=g_bodyYaw;
 fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
 fc3aim::VehicleSample170 sample={};sample.identityValid=sample.seatValid=sample.basisValid=true;sample.type=2;sample.actualSeat=1;sample.pawnId=1;sample.vehicleId=2;
 for(int i=0;i<3;++i)sample.basis[i][i]=1;
 auto camera=[](double yaw,double pitch){CameraSummary c={};double axes[3][3];VisibilityBasisMatrix68(yaw,pitch,0,1,axes);for(int j=0;j<3;++j){c.right[j]=float(axes[0][j]);c.up[j]=float(axes[1][j]);c.forward[j]=float(axes[2][j]);}return c;};
 auto head=[](double yaw,double pitch){auto& e=g_eyeViewsBody[0];e.view[8]=float(std::sin(yaw)*std::cos(pitch));e.view[9]=float(std::sin(pitch));e.view[10]=float(std::cos(yaw)*std::cos(pitch));};
 g_gliderLook257={};head(0,0);Check(TickGliderLook257(sample,camera(0,0),1000),"257 confirmed glider acquires camera");
 head(.6,-1.1);TickGliderLook257(sample,camera(0,0),1010);
 Check(g_gliderLook257.yaw.command>.25&&g_gliderLook257.pitch.command<-.25,"257 glider follows head yaw AND downward pitch");
 const double base=g_gliderLook257.body[2][0];TickGliderLook257(sample,camera(-.2,-.3),1100);
 Check(std::fabs(g_gliderLook257.body[2][0]-base)<1e-8,"257 automatic camera motion cannot rotate the visual reference");
 // R*Q^T == B: observed camera rotation cancels, leaving the original body
 // frame for exactly one HMD rotation. Check all axes, not only yaw.
 const auto c=camera(-.2,-.3);const float* axes[3]={c.right,c.up,c.forward};
 for(int i=0;i<3;++i)for(int j=0;j<3;++j){double v=0;for(int k=0;k<3;++k)v+=axes[k][i]*g_aimQ[k][j];Check(std::fabs(v-g_gliderLook257.body[j][i])<2e-6,"257 pitched native camera compensation retains one head rotation");}
 controls.rightX=.8f;fc3pad::Publish(controls);TickGliderLook257(sample,c,1110);
 Check(g_aimActive&&g_gliderLook257.manual,"257 manual stick retains compensation instead of adding the head twice");
 TickGliderLook257(sample,camera(-.5,-.3),1200);const auto beforeRelease=g_gliderLook257;
 controls.rightX=0;fc3pad::Publish(controls);TickGliderLook257(sample,camera(-.5,-.3),1400);
 Check(!g_gliderLook257.manual&&g_aimActive,"257 head follow resumes after manual stick without an episode-long latch");
 for(int i=0;i<3;++i)for(int j=0;j<3;++j)Check(std::fabs(g_gliderLook257.body[i][j]-beforeRelease.body[i][j])<2e-6,"257 stick release cannot reset the visual reference");
 const auto beforeGap=g_gliderLook257;TickGliderLook257(sample,camera(-.5,-.3),8000);
 for(int i=0;i<3;++i)for(int j=0;j<3;++j)Check(std::fabs(g_gliderLook257.body[i][j]-beforeGap.body[i][j])<2e-6,"257 pause gap cannot add the head offset to its reference");
 sample.type=0;const auto beforeOther=g_gliderLook257;Check(!TickGliderLook257(sample,c,8010)&&memcmp(&beforeOther,&g_gliderLook257,sizeof(beforeOther))==0,"257 other vehicle types keep their prior control policy");
 sample.type=2;sample.vehicleId=3;head(0,0);TickGliderLook257(sample,camera(.7,.1),8020);Check(std::fabs(g_bodyYaw-.7)<1e-6,"257 new vehicle identity starts from its own observed camera");
 sample.basis[0][0]=sample.basis[1][1]=float(std::cos(.3));sample.basis[0][1]=float(std::sin(.3));sample.basis[1][0]=float(-std::sin(.3));
 for(int i=0;i<200;++i)TickGliderLook257(sample,camera(1.,.1),8030+i);
 Check(std::fabs(g_bodyYaw-1.)<2e-5,"257 actual vehicle rotation updates the reference without guessing a forward axis");
 for(int i=0;i<3;++i)for(int j=0;j<3;++j){double dot=0;for(int k=0;k<3;++k)dot+=g_gliderLook257.body[i][k]*g_gliderLook257.body[j][k];Check(std::fabs(dot-(i==j?1:0))<1e-8,"257 repeated float basis samples cannot introduce view scale or shear");}
 GliderAxis257 axis={};GliderAxisDrive257(axis,.5,0,1000);GliderAxisDrive257(axis,.5,-.1,1400);Check(axis.sign==-1&&!axis.blocked,"257 reversed axis learned from actual camera response");
 GliderAxisDrive257(axis,.5,-.2,1800);GliderAxisDrive257(axis,.5,-.3,2200);GliderAxisDrive257(axis,.5,-.4,2600);
 Check(axis.sign==-1&&axis.learned,"258 subsequent opposite camera samples cannot repeatedly flip a learned axis");
 axis={};GliderAxisDrive257(axis,.5,0,1000);Check(GliderAxisDrive257(axis,.5,0,2600)==0&&axis.blocked,"257 unresponsive axis stops rather than pushing forever");
 g_gliderLook257=saved;g_eyeViewsBody[0]=view;g_aimActive=active;g_bodyValid=body;g_bodyYaw=savedYaw;memcpy(g_bodyAxes,savedAxes,sizeof(savedAxes));memcpy(g_aimQ,savedQ,sizeof(savedQ));fc3pad::SetLookOverride(false,0,0);controls={};fc3pad::Publish(controls);
 puts("PASS:257 glider yaw/pitch response, exact camera compensation, manual-turn continuity, release/pause stability, owner/type guards and reverse/stalled axis handling");
}
