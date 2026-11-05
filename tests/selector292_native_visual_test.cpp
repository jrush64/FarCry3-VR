#include "../src/selector_native_capture292.h"
#include <cstdio>
#include <cstdlib>
static const GUID codeGuid={0x9292,0,0,{0,0,0,0,0,0,0,1}};
void Check(bool value,const char* name){printf("%s: %s\n",value?"PASS":"FAIL",name);if(!value)exit(1);}
template<class T> void Release(T*& p){fc3visual292::Drop(p);}
int main(){
 ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;
 Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"WARP device; no game or headset");
 const char* source="cbuffer NativeMatrix:register(b0){float4 Offset;};struct I{float3 p:POSITION;float2 uv:TEXCOORD0;};struct O{float4 p:SV_POSITION;float2 uv:TEXCOORD0;float3 n:TEXCOORD1;};O vs(I i){O o;o.p=float4(i.p+Offset.xyz,1);o.uv=i.uv;o.n=float3(0,0,1);return o;}Texture2D Skin:register(t3);SamplerState SS:register(s2);cbuffer Material:register(b0){float4 Tint;};struct P{float4 colour:SV_TARGET0;float4 normal:SV_TARGET1;};P ps(O i){P o;o.colour=Skin.Sample(SS,i.uv)*Tint;o.colour.a=0;o.normal=float4(i.n,1);return o;}";
 ID3DBlob *vsCode=nullptr,*psCode=nullptr,*errors=nullptr;
 HRESULT hr=D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"vs","vs_4_0",0,0,&vsCode,&errors);if(errors)printf("%s",(char*)errors->GetBufferPointer());Release(errors);Check(SUCCEEDED(hr),"native test vertex shader compiled");
 hr=D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"ps","ps_4_0",0,0,&psCode,&errors);if(errors)printf("%s",(char*)errors->GetBufferPointer());Release(errors);Check(SUCCEEDED(hr),"native two-target material compiled");
 ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;device->CreateVertexShader(vsCode->GetBufferPointer(),vsCode->GetBufferSize(),nullptr,&vs);device->CreatePixelShader(psCode->GetBufferPointer(),psCode->GetBufferSize(),nullptr,&ps);
 vs->SetPrivateData(codeGuid,UINT(vsCode->GetBufferSize()),vsCode->GetBufferPointer());ps->SetPrivateData(codeGuid,UINT(psCode->GetBufferSize()),psCode->GetBufferPointer());
 D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};ID3D11InputLayout* layout=nullptr;device->CreateInputLayout(elements,2,vsCode->GetBufferPointer(),vsCode->GetBufferSize(),&layout);
 const float vertex[]={-.5f,-.5f,.5f,0,1,0,.5f,.5f,.5f,0,.5f,-.5f,.5f,1,1};const USHORT index[]={0,1,2};
 ID3D11Buffer *vb=nullptr,*ib=nullptr,*cb=nullptr;D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(vertex);bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;D3D11_SUBRESOURCE_DATA init={vertex,0,0};device->CreateBuffer(&bd,&init,&vb);bd.ByteWidth=sizeof(index);bd.BindFlags=D3D11_BIND_INDEX_BUFFER;init.pSysMem=index;device->CreateBuffer(&bd,&init,&ib);float tint[]={1,.5f,.25f,1};bd.ByteWidth=16;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;init.pSysMem=tint;device->CreateBuffer(&bd,&init,&cb);
 ID3D11Buffer* matrix=nullptr;float offset[]={0,0,0,0};init.pSysMem=offset;device->CreateBuffer(&bd,&init,&matrix);
 D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=1;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;UINT skin=0xff40a0e0;init={&skin,4,0};ID3D11Texture2D* texture=nullptr;ID3D11ShaderResourceView* srv=nullptr;device->CreateTexture2D(&td,&init,&texture);device->CreateShaderResourceView(texture,nullptr,&srv);
 D3D11_SAMPLER_DESC sd={};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;ID3D11SamplerState* sampler=nullptr;device->CreateSamplerState(&sd,&sampler);
 UINT stride=20,zero=0;context->IASetInputLayout(layout);context->IASetVertexBuffers(0,1,&vb,&stride,&zero);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);context->VSSetConstantBuffers(0,1,&matrix);context->PSSetConstantBuffers(0,1,&cb);context->PSSetShaderResources(3,1,&srv);context->PSSetSamplers(2,1,&sampler);
 {
 fc3visual292::Part captured;std::string reason;bool ok=fc3visual292::Capture(device,context,3,0,0,codeGuid,captured,reason);if(!ok)printf("capture: %s\n",reason.c_str());Check(ok,"shipping native stream-output capture accepts real D3D resources");
 Check(captured.textures[3]==srv&&captured.samplers[2]==sampler,"exact bound skin and sampler retained, no guessed texture slot");Check(captured.constants[0]&&captured.constants[0]!=cb,"instance material constant buffer privately snapshotted");
 ID3D11VertexShader* restored=nullptr;context->VSGetShader(&restored,nullptr,nullptr);Check(restored==vs,"game shader restored after native capture");Release(restored);
 D3D11_BUFFER_DESC read={};captured.vertices->GetDesc(&read);read.Usage=D3D11_USAGE_STAGING;read.BindFlags=0;read.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Buffer* staging=nullptr;device->CreateBuffer(&read,nullptr,&staging);context->CopyResource(staging,captured.vertices);D3D11_MAPPED_SUBRESOURCE map={};Check(SUCCEEDED(context->Map(staging,0,D3D11_MAP_READ,0,&map)),"captured geometry readback");const float* raw=(float*)map.pData;Check(raw[0]==-.5f&&raw[1]==-.5f&&raw[2]==.5f&&raw[3]==1&&raw[4]==0&&raw[5]==1,"native vertex shader positions and UVs survive stream output exactly");context->Unmap(staging,0);Release(staging);
 // Mutating the live material after capture must not recolour another slot.
 float changed[]={0,0,0,0};context->UpdateSubresource(cb,0,nullptr,changed,0,0);read.ByteWidth=16;device->CreateBuffer(&read,nullptr,&staging);context->CopyResource(staging,captured.constants[0]);context->Map(staging,0,D3D11_MAP_READ,0,&map);raw=(float*)map.pData;Check(raw[0]==1&&raw[1]==.5f&&raw[2]==.25f,"owned material remains intact after live constants change");context->Unmap(staging,0);Release(staging);
 }
 context->ClearState();context->Flush();Release(sampler);Release(srv);Release(texture);Release(matrix);Release(cb);Release(ib);Release(vb);Release(layout);Release(vs);Release(ps);Release(vsCode);Release(psCode);Release(context);Release(device);return 0;
}
