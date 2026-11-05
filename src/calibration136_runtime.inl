// Dedicated profile document: never replace or rewrite the game's cached INI.
std::vector<std::pair<std::wstring,std::wstring>> g_calibDocument136;
std::wstring CalibrationFile136(const wchar_t* base){return std::wstring(base?base:L"")+L".profiles136";}
std::wstring CalibrationTrim136(std::wstring s){const auto first=s.find_first_not_of(L" \t\r\n");if(first==std::wstring::npos)return {};const auto last=s.find_last_not_of(L" \t\r\n");return s.substr(first,last-first+1);}
void CalibrationReadDocument136(const wchar_t* path){g_calibDocument136.clear();const auto file=CalibrationFile136(path);HANDLE h=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)return;const DWORD length=GetFileSize(h,nullptr);if(length==INVALID_FILE_SIZE||length>65536){CloseHandle(h);return;}std::string bytes(length,'\0');DWORD read=0;const bool ok=ReadFile(h,length?&bytes[0]:nullptr,length,&read,nullptr)!=0&&read==length;CloseHandle(h);if(!ok||!length)return;const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),int(length),nullptr,0);if(count<=0)return;std::wstring text(count,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),int(length),&text[0],count);std::wstring section;size_t at=0;while(at<text.size()){const size_t end=text.find(L'\n',at);const auto line=CalibrationTrim136(text.substr(at,end==std::wstring::npos?text.size()-at:end-at));at=end==std::wstring::npos?text.size():end+1;if(line.empty()||line[0]==L';')continue;if(line[0]==L'['&&line.back()==L']'){section=line.substr(1,line.size()-2);continue;}const auto split=line.find(L'=');if(split==std::wstring::npos||section.empty())continue;const auto key=section+L"\n"+CalibrationTrim136(line.substr(0,split));const auto value=CalibrationTrim136(line.substr(split+1));auto found=std::find_if(g_calibDocument136.begin(),g_calibDocument136.end(),[&](const std::pair<std::wstring,std::wstring>& p){return p.first==key;});if(found==g_calibDocument136.end())g_calibDocument136.push_back({key,value});else found->second=value;}}
DWORD CalibrationString136(const wchar_t* section,const wchar_t* key,const wchar_t* fallback,wchar_t* out,DWORD size,const wchar_t*){const auto full=std::wstring(section)+L"\n"+key;const wchar_t* value=fallback;for(const auto& entry:g_calibDocument136)if(entry.first==full){value=entry.second.c_str();break;}if(!size)return 0;wcsncpy_s(out,size,value,_TRUNCATE);return DWORD(wcslen(out));}
UINT CalibrationInt136(const wchar_t* section,const wchar_t* key,INT fallback,const wchar_t* path){wchar_t text[64],initial[64];swprintf_s(initial,L"%d",fallback);CalibrationString136(section,key,initial,text,64,path);wchar_t* end=nullptr;const long result=wcstol(text,&end,10);return end==text||*end?UINT(fallback):UINT(result);}
// Body calibration is independent of inventory and never loads obsolete weapon trims.
float CalibrationRead136(const wchar_t* path,const wchar_t* section,const wchar_t* key,float initial,float low,float high){wchar_t text[64],fallback[64];swprintf_s(fallback,L"%.9g",initial);CalibrationString136(section,key,fallback,text,64,path);wchar_t* end=nullptr;double value=wcstod(text,&end);if(end==text||*end||!std::isfinite(value))return initial;return float((std::max)(double(low),(std::min)(double(high),value)));}
void CalibrationSection136(int slot,wchar_t* section){swprintf_s(section,64,L"Calibration136_Profile%d",slot+1);}
void LoadCalibration136(const wchar_t* path){
 CalibrationReadDocument136(path);
 g_calibPreview136=false;g_calibSlot136=(std::max)(0,(std::min)(2,int(CalibrationInt136(L"Calibration136",L"ActiveProfile",0,path))));
 for(int slot=0;slot<3;++slot){auto& p=g_calibProfiles136[slot];p={};wchar_t section[64],name[48],fallbackName[48];CalibrationSection136(slot,section);swprintf_s(fallbackName,L"Player %d",slot+1);CalibrationString136(section,L"Name",fallbackName,name,48,path);WideCharToMultiByte(CP_UTF8,0,name,-1,p.name,48,nullptr,nullptr);p.name[47]=0;p.valid=CalibrationInt136(section,L"Valid",0,path)!=0&&CalibrationInt136(L"Calibration136",L"BodyOnly138",0,path)==1;
  const wchar_t* shoulder[]={L"ShoulderHalfWidth",L"ShoulderHeight",L"ShoulderDepth"};for(int a=0;a<3;++a)p.shoulder[a]=CalibrationRead136(path,section,shoulder[a],p.shoulder[a],a?-.65f:.12f,a?.25f:.38f);p.reach=CalibrationRead136(path,section,L"ArmReach",.60f,.30f,.90f);p.upperLimit=CalibrationRead136(path,section,L"UpperArmStretchLimit",2.f,1,2.5f);p.forwardFit=CalibrationInt136(section,L"ForwardFit139",0,path)==1;p.yaw=CalibrationRead136(path,section,L"BodyYaw139",0,-3.142f,3.142f);p.reachDelta[0]=CalibrationRead136(path,section,L"ArmReachDelta0",0,-.08f,.08f);p.reachDelta[1]=CalibrationRead136(path,section,L"ArmReachDelta1",0,-.08f,.08f);

 }
 g_calibEnabled136=CalibrationInt136(L"Calibration136",L"Enabled",0,path)!=0&&g_calibProfiles136[g_calibSlot136].valid;
}
DWORD g_calibSaveError136=0;const char* g_calibSaveStage136="";
bool SaveCalibration136(const wchar_t* path,const CalibrationProfile136* profiles,int active,bool enabled){
 g_calibSaveError136=0;g_calibSaveStage136="prepare";if(!path||active<0||active>2)return false;const auto file=CalibrationFile136(path),tmp=file+L".tmp";if(tmp.size()>=MAX_PATH)return false;
 std::wstring document=L"; Far Cry 3 body calibration, format139\n",section;
 auto text=[&](const wchar_t* group,const wchar_t* key,const wchar_t* value){if(section!=group){section=group;document+=L"["+section+L"]\n";}document+=std::wstring(key)+L"="+value+L"\n";};
 auto put=[&](const wchar_t* group,const wchar_t* key,double value){wchar_t number[64];swprintf_s(number,L"%.9g",value);text(group,key,number);};
 for(int slot=0;slot<3;++slot){const auto& p=profiles[slot];wchar_t group[64],name[64];CalibrationSection136(slot,group);if(!MultiByteToWideChar(CP_UTF8,0,p.name,-1,name,64))wcscpy_s(name,L"Player");for(auto& c:name){if(c==L'\r'||c==L'\n')c=L' ';if(!c)break;}text(group,L"Name",name);const wchar_t* sk[]={L"ShoulderHalfWidth",L"ShoulderHeight",L"ShoulderDepth"};for(int a=0;a<3;++a)put(group,sk[a],p.shoulder[a]);put(group,L"ArmReach",p.reach);put(group,L"UpperArmStretchLimit",p.upperLimit);put(group,L"ForwardFit139",p.forwardFit);put(group,L"BodyYaw139",p.yaw);put(group,L"ArmReachDelta0",p.reachDelta[0]);put(group,L"ArmReachDelta1",p.reachDelta[1]);
  put(group,L"Valid",p.valid);
 }
 put(L"Calibration136",L"BodyOnly138",1);put(L"Calibration136",L"ActiveProfile",active);put(L"Calibration136",L"Enabled",enabled);const int count=WideCharToMultiByte(CP_UTF8,0,document.c_str(),int(document.size()),nullptr,0,nullptr,nullptr);if(count<=0||count>65536)return false;std::string bytes(count,'\0');WideCharToMultiByte(CP_UTF8,0,document.c_str(),int(document.size()),&bytes[0],count,nullptr,nullptr);
 g_calibSaveStage136="write";HANDLE h=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE){g_calibSaveError136=GetLastError();return false;}DWORD written=0;bool ok=WriteFile(h,bytes.data(),DWORD(bytes.size()),&written,nullptr)!=0&&written==bytes.size()&&FlushFileBuffers(h);const DWORD error=ok?0:GetLastError();CloseHandle(h);if(ok){g_calibSaveStage136="replace";ok=MoveFileExW(tmp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;}if(!ok){g_calibSaveError136=error?error:GetLastError();DeleteFileW(tmp.c_str());SetLastError(g_calibSaveError136);}return ok;
}
struct CalibrationWizard136 {
 int step=-1,slot=0;bool armed=false,gripReady=false,prepared=false;ULONGLONG start=0,stableAt=0;LONG frame=-1;int samples=0;double sum[2][16]={};float first[2][16]={};V3 head={},headSum={},neutralHead={},forward={},forwardSum={},neutralForward={0,0,1},sideways[2]={};float yaw=0;char message[160]={};
}g_calibWizard136;
const char* kCalibrationSteps136[]={"1 of 4: Normal playing position","2 of 4: Arms straight out to your sides","3 of 4: Arms straight forward","4 of 4: Review your arms and save"};
void PauseCalibration136(){auto& w=g_calibWizard136;w.armed=false;w.gripReady=false;w.samples=0;w.stableAt=0;strcpy_s(w.message,w.step==3?"Review kept. Reopen Insert to save or adjust your body profile.":"Capture paused. Reopen Insert, release both grips, then squeeze both when ready.");}
void CancelCalibration136(){g_calibWizard136={};g_calibWizard136.step=-1;g_calibPreview136=false;}
void StartCalibration136(int slot,bool edit=false){g_calibDraft136=g_calibProfiles136[slot];g_calibWizard136={};g_calibWizard136.slot=slot;g_calibWizard136.step=edit?3:0;g_calibPreview136=edit;if(!edit){char name[48];strcpy_s(name,g_calibDraft136.name);g_calibDraft136={};strcpy_s(g_calibDraft136.name,name);}}
void ArmCalibration136(ULONGLONG now,bool prepared=false){auto& w=g_calibWizard136;w.armed=true;w.prepared=prepared;w.start=now;w.stableAt=0;w.samples=0;w.frame=-1;memset(w.sum,0,sizeof(w.sum));strcpy_s(w.message,prepared?"Both grips pressed. Hold steady briefly to record this pose.":"Prepare your pose: capture begins after 3 seconds.");}
Mat4 CalibrationAverage136(int side){const auto& w=g_calibWizard136;Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=w.sum[side][i]/w.samples;V3 z=Unit({m.m[2],m.m[6],m.m[10]}),up={m.m[1],m.m[5],m.m[9]};const auto basis=ScopeBasis72(z,up);for(int r=0;r<3;++r)for(int c=0;c<3;++c)m.m[r*4+c]=basis.m[r*4+c];return m;}
bool FinishCalibrationSample136(){auto& w=g_calibWizard136;auto& p=g_calibDraft136;const double scale=(std::max)(.1f,g_worldScale);const V3 head=Scale(w.headSum,1./w.samples);const Mat4 right=CalibrationAverage136(0),left=CalibrationAverage136(1);float rr[16],ll[16];for(int i=0;i<16;++i){rr[i]=float(right.m[i]);ll[i]=float(left.m[i]);}
 if(w.step==0){w.neutralHead=head;w.neutralForward=Unit({w.forwardSum.x,0,w.forwardSum.z});if(Length(w.neutralForward)<.9){strcpy_s(w.message,"Face straight ahead, then release both grips and capture again.");return false;}w.yaw=float(atan2(w.neutralForward.x,w.neutralForward.z));++w.step;return true;}
 if(fabs(head.y-w.neutralHead.y)>.18*scale){strcpy_s(w.message,"No pose recorded: keep the same sitting/standing height as step 1, then release both grips and retry.");return false;}
 const V3 nowForward=Unit({w.forwardSum.x,0,w.forwardSum.z});if(Dot(nowForward,w.neutralForward)<.966){strcpy_s(w.message,"Keep facing the same direction as step 1. Release both grips, look forward and capture again.");return false;}
 auto local=[&](V3 v){v=Scale(Sub(v,head),1/scale);return V3{cos(w.yaw)*v.x-sin(w.yaw)*v.z,v.y,sin(w.yaw)*v.x+cos(w.yaw)*v.z};};
 const V3 wrists[]={local(ControllerWrist(rr,0)),local(ControllerWrist(ll,1))};
 if(w.step==1){const V3 r=wrists[0],l=wrists[1];const double span=r.x-l.x;if(span<.7||span>2.2||fabs(r.y-l.y)>.15||fabs(r.z-l.z)>.20){strcpy_s(w.message,"No pose recorded. Face forward with both arms straight sideways at the same shoulder height, then capture again.");return false;}w.sideways[0]=r;w.sideways[1]=l;++w.step;return true;}
 if(w.step==2){CalibrationProfile136 fit=p;if(!CalibrationFit139(w.sideways,wrists,fit)){strcpy_s(w.message,"No pose recorded. Reach both arms straight FORWARD at shoulder height, hands shoulder-width apart. Release both grips and retry.");return false;}fit.yaw=w.yaw;p=fit;g_calibPreview136=true;++w.step;return true;}
 return false;
}
// Read UI-owned raw controls: gameplay remains blocked throughout the gesture.
void TickCalibrationAt136(ULONGLONG now){auto& w=g_calibWizard136;
 if(!g_pistolMenu.open){if(w.armed)PauseCalibration136();w.gripReady=false;return;}
 if(w.step>=0&&w.step<3){const auto input=fc3pad::NativeUiControls();if(!input.active||fc3pad::NativeMenuOpen())w.gripReady=false;else if(input.leftGrip<.3f&&input.rightGrip<.3f)w.gripReady=true;else if(input.leftGrip>.7f&&input.rightGrip>.7f&&w.gripReady){w.gripReady=false;if(!w.armed)ArmCalibration136(now,true);}}
 if(!w.armed)return;if(now-w.start<(w.prepared?0:3000))return;if(now-w.start>23000){PauseCalibration136();strcpy_s(w.message,"No pose recorded: capture timed out. Release both grips, get into the pose and squeeze both to retry.");return;}if(!g_inGameplay||!g_eyeViewsSet||!g_calibRawValid136[0]||!g_calibRawValid136[1]||fc3pad::NativeMenuOpen()){w.samples=0;w.stableAt=0;strcpy_s(w.message,"Waiting for headset and both controllers to be tracked.");return;}if(w.frame==g_frame)return;w.frame=g_frame;V3 head={},forward={};if(!CalibrationHead136(head,&forward))return;const double scale=(std::max)(.1f,g_worldScale);
 bool moved=w.samples&&(Length(Sub(head,w.head))>.04*scale||Dot(forward,w.forward)<.9945);for(int side=0;side<2&&w.samples;++side){V3 a={g_calibRaw136[side][3],g_calibRaw136[side][7],g_calibRaw136[side][11]},b={w.first[side][3],w.first[side][7],w.first[side][11]};double rotation=0;for(int col=0;col<3;++col){double dot=0;for(int row=0;row<3;++row)dot+=g_calibRaw136[side][row*4+col]*w.first[side][row*4+col];rotation=(std::max)(rotation,acos((std::max)(-1.,(std::min)(1.,dot))));}moved=moved||Length(Sub(a,b))>.025*scale||rotation>.10;}
 if(moved){w.samples=0;w.stableAt=0;strcpy_s(w.message,"Hold still briefly; collecting a fresh stable pose.");}if(!w.samples){memset(w.sum,0,sizeof(w.sum));w.headSum={};w.forwardSum={};w.head=head;w.forward=forward;w.stableAt=now;memcpy(w.first,g_calibRaw136,sizeof(w.first));}for(int side=0;side<2;++side)for(int i=0;i<16;++i)w.sum[side][i]+=g_calibRaw136[side][i];w.headSum=Add(w.headSum,head);w.forwardSum=Add(w.forwardSum,forward);++w.samples;
 if(w.samples>=30&&now-w.stableAt>=750){w.armed=false;const int captured=w.step;if(FinishCalibrationSample136())strcpy_s(w.message,captured==0?"Normal position captured. NEXT: raise both arms sideways to shoulder height, then squeeze both grips.":captured==1?"T-pose captured. NEXT: bring BOTH arms straight FORWARD at shoulder height, then squeeze both grips.":"Forward reach captured. LOWER YOUR ARMS. NEXT: review the fit and click SAVE BODY PROFILE.");w.samples=0;}
}
void TickCalibration136(){TickCalibrationAt136(GetTickCount64());}
ImGuiContext* g_calibContext136=nullptr;ID3D11Device* g_calibDevice136=nullptr;
bool CalibrationPixel136(int eye,V3 point,float width,float height,ImVec2& pixel){Mat4 view={};const auto& e=g_aimActive?g_eyeViewsBody[eye]:g_eyeViews[eye];for(int i=0;i<16;++i)view.m[i]=e.view[i];const V3 p=Apply(view,point);if(p.z<.035)return false;const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);const double x=projection.m[0]*p.x/p.z+projection.m[2],y=projection.m[5]*p.y/p.z+projection.m[6];if(!std::isfinite(x)||!std::isfinite(y)||fabs(x)>4||fabs(y)>4)return false;pixel={float((x+1)*width*.5),float((1-y)*height*.5)};return true;}
void CalibrationMirror136(){
 auto* draw=ImGui::GetWindowDrawList();const ImVec2 origin=ImGui::GetCursorScreenPos();const float width=(std::min)(ImGui::GetContentRegionAvail().x,460.f),height=190;ImGui::Dummy({width,height});draw->AddRectFilled(origin,{origin.x+width,origin.y+height},IM_COL32(20,25,32,230));
 V3 head={};CalibrationHead136(head);const bool top=g_calibWizard136.step==2;const float ratio=width/2.1f,yaw=g_calibWizard136.step>=0?g_calibWizard136.yaw:CalibrationCurrent136().yaw;
 const ImVec2 centre={origin.x+width*.5f,origin.y+(top?height-35:40)};
 auto local=[&](V3 p){p=Scale(Sub(p,head),1/(std::max)(.1f,g_worldScale));return V3{cos(yaw)*p.x-sin(yaw)*p.z,p.y,sin(yaw)*p.x+cos(yaw)*p.z};};
 auto screen=[&](V3 p){return ImVec2(centre.x+float(p.x)*ratio,centre.y-float(top?p.z:p.y)*ratio);};
 const bool preview=g_calibPreview136;const float half=preview?CalibrationCurrent136().shoulder[0]:.21f,sy=preview?CalibrationCurrent136().shoulder[1]:-.24f,sz=preview?CalibrationCurrent136().shoulder[2]:-.05f;
 draw->PushClipRect(origin,{origin.x+width,origin.y+height-20},true);draw->AddCircle(centre,10,IM_COL32(200,215,230,255),24,2);const ImVec2 ls=screen({-half,sy,sz}),rs=screen({half,sy,sz});draw->AddLine(ls,rs,IM_COL32(140,160,180,255),2);
 if(top){draw->AddLine({centre.x,origin.y+55},{centre.x,origin.y+22},IM_COL32(220,220,220,255),2);draw->AddLine({centre.x,origin.y+22},{centre.x-5,origin.y+30},IM_COL32(220,220,220,255),2);draw->AddLine({centre.x,origin.y+22},{centre.x+5,origin.y+30},IM_COL32(220,220,220,255),2);}
 else draw->AddLine({centre.x,centre.y+11},{centre.x,origin.y+height-22},IM_COL32(140,160,180,255),2);
 for(int side=0;side<2;++side){if(!g_calibRawValid136[side])continue;const ImVec2 wrist=screen(local(ControllerWrist(g_calibRaw136[side],side)));draw->AddLine(side?ls:rs,wrist,IM_COL32(80,220,180,255),2);draw->AddCircleFilled(wrist,5,side?IM_COL32(80,180,255,255):IM_COL32(255,180,70,255));}
 if(g_calibWizard136.step==1)for(double x:{-.75,.75})draw->AddCircle(screen({x,sy,0}),7,IM_COL32(220,220,220,255));
 if(top)for(double x:{-.21,.21}){const auto target=screen({x,sy,.55});draw->AddCircle(target,8,IM_COL32(220,220,220,255),24,2);draw->AddLine(x<0?ls:rs,target,IM_COL32(110,120,135,180),1);}
 draw->PopClipRect();draw->AddText({origin.x+8,origin.y+height-18},IM_COL32(190,205,220,255),top?"TOP VIEW: reach forward with hands apart":"FRONT VIEW: orange right wrist / blue left wrist");
}
void CalibrationFonts139(){
 auto* atlas=ImGui::GetIO().Fonts;for(auto* font:atlas->Fonts)if(!strcmp(font->GetDebugName(),"CalibrationBold139"))return;
 if(atlas->Fonts.empty())atlas->AddFontDefault();ImFontConfig config;strcpy_s(config.Name,"CalibrationBold139");char path[MAX_PATH]={};GetWindowsDirectoryA(path,MAX_PATH);strcat_s(path,"\\Fonts\\segoeuib.ttf");
 if(GetFileAttributesA(path)!=INVALID_FILE_ATTRIBUTES&&atlas->AddFontFromFileTTF(path,15.f,&config))return;
 config.RasterizerMultiply=1.6f;atlas->AddFontDefault(&config);
}
void CalibrationInstruction139(const char* text){ImFont* bold=nullptr;for(auto* font:ImGui::GetIO().Fonts->Fonts)if(!strcmp(font->GetDebugName(),"CalibrationBold139")){bold=font;break;}ImGui::PushFont(bold,15.f);ImGui::TextWrapped("%s",text);ImGui::PopFont();}
// Keep the current instruction above the diagram and advanced controls.
void CalibrationMenu136(){
 static int selected=0;static bool saveFailed=false,saved=false;static int shownStep=-2;
 if(g_calibWizard136.step>=0)ImGui::SetNextItemOpen(true,ImGuiCond_Always);
 if(!ImGui::CollapsingHeader("Body / arm calibration"))return;
 auto& w=g_calibWizard136;
 if(w.step<0){
  ImGui::TextWrapped("Set the shoulders and arm reach for your body. No weapon is required. You can sit or stand in your usual playing position.");
  const char* slots[]={"Profile 1","Profile 2","Profile 3"};ImGui::Combo("Saved profile slot",&selected,slots,3);
  ImGui::Text("Active body profile: %s%s",g_calibProfiles136[g_calibSlot136].name,g_calibEnabled136?"":" (off)");
  if(g_calibEnabled136&&g_calibProfiles136[g_calibSlot136].forwardFit)CalibrationInstruction139("Your sideways and forward-reach measurements are saved and ACTIVE. To check the fit, use Review / adjust saved body profile.");
  if(saved)ImGui::TextColored({.35f,1.f,.6f,1},"Body profile saved. It will load next time you play.");
  if(ImGui::Button("Start body calibration")){StartCalibration136(selected);saveFailed=saved=false;}
  if(g_calibProfiles136[selected].valid){
   if(ImGui::Button("Review / adjust saved body profile")){StartCalibration136(selected,true);saveFailed=saved=false;}
   if(ImGui::Button("Use selected body profile")){wchar_t path[MAX_PATH];ManualSettingsPath(path);saveFailed=!SaveCalibration136(g_menuSavePath?g_menuSavePath:path,g_calibProfiles136,selected,true);if(!saveFailed){g_calibSlot136=selected;g_calibEnabled136=true;saved=true;}}
  }
  if(g_calibEnabled136&&ImGui::Button("Turn body calibration off")){wchar_t path[MAX_PATH];ManualSettingsPath(path);saveFailed=!SaveCalibration136(g_menuSavePath?g_menuSavePath:path,g_calibProfiles136,g_calibSlot136,false);if(!saveFailed){g_calibEnabled136=false;saved=false;}}
 }
 if(w.step<0){shownStep=-2;if(saveFailed)ImGui::TextWrapped("Save failed. Your previous saved profile is unchanged.");return;}
 ImGui::Separator();ImGui::TextColored({.5f,.85f,1,1},"%s",kCalibrationSteps136[w.step]);
 if(shownStep!=w.step){ImGui::SetScrollHereY(0);shownStep=w.step;}
 ImGui::TextWrapped("Normal position  >  Arms sideways  >  Arms forward  >  Review and save");
 if(w.message[0])CalibrationInstruction139(w.message);
 if(w.step==0){
  CalibrationInstruction139("DO THIS: sit or stand normally, face forward and let both hands rest comfortably near your sides. Keep wearing the headset and holding both controllers.");
  CalibrationInstruction139("CAPTURE: release both grip buttons, then squeeze BOTH GRIPS together. Keep your head and hands still for about one second.");
  CalibrationInstruction139("AFTER: the wizard will say Normal position captured and show the arms-out step.");
 }else if(w.step==1){
  CalibrationInstruction139("DO THIS: face forward. Raise BOTH arms out to your sides at shoulder height, like the letter T. Palms face down. Keep elbows comfortably straight; do not strain.");
  CalibrationInstruction139("CAPTURE: while your arms are out, release both grip buttons, then squeeze BOTH GRIPS together. Hold that pose still for about one second.");
  CalibrationInstruction139("AFTER: wait for T-pose captured. The next step asks you to reach both arms straight FORWARD.");
 }else if(w.step==2){
  CalibrationInstruction139("DO THIS: keep facing forward. Bring BOTH arms straight out IN FRONT of you at shoulder height. Keep your hands SHOULDER-WIDTH APART, palms down. Do not put your hands together. Reach fully but comfortably; do not lock your elbows hard.");
  CalibrationInstruction139("CAPTURE: with your arms still reaching forward, RELEASE both grips, then SQUEEZE BOTH GRIPS together. Hold still for about one second.");
  CalibrationInstruction139("AFTER: wait for Forward reach captured. Then LOWER YOUR ARMS. The next page lets you check the fit and SAVE.");
 }else{
  CalibrationInstruction139("DO THIS: lower your arms, then slowly bend and extend them. Look at your wrists, elbows and where the sleeves meet the shoulders. You may close Insert to inspect your arms freely. Reopen Insert to return here and save.");
  CalibrationInstruction139("CHECK 1 - WRISTS: move your hands slowly. Each virtual wrist should stay with its controller, without a jump or drift.");
  CalibrationInstruction139("CHECK 2 - FULL REACH: extend your real arm fully but comfortably. Jason's elbow should become NEARLY STRAIGHT too. A small bend is normal; a large bend means the fit needs adjustment.");
  CalibrationInstruction139("CHECK 3 - BENDING: bring your hands toward your chest. The virtual elbows should bend smoothly, and the sleeves should stay joined at the shoulders.");
  CalibrationInstruction139("FINISH: enter a profile name and click SAVE BODY PROFILE below. The profile loads automatically next time. To discard this preview, click Cancel.");
  CalibrationInstruction139("You are DONE capturing. The adjustment panel below is only for manual corrections if the fit looks wrong. You can leave it closed and SAVE now.");
  ImGui::InputText("Body profile name",g_calibDraft136.name,sizeof(g_calibDraft136.name));
  if(ImGui::Button("SAVE BODY PROFILE")){
   const auto previous=g_calibProfiles136[w.slot];g_calibProfiles136[w.slot]=g_calibDraft136;wchar_t path[MAX_PATH];ManualSettingsPath(path);
   saveFailed=!SaveCalibration136(g_menuSavePath?g_menuSavePath:path,g_calibProfiles136,w.slot,true);
   if(saveFailed)g_calibProfiles136[w.slot]=previous;else{g_calibSlot136=w.slot;g_calibEnabled136=true;g_calibPreview136=false;w.step=-1;saved=true;}
  }
  if(saved&&w.step<0)ImGui::TextWrapped("SAVED. Body profile is active and will load next time. Close Insert to play.");
 }
 if(w.step<0)return;
 ImGui::Separator();CalibrationMirror136();
 if(w.step<3){
  if(w.armed){
   const auto now=GetTickCount64(),elapsed=now-w.start,delay=w.prepared?0ull:3000ull;
   if(elapsed<delay)ImGui::Text("Get ready: %.1f seconds",(delay-elapsed)*.001);
   else{const float progress=w.stableAt?float((std::min)(1.,(now-w.stableAt)/750.)):0;ImGui::ProgressBar(progress,{-1,0},"Hold your pose still - recording");}
   if(ImGui::Button("Stop this capture"))PauseCalibration136();
  }else CalibrationInstruction139(w.gripReady?"READY: get into the pose above, then squeeze BOTH GRIPS.":"First RELEASE BOTH GRIPS. Then get into the pose and squeeze both to capture.");

 }
 if(w.step==3&&ImGui::TreeNode("Adjust body fit by hand - only if it looks wrong")){
  auto& p=g_calibDraft136;CalibrationInstruction139("Calibration is finished. You can SAVE without opening or changing these sliders. Use them only to correct a visible fit problem. They move the estimated shoulders or change arm reach; they are not another capture step.");
  ImGui::SliderFloat("Half shoulder width",&p.shoulder[0],.12f,.38f,"%.3f m");ImGui::SliderFloat("Shoulder height below head",&p.shoulder[1],-.55f,-.06f,"%.3f m");ImGui::SliderFloat("Shoulder forward/back",&p.shoulder[2],-.4f,.15f,"%.3f m");ImGui::SliderFloat("Full arm reach",&p.reach,.3f,.9f,"%.3f m");ImGui::SliderFloat("Upper arm extension limit",&p.upperLimit,1,2.5f,"%.2fx");ImGui::TreePop();
 }
 if(saveFailed)ImGui::TextWrapped("Save failed. Your previous saved profile is unchanged. Retry Save or Cancel.");
 if(ImGui::Button("Cancel - discard body changes")){CancelCalibration136();saveFailed=saved=false;}
}
void RenderCalibration136(ID3D11Texture2D* left,ID3D11Texture2D* right){if(g_calibWizard136.step<0||!g_pistolMenu.open||!g_eyeViewsSet||!g_inGameplay||!left||!right||!g_dev||!g_ctx||fc3pad::NativeMenuOpen())return;const bool previousOwn=g_own;g_own=true;ImGuiContext* prior=ImGui::GetCurrentContext();if(g_calibContext136&&g_calibDevice136!=g_dev){ImGui::SetCurrentContext(g_calibContext136);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext(g_calibContext136);g_calibContext136=nullptr;}if(!g_calibContext136){g_calibContext136=ImGui::CreateContext();ImGui::SetCurrentContext(g_calibContext136);CalibrationFonts139();ImGui::GetIO().IniFilename=ImGui::GetIO().LogFilename=nullptr;if(!ImGui_ImplDX11_Init(g_dev,g_ctx)){ImGui::DestroyContext(g_calibContext136);g_calibContext136=nullptr;ImGui::SetCurrentContext(prior);g_own=previousOwn;return;}g_calibDevice136=g_dev;}ImGui::SetCurrentContext(g_calibContext136);ID3D11RenderTargetView* saved[8]={};ID3D11DepthStencilView* depth=nullptr;g_ctx->OMGetRenderTargets(8,saved,&depth);ID3D11Texture2D* images[]={left,right};V3 head={},forward={};CalibrationHead136(head,&forward);const double scale=(std::max)(.1f,g_worldScale);
 for(int eye=0;eye<2;++eye){D3D11_TEXTURE2D_DESC td={};images[eye]->GetDesc(&td);ID3D11RenderTargetView* target=nullptr;if(FAILED(g_dev->CreateRenderTargetView(images[eye],nullptr,&target)))continue;auto& io=ImGui::GetIO();io.DisplaySize={float(td.Width),float(td.Height)};io.DeltaTime=1.f/60;ImGui_ImplDX11_NewFrame();ImGui::NewFrame();auto* draw=ImGui::GetForegroundDrawList();auto point=[&](V3 p,ImU32 colour,float radius){ImVec2 xy;if(CalibrationPixel136(eye,p,float(td.Width),float(td.Height),xy))draw->AddCircle(xy,radius,colour,24,2);};auto line=[&](V3 a,V3 b,ImU32 colour){ImVec2 x,y;if(CalibrationPixel136(eye,a,float(td.Width),float(td.Height),x)&&CalibrationPixel136(eye,b,float(td.Width),float(td.Height),y))draw->AddLine(x,y,colour,2);};
  for(int side=0;side<2;++side){if(!g_calibRawValid136[side])continue;const ImU32 colour=side?IM_COL32(60,160,255,255):IM_COL32(255,170,60,255);const V3 wrist=ControllerWrist(g_handBody[side],side);point(wrist,colour,8);const float* pose=g_handBody[side];const V3 axis[]={V3{pose[0],pose[4],pose[8]},V3{pose[1],pose[5],pose[9]},V3{pose[2],pose[6],pose[10]}};for(int a=0;a<3;++a)line(wrist,Add(wrist,Scale(axis[a],.07*scale)),a==0?IM_COL32(255,70,70,255):a==1?IM_COL32(70,255,90,255):IM_COL32(70,140,255,255));V3 shoulder={};if(CalibrationShoulder136(side,shoulder)){point(shoulder,IM_COL32(220,220,220,255),6);Mat4 inverse={};if(Invert(ArmBodyToCamera(),inverse)){const V3 elbow=Apply(inverse,g_armElbow77[side]);const bool over=Length(Sub(wrist,shoulder))>(CalibrationCurrent136().reach+CalibrationCurrent136().reachDelta[side])*scale;const ImU32 reachColour=over?IM_COL32(255,100,60,220):IM_COL32(70,200,150,200);point(elbow,reachColour,5);line(shoulder,elbow,reachColour);line(elbow,wrist,reachColour);}}}
  ImGui::Render();g_ctx->OMSetRenderTargets(1,&target,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());SafeRelease(target);
 }g_ctx->OMSetRenderTargets(8,saved,depth);for(auto*& p:saved)SafeRelease(p);SafeRelease(depth);ImGui::SetCurrentContext(prior);g_own=previousOwn;
}
