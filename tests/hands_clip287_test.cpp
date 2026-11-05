#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../src/dxbc_patch.cpp"
void Check(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
ID3DBlob* Compile(const char* text,const char* target){ID3DBlob* b=nullptr,*error=nullptr;const auto hr=D3DCompile(text,strlen(text),nullptr,nullptr,nullptr,"main",target,D3DCOMPILE_SKIP_OPTIMIZATION,0,&b,&error);if(FAILED(hr)&&error)fprintf(stderr,"%s\n",static_cast<const char*>(error->GetBufferPointer()));if(error)error->Release();Check(SUCCEEDED(hr),"test shader compile");return b;}
int main(int argc,char** argv){
 wchar_t system[MAX_PATH];GetSystemDirectoryW(system,MAX_PATH);wcscat_s(system,L"\\d3d11.dll");auto create=reinterpret_cast<decltype(&D3D11CreateDevice)>(GetProcAddress(LoadLibraryW(system),"D3D11CreateDevice"));
 ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;Check(SUCCEEDED(create(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"WARP device");
 if(argc==2){
  FILE* f=nullptr;fopen_s(&f,argv[1],"rb");Check(f!=nullptr,"captured native arm shader available");fseek(f,0,SEEK_END);const long size=ftell(f);rewind(f);Check(size>0&&size<65536,"bounded native shader capture");std::vector<uint8_t> native(size);Check(fread(native.data(),1,size,f)==size_t(size),"read native shader");fclose(f);
  std::vector<uint8_t> stereo,clipped;Check(dxbc::PatchArmVertexShader(native.data(),native.size(),stereo)==dxbc::Result::Patched,"actual captured FC3 arm shader accepts baseline stereo patch");
  Check(dxbc::PatchHandsOnly287(stereo.data(),stereo.size(),clipped)==dxbc::Result::Patched&&dxbc::VerifyChecksum(clipped.data(),clipped.size()),"actual native shader accepts additional independent clip plane");
  ID3D11VertexShader* shader=nullptr;Check(SUCCEEDED(device->CreateVertexShader(clipped.data(),clipped.size(),nullptr,&shader)),"GPU accepts patched actual FC3 arm shader with native plane preserved");shader->Release();
  ID3DBlob* disassembly=nullptr;if(SUCCEEDED(D3DDisassemble(clipped.data(),clipped.size(),0,nullptr,&disassembly))){fopen_s(&f,"evidence/native-arm-patched287.asm","wb");if(f){fwrite(disassembly->GetBufferPointer(),1,disassembly->GetBufferSize(),f);fclose(f);}disassembly->Release();}
  puts("PASS:287 actual captured native FC3 arm shader, retained native clip plane, independent wrist plane and GPU shader creation");context->Release();device->Release();return 0;
 }
 auto* original=Compile("struct O{float4 position:SV_Position;float nativeClip:SV_ClipDistance0;};O main(int4 p:POSITION){O o;o.position=float4(p.y/16384.0,p.z/16384.0,.5,1);o.nativeClip=p.w<1?1:-1;return o;}","vs_4_0");
 std::vector<uint8_t> stereo,clipped;Check(dxbc::PatchArmVertexShader(original->GetBufferPointer(),original->GetBufferSize(),stereo)==dxbc::Result::Patched,"existing arm stereo patch");
 Check(dxbc::PatchHandsOnly287(stereo.data(),stereo.size(),clipped)==dxbc::Result::Patched&&dxbc::VerifyChecksum(clipped.data(),clipped.size()),"hands clip patch and valid container checksum");
 ID3D11VertexShader* vs=nullptr;Check(SUCCEEDED(device->CreateVertexShader(clipped.data(),clipped.size(),nullptr,&vs)),"GPU accepts clip-distance shader");
 ID3DBlob* disassembly=nullptr;if(SUCCEEDED(D3DDisassemble(clipped.data(),clipped.size(),0,nullptr,&disassembly))){FILE* f=nullptr;fopen_s(&f,"evidence/hands-clip287.asm","wb");if(f){fwrite(disassembly->GetBufferPointer(),1,disassembly->GetBufferSize(),f);fclose(f);}disassembly->Release();}
 std::vector<uint8_t> twice;Check(dxbc::PatchHandsOnly287(clipped.data(),clipped.size(),twice)==dxbc::Result::Unsupported,"duplicate clip patch is refused");
 auto* psCode=Compile("float4 main():SV_Target{return float4(1,1,1,1);}","ps_4_0");ID3D11PixelShader* ps=nullptr;Check(SUCCEEDED(device->CreatePixelShader(psCode->GetBufferPointer(),psCode->GetBufferSize(),nullptr,&ps)),"pixel shader");
 D3D11_INPUT_ELEMENT_DESC element={"POSITION",0,DXGI_FORMAT_R16G16B16A16_SINT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};ID3D11InputLayout* layout=nullptr;Check(SUCCEEDED(device->CreateInputLayout(&element,1,original->GetBufferPointer(),original->GetBufferSize(),&layout)),"native integer position input layout");
 short vertices[12][4];const float corners[6][2]={{-.9f,-.8f},{-.1f,-.8f},{-.9f,.8f},{-.1f,-.8f},{-.1f,.8f},{-.9f,.8f}};
 for(int half=0;half<2;++half)for(int n=0;n<6;++n){auto& v=vertices[half*6+n];v[0]=short((half?.65f:.45f)*16384);v[1]=short((corners[n][0]+half)*16384);v[2]=short(corners[n][1]*16384);v[3]=0;}
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(vertices);bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;D3D11_SUBRESOURCE_DATA initial={vertices,0,0};ID3D11Buffer* vb=nullptr;Check(SUCCEEDED(device->CreateBuffer(&bd,&initial,&vb)),"test vertices");
 float controls[4]={1,.57f*.57f,0,0};bd.ByteWidth=16;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;initial.pSysMem=controls;ID3D11Buffer* on=nullptr;Check(SUCCEEDED(device->CreateBuffer(&bd,&initial,&on)),"immutable hands control");
 D3D11_TEXTURE2D_DESC td={};td.Width=32;td.Height=16;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_RENDER_TARGET;ID3D11Texture2D* target=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&target)),"target");ID3D11RenderTargetView* rtv=nullptr;device->CreateRenderTargetView(target,nullptr,&rtv);
 td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* readback=nullptr;device->CreateTexture2D(&td,nullptr,&readback);
 D3D11_RASTERIZER_DESC raster={};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;ID3D11RasterizerState* rs=nullptr;device->CreateRasterizerState(&raster,&rs);
 auto draw=[&](ID3D11DeviceContext* c,ID3D11Buffer* control){const float clear[4]={};c->ClearRenderTargetView(rtv,clear);c->OMSetRenderTargets(1,&rtv,nullptr);D3D11_VIEWPORT vp={0,0,32,16,0,1};c->RSSetViewports(1,&vp);c->RSSetState(rs);c->VSSetShader(vs,nullptr,0);c->PSSetShader(ps,nullptr,0);c->IASetInputLayout(layout);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);UINT stride=8,offset=0;c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->VSSetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&control);c->Draw(12,0);};
 int readIndex=0;auto pixels=[&](int& left,int& right){++readIndex;context->CopyResource(readback,target);D3D11_MAPPED_SUBRESOURCE map={};const auto hr=context->Map(readback,0,D3D11_MAP_READ,0,&map);if(FAILED(hr))fprintf(stderr,"pixel read%d: %08X device %08X\n",readIndex,unsigned(hr),unsigned(device->GetDeviceRemovedReason()));Check(SUCCEEDED(hr),"read final pixels");left=right=0;for(int y=0;y<16;++y)for(int x=0;x<32;++x)if(static_cast<const unsigned char*>(map.pData)[y*map.RowPitch+x*4]>128)(x<16?left:right)++;context->Unmap(readback,0);};
 int left=0,right=0;draw(context,nullptr);pixels(left,right);Check(left>100&&right>100,"unbound control preserves full arm and hand pixels");
 draw(context,on);pixels(left,right);Check(left==0&&right>100,"enabled clip removes forearm pixels and preserves hand pixels");
 ID3D11DeviceContext* deferred=nullptr;device->CreateDeferredContext(0,&deferred);draw(deferred,on);ID3D11CommandList* list=nullptr;deferred->FinishCommandList(FALSE,&list);ID3D11Buffer* off=nullptr;context->VSSetConstantBuffers(dxbc::kHandsClipCbSlot287,1,&off); // immediate bindings cannot replace recorded controls
 context->ExecuteCommandList(list,TRUE);pixels(left,right);Check(left==0&&right>100,"deferred command list retains the clip buffer used by its own draw");
 for(int n=6;n<12;++n)vertices[n][3]=1;vb->Release();vb=nullptr;bd.ByteWidth=sizeof(vertices);bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;initial.pSysMem=vertices;Check(SUCCEEDED(device->CreateBuffer(&bd,&initial,&vb)),"test native clip plane geometry");
 draw(context,nullptr);pixels(left,right);Check(left>100&&right==0,"disabled wrist clip preserves native clip plane");
 draw(context,on);pixels(left,right);Check(left==0&&right==0,"enabled wrist plane and original native plane both remain active");
 puts("PASS:287 GPU clip-distance shader, unchanged disabled pixels, clipped forearm/preserved hand and deferred control ownership");
 context->ClearState();list->Release();deferred->Release();rs->Release();readback->Release();rtv->Release();target->Release();on->Release();vb->Release();layout->Release();ps->Release();psCode->Release();vs->Release();original->Release();context->Release();device->Release();
}
