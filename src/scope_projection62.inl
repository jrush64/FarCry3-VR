// Authoritative current shot ray, rather than last frame's animated lens axis.
LONG g_scopeShotFrame129[2]={-1000,-1000};
bool ScopeShotAxis129(V3& axis){
 if(!g_aimActive||g_shotAimProfile129!=g_weaponMechanicProfile||g_shotAimEpoch107!=g_armPoseEpoch||
    std::abs(g_frame-g_shotAimFrame107)>1||Length(g_shotAimWorld107)<.5)return false;
 axis=Unit(g_shotAimWorld107);return std::isfinite(axis.x)&&std::isfinite(axis.y)&&std::isfinite(axis.z);
}
ImVec2 ScopeSampleOffset129(ImVec2 physical,int eye,float zoom){
 // The optical raster was centred on the outgoing ray in this exact frame.
 // The reticle at the lens centre must sample that centre, not a second,
 // independently animated/captured axis. Older packets retain their old path.
 if(eye>=0&&eye<2&&g_scopeShotFrame129[eye]==g_frame)return {0,0};
 return {zoom*(physical.x-g_scopeSceneAxis62[eye][0]),zoom*(physical.y-g_scopeSceneAxis62[eye][1])};
}
// Optical rasterization and viewmodel presentation are deliberately separate.
// The first changes scene detail; the second makes the ocular readable without
// moving controller poses, reload contacts or the gameplay shot origin.
Mat4 ScopeCrop62(double scale,double x,double y){Mat4 h=HandIdentity();h.m[0]=h.m[5]=scale;h.m[3]=(1-scale)*x;h.m[7]=(1-scale)*y;return h;}
ScopeEye60 ScopeBase62(const ScopeEye60& packet){auto base=packet;for(int i=0;i<16;++i)base.draw.eye[i]=packet.baseEye[i];return base;}
Mat4 ScopeRigBody72(int side){
 Mat4 rig={};for(int i=0;i<16;++i)rig.m[i]=g_handBody[side][i];
 const bool stabilized=side==0&&ScopeAimValid72();if(stabilized)rig=g_scopeAim72.basis;
 const V3 wrist=stabilized?g_scopeAim72.pivot:ControllerWrist(g_handBody[side],side);rig.m[3]=wrist.x;rig.m[7]=wrist.y;rig.m[11]=wrist.z;return rig;
}
Mat4 ScopeBodyView72(int eye){Mat4 view={};const auto& e=g_aimActive?g_eyeViewsBody[eye]:g_eyeViews[eye];for(int i=0;i<16;++i)view.m[i]=e.view[i];return view;}
void ScopeCaptureBody72(ScopeEye60& packet,int eye){
 packet.bodyValid72=false;if(packet.spec<0||!g_eyeViewsSet||!g_handRightValid)return;
 packet.bodySide72=R700LeftHold()?1:0;if(packet.bodySide72&&!g_handLeftValid)return;
 Mat4 inverseRig={},inverseView={};if(!Invert(ScopeRigBody72(packet.bodySide72),inverseRig)||!Invert(ScopeBodyView72(eye),inverseView))return;
 const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);if(fabs(projection.m[0])<1e-6||fabs(projection.m[5])<1e-6)return;
 auto base=ScopeBase62(packet);if(packet.bodyDepth72)base.draw.camera[111]=0;const auto& spec=kScopeSpecs60[packet.spec];
 auto local=[&](V3 point,V3& out){float clip[4]={};if(!Native1911Clip48(base.draw,point,clip)||clip[3]<=.025)return false;
  const V3 eyePoint={(clip[0]-projection.m[2]*clip[3])/projection.m[0],(clip[1]-projection.m[6]*clip[3])/projection.m[5],clip[3]};
  out=Apply(inverseRig,Apply(inverseView,eyePoint));return std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z);};
 if(!local(spec.center,packet.localCenter72)||!local(Add(spec.center,{0,100,0}),packet.localAxis72))return;
 for(int i=0;i<64;++i){const double a=6.283185307179586*i/64;const V3 point=spec.outline?spec.outline[i]:Add(spec.center,{spec.radius*std::cos(a),0,spec.radius*std::sin(a)});if(!local(point,packet.localEdge72[i]))return;}
 packet.bodyValid72=true;
}
bool ScopeBodyOptics72(const ScopeEye60& packet,int eye,ScopeOptics60& out){
 if(!packet.bodyValid72||packet.bodySide72!=(R700LeftHold()?1:0))return false;
 const Mat4 rig=ScopeRigBody72(packet.bodySide72),view=ScopeBodyView72(eye);const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);
 auto project=[&](V3 local,ImVec2& ndc){V3 body=Apply(rig,local);if(packet.bodyDepth72&&fabs(packet.draw.camera[111])>.000001){Mat4 inverseTurn={};const bool corrected=ScopeAimValid72()&&Invert(g_scopeAim72.turn,inverseTurn);V3 raw=corrected?Apply(inverseTurn,body):body;const double distance=Length(raw);if(distance<1e-6)return false;raw=Add(raw,Scale(raw,double(packet.draw.camera[111])/distance));body=corrected?Apply(g_scopeAim72.turn,raw):raw;}const V3 p=Apply(view,body);if(p.z<=.025)return false;ndc={float(projection.m[0]*p.x/p.z+projection.m[2]),float(projection.m[5]*p.y/p.z+projection.m[6])};return std::isfinite(ndc.x)&&std::isfinite(ndc.y);};
 if(!project(packet.localCenter72,out.center)||!project(packet.localAxis72,out.axis))return false;
 for(int i=0;i<64;++i)if(!project(packet.localEdge72[i],out.edge[i]))return false;out.valid=true;return true;
}
bool ScopeBodyWorldAxis72(int eye,V3& axis){
 const auto& p=g_scopeEyes60[eye];if(!p.bodyValid72||!g_bodyValid||p.bodySide72!=(R700LeftHold()?1:0))return false;
 const Mat4 rig=ScopeRigBody72(p.bodySide72);const V3 h=Unit(Sub(Apply(rig,p.localAxis72),Apply(rig,p.localCenter72)));
 axis={g_bodyAxes[0][0]*h.x+g_bodyAxes[1][0]*h.y+g_bodyAxes[2][0]*h.z,g_bodyAxes[0][1]*h.x+g_bodyAxes[1][1]*h.y+g_bodyAxes[2][1]*h.z,g_bodyAxes[0][2]*h.x+g_bodyAxes[1][2]*h.y+g_bodyAxes[2][2]*h.z};return std::isfinite(axis.x)&&std::isfinite(axis.y)&&std::isfinite(axis.z);
}
bool ScopeFocusPacket62(int eye,ScopeOptics60& optics){
 if(!ScopeCleanAllowed61()||!g_eyeViewsSet||eye<0||eye>1)return false;
 const auto& packet=g_scopeEyes60[eye];if(packet.spec<0||packet.spec>=int(sizeof(kScopeSpecs60)/sizeof(kScopeSpecs60[0]))||(kScopeSpecs60[packet.spec].profile>=0&&kScopeSpecs60[packet.spec].profile!=g_weaponMechanicProfile)||packet.epoch!=g_armPoseEpoch||packet.ownerProfile!=g_weaponMechanicProfile||packet.frame>g_frame||g_frame-packet.frame>2)return false;
 const auto& other=g_scopeEyes60[1-eye];if(other.spec!=packet.spec||other.epoch!=g_armPoseEpoch||other.ownerProfile!=packet.ownerProfile||other.frame>g_frame||g_frame-other.frame>2)return false;
 ScopeOptics60 otherOptics={};if(!ScopeBodyOptics72(other,1-eye,otherOptics))otherOptics=ScopeOpticsForEye60(ScopeBase62(other),ScopeZoom63(),true);if(!otherOptics.valid)return false;
 if(!ScopeBodyOptics72(packet,eye,optics))optics=ScopeOpticsForEye60(ScopeBase62(packet),ScopeZoom63(),true);
 return optics.valid;
}
LONG g_scopePresentationFrame72=-1,g_scopePresentationProfile72=-1;UINT g_scopePresentationEpoch72=0;int g_scopePresentationSpec72=-1;
void ResetScopeProjection62(){g_scopePresentationFrame72=-1;ResetScopeStabilization72();for(int eye=0;eye<2;++eye){SafeRelease(g_scopeVs62[eye]);SafeRelease(g_scopePs62[eye]);SafeRelease(g_scopePsTile62[eye]);SafeRelease(g_scopeTile62[eye]);g_scopeProjectionFrame62[eye]=-1;const auto identity=HandIdentity();for(int i=0;i<16;++i)g_scopePresentation62[eye][i]=float(identity.m[i]);}}
bool ScopeBuffer62(ID3D11Buffer*& buffer,UINT bytes){if(buffer)return true;D3D11_BUFFER_DESC d={};d.ByteWidth=bytes;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;return SUCCEEDED(g_dev->CreateBuffer(&d,nullptr,&buffer));}
// Image center is not the same viewing ray in asymmetric XR frusta. Position
// the ocular on ONE head-forward plane, then project it separately for each
// eye. Preserve the headset's real eye separation and projection offsets.
ImVec2 ScopeTargetCenter64(int eye){
 Mat4 view[2]={},inverse[2]={};for(int e=0;e<2;++e){for(int i=0;i<16;++i)view[e].m[i]=g_eyeViews[e].view[i];if(!Invert(view[e],inverse[e]))return {};}
 const V3 origin={(inverse[0].m[3]+inverse[1].m[3])*.5,(inverse[0].m[7]+inverse[1].m[7])*.5,(inverse[0].m[11]+inverse[1].m[11])*.5};
 const V3 forward=Unit({inverse[0].m[2]+inverse[1].m[2],inverse[0].m[6]+inverse[1].m[6],inverse[0].m[10]+inverse[1].m[10]});
 const V3 p=Apply(view[eye],Add(origin,Scale(forward,.70)));const auto& e=g_eyeViews[eye];if(p.z<=.025||e.tanRight-e.tanLeft<.01||e.tanUp-e.tanDown<.01)return {};
 const auto projection=EyeProjectionMatrix(e,g_worldProjection);const ImVec2 center={float(projection.m[0]*p.x/p.z+projection.m[2]),float(projection.m[5]*p.y/p.z+projection.m[6])};return std::isfinite(center.x)&&std::isfinite(center.y)?center:ImVec2{};
}
bool ScopeTargetCenters72(const ScopeOptics60* optics,ImVec2* centers){
 Mat4 view[2]={},inverse[2]={};V3 origin={},direction={};
 for(int eye=0;eye<2;++eye){for(int i=0;i<16;++i)view[eye].m[i]=g_eyeViews[eye].view[i];if(!Invert(view[eye],inverse[eye]))return false;
  const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);if(fabs(projection.m[0])<1e-6||fabs(projection.m[5])<1e-6)return false;
  const V3 ray={(optics[eye].center.x-projection.m[2])/projection.m[0],(optics[eye].center.y-projection.m[6])/projection.m[5],1};
  const V3 eyeOrigin=Apply(inverse[eye],{});origin=Add(origin,Scale(eyeOrigin,.5));direction=Add(direction,Unit(Sub(Apply(inverse[eye],ray),eyeOrigin)));
 }
 if(Length(direction)<1e-6)return false;const V3 target=Add(origin,Scale(Unit(direction),.70));
 for(int eye=0;eye<2;++eye){const V3 p=Apply(view[eye],target);if(p.z<=.025)return false;const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);
  centers[eye]={float(projection.m[0]*p.x/p.z+projection.m[2]),float(projection.m[5]*p.y/p.z+projection.m[6])};if(!std::isfinite(centers[eye].x)||!std::isfinite(centers[eye].y))return false;}
 return true;
}
struct ScopeSizeState67 {double scale=1;LONG frame=-1,profile=-1;UINT epoch=0;int spec=-1;ULONGLONG time=0;}g_scopeSizeState67[2];
double ScopeSizeFilter67(double wanted,int eye,bool active){
 auto& s=g_scopeSizeState67[eye];if(!active){s={};return wanted;}
 const auto now=GetTickCount64();const auto& packet=g_scopeEyes60[eye];
 if(s.frame<0||s.profile!=g_weaponMechanicProfile||s.epoch!=g_armPoseEpoch||s.spec!=packet.spec||g_scopeSizeSteady67<=0||now-s.time>250){s.scale=wanted;s.frame=g_frame;s.profile=g_weaponMechanicProfile;s.epoch=g_armPoseEpoch;s.spec=packet.spec;s.time=now;return wanted;}
 if(s.frame!=g_frame){const double dt=(std::max)(.001,double(now-s.time)*.001),tau=double(g_scopeSizeSteady67)*.15;s.scale+=(wanted-s.scale)*dt/(dt+tau);s.frame=g_frame;s.time=now;}
 return s.scale;
}
void ScopePresentation62(float* values,int variant,int eye){
 if((variant!=1&&variant!=3&&variant!=4)||eye<0||eye>1)return;
 // Physical optics131: never enlarge, translate or converge the gun/arms.
 // Lens geometry is projected exactly where the game and controller rig drew it.
 (void)values;const auto identity=HandIdentity();for(int e=0;e<2;++e){for(int i=0;i<16;++i)g_scopePresentation62[e][i]=float(identity.m[i]);ScopeSizeFilter67(1,e,false);}
 g_scopePresentationFrame72=g_frame;g_scopePresentationEpoch72=g_armPoseEpoch;g_scopePresentationProfile72=g_weaponMechanicProfile;g_scopePresentationSpec72=g_scopeEyes60[0].spec;
}
void UpdateScopeProjection62(const double* world,int eye){
 g_scopeProjectionFrame62[eye]=-1;g_scopeShotFrame129[eye]=-1000;ScopeOptics60 optics;if(!ScopeFocusPacket62(eye,optics))return;
 if(!ScopeBuffer62(g_scopeVs62[eye],dxbc::kEyeRows*16)||!ScopeBuffer62(g_scopePs62[eye],80)||!ScopeBuffer62(g_scopePsTile62[eye],80)||!ScopeBuffer62(g_scopeTile62[eye],48))return;
 const double zoom=ScopeZoom63();
 Mat4 normal={};memcpy(normal.m,world,sizeof(normal.m));
 // Reproject the observed weapon axis through THIS scene camera and current
 // eye matrix. A prior lens draw must not retain the prior head-view axis.
 if(g_frameCameraValid&&g_frameCamera.mainView){const auto& packet=g_scopeEyes60[eye];const auto& spec=kScopeSpecs60[packet.spec];const V3 point=Apply(packet.draw.anchor.world,Add(spec.center,{0,100,0}));V3 stableAxis72={};const bool shot129=ScopeShotAxis129(stableAxis72);const V3 relative=(shot129||ScopeBodyWorldAxis72(eye,stableAxis72)||ScopeStableWorldAxis72(stableAxis72))?Scale(stableAxis72,100):Sub(point,{g_frameCamera.position[0],g_frameCamera.position[1],g_frameCamera.position[2]});const auto dot=[&](const float* v){return v[0]*relative.x+v[1]*relative.y+v[2]*relative.z;};const double depth=dot(g_frameCamera.forward);const double p[4]={dot(g_frameCamera.right)*g_frameCamera.xScale,dot(g_frameCamera.up)*g_frameCamera.yScale,depth*g_frameCamera.zScale+g_frameCamera.zOffset,depth};double projected[4]={};for(int r=0;r<4;++r)for(int c=0;c<4;++c)projected[r]+=normal.m[r*4+c]*p[c];if(projected[3]>.025){optics.axis={float(projected[0]/projected[3]),float(projected[1]/projected[3])};if(shot129)g_scopeShotFrame129[eye]=g_frame;}}
 Mat4 crop=HandIdentity();crop.m[0]=crop.m[5]=zoom;crop.m[3]=-zoom*optics.axis.x;crop.m[7]=-zoom*optics.axis.y;
 const Mat4 scoped=Multiply(crop,normal);Mat4 inverse={};if(!Invert(scoped,inverse))return;
 float rows[dxbc::kEyeRows*4]={},columns[20]={},tile[12]={};for(int i=0;i<16;++i)rows[i]=float(scoped.m[i]-(i%5==0?1.0:0.0));
 for(int col=0;col<4;++col)for(int row=0;row<4;++row)columns[col*4+row]=float(inverse.m[row*4+col]-(row==col?1.0:0.0));
 const int indices[3]={0,1,3};const double d=30,a=g_worldProjection.a,b=g_worldProjection.b;
 for(int row=0;row<3;++row){const auto* m=scoped.m+indices[row]*4;tile[row*4]=float(m[0]*d);tile[row*4+1]=float(m[1]*d);tile[row*4+2]=float(m[2]*(a*d+b)+m[3]*d);}
 // Preserve the ordinary lighting coverage margin in the enlarged raster.
 float ordinaryTile[12]={};if(ReadCopy(g_tileCb[eye],ordinaryTile,sizeof(ordinaryTile))==sizeof(ordinaryTile)){tile[3]=ordinaryTile[3]*float(zoom);tile[7]=ordinaryTile[7]*float(zoom);}else tile[3]=tile[7]=.6f*float(zoom);
 const bool own=g_own;g_own=true;g_oUpdateSubresource(g_ctx,g_scopeVs62[eye],0,nullptr,rows,0,0);g_oUpdateSubresource(g_ctx,g_scopePs62[eye],0,nullptr,columns,0,0);columns[16]=1;g_oUpdateSubresource(g_ctx,g_scopePsTile62[eye],0,nullptr,columns,0,0);g_oUpdateSubresource(g_ctx,g_scopeTile62[eye],0,nullptr,tile,0,0);g_own=own;
 g_scopeSceneAxis62[eye][0]=optics.axis.x;g_scopeSceneAxis62[eye][1]=optics.axis.y;g_scopeProjectionFrame62[eye]=g_frame;
}
