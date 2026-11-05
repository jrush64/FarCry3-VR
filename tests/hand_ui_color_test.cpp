// Audit the production CopyTexture path: a raw GPU copy from format 28 to 29.
// This is not an OpenXR runtime or scene-lighting test.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cmath>
#include <cstring>

int main() {
 ID3D11Device* d=nullptr; ID3D11DeviceContext* c=nullptr;
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c))) return 1;
 unsigned pixels[256]; for(unsigned i=0;i<256;++i)pixels[i]=0xff000000u|(i<<16)|(i<<8)|i;
 D3D11_TEXTURE2D_DESC desc={};desc.Width=256;desc.Height=1;desc.MipLevels=1;desc.ArraySize=1;desc.SampleDesc.Count=1;
 desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA initial={pixels,1024,0};ID3D11Texture2D *source=nullptr,*eye=nullptr,*readback=nullptr;
 if(FAILED(d->CreateTexture2D(&desc,&initial,&source))) return 2;
 desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
 if(FAILED(d->CreateTexture2D(&desc,nullptr,&eye)))return 3;
 c->CopyResource(eye,source); // identical operation to fc3stereo::CopyTexture
 desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 if(FAILED(d->CreateTexture2D(&desc,nullptr,&readback)))return 4;
 c->CopyResource(readback,eye);D3D11_MAPPED_SUBRESOURCE mapped={};
 if(FAILED(c->Map(readback,0,D3D11_MAP_READ,0,&mapped)))return 5;
 bool equal=std::memcmp(mapped.pData,pixels,sizeof(pixels))==0;c->Unmap(readback,0);
 if(!equal){std::puts("FAIL: UNORM -> SRGB GPU copy changed encoded bytes");return 6;}
 const char* shader="Texture2D<float4> input:register(t0);RWStructuredBuffer<float4> output:register(u0);[numthreads(256,1,1)]void main(uint3 i:SV_DispatchThreadID){output[i.x]=input.Load(int3(i.x,0,0));}";
 ID3DBlob* compiled=nullptr;ID3DBlob* error=nullptr;
 if(FAILED(D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"main","cs_5_0",0,0,&compiled,&error)))return 7;
 ID3D11ComputeShader* cs=nullptr;d->CreateComputeShader(compiled->GetBufferPointer(),compiled->GetBufferSize(),nullptr,&cs);
 ID3D11ShaderResourceView* srv=nullptr;if(FAILED(d->CreateShaderResourceView(eye,nullptr,&srv)))return 8;
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=256*16;bd.BindFlags=D3D11_BIND_UNORDERED_ACCESS;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=16;
 ID3D11Buffer *output=nullptr,*staging=nullptr;d->CreateBuffer(&bd,nullptr,&output);
 D3D11_UNORDERED_ACCESS_VIEW_DESC ud={};ud.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;ud.Buffer.NumElements=256;
 ID3D11UnorderedAccessView* uav=nullptr;if(FAILED(d->CreateUnorderedAccessView(output,&ud,&uav)))return 9;
 c->CSSetShader(cs,nullptr,0);c->CSSetShaderResources(0,1,&srv);c->CSSetUnorderedAccessViews(0,1,&uav,nullptr);c->Dispatch(1,1,1);
 bd.BindFlags=0;bd.MiscFlags=0;bd.Usage=D3D11_USAGE_STAGING;bd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;d->CreateBuffer(&bd,nullptr,&staging);c->CopyResource(staging,output);
 if(FAILED(c->Map(staging,0,D3D11_MAP_READ,0,&mapped)))return 10;
 float worst=0;const float* values=static_cast<const float*>(mapped.pData);
 for(unsigned i=0;i<256;++i){float encoded=i/255.0f;float expected=encoded<=.04045f?encoded/12.92f:std::pow((encoded+.055f)/1.055f,2.4f);for(unsigned channel=0;channel<3;++channel)worst=fmaxf(worst,fabsf(values[4*i+channel]-expected));}
 c->Unmap(staging,0);
 if(worst>.001f){std::printf("FAIL: compositor-style sRGB decode error %.9f\n",worst);return 11;}
 std::printf("PASS: all 256 encoded ramp bytes preserved; 768 sRGB decoded channels match, worst error %.9f\n",worst);
 std::puts("Hand UI source UNORM and XR SRGB destination preserve encoded bytes with one decode.");
 return 0;
}
