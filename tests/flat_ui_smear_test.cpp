#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include "../src/dxbc_patch.h"
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")
void Check(bool x,const char* m){if(!x){std::fprintf(stderr,"FAIL: %s\n",m);std::exit(1);}}
ID3DBlob* Compile(const char* s,const char* profile){ID3DBlob* code=nullptr,*err=nullptr;HRESULT hr=D3DCompile(s,strlen(s),nullptr,nullptr,nullptr,"main",profile,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&err);if(err){std::fprintf(stderr,"%s",(const char*)err->GetBufferPointer());err->Release();}Check(SUCCEEDED(hr),"compile");return code;}
int main(){
 ID3D11Device* dev=nullptr;ID3D11DeviceContext* ctx=nullptr;Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,nullptr,&ctx)),"WARP");
 auto* vsCode=Compile("float4 main(float4 p:POSITION):SV_Position{return p;}","vs_5_0");
 std::vector<uint8_t> patched;Check(dxbc::PatchVertexShader(vsCode->GetBufferPointer(),vsCode->GetBufferSize(),patched)==dxbc::Result::Patched,"real production vertex patch");
 ID3D11VertexShader* vs=nullptr;Check(SUCCEEDED(dev->CreateVertexShader(patched.data(),patched.size(),nullptr,&vs)),"patched VS create");
 D3D11_INPUT_ELEMENT_DESC el={"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};ID3D11InputLayout* layout=nullptr;Check(SUCCEEDED(dev->CreateInputLayout(&el,1,vsCode->GetBufferPointer(),vsCode->GetBufferSize(),&layout)),"layout");vsCode->Release();
 auto* gsCode=Compile("struct O{float4 p:SV_Position;};[maxvertexcount(1)]void main(point O input[1],inout PointStream<O> output){output.Append(input[0]);}","gs_5_0");
 D3D11_SO_DECLARATION_ENTRY so={0,"SV_Position",0,0,4,0};UINT stride=16;ID3D11GeometryShader* gs=nullptr;Check(SUCCEEDED(dev->CreateGeometryShaderWithStreamOutput(gsCode->GetBufferPointer(),gsCode->GetBufferSize(),&so,1,&stride,1,D3D11_SO_NO_RASTERIZED_STREAM,nullptr,&gs)),"SO geometry shader");gsCode->Release();
 const float positions[3][4]={{-.02f,.03f,0,1},{-.65f,.45f,0,1},{.7f,-.4f,0,1}};
 D3D11_BUFFER_DESC bd={sizeof(positions),D3D11_USAGE_DEFAULT,D3D11_BIND_VERTEX_BUFFER,0,0,0};D3D11_SUBRESOURCE_DATA sd={positions,0,0};ID3D11Buffer* vb=nullptr;Check(SUCCEEDED(dev->CreateBuffer(&bd,&sd,&vb)),"VB");
 bd.ByteWidth=16*4*sizeof(float);bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;ID3D11Buffer* cb=nullptr;Check(SUCCEEDED(dev->CreateBuffer(&bd,nullptr,&cb)),"CB");
 bd.ByteWidth=sizeof(positions);bd.BindFlags=D3D11_BIND_STREAM_OUTPUT;ID3D11Buffer* output=nullptr;Check(SUCCEEDED(dev->CreateBuffer(&bd,nullptr,&output)),"output");bd.BindFlags=0;bd.Usage=D3D11_USAGE_STAGING;bd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Buffer* stage=nullptr;Check(SUCCEEDED(dev->CreateBuffer(&bd,nullptr,&stage)),"readback");
 UINT zero=0;ctx->IASetInputLayout(layout);ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);ctx->IASetVertexBuffers(0,1,&vb,&stride,&zero);ctx->VSSetShader(vs,nullptr,0);ctx->VSSetConstantBuffers(13,1,&cb);ctx->GSSetShader(gs,nullptr,0);
 for(bool menu:{false,true}){
  float rows[16][4]={};rows[4][0]=.06f;rows[4][1]=-.03f;rows[5][0]=-.4f;rows[5][1]=-.5f;
  rows[14][0]=menu?rows[4][0]:.34f;rows[14][1]=menu?rows[4][1]:-.7f;rows[15][0]=menu?rows[5][0]:0;rows[15][1]=menu?rows[5][1]:0;
  ctx->UpdateSubresource(cb,0,nullptr,rows,0,0);ctx->SOSetTargets(1,&output,&zero);ctx->Draw(3,0);ID3D11Buffer* none=nullptr;ctx->SOSetTargets(1,&none,&zero);ctx->CopyResource(stage,output);D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(ctx->Map(stage,0,D3D11_MAP_READ,0,&mapped)),"readback map");
  const auto* got=(const float*)mapped.pData;
  for(int i=0;i<3;++i)for(int c=0;c<4;++c){const int offset=(!menu&&i==0)?14:4;const float expected=positions[i][c]*(1+rows[offset+1][c])+rows[offset][c];Check(std::fabs(got[i*4+c]-expected)<1e-6f,"actual patched VS positions match branch ownership");}
  if(!menu){Check(std::fabs(got[0]-(positions[0][0]*.6f+.06f))>.2f,"old map center vertex torn away from ordinary panel vertices");puts("PASS: old independent crosshair placement reproduces center-vertex map tear; gameplay aim branch retained");}
  else puts("PASS: native-menu identical crosshair/UI constants keep every map vertex on the same panel in patched VS WARP execution");
  ctx->Unmap(stage,0);
 }
 ctx->ClearState();stage->Release();output->Release();cb->Release();vb->Release();gs->Release();layout->Release();vs->Release();ctx->Release();dev->Release();return 0;
}
