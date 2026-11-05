#include <d3dcompiler.h>
struct SelectorGpu291 {
 ID3D11Device* device=nullptr;ID3D11DeviceContext* deferred=nullptr;
 ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;ID3D11InputLayout* layout=nullptr;ID3D11Buffer* constants=nullptr;
 ID3D11RasterizerState* raster=nullptr;ID3D11DepthStencilState* depthState=nullptr;
 ID3D11Texture2D* depth=nullptr;ID3D11DepthStencilView* dsv=nullptr;
 XrSwapchain chain=XR_NULL_HANDLE;XrSession session=XR_NULL_HANDLE;
 ID3D11Texture2D* images[8]={};ID3D11RenderTargetView* targets[8][2]={};UINT count=0,width=0,height=0;
} g_selectGpu291;
void SelectorGpuRelease291(){auto& r=g_selectGpu291;for(auto& mesh:g_selectMesh291){if(mesh.buffer)mesh.buffer->Release();mesh.buffer=nullptr;}
 for(auto& pair:r.targets)for(auto& p:pair){if(p)p->Release();p=nullptr;}
 if(r.chain)p_xrDestroySwapchain(r.chain);
 if(r.deferred)r.deferred->Release();if(r.vs)r.vs->Release();if(r.ps)r.ps->Release();if(r.layout)r.layout->Release();if(r.constants)r.constants->Release();if(r.raster)r.raster->Release();if(r.depthState)r.depthState->Release();if(r.dsv)r.dsv->Release();if(r.depth)r.depth->Release();if(r.device)r.device->Release();r={};
}
bool SelectorGpuResources291(ID3D11Device* device){auto& r=g_selectGpu291;if(r.device&&(r.device!=device||r.session!=g_session))SelectorGpuRelease291();if(r.chain&&r.vs&&r.ps&&r.dsv)return true;
 if(!r.device){r.device=device;device->AddRef();r.session=g_session;}
 const char* code="cbuffer Model : register(b0) { float4 Rows[4]; float4 Colour; };struct V {float3 p:POSITION;float3 n:NORMAL;};struct O {float4 p:SV_POSITION;float3 n:TEXCOORD0;};O vs(V v){O o;float4 p=float4(v.p,1);o.p=float4(dot(Rows[0],p),dot(Rows[1],p),dot(Rows[2],p),dot(Rows[3],p));o.n=v.n;return o;}float4 ps(O o):SV_TARGET {float l=.28+.72*abs(dot(normalize(o.n),normalize(float3(.3,.8,1))));return float4(Colour.rgb*l,1);}";
 ID3DBlob *vs=nullptr,*ps=nullptr,*errors=nullptr;
 HRESULT hr=D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"vs","vs_4_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&vs,&errors);if(errors)errors->Release();errors=nullptr;
 if(SUCCEEDED(hr))hr=D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"ps","ps_4_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&ps,&errors);if(errors)errors->Release();
 if(FAILED(hr)){if(vs)vs->Release();if(ps)ps->Release();return false;}
 hr=device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&r.vs);if(SUCCEEDED(hr))hr=device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&r.ps);
 D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
 if(SUCCEEDED(hr))hr=device->CreateInputLayout(elements,2,vs->GetBufferPointer(),vs->GetBufferSize(),&r.layout);vs->Release();ps->Release();if(FAILED(hr))return false;
 D3D11_BUFFER_DESC cb={};cb.ByteWidth=80;cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(FAILED(device->CreateBuffer(&cb,nullptr,&r.constants)))return false;
 D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;if(FAILED(device->CreateRasterizerState(&rd,&r.raster)))return false;
 D3D11_DEPTH_STENCIL_DESC ds={};ds.DepthEnable=TRUE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;ds.DepthFunc=D3D11_COMPARISON_LESS;if(FAILED(device->CreateDepthStencilState(&ds,&r.depthState)))return false;
 if(FAILED(device->CreateDeferredContext(0,&r.deferred)))return false;
 r.width=(std::min)(1536u,g_width);r.height=(std::min)(1536u,g_height);if(!r.width||!r.height)return false;
 XrSwapchainCreateInfo ci={XR_TYPE_SWAPCHAIN_CREATE_INFO};ci.format=g_format;ci.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;ci.width=r.width;ci.height=r.height;ci.arraySize=2;ci.faceCount=ci.mipCount=ci.sampleCount=1;
 if(!XR_SUCCEEDED(p_xrCreateSwapchain(g_session,&ci,&r.chain)))return false;
 UINT count=0;p_xrEnumerateSwapchainImages(r.chain,0,&count,nullptr);if(!count||count>8)return false;
 XrSwapchainImageD3D11KHR images[8];for(UINT i=0;i<count;++i)images[i]={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};if(!XR_SUCCEEDED(p_xrEnumerateSwapchainImages(r.chain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images))))return false;
 r.count=count;for(UINT i=0;i<count;++i){r.images[i]=images[i].texture;for(UINT eye=0;eye<2;++eye){D3D11_RENDER_TARGET_VIEW_DESC view={};view.Format=g_format;view.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2DARRAY;view.Texture2DArray.FirstArraySlice=eye;view.Texture2DArray.ArraySize=1;if(FAILED(device->CreateRenderTargetView(images[i].texture,&view,&r.targets[i][eye])))return false;}}
 D3D11_TEXTURE2D_DESC td={};td.Width=r.width;td.Height=r.height;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;td.BindFlags=D3D11_BIND_DEPTH_STENCIL;
 if(FAILED(device->CreateTexture2D(&td,nullptr,&r.depth))||FAILED(device->CreateDepthStencilView(r.depth,nullptr,&r.dsv)))return false;
 Log("[SELECT291] actual3D stereo projection ready %ux%u, two array eyes",r.width,r.height);return true;
}
void SelectorMatrix291(int slot,int eye,float (&data)[20]){
 const auto q=testnotes::Multiply(testnotes::Inverse(g_views[eye].pose.orientation),g_selectPose290.orientation);
 const auto centre=fc3select290::Centre(slot);const auto world=testnotes::Add(g_selectPose290.position,testnotes::Rotate(g_selectPose290.orientation,{centre.x,centre.y,centre.z}));
 const auto t=testnotes::Rotate(testnotes::Inverse(g_views[eye].pose.orientation),testnotes::Sub(world,g_views[eye].pose.position));
 const auto x=testnotes::Rotate(q,{1,0,0}),y=testnotes::Rotate(q,{0,1,0}),z=testnotes::Rotate(q,{0,0,1});
 const float e[3][4]={{x.x,y.x,z.x,t.x},{x.y,y.y,z.y,t.y},{x.z,y.z,z.z,t.z}};
 const auto& f=g_views[eye].fov;const float l=std::tan(f.angleLeft),r=std::tan(f.angleRight),d=std::tan(f.angleDown),u=std::tan(f.angleUp);const float clipNear=.02f,clipFar=5;
 for(int i=0;i<4;++i){data[i]=(2*e[0][i]+(r+l)*e[2][i])/(r-l);data[4+i]=(2*e[1][i]+(u+d)*e[2][i])/(u-d);data[8+i]=-clipFar/(clipFar-clipNear)*e[2][i];data[12+i]=-e[2][i];}data[11]-=clipFar*clipNear/(clipFar-clipNear);
 const bool hover=g_selectGesture290.hover==slot;data[16]=hover?.12f:.70f;data[17]=hover?1.f:.74f;data[18]=hover?.35f:.78f;data[19]=1;
}
struct SelectorRenderRequest291 {ID3D11Device* device;XrCompositionLayerProjection* layer;XrCompositionLayerProjectionView* views;};
bool RenderSelectorOwned291(void* opaque){auto& request=*static_cast<SelectorRenderRequest291*>(opaque);if(!SelectorGpuResources291(request.device))return false;auto& r=g_selectGpu291;
 UINT image=0;XrSwapchainImageAcquireInfo ac={XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};if(!XR_SUCCEEDED(p_xrAcquireSwapchainImage(r.chain,&ac,&image)))return false;
 XrSwapchainImageWaitInfo wait={XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;const bool waited=XR_SUCCEEDED(p_xrWaitSwapchainImage(r.chain,&wait));if(!waited)return false;
 bool ok=image<r.count;auto* c=r.deferred;
 if(ok){c->ClearState();c->IASetInputLayout(r.layout);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);c->VSSetShader(r.vs,nullptr,0);c->PSSetShader(r.ps,nullptr,0);c->VSSetConstantBuffers(0,1,&r.constants);c->PSSetConstantBuffers(0,1,&r.constants);c->RSSetState(r.raster);c->OMSetDepthStencilState(r.depthState,0);D3D11_VIEWPORT vp={0,0,float(r.width),float(r.height),0,1};c->RSSetViewports(1,&vp);
  for(int eye=0;eye<2&&ok;++eye){const float clear[4]={};c->ClearRenderTargetView(r.targets[image][eye],clear);c->ClearDepthStencilView(r.dsv,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1,0);c->OMSetRenderTargets(1,&r.targets[image][eye],r.dsv);
   for(int slot=0;slot<4;++slot){auto& mesh=g_selectMesh291[slot];if(!mesh.valid||!g_selectShown290.weapons[slot].entity)continue;
    if(!mesh.buffer){D3D11_BUFFER_DESC desc={};desc.ByteWidth=UINT(mesh.vertices.size()*sizeof(fc3select291::Vertex));desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;D3D11_SUBRESOURCE_DATA initial={mesh.vertices.data(),0,0};if(FAILED(request.device->CreateBuffer(&desc,&initial,&mesh.buffer))){ok=false;break;}}
    float data[20];SelectorMatrix291(slot,eye,data);c->UpdateSubresource(r.constants,0,nullptr,data,0,0);const UINT stride=24,offset=0;c->IASetVertexBuffers(0,1,&mesh.buffer,&stride,&offset);c->Draw(UINT(mesh.vertices.size()),0);
   }
  }
  ID3D11CommandList* commands=nullptr;if(ok)ok=SUCCEEDED(c->FinishCommandList(FALSE,&commands));if(ok){ID3D11DeviceContext* immediate=nullptr;request.device->GetImmediateContext(&immediate);immediate->ExecuteCommandList(commands,TRUE);immediate->Release();commands->Release();}
 }
 XrSwapchainImageReleaseInfo release={XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};const bool released=XR_SUCCEEDED(p_xrReleaseSwapchainImage(r.chain,&release));if(!ok||!released)return false;
 for(int eye=0;eye<2;++eye){auto& view=request.views[eye];view={XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};view.pose=g_views[eye].pose;view.fov=g_views[eye].fov;view.subImage.swapchain=r.chain;view.subImage.imageArrayIndex=eye;view.subImage.imageRect.extent={int32_t(r.width),int32_t(r.height)};}
 *request.layer={XR_TYPE_COMPOSITION_LAYER_PROJECTION};request.layer->layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;request.layer->space=g_space;request.layer->viewCount=2;request.layer->views=request.views;return true;
}
bool SelectorLayer291(XrCompositionLayerProjection& layer,XrCompositionLayerProjectionView (&views)[2],ID3D11Texture2D* source){
 if(!g_selectGesture290.open||!g_viewsTracked||!g_referenceValid||fc3pad::MenuOpen())return false;
 ID3D11Device* device=nullptr;source->GetDevice(&device);SelectorRenderRequest291 request={device,&layer,views};const bool ok=fc3stereo::RunOverlay291(RenderSelectorOwned291,&request);device->Release();
 if(!ok){Log("[SELECT291]3D rendering failed; selector cancelled without an equip request");g_selectGesture290.Close();fc3select290::KeepAlive(false);fc3pad::SetSelector290(false);SelectorGpuRelease291();}
 return ok;
}
