// Native lighter geometry is read from a local, game-derived sidecar. No archive
// edits, no copyrighted mesh embedded in the proxy or published source.
struct LighterVertex158 {float xyz[3];UINT flags;float uv[2],normal[3],tangent[4];};
struct LighterTexture159 {UINT width=0,height=0;std::vector<BYTE> rgba;};
constexpr float kLighterDefaults164[16]={.017f,-.017f,-.025f,12.6f,-69.2f,-25.1f,1.f,0.f,.001f,0.f,0.f,0.f,0.f,2.89f,2.01f,2.29f};
struct MolotovManual158 {
 bool manualIgnition159=true,guides159=true,contactFeedback159=false;int stage159=0;float fromLid159=0;float closeAngle159=1.683f;LighterTexture159 texture159[3];
 bool enabled=true,held=false,flame=false,lit=false,requested=false,previousTrigger=false,loaded=false;
 UINT epoch=0;unsigned serial=0;ULONGLONG contact=0,openedAt=0;float lid=0;
 std::vector<LighterVertex158> mesh;V3 pivot={},anchor={};
 float offset[3]={kLighterDefaults164[0],kLighterDefaults164[1],kLighterDefaults164[2]},rotation[3]={kLighterDefaults164[3],kLighterDefaults164[4],kLighterDefaults164[5]};
 bool calibrate163=false,previewOpen163=true;int saveStatus163=0;float scale163=1,flameOffset163[3]={0,kLighterDefaults164[8],0},flameRotation163[3]={},flameScale163[3]={kLighterDefaults164[13],kLighterDefaults164[14],kLighterDefaults164[15]};
 LONG visualFrame=-100000;UINT visualEpoch=0;std::vector<float> clip[2];
}g_molotov158;
bool ManualMolotov158(){return g_molotov158.enabled&&g_throw144.owned&&g_throw144.kind==2;}
bool MolotovHeld158(){return ManualMolotov158()&&ThrowableLease144()&&!g_throw144.releasing;}
bool LighterPreview163(){return g_molotov158.calibrate163&&g_pistolMenu.open&&g_inGameplay&&g_motionGun&&g_handLeftValid&&!fc3pad::NativeMenuOpen()&&!fc3aim::NativeActionOwnsArms128();}
bool LighterActive163(){return LighterPreview163()||(MolotovHeld158()&&g_molotov158.held);}
bool MolotovRagGuide177(){return MolotovHeld158()&&g_molotovRag177.guide;}
bool MolotovVisual177(){return LighterActive163()||MolotovRagGuide177();}
float* LighterValue163(int n){auto& s=g_molotov158;return n<3?s.offset+n:n<6?s.rotation+n-3:n==6?&s.scale163:n<10?s.flameOffset163+n-7:n<13?s.flameRotation163+n-10:s.flameScale163+n-13;}
const wchar_t* const kLighterKeys163[]={L"PositionX",L"PositionY",L"PositionZ",L"RotationX",L"RotationY",L"RotationZ",L"Scale",L"FlamePositionX",L"FlamePositionY",L"FlamePositionZ",L"FlameRotationX",L"FlameRotationY",L"FlameRotationZ",L"FlameScaleX",L"FlameScaleY",L"FlameScaleZ"};
void LoadLighterSettings163(const wchar_t* path){LoadMolotovRag177(path);for(int n=0;n<16;++n){const bool scale=n==6||n>=13,angle=(n>=3&&n<6)||(n>=10&&n<13);const float fallback=kLighterDefaults164[n];wchar_t text[64]={};GetPrivateProfileStringW(L"LighterCalibration",kLighterKeys163[n],L"",text,64,path);wchar_t* end=nullptr;float value=text[0]?float(wcstod(text,&end)):fallback;if(text[0]&&(!end||end==text||*end))value=fallback;const float limit=angle?180.f:.3f;if(!std::isfinite(value)||(scale?(value<.1f||value>4.f):fabs(value)>limit))value=fallback;*LighterValue163(n)=value;}}
bool SaveLighterSettings163(const wchar_t* path){bool ok=SaveMolotovRag177(path);for(int n=0;n<16;++n){wchar_t value[48];swprintf_s(value,L"%.9g",double(*LighterValue163(n)));if(!WritePrivateProfileStringW(L"LighterCalibration",kLighterKeys163[n],value,path))ok=false;}return ok;}
void MolotovRagMenu177(){
 auto& s=g_molotovRag177;if(!ImGui::TreeNode("Molotov rag ignition and flame positioning"))return;
 ImGui::TextWrapped("Draw and HOLD an unlit Molotov in your RIGHT hand, then open Insert. Enable guides: GREEN capsule is the ignition contact area; ORANGE cross is the native rag flame origin. Move the green guide onto the hanging cloth. Flame position is separate from the lighter burner.");
 ImGui::Checkbox("Show rag trigger and flame guides",&s.guide);
 bool edit=ImGui::DragFloat3("Rag trigger offset (m)",s.contact,.001f,-.3f,.3f,"%.3f",ImGuiSliderFlags_AlwaysClamp);
 edit=ImGui::SliderFloat("Rag trigger radius (m)",&s.radius,.005f,.08f,"%.3f",ImGuiSliderFlags_AlwaysClamp)||edit;
 edit=ImGui::DragFloat3("Rag flame offset (m)",s.flame,.001f,-.3f,.3f,"%.3f",ImGuiSliderFlags_AlwaysClamp)||edit;if(edit)s.saved=0;
 wchar_t path[MAX_PATH];ManualSettingsPath(path);if(ImGui::Button("Save Molotov rag trigger and flame"))s.saved=SaveMolotovRag177(path)?1:-1;
 if(ImGui::Button("Reload saved Molotov rag values")){LoadMolotovRag177(path);s.saved=0;}ImGui::SameLine();if(ImGui::Button("Reset rag to cloth defaults")){for(int n=0;n<7;++n)*MolotovRagValue177(n)=n==6?.025f:0.f;s.saved=0;}
 if(s.saved==1)ImGui::TextWrapped("Saved for next time.");else if(s.saved<0)ImGui::TextWrapped("Save failed; current adjustments remain here.");
 ImGui::TextWrapped("Offsets follow the cloth. Close Insert and touch the GREEN capsule with the lighter flame to test. This does not change the accepted lighter calibration or throwing.");ImGui::TreePop();
}
void LighterMenu163(){MolotovRagMenu177();
 auto& s=g_molotov158;if(!ImGui::TreeNode("Lighter positioning and flame"))return;
 ImGui::TextWrapped("Enable preview, then move your left hand into view. No grip or trigger hold is needed. Adjust the lighter in the posed hand, then save as your default.");
 ImGui::Checkbox("Calibrate lighter (held-pose preview)",&s.calibrate163);ImGui::Checkbox("Preview open lid and flame",&s.previewOpen163);
 bool edited=false;edited=ImGui::DragFloat3("Lighter position (m)",s.offset,.001f,-.3f,.3f,"%.3f",ImGuiSliderFlags_AlwaysClamp)||edited;edited=ImGui::SliderFloat3("Lighter rotation (degrees)",s.rotation,-180,180,"%.1f",ImGuiSliderFlags_AlwaysClamp)||edited;edited=ImGui::SliderFloat("Lighter scale",&s.scale163,.1f,4.f,"%.2fx",ImGuiSliderFlags_AlwaysClamp)||edited;
 ImGui::Separator();ImGui::TextWrapped("Flame adjustments are relative to the lighter's burner. Moving or scaling the lighter carries the flame with it.");
 edited=ImGui::DragFloat3("Flame position (m)",s.flameOffset163,.001f,-.3f,.3f,"%.3f",ImGuiSliderFlags_AlwaysClamp)||edited;edited=ImGui::SliderFloat3("Flame rotation (degrees)",s.flameRotation163,-180,180,"%.1f",ImGuiSliderFlags_AlwaysClamp)||edited;edited=ImGui::SliderFloat3("Flame scale (width / depth / height)",s.flameScale163,.1f,4.f,"%.2fx",ImGuiSliderFlags_AlwaysClamp)||edited;
 if(edited)s.saveStatus163=0;
 wchar_t path[MAX_PATH];ManualSettingsPath(path);
 if(ImGui::Button("Save lighter and flame as default"))s.saveStatus163=SaveLighterSettings163(path)?1:-1;
 if(ImGui::Button("Reload saved lighter default")){LoadLighterSettings163(path);s.saveStatus163=0;}ImGui::SameLine();if(ImGui::Button("Reset to shipped lighter default")){for(int n=0;n<16;++n)*LighterValue163(n)=kLighterDefaults164[n];s.saveStatus163=0;}
 if(s.saveStatus163==1)ImGui::TextWrapped("Saved. These values will load next time.");else if(s.saveStatus163<0)ImGui::TextWrapped("Save failed. Your adjustments are still available here; try saving again.");
 ImGui::TextWrapped("Closing Insert ends the preview. Normal use remains LEFT GRIP to hold, LT to open/light, release LT to close. This does not change the Molotov's own flame.");ImGui::TreePop();
}
bool LoadLighter158(const wchar_t* source=nullptr){
 auto& s=g_molotov158;if(s.loaded)return !s.mesh.empty();s.loaded=true;
 wchar_t path[MAX_PATH];swprintf_s(path,L"%sfc3_lighter159.mesh",fc3::DataFolder());if(source)wcscpy_s(path,source);
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE){Log("[LIGHTER159] missing chrome mesh: %ls",path);return false;}
 struct Header {UINT magic,version,count;float pivot[3],anchor[3],angle;} h={};DWORD got=0;
 bool ok=ReadFile(file,&h,sizeof(h),&got,nullptr)&&got==sizeof(h)&&h.magic==0x3954494c&&h.version==2&&h.count==1386&&GetFileSize(file,nullptr)==sizeof(h)+h.count*sizeof(LighterVertex158);
 for(int i=0;i<3;++i)ok=ok&&std::isfinite(h.pivot[i])&&std::isfinite(h.anchor[i])&&fabs(h.pivot[i])<.15&&fabs(h.anchor[i])<.15;
 ok=ok&&std::isfinite(h.angle)&&h.angle>1&&h.angle<2;
 if(ok){s.mesh.resize(h.count);ok=ReadFile(file,s.mesh.data(),UINT(s.mesh.size()*sizeof(LighterVertex158)),&got,nullptr)&&got==s.mesh.size()*sizeof(LighterVertex158);s.pivot={h.pivot[0],h.pivot[1],h.pivot[2]};s.anchor={h.anchor[0],h.anchor[1],h.anchor[2]};s.closeAngle159=h.angle;}
 CloseHandle(file);
 for(const auto& v:s.mesh){for(float value:v.xyz)ok=ok&&std::isfinite(value)&&fabs(value)<.15;for(float value:v.uv)ok=ok&&std::isfinite(value)&&fabs(value)<16;for(float value:v.normal)ok=ok&&std::isfinite(value)&&fabs(value)<1.01f;for(float value:v.tangent)ok=ok&&std::isfinite(value)&&fabs(value)<1.01f;ok=ok&&(v.flags==0||v.flags==2);}
 wchar_t* slash=wcsrchr(path,L'\\');wchar_t* other=wcsrchr(path,L'/');if(other&&(!slash||other>slash))slash=other;if(slash)wcscpy_s(slash+1,MAX_PATH-(slash+1-path),L"fc3_lighter159.material");else wcscpy_s(path,L"fc3_lighter159.material");
 file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);UINT head[3]={};ok=ok&&file!=INVALID_HANDLE_VALUE;
 if(ok)ok=ReadFile(file,head,sizeof(head),&got,nullptr)&&got==sizeof(head)&&head[0]==0x3958544c&&head[1]==1&&head[2]==3;
 for(int i=0;i<3&&ok;++i){UINT d[3]={};ok=ReadFile(file,d,sizeof(d),&got,nullptr)&&got==sizeof(d)&&d[0]>=16&&d[0]<=512&&d[1]>=16&&d[1]<=512&&d[2]==d[0]*d[1]*4;if(!ok)break;auto& t=s.texture159[i];t.width=d[0];t.height=d[1];t.rgba.resize(d[2]);ok=ReadFile(file,t.rgba.data(),d[2],&got,nullptr)&&got==d[2];}
 if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);if(!ok)s.mesh.clear();Log("[LIGHTER159] native chrome mesh/materials loaded: %zu vertices, lid %.2f degrees",s.mesh.size(),s.closeAngle159*57.2957795);return ok;
}
void PauseMolotov158(){fc3pad::SetLighterHeld161(false);auto& s=g_molotov158;s.fromLid159=0;s.contactFeedback159=false;s.stage159=0;s.held=s.flame=s.previousTrigger=false;s.contact=s.openedAt=0;s.lid=0;}
void ResetMolotov158(){fc3pad::SetLighterHeld161(false);auto& s=g_molotov158;s.fromLid159=0;s.contactFeedback159=false;s.stage159=0;s.held=s.flame=s.lit=s.requested=s.previousTrigger=false;s.contact=s.openedAt=0;s.lid=0;s.serial=0;s.visualFrame=-100000;}
// Follow actual weighted hanging-cloth vertices. The authored FX_FIRE node
// is at the bottle neck and is not a suitable physical ignition target.
bool MolotovWickBody158(V3& wick){
 Mat4 frame={},inverse={};if(!g_frameCameraValid||!ThrowableFrame144(g_frameCamera,frame)||!Invert(ArmBodyToCamera(),inverse))return false;
 wick=MolotovRagPoint177(Multiply(inverse,frame),false);return std::isfinite(wick.x)&&std::isfinite(wick.y)&&std::isfinite(wick.z);
}
bool MolotovGuide159(){return MolotovHeld158()&&g_molotov158.guides159&&g_molotov158.held;}
bool RagContact159(V3 flame,V3& closest,double& distance){
 Mat4 frame={},inverse={};if(!g_frameCameraValid||!ThrowableFrame144(g_frameCamera,frame)||!Invert(ArmBodyToCamera(),inverse))return false;
 const Mat4 bodyFrame=Multiply(inverse,frame);const V3 a=MolotovRagPoint177(bodyFrame,false),b=MolotovRagOffset177(bodyFrame,MolotovRagSample177(bodyFrame,false),g_molotovRag177.contact),axis=Sub(b,a);
 const double length2=Dot(axis,axis);if(!std::isfinite(length2)||length2<1e-8)return false;
 const double t=(std::max)(0.0,(std::min)(1.0,Dot(Sub(flame,a),axis)/length2));closest=Add(a,Scale(axis,t));distance=Length(Sub(flame,closest));return std::isfinite(distance);
}
bool LighterBody158(Mat4& body){
 V3 palm={};if(!g_handLeftValid||!FreeHandPalm(1,palm))return false;
 Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[1][i];const Mat4 wrist=Multiply(controller,g_freeHandAttachment[1].relative);
 // Anatomical axes from the drawn wrist: index-to-pinky across the case,
 // palm-to-thumb along its upright edge. Raw controller pitch is irrelevant.
 const V3 index=Apply(wrist,{-.658,.076,1.079}),pinky=Apply(wrist,{-.658,-.003,1.073}),thumb=Apply(wrist,{-.610,.065,1.110});
 V3 x=Unit(Sub(index,pinky)),z=Unit(Sub(palm,thumb));z=Unit(Sub(z,Scale(x,Dot(z,x))));const V3 y=Unit(Cross(z,x));x=Unit(Cross(y,z));
 body=HandIdentity();for(int i=0;i<3;++i){body.m[i*4]=(&x.x)[i];body.m[i*4+1]=(&y.x)[i];body.m[i*4+2]=(&z.x)[i];body.m[i*4+3]=(&palm.x)[i]-.024*(&z.x)[i];}
 body=W7Transform(body,g_molotov158.offset,g_molotov158.rotation);if(!RepairRigidMatrix(body))return false;for(int r=0;r<3;++r)for(int c=0;c<3;++c)body.m[r*4+c]*=g_molotov158.scale163;return true;
}
Mat4 LighterFlame163(const Mat4& lighter){auto& s=g_molotov158;float origin[3];for(int k=0;k<3;++k)origin[k]=float((&s.anchor.x)[k])+s.flameOffset163[k];Mat4 flame=W7Transform(lighter,origin,s.flameRotation163);for(int r=0;r<3;++r)for(int c=0;c<3;++c)flame.m[r*4+c]*=s.flameScale163[c];return flame;}
// Native world +Z is up. Convert that direction through the same camera/body
// transform as the drawn lighter; body +Y need not be gravity under camera roll.
V3 LighterGravity175(){
 if(!g_frameCameraValid)return {0,1,0};
 const Mat4 rotation=ArmBodyToCamera(); // orthonormal rotation; transpose is its inverse
 const V3 cameraUp={g_frameCamera.right[2],g_frameCamera.up[2],g_frameCamera.forward[2]};
 const V3 up={rotation.m[0]*cameraUp.x+rotation.m[4]*cameraUp.y+rotation.m[8]*cameraUp.z,
              rotation.m[1]*cameraUp.x+rotation.m[5]*cameraUp.y+rotation.m[9]*cameraUp.z,
              rotation.m[2]*cameraUp.x+rotation.m[6]*cameraUp.y+rotation.m[10]*cameraUp.z};
 return Length(up)>.5?Unit(up):V3{0,1,0};
}
V3 LighterFlameCurve175(const Mat4& flame,const V3& local,double fraction,const V3& up){
 const V3 origin=Apply(flame,{0,0,0}),axis={flame.m[2],flame.m[6],flame.m[10]};
 const double bend=.85*(std::max)(0.0,(std::min)(1.0,fraction));
 return Add(Apply(flame,local),Scale(Sub(Scale(up,Length(axis)),axis),local.z*bend));
}
V3 LighterFlamePoint167(const Mat4& flame,const V3& local,double fraction){return LighterFlameCurve175(flame,local,fraction,LighterGravity175());}
V3 LighterFlameWorld175(const Mat4& flame,const V3& local,double fraction){return LighterFlameCurve175(flame,local,fraction,{0,0,1});}
// Bounded read-only timeline: 16 bottles, 60 seconds each, <=10 samples/sec.
// Entity disappearance is recorded as such, never called a verified impact/fire.
void TickMolotovProbe166(){
 static fc3throw::Prepared143 target;static UINT component=0;static unsigned bottles=0,rows=0;static ULONGLONG began=0,next=0,launchReceipt=0;const auto now=GetTickCount64();
 if(g_throw144.kind==2&&g_throw144.prepared.projectile&&g_throw144.prepared.serial!=target.serial&&bottles<16){target=g_throw144.prepared;component=0;began=now;next=0;launchReceipt=0;++bottles;Log("[MOLOTOV166 BEGIN] item=%016llX serial=%u epoch=%u",target.projectile,target.serial,target.epoch);for(int i=0;i<16;++i)Log("[MOLOTOV166 CAL] item=%016llX %ls=%.9g",target.projectile,kLighterKeys163[i],double(*LighterValue163(i)));}
 if(!target.projectile||now<next||now<began||now-began>60000||(launchReceipt&&now>=launchReceipt&&now-launchReceipt>10000)||rows>=3200)return;next=now+100;++rows;
 const auto native=fc3throw::InspectMolotov166(target,component);if(native.componentAddress)component=native.componentAddress;
 const bool current=g_throw144.prepared.projectile==target.projectile;const auto input=ThrowableControls361();Mat4 lighter={};V3 tip={},rag={};double distance=-1;
 if(current&&LighterBody158(lighter)){const Mat4 flame=LighterFlame163(lighter);for(int i=0;i<=8;++i){const V3 point=LighterFlamePoint167(flame,{0,0,.027*(std::max)(.1f,g_worldScale)*i/8.},i/8.);V3 nearest={};double d=0;if(RagContact159(point,nearest,d)&&(distance<0||d<distance)){distance=d;tip=point;rag=nearest;}}}
 ULONGLONG launch=0;const bool launched=fc3throw::ReleaseCompleted157(target,launch);if(launched)launchReceipt=launch;
 Log("[MOLOTOV166 SAMPLE] item=%016llX t=%llu frame=%ld entity=%d component=%d active=%d owned=%d releasing=%d launch=%llu Rgrip=%.3f Lgrip=%.3f LT=%.3f lighter=%d flame=%d contact_mm=%.2f dwell=%llu request=%d positionValid=%d position=%.5f/%.5f/%.5f tipBody=%.5f/%.5f/%.5f ragBody=%.5f/%.5f/%.5f effectMs=%llu nativeFX=%.5f/%.5f/%.5f targetFX=%.5f/%.5f/%.5f uploads=%u spawns=%u blockedTotal=%u armedTotal=%u",target.projectile,now-began,g_frame,native.entity,native.component,native.lit,current&&g_throw144.owned,current&&g_throw144.releasing,launchReceipt,input.rightGrip,input.leftGrip,input.leftTrigger,g_molotov158.held,g_molotov158.flame,distance<0?-1:distance*1000,current&&g_molotov158.contact&&now>=g_molotov158.contact?now-g_molotov158.contact:0,current&&g_molotov158.requested,native.positionValid,native.position[0],native.position[1],native.position[2],tip.x,tip.y,tip.z,rag.x,rag.y,rag.z,native.effectMs,native.nativeFlame[0],native.nativeFlame[1],native.nativeFlame[2],native.ragTarget[0],native.ragTarget[1],native.ragTarget[2],native.updates,native.spawns,native.blocked,native.armed);
 // Same bounded 10 Hz timeline as SAMPLE. Explicit receipt age prevents treating
 // a previously uploaded effect as this frame's final rendered particle position.
 if(native.effectMs){
  Log("[MOLOTOV169 EFFECT] item=%016llX serial=%u epoch=%u frame=%ld nowMs=%llu uploadMs=%llu publishedMs=%llu spawn=%d source=%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g applied=%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g/%.9g",
   target.projectile,target.serial,target.epoch,g_frame,GetTickCount64(),native.effectMs,native.effectPublishedMs169,native.effectSpawn169,
   native.effectSource169[0],native.effectSource169[1],native.effectSource169[2],native.effectSource169[3],native.effectSource169[4],native.effectSource169[5],native.effectSource169[6],native.effectSource169[7],native.effectSource169[8],native.effectSource169[9],native.effectSource169[10],native.effectSource169[11],native.effectSource169[12],native.effectSource169[13],native.effectSource169[14],native.effectSource169[15],
   native.effectApplied169[0],native.effectApplied169[1],native.effectApplied169[2],native.effectApplied169[3],native.effectApplied169[4],native.effectApplied169[5],native.effectApplied169[6],native.effectApplied169[7],native.effectApplied169[8],native.effectApplied169[9],native.effectApplied169[10],native.effectApplied169[11],native.effectApplied169[12],native.effectApplied169[13],native.effectApplied169[14],native.effectApplied169[15]);
 }
 // Child transforms are sampled BEFORE native composition, never claimed as
 // this frame's final particle positions. Exact held-item scope, bounded 4x10Hz.
 if(current&&g_throw144.owned&&native.effectMs){const auto children=fc3throw::InspectMolotovChildren178(target);
  for(UINT n=0;n<(std::min)(children.count,4u);++n){const auto& x=children.child[n];if(!x.valid)continue;
   Log("[MOLOTOV178 CHILD] item=%016llX serial=%u epoch=%u frame=%ld uploadMs=%llu index=%u count=%u flags=%08X scale=%.9g localT=%.9g/%.9g/%.9g beforeT=%.9g/%.9g/%.9g beforeX=%.9g/%.9g/%.9g beforeY=%.9g/%.9g/%.9g beforeZ=%.9g/%.9g/%.9g localX=%.9g/%.9g/%.9g localY=%.9g/%.9g/%.9g localZ=%.9g/%.9g/%.9g",
    target.projectile,target.serial,target.epoch,g_frame,children.ms,n,children.count,x.flags,x.scale,
    x.local[12],x.local[13],x.local[14],x.before[12],x.before[13],x.before[14],x.before[0],x.before[1],x.before[2],x.before[4],x.before[5],x.before[6],x.before[8],x.before[9],x.before[10],x.local[0],x.local[1],x.local[2],x.local[4],x.local[5],x.local[6],x.local[8],x.local[9],x.local[10]);
  }
 }
 if(!native.entity){Log("[MOLOTOV166 END] item=%016llX entity unavailable/reused; not proof of collision or fire",target.projectile);began=0;target.projectile=0;}
}
bool MolotovFingers158(int side,float* bones){
 if(side!=1||!LighterActive163())return false;
 const auto& key=kThrowGrenadeClipA[kThrowGrenadeAIdx_pinGripped];const Mat4 wrist=BoneMatrix(bones,2);
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(key.leftFingers,b-3)));return true;
}
void TickMolotov158(){
 auto& s=g_molotov158;fc3pad::SetLighterHeld161(false);if(!MolotovHeld158()){s.held=s.flame=false;s.contact=0;return;}
 const auto c=ThrowableControls361();if(s.serial!=g_throw144.prepared.serial||s.epoch!=g_armPoseEpoch){ResetMolotov158();s.serial=g_throw144.prepared.serial;s.epoch=g_armPoseEpoch;}
 if(!c.active||!g_handLeftValid||fc3pad::NativeMenuOpen()||g_pistolMenu.open){s.held=s.flame=false;s.contact=0;return;}
 const auto now=GetTickCount64();const bool wasHeld=s.held;s.held=c.leftGrip>(wasHeld?.3f:.7f)&&LoadLighter158();
 fc3pad::SetLighterHeld161(s.held);
 const bool trigger=c.leftTrigger>(s.previousTrigger?.2f:.6f);
 const bool wasFlame=s.flame;
 if(s.held&&(trigger!=s.previousTrigger||!wasHeld)){s.fromLid159=s.lid;s.openedAt=now;Log("[LIGHTER159] LT %s; lid target %s",trigger?"HELD":"RELEASED",trigger?"OPEN":"CLOSED");}
 const double elapsed=s.openedAt&&now>=s.openedAt?double(now-s.openedAt):0;
 s.lid=s.held?float((std::max)(0.0,(std::min)(1.0,s.fromLid159+(trigger?elapsed/180.0:-elapsed/140.0)))):0;
 s.flame=s.held&&trigger&&s.lid>=1;s.stage159=s.flame?3:s.lid>0?(trigger?1:2):0;
 if(s.flame&&!wasFlame){fc3xr::Buzz(1);Log("[LIGHTER159] lid OPEN; LT held; flame ON");}
 if(wasFlame&&!s.flame)Log("[LIGHTER159] flame OFF");
 s.previousTrigger=trigger;if(!s.held){s.openedAt=0;s.fromLid159=0;s.lid=0;}
 const bool wasLit=s.lit;s.lit=fc3throw::MolotovLit158(g_throw144.prepared);
 if(s.lit&&!wasLit){s.requested=false;fc3xr::Buzz(2);Log("[MOLOTOV158] native ignition acknowledged, exact projectile %016llX",g_throw144.prepared.projectile);}
 if(s.lit||s.requested){s.contact=0;return;}
 Mat4 lighter={};if(!s.held||!LighterBody158(lighter)){s.contact=0;s.contactFeedback159=false;return;}
 const Mat4 flameFrame=LighterFlame163(lighter);V3 flame=Apply(flameFrame,{0,0,0}),nearest={};double distance=1e9;const double scale=(std::max)(.1f,g_worldScale);
 // Use the actual calibrated flame volume, not only the lighter's burner.
 for(int i=0;i<=8;++i){const V3 point=LighterFlamePoint167(flameFrame,{0,0,.027*scale*i/8.},i/8.);V3 rag={};double d=0;if(RagContact159(point,rag,d)&&d<distance){distance=d;nearest=rag;flame=point;}}
 if(distance==1e9){s.contact=0;s.contactFeedback159=false;return;}
 const bool touching=s.flame&&distance<=g_molotovRag177.radius*scale;
 if(s.guides159){float p[3]={float(nearest.x),float(nearest.y),float(nearest.z)},h[3]={float(flame.x),float(flame.y),float(flame.z)};fc3xr::SetGrabMarkers(p,h,touching,g_molotovRag177.radius,float(.008));}
 if(!touching){s.contact=0;s.contactFeedback159=false;return;}
 if(!s.contact||now<s.contact){s.contact=now;return;}
 if(now-s.contact>=100&&!s.contactFeedback159){s.contactFeedback159=true;fc3xr::Buzz(1);Log("[LIGHTER159] flame entered rag capsule; distance %.1f mm; manual ignition %d",distance*1000,s.manualIgnition159?1:0);}
 if(s.contactFeedback159&&s.manualIgnition159&&fc3throw::IgniteMolotov158(g_throw144.prepared)){s.requested=true;s.contact=0;Log("[MOLOTOV158] flame-to-rag contact requests native ignition once");}

}
bool MolotovNote158(){
 if(!MolotovHeld158())return false;
 const auto& s=g_molotov158;
 if(s.lit){fc3xr::SetTestNote(L"MOLOTOV LIT\nRelease RIGHT GRIP to throw.\nThe bottle stays lit when you close the lighter.");return true;}
 if(s.mesh.empty()&&s.loaded)fc3xr::SetTestNote(L"CHROME LIGHTER FILES MISSING");
 else if(!s.held)fc3xr::SetTestNote(L"CHROME LIGHTER TEST\nKeep RIGHT GRIP on the bottle.\nHold LEFT GRIP to hold the lighter.\nHold LEFT TRIGGER to OPEN and LIGHT.\nRelease LEFT TRIGGER to CLOSE and extinguish.\nBottle starts UNLIT. Touch the rag with the lighter flame.");
 else if(s.stage159==0)fc3xr::SetTestNote(L"LIGHTER CLOSED\nHold LEFT GRIP and hold LEFT TRIGGER to OPEN and LIGHT.");
 else if(s.stage159==1)fc3xr::SetTestNote(L"OPENING LIGHTER LID");
 else if(s.stage159==2)fc3xr::SetTestNote(L"CLOSING LIGHTER - FLAME OFF\nHold LEFT TRIGGER to open and light again.");
 else if(s.contactFeedback159)fc3xr::SetTestNote(L"LIGHTER ON - RAG CONTACT\nKeep the flame touching the rag until ignition is confirmed.");
 else fc3xr::SetTestNote(L"LIGHTER FLAME ON\nRelease LEFT GRIP to stow.\nRelease LEFT TRIGGER to CLOSE and extinguish.\nTouch the flame to the rag to light the bottle.");
 return true;
}
ID3D11ShaderResourceView* g_lighterTex159[3]={};ID3D11SamplerState* g_lighterSampler159=nullptr;
ID3D11Texture2D* g_lighterDepthTexture160=nullptr;ID3D11DepthStencilView* g_lighterDepth160=nullptr;ID3D11DepthStencilState* g_lighterDepthState160=nullptr;UINT g_lighterWidth160=0,g_lighterHeight160=0;
bool LighterDepth160(ID3D11Device* device,UINT width,UINT height){
 if(g_lighterDepth160&&g_lighterWidth160==width&&g_lighterHeight160==height)return true;
 SafeRelease(g_lighterDepth160);SafeRelease(g_lighterDepthTexture160);D3D11_TEXTURE2D_DESC d={};d.Width=width;d.Height=height;d.MipLevels=d.ArraySize=1;d.Format=DXGI_FORMAT_D32_FLOAT;d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_DEPTH_STENCIL;
 const bool own=g_own;g_own=true;const bool ok=SUCCEEDED(device->CreateTexture2D(&d,nullptr,&g_lighterDepthTexture160))&&SUCCEEDED(device->CreateDepthStencilView(g_lighterDepthTexture160,nullptr,&g_lighterDepth160));g_own=own;
 if(ok){g_lighterWidth160=width;g_lighterHeight160=height;}return ok;
}
struct LighterScene161 {ID3D11Texture2D* texture[2]={};ID3D11DepthStencilView* view[2]={};ID3D11DepthStencilState* state=nullptr;LONG frame=-1;UINT epoch=0;D3D11_VIEWPORT viewport={};}g_lighterScene161;
void ClearLighterScene161(){for(int i=0;i<2;++i){SafeRelease(g_lighterScene161.view[i]);SafeRelease(g_lighterScene161.texture[i]);}SafeRelease(g_lighterScene161.state);g_lighterScene161.frame=-1;}
bool MolotovResources158(ID3D11Device*);
void CaptureLighterDepth161(ID3D11DeviceContext* c,ID3D11DepthStencilView* left,ID3D11DepthStencilView* right){
 if(!MolotovVisual177()||!left||!right)return;
 ID3D11DepthStencilState* state=nullptr;UINT ref=0;c->OMGetDepthStencilState(&state,&ref);D3D11_DEPTH_STENCIL_DESC sd={};if(state)state->GetDesc(&sd);else{sd.DepthEnable=true;sd.DepthFunc=D3D11_COMPARISON_LESS;}
 SafeRelease(state);if(!sd.DepthEnable||(sd.DepthFunc!=D3D11_COMPARISON_LESS&&sd.DepthFunc!=D3D11_COMPARISON_LESS_EQUAL&&sd.DepthFunc!=D3D11_COMPARISON_GREATER&&sd.DepthFunc!=D3D11_COMPARISON_GREATER_EQUAL))return;
 D3D11_VIEWPORT vp={};UINT n=1;c->RSGetViewports(&n,&vp);if(!n||vp.TopLeftX!=0||vp.TopLeftY!=0)return;
 ID3D11Device* device=nullptr;c->GetDevice(&device);if(!MolotovResources158(device)){SafeRelease(device);return;}auto& out=g_lighterScene161;out.frame=-1;bool ok=true;
 for(int eye=0;eye<2&&ok;++eye){ID3D11Resource* resource=nullptr;(eye?right:left)->GetResource(&resource);ID3D11Texture2D* texture=nullptr;resource->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&texture);SafeRelease(resource);if(!texture){ok=false;break;}
 D3D11_TEXTURE2D_DESC d={};texture->GetDesc(&d);D3D11_DEPTH_STENCIL_VIEW_DESC vd={};(eye?right:left)->GetDesc(&vd);
 if(d.SampleDesc.Count!=1||d.ArraySize!=1||d.MipLevels!=1||d.Width!=UINT(vp.Width)||d.Height!=UINT(vp.Height)){SafeRelease(texture);ok=false;break;}
 D3D11_TEXTURE2D_DESC prior={};if(out.texture[eye])out.texture[eye]->GetDesc(&prior);
 if(!out.texture[eye]||prior.Width!=d.Width||prior.Height!=d.Height||prior.Format!=d.Format){SafeRelease(out.view[eye]);SafeRelease(out.texture[eye]);d.BindFlags=D3D11_BIND_DEPTH_STENCIL;d.Usage=D3D11_USAGE_DEFAULT;d.CPUAccessFlags=d.MiscFlags=0;ok=SUCCEEDED(device->CreateTexture2D(&d,nullptr,&out.texture[eye]));vd.Flags=0;if(ok)ok=SUCCEEDED(device->CreateDepthStencilView(out.texture[eye],&vd,&out.view[eye]));}
 if(ok)c->CopyResource(out.texture[eye],texture);SafeRelease(texture);}
 sd.StencilEnable=false;sd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;SafeRelease(out.state);if(ok)ok=SUCCEEDED(device->CreateDepthStencilState(&sd,&out.state));SafeRelease(device);
 if(ok){out.frame=g_frame;out.epoch=g_armPoseEpoch;out.viewport=vp;static unsigned reports=0;if(reports++<8)Log("[LIGHTER161] native arm depth captured: %ux%u compare %u frame %ld",UINT(vp.Width),UINT(vp.Height),UINT(sd.DepthFunc),g_frame);}
}
ID3D11Device* g_molotovDevice158=nullptr;
ID3D11VertexShader* g_molotovVs158=nullptr;ID3D11PixelShader* g_molotovPs158=nullptr;ID3D11InputLayout* g_molotovLayout158=nullptr;ID3D11Buffer* g_molotovVb158=nullptr;
void ClearMolotov158(){ClearLighterScene161();SafeRelease(g_lighterDepth160);SafeRelease(g_lighterDepthTexture160);SafeRelease(g_lighterDepthState160);g_lighterWidth160=g_lighterHeight160=0;for(auto*& texture:g_lighterTex159)SafeRelease(texture);SafeRelease(g_lighterSampler159);g_molotovDevice158=nullptr;SafeRelease(g_molotovVs158);SafeRelease(g_molotovPs158);SafeRelease(g_molotovLayout158);SafeRelease(g_molotovVb158);}
bool MolotovResources158(ID3D11Device* device){
 if(g_molotovDevice158!=device)ClearMolotov158();if(!Native1911Resources48(device))return false;if(g_molotovVs158)return true;
 const char* vertex=R"(struct V{float4 p:POSITION;float4 c:COLOR;float2 uv:TEXCOORD0;float3 n:NORMAL;float3 v:TEXCOORD1;float4 t:TANGENT;};struct O{float4 p:SV_Position;float4 c:COLOR;float2 uv:TEXCOORD0;float3 n:NORMAL;float3 v:TEXCOORD1;float4 t:TANGENT;};O main(V i){O o;o.p=i.p;o.c=i.c;o.uv=i.uv;o.n=i.n;o.v=i.v;o.t=i.t;return o;})";
 const char* pixel=R"(Texture2D diffuseMap:register(t0);Texture2D normalMap:register(t1);Texture2D maskMap:register(t2);SamplerState metalSampler:register(s0);
 float4 main(float4 p:SV_Position,float4 c:COLOR,float2 uv:TEXCOORD0,float3 normal:NORMAL,float3 view:TEXCOORD1,float4 tangent:TANGENT):SV_Target{
 if(c.a>.5)return float4(c.rgb,1);
 float3 N=normalize(normal),T=normalize(tangent.xyz-N*dot(N,tangent.xyz));float3 B=normalize(cross(N,T))*tangent.w;
 float2 xy=normalMap.Sample(metalSampler,uv).ag*2-1;N=normalize(T*xy.x+B*xy.y+N*sqrt(saturate(1-dot(xy,xy))));
 float3 V=normalize(view);if(dot(N,V)<0)N=-N;float3 L=normalize(float3(-.4,-.6,.8)),H=normalize(V+L);float mask=maskMap.Sample(metalSampler,uv).r;
 float3 base=pow(diffuseMap.Sample(metalSampler,uv).rgb,2.2);float fresnel=.55+.3*pow(1-saturate(dot(N,V)),5);
 float3 R=reflect(-V,N);float sky=saturate(R.z*.5+.5);float3 environment=lerp(float3(.035,.035,.035),float3(.16,.17,.18),sky);
 float specular=pow(saturate(dot(N,H)),lerp(64,18,mask))*.12;
 float3 result=base*(.18+.3*saturate(dot(N,L)))+environment*fresnel*.65+specular;
 return float4(pow(saturate(result),1/2.2),1);})";
 auto* vs=Native1911Compile48(vertex,"vs_5_0");auto* ps=Native1911Compile48(pixel,"ps_5_0");
 D3D11_INPUT_ELEMENT_DESC input[6]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,40,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",1,DXGI_FORMAT_R32G32B32_FLOAT,0,52,D3D11_INPUT_PER_VERTEX_DATA,0},{"TANGENT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,64,D3D11_INPUT_PER_VERTEX_DATA,0}};
 D3D11_BUFFER_DESC vb={};vb.ByteWidth=8190*80;vb.BindFlags=D3D11_BIND_VERTEX_BUFFER;
 bool ok=vs&&ps&&SUCCEEDED(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&g_molotovVs158))&&SUCCEEDED(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&g_molotovPs158))&&SUCCEEDED(device->CreateInputLayout(input,6,vs->GetBufferPointer(),vs->GetBufferSize(),&g_molotovLayout158))&&SUCCEEDED(device->CreateBuffer(&vb,nullptr,&g_molotovVb158));
 for(int i=0;i<3&&ok;++i){const auto& t=g_molotov158.texture159[i];if(t.rgba.empty()){ok=false;break;}D3D11_TEXTURE2D_DESC d={};d.Width=t.width;d.Height=t.height;d.MipLevels=d.ArraySize=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA data={t.rgba.data(),t.width*4,0};ID3D11Texture2D* texture=nullptr;ok=SUCCEEDED(device->CreateTexture2D(&d,&data,&texture))&&SUCCEEDED(device->CreateShaderResourceView(texture,nullptr,&g_lighterTex159[i]));SafeRelease(texture);}
 D3D11_SAMPLER_DESC sampler={};sampler.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sampler.MaxLOD=D3D11_FLOAT32_MAX;ok=ok&&SUCCEEDED(device->CreateSamplerState(&sampler,&g_lighterSampler159));
 D3D11_DEPTH_STENCIL_DESC depth160={};depth160.DepthEnable=true;depth160.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;depth160.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;ok=ok&&SUCCEEDED(device->CreateDepthStencilState(&depth160,&g_lighterDepthState160));
 SafeRelease(vs);SafeRelease(ps);if(!ok)ClearMolotov158();else g_molotovDevice158=device;return ok;
}
void DrawMolotovMesh158(ID3D11DeviceContext* c,ID3D11Texture2D* texture,const float* vertices,UINT vertexCount,int sceneEye=-1){
 if(!c||!texture||!vertices||!vertexCount||vertexCount>8190||vertexCount%3)return;ID3D11Device* device=nullptr;c->GetDevice(&device);if(!MolotovResources158(device)){SafeRelease(device);return;}ID3D11RenderTargetView* target=nullptr;if(FAILED(device->CreateRenderTargetView(texture,nullptr,&target))){SafeRelease(device);return;}D3D11_TEXTURE2D_DESC td={};texture->GetDesc(&td);if(!LighterDepth160(device,td.Width,td.Height)){SafeRelease(device);SafeRelease(target);return;}SafeRelease(device);
 const bool own=g_own;g_own=true;
 ID3D11RenderTargetView* rt[8]={};ID3D11DepthStencilView* depth=nullptr;c->OMGetRenderTargets(8,rt,&depth);
 ID3D11BlendState* blend=nullptr;float factor[4];UINT mask=0;c->OMGetBlendState(&blend,factor,&mask);ID3D11DepthStencilState* ds=nullptr;UINT stencil=0;c->OMGetDepthStencilState(&ds,&stencil);ID3D11RasterizerState* rs=nullptr;c->RSGetState(&rs);D3D11_VIEWPORT viewport[16];UINT count=16;c->RSGetViewports(&count,viewport);
 ID3D11InputLayout* layout=nullptr;ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;D3D11_PRIMITIVE_TOPOLOGY topology;c->IAGetInputLayout(&layout);c->IAGetVertexBuffers(0,1,&vb,&stride,&offset);c->IAGetPrimitiveTopology(&topology);
 #define GUIDE_SAVE_SHADER48(stage,type) type* saved##stage=nullptr;ID3D11ClassInstance* instances##stage[256]={};UINT n##stage=256;c->stage##GetShader(&saved##stage,instances##stage,&n##stage)
 GUIDE_SAVE_SHADER48(VS,ID3D11VertexShader);GUIDE_SAVE_SHADER48(PS,ID3D11PixelShader);GUIDE_SAVE_SHADER48(GS,ID3D11GeometryShader);GUIDE_SAVE_SHADER48(HS,ID3D11HullShader);GUIDE_SAVE_SHADER48(DS,ID3D11DomainShader);
 ID3D11ShaderResourceView* oldTextures159[3]={};ID3D11SamplerState* oldSampler159=nullptr;c->PSGetShaderResources(0,3,oldTextures159);c->PSGetSamplers(0,1,&oldSampler159);c->PSSetShaderResources(0,3,g_lighterTex159);c->PSSetSamplers(0,1,&g_lighterSampler159);
 c->VSSetShader(g_molotovVs158,nullptr,0);c->PSSetShader(g_molotovPs158,nullptr,0);c->GSSetShader(nullptr,nullptr,0);c->HSSetShader(nullptr,nullptr,0);c->DSSetShader(nullptr,nullptr,0);c->IASetInputLayout(g_molotovLayout158);UINT size=80,zero=0;c->IASetVertexBuffers(0,1,&g_molotovVb158,&size,&zero);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);if(sceneEye<0)c->ClearDepthStencilView(g_lighterDepth160,D3D11_CLEAR_DEPTH,1,0);c->OMSetRenderTargets(1,&target,sceneEye<0?g_lighterDepth160:g_lighterScene161.view[sceneEye]);c->OMSetBlendState(nullptr,nullptr,~0u);c->OMSetDepthStencilState(sceneEye<0?g_lighterDepthState160:g_lighterScene161.state,0);c->RSSetState(g_nativeGuideRs48);
 D3D11_VIEWPORT vp={0,0,float(td.Width),float(td.Height),0,1};if(sceneEye>=0)vp=g_lighterScene161.viewport;c->RSSetViewports(1,&vp);D3D11_BOX upload={0,0,0,vertexCount*80,1,1};c->UpdateSubresource(g_molotovVb158,0,&upload,vertices,0,0);if(g_oDraw)g_oDraw(c,vertexCount,0);else c->Draw(vertexCount,0);
 #define GUIDE_RESTORE_SHADER48(stage) c->stage##SetShader(saved##stage,instances##stage,n##stage);SafeRelease(saved##stage);for(UINT i=0;i<n##stage;++i)SafeRelease(instances##stage[i])
 GUIDE_RESTORE_SHADER48(VS);GUIDE_RESTORE_SHADER48(PS);GUIDE_RESTORE_SHADER48(GS);GUIDE_RESTORE_SHADER48(HS);GUIDE_RESTORE_SHADER48(DS);
 #undef GUIDE_SAVE_SHADER48
 #undef GUIDE_RESTORE_SHADER48
 c->PSSetShaderResources(0,3,oldTextures159);c->PSSetSamplers(0,1,&oldSampler159);for(auto*& texture:oldTextures159)SafeRelease(texture);SafeRelease(oldSampler159);
 c->IASetInputLayout(layout);c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->IASetPrimitiveTopology(topology);c->RSSetViewports(count,viewport);c->RSSetState(rs);c->OMSetBlendState(blend,factor,mask);c->OMSetDepthStencilState(ds,stencil);c->OMSetRenderTargets(8,rt,depth);
 SafeRelease(layout);SafeRelease(vb);SafeRelease(rs);SafeRelease(blend);SafeRelease(ds);SafeRelease(depth);for(auto*& r:rt)SafeRelease(r);SafeRelease(target);g_own=own;
}

