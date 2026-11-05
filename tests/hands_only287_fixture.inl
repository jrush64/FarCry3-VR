HRESULT STDMETHODCALLTYPE HandsCreateBuffer287(ID3D11Device* device,const D3D11_BUFFER_DESC* desc,const D3D11_SUBRESOURCE_DATA* data,ID3D11Buffer** buffer){return device->CreateBuffer(desc,data,buffer);}
void TestHandsOnly287(ID3D11Device* device,ID3D11DeviceContext* context){
 g_dev=device;g_ctx=context;g_oCreateBuffer=HandsCreateBuffer287;
 g_enabled=g_motionGun=g_eyeViewsSet=g_inGameplay=true;g_visibleWeaponDrawsLastFrame=10;g_pistolMenu.open=false;
 fc3aim::SetVehicleStateForTest(0);fc3aim::SetNativeActionForTest128(0);
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(true);
 g_handsClipAvailable287=true;g_handsShaderDevice287=device;g_handsOnly287=false;
 Check(!HandsOnlyActive287()&&!g_handsClipBuffer287,"287 full arms remain default with no clipping buffer allocation");
 g_handsOnly287=true;Check(HandsOnlyActive287(),"287 optional hands-only presentation is eligible during tracked weapon gameplay");
 D3D11_BUFFER_DESC desc={};g_handsClipBuffer287->GetDesc(&desc);Check(desc.Usage==D3D11_USAGE_IMMUTABLE&&desc.ByteWidth==16,"287 each recorded draw owns immutable clip controls");
 auto* old=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);context->VSSetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&old);
 {
  HandsClipBinding287 binding(context,true,true);ID3D11Buffer* current=nullptr;context->VSGetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&current);
  Check(current==g_handsClipBuffer287,"287 eligible arm draws bind dedicated controls");SafeRelease(current);
 }
 ID3D11Buffer* restored=nullptr;context->VSGetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&restored);Check(restored==old,"287 previous constant buffer restored after arm draw");SafeRelease(restored);
 {
  HandsClipBinding287 binding(context,true,false);ID3D11Buffer* current=nullptr;context->VSGetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&current);Check(!current,"287 disabled or native-action arm draw cannot inherit an earlier clip");SafeRelease(current);
 }
 fc3aim::SetNativeActionForTest128(1);Check(!HandsOnlyActive287(),"287 climbing/scripted actions retain native animated arms");fc3aim::SetNativeActionForTest128(0);
 fc3aim::SetVehicleStateForTest(1);Check(!HandsOnlyActive287(),"287 native vehicle ownership preserves existing presentation");fc3aim::SetVehicleStateForTest(0);
 auto* initial=g_handsClipBuffer287;initial->AddRef();g_wristCut287=.59f;Check(HandsOnlyBuffer287()!=initial,"287 wrist edit creates new controls without mutating recorded buffer");initial->Release();
 wchar_t folder[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,folder);GetTempFileNameW(folder,L"hnd",0,path);
 WritePrivateProfileStringW(L"Avatar",L"HandsOnly",L"1",path);WritePrivateProfileStringW(L"Avatar",L"WristCut",L"0.59",path);LoadHandsOnly287(path);Check(g_handsOnly287&&fabs(g_wristCut287-.59f)<1e-5f,"287 saved mode and wrist edge load");
 WritePrivateProfileStringW(L"Avatar",L"WristCut",L"nan",path);LoadHandsOnly287(path);Check(g_wristCut287==.57f,"287 invalid wrist trim falls back safely");DeleteFileW(path);
 g_inGameplay=false;Check(!HandsOnlyActive287(),"300 menu/title scene preserves native avatar");g_inGameplay=true;
 g_eyeViewsSet=false;Check(!HandsOnlyActive287(),"300 missing stereo eye views preserves native avatar");g_eyeViewsSet=true;
 g_handsClipAvailable287=false;Check(!HandsOnlyActive287(),"300 unpatched shader preserves native avatar");g_handsClipAvailable287=true;
 fc3pad::SetPistolCalibration(true);Check(!HandsOnlyActive287(),"300 open VR settings blocks avatar override");fc3pad::SetPistolCalibration(false);
 g_handsOnly287=false;context->VSSetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&restored);old->Release();SafeRelease(g_handsClipBuffer287);
 puts("PASS:287 disabled default, immutable controls, restored draw state, native traversal/vehicle priority and bounded saved appearance settings");
}
