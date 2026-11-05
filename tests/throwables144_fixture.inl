#include "molotov_rag176_fixture.inl"
#include "molotov_lighter158_fixture.inl"
#include "placeables177_fixture.inl"
__declspec(naked) void PinActivationFixture155(){
 __asm mov eax,[esp+4]
 __asm mov byte ptr [eax+234h],1
 __asm ret 4
}
UINT releaseFrame148=0;unsigned nativeCalls148=0;
void __fastcall DropReleaseFixture148(void* self,void*,UINT wrapper){
 const UINT strategy=reinterpret_cast<UINT>(self);++nativeCalls148;Check(wrapper==fc3throw::g_pending143.target.nativeWrapper149&&wrapper!=fc3throw::Word(strategy,0x38),"drop uses distinct prepared native execution wrapper");
 UINT regs[9]={strategy,0,releaseFrame148,0,strategy,0,fc3throw::Word(fc3throw::Word(strategy,0xf4),12),0,0};
 fc3throw::Observe(2,regs);fc3throw::Observe(4,regs);fc3throw::Observe(5,regs);
}
bool ThrowableLocalForFixture144(ULONGLONG& owner){owner=9;return true;}
void TestThrowables144(ID3D11Device* device,ID3D11DeviceContext* context){
 const wchar_t* defaultsPath=L"build\\stereo\\throw151-defaults.ini";DeleteFileW(defaultsPath);
 g_throw144.zeroDrop148=true;LoadThrowables144(defaultsPath);Check(!g_throw144.zeroDrop148,"unsaved settings start in actual physical throwing mode instead of silently forcing every swing to drop");
 g_throw144.zeroDrop148=true;Check(SaveThrowables144(defaultsPath),"explicit optional drop diagnostic saves");g_throw144.zeroDrop148=false;LoadThrowables144(defaultsPath);Check(g_throw144.zeroDrop148,"explicit diagnostic preference remains available");DeleteFileW(defaultsPath);g_throw144.zeroDrop148=false;
 const wchar_t* stockPath=L"build\\stereo\\unlimited147-test.ini";
 fc3throw::SetUnlimited147(false);Check(SaveThrowables144(stockPath),"unlimited stock off setting saves");fc3throw::SetUnlimited147(true);LoadThrowables144(stockPath);Check(!fc3throw::Unlimited147(),"unlimited stock off setting persists");
 fc3throw::SetUnlimited147(true);Check(SaveThrowables144(stockPath),"unlimited stock on setting saves");fc3throw::SetUnlimited147(false);LoadThrowables144(stockPath);Check(fc3throw::Unlimited147(),"unlimited stock on setting persists");
 const bool savedManualPin154=g_throw144.manualPin154;g_throw144.manualPin154=false;
 context->ClearState();g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;
 g_oCreateBuffer=[](ID3D11Device* d,const D3D11_BUFFER_DESC* desc,const D3D11_SUBRESOURCE_DATA* initial,ID3D11Buffer** out)->HRESULT{return d->CreateBuffer(desc,initial,out);};
 g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=true;g_weaponMotionGuard=true;g_flatViewActive=false;g_machete.held=false;g_pistolMenu.open=false;g_aimActive=false;g_twoHand=false;g_reload={};g_r700={};g_lever={};g_reloadAlignment=false;
 fc3aim::SetNativeActionForTest128(0);fc3aim::SetVehicleStateForTest(0);fc3pad::SetPistolCalibration(false);fc3pad::SetGameplayControls(true);fc3pad::SetDedicatedInteractions(true);fc3pad::SetSwapBX(false);fc3pad::SetBowInput(false,0);fc3pad::SetMacheteContext(false);fc3pad::SetMacheteHandling(false);
 fc3pad::Controllers input={};input.active=true;fc3pad::Publish(input);
 auto id=HandIdentity();++g_armPoseEpoch;g_weaponMechanicProfile=-1;g_weaponMechanicFrame=-100000;g_visibleWeaponDrawsLastFrame=0;
 for(int side=0;side<2;++side){for(int i=0;i<16;++i)g_handBody[side][i]=g_handRef[side][i]=float(id.m[i]);g_handBody[side][3]=side?-.2f:.2f;g_handBody[side][7]=-.2f;g_handBody[side][11]=.5f;memcpy(side?g_handLeft:g_handRight,g_handBody[side],64);}
 for(int eye=0;eye<2;++eye){for(int i=0;i<16;++i)g_eyeViews[eye].view[i]=g_eyeViewsBody[eye].view[i]=float(i%5==0);g_eyeViews[eye].tanLeft=g_eyeViews[eye].tanDown=-1;g_eyeViews[eye].tanRight=g_eyeViews[eye].tanUp=1;}
 auto* oldWeapon=g_weaponPaletteCb;auto* oldArm=g_armPaletteCb[0];auto* oldBody=g_bodyPaletteCb;g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);g_armPaletteCb[0]=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);g_bodyPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
 auto* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);auto* camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);auto* palette=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);auto* arm=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);
 ID3D11Buffer* cb[4]={globals,camera,nullptr,palette};context->VSSetConstantBuffers(0,4,cb);
 float rows[88]={};rows[0]=rows[5]=rows[10]=1;globals->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);
 CameraSummary cam={};cam.mainView=1;cam.right[0]=cam.up[1]=cam.forward[2]=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);g_frameCamera=cam;g_frameCameraValid=true;
 float native[576]={};for(int b=0;b<48;++b)StoreBone(native,b,id);palette->SetPrivateData(kCbCopyGuid,sizeof(native),native);
 TestPlaceables177(device,context);
 auto equal=[](const Mat4& a,const Mat4& b){for(int i=0;i<12;++i)if(fabs(a.m[i]-b.m[i])>3e-4)return false;return true;};
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,16384,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"throw144 native fixture allocation");const UINT base=reinterpret_cast<UINT>(page);
 const UINT strategy=base,wrapper=base+1024,ownerRef=base+2048,owner=base+3072,projectileRef=base+4096,projectile=base+5120,inventory=base+6144,event=base+7168,frame=base+9216,config=base+10240;
 auto word=[](UINT p,UINT value){*reinterpret_cast<UINT*>(p)=value;};
 fc3throw::g_engine=0x10000000;fc3throw::g_enabled=1;fc3throw::g_capture143=0;fc3throw::SetIdentityReader143(ThrowableLocalForFixture144);
 word(strategy,fc3throw::g_engine+fc3throw::kStrategyVtable);word(strategy+0x38,wrapper);word(strategy+0xf4,projectileRef);word(wrapper+4,base+11264);word(base+11264+12,base+12288);*reinterpret_cast<ULONGLONG*>(base+12288+8)=0xb566ca78807a5500ULL;word(wrapper+0x28,ownerRef);*reinterpret_cast<ULONGLONG*>(ownerRef)=9;word(wrapper+0x24,config);word(wrapper+0x3c,inventory);word(ownerRef+12,owner);word(projectileRef+12,projectile);word(frame+12,wrapper);word(frame+16,projectileRef);*reinterpret_cast<ULONGLONG*>(owner+8)=9;
 const UINT execution=base+13312,executionRef=base+13824,executionEntity=base+14336;
 word(execution+4,executionRef);word(executionRef+12,executionEntity);word(execution+0x24,config);word(frame+12,execution);
 *reinterpret_cast<ULONGLONG*>(strategy+8)=*reinterpret_cast<ULONGLONG*>(executionEntity+8)=0x1122334400009900ULL;
 UINT regs[9]={strategy,execution,frame,0,strategy,0,projectile,0,0};
 auto* safeComponent=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(safeComponent!=nullptr,"native safety integration component");
 float savedStrength180[4];memcpy(savedStrength180,g_throw144.itemStrength180,sizeof(savedStrength180));g_throw144.itemStrength180[0]=g_throw144.itemStrength180[1]=float(3.0/8.0); // 182 slider scale: 0.375 is the old 1x
 for(int pass=0;pass<9;++pass){
  const int kind=(pass==1||pass==8)?2:1;const bool proof148=pass==2;const bool safeDrop156=pass==7||pass==8;g_throw144.zeroDrop148=proof148;
  ResetThrowable144("fixture reset");g_throw144.ignoreUntil=0;g_throw144.enabled=true;g_throw144.previousB=false;fc3pad::Publish(input);g_frame+=10;
  *reinterpret_cast<ULONGLONG*>(strategy+0x40)=kind==1?0x03c006e5ac354e19ULL:0x000006f6a4abae8dULL;
  *reinterpret_cast<ULONGLONG*>(projectile+8)=100+pass;regs[0]=strategy;
  // Native prepare happens before the drawable appears or the manual context
  // knows which item the wheel selected. Exercise adoption, not a fake ready ID.
  fc3throw::g_safe155={};fc3throw::SetPinIntent155(pass==0||safeDrop156);fc3throw::SetMolotovIntent158(kind==2);fc3throw::Observe(1,regs);
  if(pass==0||safeDrop156||kind==2){
   word(reinterpret_cast<UINT>(safeComponent),fc3throw::g_engine+0x1fc67c0);word(reinterpret_cast<UINT>(safeComponent)+4,projectileRef);safeComponent[0x234]=0;
   const UINT stack=base+15360;word(stack+8,reinterpret_cast<UINT>(safeComponent));UINT activation[9]={};activation[3]=stack;activation[7]=event;
   Check(fc3throw::BlockActivation155(activation)&&!safeComponent[0x234],"stock draw activation actually blocked before frontend acquires grenade");
   fc3throw::g_activateOriginal155=reinterpret_cast<void*>(PinActivationFixture155);
  }
  input.b=true;input.rightGrip=1;fc3pad::Publish(input);TickThrowable144(true);Check(g_throw144.drawing&&fc3pad::ButtonsOutput()&0x200,"draw chord owns native RB without altering wheel selection");
  input.b=false;fc3pad::Publish(input);
  auto* vb=Buffer(device,kind==1?37888:43360,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,kind==1?9276:11358,D3D11_BIND_INDEX_BUFFER);UINT stride=kind==1?32:40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
  DrawCall draw={};draw.kind=0;draw.b=0;draw.a=kind==1?2334:2799;ObserveThrowable144(context,draw);TickThrowable144(true);
  Check(g_throw144.kind==kind&&g_throw144.prepared.serial&&ThrowableLease144()&&MotionInteractionEnabled(),"actual selected native model acquires lease and revalidated pre-draw projectile");
  Mat4 frameCam={},rightHand={};Check(ThrowableFrame144(cam,frameCam)&&ThrowableRightHand144(rightHand),"tracked item root ready");
  if(kind==1){auto* uploaded=ThrowableWorld144(context,0,2334);Check(uploaded!=nullptr,"grenade actual rigid constant-buffer upload");float out[88]={};Read(device,context,uploaded,out);Check(equal(HullWorld(out),Multiply(frameCam,ThrowMat144(kThrowGrenadeClipA[kThrowGrenadeAIdx_hold].part[0]))),"actual grenade mesh shares captured wrist attachment");}
  if(kind==1){
   const UINT starts[4]={0,2334,2838,3162},counts[4]={2334,504,324,1476};
   Mat4 displayedBody={},sourceBodyInverse={};float out[88]={};
   Read(device,context,ThrowableWorld144(context,0,2334),out);Invert(HullWorld(out),displayedBody);
   Invert(ThrowMat144(kThrowGrenadeClipA[kThrowGrenadeAIdx_appear].part[0]),sourceBodyInverse);
   for(int part=1;part<4;++part){Read(device,context,ThrowableWorld144(context,starts[part],counts[part]),out);
    Check(equal(Multiply(displayedBody,HullWorld(out)),Multiply(sourceBodyInverse,ThrowMat144(kThrowGrenadeClipA[kThrowGrenadeAIdx_appear].part[part]))),"actual pin/ring/safety mesh keeps original attached transform relative to held body");}
  }
  else {
   auto* uploaded=ThrowablePalette144(context,0,2799);Check(uploaded!=nullptr,"Molotov actual skinned upload");float out[576]={};Read(device,context,uploaded,out);
   const auto& key=kThrowMolotovClipB[kThrowMolotovBIdx_rest];
   Check(equal(BoneMatrix(out,0),Multiply(frameCam,ThrowMat144(key.part[1])))&&equal(BoneMatrix(out,1),Multiply(frameCam,ThrowMat144(key.part[2]))),"177 actual cloth uses GENERICBONE and Anchor slots in the same controller frame");
   uploaded=ThrowablePalette144(context,2799,2880);Read(device,context,uploaded,out);
   Check(equal(BoneMatrix(out,0),Multiply(frameCam,ThrowMat144(key.part[2]))),"177 actual GLASS draw uses Anchor in slot zero");
   Mat4 ragToBody160={};V3 actualWick160={};Check(Invert(ArmBodyToCamera(),ragToBody160)&&MolotovWickBody158(actualWick160),"176 rag flame target available");
   uploaded=ThrowablePalette144(context,0,2799);Read(device,context,uploaded,out);
   const V3 rawTip177={.012176885269582272,-.03637806296348572,.17822809517383575};
   const V3 expectedWick160=Add(Scale(Apply(Multiply(ragToBody160,BoneMatrix(out,0)),rawTip177),254./255.),Scale(Apply(Multiply(ragToBody160,BoneMatrix(out,1)),rawTip177),1./255.));
   Check(Length(Sub(actualWick160,expectedWick160))<1e-4,"177 contact matches actual two-slot weighted hanging cloth vertex, not glass or FX neck");
  }
  if(kind==2){
   // The authored flame node belongs to model root. Check near/far emitters;
   // unknown layouts and unrelated scene fires must never qualify.
   float emitter[256]={};const V3 fire=MolotovFire145(id);emitter[88]=float(fire.x);emitter[89]=float(fire.y);emitter[90]=float(fire.z);
   Check(MolotovEmitter145(id,emitter,sizeof(emitter))&&!MolotovEmitter145(id,emitter,352),"Molotov emitter requires FX_FIRE position and complete native emitter constant");
   emitter[88]+=1;Check(!MolotovEmitter145(id,emitter,sizeof(emitter)),"unrelated world fire cannot be moved with held bottle");
   emitter[88]-=1;auto* effectCb=Buffer(device,1024,D3D11_BIND_CONSTANT_BUFFER);effectCb->SetPrivateData(kCbCopyGuid,sizeof(emitter),emitter);context->VSSetConstantBuffers(0,1,&effectCb);
   const char* shaderCode="float4 main(float3 p:POSITION):SV_Position{return float4(p,1);}";ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;
   Check(SUCCEEDED(D3DCompile(shaderCode,strlen(shaderCode),nullptr,nullptr,nullptr,"main","vs_5_0",0,0,&code,&errors)),"Molotov particle fixture shader");SafeRelease(errors);
   ID3D11VertexShader* vs=nullptr;Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs)),"Molotov particle fixture VS creation");code->Release();
   ULONGLONG hash=0xDB23497C92F47D21ULL;vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);context->VSSetShader(vs,nullptr,0);
   D3D11_BLEND_DESC blendDesc={};blendDesc.RenderTarget[0].BlendEnable=true;blendDesc.RenderTarget[0].SrcBlend=blendDesc.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE;blendDesc.RenderTarget[0].DestBlend=blendDesc.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_ZERO;blendDesc.RenderTarget[0].BlendOp=blendDesc.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;blendDesc.RenderTarget[0].RenderTargetWriteMask=15;
   ID3D11BlendState* blend=nullptr;Check(SUCCEEDED(device->CreateBlendState(&blendDesc,&blend)),"Molotov particle blend");context->OMSetBlendState(blend,nullptr,~0u);
   float points[12]={0,float(fire.y),float(fire.z), .02f,float(fire.y),float(fire.z), 0,float(fire.y),float(fire.z)+.02f, 20,30,40};
   D3D11_BUFFER_DESC desc={};desc.ByteWidth=sizeof(points);desc.Usage=D3D11_USAGE_DYNAMIC;desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
   ID3D11Buffer* effectVb=nullptr;D3D11_SUBRESOURCE_DATA initial={points};Check(SUCCEEDED(device->CreateBuffer(&desc,&initial,&effectVb)),"Molotov world particle source");effectVb->SetPrivateData(kCbCopyGuid,sizeof(points),points);effectVb->SetPrivateData(kRepairVertexFrameGuid,sizeof(g_frame),&g_frame);
   auto* effectIb=Buffer(device,16,D3D11_BIND_INDEX_BUFFER);unsigned short indices[8]={0,1,2};effectIb->SetPrivateData(kCbCopyGuid,sizeof(indices),indices);
   stride=12;context->IASetVertexBuffers(0,1,&effectVb,&stride,&offset);context->IASetIndexBuffer(effectIb,DXGI_FORMAT_R16_UINT,0);
   g_throw144.nativeMolotovWorld=id;g_throw144.nativeMolotovFrame=g_frame;DrawCall particles={};particles.kind=0;particles.a=3;ID3D11Buffer* originalVb=nullptr;UINT effectStride=0,effectOffset=0;
   Check(!BeginMolotovParticles145(context,particles,0,originalVb,effectStride,effectOffset)&&!originalVb,"160 never guesses flame identity from a shared world-particle shader");
   float source[12]={};Read(device,context,effectVb,source);Compare(source,points,12,"native160 leaves all shared scene particle vertices untouched");
   context->VSSetShader(nullptr,nullptr,0);context->OMSetBlendState(nullptr,nullptr,~0u);context->VSSetConstantBuffers(0,1,&globals);context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
   effectCb->Release();effectVb->Release();effectIb->Release();vs->Release();blend->Release();

  }
  // Actual arm upload, then captured fingers at its final wrist.
  stride=40;context->IASetVertexBuffers(0,1,&arm,&stride,&offset);memcpy(native,kNeutralArmView[0],sizeof(kNeutralArmView[0]));palette->SetPrivateData(kCbCopyGuid,sizeof(native),native);g_armJoints[0]={{.2,-.15,-.2},{.35,-.3,.05},{.35,-.2,.4},true};
  auto* armOut=BuildArmPalette(context,0);Check(armOut!=nullptr,"throwable arm upload");float armBones[576];Read(device,context,armOut,armBones);Check(equal(BoneMatrix(armBones,2),rightHand),"actual rendered right wrist matches item attachment");const float* fingers=kind==1?kThrowGrenadeClipA[kThrowGrenadeAIdx_hold].rightFingers:kThrowMolotovClipB[kThrowMolotovBIdx_rest].rightFingers;for(int b=3;b<=19;++b)Check(equal(BoneMatrix(armBones,b),Multiply(rightHand,BoneMatrix(fingers,b-3))),"captured throwable fingers preserve final wrist");
  if(pass==0)TestGrenadePin154(device,context,vb,ib,arm,globals,camera,palette);
  if(pass==1){Check(g_molotov158.manualIgnition159&&fc3throw::g_molotovIntent158,"manual Molotov activation deferred until calibrated flame touches rag");TestMolotovLighter158(device,context,camera);}
  input.active=false;fc3pad::Publish(input);TickThrowable144(true);Check((fc3pad::ButtonsOutput()&0x200)!=0&&!g_throw144.gripped,"tracking loss preserves native hold and disarms release edge");
  input.active=true;input.rightGrip=1;fc3pad::Publish(input);TickThrowable144(true);Check(!g_throw144.releasing&&g_throw144.prepared.serial,"tracking recovery while RIGHT GRIP remains held revalidates the same grenade without throwing");
  input.y=true;fc3pad::Publish(input);TickThrowable144(true);
  Check(ThrowableOwns144()&&(fc3pad::ButtonsOutput()&0x200)&&!(fc3pad::ButtonsOutput()&0x8000),"Y alone cannot reset ownership and accidentally throw");
  input.y=false;
  input.rightGrip=1;fc3pad::Publish(input);TickThrowable144(true);
  // Normal hand motion creates the real release sample through Tick, rather
  // than injecting a precomputed trajectory directly into the bridge.
  const ULONGLONG priorClock171=g_fixtureClock171;g_fixtureClock171=GetTickCount64();
  g_throw144.history.Clear();TickThrowable144(true); // real Tick builds history on an explicit clock
  if(proof148||safeDrop156)g_throw144.history.Clear();
  else for(int n=0;n<(pass==6?11:6);++n){g_fixtureClock171+=kind==2?50:10;++g_frame;g_throw144.seen=g_frame;g_handBody[0][3]+=pass==6?(n<5?.04f:.0001f):pass==3?.004f:pass==4?.06f:pass==5?-.025f:kind==2?.06f:.015f;TickThrowable144(true);}
  if(pass==0){g_throw144.manualPin154=true;g_grenadePin154.out=true;g_grenadePin154.fraction=1;Check(fc3throw::RequestPinOut155(g_throw144.prepared),"mechanical pin completion requests native activation before grip release");}
  if(safeDrop156){g_throw144.manualPin154=true;g_grenadePin154.out=false;input.active=false;input.rightGrip=0;fc3pad::Publish(input);TickThrowable144(true);Check(!g_throw144.releasing,"tracking pause alone cannot create a release");input.active=true;g_throw144.gripped=false;}
  if(pass==6)Sleep(50); // scheduling stalls must not change the authored sample timestamps
  input.rightGrip=0;fc3pad::Publish(input);TickThrowable144(true);Check(g_throw144.releasing&&!(fc3pad::ButtonsOutput()&0x200)&&fc3pad::FireOutput()==0,"grip release after completed manual pin queues accepted physical trajectory and releases native RB without gun fire");
  const auto queued=fc3throw::g_pending143.release;
  ULONGLONG receipt157=0;
  Check(!fc3throw::ReleaseCompleted157(g_throw144.prepared,receipt157)&&!ThrowableHidden144()&&ThrowableLease144(),"queued release keeps held mesh until its exact native launch; no speculative disappearance");
  Check((proof148||safeDrop156)?fc3manual::Length(queued.velocity)==0:pass==6?queued.velocity.x>2.5&&queued.velocity.x<5:pass==3?queued.velocity.x>.1&&queued.velocity.x<.8:pass==4?queued.velocity.x>3&&queued.velocity.x<8:pass==5?queued.velocity.x<-1&&queued.velocity.x>-4:queued.velocity.x>.5&&queued.velocity.x<3&&fabs(queued.velocity.y)<.05,"physical gentle strong sideways and delayed-release throws follow hand stroke; optional proof alone forces zero");
  float sourceOrigin[3]={float(queued.origin.x),float(queued.origin.y),float(queued.origin.z)};memcpy(reinterpret_cast<void*>(strategy+0x88),sourceOrigin,12);regs[0]=strategy;
  {
   *reinterpret_cast<UINT*>(projectile+0x60)=0x208;releaseFrame148=frame;nativeCalls148=0;fc3throw::g_nativeRelease146=reinterpret_cast<fc3throw::NativeRelease146>(DropReleaseFixture148);
   const UINT stack=base+15360;word(stack+8,execution);UINT entry[9]={};entry[6]=strategy;entry[3]=stack;fc3throw::PumpRelease146(entry);
   if(safeDrop156)Check(!safeComponent[0x234]&&fc3throw::g_safe155.loose,"actual RIGHT GRIP release launches unarmed grenade or unlit Molotov without activation");
   if(pass==0)Check(safeComponent[0x234]&&fc3throw::g_safe155.armed,"real frontend grip release -> native update arms deferred grenade before physical dispatch");
   Check(nativeCalls148==1&&fc3throw::ManualStatus143().completed>g_throw144.completedBefore,"frontend moving or stationary release launches on next real native update path with actual0x208 flags");fc3throw::PumpRelease146(entry);Check(nativeCalls148==1,"physical throw or drop dispatches once");fc3throw::g_nativeRelease146=nullptr;
   if(safeDrop156)std::puts("PASS:156 open RIGHT GRIP without a fresh edge and without motion history -> pin-in unarmed native physical drop.");
   else if(proof148)std::puts("PASS:148 stationary controller grip release with no velocity history -> native update0x208 -> exact zero velocity -> one launch dispatch.");
   else std::puts("PASS:151 moving controller grip release -> native update0x208 -> measured velocity -> one launch dispatch.");
  }
  Check(fabs(reinterpret_cast<float*>(strategy+0xc8)[0]-queued.velocity.x)<1e-5,"frontend release reaches actual production native velocity write");
  auto wrongReceipt157=g_throw144.prepared;++wrongReceipt157.serial;
  Check(!fc3throw::ReleaseCompleted157(wrongReceipt157,receipt157),"another release serial cannot retire the held visual");
  Check(!g_throw144.launched&&fc3throw::ReleaseCompleted157(g_throw144.prepared,receipt157)&&receipt157>=queued.ms,"native launch receipt precedes the next frontend Tick");
  if(pass%2)Check(ThrowableArmLease150(),"body draw consumes native launch and retains tracked arm before next Present");
  else Check(ThrowableHidden144(),"weapon draw hides launched held mesh before next Present");
  Check(g_throw144.launched&&ThrowableHidden144()&&!ThrowableLease144()&&ThrowableArmLease150(),"launch retires item/finger hold and starts arm cleanup together, before next frontend Tick");
  TickThrowable144(true);Check(g_throw144.launched&&ThrowableHidden144(),"later frontend acknowledgement cannot revive launched held visual");
  // Native throw animation keeps drawing after the item disappears. Exercise
  // the actual uploaded palette with no weapon draws, both before and after
  // the item/input lease ends. All arm bones must reject native animation.
  stride=40;context->IASetVertexBuffers(0,1,&arm,&stride,&offset);
  auto checkCleanup=[&](){
   Check(ThrowableArmMotion150()&&!MotionInteractionEnabled()&&!ThrowableLease144(),"render-only post-launch arm lease survives missing held item without reviving projectile aim or camera lease");
   Mat4 wanted={};Check(ThrowableRightHand144(wanted),"post-launch tracked wrist target remains available");
   float first[576]={},second[576]={};memcpy(native,kNeutralArmView[0],sizeof(kNeutralArmView[0]));palette->SetPrivateData(kCbCopyGuid,sizeof(native),native);
   auto* output=BuildArmPalette(context,0);Check(output!=nullptr,"post-launch actual arm GPU upload with no weapon");Read(device,context,output,first);
   Check(equal(BoneMatrix(first,2),wanted),"post-launch wrist follows controller instead of throwing");
   const Mat4 animation=TurnAndMove({0,0,1},{0,1,0},{0,0,0},{.3,.4,-.2});MoveBones(native,0,24,animation);palette->SetPrivateData(kCbCopyGuid,sizeof(native),native);
   output=BuildArmPalette(context,0);Check(output!=nullptr,"animated throw source still produces controlled arm upload");Read(device,context,output,second);
   for(int b=0;b<25;++b)Check(equal(BoneMatrix(first,b),BoneMatrix(second,b)),"entire final arm including shoulder elbow wrist and fingers rejects native throw motion");
   // The world-camera shirt draw has its own dispatch guard. Verify that
   // guard and its real GPU buffer, rather than only calling the arm path.
   Check(ControllerBodyDraw151(0)&&!ControllerBodyDraw151(1),"world-camera shirt dispatch remains active through render-only arm cleanup");
   g_bodyDrawStart=0;g_bodyDrawCount=3687;g_armFinalFrame[1]=-10;
   float shirtSource[576]={},shirtOut[576]={};memcpy(shirtSource,native,sizeof(native));palette->SetPrivateData(kCbCopyGuid,sizeof(shirtSource),shirtSource);
   auto* shirt=ControllerBodyDraw151(0)?BuildBodyPalette(context):nullptr;Check(shirt!=nullptr,"actual world-camera shirt GPU upload after held item disappears");Read(device,context,shirt,shirtOut);
   const int bodySlots[3]={11,12,13},armSlots[3]={0,22,23};
   for(int n=0;n<3;++n)Check(equal(BoneMatrix(shirtOut,bodySlots[n]),BoneMatrix(second,armSlots[n])),"shirt shoulder bones follow final controller arm instead of native throw animation");
   Check(equal(BoneMatrix(shirtOut,4),BoneMatrix(shirtSource,4)),"unrelated torso bone is preserved");
  };
  checkCleanup();
  g_frame+=6;g_throw144.started=GetTickCount64()-600;TickThrowable144(true);
  Check(!ThrowableOwns144()&&!(fc3pad::ButtonsOutput()&0x200),"missing item ends input ownership and RB hold independently of arm recovery");
  g_handBody[0][3]+=.09f;g_handBody[0][7]+=.04f;checkCleanup();
  fc3aim::SetNativeActionForTest128(1);Check(!ThrowableArmMotion150()&&!MotionInteractionEnabled(),"native traversal retains priority over throw arm recovery");fc3aim::SetNativeActionForTest128(0);
  if(pass==0){g_weaponMechanicFrame=++g_frame;TickThrowable144(true);Check(!ThrowableArmLease150(),"observed returning firearm ends cleanup lease");g_weaponMechanicFrame=-100000;}
  else if(pass==1){g_throwArm150.until=GetTickCount64()-1;TickThrowable144(true);Check(!ThrowableArmLease150(),"unarmed cleanup lease expires without requiring a firearm");}
  else {++g_armPoseEpoch;TickThrowable144(true);Check(!ThrowableArmLease150(),"recenter cancels throw arm recovery");}
  // Restore owned state solely to retain the existing wheel-priority check.
  g_throw144.owned=g_throw144.launched=true;g_throw144.epoch=g_armPoseEpoch;g_throw144.seen=g_frame;g_throw144.started=GetTickCount64();g_throw144.releaseAt=GetTickCount64();
  input.leftGrip=1;input.y=true;fc3pad::Publish(input);TickThrowable144(true);Check(!ThrowableOwns144()&&(fc3pad::ButtonsOutput()&0x100),"native weapon wheel outranks throwable lease and remains selectable");
  g_fixtureClock171=priorClock171;
  vb->Release();ib->Release();g_throw144.kind=0;g_throw144.manualPin154=false;input.b=input.y=false;input.leftGrip=input.rightGrip=0;fc3pad::Publish(input);
 }
 fc3throw::g_activateOriginal155=nullptr;fc3throw::g_safe155={};VirtualFree(safeComponent,0,MEM_RELEASE);
 ResetThrowable144("fixture complete");fc3throw::SetIdentityReader143(nullptr);fc3throw::g_enabled=0;fc3throw::g_capture143=1;context->ClearState();VirtualFree(page,0,MEM_RELEASE);
 g_weaponPaletteCb->Release();g_weaponPaletteCb=oldWeapon;g_armPaletteCb[0]->Release();g_armPaletteCb[0]=oldArm;g_bodyPaletteCb->Release();g_bodyPaletteCb=oldBody;globals->Release();camera->Release();palette->Release();arm->Release();fc3aim::SetNativeActionForTest128(-1);fc3aim::SetVehicleStateForTest(-1);
 g_throw144.manualPin154=savedManualPin154;memcpy(g_throw144.itemStrength180,savedStrength180,sizeof(savedStrength180));
 std::puts("PASS: wheel-selected frag/Molotov native preparation adoption, actual item/arm/finger GPU uploads, moving controller Tick history, grip-to-RB handoff, real native launch vector writes and post-launch held-visual suppression.");
}
