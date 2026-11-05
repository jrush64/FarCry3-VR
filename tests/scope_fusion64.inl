void TestScopeFusion64(){
 const auto oldLeft=g_eyeViews[0],oldRight=g_eyeViews[1];
 for(int eye=0;eye<2;++eye){auto& e=g_eyeViews[eye];const auto identity=HandIdentity();for(int i=0;i<16;++i)e.view[i]=float(identity.m[i]);e.view[3]=eye?-.032f:.032f;e.tanLeft=eye?-.9f:-1.1f;e.tanRight=eye?1.1f:.9f;e.tanUp=eye?.95f:.85f;e.tanDown=eye?-.85f:-.95f;}
 const ImVec2 center[2]={ScopeTargetCenter64(0),ScopeTargetCenter64(1)};
 // Independently reconstruct the physical viewing rays from XR half-angle
 // tangents. They must intersect at the SAME point on the shared 0.70m plane.
 auto hit=[&](int eye,ImVec2 ndc){const auto& e=g_eyeViews[eye];const double x=e.tanLeft+(ndc.x+1)*.5*(e.tanRight-e.tanLeft),y=e.tanDown+(ndc.y+1)*.5*(e.tanUp-e.tanDown);return V3{(eye?.032:-.032)+x*.70,y*.70,.70};};
 const auto left=hit(0,center[0]),right=hit(1,center[1]);const double gap=Length(Sub(left,right)),oldGap=Length(Sub(hit(0,{}),hit(1,{})));printf("scope64 projected centers %.6f %.6f / %.6f %.6f; ray gap %.9fm, old image-center gap %.6fm\n",center[0].x,center[0].y,center[1].x,center[1].y,gap,oldGap);
 Check(gap<1e-6&&Length(Sub(left,{0,0,.70}))<1e-6,"asymmetric XR eyes converge on one scope viewing position");Check(oldGap>.20,"negative control: previous image-centered scopes point at different positions");
 // Separate eye images can have different axes and amplified disparity. Lens
 // mode must map the same texture feature to the same aperture coordinates.
 ScopeOptics60 optics[2]={};optics[0].axis={.13f,-.08f};optics[1].axis={-.23f,.19f};for(int i=0;i<64;++i){optics[0].uv[i]={i*.01f,.2f};optics[1].uv[i]={.9f-i*.01f,.7f};optics[1].edge[i]={i*.02f,-.1f};}const auto originalRight=optics[1];ScopeFuseLens64(optics);for(int i=0;i<64;++i){Check(optics[1].uv[i].x==optics[0].uv[i].x&&optics[1].uv[i].y==optics[0].uv[i].y,"optical image features share UV positions in both eyes");Check(optics[1].edge[i].x==originalRight.edge[i].x&&optics[1].edge[i].y==originalRight.edge[i].y,"sharing optical content preserves separate eye aperture geometry");}Check(optics[1].axis.x==optics[0].axis.x&&optics[1].axis.y==optics[0].axis.y,"reticle and optical image center agree in both eyes");g_eyeViews[0]=oldLeft;g_eyeViews[1]=oldRight;puts("PASS: scope64 asymmetric eye convergence, rejected old binocular placement, common optical features and retained eye aperture geometry");
}
