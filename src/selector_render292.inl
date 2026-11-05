struct SelectorGpu292 {
 ID3D11Device* device=nullptr;ID3D11DeviceContext* deferred=nullptr;ID3D11Buffer* constants=nullptr;
 ID3D11PixelShader* outline=nullptr;ID3D11Buffer* outlineTint=nullptr;ID3D11DepthStencilState* outlineDepth=nullptr;
 ID3D11VertexShader* zoneVs304=nullptr;ID3D11InputLayout* zoneLayout304=nullptr;ID3D11Buffer* zoneVertices304=nullptr;
 ID3D11PixelShader* coverage=nullptr;ID3D11BlendState* alphaWrite=nullptr;
 ID3D11RasterizerState* raster=nullptr;ID3D11DepthStencilState *depthState=nullptr,*coverageDepth=nullptr;
 ID3D11Texture2D* depth=nullptr;ID3D11DepthStencilView* dsv=nullptr;
 ID3D11Texture2D* scratch[7]={};ID3D11RenderTargetView* scratchViews[7]={};
 XrSwapchain chain=XR_NULL_HANDLE;XrSession session=XR_NULL_HANDLE;
 ID3D11Texture2D* images[8]={};ID3D11RenderTargetView* targets[8][2]={};UINT count=0,width=0,height=0;
}g_selectGpu292;
void SelectorGpuRelease291(){auto& r=g_selectGpu292;
 for(auto& pair:r.targets)for(auto*& p:pair)fc3visual292::Drop(p);
 if(r.chain)p_xrDestroySwapchain(r.chain);for(auto*& p:r.scratchViews)fc3visual292::Drop(p);for(auto*& p:r.scratch)fc3visual292::Drop(p);
 fc3visual292::Drop(r.zoneVs304);fc3visual292::Drop(r.zoneLayout304);fc3visual292::Drop(r.zoneVertices304);
 fc3visual292::Drop(r.outline);fc3visual292::Drop(r.outlineTint);fc3visual292::Drop(r.outlineDepth);fc3visual292::Drop(r.deferred);fc3visual292::Drop(r.constants);fc3visual292::Drop(r.coverage);fc3visual292::Drop(r.alphaWrite);fc3visual292::Drop(r.raster);fc3visual292::Drop(r.depthState);fc3visual292::Drop(r.coverageDepth);fc3visual292::Drop(r.dsv);fc3visual292::Drop(r.depth);fc3visual292::Drop(r.device);r={};
}
bool SelectorGpuResources292(ID3D11Device* device){auto& r=g_selectGpu292;
 if(r.device&&(r.device!=device||r.session!=g_session))SelectorGpuRelease291();if(r.chain&&r.constants&&r.coverage&&r.dsv&&r.scratchViews[6])return true;
 if(!r.device){r.device=device;device->AddRef();r.session=g_session;}
 const char* code="float4 main():SV_TARGET0{return float4(0,0,0,1);}";ID3DBlob *shader=nullptr,*error=nullptr;
 HRESULT hr=D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"main","ps_4_0",0,0,&shader,&error);fc3visual292::Drop(error);
 if(FAILED(hr))return false;hr=device->CreatePixelShader(shader->GetBufferPointer(),shader->GetBufferSize(),nullptr,&r.coverage);fc3visual292::Drop(shader);if(FAILED(hr))return false;
 D3D11_BUFFER_DESC cb={};cb.ByteWidth=64;cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(FAILED(device->CreateBuffer(&cb,nullptr,&r.constants)))return false;
 D3D11_RASTERIZER_DESC raster={};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;if(FAILED(device->CreateRasterizerState(&raster,&r.raster)))return false;
 D3D11_DEPTH_STENCIL_DESC ds={};ds.DepthEnable=TRUE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;ds.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;if(FAILED(device->CreateDepthStencilState(&ds,&r.depthState)))return false;
 ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;ds.DepthFunc=D3D11_COMPARISON_EQUAL;if(FAILED(device->CreateDepthStencilState(&ds,&r.coverageDepth)))return false;
 ds.DepthEnable=FALSE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;if(FAILED(device->CreateDepthStencilState(&ds,&r.outlineDepth)))return false;
 const char* outlineCode="cbuffer Colour:register(b0){float4 colour;};float4 main():SV_TARGET0{return colour;}";
 hr=D3DCompile(outlineCode,strlen(outlineCode),nullptr,nullptr,nullptr,"main","ps_4_0",0,0,&shader,&error);fc3visual292::Drop(error);if(FAILED(hr))return false;hr=device->CreatePixelShader(shader->GetBufferPointer(),shader->GetBufferSize(),nullptr,&r.outline);fc3visual292::Drop(shader);if(FAILED(hr))return false;
 cb.ByteWidth=16;if(FAILED(device->CreateBuffer(&cb,nullptr,&r.outlineTint)))return false;
 D3D11_BLEND_DESC blend={};blend.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALPHA;if(FAILED(device->CreateBlendState(&blend,&r.alphaWrite)))return false;
 if(FAILED(device->CreateDeferredContext(0,&r.deferred)))return false;
 r.width=(std::min)(1536u,g_width);r.height=(std::min)(1536u,g_height);if(!r.width||!r.height)return false;
 XrSwapchainCreateInfo ci={XR_TYPE_SWAPCHAIN_CREATE_INFO};ci.format=g_format;ci.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;ci.width=r.width;ci.height=r.height;ci.arraySize=2;ci.faceCount=ci.mipCount=ci.sampleCount=1;
 if(!XR_SUCCEEDED(p_xrCreateSwapchain(g_session,&ci,&r.chain)))return false;
 UINT count=0;p_xrEnumerateSwapchainImages(r.chain,0,&count,nullptr);if(!count||count>8)return false;
 XrSwapchainImageD3D11KHR images[8];for(UINT i=0;i<count;++i)images[i]={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};if(!XR_SUCCEEDED(p_xrEnumerateSwapchainImages(r.chain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images))))return false;
 r.count=count;for(UINT i=0;i<count;++i){r.images[i]=images[i].texture;for(UINT eye=0;eye<2;++eye){D3D11_RENDER_TARGET_VIEW_DESC view={};view.Format=g_format;view.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2DARRAY;view.Texture2DArray.FirstArraySlice=eye;view.Texture2DArray.ArraySize=1;if(FAILED(device->CreateRenderTargetView(images[i].texture,&view,&r.targets[i][eye])))return false;}}
 D3D11_TEXTURE2D_DESC td={};td.Width=r.width;td.Height=r.height;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;td.BindFlags=D3D11_BIND_DEPTH_STENCIL;
 if(FAILED(device->CreateTexture2D(&td,nullptr,&r.depth))||FAILED(device->CreateDepthStencilView(r.depth,nullptr,&r.dsv)))return false;
 td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.BindFlags=D3D11_BIND_RENDER_TARGET;
 for(int i=0;i<7;++i)if(FAILED(device->CreateTexture2D(&td,nullptr,&r.scratch[i]))||FAILED(device->CreateRenderTargetView(r.scratch[i],nullptr,&r.scratchViews[i])))return false;
 Log("[SELECT295] native material stereo projection ready %ux%u",r.width,r.height);return true;
}
struct Matrix292 {float m[16]={};};
Matrix292 Multiply292(const Matrix292& a,const Matrix292& b){Matrix292 result;for(int r=0;r<4;++r)for(int c=0;c<4;++c)for(int k=0;k<4;++k)result.m[r*4+c]+=a.m[r*4+k]*b.m[k*4+c];return result;}
Matrix292 PosePlacement304(XrPosef pose,int eye,bool native=true){
 const auto q=testnotes::Multiply(testnotes::Inverse(g_views[eye].pose.orientation),pose.orientation);
 const auto t=testnotes::Rotate(testnotes::Inverse(g_views[eye].pose.orientation),testnotes::Sub(pose.position,g_views[eye].pose.position));
 // Wrist pose already converts native bind-local axes to XR. Local captured
 // vertices stay native; a second Z reflection here would mirror the weapon.
 const auto x=testnotes::Rotate(q,{1,0,0}),y=testnotes::Rotate(q,{0,1,0}),z=testnotes::Rotate(q,{0,0,1});
 const float scale=native?1.f/(std::max)(.1f,fc3stereo::WorldScale()):1.f;Matrix292 view;const float rows[16]={x.x*scale,y.x*scale,z.x*scale,t.x,x.y*scale,y.y*scale,z.y*scale,t.y,x.z*scale,y.z*scale,z.z*scale,t.z,0,0,0,1};memcpy(view.m,rows,64);
 const auto& f=g_views[eye].fov;const float l=std::tan(f.angleLeft),r=std::tan(f.angleRight),d=std::tan(f.angleDown),u=std::tan(f.angleUp),n=.02f,clipFar=8;
 Matrix292 projection;projection.m[0]=2/(r-l);projection.m[2]=(r+l)/(r-l);projection.m[5]=2/(u-d);projection.m[6]=(u+d)/(u-d);projection.m[10]=-clipFar/(clipFar-n);projection.m[11]=-clipFar*n/(clipFar-n);projection.m[14]=-1;
 return Multiply292(projection,view);
}
Matrix292 SelectorPlacement292(int slot,int eye){return PosePlacement304(g_pick292.slot==slot?g_drawHeld292:RackHandle292(slot),eye);}
bool ZoneResources304(ID3D11Device* device){auto& r=g_selectGpu292;if(r.zoneVertices304&&r.zoneVs304&&r.zoneLayout304)return true;
 const char* code="cbuffer Pose:register(b0){row_major float4x4 m;};float4 main(float3 p:POSITION):SV_POSITION{return mul(m,float4(p,1));}";ID3DBlob *blob=nullptr,*error=nullptr;
 HRESULT hr=D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"main","vs_4_0",0,0,&blob,&error);fc3visual292::Drop(error);if(FAILED(hr))return false;
 hr=device->CreateVertexShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&r.zoneVs304);const D3D11_INPUT_ELEMENT_DESC layout={"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};if(SUCCEEDED(hr))hr=device->CreateInputLayout(&layout,1,blob->GetBufferPointer(),blob->GetBufferSize(),&r.zoneLayout304);fc3visual292::Drop(blob);if(FAILED(hr))return false;
 D3D11_BUFFER_DESC desc={};desc.ByteWidth=4096*sizeof(XrVector3f);desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;return SUCCEEDED(device->CreateBuffer(&desc,nullptr,&r.zoneVertices304));
}
void DrawZone304(ID3D11DeviceContext* c,int slot,int eye,const float* tint){auto& r=g_selectGpu292;XrVector3f vertices[4096]={};UINT count=0;const float radius=g_holsters297.radius,half=fc3grab295::HalfLength;
 auto line=[&](XrVector3f a,XrVector3f b){vertices[count++]=a;vertices[count++]=b;};
 for(int j=0;j<32;++j){const float a=j*6.2831853f/32,b=(j+1)*6.2831853f/32;
 for(int end=-1;end<=1;end+=2)line({radius*cosf(a),end*half,radius*sinf(a)},{radius*cosf(b),end*half,radius*sinf(b)});
 for(int plane=0;plane<2;++plane){auto point=[&](float angle){const float x=radius*cosf(angle),y=radius*sinf(angle)+(sinf(angle)>=0?half:-half);return plane?XrVector3f{0,y,x}:XrVector3f{x,y,0};};line(point(a),point(b));}}
 const UINT stride=sizeof(XrVector3f),zero=0;c->UpdateSubresource(r.zoneVertices304,0,nullptr,vertices,0,0);auto pose=RackHandle292(slot);pose.position=Palm292(pose);const auto placement=PosePlacement304(pose,eye,false);c->UpdateSubresource(r.constants,0,nullptr,placement.m,0,0);c->UpdateSubresource(r.outlineTint,0,nullptr,tint,0,0);
 c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);c->IASetInputLayout(r.zoneLayout304);c->IASetVertexBuffers(0,1,&r.zoneVertices304,&stride,&zero);c->VSSetShader(r.zoneVs304,nullptr,0);c->PSSetShader(r.outline,nullptr,0);c->PSSetConstantBuffers(0,1,&r.outlineTint);c->OMSetDepthStencilState(r.outlineDepth,0);c->OMSetBlendState(nullptr,nullptr,~0u);c->Draw(count,0);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
