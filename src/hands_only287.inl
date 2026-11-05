constexpr GUID kHandsClipShader287={0x7bac0287,0x61c4,0x42d1,{0x92,0x64,0x31,0x88,0x27,0x01,0xb7,0xaa}};
bool g_handsOnly287=false,g_handsClipAvailable287=false;
ID3D11Device* g_handsShaderDevice287=nullptr; // identity only; shader/device lifetimes belong to the game
float g_wristCut287=.57f,g_wristCutUploaded287=-1;
ID3D11Buffer* g_handsClipBuffer287=nullptr;
void LoadHandsOnly287(const wchar_t* path){
 g_handsOnly287=GetPrivateProfileIntW(L"Avatar",L"HandsOnly",0,path)!=0;
 wchar_t text[64]={};GetPrivateProfileStringW(L"Avatar",L"WristCut",L"0.57",text,64,path);
 wchar_t* end=nullptr;const float value=float(wcstod(text,&end));g_wristCut287=end!=text&&!*end&&std::isfinite(value)&&value>=.54f&&value<=.62f?value:.57f;
}
ID3D11Buffer* HandsOnlyBuffer287(){
 if(!g_dev||!g_oCreateBuffer||!std::isfinite(g_wristCut287)||g_wristCut287<.54f||g_wristCut287>.62f)return nullptr;
 if(g_handsClipBuffer287&&g_wristCutUploaded287==g_wristCut287)return g_handsClipBuffer287;
 // Immutable per setting: deferred command lists retain the exact buffer used by their draw.
 const float values[4]={1,g_wristCut287*g_wristCut287,0,0};D3D11_BUFFER_DESC desc={};desc.ByteWidth=16;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
 D3D11_SUBRESOURCE_DATA initial={values,0,0};ID3D11Buffer* next=nullptr;
 if(FAILED(g_oCreateBuffer(g_dev,&desc,&initial,&next)))return nullptr;
 SafeRelease(g_handsClipBuffer287);g_handsClipBuffer287=next;g_wristCutUploaded287=g_wristCut287;return next;
}
bool HandsOnlyActive287(){return g_handsOnly287&&g_handsClipAvailable287&&g_handsShaderDevice287==g_dev&&g_enabled&&g_inGameplay&&g_eyeViewsSet&&MotionInteractionEnabled()&&!fc3pad::GameplayInputBlocked()&&HandsOnlyBuffer287()!=nullptr;}
struct HandsClipBinding287 {
 ID3D11DeviceContext* context=nullptr;ID3D11Buffer* old=nullptr;
 HandsClipBinding287(ID3D11DeviceContext* c,bool capable,bool active){if(!capable)return;context=c;context->VSGetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&old);ID3D11Buffer* buffer=active?HandsOnlyBuffer287():nullptr;context->VSSetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&buffer);}
 ~HandsClipBinding287(){if(context){context->VSSetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&old);SafeRelease(old);}}
};
