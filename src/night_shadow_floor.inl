// Neutralize only the demonstrated negative final brightness offset in VR
// gameplay. The original engine buffer, contrast, gamma and LUT stay intact.
struct NightShadowFloor {
 ID3D11DeviceContext* context=nullptr;ID3D11Buffer* original=nullptr;ID3D11Buffer* adjusted=nullptr;
 NightShadowFloor(ID3D11DeviceContext* c,unsigned long long shader,bool gameplay){
  if(!gameplay||shader!=0xC5D7AC9B41B47C00ull)return;
  c->PSGetConstantBuffers(0,1,&original);UINT n=0;
  if(!original||FAILED(original->GetPrivateData(kCbCopyGuid,&n,nullptr))||n<608||n>4096)return;
  std::vector<unsigned char> bytes(n);if(FAILED(original->GetPrivateData(kCbCopyGuid,&n,bytes.data())))return;
  float brightness=0;memcpy(&brightness,bytes.data()+37*16,4);
  if(!std::isfinite(brightness)||brightness>=0||brightness<-.1f)return;
  D3D11_BUFFER_DESC desc={};original->GetDesc(&desc);if(desc.ByteWidth!=n)return;
  const float neutral=0;memcpy(bytes.data()+37*16,&neutral,4);
  desc.Usage=D3D11_USAGE_DEFAULT;desc.CPUAccessFlags=desc.MiscFlags=0;D3D11_SUBRESOURCE_DATA data={bytes.data(),0,0};
  ID3D11Device* device=nullptr;c->GetDevice(&device);const HRESULT hr=device?device->CreateBuffer(&desc,&data,&adjusted):E_FAIL;SafeRelease(device);if(FAILED(hr))return;
  context=c;c->PSSetConstantBuffers(0,1,&adjusted);
  static unsigned logged=0;if(logged++<2)Log("[NIGHT39] final gameplay brightness offset %f -> 0; original CB, gamma, contrast and color LUT preserved",brightness);
 }
 ~NightShadowFloor(){if(context)context->PSSetConstantBuffers(0,1,&original);SafeRelease(adjusted);SafeRelease(original);}
};