void CaptureMolotovVisual158(ID3D11DeviceContext* context){
 auto& s=g_molotov158;if(!MolotovVisual177()||s.visualFrame==g_frame)return;
 CameraSummary camera={};Mat4 inverse={};if(!PistolGuideCamera(context,camera)||!Invert(HullView(camera),inverse))return;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);NativeGuideEye48 draw[2];const UINT bytes=ReadCopy(cb,draw[0].camera,sizeof(draw[0].camera));SafeRelease(cb);if(bytes!=sizeof(draw[0].camera))return;
 memcpy(draw[1].camera,draw[0].camera,sizeof(draw[0].camera));
 for(int eye=0;eye<2;++eye){draw[eye].anchor.world=HandIdentity();float stereo[64]={};if(ReadCopy(g_eyeCb[1][eye],stereo,sizeof(stereo))!=sizeof(stereo))return;for(int i=0;i<16;++i)draw[eye].eye[i]=stereo[i]+(i%5==0?1.f:0.f);s.clip[eye].clear();}
 const Mat4 bodyToWorld=Multiply(inverse,ArmBodyToCamera());
 struct Triangle {V3 p[3];float colour[3];double distance;bool material=false;LighterVertex158 source[3]={};};std::vector<Triangle> triangles;
 const V3 eye={camera.position[0],camera.position[1],camera.position[2]};
 auto triangle=[&](V3 a,V3 b,V3 c,float r,float g,float blue){const V3 centre=Scale(Add(Add(a,b),c),1.0/3),d=Sub(centre,eye);triangles.push_back({{a,b,c},{r,g,blue},Dot(d,d)});};
 Mat4 lighter={};const bool preview=LighterPreview163();const bool lighterValid=LighterActive163()&&LighterBody158(lighter)&&LoadLighter158();if(lighterValid){
  lighter=Multiply(bodyToWorld,lighter);const double angle=(1-(preview?(s.previewOpen163?1.f:0.f):s.lid))*s.closeAngle159,co=cos(angle),si=sin(angle);
  for(size_t i=0;i+2<s.mesh.size();i+=3){V3 p[3];for(int v=0;v<3;++v){const auto& x=s.mesh[i+v];p[v]={x.xyz[0],x.xyz[1],x.xyz[2]};if(x.flags&2){const V3 q=Sub(p[v],s.pivot);p[v]=Add(s.pivot,{co*q.x+si*q.z,q.y,-si*q.x+co*q.z});}p[v]=Apply(lighter,p[v]);}
   triangle(p[0],p[1],p[2],1,1,1);auto& t=triangles.back();t.material=true;
   for(int v=0;v<3;++v){t.source[v]=s.mesh[i+v];auto rotate=[&](V3 n){if(t.source[v].flags&2)n={co*n.x+si*n.z,n.y,-si*n.x+co*n.z};return Unit(Sub(Apply(lighter,n),Apply(lighter,{0,0,0})));};
    const auto& raw=t.source[v];const V3 n=rotate({raw.normal[0],raw.normal[1],raw.normal[2]}),tan=rotate({raw.tangent[0],raw.tangent[1],raw.tangent[2]});for(int k=0;k<3;++k){t.source[v].normal[k]=float((&n.x)[k]);t.source[v].tangent[k]=float((&tan.x)[k]);}}

  }
 }
 if(lighterValid&&(preview?s.previewOpen163:s.flame)){
  const Mat4 flame=LighterFlame163(lighter);
  const double height=.027*(std::max)(.1f,g_worldScale),flicker=1+.09*sin(GetTickCount64()*.027);
  for(int plane=0;plane<2;++plane)for(int layer=0;layer<3;++layer){const double h=height*flicker*(1-layer*.22),w=height*(.15-layer*.035);
   for(int segment=0;segment<12;++segment){auto edge=[&](double t,double side){const double radius=w*pow(sin(3.141592653589793*t),.7)*(1-.55*t);V3 p={0,0,h*t};(&p.x)[plane]=side*radius+.001*sin(t*4+GetTickCount64()*.006)*t;return LighterFlameWorld175(flame,p,t);};const double t0=segment/12.,t1=(segment+1)/12.;const V3 a=edge(t0,-1),b=edge(t0,1),c=edge(t1,-1),d=edge(t1,1);const float green=layer==0?.28f:layer==1?.7f:.95f,blue=layer==2?.65f:.03f;triangle(a,b,c,1,green,blue);triangle(b,d,c,1,green,blue);}
  }
 }
 if(MolotovRagGuide177()){
  Mat4 frame={};if(ThrowableFrame144(camera,frame)){
   const Mat4 worldFrame=Multiply(inverse,frame);const V3 a=MolotovRagPoint177(worldFrame,false),b=MolotovRagOffset177(worldFrame,MolotovRagSample177(worldFrame,false),g_molotovRag177.contact),f=MolotovRagPoint177(worldFrame,true);
   const double radius=g_molotovRag177.radius*(std::max)(.1f,g_worldScale),width=.0008;
   auto line=[&](V3 x,V3 y,float red,float green,float blue){V3 n=Unit(Cross(Sub(y,x),Sub(eye,x)));if(Length(n)<.1)n={1,0,0};n=Scale(n,width);triangle(Add(x,n),Sub(x,n),Add(y,n),red,green,blue);triangle(Sub(x,n),Sub(y,n),Add(y,n),red,green,blue);};
   // Three intersecting rings at each capsule endpoint show its true radius.
   for(const V3 centre:{a,b})for(int plane=0;plane<3;++plane)for(int i=0;i<24;++i){V3 x=centre,y=centre;const double t=i*6.283185307/24,u=(i+1)*6.283185307/24;(&x.x)[(plane+1)%3]+=radius*cos(t);(&x.x)[(plane+2)%3]+=radius*sin(t);(&y.x)[(plane+1)%3]+=radius*cos(u);(&y.x)[(plane+2)%3]+=radius*sin(u);line(x,y,.1f,1.f,.2f);}line(a,b,.1f,1.f,.2f);
   for(int axis=0;axis<3;++axis){V3 x=f,y=f;(&x.x)[axis]-=.012;(&y.x)[axis]+=.012;line(x,y,1.f,.45f,.05f);}
  }
 }
 // Native Molotov owns its own flame. Do not add a second decorative rag flame.
 std::sort(triangles.begin(),triangles.end(),[](const Triangle& a,const Triangle& b){return a.distance>b.distance;});
 for(int eyeIndex=0;eyeIndex<2;++eyeIndex)for(const auto& t:triangles){float vertex[3][20]={};bool valid=true;for(int v=0;v<3;++v){valid=valid&&Native1911Clip48(draw[eyeIndex],t.p[v],vertex[v]);for(int k=0;k<3;++k)vertex[v][4+k]=t.colour[k];vertex[v][7]=t.material?0.f:1.f;
  if(t.material){const auto& raw=t.source[v];vertex[v][8]=raw.uv[0];vertex[v][9]=raw.uv[1];for(int k=0;k<3;++k){vertex[v][10+k]=raw.normal[k];vertex[v][13+k]=float((&eye.x)[k]-(&t.p[v].x)[k]);}for(int k=0;k<4;++k)vertex[v][16+k]=raw.tangent[k];}}
  if(valid)for(const auto& v:vertex)s.clip[eyeIndex].insert(s.clip[eyeIndex].end(),v,v+20);}

 s.visualFrame=g_frame;s.visualEpoch=g_armPoseEpoch;
}
void RenderMolotov158(ID3D11Texture2D* left,ID3D11Texture2D* right){
 const auto& s=g_molotov158;if(!MolotovVisual177()||s.visualFrame!=g_frame||s.visualEpoch!=g_armPoseEpoch||(g_pistolMenu.open&&!LighterPreview163()&&!MolotovRagGuide177())||fc3pad::NativeMenuOpen()||!left||!right)return;
 if(g_lighterScene161.frame!=g_frame||g_lighterScene161.epoch!=g_armPoseEpoch){static unsigned missed=0;if(missed++<8)Log("[LIGHTER161] waiting for same-frame native arm depth; no through-hand overlay");return;}
 for(int eye=0;eye<2;++eye){D3D11_TEXTURE2D_DESC target={};(eye?right:left)->GetDesc(&target);if(target.Width!=UINT(g_lighterScene161.viewport.Width)||target.Height!=UINT(g_lighterScene161.viewport.Height))continue;if(!s.clip[eye].empty())DrawMolotovMesh158(g_ctx,eye?right:left,s.clip[eye].data(),UINT(s.clip[eye].size()/20),eye);}
}
