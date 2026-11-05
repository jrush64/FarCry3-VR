// The insertion guide is geometry in the rendered eye images. Capture the
// FRAME draw's actual constants; no OpenXR reference-space reconstruction.
struct NativeGuideEye48 {PistolFrameAnchor45 anchor={};float camera[224]={},eye[16]={};LONG frame=-1;bool valid=false;};
NativeGuideEye48 g_nativeGuideEye48[2];
struct NativeGuideVertices48 {float clip[2][24][4]={};LONG frame=-1;UINT epoch=0;bool ready=false;}g_nativeGuideVertices48;
void Capture1911Guide48(ID3D11DeviceContext* context,UINT start,UINT count,ID3D11Buffer* world,int variant,int eye){
 if(eye<0||eye>1||variant!=1||start!=18||count!=276||g_weaponMechanicProfile!=20)return;
 const auto& a=g_1911FrameFinal;if(!a.valid||a.frame!=g_frame||a.epoch!=g_armPoseEpoch||a.context!=context||a.buffer!=world)return;
 auto& out=g_nativeGuideEye48[eye];if(out.valid&&out.frame==g_frame)return;
 ID3D11Buffer* camera=nullptr,*stereo=nullptr;context->VSGetConstantBuffers(1,1,&camera);context->VSGetConstantBuffers(dxbc::kEyeCbSlot,1,&stereo);
 float native[224]={},rows[dxbc::kEyeRows*4]={};const UINT nc=ReadCopy(camera,native,sizeof(native)),ne=ReadCopy(stereo,rows,sizeof(rows));SafeRelease(camera);SafeRelease(stereo);
 if(nc!=sizeof(native)||ne!=sizeof(rows)){static LONG logs=0;if(logs++<12)Log("[GUIDE48] FRAME guide capture missing actual camera/eye shadow (%u/%u bytes)",nc,ne);return;}
 for(int i=0;i<16;++i)if(!std::isfinite(native[i])||!std::isfinite(rows[i]))return;for(int i:{108,109,110,111,152,153,154})if(!std::isfinite(native[i]))return;
 out.anchor=a;memcpy(out.camera,native,sizeof(native));for(int i=0;i<16;++i)out.eye[i]=rows[i]+(i%5==0?1.f:0.f);out.frame=g_frame;out.valid=true;
}
bool Native1911Clip48(const NativeGuideEye48& draw,V3 local,float* clip){
 const V3 world=Apply(draw.anchor.world,local);double p[4]={world.x-double(draw.camera[152])-draw.camera[108],world.y-double(draw.camera[153])-draw.camera[109],world.z-double(draw.camera[154])-draw.camera[110],1};
 const double length=std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);if(!std::isfinite(length)||length<1e-8)return false;
 for(int i=0;i<3;++i)p[i]+=p[i]/length*draw.camera[111]; // native C386 camera-relative depth offset
 double native[4]={};for(int r=0;r<4;++r)for(int c=0;c<4;++c)native[r]+=draw.camera[r*4+c]*p[c];
 for(int r=0;r<4;++r){double f=0;for(int c=0;c<4;++c)f+=draw.eye[r*4+c]*native[c];clip[r]=float(f);if(!std::isfinite(clip[r]))return false;}return true;
}
void Native1911GuideRequest48(const Mat4&,int target){
 g_nativeGuideVertices48.ready=false;if(target!=0)return;
 const int edges[24]={0,1,2,3,4,5,6,7,0,2,1,3,4,6,5,7,0,4,1,5,2,6,3,7};
 for(int eye=0;eye<2;++eye){const auto& draw=g_nativeGuideEye48[eye];if(!draw.valid||draw.frame>g_frame||(!g_pistolMenu.open&&draw.frame!=g_frame)||draw.anchor.epoch!=g_armPoseEpoch)return;
  if((draw.anchor.weapon&&g_pistolReload.weapon&&draw.anchor.weapon!=g_pistolReload.weapon)||(draw.anchor.wrapper&&g_pistolReload.wrapper&&draw.anchor.wrapper!=g_pistolReload.wrapper))return;
  const Mat4 box=PistolGuideBox(draw.anchor.body,0);Mat4 inverse={};if(!Invert(draw.anchor.body,inverse))return;const Mat4 localBox=Multiply(inverse,box);
  V3 corners[8];for(int i=0;i<8;++i){const double scale=(std::max)(.1f,g_worldScale);corners[i]=Apply(localBox,{g_pistolGuides.size[0][0]*scale*.5*(i&1?1:-1),g_pistolGuides.size[0][1]*scale*.5*(i&2?1:-1),g_pistolGuides.size[0][2]*scale*.5*(i&4?1:-1)});}
  for(int i=0;i<24;++i)if(!Native1911Clip48(draw,corners[edges[i]],g_nativeGuideVertices48.clip[eye][i]))return;
 }
 g_nativeGuideVertices48.frame=g_frame;g_nativeGuideVertices48.epoch=g_armPoseEpoch;g_nativeGuideVertices48.ready=true;
}
ID3D11Device* g_nativeGuideDevice48=nullptr;ID3D11VertexShader* g_nativeGuideVs48=nullptr;ID3D11PixelShader* g_nativeGuidePs48=nullptr;ID3D11PixelShader* g_nativeGuideColourPs50[2]={};ID3D11PixelShader* g_laserColourPs205[8]={};ID3D11InputLayout* g_nativeGuideLayout48=nullptr;ID3D11Buffer* g_nativeGuideVb48=nullptr;ID3D11RasterizerState* g_nativeGuideRs48=nullptr;ID3D11DepthStencilState* g_nativeGuideDs48=nullptr;
void ClearNative1911Resources48(){SafeRelease(g_nativeGuideVs48);SafeRelease(g_nativeGuidePs48);for(auto*& ps:g_nativeGuideColourPs50)SafeRelease(ps);for(auto*& ps:g_laserColourPs205)SafeRelease(ps);SafeRelease(g_nativeGuideLayout48);SafeRelease(g_nativeGuideVb48);SafeRelease(g_nativeGuideRs48);SafeRelease(g_nativeGuideDs48);g_nativeGuideDevice48=nullptr;}
ID3DBlob* Native1911Compile48(const char* source,const char* target){
 using Compile=HRESULT (WINAPI*)(LPCVOID,SIZE_T,LPCSTR,const void*,void*,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob**,ID3DBlob**);
 HMODULE module=LoadLibraryW(L"d3dcompiler_47.dll");auto compile=module?reinterpret_cast<Compile>(GetProcAddress(module,"D3DCompile")):nullptr;ID3DBlob* code=nullptr,*error=nullptr;
 if(compile)compile(source,strlen(source),nullptr,nullptr,nullptr,"main",target,0,0,&code,&error);SafeRelease(error);if(module)FreeLibrary(module);return code;
}
bool Native1911Resources48(ID3D11Device* device){
 if(g_nativeGuideDevice48!=device)ClearNative1911Resources48();if(g_nativeGuideVs48)return true;
 auto* vs=Native1911Compile48("float4 main(float4 p:POSITION):SV_Position{return p;}","vs_5_0");auto* ps=Native1911Compile48("float4 main():SV_Target{return float4(.06,.35,1,1);}","ps_5_0");if(!vs||!ps){SafeRelease(vs);SafeRelease(ps);return false;}
 D3D11_INPUT_ELEMENT_DESC input={"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};D3D11_BUFFER_DESC vb={};vb.ByteWidth=192*16;vb.BindFlags=D3D11_BIND_VERTEX_BUFFER;
 D3D11_RASTERIZER_DESC rs={};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=true;rs.AntialiasedLineEnable=true;D3D11_DEPTH_STENCIL_DESC ds={};ds.DepthEnable=false;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
 const bool ok=SUCCEEDED(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&g_nativeGuideVs48))&&SUCCEEDED(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&g_nativeGuidePs48))&&SUCCEEDED(device->CreateInputLayout(&input,1,vs->GetBufferPointer(),vs->GetBufferSize(),&g_nativeGuideLayout48))&&SUCCEEDED(device->CreateBuffer(&vb,nullptr,&g_nativeGuideVb48))&&SUCCEEDED(device->CreateRasterizerState(&rs,&g_nativeGuideRs48))&&SUCCEEDED(device->CreateDepthStencilState(&ds,&g_nativeGuideDs48));SafeRelease(vs);SafeRelease(ps);if(!ok){ClearNative1911Resources48();return false;}const char* colours[]={"float4 main():SV_Target{return float4(1,.8,.06,1);}","float4 main():SV_Target{return float4(1,.06,.03,1);}"};for(int i=0;i<2;++i){auto* code=Native1911Compile48(colours[i],"ps_5_0");const bool created=code&&SUCCEEDED(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&g_nativeGuideColourPs50[i]));SafeRelease(code);if(!created){ClearNative1911Resources48();return false;}}g_nativeGuideDevice48=device;return true;
}
void DrawNative1911Guide48(ID3D11DeviceContext* c,ID3D11Texture2D* texture,const float* vertices,UINT vertexCount=24,int colour=0,ID3D11DepthStencilView* laserDepth=nullptr,ID3D11DepthStencilState* laserState=nullptr,float minDepth=0,float maxDepth=1){
 if(!c||!texture||!vertices||!vertexCount||vertexCount>192||(vertexCount&1)||colour<0||colour>10||(laserDepth&&!laserState))return;ID3D11Device* device=nullptr;c->GetDevice(&device);if(!Native1911Resources48(device)){SafeRelease(device);return;}
 if(colour>=3&&!g_laserColourPs205[colour-3]){const ImU32 value=kReticleColours52[colour-3].colour;char source[128];sprintf_s(source,"float4 main():SV_Target{return float4(%f,%f,%f,1);}",float(value&255)/255.f,float((value>>8)&255)/255.f,float((value>>16)&255)/255.f);auto* code=Native1911Compile48(source,"ps_5_0");if(code){device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&g_laserColourPs205[colour-3]);SafeRelease(code);}if(!g_laserColourPs205[colour-3]){SafeRelease(device);return;}}
 ID3D11RenderTargetView* target=nullptr;if(FAILED(device->CreateRenderTargetView(texture,nullptr,&target))){SafeRelease(device);return;}SafeRelease(device);
 const bool own=g_own;g_own=true;
 ID3D11RenderTargetView* rt[8]={};ID3D11DepthStencilView* depth=nullptr;c->OMGetRenderTargets(8,rt,&depth);
 ID3D11BlendState* blend=nullptr;float factor[4];UINT mask=0;c->OMGetBlendState(&blend,factor,&mask);ID3D11DepthStencilState* ds=nullptr;UINT stencil=0;c->OMGetDepthStencilState(&ds,&stencil);ID3D11RasterizerState* rs=nullptr;c->RSGetState(&rs);D3D11_VIEWPORT viewport[16];UINT count=16;c->RSGetViewports(&count,viewport);
 ID3D11InputLayout* layout=nullptr;ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;D3D11_PRIMITIVE_TOPOLOGY topology;c->IAGetInputLayout(&layout);c->IAGetVertexBuffers(0,1,&vb,&stride,&offset);c->IAGetPrimitiveTopology(&topology);
 #define GUIDE_SAVE_SHADER48(stage,type) type* saved##stage=nullptr;ID3D11ClassInstance* instances##stage[256]={};UINT n##stage=256;c->stage##GetShader(&saved##stage,instances##stage,&n##stage)
 GUIDE_SAVE_SHADER48(VS,ID3D11VertexShader);GUIDE_SAVE_SHADER48(PS,ID3D11PixelShader);GUIDE_SAVE_SHADER48(GS,ID3D11GeometryShader);GUIDE_SAVE_SHADER48(HS,ID3D11HullShader);GUIDE_SAVE_SHADER48(DS,ID3D11DomainShader);
 c->VSSetShader(g_nativeGuideVs48,nullptr,0);c->PSSetShader(colour>=3?g_laserColourPs205[colour-3]:(colour?g_nativeGuideColourPs50[colour-1]:g_nativeGuidePs48),nullptr,0);c->GSSetShader(nullptr,nullptr,0);c->HSSetShader(nullptr,nullptr,0);c->DSSetShader(nullptr,nullptr,0);c->IASetInputLayout(g_nativeGuideLayout48);UINT size=16,zero=0;c->IASetVertexBuffers(0,1,&g_nativeGuideVb48,&size,&zero);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);c->OMSetRenderTargets(1,&target,laserDepth);c->OMSetBlendState(nullptr,nullptr,~0u);c->OMSetDepthStencilState(laserState?laserState:g_nativeGuideDs48,0);c->RSSetState(g_nativeGuideRs48);
 D3D11_TEXTURE2D_DESC td={};texture->GetDesc(&td);D3D11_VIEWPORT vp={0,0,float(td.Width),float(td.Height),minDepth,maxDepth};c->RSSetViewports(1,&vp);D3D11_BOX upload={0,0,0,vertexCount*16,1,1};c->UpdateSubresource(g_nativeGuideVb48,0,&upload,vertices,0,0);if(g_oDraw)g_oDraw(c,vertexCount,0);else c->Draw(vertexCount,0);
 #define GUIDE_RESTORE_SHADER48(stage) c->stage##SetShader(saved##stage,instances##stage,n##stage);SafeRelease(saved##stage);for(UINT i=0;i<n##stage;++i)SafeRelease(instances##stage[i])
 GUIDE_RESTORE_SHADER48(VS);GUIDE_RESTORE_SHADER48(PS);GUIDE_RESTORE_SHADER48(GS);GUIDE_RESTORE_SHADER48(HS);GUIDE_RESTORE_SHADER48(DS);
 #undef GUIDE_SAVE_SHADER48
 #undef GUIDE_RESTORE_SHADER48
 c->IASetInputLayout(layout);c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->IASetPrimitiveTopology(topology);c->RSSetViewports(count,viewport);c->RSSetState(rs);c->OMSetBlendState(blend,factor,mask);c->OMSetDepthStencilState(ds,stencil);c->OMSetRenderTargets(8,rt,depth);
 SafeRelease(layout);SafeRelease(vb);SafeRelease(rs);SafeRelease(blend);SafeRelease(ds);SafeRelease(depth);for(auto*& r:rt)SafeRelease(r);SafeRelease(target);g_own=own;
}
void RenderNative1911Guide48(ID3D11Texture2D* left,ID3D11Texture2D* right){
 if(!g_nativeGuideVertices48.ready||g_nativeGuideVertices48.frame!=g_frame||g_nativeGuideVertices48.epoch!=g_armPoseEpoch||g_weaponMechanicProfile!=20||!g_showWeaponReloadGuides||!PhysicalReloadGuidesEnabled()||!g_pistolGuides.box||fc3pad::NativeMenuOpen()||!left||!right)return;
 DrawNative1911Guide48(g_ctx,left,&g_nativeGuideVertices48.clip[0][0][0]);DrawNative1911Guide48(g_ctx,right,&g_nativeGuideVertices48.clip[1][0][0]);
 static LONG logs=0;if(logs++<12)Log("[GUIDE48] insertion box geometry submitted to both game-eye images, frame %ld",g_frame);
}
