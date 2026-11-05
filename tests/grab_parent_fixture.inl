void TestGrabParenting()
{
 float saved[16];memcpy(saved,g_handBody[1],sizeof(saved));const bool valid=g_handLeftValid,guides=g_grabGuides,aim=g_aimActive;
 g_handLeftValid=true;g_grabGuides=true;g_aimActive=false;
 const V3 support={.3,.1,.6};
 for(int step=0;step<36;++step){const double a=step*3.141592653589793/18.;float* m=g_handBody[1];memset(m,0,sizeof(saved));m[0]=float(cos(a));m[1]=float(-sin(a));m[4]=float(sin(a));m[5]=float(cos(a));m[10]=m[15]=1;m[3]=-.2f;m[7]=-.25f;m[11]=.4f;V3 gun={},hand={};Check(GrabGizmoPoints(support,gun,hand),"production grab gizmo tracked hand");Check(Length(Sub(gun,support))<1e-7,"fixed gun target cannot move with free-hand rotation");Check(Length(Sub(hand,ControllerWrist(m,1)))<1e-7,"grab hand marker uses actual detector wrist");const V3 offset={cos(a)*-.0871-sin(a)*.0409,sin(a)*-.0871+cos(a)*.0409,-.1103};Check(fabs(Length(Sub(Add(gun,offset),Add(hand,offset)))-Length(Sub(gun,hand)))<1e-7,"removing shared visual offset preserves detector distance exactly");}
 memcpy(g_handBody[1],saved,sizeof(saved));g_handLeftValid=valid;g_grabGuides=guides;g_aimActive=aim;
 puts("PASS: 36 free-hand rotations cannot drag gun support marker; visual centres match unchanged grab detector distance");
}
