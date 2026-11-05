// A black metallic diffuse texture can conceal a colour multiplier. While the
// Vector's handle alone is being cued, sample white in its reflected diffuse
// slots; the selected pulsing material colour supplies the guide. Other maps
// and all normal receiver segments keep their exact original bindings.
struct ActionDiffuseSlots231 {UINT count=0,slots[2]={};};
const GUID kActionDiffuse231={0xaca12310,0x31bf,0x4c25,{0x91,0xbc,0x46,0x42,0x55,0x72,0x31,0x01}};
ID3D11ShaderResourceView* g_actionWhite231=nullptr;ID3D11Device* g_actionWhiteDevice231=nullptr;
void ReleaseActionWhite231(){SafeRelease(g_actionWhite231);g_actionWhiteDevice231=nullptr;}
struct VectorDiffuseCue231 {
 ID3D11DeviceContext* context=nullptr;ActionDiffuseSlots231 layout;ID3D11ShaderResourceView* original[2]={};
 VectorDiffuseCue231(ID3D11DeviceContext* ctx,bool active){
  if(!active)return;ID3D11PixelShader* shader=nullptr;ctx->PSGetShader(&shader,nullptr,nullptr);if(!shader)return;
  UINT bytes=sizeof(layout);
  if(FAILED(shader->GetPrivateData(kActionDiffuse231,&bytes,&layout))||bytes!=sizeof(layout)){
   layout={};UINT length=0;if(SUCCEEDED(shader->GetPrivateData(kEjectionBytecodeGuid,&length,nullptr))&&length&&length<=65536){
    std::vector<BYTE> code(length);ID3D11ShaderReflection* reflection=nullptr;
    if(SUCCEEDED(shader->GetPrivateData(kEjectionBytecodeGuid,&length,code.data()))&&SUCCEEDED(D3DReflect(code.data(),length,IID_ID3D11ShaderReflection,reinterpret_cast<void**>(&reflection)))){
     D3D11_SHADER_DESC shaderDesc={};reflection->GetDesc(&shaderDesc);
     // Native SM4 metadata gives the texture and sampler the SAME name.
     // Lookup by name alone returns the sampler on many metallic shaders.
     for(UINT resource=0;resource<shaderDesc.BoundResources;++resource){D3D11_SHADER_INPUT_BIND_DESC binding={};
      if(layout.count<2&&SUCCEEDED(reflection->GetResourceBindingDesc(resource,&binding))&&binding.Name&&(!strcmp(binding.Name,"DiffuseTexture1")||!strcmp(binding.Name,"DiffuseTexture2")||!strcmp(binding.Name,"BaseDiffuseTexture"))&&binding.Type==D3D_SIT_TEXTURE&&binding.Dimension==D3D_SRV_DIMENSION_TEXTURE2D&&binding.BindCount==1&&binding.BindPoint<128){bool duplicate=false;for(UINT n=0;n<layout.count;++n)if(layout.slots[n]==binding.BindPoint)duplicate=true;if(!duplicate)layout.slots[layout.count++]=binding.BindPoint;}
     }SafeRelease(reflection);
    }
   }shader->SetPrivateData(kActionDiffuse231,sizeof(layout),&layout);
  }SafeRelease(shader);if(!layout.count)return;
  ID3D11Device* device=nullptr;ctx->GetDevice(&device);if(g_actionWhiteDevice231!=device)ReleaseActionWhite231();
  if(!g_actionWhite231){D3D11_TEXTURE2D_DESC desc={};desc.Width=desc.Height=desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.Usage=D3D11_USAGE_IMMUTABLE;
   const UINT white=0xffffffff;D3D11_SUBRESOURCE_DATA data={};data.pSysMem=&white;data.SysMemPitch=4;ID3D11Texture2D* texture=nullptr;
   if(SUCCEEDED(device->CreateTexture2D(&desc,&data,&texture)))device->CreateShaderResourceView(texture,nullptr,&g_actionWhite231);SafeRelease(texture);if(g_actionWhite231)g_actionWhiteDevice231=device;
  }SafeRelease(device);if(!g_actionWhite231)return;
  context=ctx;for(UINT n=0;n<layout.count;++n){ctx->PSGetShaderResources(layout.slots[n],1,&original[n]);ctx->PSSetShaderResources(layout.slots[n],1,&g_actionWhite231);}
 }
 ~VectorDiffuseCue231(){if(context)for(UINT n=0;n<layout.count;++n){context->PSSetShaderResources(layout.slots[n],1,&original[n]);SafeRelease(original[n]);}}
};
