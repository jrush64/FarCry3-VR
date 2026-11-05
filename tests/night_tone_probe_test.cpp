#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <cstdio>
#include <vector>
#include "../src/night_tone_probe.inl"
int main(){
 ID3D11Device* d=nullptr;ID3D11DeviceContext* c=nullptr;if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c)))return 1;
 std::vector<unsigned> pixels(128*96);for(unsigned y=0;y<96;++y)for(unsigned x=0;x<128;++x)pixels[y*128+x]=0xff000000|x|(y<<8);
 D3D11_TEXTURE2D_DESC td={};td.Width=128;td.Height=96;td.MipLevels=1;td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
 ID3D11Texture2D* texture=nullptr;D3D11_SUBRESOURCE_DATA data={pixels.data(),128*4,0};if(FAILED(d->CreateTexture2D(&td,&data,&texture)))return 2;
 ID3D11ShaderResourceView* srv=nullptr;d->CreateShaderResourceView(texture,nullptr,&srv);c->PSSetShaderResources(0,1,&srv);
 bool armed=fc3night::BeginActualDraw(c,0x6A620A60024D1B32ull,700,4,0,true);if(!armed||fc3night::count!=1)return 3;
 ID3D11ShaderResourceView* none=nullptr;c->PSSetShaderResources(0,1,&none);ID3D11RenderTargetView* rtv=nullptr;d->CreateRenderTargetView(texture,nullptr,&rtv);c->OMSetRenderTargets(1,&rtv,nullptr);
 fc3night::EndActualDraw(c,0x6A620A60024D1B32ull,700,4,0,armed);if(fc3night::count!=2)return 4;c->Flush();
 for(unsigned tries=0;tries<200;++tries){BOOL done=FALSE;if(c->GetData(fc3night::records[0].ready,&done,sizeof(done),0)==S_OK&&done)break;Sleep(1);}
 for(unsigned i=0;i<2;++i){auto& r=fc3night::records[i];D3D11_MAPPED_SUBRESOURCE m={};if(FAILED(c->Map(r.staging,0,D3D11_MAP_READ,0,&m)))return 5;
 for(unsigned p=0;p<5;++p){auto box=fc3night::PatchAt(128,96,p);for(unsigned y=0;y<box.h;++y)for(unsigned x=0;x<box.w;++x){unsigned actual=*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(m.pData)+y*m.RowPitch+(p*64+x)*4);if(actual!=pixels[(box.y+y)*128+box.x+x])return 6;}}c->Unmap(r.staging,0);
 if(r.resource!=reinterpret_cast<unsigned long long>(texture)||r.contextType!=D3D11_DEVICE_CONTEXT_IMMEDIATE||r.viewFormat!=28||r.frame!=700||r.draw!=4)return 7;
 }
 if(fc3night::BeginActualDraw(c,0x6A620A60024D1B32ull,701,5,0,true))return 8;
 fc3night::FlushOnPresent(c,L"build");for(unsigned i=0;i<2;++i)if(fc3night::records[i].staging)return 9;
 ID3D11DeviceContext* deferred=nullptr;if(FAILED(d->CreateDeferredContext(0,&deferred)))return 10;
 deferred->PSSetShaderResources(0,1,&srv);armed=fc3night::BeginActualDraw(deferred,0xC5D7AC9B41B47C00ull,700,8,1,true);
 deferred->PSSetShaderResources(0,1,&none);deferred->OMSetRenderTargets(1,&rtv,nullptr);fc3night::EndActualDraw(deferred,0xC5D7AC9B41B47C00ull,700,8,1,armed);
 if(fc3night::count!=4||fc3night::records[2].contextType!=D3D11_DEVICE_CONTEXT_DEFERRED)return 11;
 ID3D11CommandList* list=nullptr;if(FAILED(deferred->FinishCommandList(FALSE,&list)))return 12;
 fc3night::FlushOnPresent(c,L"build");if(!fc3night::records[2].staging)return 13; // cannot read before actual list execution
 c->ExecuteCommandList(list,FALSE);c->Flush();
 for(unsigned tries=0;tries<200&&fc3night::records[2].staging;++tries){fc3night::FlushOnPresent(c,L"build");Sleep(1);}
 if(fc3night::records[2].staging||fc3night::records[3].staging)return 14;
 std::vector<unsigned> lut(32*32*32);for(unsigned z=0;z<32;++z)for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x)lut[(z*32+y)*32+x]=0xff000000|x|(y<<8)|(z<<16);
 D3D11_TEXTURE3D_DESC vd={};vd.Width=vd.Height=vd.Depth=32;vd.MipLevels=1;vd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;vd.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA ld={lut.data(),32*4,32*32*4};ID3D11Texture3D* volume=nullptr;if(FAILED(d->CreateTexture3D(&vd,&ld,&volume)))return 15;
 ID3D11ShaderResourceView* lv=nullptr;d->CreateShaderResourceView(volume,nullptr,&lv);deferred->PSSetShaderResources(2,1,&lv);
 armed=fc3night::BeginActualDraw(deferred,0x6A620A60024D1B32ull,700,9,1,true);if(!armed||fc3night::count!=5||!fc3night::records[4].volume)return 16;
 deferred->FinishCommandList(FALSE,&list);fc3night::FlushOnPresent(c,L"build");if(!fc3night::records[4].staging)return 17;
 c->ExecuteCommandList(list,FALSE);c->Flush();BOOL ready=FALSE;for(unsigned n=0;n<200;++n){if(c->GetData(fc3night::records[4].ready,&ready,sizeof(ready),0)==S_OK&&ready)break;Sleep(1);}
 D3D11_MAPPED_SUBRESOURCE vm={};if(FAILED(c->Map(fc3night::records[4].staging,0,D3D11_MAP_READ,0,&vm)))return 18;
 for(unsigned z=0;z<32;++z)for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x){unsigned actual=*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(vm.pData)+z*vm.DepthPitch+y*vm.RowPitch+x*4);if(actual!=lut[(z*32+y)*32+x])return 19;}
 c->Unmap(fc3night::records[4].staging,0);fc3night::FlushOnPresent(c,L"build");if(fc3night::records[4].staging)return 20;
 FILE* file=nullptr;fopen_s(&file,"build/fc3_night_6A620A60024D1B32_f700_d9_e1_pre_s2.bin","rb");if(!file)return 21;std::vector<unsigned> read(lut.size());size_t got=fread(read.data(),4,read.size(),file);fclose(file);if(got!=lut.size()||read!=lut)return 22;
 if(fc3night::records[4].depth!=32||fc3night::records[4].slot!=2||fc3night::records[4].viewFormat!=28)return 23;
 std::puts("PASS: exact2D patches and32768 full3D LUT texels/filebytes; real deferred execution gates both captures; one-shot metadata preserved.");return 0;
}
