// [TRIGGER262] the right index chain bends by the trigger: measured joint angles.
static Mat4 TurnAbout262(const double axis[3],const double point[3],double angle){
 const double c=std::cos(angle),s=std::sin(angle),t=1-c;Mat4 m={};m.m[15]=1;
 const double rr[3][3]={{t*axis[0]*axis[0]+c,t*axis[0]*axis[1]-s*axis[2],t*axis[0]*axis[2]+s*axis[1]},{t*axis[0]*axis[1]+s*axis[2],t*axis[1]*axis[1]+c,t*axis[1]*axis[2]-s*axis[0]},{t*axis[0]*axis[2]-s*axis[1],t*axis[1]*axis[2]+s*axis[0],t*axis[2]*axis[2]+c}};
 for(int i=0;i<3;++i){for(int k=0;k<3;++k)m.m[i*4+k]=rr[i][k];m.m[i*4+3]=point[i]-(rr[i][0]*point[0]+rr[i][1]*point[1]+rr[i][2]*point[2]);}
 return m;
}
static double RelativeAngle262(const float* bones,int parent,int child){
 Mat4 inv={};if(!Invert(BoneMatrix(bones,parent),inv))return -1;const Mat4 r=Multiply(inv,BoneMatrix(bones,child));
 double c=(r.m[0]+r.m[5]+r.m[10]-1)*.5;c=(std::max)(-1.0,(std::min)(1.0,c));return std::acos(c);
}
void TestTriggerFinger262(){
 float bones[25*12];for(int b=0;b<25;++b)for(int i=0;i<12;++i)bones[b*12+i]=(i%5==0&&i<11)?1.f:0.f; // identity 3x4 for every bone
 const FingerChain& chain=kFingers[1];const double axis[3]={0,1,0};const double bend=30*0.0174533;
 Mat4 acc={};acc.m[0]=acc.m[5]=acc.m[10]=acc.m[15]=1;
 for(int k=0;k<3;++k){acc=Multiply(acc,TurnAbout262(axis,chain.joints[k],bend));StoreBone(bones,chain.bones[k],acc);} // the game's grip: 30 deg at each index joint
 float work[25*12];memcpy(work,bones,sizeof(work));
 TriggerFingerCurl262(work,1.0);
 for(int k=0;k<3;++k)Check(std::fabs(RelativeAngle262(work,k?chain.bones[k-1]:chain.parent,chain.bones[k])-bend)<1e-6,"262 amount 1 keeps the game's grip pose");
 memcpy(work,bones,sizeof(work));TriggerFingerCurl262(work,0.5);
 for(int k=0;k<3;++k)Check(std::fabs(RelativeAngle262(work,k?chain.bones[k-1]:chain.parent,chain.bones[k])-bend*.5)<1e-6,"262 amount 0.5 halves every index joint's bend about its own joint");
 // the fingertip stays attached: the first joint point is a fixed point of the first bone's change
 {const double* j=chain.joints[0];const V3 before=Apply(BoneMatrix(bones,chain.bones[0]),{j[0],j[1],j[2]}),after=Apply(BoneMatrix(work,chain.bones[0]),{j[0],j[1],j[2]});
  Check(Length(Sub(before,after))<1e-6,"262 the bend pivots on the joint point (no finger detachment)");}
 for(int b=0;b<25;++b)if(b!=7&&b!=8&&b!=9)for(int i=0;i<12;++i)Check(work[b*12+i]==bones[b*12+i],"262 only the index chain changes");
 fc3pad::Controllers c={};c.active=true;
 c.rightTrigger=1;c.rightIndexTouch=true;Check(std::fabs(TriggerFingerAmount262(c)-1.0)<1e-9,"262 full pull = the game's pose");
 c.rightTrigger=0;c.rightIndexTouch=true;Check(std::fabs(TriggerFingerAmount262(c)-g_triggerFingerTouch262)<1e-9,"262 finger on the sensor, no pull = touch pose");
 c.rightIndexTouch=false;Check(std::fabs(TriggerFingerAmount262(c)-g_triggerFingerRest262)<1e-9,"262 finger off the trigger = rest pose");
 c.rightTrigger=.5f;Check(std::fabs(TriggerFingerAmount262(c)-(g_triggerFingerTouch262+.5*(1-g_triggerFingerTouch262)))<1e-6,"262 a half pull without the sensor still counts as touching");
 const auto saved=fc3pad::Latest();fc3pad::Controllers off={};fc3pad::Publish(off);memcpy(work,bones,sizeof(work));TriggerFinger262(work);
 Check(memcmp(work,bones,sizeof(work))==0,"262 no controllers: the pose is untouched");
 fc3pad::Publish(saved);
 puts("PASS:262 trigger finger bends the right index chain by the trigger pull about its own joints and leaves everything else alone");
}