struct SelectorRenderRequest292 {ID3D11Device* device;XrCompositionLayerProjection* layer;XrCompositionLayerProjectionView* views;};
bool RenderSelectorOwned292(void* opaque){const auto handState303=fc3hands303::Read();auto& request=*static_cast<SelectorRenderRequest292*>(opaque);if(!SelectorGpuResources292(request.device))return false;auto& r=g_selectGpu292;
 UINT image=0;XrSwapchainImageAcquireInfo ac={XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};if(!XR_SUCCEEDED(p_xrAcquireSwapchainImage(r.chain,&ac,&image)))return false;
 XrSwapchainImageWaitInfo wait={XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;const bool waited=XR_SUCCEEDED(p_xrWaitSwapchainImage(r.chain,&wait));
 bool ok=waited&&image<r.count;auto* c=r.deferred;
 if(ok){c->ClearState();c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);c->VSSetConstantBuffers(0,1,&r.constants);c->RSSetState(r.raster);D3D11_VIEWPORT vp={0,0,float(r.width),float(r.height),0,1};c->RSSetViewports(1,&vp);
  for(int eye=0;eye<2&&ok;++eye){const float clear[4]={};c->ClearRenderTargetView(r.targets[image][eye],clear);c->ClearDepthStencilView(r.dsv,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1,0);ID3D11RenderTargetView* targets[8]={r.targets[image][eye]};for(int i=0;i<7;++i)targets[i+1]=r.scratchViews[i];c->OMSetRenderTargets(8,targets,r.dsv);
   const bool body304=g_holsters297.mode!=fc3holster297::Front;
   const bool ghost304=body304&&g_holsters297.stowOutline&&handState303.enabled&&handState303.engaged&&handState303.active&&!handState303.empty&&handState303.slot>=0&&handState303.slot<4;
   if(g_zonesVisible304){if(!ZoneResources304(request.device)){ok=false;break;}const float colours[4][4]={{.1f,.6f,1,1},{1,.35f,.15f,1},{.8f,.2f,1,1},{.15f,1,.6f,1}};for(int i=0;i<4;++i)if(g_holsters297.mode!=fc3holster297::Single||i==1)DrawZone304(c,i,eye,colours[i]);}
   if(ghost304&&g_pick292.slot<0){if(!ZoneResources304(request.device)){ok=false;break;}const float gold[4]={.75f,.47f,.04f,.75f},green[4]={.1f,1,.25f,1};DrawZone304(c,handState303.slot,eye,g_stowReady304?green:gold);}
   for(int slot=0;slot<4;++slot){const bool stow304=ghost304&&slot==handState303.slot;
   if(g_pick292.slot<0&&!stow304&&!fc3hands303::HeldVisible(handState303,g_selectShown290.equipped,g_selectShown290.weapons[slot].entity))continue;if(g_pick292.slot>=0&&g_pick292.slot!=slot)continue;if(g_holsters297.mode==fc3holster297::Single&&slot!=g_singleSlot297&&!stow304)continue;const auto* visual=fc3visual292::Read(g_selectShown290,slot);if(!visual)continue;
    const auto placement=SelectorPlacement292(slot,eye);
    if(stow304||g_pick292.slot==slot||g_selectGesture290.hover==slot){
     const float cyan[4]={0,.75f,1,1},green[4]={.1f,1,.25f,1};const float gold304[4]={.75f,.47f,.04f,.75f};c->UpdateSubresource(r.outlineTint,0,nullptr,stow304?(g_stowReady304?green:gold304):(g_pick292.slot==slot?green:cyan),0,0);c->PSSetConstantBuffers(0,1,&r.outlineTint);c->PSSetShader(r.outline,nullptr,0);c->OMSetDepthStencilState(r.outlineDepth,0);c->OMSetBlendState(nullptr,nullptr,~0u);
     const float offsets[8][2]={{-1,0},{1,0},{0,-1},{0,1},{-.707f,-.707f},{.707f,-.707f},{-.707f,.707f},{.707f,.707f}};
     for(const auto& part:visual->parts){if(part->hand)continue;Matrix292 local;memcpy(local.m,part->clipToGrip,64);const auto original=Multiply292(placement,local);c->IASetInputLayout(part->layout);const UINT zero=0;c->IASetVertexBuffers(0,1,&part->vertices,&part->stride,&zero);c->VSSetShader(part->place,nullptr,0);
      for(const auto& offset:offsets){auto expanded=original;for(int col=0;col<4;++col){expanded.m[col]+=offset[0]*6.f/r.width*original.m[12+col];expanded.m[4+col]+=offset[1]*6.f/r.height*original.m[12+col];}c->UpdateSubresource(r.constants,0,nullptr,expanded.m,0,0);c->Draw(part->count,0);}
     }
    }
    if(stow304){const float fill304[4]={g_stowReady304?.015f:.06f,g_stowReady304?.08f:.035f,.005f,.10f};c->UpdateSubresource(r.outlineTint,0,nullptr,fill304,0,0);c->PSSetShader(r.outline,nullptr,0);c->PSSetConstantBuffers(0,1,&r.outlineTint);c->OMSetDepthStencilState(r.outlineDepth,0);
     for(const auto& part:visual->parts){if(part->hand)continue;Matrix292 local;memcpy(local.m,part->clipToGrip,64);const auto matrix=Multiply292(placement,local);c->UpdateSubresource(r.constants,0,nullptr,matrix.m,0,0);c->IASetInputLayout(part->layout);const UINT zero=0;c->IASetVertexBuffers(0,1,&part->vertices,&part->stride,&zero);c->VSSetShader(part->place,nullptr,0);c->Draw(part->count,0);}continue;}
    for(const auto& part:visual->parts){if(part->hand&&handState303.enabled&&handState303.engaged)continue;if(part->hand&&g_pick292.slot!=slot&&!(g_selectGesture290.hover==slot&&g_selectorGrip292>=.75f))continue;if(!part->ready){ok=false;break;}Matrix292 source;memcpy(source.m,part->clipToGrip,64);const auto matrix=Multiply292(placement,source);
     c->UpdateSubresource(r.constants,0,nullptr,matrix.m,0,0);c->IASetInputLayout(part->layout);const UINT zero=0;c->IASetVertexBuffers(0,1,&part->vertices,&part->stride,&zero);c->VSSetShader(part->place,nullptr,0);
     c->PSSetConstantBuffers(0,14,part->constants);c->PSSetShaderResources(0,128,part->textures);c->PSSetSamplers(0,16,part->samplers);
     c->OMSetDepthStencilState(r.depthState,0);c->OMSetBlendState(nullptr,nullptr,~0u);c->PSSetShader(part->material,nullptr,0);c->Draw(part->count,0);
     // Native G-buffer alpha is material data. Write coverage separately so it
     // never makes the actual material transparent in the XR composition layer.
     c->OMSetDepthStencilState(r.coverageDepth,0);c->OMSetBlendState(r.alphaWrite,nullptr,~0u);c->PSSetShader(r.coverage,nullptr,0);c->Draw(part->count,0);
    }
   }
  }
  ID3D11CommandList* commands=nullptr;if(ok)ok=SUCCEEDED(c->FinishCommandList(FALSE,&commands));if(ok){ID3D11DeviceContext* immediate=nullptr;request.device->GetImmediateContext(&immediate);immediate->ExecuteCommandList(commands,TRUE);immediate->Release();commands->Release();}
 }
 XrSwapchainImageReleaseInfo release={XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};const bool released=waited&&XR_SUCCEEDED(p_xrReleaseSwapchainImage(r.chain,&release));if(!ok||!released)return false;
 for(int eye=0;eye<2;++eye){auto& view=request.views[eye];view={XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};view.pose=g_views[eye].pose;view.fov=g_views[eye].fov;view.subImage.swapchain=r.chain;view.subImage.imageArrayIndex=eye;view.subImage.imageRect.extent={int32_t(r.width),int32_t(r.height)};}
 *request.layer={XR_TYPE_COMPOSITION_LAYER_PROJECTION};request.layer->layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;request.layer->space=g_space;request.layer->viewCount=2;request.layer->views=request.views;return true;
}
bool SelectorLayer291(XrCompositionLayerProjection& layer,XrCompositionLayerProjectionView (&views)[2],ID3D11Texture2D* source){
 const auto held304=fc3hands303::Read();const bool ghost304=g_holsters297.mode!=fc3holster297::Front&&g_holsters297.stowOutline&&held304.enabled&&held304.engaged&&held304.active&&!held304.empty;
 if((!g_selectGesture290.open&&!g_bodyVisible297&&!g_zonesVisible304&&!ghost304)||(!g_warm292.done&&!g_zonesVisible304)||!g_viewsTracked||!g_referenceValid||(fc3pad::MenuOpen()&&!g_zonesVisible304))return false;
 ID3D11Device* device=nullptr;source->GetDevice(&device);SelectorRenderRequest292 request={device,&layer,views};const bool ok=fc3stereo::RunOverlay291(RenderSelectorOwned292,&request);device->Release();
 if(ok&&g_bodyRenderFailed297&&g_holsters297.mode!=fc3holster297::Front){g_bodyRenderFailed297=false;g_bodyReady297=false;g_bodyGripWas297=true;if(g_hands303.empty){g_hands303.wasPaused=true;g_hands303.ready[0]=g_hands303.ready[1]=false;}Log("[HOLSTER358] successful two-eye native overlay restored draw availability; neutral grip required; inventory/equip readback gates retained");}
 if(!ok){if(g_holsters297.mode!=fc3holster297::Front){g_bodyRenderFailed297=true;g_bodyVisible297=g_bodyReady297=g_singleHeld297=false;g_pick292.Cancel();}Log("[SELECT295] native rendering failed; cancelled without invisible selection");g_selectGesture290.Close();fc3select290::KeepAlive(false);fc3pad::SetSelector290(false);fc3visual292::ClearContact();SelectorGpuRelease291();}return ok;
}
